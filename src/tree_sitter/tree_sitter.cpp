#include "tree_sitter/tree_sitter.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <tree_sitter/api.h>
#include <utility>
#include <vector>

namespace ts {

// helper function to print a vector of printable objects
template <typename T>
static std::ostream& _print_vector(std::ostream& o, const std::vector<T>& items) {
    o << "[ ";

    const char* sep = "";

    for (const auto& item : items) {
        o << sep << item;
        sep = ", ";
    }

    o << " ]";
    return o;
}

static std::string _query_error(TSQueryError error) {
    switch (error) {
    case TSQueryErrorSyntax:
        return "syntax";
    case TSQueryErrorNodeType:
        return "node type";
    case TSQueryErrorField:
        return "field";
    case TSQueryErrorCapture:
        return "capture";
    case TSQueryErrorNone:
    default:
        return "unknown";
    }
}

// class ParserLanguageException
const char* ParserLanguageException::what() const noexcept {
    return "failed to set language on tree-sitter parser";
}

// class ParseFailureException
const char* ParseFailureException::what() const noexcept { return "failed to parse"; }

// class NullNodeException
const char* NullNodeException::what() const noexcept { return "can't create a null node"; }

// class QueryError
QueryException::QueryException(TSQueryError error, std::uint32_t error_offset)
    : std::runtime_error(
          "failed to create query: " + _query_error(error) + " error at position " +
          std::to_string(error_offset)),
      error_(error), error_offset_(error_offset) {}

TSQueryError QueryException::query_error() const { return this->error_; }
std::uint32_t QueryException::error_offset() const { return this->error_offset_; }

// class MultilineEditException
MultilineEditException::MultilineEditException()
    : std::runtime_error("multiline edits are not supported") {}

// class OverlppingEditException
OverlappingEditException::OverlappingEditException()
    : std::runtime_error("overlapping edits are not allowed") {}

// class ZeroSizedEditException
ZeroSizedEditException::ZeroSizedEditException()
    : std::runtime_error("zero-sized edits are not allowed") {}

// struct Point
bool operator==(const Point& lhs, const Point& rhs) {
    return lhs.row == rhs.row && lhs.column == rhs.column;
}
bool operator!=(const Point& lhs, const Point& rhs) { return !(lhs == rhs); }
bool operator<(const Point& lhs, const Point& rhs) {
    return lhs.row < rhs.row || (lhs.row == rhs.row && lhs.column < rhs.column);
}
bool operator<=(const Point& lhs, const Point& rhs) {
    return lhs.row < rhs.row || (lhs.row == rhs.row && lhs.column <= rhs.column);
}
bool operator>(const Point& lhs, const Point& rhs) {
    return lhs.row > rhs.row || (lhs.row == rhs.row && lhs.column > rhs.column);
}
bool operator>=(const Point& lhs, const Point& rhs) {
    return lhs.row > rhs.row || (lhs.row == rhs.row && lhs.column >= rhs.column);
}
std::ostream& operator<<(std::ostream& o, const Point& self) {
    return o << "Point{ .row = " << self.row << ", .column = " << self.column << "}";
}

// struct Location
bool operator==(const Location& lhs, const Location& rhs) {
    return lhs.point == rhs.point && lhs.byte == rhs.byte;
}
bool operator!=(const Location& lhs, const Location& rhs) { return !(lhs == rhs); }
bool operator<(const Location& lhs, const Location& rhs) { return lhs.byte < rhs.byte; }
bool operator<=(const Location& lhs, const Location& rhs) { return lhs.byte <= rhs.byte; }
bool operator>(const Location& lhs, const Location& rhs) { return lhs.byte > rhs.byte; }
bool operator>=(const Location& lhs, const Location& rhs) { return lhs.byte >= rhs.byte; }
std::ostream& operator<<(std::ostream& o, const Location& self) {
    return o << "Location{ .point = " << self.point << ", .byte = " << self.byte << "}";
}

// struct Range
bool Range::overlaps(const Range& other) const {
    if (this->start > other.start) {
        return other.overlaps(*this);
    }

    // from here on: this->start <= other.start

    // ranges can be arranged like this:
    //
    // 1)
    // ----
    //   ----
    // 2)
    // ------
    //   ----
    // 3)
    // ------
    //   ---
    // 4)
    // ------
    // ---
    // 5)
    // ----
    //       ----

    return this->end > other.start;
}
bool operator==(const Range& lhs, const Range& rhs) {
    return lhs.start == rhs.start && lhs.end == rhs.end;
}
bool operator!=(const Range& lhs, const Range& rhs) { return !(lhs == rhs); }
std::ostream& operator<<(std::ostream& o, const Range& self) {
    return o << "Range{ .start = " << self.start << ", .end = " << self.end << "}";
}
std::ostream& operator<<(std::ostream& o, const std::vector<Range>& ranges) {
    return _print_vector(o, ranges);
}

// struct Edit
bool operator==(const Edit& lhs, const Edit& rhs) {
    return lhs.range == rhs.range && lhs.replacement == rhs.replacement;
}
bool operator!=(const Edit& lhs, const Edit& rhs) { return !(lhs == rhs); }
std::ostream& operator<<(std::ostream& o, const Edit& self) {
    return o << "Edit{ .range = " << self.range << ", .replacement = " << self.replacement << "}";
}
std::ostream& operator<<(std::ostream& o, const std::vector<Edit>& edits) {
    return _print_vector(o, edits);
}

// class Language
Language::Language(const TSLanguage* lang) noexcept : lang(lang) {}

const TSLanguage* Language::raw() const { return this->lang; }

std::uint32_t Language::node_type_count() const { return ts_language_symbol_count(this->raw()); }

const char* Language::node_type_name(TypeId type_id) const {
    return ts_language_symbol_name(this->raw(), type_id);
}

TypeId Language::node_type_id(std::string_view name, bool is_named) const {
    return ts_language_symbol_for_name(this->raw(), name.data(), name.size(), is_named);
}

std::uint32_t Language::field_count() const { return ts_language_field_count(this->raw()); }

const char* Language::field_name(FieldId field_id) const {
    return ts_language_field_name_for_id(this->raw(), field_id);
}

FieldId Language::field_id(std::string_view name) const {
    return ts_language_field_id_for_name(this->raw(), name.data(), name.size());
}

TypeKind Language::node_type_kind(TypeId type_id) const {
    switch (ts_language_symbol_type(this->raw(), type_id)) {
    case TSSymbolTypeRegular:
        return TypeKind::Named;
    case TSSymbolTypeAnonymous:
        return TypeKind::Anonymous;
    case TSSymbolTypeAuxiliary:
        return TypeKind::Hidden;
    }
}

std::uint32_t Language::version() const { return ts_language_version(this->raw()); }

bool language_compatible(const Language& lang) {
    return lang.version() >= TREE_SITTER_MIN_VERSION && lang.version() <= TREE_SITTER_VERSION;
}

// class Node
Node::Node(Node::unsafe_t, TSNode node, const Tree& tree) noexcept : node(node), tree_(&tree) {}
Node::Node(TSNode node, const Tree& tree) : Node(Node::unsafe, node, tree) {
    if (ts_node_is_null(node)) {
        throw NullNodeException();
    }
}

std::optional<Node> Node::or_null(TSNode node, const Tree& tree) noexcept {
    if (ts_node_is_null(node)) {
        return std::nullopt;
    } else {
        return Node(Node::unsafe, node, tree);
    }
}

TSNode Node::raw() const { return this->node; }
const Tree& Node::tree() const { return *this->tree_; }

bool Node::is_named() const { return ts_node_is_named(this->node); }
bool Node::is_missing() const { return ts_node_is_missing(this->node); }
bool Node::is_extra() const { return ts_node_is_extra(this->node); }
bool Node::has_changes() const { return ts_node_has_changes(this->node); }
bool Node::has_error() const { return ts_node_has_error(this->node); }

const char* Node::type() const { return ts_node_type(this->node); }
TypeId Node::type_id() const { return ts_node_symbol(this->node); }

std::optional<Node> Node::parent() const {
    return Node::or_null(ts_node_parent(this->node), this->tree());
}

std::uint32_t Node::child_count() const { return ts_node_child_count(this->node); }

std::optional<Node> Node::child(std::uint32_t index) const {
    return Node::or_null(ts_node_child(this->node, index), this->tree());
}

std::vector<Node> Node::children() const {
    std::uint32_t num_children = this->child_count();

    std::vector<Node> children;
    children.reserve(num_children);
    for (std::uint32_t index = 0; index < num_children; ++index) {
        // we know how many children there are so dereferencing the optional is fine
        children.push_back(*this->child(index));
    }

    return children;
}

std::uint32_t Node::named_child_count() const { return ts_node_named_child_count(this->node); }

std::optional<Node> Node::named_child(std::uint32_t index) const {
    return Node::or_null(ts_node_named_child(this->node, index), this->tree());
}

std::vector<Node> Node::named_children() const {
    std::uint32_t num_children = this->named_child_count();

    std::vector<Node> children;
    children.reserve(num_children);
    for (std::uint32_t index = 0; index < num_children; ++index) {
        // we know how many children there are so dereferencing the optional is fine
        children.push_back(*this->named_child(index));
    }

    return children;
}

std::optional<Node> Node::next_sibling() const {
    return Node::or_null(ts_node_next_sibling(this->node), this->tree());
}
std::optional<Node> Node::prev_sibling() const {
    return Node::or_null(ts_node_prev_sibling(this->node), this->tree());
}
std::optional<Node> Node::next_named_sibling() const {
    return Node::or_null(ts_node_next_named_sibling(this->node), this->tree());
}
std::optional<Node> Node::prev_named_sibling() const {
    return Node::or_null(ts_node_prev_named_sibling(this->node), this->tree());
}

std::uint32_t Node::start_byte() const { return ts_node_start_byte(this->node); }
std::uint32_t Node::end_byte() const { return ts_node_end_byte(this->node); }

Point Node::start_point() const {
    TSPoint point = ts_node_start_point(this->node);
    return {.row = point.row, .column = point.column};
}
Point Node::end_point() const {
    TSPoint point = ts_node_end_point(this->node);
    return {.row = point.row, .column = point.column};
}

Location Node::start() const {
    return Location{
        .point = this->start_point(),
        .byte = this->start_byte(),
    };
}
Location Node::end() const {
    return Location{
        .point = this->end_point(),
        .byte = this->end_byte(),
    };
}

Range Node::range() const {
    return Range{
        .start = this->start(),
        .end = this->end(),
    };
}

std::string Node::text() const {
    auto start = this->start_byte();
    auto count = this->end_byte() - start;
    return this->tree().source().substr(start, count);
}

std::string Node::as_s_expr() const {
    std::unique_ptr<char, decltype(&free)> raw_string{ts_node_string(this->node), free};
    if (raw_string == nullptr) {
        return std::string();
    } else {
        return std::string(raw_string.get());
    }
}

bool operator==(const Node& lhs, const Node& rhs) {
    // was created from the same tree and nodes are equal
    return ts_node_eq(lhs.raw(), rhs.raw());
}
bool operator!=(const Node& lhs, const Node& rhs) { return !(lhs == rhs); }

std::ostream& operator<<(std::ostream& os, const Node& node) { return os << node.as_s_expr(); }

std::ostream& operator<<(std::ostream& o, const std::optional<Node>& node) {
    o << "optional<";
    if (node) {
        o << *node;
    }
    return o << ">";
}

// struct AppliedEdit
bool operator==(const AppliedEdit& self, const AppliedEdit& other) {
    return self.before == other.before && self.after == other.after &&
           self.old_source == other.old_source && self.replacement == other.replacement;
}
bool operator!=(const AppliedEdit& self, const AppliedEdit& other) { return !(self == other); }
std::ostream& operator<<(std::ostream& o, const AppliedEdit& self) {
    return o << "AppliedEdit { .before = " << self.before << ", .after = " << self.after
             << ", .old_source = \"" << self.old_source << "\", .replacement = \""
             << self.replacement << "\" }";
}
std::ostream& operator<<(std::ostream& o, const std::vector<AppliedEdit>& items) {
    return _print_vector(o, items);
}

// struct EditResult
bool operator==(const EditResult& self, const EditResult& other) {
    return self.applied_edits == other.applied_edits && self.changed_ranges == other.changed_ranges;
}
bool operator!=(const EditResult& self, const EditResult& other) { return !(self == other); }
std::ostream& operator<<(std::ostream& o, const EditResult& self) {
    return o << "EditResult { .applied_edits = " << self.applied_edits
             << ", .changed_ranges = " << self.changed_ranges << " }";
}

// class Tree
Tree::Tree(TSTree* tree, std::string source, Parser& parser)
    : tree(tree, ts_tree_delete), source_(std::move(source)), parser(&parser) {}

Tree::Tree(const Tree& other)
    : tree(ts_tree_copy(other.raw()), ts_tree_delete), source_(other.source()),
      parser(other.parser) {}
Tree& Tree::operator=(const Tree& other) {
    Tree copy{other};
    swap(copy, *this);
    return *this;
}

void swap(Tree& self, Tree& other) noexcept {
    using std::swap;
    swap(self.tree, other.tree);
    swap(self.source_, other.source_);
    swap(self.parser, other.parser);
}

const TSTree* Tree::raw() const { return this->tree.get(); }

const std::string& Tree::source() const { return this->source_; }

Node Tree::root_node() const { return Node(Node::unsafe, ts_tree_root_node(this->raw()), *this); }

Language Tree::language() const { return Language(ts_tree_language(this->raw())); }

// helper function to apply one edit to the tree and source code
static AppliedEdit _apply_edit(const Edit& edit, TSTree* tree, std::string& source) {
    long old_size = edit.range.end.byte - edit.range.start.byte;

    std::string old_source = source.substr(edit.range.start.byte, old_size);

    source.replace(edit.range.start.byte, old_size, edit.replacement);

    long end_byte_diff = static_cast<long>(edit.replacement.size()) - old_size;

    Range before = edit.range;
    Range after{
        .start = edit.range.start,
        .end = {
            .point =
                {.row = edit.range.end.point.row,
                 .column = static_cast<std::uint32_t>(before.end.point.column + end_byte_diff)},
            .byte = static_cast<std::uint32_t>(before.end.byte + end_byte_diff),
        }};

    TSInputEdit input_edit{
        .start_byte = before.start.byte,
        .old_end_byte = before.end.byte,
        .new_end_byte = after.end.byte,
        .start_point =
            TSPoint{
                .row = before.start.point.row,
                .column = before.start.point.column,
            },
        .old_end_point =
            TSPoint{
                .row = before.end.point.row,
                .column = before.end.point.column,
            },
        .new_end_point =
            TSPoint{
                .row = after.end.point.row,
                .column = after.end.point.column,
            },
    };

    ts_tree_edit(tree, &input_edit);

    return AppliedEdit{
        .before = before,
        .after = after,
        .old_source = old_source,
        .replacement = edit.replacement,
    };
}

static inline Point _point(const TSPoint& point) {
    return Point{
        .row = point.row,
        .column = point.column,
    };
}

static inline Location _location(const TSPoint& point, const std::uint32_t byte) {
    return Location{
        .point = _point(point),
        .byte = byte,
    };
}

static inline Range _range(const TSRange& range) {
    return Range{
        .start = _location(range.start_point, range.start_byte),
        .end = _location(range.end_point, range.end_byte),
    };
}

static std::vector<Range> _get_changed_ranges(const TSTree* old_tree, const TSTree* new_tree) {
    // this will always be set by ts_tree_get_changed_ranges
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    std::uint32_t length;

    std::unique_ptr<TSRange, decltype(&free)> ranges{
        ts_tree_get_changed_ranges(old_tree, new_tree, &length), free};

    if (length == 0) {
        return {};
    }

    const TSRange* begin = ranges.get();
    // we have no other choise because we get the point + length from a c api
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const TSRange* end = begin + static_cast<std::size_t>(length);

    std::vector<Range> changed_ranges{length};

    std::transform(
        begin, end, changed_ranges.begin(), [](const TSRange& range) { return _range(range); });

    return changed_ranges;
}

void _check_edits(const std::vector<Edit>& edits) {
    // NOTE: assumes that the ranges are already sorted by edit.range.start.byte

    for (int i = 0; i < edits.size(); ++i) {
        const auto& edit1 = edits[i];

        if (edit1.range.start == edit1.range.end) {
            throw ZeroSizedEditException();
        }

        if (edit1.range.start.point.row != edit1.range.end.point.row ||
            edit1.replacement.find('\n') != std::string::npos) {
            throw MultilineEditException();
        }

        for (int j = 0; j < edits.size(); ++j) {
            if (i == j) {
                continue;
            }

            const auto& edit2 = edits[j];

            if (edit1.range.overlaps(edit2.range)) {
                throw OverlappingEditException();
            }
        }
    }
}

EditResult Tree::edit(std::vector<Edit> edits) {
    // save copies of the previous values so we can return the changed ranges
    std::string new_source = this->source();
    std::unique_ptr<TSTree, void (*)(TSTree*)> old_tree = std::move(this->tree);

    // sorts the from the earliest in the source code to the latest in the source
    // code.
    // this is done so the locations for edits in the same line can be adjusted
    // so we can return the ranges of the edit before and after
    std::sort(edits.begin(), edits.end(), [](const Edit& edit1, const Edit& edit2) {
        return edit1.range.start.byte <= edit2.range.start.byte;
    });

    // NOTE: this throws exceptions if there is something wrong with the edits
    _check_edits(edits);

    std::vector<AppliedEdit> applied_edits;
    applied_edits.reserve(edits.size());

    // used to adjust subsequent edits
    Point last_point{};
    int last_width_change = 0;
    int comulative_byte_change = 0;

    for (auto& edit : edits) {
        Range range_before_adjustments = edit.range;
        // if the last edit was in the current line we need to adjust the
        // location of the next edit
        if (last_width_change != 0 && last_point.row == edit.range.start.point.row) {
            edit.range.start.point.column += last_width_change;
            edit.range.end.point.column += last_width_change;
        }

        edit.range.start.byte += comulative_byte_change;
        edit.range.end.byte += comulative_byte_change;

        AppliedEdit applied_edit = _apply_edit(edit, old_tree.get(), new_source);
        applied_edit.before = range_before_adjustments;
        applied_edits.push_back(applied_edit);

        int before_width =
            applied_edit.before.end.point.column - applied_edit.before.start.point.column;
        int after_width =
            applied_edit.after.end.point.column - applied_edit.after.start.point.column;
        last_width_change = after_width - before_width;

        int byte_change = applied_edit.after.end.byte - applied_edit.before.end.byte;
        comulative_byte_change += byte_change;
        last_point = edit.range.end.point;
    }

    // reparse the source code
    Tree new_tree = this->parser->parse_string(old_tree.get(), std::move(new_source));

    // update this tree
    swap(*this, new_tree);

    std::vector<Range> changed_ranges = _get_changed_ranges(old_tree.get(), this->raw());

    return EditResult{
        .changed_ranges = changed_ranges,
        .applied_edits = applied_edits,
    };
}

void Tree::print_dot_graph(std::string_view file) const {
    std::unique_ptr<std::FILE, decltype(&fclose)> f{std::fopen(file.data(), "w"), fclose};
    ts_tree_print_dot_graph(this->raw(), f.get());
}

// class Cursor
Cursor::Cursor(Node node) noexcept : cursor(ts_tree_cursor_new(node.raw())), tree(&node.tree()) {}
Cursor::Cursor(const Tree& tree) noexcept : Cursor(tree.root_node()) {}
Cursor::~Cursor() noexcept { ts_tree_cursor_delete(&this->cursor); }
Cursor::Cursor(const Cursor& cursor) noexcept
    : cursor(ts_tree_cursor_copy(&cursor.cursor)), tree(cursor.tree) {}

Cursor& Cursor::operator=(const Cursor& other) noexcept {
    Cursor copy{other};
    swap(copy, *this);
    return *this;
}

void swap(Cursor& self, Cursor& other) noexcept {
    using std::swap;
    swap(self.cursor, other.cursor);
    swap(self.tree, other.tree);
}

void Cursor::reset(Node node) { ts_tree_cursor_reset(&this->cursor, node.raw()); }
void Cursor::reset(const Tree& tree) {
    ts_tree_cursor_reset(&this->cursor, tree.root_node().raw());
}

Node Cursor::current_node() const {
    return Node(ts_tree_cursor_current_node(&this->cursor), *this->tree);
}
const char* Cursor::current_field_name() const {
    return ts_tree_cursor_current_field_name(&this->cursor);
}
FieldId Cursor::current_field_id() const { return ts_tree_cursor_current_field_id(&this->cursor); }

bool Cursor::goto_parent() { return ts_tree_cursor_goto_parent(&this->cursor); }
bool Cursor::goto_first_child() { return ts_tree_cursor_goto_first_child(&this->cursor); }
bool Cursor::goto_next_sibling() { return ts_tree_cursor_goto_next_sibling(&this->cursor); }
bool Cursor::goto_first_named_child() {
    if (!this->goto_first_child()) {
        return false;
    }

    // walk over siblings until we encounter a named node
    while (!this->current_node().is_named()) {
        if (!this->goto_next_sibling()) {
            return false;
        }
    }
    return true;
}
bool Cursor::goto_next_named_sibling() {
    // walk over siblings until we encounter a named node
    do {
        if (!this->goto_next_sibling()) {
            return false;
        }
    } while (!this->current_node().is_named());
    return true;
}

std::vector<Node> Cursor::children() {
    std::vector<Node> children;

    if (!this->goto_first_child()) {
        return children;
    }

    children.push_back(this->current_node());

    while (this->goto_next_sibling()) {
        children.push_back(this->current_node());
    }

    return children;
}

std::vector<Node> Cursor::named_children() {
    std::vector<Node> children;

    if (!this->goto_first_named_child()) {
        return children;
    }

    children.push_back(this->current_node());

    while (this->goto_next_named_sibling()) {
        children.push_back(this->current_node());
    }

    return children;
}

// class Parser
Parser::Parser() : Parser(LUA_LANGUAGE) {}
Parser::Parser(const Language& lang) : parser(ts_parser_new(), ts_parser_delete) {
    if (!ts_parser_set_language(this->parser.get(), lang.raw())) {
        // only occurs when version of lua parser and tree-sitter library
        // are incompatible
        // see: ts_language_version, TREE_SITTER_LANGUAGE_VERSION,
        // TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION
        throw ParserLanguageException();
    }
}

TSParser* Parser::raw() const { return this->parser.get(); }

Language Parser::language() const { return Language(ts_parser_language(this->raw())); }

Tree Parser::parse_string(const TSTree* old_tree, std::string source) {
    TSTree* tree = ts_parser_parse_string(this->raw(), old_tree, source.c_str(), source.length());
    if (tree == nullptr) {
        // TL;DR this should never happen
        // This can occur when:
        // - there is no language set (should not happen because we manage that)
        // - or the timeout was reached (see ts_parser_set_timeout_micros)
        // - or the parsing was cancelled using ts_parser_set_cancellation_flag
        // In the latter two cases the parser could be restarted by calling it
        // with the same arguments. But these cases also can't happen because
        // we don't allow setting those flags.
        throw ParseFailureException();
    }
    return Tree(tree, std::move(source), *this);
}
Tree Parser::parse_string(std::string str) { return parse_string(nullptr, std::move(str)); }

// class Query
static TSQuery* _make_query(std::string_view source) {
    std::uint32_t error_offset;
    TSQueryError error_type;
    TSQuery* query = ts_query_new(
        LUA_LANGUAGE.raw(), source.data(), source.length(), &error_offset, &error_type);

    if (query == nullptr) {
        throw QueryException(error_type, error_offset);
    }

    return query;
}

Query::Query(std::string_view source) : query(_make_query(source), ts_query_delete) {}

void swap(Query& self, Query& other) noexcept { std::swap(self.query, other.query); }

const TSQuery* Query::raw() const { return this->query.get(); }
TSQuery* Query::raw() { return this->query.get(); }

std::uint32_t Query::pattern_count() const { return ts_query_pattern_count(this->raw()); }
std::uint32_t Query::capture_count() const { return ts_query_capture_count(this->raw()); }
std::uint32_t Query::string_count() const { return ts_query_string_count(this->raw()); }

std::uint32_t Query::start_byte_for_pattern(std::uint32_t id) const {
    return ts_query_start_byte_for_pattern(this->raw(), id);
}

std::string_view Query::capture_name_for_id(std::uint32_t id) const {
    std::uint32_t length = 0;
    const char* name = ts_query_capture_name_for_id(this->raw(), id, &length);
    return std::string_view(name, length);
}
std::string_view Query::string_value_for_id(std::uint32_t id) const {
    std::uint32_t length;
    const char* value = ts_query_string_value_for_id(this->raw(), id, &length);
    return std::string_view(value, length);
}

void Query::disable_capture(std::string_view name) {
    ts_query_disable_capture(this->raw(), name.data(), name.length());
}
void Query::disable_pattern(std::uint32_t id) { ts_query_disable_pattern(this->raw(), id); }

// struct Capture
Capture::Capture(TSQueryCapture capture, const Tree& tree) noexcept
    : node(Node(capture.node, tree)), index(capture.index) {}

std::ostream& operator<<(std::ostream& os, const Capture& capture) {
    return os << "Capture { .node = " << capture.node << ", .index = " << capture.index << " }";
}
std::ostream& operator<<(std::ostream& os, const std::vector<Capture>& captures) {
    os << "[ ";
    const char* sep = "";
    for (const auto& capture : captures) {
        os << sep << capture;
        sep = ", ";
    }
    os << " ]";
    return os;
}

// struct Match
Match::Match(TSQueryMatch match, const Tree& tree) noexcept
    : id(match.id), pattern_index(match.pattern_index) {
    const TSQueryCapture* start = match.captures;
    const TSQueryCapture* end = start + match.capture_count;
    const std::size_t size = end - start;

    this->captures.reserve(size);
    std::transform(
        start, end, std::back_inserter(this->captures),
        [&tree](const TSQueryCapture capture) { return Capture(capture, tree); });
}

std::optional<Capture> Match::capture_with_index(std::uint32_t index) const {
    for (const auto& capture : this->captures) {
        if (capture.index == index) {
            return capture;
        }
    }
    return std::nullopt;
}

std::ostream& operator<<(std::ostream& os, const Match& match) {
    return os << "Match { .id = " << match.id << ", .pattern_index = " << match.pattern_index
              << ", .captures = " << match.captures.size() << " }";
}
std::ostream& operator<<(std::ostream& os, const std::vector<Match>& matches) {
    os << "[ ";
    const char* sep = "";
    for (const auto& match : matches) {
        os << sep << match;
        sep = ", ";
    }
    os << " ]";
    return os;
}

// class QueryCursor
QueryCursor::QueryCursor(const Tree& tree) noexcept
    : cursor(ts_query_cursor_new(), ts_query_cursor_delete), tree(&tree) {}

const TSQueryCursor* QueryCursor::raw() const { return this->cursor.get(); }
TSQueryCursor* QueryCursor::raw() { return this->cursor.get(); }

void QueryCursor::exec(const Query& query, Node node) {
    ts_query_cursor_exec(this->raw(), query.raw(), node.raw());
}

void QueryCursor::exec(const Query& query) {
    ts_query_cursor_exec(this->raw(), query.raw(), this->tree->root_node().raw());
}

std::optional<Match> QueryCursor::next_match() {
    TSQueryMatch match;
    if (ts_query_cursor_next_match(this->raw(), &match)) {
        return Match(match, *this->tree);
    } else {
        return std::nullopt;
    }
}

std::optional<Capture> QueryCursor::next_capture() {
    TSQueryMatch raw_match;
    std::uint32_t index;
    if (ts_query_cursor_next_capture(this->raw(), &raw_match, &index)) {
        Match match(raw_match, *this->tree);
        return match.capture_with_index(index);
    } else {
        return std::nullopt;
    }
}

std::vector<Match> QueryCursor::matches() {
    std::vector<Match> matches;

    while (std::optional<Match> match = this->next_match()) {
        matches.push_back(*match);
    }

    return matches;
}

} // namespace ts

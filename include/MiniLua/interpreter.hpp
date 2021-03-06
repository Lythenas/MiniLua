#ifndef MINILUA_INTERPRETER_H
#define MINILUA_INTERPRETER_H

#include <memory>
#include <string>
#include <vector>

#include "environment.hpp"
#include "source_change.hpp"
#include "values.hpp"

namespace minilua {

/**
 * @brief Result of calling Interpreter::parse.
 *
 * Contains a list of errors and can be converted to a bool.
 * True means sucessful parse, false means there are errors.
 */
struct ParseResult {
    /**
     * @brief The errors.
     */
    std::vector<std::string> errors; // NOLINT(misc-non-private-member-variables-in-classes)

    /**
     * @brief Convert to `false` if there was an error, `true` otherwise.
     */
    operator bool() const;
};

/**
 * @brief Result of calling Interpreter::evaluate.
 *
 * Contains the return value of the lua code (if any)
 * and the produced source changes.
 */
struct EvalResult {
    /**
     * @brief The return value.
     */
    Value value;
    /**
     * @brief The generated source changes.
     */
    std::optional<SourceChangeTree> source_change;

    EvalResult();
};

auto operator<<(std::ostream&, const EvalResult&) -> std::ostream&;

/**
 * @brief Configuration for the Interpreter.
 *
 * This controls the debug logging.
 *
 * The target defaults to `std::cerr`.
 */
struct InterpreterConfig {
    /**
     * @brief Where to log to.
     *
     * Defaults to `std::cerr`.
     */
    std::ostream* target;
    /**
     * @brief Trace node enter and exit.
     */
    bool trace_nodes;
    /**
     * @brief Trace calls.
     */
    bool trace_calls;
    /**
     * @brief Trace entering blocks.
     */
    bool trace_enter_block;
    /**
     * @brief Trace creating expression lists.
     */
    bool trace_exprlists;
    /**
     * @brief Trace breaking in loops.
     */
    bool trace_break;
    /**
     * @brief Trace creating varargs.
     */
    bool trace_varargs;

    /**
     * @brief Default constructor turns all tracing off.
     */
    InterpreterConfig();
    /**
     * @brief Sets all tracing to the given value.
     */
    InterpreterConfig(bool);

    /**
     * @brief Set all trace flags to the given value.
     */
    void all(bool);
};

/**
 * @brief Exception thrown by the interpreter.
 */
class InterpreterException : public std::runtime_error {
public:
    InterpreterException(const std::string& what);
};

/**
 * @brief An interpreter instance is used to parse and evaluate lua source
 * code.
 *
 * You can create the interpreter with initial source code or empty source code
 * and then update the source code by calling Interpreter::parse and run the
 * current source code by calling Interpreter::evaluate.
 *
 * You can also apply a source change that was generated by the interpreter
 * while evaluating and get the updated source code.
 *
 * # Example
 *
 * ```cpp
 * minilua::Interpreter interpreter{initial_source_code};
 * interpreter.parse(new_source_code);
 * auto res = interpreter.evaluate();
 * if (res.source_change) {
 *   interpreter.apply_source_change(res.source_change);
 * }
 * interpreter.evaluate();
 * ```
 */
class Interpreter {
    struct Impl;
    std::unique_ptr<Impl> impl;

    InterpreterConfig _config;

public:
    /**
     * @brief Initializes the interpreter with empty source code
     */
    Interpreter();

    /**
     * @brief Initializes the interpreter with the given source code
     *
     * Throws an exception if there was an error parsing the source code.
     */
    Interpreter(std::string initial_source_code);

    ~Interpreter();

    /**
     * @brief Returns the current configuration.
     */
    auto config() -> InterpreterConfig&;
    /**
     * @brief Returns the current configuration.
     */
    [[nodiscard]] auto config() const -> const InterpreterConfig&;
    /**
     * @brief Sets the configuration.
     */
    void set_config(InterpreterConfig);

    /**
     * @brief Returns the environment for modification.
     *
     * \note This will not directly be used by the interpreter so you can just
     * reuse an existing interpreter/environment without resetting it.
     */
    [[nodiscard]] auto environment() const -> Environment&;

    /**
     * @brief Returns a view into the current source code.
     *
     * \warning The returned value will become invalid if the source code is
     * changed (by calling parse or apply_source_changes).
     */
    [[nodiscard]] auto source_code() const -> std::string_view;

    /**
     * @brief Parse fresh source code.
     *
     * Errors are part of ParseResult.
     */
    auto parse(std::string source_code) -> ParseResult;

    /**
     * @brief Applies a list of single source changes.
     */
    void apply_source_changes(std::vector<SourceChange>);

    /**
     * @brief Run the parsed program.
     *
     * Throws an exception when there is an error in the program.
     * E.g. when you try to add incompatible values in lua.
     *
     * Currently this throws `std::runtime_error`.
     */
    auto evaluate() -> EvalResult;
};

}; // namespace minilua

#endif

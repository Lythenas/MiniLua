assert(1 + 4 == 5)
assert(true and 7 == 7)
assert(true and false == false)
assert(7 or false == 7)
assert("hi " .. "world" == "hi world")
assert(3 - 5.2 == -2.2)
assert(7 * 1.5 == 10.5)
assert(40 / 8 == 5)
assert(42 // 8 == 5)
assert(3 ^ 4 == 81)
assert(17 % 4 == 1)
assert(0xf3 & 0x42 == 0x42)
assert(0x33 | 0x44 == 0x77)
assert(0xf3 ~ 0x4f == 0xBC)
assert(0xf3 >> 2 == 0x3C)
assert(0xf3 << 2 == 0x3CC)
assert(~0xf3 == 0xffffffffffffff0c)
assert(22 ~= 21)
assert(22 >= 21)
assert(22 > 21)
assert(17 <= 21)
assert(17 < 21)

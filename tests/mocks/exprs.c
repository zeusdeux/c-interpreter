// "multi word \"string\""
// 100
// -100
// asd
// &deref_me
// test
// *one
// **potato
// *****tomato
// &( *deref_me)
()
-()
(123)
((123))
(    "omg")
((    "omg", 123, * test, & omg, **test, "some other '\"string\"'"))
-(-&(*bruh))
// 100 + 200 + (20 + 20)

// // 20)0 should be an error
// 100L
// 100l
// -100L
// -100l
// 100U
// 100u
// -100U
// -100u
// 100LU
// 100lu
// -100LLU
// -100llu
// 10.34
// 10.34F
// 10.34f
// -10.34
// -10.34F
// -10.34f


// 100 + 10
// 100 - 10
// 100 * 10
// 100 / 10
// // TODO(mudit): should this be a parse error or runtime error?
// 120 + "multi word string"
// // 125
// 100 + 10 / 2 * 5
// // 104
// // 11
// 100 + 10 * 2 / 5
// (100 + 10) / (2 * 5)
// // 101
// 100 + 10 / (2 * 5)
// // test "string"
// (100, 20, "test \"string\"")
// // 1
// 100 / add(10, 90)
// // 4
// (100 / add(10, 40) * 2)

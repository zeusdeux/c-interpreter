"multi word \"string\""
100
-100
asd
&deref_me
test
*one
**potato
*****tomato
&( *deref_me)
-(-&(*bruh))
()
-()
(123)
((123))
(((    "omg", 900, asd)))
(    "omg", 123, * test, & omg, **test, "some other '\"string\"'")
// test "string"
(100, 20, "test \"string\"")
// TODO(mudit): Do AST optimization wherein nodes with no context such as 100 + 10 here can be nuked from the tree safely
(100 + 10, 20 * 200)

100 + 10
100 - 10
100 * 10
100 / 10
// 125
100 + 10 / 2 * 5
// 104
100 + 10 * 2 / 5
// 275
(100 + 10) / 2 * 5
(100 + 10) / (2 * 5)
// TODO(mudit): should this be a parse error or runtime error?
120 + "multi word string"
// // 1
// 100 / add(10, 90)
// // 4
// (100 / add(10, 40) * 2)
100 - -10
100 - +10
100 - !(s + 200 * 100)
// 60
100 + 20 * 200  / 100 - 80
100 + !s * 200  / 100 - 80

// (()
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

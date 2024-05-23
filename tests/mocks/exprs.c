100 + 10
100 - 10
100 * 10
100 / 10
// TODO(mudit): should this be a parse error or runtime error?
120 + "multi word string"
// 125
100 + 10 / 2 * 5
// 104
// 11
100 + 10 * 2 / 5
(100 + 10) / (2 * 5)
// 101
100 + 10 / (2 * 5)
// test "string"
(100, 20, "test \"string\"")
// 1
100 / add(10, 90)
// 4
(100 / add(10, 40) * 2)

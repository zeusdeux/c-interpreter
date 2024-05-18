const char *const *const potato = &conststr;
static const unsigned char *_s = &"Test string";
static const int a = 1000;
a = "asd";
printf("%s (%zu bytes)\n", _s, sizeof(*_s));
typedef int potato;

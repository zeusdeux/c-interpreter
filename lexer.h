#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>

#include "./zdx_string_view.h"

typedef struct {
  size_t cursor;
  size_t bol;
  size_t line;
  const sv_t *input;
} lexer_t;

typedef enum {
  TOKEN_KIND_END,
  TOKEN_KIND_WS,
  TOKEN_KIND_NEWLINE,
  TOKEN_KIND_OPAREN,
  TOKEN_KIND_CPAREN,
  TOKEN_KIND_COMMA,
  TOKEN_KIND_SEMICOLON,
  TOKEN_KIND_AMPERSAND,
  TOKEN_KIND_EXCLAMATION,
  TOKEN_KIND_EQL,
  TOKEN_KIND_STAR,
  TOKEN_KIND_PLUS,
  TOKEN_KIND_MINUS,
  TOKEN_KIND_FSLASH,
  TOKEN_KIND_TYPEDEF,
  TOKEN_KIND_STORAGE, // static, extern, auto, register and _Thread_local
  TOKEN_KIND_QUALIFIER, // const, restrict, volatile, _Atomic
  TOKEN_KIND_SYMBOL,
  TOKEN_KIND_STRING,
  TOKEN_KIND_SIGNED_INT,
  TOKEN_KIND_UNSIGNED_INT,
  TOKEN_KIND_FLOAT,
  TOKEN_KIND_DOUBLE,
  TOKEN_KIND_UNKNOWN,
  TOKEN_KIND_COUNT
} token_kind_t;

typedef struct {
  token_kind_t kind;
  sv_t value;
  const char *err;
} token_t;

const char* token_kind_name(token_kind_t kind);
void print_token(const token_t tok);
token_t get_next_token(lexer_t lexer[const static 1]);
token_t peek_next_token(const lexer_t lexer[const static 1]);
void reset_lexer(lexer_t dst[const static 1], const lexer_t src);

#endif // LEXER_H_

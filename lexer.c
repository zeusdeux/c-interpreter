#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "./lexer.h"

#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"

// section 6.7.1 of c17 standard also lists "typedef" as a storage class
// but we tokenize that separately into TOKEN_KIND_TYPEDEF
static const char *storage_classes[] = {
  "extern",
  "static",
  "_Thread_local",
  "auto",
  "register"
};
static const uint8_t storage_class_lengths[] = {
  6, // extern
  6, // static
  13, // _Thread_local
  4, // auto
  8, // register
};

static const char *type_qualifiers[] = {
  "const",
  "restrict",
  "volatile",
  "_Atomic"
};
static const uint8_t type_qualifiers_lengths[] = {
  5, // const
  8, // restrict
  8, // volatile
  7, // _Atomic
};


const char* token_kind_name(token_kind_t kind)
{
  static const char *token_to_str[] = {
    "TOKEN_KIND_END",
    "TOKEN_KIND_WS",
    "TOKEN_KIND_NEWLINE",
    "TOKEN_KIND_OPAREN",
    "TOKEN_KIND_CPAREN",
    "TOKEN_KIND_COMMA",
    "TOKEN_KIND_SEMICOLON",
    "TOKEN_KIND_AMPERSAND",
    "TOKEN_KIND_EXCLAMATION",
    "TOKEN_KIND_EQL",
    "TOKEN_KIND_STAR",
    "TOKEN_KIND_PLUS",
    "TOKEN_KIND_MINUS",
    "TOKEN_KIND_FSLASH",
    "TOKEN_KIND_TYPEDEF",
    "TOKEN_KIND_STORAGE", // static, extern, auto, register and _Thread_local
    "TOKEN_KIND_QUALIFIER", // const, restrict, volatile, _Atomic
    "TOKEN_KIND_SYMBOL",
    "TOKEN_KIND_STRING",
    "TOKEN_KIND_SIGNED_INT",
    "TOKEN_KIND_UNSIGNED_INT",
    "TOKEN_KIND_FLOAT",
    "TOKEN_KIND_DOUBLE",
    "TOKEN_KIND_UNKNOWN",
  };

  _Static_assert(zdx_arr_len(token_to_str) == TOKEN_KIND_COUNT,
                 "Some token kinds are missing their corresponding strings in token to string map");

  assertm(kind < TOKEN_KIND_COUNT, "Invalid token kind %d", kind);

  return token_to_str[kind];
}

void print_token(const token_t tok)
{
  if (tok.kind == TOKEN_KIND_END) {
    printf("kind = %s    \tlength = %zu \tval = %s\n",
           token_kind_name(tok.kind), tok.value.length, tok.value.buf);
  } else if (sv_eq_cstr(tok.value, "\n")) {
    printf("kind = %s    \tlength = %zu \tval = '\\n'\n",
           token_kind_name(tok.kind), tok.value.length);
  } else {
    printf("kind = %s    \tlength = %zu \tval = '"SV_FMT"'\n",
           token_kind_name(tok.kind), tok.value.length, sv_fmt_args(tok.value));
  }
}

void reset_lexer(lexer_t dst[const static 1], const lexer_t src)
{
  *dst = src;
}


token_t peek_next_token(const lexer_t lexer[const static 1])
{
  lexer_t temp_lexer = *lexer;
  token_t next_tok = get_next_token(&temp_lexer);

  return next_tok;
}

token_t get_next_token(lexer_t lexer[const static 1])
{
  token_t tok = {0};

  assertm(lexer->cursor - lexer->bol >= 0, "Expected: lexer cursor to be greater or equal to lexer.bol, "
          "Recevied: (cursor = %zu, bol = %zu)", lexer->cursor, lexer->bol);

  if (lexer->input->length <= 0 || lexer->cursor >= lexer->input->length) {
    tok.kind = TOKEN_KIND_END;

    return tok;
  }

  if (isspace(lexer->input->buf[lexer->cursor])) {
    tok.kind = TOKEN_KIND_WS;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    if (sv_eq_cstr(tok.value, "\n")) {
      tok.kind = TOKEN_KIND_NEWLINE;
      lexer->bol = lexer->cursor;
      lexer->line++;
    }

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '*') {
    tok.kind = TOKEN_KIND_STAR;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '=') {
    tok.kind = TOKEN_KIND_EQL;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '(') {
    tok.kind = TOKEN_KIND_OPAREN;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == ')') {
    tok.kind = TOKEN_KIND_CPAREN;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == ',') {
    tok.kind = TOKEN_KIND_COMMA;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == ';') {
    tok.kind = TOKEN_KIND_SEMICOLON;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '&') {
    tok.kind = TOKEN_KIND_AMPERSAND;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '!') {
    tok.kind = TOKEN_KIND_EXCLAMATION;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '+') {
    tok.kind = TOKEN_KIND_PLUS;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  if (lexer->input->buf[lexer->cursor] == '-') {
    tok.kind = TOKEN_KIND_MINUS;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  // this == '/' and cursor+1 != '/' is as "//" are to be grouped as a single line comment starter
  if (lexer->input->buf[lexer->cursor] == '/' && lexer->input->buf[lexer->cursor + 1] != '/') {
    tok.kind = TOKEN_KIND_FSLASH;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    return tok;
  }

  {
    sv_t temp = {
      .buf = &lexer->input->buf[lexer->cursor],
      .length = lexer->input->length - lexer->cursor
    };

    // typedef
    if (sv_begins_with_word_cstr(temp, "typedef")) {
      tok.kind = TOKEN_KIND_TYPEDEF;
      tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 7); // 7 = strlen("typedef")
      lexer->cursor += 7;

      return tok;
    }

    // storage classes
    for(size_t i = 0; i < zdx_arr_len(storage_classes); i++) {
      const char *storage_class = storage_classes[i];

      if (sv_begins_with_word_cstr(temp, storage_class)) {
        const uint8_t token_length = storage_class_lengths[i];

        tok.kind = TOKEN_KIND_STORAGE;
        tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor], token_length);
        lexer->cursor += token_length;

        return tok;
      }
    }

    // type qualifiers
    for(size_t i = 0; i < zdx_arr_len(type_qualifiers); i++) {
      const char *type_qualifier = type_qualifiers[i];

      if (sv_begins_with_word_cstr(temp, type_qualifier)) {
        const uint8_t token_length = type_qualifiers_lengths[i];

        tok.kind = TOKEN_KIND_QUALIFIER;
        tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor], token_length);
        lexer->cursor += token_length;

        return tok;
      }
    }
  } // sv_t temp; out of scope here


  // symbol
  if (isalpha(lexer->input->buf[lexer->cursor]) || lexer->input->buf[lexer->cursor] == '_') {
    char c;

    tok.kind = TOKEN_KIND_SYMBOL;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], 1);

    c = lexer->input->buf[lexer->cursor];

    while(isalpha(c) || isdigit(c) || c == '_') {
      tok.value.length++;
      lexer->cursor++;
      c = lexer->input->buf[lexer->cursor];
    }

    return tok;
  }

  // comment: single line
  if (lexer->input->buf[lexer->cursor] == '/' && lexer->input->buf[lexer->cursor + 1] == '/') {
    // swallow "//"
    lexer->cursor += 2;
    // swallow comment
    while(lexer->input->buf[lexer->cursor] != '\n') {
      lexer->cursor++;
    }
    // return token after comment
    return get_next_token(lexer);
  }

  // string
  if (lexer->input->buf[lexer->cursor] == '"') {
    lexer->cursor += 1;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor], 0);

    while(true) {
      // end of input before end of string so return TOKEN_KIND_UNKNOWN
      if (lexer->cursor >= lexer->input->length || lexer->input->buf[lexer->cursor] == '\n') {
        tok.kind = TOKEN_KIND_UNKNOWN;
        tok.err = "Expected a closing quote (\")";

        return tok;
      }

      if (lexer->input->buf[lexer->cursor] == '"' && lexer->input->buf[lexer->cursor - 1] != '\\') {
        tok.kind = TOKEN_KIND_STRING;
        lexer->cursor += 1;

        return tok;
      }

      tok.value.length += 1;
      lexer->cursor += 1;
    }

    assertm(false, "UNREACHABLE: Expected TOKEN_KIND_STRING or "
            "TOKEN_KIND_UNKNOWN to have already been returned");
  }

  // signed int -> it's the default to align with how C behaves
  if (isdigit(lexer->input->buf[lexer->cursor])) {
    size_t length = 1;

    tok.kind = TOKEN_KIND_SIGNED_INT;
    tok.value = sv_from_buf(&lexer->input->buf[lexer->cursor++], length);

    while (isdigit(lexer->input->buf[lexer->cursor])) {
      lexer->cursor++;
      length++;
    }

    tok.value.length = length;

    return tok;
  }

  assertm(false, "TODO: implement lexing of other token types");

  return tok;
}

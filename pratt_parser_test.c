#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "./zdx_util.h"

#define ZDX_STRING_VIEW_IMPLEMENTATION
#include "./zdx_string_view.h"

typedef struct {
  sv_t input;
  size_t cursor;
} lexer_t;

typedef enum {
  TOKEN_KIND_UNKNOWN,
  TOKEN_KIND_END,
  TOKEN_KIND_OPAREN,
  TOKEN_KIND_CPAREN,
  TOKEN_KIND_NUMBER,
  TOKEN_KIND_PLUS,
  TOKEN_KIND_MINUS,
  TOKEN_KIND_ASTERISK,
  TOKEN_KIND_FSLASH,
  TOKEN_KIND_COUNT,
} token_kind_t;

typedef struct {
  token_kind_t kind;
  sv_t value;
} token_t;

typedef struct {
  size_t length;
  size_t capacity;
  sv_t *items;
} sv_arr_t;

typedef enum {
  LITERAL_KIND_UNKNOWN,
  LITERAL_KIND_NUMBER,
  LITERAL_KIND_COUNT,
} literal_kind_t;

typedef enum {
  BINOP_KIND_UNKNOWN,
  BINOP_KIND_ADD,
  BINOP_KIND_SUB,
  BINOP_KIND_MULT,
  BINOP_KIND_DIV,
  BINOP_KIND_COUNT,
} binop_kind_t;

typedef enum {
  NODE_KIND_UNKNOWN,
  NODE_KIND_ERROR,
  NODE_KIND_LITERAL,
  NODE_KIND_BINOP,
  NODE_KIND_COUNT,
} node_kind_t;

typedef struct node_t {
  node_kind_t kind;

  union {
    struct {
      const char *msg;
    } err;

    struct {
      literal_kind_t kind;
      sv_t value;
    } literal;

    struct {
      binop_kind_t kind;
      struct node_t *left;
      struct node_t *right;
    } binop;
  };
} node_t;

typedef struct {
  size_t left;
  size_t right;
  const char *err;
} precedence_t;


void indent(size_t depth)
{
  while(depth--) {
    fprintf(stderr, "  ");
  }
}

void print_token(token_t token)
{
  static const char *token_kind_to_str[] = {
    "TOKEN_KIND_UNKNOWN",
    "TOKEN_KIND_END",
    "TOKEN_KIND_OPAREN",
    "TOKEN_KIND_CPAREN",
    "TOKEN_KIND_NUMBER",
    "TOKEN_KIND_PLUS",
    "TOKEN_KIND_MINUS",
    "TOKEN_KIND_ASTERISK",
    "TOKEN_KIND_FSLASH",
  };
  fprintf(stderr, "Token: kind %s, value "SV_FMT"\n", token_kind_to_str[token.kind], sv_fmt_args(token.value));
}

void print_node(node_t node, size_t depth)
{
  static const char *node_kind_to_str[] = {
    "NODE_KIND_UNKNOWN",
    "NODE_KIND_ERROR",
    "NODE_KIND_LITERAL",
    "NODE_KIND_BINOP",
  };

  _Static_assert(zdx_arr_len(node_kind_to_str) == NODE_KIND_COUNT, "Missing node name for node kind");

  static const char *literal_kind_to_str[] = {
    "LITERAL_KIND_UNKNOWN",
    "LITERAL_KIND_NUMBER",
  };

  _Static_assert(zdx_arr_len(literal_kind_to_str) == LITERAL_KIND_COUNT, "Missing literal name for literal kind");

  static const char *binop_kind_to_str[] = {
    "BINOP_KIND_UNKNOWN",
    "BINOP_KIND_ADD",
    "BINOP_KIND_SUB",
    "BINOP_KIND_MULT",
    "BINOP_KIND_DIV",
  };

  _Static_assert(zdx_arr_len(binop_kind_to_str) == BINOP_KIND_COUNT, "Missing binop name for binop kind");

  indent(depth);
  fprintf(stderr, "Node kind: %s\n", node_kind_to_str[node.kind]);

  switch(node.kind) {
    case NODE_KIND_ERROR: {
      indent(depth);
      fprintf(stderr, "Message: %s\n", node.err.msg);
    } break;
    case NODE_KIND_LITERAL: {
      indent(depth);
      fprintf(stderr, "Literal kind: %s\n", literal_kind_to_str[node.literal.kind]);
      indent(depth);
      fprintf(stderr, "Literal value: "SV_FMT"\n", sv_fmt_args(node.literal.value));
    } break;
    case NODE_KIND_BINOP: {
      indent(depth);
      fprintf(stderr, "Binop kind: %s\n", binop_kind_to_str[node.binop.kind]);
      indent(depth);
      fprintf(stderr, "Binop left: \n");
      print_node(*node.binop.left, depth + 1);
      indent(depth);
      fprintf(stderr, "Binop right: \n");
      print_node(*node.binop.right, depth + 1);
    } break;
    default: {
      assertm(false, "Unrecognized node kind %d", node.kind);
    } break;
  }
}


void reset_lexer(lexer_t *dst, const lexer_t *const src)
{
  *dst = *src;
}

token_t get_next_token(lexer_t lexer[const static 1])
{
  if (lexer->cursor >= lexer->input.length) {
    return (token_t){ .kind = TOKEN_KIND_END };
  }

  if (isspace(lexer->input.buf[lexer->cursor])) {
    lexer->cursor++;

    return get_next_token(lexer);
  }

  if (lexer->input.buf[lexer->cursor] == '(') {
    return (token_t){
      .kind = TOKEN_KIND_OPAREN,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (lexer->input.buf[lexer->cursor] == ')') {
    return (token_t){
      .kind = TOKEN_KIND_CPAREN,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (lexer->input.buf[lexer->cursor] == '+') {
    return (token_t){
      .kind = TOKEN_KIND_PLUS,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (lexer->input.buf[lexer->cursor] == '-') {
    return (token_t){
      .kind = TOKEN_KIND_MINUS,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (lexer->input.buf[lexer->cursor] == '*') {
    return (token_t){
      .kind = TOKEN_KIND_ASTERISK,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (lexer->input.buf[lexer->cursor] == '/') {
    return (token_t){
      .kind = TOKEN_KIND_FSLASH,
      .value = sv_from_buf(&lexer->input.buf[lexer->cursor++], 1)
    };
  }

  if (isdigit(lexer->input.buf[lexer->cursor])) {
    const char *start = &lexer->input.buf[lexer->cursor];
    size_t len = 1;
    lexer->cursor += 1;
    while (isdigit(lexer->input.buf[lexer->cursor])) {
      len++;
      lexer->cursor++;
    }

    return (token_t){
      .kind = TOKEN_KIND_NUMBER,
      .value = sv_from_buf(start, len)
    };
  }

  assertm(false, "Unrecognized character: %c", lexer->input.buf[lexer->cursor]);
}

token_t peek_next_token(lexer_t lexer[const static 1])
{
  lexer_t before = *lexer;
  token_t tok = get_next_token(lexer);

  *lexer = before;
  return tok;
}

node_t parse_literal(lexer_t lexer[const static 1])
{
  token_t token = peek_next_token(lexer);

  if (token.kind != TOKEN_KIND_NUMBER) {
    return (node_t){
      .kind = NODE_KIND_ERROR,
      .err = {
        .msg = "Number token expected"
      }
    };
  }

  token = get_next_token(lexer);

  return (node_t){
    .kind = NODE_KIND_LITERAL,
    .literal = {
      .kind = LITERAL_KIND_NUMBER,
      .value = token.value
    }
  };
}

// fwd declaration for use below
node_t parse_expr(lexer_t lexer[const static 1], uint8_t parser_choice);

node_t parse_parenthesized_expr(lexer_t lexer[const static 1])
{
  token_t token = peek_next_token(lexer);

  if (token.kind != TOKEN_KIND_OPAREN) {
    return (node_t){
      .kind = NODE_KIND_ERROR,
      .err = {
        .msg = "Unexpected token where open paren expected"
      }
    };
  }

  get_next_token(lexer);

  node_t expr = parse_expr(lexer, 0);

  if (expr.kind == NODE_KIND_ERROR) {
    return expr;
  }

  token = peek_next_token(lexer);

  if (token.kind != TOKEN_KIND_CPAREN) {
    return (node_t){
      .kind = NODE_KIND_ERROR,
      .err = {
        .msg = "Unexpected token where closed paren expected"
      }
    };
  }

  get_next_token(lexer);

  return expr;
}

static inline precedence_t get_infix_precedence(token_t op)
{
  if (op.kind == TOKEN_KIND_PLUS || op.kind == TOKEN_KIND_MINUS) {
    return (precedence_t){ .left = 1, .right = 2 };
  }

  if (op.kind == TOKEN_KIND_ASTERISK || op.kind == TOKEN_KIND_FSLASH) {
    return (precedence_t){ .left = 3, .right = 4 };
  }

  return (precedence_t){ .err = "No precedence value for op" };
}

// precedence climbing for math binary ops via a pratt parser
node_t pratt_parser_expr(lexer_t lexer[const static 1], uint8_t min_precedence, uint8_t parser_choice)
{
  node_t lhs = parse_expr(lexer, parser_choice + 1);

  if (lhs.kind == NODE_KIND_ERROR) {
    return lhs;
  }

  for(;;) {
    token_t op = peek_next_token(lexer);
    binop_kind_t binop_kind = {0};

    if (op.kind == TOKEN_KIND_PLUS) {
      binop_kind = BINOP_KIND_ADD;
    }
    else if (op.kind == TOKEN_KIND_MINUS) {
      binop_kind = BINOP_KIND_SUB;
    }
    else if (op.kind == TOKEN_KIND_ASTERISK) {
      binop_kind = BINOP_KIND_MULT;
    }
    else if (op.kind == TOKEN_KIND_FSLASH) {
      binop_kind = BINOP_KIND_DIV;
    }
    else if (op.kind == TOKEN_KIND_END) {
      break;
    }
    else {
      break;
    }

    precedence_t p = get_infix_precedence(op);

    if (p.err) {
      node_t error = (node_t){
        .kind = NODE_KIND_ERROR,
        .err = {
          .msg = p.err
        }
      };

      return error;
    }

    if (p.left < min_precedence) {
      break;
    }

    get_next_token(lexer); // consume op

    // consume following exprs with greater precendence until same or
    // lower precendence op is hit. Try it with a + b * c * d + e in
    // your head
    node_t rhs = pratt_parser_expr(lexer, p.right, parser_choice);

    if (rhs.kind == NODE_KIND_ERROR) {
      return rhs;
    }

    node_t *p_lhs = malloc(sizeof(*p_lhs));
    memcpy(p_lhs, &lhs, sizeof(lhs));
    node_t *p_rhs = malloc(sizeof(*p_rhs));
    memcpy(p_rhs, &rhs, sizeof(rhs));

    node_t binop_node = {
      .kind = NODE_KIND_BINOP,
      .binop = {
        .kind = binop_kind,
        .left = p_lhs,
        .right = p_rhs
      }
    };

    lhs = binop_node;
  }

  return lhs;
}

node_t parse_expr(lexer_t lexer[const static 1], uint8_t parser_choice)
{
  node_t node = {0};
  lexer_t before = *lexer;

  while(true) {
    switch(parser_choice) {
      case 0: {
        node = pratt_parser_expr(lexer, 0, parser_choice); // lowest precendence is 0
      } break;
      case 1: {
        node = parse_parenthesized_expr(lexer);
      } break;
      case 2: {
        node = parse_literal(lexer);
      } break;
      default: {
        return (node_t){
          .kind = NODE_KIND_ERROR,
          .err = {
            .msg = "Cannot parse as expr"
          }
        };
      }
    }

    if (node.kind == NODE_KIND_ERROR) {
      parser_choice++;
      reset_lexer(lexer, &before);
      node = (node_t){0};
    } else {
      break;
    }
  }

  return node;
}

node_t parse(const char* input)
{
  lexer_t lexer = {
    .input = sv_from_buf(input, strlen(input))
  };
  node_t result = {0};
  token_t tok = peek_next_token(&lexer);

  while (tok.kind != TOKEN_KIND_END) {
    result = parse_expr(&lexer, 0);
    if (result.kind == NODE_KIND_ERROR) {
      break;
    }
    tok = peek_next_token(&lexer);
  }

  return result;
}

// gcc -O2 -g -std=c17 -Wall -Wdeprecated -Wpedantic -Wextra -o pratt_parser_test pratt_parser_test.c && ./pratt_parser_test
int main(void)
{
  const char *exprs[] = {
    "100 + 10",
    "100 - 10",
    "100 * 10",
    "100 / 10",
    "100 + 10 - 20",
    "100 + 10 * 20",
    // 125 aka (+ 100 (* (/ 10 2) 5))
    "100 + 10 / 2 * 5",
    // 104
    // 11
    "100 + 10 * 2 / 5",
    // 101
    // 24000600
    "100 + 200 * 300 * 400 + 500",
    "(1)",
    "(1 + 2)",
    "((1))",
    "((((1))))",
    "(1 + 2) * 3",
    "100 + 10 / (2 * 5)",
    "((1)",
  };

  for (size_t i = 0; i < zdx_arr_len(exprs); i++) {
    const char *input = exprs[i];
    fprintf(stderr, "%sInput: %s\n", i ? "\n" : "", input);
    print_node(parse(input), 0);
  }

  return 0;
}

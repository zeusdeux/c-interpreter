#include <string.h>

#include "./zdx_util.h"
#include "./parser2.h"

// ------------------------------------ PRINT DEBUGGING ------------------------------------

static const char *node_kind_name(const ast_node_kind_t kind)
{
  static const char *node_kind_to_str[] = {
    "AST_NODE_KIND_UNKNOWN",
    "AST_NODE_KIND_ERROR",
    "AST_NODE_KIND_LITERAL",
    "AST_NODE_KIND_SYMBOL",
    "AST_NODE_KIND_UNARY_OP",
  };

  _Static_assert(zdx_arr_len(node_kind_to_str) == AST_NODE_KIND_COUNT,
                 "Some AST node kinds are missing their corresponding strings in token to string map");

  assertm(kind < AST_NODE_KIND_COUNT, "Invalid node kind %d", kind);

  return node_kind_to_str[kind];
}

static const char *literal_kind_name(const literal_kind_t kind)
{
  static const char *literal_kind_to_str[] = {
  "LITERAL_KIND_UNKNOWN",
  "LITERAL_KIND_NUMBER",
  "LITERAL_KIND_STRING",
  "LITERAL_KIND_BOOL",
};

_Static_assert(zdx_arr_len(literal_kind_to_str) == LITERAL_KIND_COUNT,
               "Some Literal node kinds are missing their corresponding strings in token to string map");

 assertm(kind < LITERAL_KIND_COUNT, "Invalid literal kind %d", kind);

 return literal_kind_to_str[kind];
}

static const char *unary_kind_name(const unary_op_kind_t kind)
{
  static const char *unary_kind_to_str[] = {
    "UNARY_OP_UNKNOWN",
    "UNARY_OP_ADDR_OF",
    "UNARY_OP_DEREF"
  };

  _Static_assert(zdx_arr_len(unary_kind_to_str) == UNARY_OP_COUNT,
                 "Some Unary op kinds are missing their corresponding strings in token to string map");

  assertm(kind < UNARY_OP_COUNT, "Invalid unary op kind %d", kind);

  return unary_kind_to_str[kind];
}

void print_ast(const ast_node_t node)
{
  log(L_INFO, "Node kind: %s", node_kind_name(node.kind));
  switch(node.kind) {
  case AST_NODE_KIND_LITERAL: {
    log(L_INFO, "Literal kind: %s, value: "SV_FMT, literal_kind_name(node.literal.kind), sv_fmt_args(node.literal.value));
  } break;

  case AST_NODE_KIND_SYMBOL: {
    log(L_INFO, "value: "SV_FMT, sv_fmt_args(node.symbol.name));
  } break;

  case AST_NODE_KIND_UNARY_OP: {
    log(L_INFO, "Op: %s", unary_kind_name(node.unary_op.kind));
    print_ast(*node.unary_op.expr);
  } break;

  default: assertm(false, "Missing case of ast node of kind %d", node.kind);
  }

  log(L_INFO, "--------------------");
}


// ------------------------------------ COMBINATORS ------------------------------------

// ? op
bool zero_or_one(lexer_t lexer[const static 1], token_kind_t kind, sv_t *const binding)
{
  token_t tok = peek_next_token(lexer);

  // zero match
  if (tok.kind != kind) {
    return true;
  }

  // one match
  get_next_token(lexer);

  if (binding) {
    binding->buf = tok.value.buf; // buf is an address in lexer->input buffer so their lifetimes are bound
    binding->length = tok.value.length;
  }

  return true;
}

// * op
bool zero_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  while(tok.kind == kind) {
    get_next_token(lexer);
    tok = peek_next_token(lexer);
  }

  return true;
}

// + op
bool one_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  if (tok.kind != kind) {
    return false;
  }

  get_next_token(lexer);

  return zero_or_more(lexer, kind);
}

bool exactly_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding)
{
  token_t tok = peek_next_token(lexer);

  if (tok.kind != kind) {
    return false;
  }

  tok = get_next_token(lexer);

  if (binding) {
    binding->buf = tok.value.buf; // buf is an address in lexer->input buffer so their lifetimes are bound
    binding->length = tok.value.length;
  }

  return true;
}


// ------------------------------------ PARSERS ------------------------------------

// forward sub-parser declarations
static ast_node_t parse_expr(arena_t arena[const static 1], lexer_t lexer[const static 1], size_t parser_choice);

static literal_kind_t get_literal_kind(const token_kind_t token_kind)
{
  switch(token_kind){
  case TOKEN_KIND_SIGNED_INT:
  case TOKEN_KIND_UNSIGNED_INT:
  case TOKEN_KIND_FLOAT:
  case TOKEN_KIND_DOUBLE: return LITERAL_KIND_NUMBER;
  case TOKEN_KIND_STRING: return LITERAL_KIND_STRING;
  default: return LITERAL_KIND_UNKNOWN;
  }
}

static ast_node_t parse_literal(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  token_kind_t literal_kinds[] = {
    TOKEN_KIND_SIGNED_INT,
    TOKEN_KIND_UNSIGNED_INT,
    TOKEN_KIND_FLOAT,
    TOKEN_KIND_DOUBLE,
    TOKEN_KIND_STRING,
  };
  token_kind_t literal_kind;
  bool found_literal = false;
  sv_t literal = {0};

  for (size_t i = 0; i < zdx_arr_len(literal_kinds); i++) {
    literal_kind = literal_kinds[i];

    if (exactly_one(lexer, literal_kind, &literal)) {
      found_literal = true;
      break;
    }
  }

  if (!found_literal) {
    return (ast_node_t) {
      .kind = AST_NODE_KIND_ERROR,
      .err = {
        .msg = "Unexpected character instead of valid expression",
        .line = lexer->line,
        .bol = lexer->bol,
        .cursor = lexer->cursor,
      }
    };
  }

  // TODO(mudit): should we parse the value here or instead of letting
  // the intepreter do it since it knows better what kind of conversion
  // of the literal is necessary based on context?
  ast_node_t node = {
    .kind = AST_NODE_KIND_LITERAL,
    .literal = {
      .kind = get_literal_kind(literal_kind),
      .value = literal
    },
  };

  return node;
}

static ast_node_t parse_symbol(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  sv_t symbol = {0};
  exactly_one(lexer, TOKEN_KIND_SYMBOL, &symbol);

  if (sv_is_empty(symbol)) {
    return (ast_node_t){
      .kind = AST_NODE_KIND_ERROR,
      .err = {
        .msg = "Expected symbol but found nothing",
        .line = lexer->line,
        .bol = lexer->bol,
        .cursor = lexer->cursor
      }
    };
  }

  ast_node_t node = {
    .kind = AST_NODE_KIND_SYMBOL,
    .symbol = {
      .name = symbol
    }
  };

  return node;
}

static inline unary_op_kind_t get_unary_op_kind(const token_kind_t token_kind)
{
  switch(token_kind) {
  case TOKEN_KIND_STAR: return UNARY_OP_DEREF;
  case TOKEN_KIND_AMPERSAND: return UNARY_OP_ADDR_OF;
  default: return UNARY_OP_UNKNOWN;
  }
}

static ast_node_t parse_unary_op(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  sv_t op = {0};
  // TODO(mudit): add other unary ops here
  const token_kind_t unary_ops[] = {
    TOKEN_KIND_STAR,
    TOKEN_KIND_AMPERSAND,
  };
  token_kind_t unary_op = {0};
  bool found_unary_op = false;

  for (size_t i = 0; i < zdx_arr_len(unary_ops); i++) {
    unary_op = unary_ops[i];

    if (exactly_one(lexer, unary_op, &op)) {
      found_unary_op = true;
      break;
    }
  }

  if (sv_is_empty(op) || !found_unary_op) {
    return (ast_node_t){
      .kind = AST_NODE_KIND_ERROR,
      .err = {
        .msg = "No unary operator found",
        .line = lexer->line,
        .bol = lexer->bol,
        .cursor = lexer->cursor
      }
    };
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  ast_node_t expr = parse_expr(arena, lexer, 0);

  if (has_err(expr)) {
    return expr;
  }

  ast_node_t node = {
    .kind = AST_NODE_KIND_UNARY_OP,
    .unary_op = {
      .kind = get_unary_op_kind(unary_op),
      .expr = arena_alloc(arena, sizeof(expr))
    }
  };

  memcpy(node.unary_op.expr, &expr, sizeof(expr));

  return node;
}

static ast_node_t parse_expr(arena_t arena[const static 1], lexer_t lexer[const static 1], size_t parser_choice)
{
  ast_node_t node = {0};

  size_t max_chars_consumed = 0;
  char *error_msg = "Parsing expression error";
  size_t error_line = 0;
  size_t error_bol = 0;
  size_t error_cursor = 0;

  lexer_t before = *lexer;

  while(true) {
    switch(parser_choice) {
    case 0: {
      node = parse_unary_op(arena, lexer);
    } break;
    case 1: {
      node = parse_symbol(arena, lexer);
    } break;
    case 2: {
      node = parse_literal(arena, lexer);
    } break;
    default: return (ast_node_t){
        .kind = AST_NODE_KIND_ERROR,
        .err = {
          .msg = error_msg,
          .line = error_line,
          .bol = error_bol,
          .cursor = error_cursor
        }
      };
    }

    if (has_err(node)) {
      const size_t chars_consumed = lexer->cursor - before.cursor;

      if (chars_consumed >= max_chars_consumed) {
        max_chars_consumed = chars_consumed;
        error_msg = node.err.msg;
        error_line = node.err.line;
        error_bol = node.err.bol;
        error_cursor = node.err.cursor;
      }

      reset_lexer(lexer, before);
      parser_choice++;
    }
    else {
      return node;
    }
  }
}

ast_node_list_t parse(arena_t arena[const static 1], const char path[const static 1], const char source[const static 1], const size_t source_length)
{
  ast_node_list_t statements = {0};
  const sv_t input = sv_from_buf(source, source_length);
  lexer_t lexer = {
    .input = &input
  };

  size_t max_chars_consumed = 0;
  char *error_msg = "Parsing error";
  size_t error_line = 0;
  size_t error_bol = 0;
  size_t error_cursor = 0;

  lexer_t before = lexer;
  uint8_t parser_choice = 0;
  token_t token = peek_next_token(&lexer);

  while(token.kind != TOKEN_KIND_END) {
    assertm(before.cursor <= lexer.cursor, "Expected: previous lexer cursor to be smaller than current, "
            "Received: (previous = %zu, current = %zu)", before.cursor, lexer.cursor);

    ast_node_t node = {0};

    if (token.kind == TOKEN_KIND_WS || token.kind == TOKEN_KIND_NEWLINE) {
      get_next_token(&lexer); // consume WS or NEWLINE
    }
    else {
      switch(parser_choice) {
      case 0: {
        node = parse_expr(arena, &lexer, 0);
      } break;
      default: {
        // + 1's as bol, line, cursor are all zero-indexed
        bail("%s:%zu:%zu: Error: %s \"%c\"",
             path, error_line + 1, error_cursor - error_bol + 1, error_msg, input.buf[error_cursor]);
      } break;
      }

      if (has_err(node)) {
        size_t chars_consumed = lexer.cursor - before.cursor;

        // choose error from the node that was returned by the
        // sub-parser that consumed the most of the source input
        if (chars_consumed >= max_chars_consumed) {
          max_chars_consumed = chars_consumed;
          error_msg = node.err.msg;
          error_line = node.err.line;
          error_bol = node.err.bol;
          error_cursor = node.err.cursor;
        }

        reset_lexer(&lexer, before);
        parser_choice++;
      }
      else {
        da_push(arena, &statements, node);
        node = (ast_node_t){0};
        before = lexer;
        parser_choice = 0;
      }
    }

    token = peek_next_token(&lexer);
  }

  token = get_next_token(&lexer);
  assertm(token.kind == TOKEN_KIND_END, "Expected: TOKEN_KIND_END, Received: %s (%d)",
          token_kind_to_cstr(token.kind), token.kind);

  return statements;
}

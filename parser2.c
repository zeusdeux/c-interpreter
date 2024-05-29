#include <string.h>

#include "./zdx_util.h"
#include "./parser2.h"

// ------------------------------------ PRINT DEBUGGING ------------------------------------

const char *node_kind_name(const ast_node_kind_t kind)
{
  static const char *node_kind_to_str[] = {
    "AST_NODE_KIND_UNKNOWN",
    "AST_NODE_KIND_LIST",
    "AST_NODE_KIND_ERROR",
    "AST_NODE_KIND_LITERAL",
    "AST_NODE_KIND_SYMBOL",
    "AST_NODE_KIND_UNARY_OP",
    "AST_NODE_KIND_BINARY_OP",
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
    "UNARY_OP_DEREF",
    "UNARY_OP_NEGATE"
  };

  _Static_assert(zdx_arr_len(unary_kind_to_str) == UNARY_OP_COUNT,
                 "Some Unary op kinds are missing their corresponding strings in token to string map");

  assertm(kind < UNARY_OP_COUNT, "Invalid unary op kind %d", kind);

  return unary_kind_to_str[kind];
}

static const char *binary_kind_name(const binary_op_kind_t kind)
{
  static const char *binary_kind_to_str[] = {
    "BINARY_OP_UNKNOWN",
    "BINARY_OP_ADD",
    "BINARY_OP_SUB",
    "BINARY_OP_MULT",
    "BINARY_OP_DIV",
    "BINARY_OP_EXPO",
  };

  _Static_assert(zdx_arr_len(binary_kind_to_str) == BINARY_OP_COUNT,
                 "Some Binary op kinds are missing their corresponding strings in token to string map");

  assertm(kind < BINARY_OP_COUNT, "Invalid binary op kind %d", kind);

  return binary_kind_to_str[kind];
}

#define indent(depth) do {                      \
    for (size_t i = 0; i < (depth); i++) {      \
      fprintf(stderr, "   ");                   \
    }                                           \
  } while(0)

void print_ast_(const ast_node_t node, size_t depth)
{
  indent(depth);
  fprintf(stderr, "Node kind: %s\n", node_kind_name(node.kind));
  indent(depth);
  switch(node.kind) {
  case AST_NODE_KIND_ERROR: {
    fprintf(stderr, "message: %s\n", node.err.msg);
    indent(depth);
    fprintf(stderr, "line: %zu\n", node.err.line);
    indent(depth);
    fprintf(stderr, "cursor: %zu\n", node.err.cursor);
    indent(depth);
    fprintf(stderr, "bol: %zu\n", node.err.bol);
  } break;

  case AST_NODE_KIND_LITERAL: {
    fprintf(stderr, "Literal kind: %s\n", literal_kind_name(node.literal.kind));
    indent(depth);
    fprintf(stderr, "Value: "SV_FMT"\n", sv_fmt_args(node.literal.value));
  } break;

  case AST_NODE_KIND_SYMBOL: {
    fprintf(stderr, "Value: "SV_FMT"\n", sv_fmt_args(node.symbol.name));
  } break;

  case AST_NODE_KIND_UNARY_OP: {
    fprintf(stderr, "Op: %s\n", unary_kind_name(node.unary_op.kind));
    indent(depth);
    fprintf(stderr, "Expr:\n");
    print_ast_(*node.unary_op.expr, depth + 1);
  } break;

  case AST_NODE_KIND_BINARY_OP: {
    fprintf(stderr, "Op: %s\n", binary_kind_name(node.binary_op.kind));
    indent(depth);
    fprintf(stderr, "Left:\n");
    print_ast_(*node.binary_op.left, depth + 1);
    indent(depth);
    fprintf(stderr, "Right:\n");
    print_ast_(*node.binary_op.right, depth + 1);
  } break;

  case AST_NODE_KIND_LIST: {
    if (node.children) {
      fprintf(stderr, "Children: (length = %zu)\n", node.children->length);
      for (size_t i = 0; i < node.children->length; i++) {
        print_ast_(node.children->items[i], depth + 1);
      }
    } else {
      fprintf(stderr, "Children: None\n");
    }
  } break;

  default: assertm(false, "Missing case of ast node of kind %d", node.kind);
  }

  if (depth == 0) {
    fprintf(stderr, "--------------------\n");
  }
}


// ------------------------------------ COMBINATORS ------------------------------------

// ? op
static bool zero_or_one(lexer_t lexer[const static 1], token_kind_t kind, sv_t *const binding)
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
static bool zero_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  while(tok.kind == kind) {
    get_next_token(lexer);
    tok = peek_next_token(lexer);
  }

  return true;
}

// + op
static bool one_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  if (tok.kind != kind) {
    return false;
  }

  get_next_token(lexer);

  return zero_or_more(lexer, kind);
}

static bool exactly_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding)
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

bool is_next(const lexer_t *const lexer, token_kind_t kind)
{
  token_t token = peek_next_token(lexer);

  return token.kind == kind;
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
        .msg = "Unexpected character instead of valid literal",
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
        .msg = "Unexpected character instead of valid symbol",
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
  case TOKEN_KIND_MINUS: return UNARY_OP_NEGATE;
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
    TOKEN_KIND_MINUS,
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
        .msg = "Unexpected character instead of a unary op",
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
      .expr = arena_calloc(arena, 1, sizeof(expr))
    }
  };
  memcpy(node.unary_op.expr, &expr, sizeof(expr));

  return node;
}

static ast_node_t parse_parenthesized_expr(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  if (!exactly_one(lexer, TOKEN_KIND_OPAREN, NULL)) {
    return (ast_node_t){
      .kind = AST_NODE_KIND_ERROR,
      .err = {
        .msg = "Unexpected character instead of an opening paren",
        .line = lexer->line,
        .bol = lexer->bol,
        .cursor = lexer->cursor
      }
    };
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  ast_node_list_t *expr_list = NULL;

  // this check is to parse () with no expr in it as parse_expr
  // doesn't have a case for parse_empty() or such (epsilon in the grammar)
  if (!is_next(lexer, TOKEN_KIND_CPAREN)) {
    ast_node_t expr = parse_expr(arena, lexer, 0);

    while(!(has_err(expr))) {
      if (expr_list == NULL) {
        // allocated only if we have at least one expr
        expr_list = arena_calloc(arena, 1, sizeof(*expr_list));
        assertm(!arena->err, "Expected: expr list alloc to succeed, Received: %s", arena->err);
      }

      add_node(arena, expr_list, expr);

      zero_or_more(lexer, TOKEN_KIND_WS);

      if (!is_next(lexer, TOKEN_KIND_CPAREN) && !one_or_more(lexer, TOKEN_KIND_COMMA)) {
        break;
      }

      zero_or_more(lexer, TOKEN_KIND_WS);

      expr = parse_expr(arena, lexer, 0);
    }
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  /* while (one_or_more(lexer, TOKEN_KIND_WS) || one_or_more(lexer, TOKEN_KIND_NEWLINE)) {} */

  if (!exactly_one(lexer, TOKEN_KIND_CPAREN, NULL)){
    return (ast_node_t){
      .kind = AST_NODE_KIND_ERROR,
      .err = {
        .msg = "Unexpected character instead of an closing paren",
        .line = lexer->line,
        .cursor = lexer->cursor,
        .bol = lexer->bol,
      }
    };
  }

  ast_node_t node = {
    .kind = AST_NODE_KIND_LIST,
    .children = expr_list
  };

  return node;
}

static binary_op_kind_t get_binary_op_kind(const token_kind_t token_kind)
{
  switch(token_kind){
  case TOKEN_KIND_PLUS: return BINARY_OP_ADD;
  case TOKEN_KIND_MINUS: return BINARY_OP_SUB;
  case TOKEN_KIND_STAR: return BINARY_OP_MULT;
  case TOKEN_KIND_FSLASH: return BINARY_OP_DIV;
  default: return BINARY_OP_UNKNOWN;
  }
}



static ast_node_t parse_expr(arena_t arena[const static 1], lexer_t lexer[const static 1], size_t parser_choice)
{
  ast_node_t node = {0};

  size_t max_chars_consumed = 0;
  char *error_msg = "Unexpected error while parsing expression";
  size_t error_line = 0;
  size_t error_bol = 0;
  size_t error_cursor = 0;

  lexer_t before = *lexer;

  while(true) {
    switch(parser_choice) {
    case 0: {
      node = parse_parenthesized_expr(arena, lexer);
    } break;
    case 1: {
      node = parse_unary_op(arena, lexer);
    } break;
    case 2: {
      node = parse_symbol(arena, lexer);
    } break;
    case 3: {
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

ast_node_t parse(arena_t arena[const static 1], const char source[const static 1], const size_t source_length)
{
  ast_node_t program = {
    .kind = AST_NODE_KIND_LIST,
  };
  ast_node_list_t *statements = NULL;

  const sv_t input = sv_from_buf(source, source_length);
  lexer_t lexer = {
    .input = &input
  };

  size_t max_chars_consumed = 0;
  char *error_msg = "Unexpected error while parsing";
  size_t error_line = 0;
  size_t error_bol = 0;
  size_t error_cursor = 0;

  lexer_t before = lexer;
  uint8_t parser_choice = 0;

  token_t token = peek_next_token(&lexer);

  while(token.kind != TOKEN_KIND_END) {
    assertm(before.cursor <= lexer.cursor, "Expected: previous lexer cursor to be smaller than current, "
            "Received: (previous = %zu, current = %zu)", before.cursor, lexer.cursor);


    if (token.kind == TOKEN_KIND_WS || token.kind == TOKEN_KIND_NEWLINE) {
      get_next_token(&lexer); // consume WS or NEWLINE
    }
    else {
      ast_node_t node = {0};

      if (statements == NULL) {
        // allocate only when we are sure to have at least one node in it (here it's the default case error node)
        statements = arena_calloc(arena, 1, sizeof(*statements));
        assertm(!arena->err, "Expected: statement list alloc to succeed, Received: %s", arena->err);
        program.children = statements;
      }

      switch(parser_choice) {
      case 0: {
        node = parse_expr(arena, &lexer, 0);
      } break;
      default: {
        add_node(arena, statements, (ast_node_t){
            .kind = AST_NODE_KIND_ERROR,
            .err = {
              .msg = error_msg,
              .line = error_line,
              .bol = error_bol,
              .cursor = error_cursor
            }
          });
        return program;
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
        add_node(arena, statements, node);
        node = (ast_node_t){0};
        before = lexer;
        parser_choice = 0;
      }
    }

    token = peek_next_token(&lexer);
  }

  token = get_next_token(&lexer);
  assertm(token.kind == TOKEN_KIND_END, "Expected: TOKEN_KIND_END, Received: %s (%d)",
          token_kind_name(token.kind), token.kind);

  return program;
}

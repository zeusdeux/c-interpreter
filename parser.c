#include "./zdx_util.h"
#include "./zdx_da.h"

#include "./parser.h"

// ? op
bool zero_or_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding)
{
  token_t tok = peek_next_token(lexer);

  // zero match
  if (tok.kind != kind) {
    return true;
  }

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

  get_next_token(lexer);

  if (binding) {
    binding->buf = tok.value.buf; // buf is an address in lexer->input buffer so their lifetimes are bound
    binding->length = tok.value.length;
  }

  return true;
}

void print_ast(const ast_node_t node)
{
  (void) node;
  assertm(false, "TODO: Implement");
}

ast_node_t parse_assignment_statement(arena_t *const arena, lexer_t lexer[const static 1])
{
  (void) arena;

  zero_or_more(lexer, TOKEN_KIND_WS);

  sv_t storage_class = {0};
  if (exactly_one(lexer, TOKEN_KIND_STORAGE, &storage_class) && exactly_one(lexer, TOKEN_KIND_WS, NULL)) {
    zero_or_more(lexer, TOKEN_KIND_WS);
  }

  sv_t type_qualifier = {0};
  if (exactly_one(lexer, TOKEN_KIND_QUALIFIER, &type_qualifier) && exactly_one(lexer, TOKEN_KIND_WS, NULL)) {
    zero_or_more(lexer, TOKEN_KIND_WS);
  }

  sv_t symbols[128];
  size_t symbols_count = 0;
  while(true) {
    if (exactly_one(lexer, TOKEN_KIND_SYMBOL, &symbols[symbols_count]) && exactly_one(lexer, TOKEN_KIND_WS, NULL)) {
      zero_or_more(lexer, TOKEN_KIND_WS);
      symbols_count++;
    // TODO(mudit): else if handle presence of * in symbol or typename e.g., char* s or char *s;
    } else {
      break;
    }
  }

  exactly_one(lexer, TOKEN_KIND_EQL, NULL);
  zero_or_more(lexer, TOKEN_KIND_WS);

  sv_t value = {0};
  bool got_value = exactly_one(lexer, TOKEN_KIND_INT, &value) ||
    exactly_one(lexer, TOKEN_KIND_FLOAT, &value) ||
    exactly_one(lexer, TOKEN_KIND_STRING, &value) ||
    exactly_one(lexer, TOKEN_KIND_SYMBOL, &value);

  if (!got_value) {
    // return a failure
  }

  if (!exactly_one(lexer, TOKEN_KIND_SEMICOLON, NULL)) {
    // return a failure
  }

  log(L_INFO, "storage: "SV_FMT, sv_fmt_args(storage_class));
  log(L_INFO, "qualifier: "SV_FMT, sv_fmt_args(type_qualifier));
  for (size_t i = 0; i < symbols_count; i++) {
    log(L_INFO, "symbols: "SV_FMT, sv_fmt_args(symbols[i]));
  }
  log(L_INFO, "value: "SV_FMT, sv_fmt_args(value));

  assertm(false, "TODO: Implement");
  return (ast_node_t){0};
}
ast_node_t parse_call_statement(arena_t *const arena, lexer_t lexer[const static 1])
{
  (void) arena;
  (void) lexer;
  assertm(false, "TODO: Implement");
  return (ast_node_t){0};
}

// TODO(mudit): memoize lexer->cursor + parser function that was called as the key and the value as the result
// TODO(mudit): this function currently can only consume one statement and not more. Figure out if
// you want to while(true) this until all tokens are done or if you wanna use recursion somehow
ast_node_t parse_program(arena_t *const arena, lexer_t lexer[const static 1])
{
  ast_node_t program_node = { .kind = AST_NODE_KIND_PROGRAM };

  // save lexer state before calls to sub parsers
  lexer_t before = *lexer;
  ast_node_t node = parse_assignment_statement(arena, lexer);

  // we should probably not return here and below and instead
  // push a "statement" node into items of a program node
  if (node.kind != AST_NODE_KIND_EMPTY) {
    return node;
  }

  reset_lexer(lexer, before);
  node = parse_call_statement(arena, lexer);

  // we should probably not return here and below and instead
  // push a "statement" node into items of a program node
  if (node.kind != AST_NODE_KIND_EMPTY) {
    return node;
  }

  reset_lexer(lexer, before);

  node.kind = AST_NODE_KIND_UNKNOWN;
  node.err = "Unrecognized token";

  // do something with node like push into program_node.items[] which is basically statements
  // in a program

  return program_node;
}

ast_node_t parse(arena_t *const arena, const char source[const static 1], const size_t source_length)
{
  (void) arena;
  const sv_t input = sv_from_buf(source, source_length);
  lexer_t lexer = {0};
  lexer.input = &input;

  token_t tok;
  do {
    tok = peek_next_token(&lexer);
    /* print_token(tok); */
    parse_assignment_statement(arena, &lexer);
  } while (tok.kind != TOKEN_KIND_END);

  tok = get_next_token(&lexer); // consume the end token
  // TODO(mudit): Get token string instead of printing token.kind as an integer
  assertm(tok.kind == TOKEN_KIND_END, "Expected: all tokens to have been consumed, Received: %d", tok.kind);

  return (ast_node_t){0};
}

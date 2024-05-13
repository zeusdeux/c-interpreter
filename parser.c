#include "./zdx_util.h"
#include "./zdx_da.h"

#include "./parser.h"

void print_ast(const ast_node_t node)
{
  (void) node;
  assertm(false, "TODO: Implementation");
}

ast_node_t parse_assignment_statement(arena_t *const arena, const lexer_t lexer[const static 1])
{
  (void) arena;
  (void) lexer;
  assertm(false, "TODO: Impement");
  return (ast_node_t){0};
}
ast_node_t parse_call_statement(arena_t *const arena, const lexer_t lexer[const static 1])
{
  (void) arena;
  (void) lexer;
  assertm(false, "TODO: Impement");
  return (ast_node_t){0};
}

// TODO(mudit): memoize lexer->cursor + parser function that was called as the key and the value as the result
// TODO(mudit): this function currently can only consume one statement and not more. Figure out if
// you want to while(true) this until all tokens are done or if you wanna use recursion somehow
ast_node_t parse_program(arena_t *const arena, lexer_t lexer[const static 1])
{
  ast_node_t program_node = { .kind = AST_NODE_KIND_PROGRAM };

  lexer_t before = *lexer;
  ast_node_t node = parse_assignment_statement(arena, lexer);

  // we should probably not return here and below and instead
  // push a "statement" node into items of a program node
  if (node.kind != AST_NODE_KIND_EMPTY) {
    return node;
  }

  reset_lexer(lexer, before);
  node = parse_call_statement(arena, lexer);

  if (node.kind != AST_NODE_KIND_EMPTY) {
    return node;
  }

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
    tok = get_next_token(&lexer);
    print_token(tok);
  } while (tok.kind != TOKEN_KIND_END);

  return (ast_node_t){0};
}

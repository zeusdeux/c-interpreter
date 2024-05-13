#include "./zdx_util.h"
#include "./zdx_da.h"

#include "./lexer.h"
#include "./parser.h"

void print_ast(const ast_node_t node)
{
  (void) node;
  assertm(false, "TODO: Implementation");
}

ast_node_t parse(const char source[const static 1], const size_t source_length)
{
  token_t tok;
  lexer_t lexer = {0};
  sv_t input = sv_from_buf(source, source_length);
  lexer.input = &input;

  do {
    tok = get_next_token(&lexer);
    print_token(tok);
  } while (tok.kind != TOKEN_KIND_END);

  return (ast_node_t){0};
}

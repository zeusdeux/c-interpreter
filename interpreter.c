#include <stdio.h>
#include <stdlib.h>

#define ZDX_FILE_IMPLEMENTATION
#include "./zdx_file.h"

#include "./lexer.h"

// gcc -O2 -g -std=c17 -Wall -Wdeprecated -Wpedantic -Wextra -o interpreter interpreter.c lexer.c && ./interpreter
int main(void)
{
  fl_content_t fc = fl_read_file("./tests/mocks/main.c", "r");

  log(L_INFO, "File contents: \n%s", (char *)fc.contents);

  if (fc.err) {
    log(L_ERROR, "Error: %s", fc.err);
    return 1;
  }

  lexer_t lexer = {0};
  sv_t input = sv_from_buf(fc.contents, fc.size);

  lexer.input = &input;


  token_t tok;
  // lex loop (move into parse loop in parser)
  do {
    tok = get_next_token(&lexer);
    print_token(tok);

  } while (tok.kind != TOKEN_KIND_END);

  // walk ast and interpret

  fc_deinit(&fc);
  return 0;
}

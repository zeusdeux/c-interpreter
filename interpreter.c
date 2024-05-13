#include <stdio.h>
#include <stdlib.h>

#define ZDX_FILE_IMPLEMENTATION
#include "./zdx_file.h"

#include "./parser.h"

// gcc -O2 -g -std=c17 -Wall -Wdeprecated -Wpedantic -Wextra -o interpreter interpreter.c lexer.c parser.c && ./interpreter
int main(void)
{
  fl_content_t fc = fl_read_file("./tests/mocks/main.c", "r");

  log(L_INFO, "File contents: \n%s", (char *)fc.contents);

  if (fc.err) {
    log(L_ERROR, "Error: %s", fc.err);
    return 1;
  }

  // parse
  ast_node_t program = parse(fc.contents, fc.size);
  (void) program;

  // walk ast and interpret
  // TODO: interpret(program);

  fc_deinit(&fc);
  return 0;
}

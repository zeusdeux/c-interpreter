#include <stdio.h>
#include <stdlib.h>

#include "./parser.h"

#include "./zdx_util.h"

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "./zdx_simple_arena.h"

#define ZDX_FILE_IMPLEMENTATION
#define FL_ARENA_TYPE arena_t
#define FL_ALLOC arena_alloc
#define FL_FREE(...)
#include "./zdx_file.h"


// gcc -O2 -g -std=c17 -Wall -Wdeprecated -Wpedantic -Wextra -o interpreter interpreter.c lexer.c parser.c && ./interpreter
int main(void)
{
  // this will allocate 1 MB + extra bytes to align to page size boundary (4096 on Intel, 16384 on M1)
  arena_t arena = arena_create(1 MB);
  assertm(!arena.err, "Expected: arena creation to succeed, Received: %s", arena.err);
  log(L_INFO, "Arena size = %zu KB, used = %zu bytes", arena.size / 1024, arena.offset ? arena.offset - 1: 0);

  fl_content_t fc = fl_read_file(&arena, "./tests/mocks/assigments.c", "r");

  if (fc.err) {
    log(L_ERROR, "Error: %s", fc.err);
    return 1;
  }

  log(L_INFO, "File size = %zu bytes, contents: \n%s", fc.size, (char *)fc.contents);

  // parse
  ast_node_t program = parse(&arena, fc.contents, fc.size);
  print_ast(program);

  // walk ast and interpret
  // TODO: interpret(program);

  log(L_INFO, "Arena size = %zu KB, used = %zu bytes", arena.size / 1024, arena.offset ? arena.offset - 1: 0);

  // don't really need to deinit as the arena that'd holding the file bytes is freed next anyway
  fc_deinit(&fc);
  arena_free(&arena);
  return 0;
}

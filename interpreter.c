#include <stdio.h>
#include <stdlib.h>

#include "./parser2.h"

#include "./zdx_util.h"

#define ZDX_SIMPLE_ARENA_IMPLEMENTATION
#include "./zdx_simple_arena.h"

#define ZDX_FILE_IMPLEMENTATION
#define FL_ARENA_TYPE arena_t
#define FL_ALLOC arena_alloc
#define FL_FREE(...)
#include "./zdx_file.h"


// gcc -O2 -g -std=c17 -Wall -Wdeprecated -Wpedantic -Wextra -o interpreter interpreter.c lexer.c parser2.c && ./interpreter
int main(int argc, char *argv[])
{
  if (argc < 2) {
    bail("Usage: ./interpreter <path to file to interpret>");
  }
  // this will allocate 1 MB + extra bytes to align to page size boundary (4096 on Intel, 16384 on M1)
  arena_t arena = arena_create(1 MB);
  assertm(!arena.err, "Expected: arena creation to succeed, Received: %s", arena.err);
  size_t arena_used_bytes = arena.offset ? arena.offset - 1: 0;
  log(L_INFO, "Arena size = %zu KB, used = %zu bytes", arena.size / 1024, arena_used_bytes);

  fl_content_t fc = fl_read_file(&arena, argv[1], "r");

  if (fc.err) {
    log(L_ERROR, "Error: %s", fc.err);
    return 1;
  }

  log(L_INFO, "File size = %zu bytes, path: %s, contents: \n%s", fc.size, argv[1], (char *)fc.contents);

  // parse
  ast_node_t program = parse(&arena, fc.contents, fc.size);
  check_program(program);
  ast_node_t last_node = program.children->items[program.children->length - 1];

  // the parser stops collection statements when an error occurs and
  // therefore, if there was a parse error, it'll be the last node
  // in the program statement list aka program.children
  if (has_err(last_node)) {
    char char_at_cursor[2] = {((char *)fc.contents)[last_node.err.cursor], 0};
    fprintf(stderr, "%s:%zu:%zu: Error: %s -> '%s'\n",
            fc.path, last_node.err.line + 1, last_node.err.cursor - last_node.err.bol + 1, last_node.err.msg,
            char_at_cursor[0] == '\n' ? "\\n" : char_at_cursor);
  } else {
    print_ast(program);
  }

  // walk ast and interpret
  // TODO: interpret(program);

  arena_used_bytes = arena.offset ? arena.offset - 1: 0;
  log(L_INFO, "Arena size = %zu KB, used = %zu bytes, used by file = %zu bytes, used by parser = %zu bytes",
      arena.size / 1024, arena_used_bytes, fc.size, arena_used_bytes - fc.size);

  // don't really need to deinit as the arena that'd holding the file bytes is freed next anyway
  fc_deinit(&fc);
  arena_free(&arena);
  return 0;
}

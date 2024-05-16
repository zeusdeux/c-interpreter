#ifndef PARSER_H_
#define PARSER_H_

#include <stddef.h>

#include "./lexer.h"
#include "./zdx_simple_arena.h"


typedef enum {
  AST_NODE_KIND_EMPTY,
  AST_NODE_KIND_UNKNOWN,
  AST_NODE_KIND_PROGRAM,
  AST_NODE_KIND_DECLARATION,
  AST_NODE_KIND_ASSIGNMENT,
  AST_NODE_KIND_DECLARATION_AND_ASSIGNMENT,
  AST_NODE_KIND_CALL,
  AST_NODE_KIND_COUNT
} ast_node_kind_t;

typedef struct ast_node_t {
  ast_node_kind_t kind;
  size_t length;
  size_t capacity;
  struct ast_node_t *items;
  const char *err;
} ast_node_t;

void print_ast(const ast_node_t node);
ast_node_t parse(arena_t *const arena, const char source[const static 1], const size_t source_length);

/**
 * Combinators
 */
// ? op
bool zero_or_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding);
// * op
bool zero_or_more(lexer_t *const lexer, token_kind_t kind);
// + op
bool one_or_more(lexer_t *const lexer, token_kind_t kind);
bool exactly_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding);

/**
 * Grammar:
 *
 * Program =
 *     AssignmentStatement | CallStatement
 *
 * AssignmentStatement =
 *     ws* (<storage> ws+)? <qualifier>? ws+ <symbol> ws+ <star>? ws* <symbol> ws* <eql> ws* <Expression> ws* <semi>
 *
 * CallStatement =
 *     ws* <symbol> ws* <oparen> ws* <Args> ws* <cparen> ws* <semi>
 *
 * Args =
 *     Empty | <Expression> ws* (<comma> ws* <Expression> ws*)*
 *
 */

ast_node_t parse_program(arena_t *const arena, lexer_t lexer[const static 1]);
ast_node_t parse_assignment_statement(arena_t *const arena, lexer_t lexer[const static 1]);
ast_node_t parse_call_statement(arena_t *const arena, lexer_t lexer[const static 1]);
#endif // PARSER_H_

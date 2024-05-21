#ifndef PARSER_H_
#define PARSER_H_

#include <stddef.h>

#include "./lexer.h"
#include "./zdx_simple_arena.h"

#define PARSE_ASSIGNMENT_STATEMENT_CACHE_LENGTH 128

typedef enum {
  AST_NODE_KIND_UNKNOWN,
  AST_NODE_KIND_PROGRAM,
  AST_NODE_KIND_DECLARATION,
  AST_NODE_KIND_ASSIGNMENT,
  AST_NODE_KIND_CALL,
  AST_NODE_KIND_COUNT
} ast_node_kind_t;

typedef struct ast_node_t {
  ast_node_kind_t kind;
  union {
    struct {
      sv_t storage_class;
      sv_t type_qualifier;
      struct {
        size_t length;
        sv_t *typenames;
      } datatype;
      sv_t identifier;
      struct {
        size_t length;
        sv_t *qualifiers;
      } dereference;
      bool addr_of_op;
      token_kind_t value_kind;
      union {
        unsigned int unsigned_integer;
        sv_t string;
        sv_t symbol;
      } value;
    } assignment_stmt;
  };
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

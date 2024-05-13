#ifndef PARSER_H_
#define PARSER_H_

#include "./lexer.h"

/**
 * Grammar:
 *
 * Program =
 *     AssignmentStatement | CallStatement
 *
 * AssignmentStatement =
 *     ws* <qualifier>? ws+ <symbol> ws+ <star>? ws* <symbol> ws* <eql> ws* <Expression> ws* <semi>
 *
 * CallStatement =
 *     ws* <symbol> ws* <oparen> ws* <Args> ws* <cparen> ws* <semi>
 *
 * Args =
 *     Empty | <Expression> ws* (<comma> ws* <Expression> ws*)*
 *
 */

typedef enum {
  AST_NODE_KIND_EMPTY,
  AST_NODE_KIND_UNKNOWN,
  AST_NODE_KIND_PROGRAM,
  AST_NODE_KIND_COUNT
} ast_node_kind_t;

typedef struct {
  ast_node_kind_t kind;
  ast_node_t *children;
  const char *err;
} ast_node_t;

ast_node_t parse(const char source[const static 1]);

#endif // PARSER_H_

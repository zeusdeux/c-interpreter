#ifndef PARSER_H_
#define PARSER_H_

#include <stddef.h>

#include "./lexer.h"

#include "./zdx_simple_arena.h"

#define DA_REALLOC arena_realloc
#define DA_FREE(...)
#include "./zdx_da.h"


typedef enum {
  AST_NODE_KIND_UNKNOWN,
  AST_NODE_KIND_ERROR,
  AST_NODE_KIND_LITERAL,
  AST_NODE_KIND_SYMBOL,
  AST_NODE_KIND_UNARY_OP,
  AST_NODE_KIND_COUNT,
} ast_node_kind_t;

typedef enum {
  LITERAL_KIND_UNKNOWN,
  LITERAL_KIND_NUMBER,
  LITERAL_KIND_STRING,
  LITERAL_KIND_BOOL,
  LITERAL_KIND_COUNT,
} literal_kind_t;

typedef enum {
  UNARY_OP_UNKNOWN,
  UNARY_OP_ADDR_OF,
  UNARY_OP_DEREF,
  UNARY_OP_COUNT,
} unary_op_kind_t;

typedef struct ast_node_t {
  ast_node_kind_t kind;

  union {
    struct {
      char *msg;
      size_t line;
      size_t bol;
      size_t cursor;
    } err;

    struct {
      literal_kind_t kind;
      sv_t value;
    } literal;

    struct {
      sv_t name;
    } symbol;

    struct {
      unary_op_kind_t kind;
      struct ast_node_t *expr;
    } unary_op;
  };
} ast_node_t;

typedef struct {
  size_t capacity;
  size_t length;
  ast_node_t *items;
} ast_node_list_t;

#define has_err(node) (node).kind == AST_NODE_KIND_ERROR

#define print_ast(node) print_ast_((node), 0);
void print_ast_(const ast_node_t node, size_t depth);

ast_node_list_t parse(arena_t arena[const static 1], const char source[const static 1], const size_t source_length);

#endif // PARSER_H_

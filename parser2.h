#ifndef PARSER_H_
#define PARSER_H_

#include <stddef.h>

#include "./lexer.h"

#include "./zdx_simple_arena.h"

typedef enum {
  AST_NODE_KIND_UNKNOWN,
  AST_NODE_KIND_LIST,
  AST_NODE_KIND_ERROR,
  AST_NODE_KIND_LITERAL,
  AST_NODE_KIND_SYMBOL,
  AST_NODE_KIND_UNARY_OP,
  AST_NODE_KIND_BINARY_OP,
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
  UNARY_OP_NEGATE,
  UNARY_OP_COUNT,
} unary_op_kind_t;

typedef enum {
  BINARY_OP_UNKNOWN,
  BINARY_OP_ADD,
  BINARY_OP_SUB,
  BINARY_OP_MULT,
  BINARY_OP_DIV,
  BINARY_OP_EXPO,
  BINARY_OP_COUNT,
} binary_op_kind_t;

typedef struct ast_node_list_t ast_node_list_t;

typedef struct ast_node_t {
  ast_node_kind_t kind;

  union {
    struct ast_node_list_t *children;

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
      binary_op_kind_t kind;
      struct ast_node_t *left;
      struct ast_node_t *right;
    } binary_op;

    struct {
      unary_op_kind_t kind;
      struct ast_node_t *expr;
    } unary_op;
  };
} ast_node_t;

struct ast_node_list_t {
  size_t capacity;
  size_t length;
  ast_node_t *items;
};

#define has_err(node) (node).kind == AST_NODE_KIND_ERROR
#define check_program(program)                                  \
  assertm((program).kind == AST_NODE_KIND_LIST,                 \
          "Expected: Program node, Received: %s (%d)",          \
          node_kind_name((program).kind), (program).kind)


#define NODE_ASSERT assertm
#define NODE_REALLOC arena_realloc
#define NODE_LIST_MIN_CAP 1

#define add_node(arena, arr, ...) add_node_(arena,                         \
                                            arr,                           \
                                            ((ast_node_t[]){__VA_ARGS__}), \
                                            zdx_arr_len(((ast_node_t[]){__VA_ARGS__})))

#define add_node_(arena, arr, els, reqd_cap)                                                                                    \
  do {                                                                                                                          \
    size_t orig_cap = (arr)->capacity;                                                                                          \
                                                                                                                                \
    while ((arr)->capacity < (reqd_cap) + (arr)->length) {                                                                      \
      (arr)->capacity = zdx_max((arr)->capacity, NODE_LIST_MIN_CAP);                                                            \
      (arr)->capacity *= 2;                                                                                                     \
    }                                                                                                                           \
                                                                                                                                \
    if ((arr)->capacity != orig_cap) {                                                                                          \
      (arr)->items = NODE_REALLOC((arena), (arr)->items, (arr)->length*sizeof(ast_node_t), (arr)->capacity*sizeof(ast_node_t)); \
      NODE_ASSERT(!(arena)->err, "Expected: node list resize to be successful, Received: %s", (arena)->err);                    \
    }                                                                                                                           \
                                                                                                                                \
    for (size_t i = 0; i < (reqd_cap); i++) {                                                                                   \
      (arr)->items[(arr)->length++] = (els)[i];                                                                                 \
    }                                                                                                                           \
  } while(0)

#define print_ast(node) print_ast_((node), 0);
void print_ast_(const ast_node_t node, size_t depth);
const char *node_kind_name(const ast_node_kind_t kind);

ast_node_t parse(arena_t arena[const static 1], const char source[const static 1], const size_t source_length);

#endif // PARSER_H_

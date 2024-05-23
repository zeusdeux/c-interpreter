#include "./zdx_util.h"
#include "./zdx_da.h"

#include "./parser.h"


// ------------------------------------ PRINT DEBUGGING ------------------------------------

static const char *node_kind_to_cstr[] = {
  "AST_NODE_KIND_UNKNOWN",
  "AST_NODE_KIND_PROGRAM",
  "AST_NODE_KIND_DECLARATION",
  "AST_NODE_KIND_ASSIGNMENT",
  "AST_NODE_KIND_CALL",
};
_Static_assert(zdx_arr_len(node_kind_to_cstr) == AST_NODE_KIND_COUNT,
               "Some AST node kinds are missing their corresponding strings in token to string map");

void print_ast(const ast_node_t node)
{
  log(L_INFO, "Node kind: %s", node_kind_to_cstr[node.kind]);
  switch(node.kind) {
  case AST_NODE_KIND_ASSIGNMENT: {
    log(L_INFO, "Storage class: "SV_FMT, sv_fmt_args(node.assignment_stmt.storage_class));
    log(L_INFO, "Type qualifier: "SV_FMT, sv_fmt_args(node.assignment_stmt.type_qualifier));
    log(L_INFO, "Typenames count: %zu", node.assignment_stmt.datatype.count);
    for (size_t i = 0; i < node.assignment_stmt.datatype.count; i++) {
      log(L_INFO, "Typename: "SV_FMT, sv_fmt_args(node.assignment_stmt.datatype.typenames[i]));
    }
    log(L_INFO, "Deref count: %zu", node.assignment_stmt.dereference.count);
    for (size_t i = 0; i < node.assignment_stmt.dereference.count; i++) {
      log(L_INFO, "Deref qualifier: "SV_FMT, sv_fmt_args(node.assignment_stmt.dereference.qualifiers[i]));
    }
    log(L_INFO, "Identifier: "SV_FMT, sv_fmt_args(node.assignment_stmt.identifier));
    log(L_INFO, "Address-of op (&) in use? %s", node.assignment_stmt.addr_of_op ? "true" : "false");
    log(L_INFO, "Value kind: %s", token_kind_to_cstr(node.assignment_stmt.value_kind));

    switch(node.assignment_stmt.value_kind) {
    case TOKEN_KIND_UNSIGNED_INT: {
      log(L_INFO, "Value: %u", node.assignment_stmt.value.unsigned_integer);
    } break;
    case TOKEN_KIND_STRING: {
      log(L_INFO, "Value: "SV_FMT", length: %zu", sv_fmt_args(node.assignment_stmt.value.string), node.assignment_stmt.value.string.length);
    } break;
    case TOKEN_KIND_SYMBOL: {
      log(L_INFO, "Value: "SV_FMT", length: %zu", sv_fmt_args(node.assignment_stmt.value.symbol), node.assignment_stmt.value.symbol.length);
    } break;
    default: assertm(false, "Unsupported value type in assignment ast node %d", node.assignment_stmt.value_kind);
    }
  } break;

  case AST_NODE_KIND_CALL: {
    log(L_INFO, "Function name: "SV_FMT, sv_fmt_args(node.call_stmt.name));
    for (size_t i = 0; i < node.call_stmt.args.count; i++) {
      log(L_INFO, "Arg: "SV_FMT, sv_fmt_args(node.call_stmt.args.values[i]));
    }
  } break;

  case AST_NODE_KIND_UNKNOWN: {
    if (node.err) {
      log(L_INFO, "Err: %s", node.err);
    }
  } break;
  default: assertm(false, "Missing case of ast node of kind %d", node.kind);
  }

  log(L_INFO, "--------------------");
}


// ------------------------------------ COMBINATORS ------------------------------------

// ? op
bool zero_or_one(lexer_t lexer[const static 1], token_kind_t kind, sv_t *const binding)
{
  token_t tok = peek_next_token(lexer);

  // zero match
  if (tok.kind != kind) {
    return true;
  }

  // one match
  get_next_token(lexer);

  if (binding) {
    binding->buf = tok.value.buf; // buf is an address in lexer->input buffer so their lifetimes are bound
    binding->length = tok.value.length;
  }

  return true;
}

// * op
bool zero_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  while(tok.kind == kind) {
    get_next_token(lexer);
    tok = peek_next_token(lexer);
  }

  return true;
}

// + op
bool one_or_more(lexer_t *const lexer, token_kind_t kind)
{
  token_t tok = peek_next_token(lexer);

  if (tok.kind != kind) {
    return false;
  }

  get_next_token(lexer);

  return zero_or_more(lexer, kind);
}

bool exactly_one(lexer_t *const lexer, token_kind_t kind, sv_t *binding)
{
  token_t tok = peek_next_token(lexer);

  if (tok.kind != kind) {
    return false;
  }

  tok = get_next_token(lexer);

  if (binding) {
    binding->buf = tok.value.buf; // buf is an address in lexer->input buffer so their lifetimes are bound
    binding->length = tok.value.length;
  }

  return true;
}


// ------------------------------------ SUB-PARSERS ------------------------------------

ast_node_t parse_expr(arena_t arena[const static 1], lexer_t lexer[const static 1])
{

}

ast_node_t parse_assignment_statement(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  sv_t storage_class = {0};
  exactly_one(lexer, TOKEN_KIND_STORAGE, &storage_class) && one_or_more(lexer, TOKEN_KIND_WS);

  sv_t type_qualifier = {0};
  exactly_one(lexer, TOKEN_KIND_QUALIFIER, &type_qualifier) && one_or_more(lexer, TOKEN_KIND_WS);

  sv_t datatypes[128];
  size_t datatypes_count = 0;
  while(exactly_one(lexer, TOKEN_KIND_SYMBOL, &datatypes[datatypes_count]) && one_or_more(lexer, TOKEN_KIND_WS)) {
    datatypes_count++;
  }

  size_t star_count = 0;
  sv_t star_qualifiers[128] = {0};
  size_t star_qualifiers_count = 0;
  while(exactly_one(lexer, TOKEN_KIND_STAR, NULL) &&
        zero_or_more(lexer, TOKEN_KIND_WS) &&
        zero_or_one(lexer, TOKEN_KIND_QUALIFIER, &star_qualifiers[star_qualifiers_count++]) &&
        zero_or_more(lexer, TOKEN_KIND_WS)) {
    star_count++;
  }

  if (star_count != star_qualifiers_count) {
    ast_node_t node = {0};
    node.err = "Number of qualifiers attached to the * (star) op do not match the number of star ops";

    return node;
  }

  sv_t var_name = {0};
  if (star_count) {
    exactly_one(lexer, TOKEN_KIND_SYMBOL, &var_name);
    one_or_more(lexer, TOKEN_KIND_WS);
  } else {
    if (!datatypes_count) {
      ast_node_t node = {0};
      node.err = "No datatypes/identifiers found";

      return node;
    }

    // if no '*' found, then the variable name as been collected
    // into the datatypes array so we pop the last element out
    // as the variable name
    var_name = datatypes[datatypes_count - 1];
    datatypes_count--;
  }

  if (sv_is_empty(var_name)) {
    ast_node_t node = {0};
    node.err = "No variable name found";

    return node;
  }

  if (!exactly_one(lexer, TOKEN_KIND_EQL, NULL)) {
    ast_node_t node = {0};
    node.err = "No '=' token found";

    return node;
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  sv_t addr_of_op = {0};
  zero_or_one(lexer, TOKEN_KIND_AMPERSAND, &addr_of_op);

  sv_t value = {0};
  token_kind_t value_kind = {0};
  token_kind_t value_kinds[] = {
    TOKEN_KIND_SIGNED_INT,
    TOKEN_KIND_UNSIGNED_INT,
    TOKEN_KIND_FLOAT,
    TOKEN_KIND_STRING,
    TOKEN_KIND_SYMBOL,
  };
  bool found_value = false;
  for (size_t i = 0; i < zdx_arr_len(value_kinds); i++) {
    token_kind_t kind = value_kinds[i];

    if (exactly_one(lexer, kind, &value)) {
      value_kind = kind;
      found_value = true;
      break;
    }
  }

  if (!found_value) {
    ast_node_t node = {0};
    node.err = "No value assigned to variable";

    return node;
  }

  if (!one_or_more(lexer, TOKEN_KIND_SEMICOLON)) {
    ast_node_t node = {0};
    node.err = "Missing semicolon at the end of assignment statement";

    return node;
  }

  dbg("storage: "SV_FMT, sv_fmt_args(storage_class));
  dbg("qualifier: "SV_FMT, sv_fmt_args(type_qualifier));
  dbg("datatypes count: %zu", datatypes_count);
  for (size_t i = 0; i < datatypes_count; i++) {
    dbg("datatypes: "SV_FMT, sv_fmt_args(datatypes[i]));
  }
  dbg("stars: %zu", star_count);
  for(size_t i = 0; i < star_qualifiers_count; i++) {
    dbg("star %zu qualifier: "SV_FMT, i, sv_fmt_args(star_qualifiers[i]));
  }
  dbg("variable name: "SV_FMT, sv_fmt_args(var_name));
  dbg("address-of operator(&): %s", addr_of_op.buf && addr_of_op.length ? "true" : "false");
  dbg("value: "SV_FMT" kind: %s", sv_fmt_args(value), token_kind_to_cstr(value_kind));
  dbg("--------------------");

  // Build assignment_statement AST node
  // TODO(mudit): Use arena based string builder where string forms of sv_t are needed
  // TODO(mudit): Error handling
  ast_node_t node = {0};
  node.kind = AST_NODE_KIND_ASSIGNMENT;

  if (!sv_is_empty(storage_class)) {
    node.assignment_stmt.storage_class = storage_class;
  }

  if (!sv_is_empty(type_qualifier)) {
    node.assignment_stmt.type_qualifier = type_qualifier;
  }

  // Move datatypes off stack into heap as it's an array
  sv_t *typenames = arena_alloc(arena, sizeof(*typenames) * datatypes_count);
  for (size_t i = 0; i < datatypes_count; i++) {
    typenames[i] = datatypes[i];
  }
  node.assignment_stmt.datatype.typenames = typenames;
  node.assignment_stmt.datatype.count = datatypes_count;

  sv_t *dereferences_qualifiers = arena_alloc(arena, sizeof(*dereferences_qualifiers) * star_qualifiers_count);
  for (size_t i = 0; i < star_qualifiers_count; i++) {
    dereferences_qualifiers[i] = star_qualifiers[i];
  }
  node.assignment_stmt.dereference.count = star_count;
  node.assignment_stmt.dereference.qualifiers = dereferences_qualifiers;

  node.assignment_stmt.addr_of_op = !sv_is_empty(addr_of_op);
  node.assignment_stmt.identifier = var_name;

  node.assignment_stmt.value_kind = value_kind;
  if (value_kind == TOKEN_KIND_UNSIGNED_INT) {
    char *value_cstr = arena_calloc(arena, value.length + 1, sizeof(*value_cstr));
    snprintf(value_cstr, value.length + 1, SV_FMT, sv_fmt_args(value));
    // TODO(mudit): Error handling and support for hex, octal, etc values and bases (gotta add to lexer first)
    // TODO(mudit): Implement an sv_to_ul to get rid of this allocation as it can be freed after we get
    // the return value from strtoul()
    node.assignment_stmt.value.unsigned_integer = strtoul(value_cstr, NULL, 10);
  } else if (value_kind == TOKEN_KIND_STRING) {
    node.assignment_stmt.value.string = value;
  } else if (value_kind == TOKEN_KIND_SYMBOL) {
    node.assignment_stmt.value.symbol = value;
  } else {
    assertm(false, "Unsupported value kind %d", value_kind);
  }
  // TODO(mudit): Add support for other value types

  return node;
}

ast_node_t parse_call_statement(arena_t arena[const static 1], lexer_t lexer[const static 1])
{
  sv_t fn_name = {0};
  if(!exactly_one(lexer, TOKEN_KIND_SYMBOL, &fn_name)) {
    ast_node_t node = {0};
    node.err = "Missing function name";

    return node;
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  if (!exactly_one(lexer, TOKEN_KIND_OPAREN, NULL)) {
    ast_node_t node = {0};
    node.err = "Missing opening parenthesis";

    return node;
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  // TODO(mudit): Parse stuff between parens
  /* ast_node_t expr_node = parse_expr(arena, lexer); */
  /* if (expr_node.err) { */
  /*   return expr_node; */
  /* } */

  zero_or_more(lexer, TOKEN_KIND_WS);

  if (!exactly_one(lexer, TOKEN_KIND_CPAREN, NULL)) {
    ast_node_t node = {0};
    node.err = "Missing closing parenthesis";

    return node;
  }

  zero_or_more(lexer, TOKEN_KIND_WS);

  one_or_more(lexer, TOKEN_KIND_SEMICOLON);

  ast_node_t node = {0};
  node.kind = AST_NODE_KIND_CALL;
  node.call_stmt.name = fn_name;

  return node;
}

// TODO(mudit): this function currently can only consume one statement and not more. Figure out if
// you want to while(true) this until all tokens are done or if you wanna use recursion somehow
ast_node_t parse(arena_t arena[const static 1], const char source[const static 1], const size_t source_length)
{
  const sv_t input = sv_from_buf(source, source_length);
  lexer_t lexer = {0};
  lexer.input = &input;

  uint8_t parser_choice = 0;
  token_t tok = peek_next_token(&lexer);
  char *last_error = NULL;

  while (tok.kind != TOKEN_KIND_END) {
    if (tok.kind == TOKEN_KIND_WS || tok.kind == TOKEN_KIND_NEWLINE) {
      tok = get_next_token(&lexer); // consume whitespace or newline
    } else {
      lexer_t before = lexer;
      ast_node_t node = {0};

      switch(parser_choice) {
      case 0: {
        node = parse_assignment_statement(arena, &lexer);
      } break;
      case 1: {
        node = parse_call_statement(arena, &lexer);
      } break;
      default: {
        bail("Parse error: %s", last_error);
      } break;
      }

      if (node.err) {
        reset_lexer(&lexer, before);
        // TODO(mudit): Instead of showing the error from the last parser, show error from the parser
        // that had the longest match
        last_error = (char *)node.err; // casting to get rid of the "const" qualifier used by node.err
        parser_choice++;
      } else {
        parser_choice = 0;
        print_ast(node);
      }
    }

    tok = peek_next_token(&lexer);
  }

  tok = get_next_token(&lexer); // consume the end token
  // TODO(mudit): Get token string instead of printing token.kind as an integer
  assertm(tok.kind == TOKEN_KIND_END, "Expected: all tokens to have been consumed, Received: %s", token_kind_to_cstr(tok.kind));

  return (ast_node_t){0};
}

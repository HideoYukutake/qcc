#include "qcc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Abstract Syntax Tree
/* Syntax
 *
 * program              = variable_declaration*
 *                      | function_declaration*
 *                      | function_definition*
 * type                 = "int"
 * variable_declaration = type "*"? identifier ";"
 * function_declaration = type ident "(" (primary ("," primary)*)? ")" ";"
 * function_definition  = type ident "(" (primary ("," primary)*)? ")" block
 * stmt                 = expr ";"
 *                      | block
 *                      | "if" "(" expr ")" stmt ( "else" stmt )?
 *                      | "while" "(" expr ")" stmt
 *                      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *                      | "return" expr ";"
 * block                = "{" variable_declaration* stmt* "}"
 * expr                 = assign
 * assign               = equality ("=" assign)?
 * equality             = relational ("==" relational | "!=" relational)*
 * relational           = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add                  = mul ("+" mul | "-" mul)*
 * mul                  = unary ("*" unary | "/" unary)*
 * unary                = ("+" | "-")? primary
 *                      | "*" unary
 *                      | "&" unary
 * primary              = num
 *                      | ident ("(" (primary ("," primary)*)?  ")")?
 *                      | "(" expr ")"
 *
 */

void program() {
  int i = 0;

  while (!at_eof()) {
    code[i++] = function();
  }
  code[i] = NULL;
}

Node *function() {
  Node *node;
  LVar *locals;

  Token *tok = consume_type();
  if (!tok) {
    error_at(token->str, "expected int.");
  }

  tok = consume_identifier();
  if (!tok) {
    error_at(token->str, "関数でなければなりません");
  }
  // 関数名の設定
  node = calloc(1, sizeof(Node));
  node->kind = ND_FUNCTION;
  node->name = (char *)calloc(tok->len + 1, sizeof(char));
  strncpy(node->name, tok->str, (size_t)tok->len);
  node->name[tok->len] = '\0';

  // ローカル変数格納リストの用意
  locals = calloc(1, sizeof(LVar));
  locals->name = "";
  locals->offset = 0;
  locals->next = NULL;
  node->locals = locals;

  if (!consume("(")) {
    error_at(token->str, "引数の文法エラーです");
  }
  // 引数の処理
  if (!consume(")")) {
    Compounds *params = calloc(1, sizeof(Compounds));
    node->comp = params;
    params->stmt = NULL;
    Compounds *param = calloc(1, sizeof(Compounds));
    param->stmt = expr(locals);
    params->next = param;
    params = param;
    while (!consume(")")) {
      consume(",");
      Compounds *param = calloc(1, sizeof(Compounds));
      param->stmt = expr(locals);
      params->next = param;
      params = param;
    }
    params->next = NULL;
  }

  tok = consume_reserved();
  if (tok->kind == TK_BLOCK_START) {
    node->lhs = block(locals);
  } else {
    error_at(tok->str, "'{'ではないトークンです。");
  }
  return node;
}

Node *stmt(LVar *locals) {
  Node *node;
  Token *tok = consume_reserved();
  if (tok) {
    node = calloc(1, sizeof(Node));
    switch (tok->kind) {
    case TK_RETURN:
      node->kind = ND_RETURN;
      node->lhs = expr(locals);
      break;
    case TK_IF:
      node->kind = ND_IF;
      consume("(");
      node->cond = expr(locals);
      consume(")");
      node->lhs = stmt(locals);
      tok = consume_else();
      if (tok) {
        node->rhs = stmt(locals);
      }
      return node;
    case TK_WHILE:
      node->kind = ND_WHILE;
      consume("(");
      node->cond = expr(locals);
      consume(")");
      node->lhs = stmt(locals);
      return node;
    case TK_FOR:
      node->kind = ND_FOR;
      consume("(");
      if (!consume(";")) {
        node->init = expr(locals);
        consume(";");
      }
      if (!consume(";")) {
        node->cond = expr(locals);
        consume(";");
      }
      if (!consume(")")) {
        node->step = expr(locals);
        consume(")");
      }
      node->lhs = stmt(locals);
      return node;
    case TK_BLOCK_START:
      node = block(locals);
      return node;
    }
  } else {
    node = expr(locals);
  }

  if (!consume(";")) {
    error_at(token->str, "';'ではないトークンです。");
  }
  return node;
}

Node *variable_declaration() {
  Node *node;
  LVar *locals;

  Token *tok = consume_type();
  if (!tok) {
    error_at(token->str, "expected int.");
  }
}

Node *block(LVar *locals) {
  Node *node;
  node = calloc(1, sizeof(Node));
  node->kind = ND_BLOCK;
  Compounds *comps = calloc(1, sizeof(Compounds));
  node->comp = comps;
  comps->stmt = NULL;
  while (!consume_block_end()) {
    Compounds *compound = calloc(1, sizeof(Compounds));
    compound->stmt = stmt(locals);
    compound->next = NULL;
    comps->next = compound;
    comps = compound;
  }
  return node;
}

Node *expr(LVar *locals) { return assign(locals); }

Node *assign(LVar *locals) {
  Node *node = equality(locals);
  if (consume("=")) {
    node = new_binary(ND_ASSIGN, node, assign(locals));
  }
  return node;
}

Node *equality(LVar *locals) {
  Node *node = relational(locals);

  for (;;) {
    if (consume("==")) {
      node = new_binary(ND_EQ, node, relational(locals));
    } else if (consume("!=")) {
      node = new_binary(ND_NE, node, relational(locals));
    } else {
      return node;
    }
  }
}

Node *relational(LVar *locals) {
  Node *node = add(locals);

  for (;;) {
    if (consume("<")) {
      node = new_binary(ND_LT, node, add(locals));
    } else if (consume("<=")) {
      node = new_binary(ND_LE, node, add(locals));
    } else if (consume(">")) {
      node = new_binary(ND_LT, add(locals), node);
    } else if (consume(">=")) {
      node = new_binary(ND_LE, add(locals), node);
    } else {
      return node;
    }
  }
}

Node *add(LVar *locals) {
  Node *node = mul(locals);

  for (;;) {
    if (consume("+")) {
      node = new_binary(ND_ADD, node, mul(locals));
    } else if (consume("-")) {
      node = new_binary(ND_SUB, node, mul(locals));
    } else {
      return node;
    }
  }
}

Node *unary(LVar *locals) {
  if (consume("+")) {
    return primary(locals);
  }
  if (consume("-")) {
    return new_binary(ND_SUB, new_node_num(0), primary(locals));
  }
  if (consume("&")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_ADDR;
    node->lhs = unary(locals);
    return node;
  }
  if (consume("*")) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_DEREF;
    node->lhs = unary(locals);
    return node;
  }
  return primary(locals);
}

Node *primary(LVar *locals) {
  // ( expr ) に対応する処理
  if (consume("(")) {
    Node *node = expr(locals);
    expect(")");
    return node;
  }

  // identifier に対応する処理
  Token *tok = consume_identifier();
  if (tok) {
    if (consume("(")) {
      // 関数呼び出しの場合
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_FUNCTION_CALL;
      node->name = tok->str;
      node->name[tok->len] = '\0';
      if (consume(")")) {
        // 引数なしの場合
        return node;
      } else {
        // 引数ありの場合
        Compounds *params = calloc(1, sizeof(Compounds));
        node->comp = params;
        params->stmt = NULL;
        Compounds *param = calloc(1, sizeof(Compounds));
        param->stmt = expr(locals);
        params->next = param;
        params = param;
        while (!consume(")")) {
          consume(",");
          Compounds *param = calloc(1, sizeof(Compounds));
          param->stmt = expr(locals);
          params->next = param;
          params = param;
        }
        params->next = NULL;
        return node;
      }
    } else {
      // ローカル変数の場合
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_LVAR;
      LVar *lvar = find_lvar(locals, tok);
      if (lvar) {
        node->offset = lvar->offset;
      } else {
        lvar = calloc(1, sizeof(LVar));
        lvar->name = calloc(tok->len + 1, sizeof(char));
        strncpy(lvar->name, tok->str, tok->len);
        lvar->name[tok->len] = '\0';
        lvar->offset = 8;
        if (locals->next) {
          lvar->offset = locals->next->offset + 8;
        }
        node->offset = lvar->offset;
        lvar->next = locals->next;
        locals->next = lvar;
      }
      return node;
    }
  }

  // num に対応する処理
  return new_node_num(expect_number());
}

Node *mul(LVar *locals) {
  Node *node = unary(locals);

  for (;;) {
    if (consume("*")) {
      node = new_binary(ND_MUL, node, unary(locals));
    } else if (consume("/")) {
      node = new_binary(ND_DIV, node, unary(locals));
    } else {
      return node;
    }
  }
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

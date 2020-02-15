#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qcc.h"

// Utility
void error_at(char *loc, char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);

        int pos = loc - user_input;
        fprintf(stderr, "%s\n", user_input);
        fprintf(stderr, "%*s", pos, "");
        fprintf(stderr, "^ ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
}

void error(char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);

        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        exit(1);
}

// Tokenizer
bool at_eof()
{
        return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
        Token *tok = calloc(1, sizeof(Token));
        tok->kind = kind;
        tok->str = str;
        tok->len = len;
        cur->next = tok;
        return tok;
}

bool startswith(char *p, char *q)
{
        return memcmp(p, q, strlen(q)) == 0;
}

Token *tokenize(char *p)
{
        Token head;
        head.next = NULL;
        Token *cur = &head;

        while (*p) {
                if (isspace(*p)) {
                        p++;
                        continue;
                }

                if (startswith(p, "==") || startswith(p, "!=") ||
                    startswith(p, "<=") || startswith(p, ">=")) {
                        cur = new_token(TK_RESERVED, cur, p, 2);
                        p += 2;
                        continue;
                }

                if (startswith(p, "=")) {
                        cur = new_token(TK_RESERVED, cur, p++, 1);
                        continue;
                }

                // Single-letter punctuator
                if (strchr("+-*/()><", *p)) {
                        cur = new_token(TK_RESERVED, cur, p++, 1);
                        continue;
                }

                if (*p == ';') {
                        cur = new_token(TK_RESERVED, cur, p++, 1);
                        continue;
                }

                if (isdigit(*p)) {
                        cur = new_token(TK_NUM, cur, p, 0);
                        char *q = p;
                        cur->val = strtol(p, &p, 10);
                        cur->len = p - q;
                        continue;
                }

                if ('a' <= *p && *p <= 'z') {
                        cur = new_token(TK_IDENT, cur, p++, 1);
                        continue;
                }

                error_at(token->str, "トークナイズできません");
        }

        new_token(TK_EOF, cur, p, 0);
        return head.next;
}

bool consume(char *op)
{
        if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            memcmp(token->str, op, token->len)) {
                return false;
        }
        token = token->next;
        return true;
}

Token *consume_ident()
{
        Token *tok;
        if (token->kind == TK_IDENT) {
                token = token->next;
        }
        return tok;
}

void expect(char *op)
{
        if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            memcmp(token->str, op, token->len)) {
                error_at(token->str, "'%c'ではありません", op);
        }
        token = token->next;
}

int expect_number()
{
        if (token->kind != TK_NUM) {
                error_at(token->str, "数ではありません");
        }
        int val = token->val;
        token = token->next;
        return val;
}

// Abstract Syntax Tree
/* Syntax
 *
 * program    = stmt*
 * stmt       = expr ";"
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 * primary    = num | ident | "(" expr ")"
 *
 */


void program() {
        int i = 0;
        while (!at_eof()) {
                code[i++] = stmt();
        }
        code[i] = NULL;
}

Node *stmt()
{
        Node *node = expr();
        expect(";");
        return node;
}

Node *expr()
{
        return assign();
}

Node *assign()
{
        Node *node = equality();
        if (consume("=")) {
                node = new_binary(ND_ASSIGN, node, assign());
        }
        return node;
}

Node *equality()
{
        Node *node = relational();

        for (;;) {
                if (consume("==")) {
                        node = new_binary(ND_EQ, node, relational());
                } else if (consume("!=")) {
                        node = new_binary(ND_NE, node, relational());
                } else {
                        return node;
                }
        }
}

Node *relational()
{
        Node *node = add();

        for (;;) {
                if (consume("<")) {
                        node = new_binary(ND_LT, node, add());
                } else if (consume("<=")) {
                        node = new_binary(ND_LE, node, add());
                } else if (consume(">")) {
                        node = new_binary(ND_LT, add(), node);
                } else if (consume(">=")) {
                        node = new_binary(ND_LE, add(), node);
                } else {
                        return node;
                }
        }
}

Node *add()
{
        Node *node = mul();

        for (;;) {
                if (consume("+")) {
                        node = new_binary(ND_ADD, node, mul());
                } else if (consume("-")) {
                        node = new_binary(ND_SUB, node, mul());
                } else {
                        return node;
                }
        }
}

Node *unary()
{
        if (consume("+")) {
                return primary();
        }
        if (consume("-")) {
                return new_binary(ND_SUB, new_node_num(0), primary());
        }
        return primary();
}

Node *primary()
{
        // ( expr ) に対応する処理
        if (consume("(")) {
                Node *node = expr();
                expect(")");
                return node;
        }

        // ident に対応する処理
        if (token->kind == TK_IDENT) {
                Node *node = calloc(1, sizeof(Node));
                node->kind = ND_LVAR;
                // 変数aはRBP-8, RBP-16 のように固定位置にある
                node->offset = (token->str[0] - 'a' + 1) * 8;
                token = token->next;
                return node;
        }

        // num に対応する処理
        return new_node_num(expect_number());

}

Node *mul()
{
        Node *node = unary();

        for (;;) {
                if (consume("*")) {
                        node = new_binary(ND_MUL, node, unary());
                } else if (consume("/")) {
                        node = new_binary(ND_DIV, node, unary());
                } else {
                        return node;
                }
        }

}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
        Node *node = calloc(1, sizeof(Node));
        node->kind = kind;
        node->lhs = lhs;
        node->rhs = rhs;
        return node;
}

Node *new_node_num(int val)
{
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_NUM;
        node->val = val;
        return node;
}


#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qcc.h"

LVar *locals;

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

int is_alphabet(char c)
{
        return ('a'<=c && c<='z') ||
               ('A'<=c && c<='Z') ||
               (c == '_');
}

int is_alnum(char c)
{
        return ('0'<=c && c<='9') || is_alphabet(c);
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

                if (*p == '{') {
                        cur = new_token(TK_BLOCK_START, cur, p++, 1);
                        continue;
                }

                if (*p == '}') {
                        cur = new_token(TK_BLOCK_END, cur, p++, 1);
                        continue;
                }

                if (isdigit(*p)) {
                        cur = new_token(TK_NUM, cur, p, 0);
                        char *q = p;
                        cur->val = strtol(p, &p, 10);
                        cur->len = p - q;
                        continue;
                }

                if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
                        cur = new_token(TK_RETURN, cur, p, 6);
                        p += 6;
                        continue;
                }

                if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
                        cur = new_token(TK_IF, cur, p, 2);
                        p += 2;
                        continue;
                }

                if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
                        cur = new_token(TK_ELSE, cur, p, 4);
                        p += 4;
                        continue;
                }

                if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
                        cur = new_token(TK_WHILE, cur, p, 5);
                        p += 5;
                        continue;
                }

                if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
                        cur = new_token(TK_FOR, cur, p, 3);
                        p += 3;
                        continue;
                }

                if (is_alphabet(*p)) {
                        char *q = p;
                        while (is_alnum(*p)) {
                                p++;
                        }
                        cur = new_token(TK_IDENT, cur, q, p-q);
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
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
}

Token *consume_return()
{
        Token *tok;
        if (token->kind == TK_RETURN) {
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
}

Token *consume_reserved()
{
        Token *tok;
        if (token->kind == TK_RETURN ||
            token->kind == TK_IF ||
            token->kind == TK_ELSE ||
            token->kind == TK_FOR ||
            token->kind == TK_BLOCK_START ||
            token->kind == TK_WHILE) {
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
}

Token *consume_else()
{
        Token *tok;
        if (token->kind == TK_ELSE) {
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
}

Token *consume_block()
{
        Token *tok;
        if (token->kind == TK_BLOCK_START) {
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
}

Token *consume_block_end()
{
        Token *tok;
        if (token->kind == TK_BLOCK_END) {
                tok = token;
                token = token->next;
                return tok;
        }
        return NULL;
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

LVar *find_lvar(Token *tok)
{
        LVar *var;
        for (var = locals; var; var = var->next) {
                if (var->len == tok->len &&
                    !memcmp(tok->str, var->name, var->len)) {
                        return var;
                }
        }
        return NULL;
}

// Abstract Syntax Tree
/* Syntax
 *
 * program    = stmt*
 * stmt       = expr ";"
 *            | "{" stmt* "}"
 *            | "if" "(" expr ")" stmt ( "else" stmt )?
 *            | "while" "(" expr ")" stmt
 *            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *            | "return" expr ";"
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 * primary    = num
 *            | ident ("(" (primary "," primary)?  ")")?
 *            | "(" expr ")"
 *
 */


void program() {
        int i = 0;
        locals = calloc(1, sizeof(LVar));
        locals->len = 0;
        locals->name = "";
        locals->offset = 0;
        locals->next = NULL;

        while (!at_eof()) {
                code[i++] = stmt();
        }
        code[i] = NULL;
}

Node *stmt()
{
        Node *node;
        Token *tok = consume_reserved();
        if (tok) {
                node = calloc(1, sizeof(Node));
                switch (tok->kind) {
                        case TK_RETURN:
                                node->kind = ND_RETURN;
                                node->lhs = expr();
                                break;
                        case TK_IF:
                                node->kind = ND_IF;
                                consume("(");
                                node->cond = expr();
                                consume(")");
                                node->lhs = stmt();
                                tok = consume_else();
                                if (tok) {
                                        node->rhs = stmt();
                                }
                                return node;
                        case TK_WHILE:
                                node->kind = ND_WHILE;
                                consume("(");
                                node->cond = expr();
                                consume(")");
                                node->lhs = stmt();
                                return node;
                        case TK_FOR:
                                node->kind = ND_FOR;
                                consume("(");
                                if (!consume(";")) {
                                        node->init = expr();
                                        consume(";");
                                }
                                if (!consume(";")) {
                                        node->cond = expr();
                                        consume(";");
                                }
                                if (!consume(")")) {
                                        node->step = expr();
                                        consume(")");
                                }
                                node->lhs = stmt();
                                return node;
                        case TK_BLOCK_START:
                                node->kind = ND_BLOCK;
                                Compounds *comps = calloc(1, sizeof(Compounds));
                                node->comp = comps;
                                comps->stmt = NULL;
                                while (!consume_block_end()) {
                                        Compounds *block = calloc(1, sizeof(Compounds));
                                        block->stmt = stmt();
                                        block->next = NULL;
                                        comps->next = block;
                                        comps = block;
                                }
                                return node;
                }
        } else {
                node = expr();
        }

        if (!consume(";")) {
                error_at(token->str, "';'ではないトークンです。");
        }
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
        Token *tok = consume_ident();
        if (tok) {
                if (consume("(")) {
                        // 関数呼び出しの場合
                        Node *node = calloc(1, sizeof(Node));
                        node->kind = ND_FUNCTION_CALL;
                        node->name = tok->str;
                        node->name[tok->len] = '\0';
                        consume(")");
                        return node;
                } else {
                        // ローカル変数の場合
                        Node *node = calloc(1, sizeof(Node));
                        node->kind = ND_LVAR;
                        LVar *lvar = find_lvar(tok);
                        if (lvar) {
                                node->offset = lvar->offset;
                        } else {
                                lvar = calloc(1, sizeof(LVar));
                                lvar->next = locals;
                                lvar->name = tok->str;
                                lvar->len = tok->len;
                                lvar->offset = locals->offset + 8;
                                node->offset = lvar->offset;
                                locals = lvar;
                        }
                        return node;
                }
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

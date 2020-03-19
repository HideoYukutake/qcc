#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "qcc.h"

char *filename;

// Utility
void error_at(char *loc, char *fmt, ...)
{
        char *line = loc;
        while (user_input < line && line[-1] != '\n') {
                line--;
        }

        char *end = loc;
        while (*end != '\n' && token->kind != TK_EOF) {
                end++;
        }

        int line_num = 1;
        for (char *p = user_input; p < line; p++) {
                if (*p == '\n') {
                        line_num++;
                }
        }

        int indent = fprintf(stderr, "%s:%d ", filename, line_num);
        fprintf(stderr, "%.*s\n", (int)(end - line), line);

        int pos = loc - line + indent;
        fprintf(stderr, "%*s", pos, "");
        fprintf(stderr, "^ %s\n", fmt);
        //vfprintf(stderr, fmt, ap);
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
                if (strchr("&+-*/()><,", *p)) {
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

LVar *find_lvar(LVar *locals, Token *tok)
{
        LVar *var;
        for (var = locals; var; var = var->next) {
                if (!memcmp(tok->str, var->name, tok->len)) {
                        return var;
                }
        }
        return NULL;
}

// Abstract Syntax Tree
/* Syntax
 *
 * program    = function*
 * function   = ident "(" (primary ("," primary)*)? ")" block
 * stmt       = expr ";"
 *            | block
 *            | "if" "(" expr ")" stmt ( "else" stmt )?
 *            | "while" "(" expr ")" stmt
 *            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
 *            | "return" expr ";"
 * block      = "{" stmt* "}"
 * expr       = assign
 * assign     = equality ("=" assign)?
 * equality   = relational ("==" relational | "!=" relational)*
 * relational = add ("<" add | "<=" add | ">" add | ">=" add)*
 * add        = mul ("+" mul | "-" mul)*
 * mul        = unary ("*" unary | "/" unary)*
 * unary      = ("+" | "-")? primary
 *            | "*" unary
 *            | "&" unary
 * primary    = num
 *            | ident ("(" (primary ("," primary)*)?  ")")?
 *            | "(" expr ")"
 *
 */


void program()
{
        int i = 0;

        while (!at_eof()) {
                code[i++] = function();
        }
        code[i] = NULL;
}

Node *function()
{
        Node *node;
        LVar *locals;

        Token *tok = consume_ident();
        if (!tok) {
                error_at(token->str, "関数でなければなりません");
        }
        // 関数名の設定
        node = calloc(1, sizeof(Node));
        node->kind = ND_FUNCTION;
        node->name = (char *)calloc(tok->len+1, sizeof(char));
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

Node *stmt(LVar *locals)
{
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

Node *block(LVar *locals)
{
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

Node *expr(LVar *locals)
{
        return assign(locals);
}

Node *assign(LVar *locals)
{
        Node *node = equality(locals);
        if (consume("=")) {
                node = new_binary(ND_ASSIGN, node, assign(locals));
        }
        return node;
}

Node *equality(LVar *locals)
{
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

Node *relational(LVar *locals)
{
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

Node *add(LVar *locals)
{
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

Node *unary(LVar *locals)
{
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

Node *primary(LVar *locals)
{
        // ( expr ) に対応する処理
        if (consume("(")) {
                Node *node = expr(locals);
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
                                lvar->name = calloc(tok->len+1, sizeof(char));
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

Node *mul(LVar *locals)
{
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

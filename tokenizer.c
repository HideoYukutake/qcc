#include "qcc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *filename;

// Utility
void error_at(char *loc, char *fmt, ...) {
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
  // vfprintf(stderr, fmt, ap);
  exit(1);
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Tokenizer
bool at_eof() { return token->kind == TK_EOF; }

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q) { return memcmp(p, q, strlen(q)) == 0; }

int is_alphabet(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) { return ('0' <= c && c <= '9') || is_alphabet(c); }

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") ||
        startswith(p, ">=")) {
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

    if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_TYPE, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_alphabet(*p)) {
      char *q = p;
      while (is_alnum(*p)) {
        p++;
      }
      cur = new_token(TK_IDENTIFIER, cur, q, p - q);
      continue;
    }

    error_at(token->str, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    return false;
  }
  token = token->next;
  return true;
}

Token *consume_type() {
  Token *tok;
  if (token->kind == TK_TYPE) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_identifier() {
  Token *tok;
  if (token->kind == TK_IDENTIFIER) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_return() {
  Token *tok;
  if (token->kind == TK_RETURN) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_reserved() {
  Token *tok;
  if (token->kind == TK_RETURN || token->kind == TK_IF ||
      token->kind == TK_ELSE || token->kind == TK_FOR ||
      token->kind == TK_BLOCK_START || token->kind == TK_WHILE ||
      token->kind == TK_TYPE) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_else() {
  Token *tok;
  if (token->kind == TK_ELSE) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_block() {
  Token *tok;
  if (token->kind == TK_BLOCK_START) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

Token *consume_block_end() {
  Token *tok;
  if (token->kind == TK_BLOCK_END) {
    tok = token;
    token = token->next;
    return tok;
  }
  return NULL;
}

void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    error_at(token->str, "'%c'ではありません", op);
  }
  token = token->next;
}

int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "数ではありません");
  }
  int val = token->val;
  token = token->next;
  return val;
}

LVar *find_lvar(LVar *locals, Token *tok) {
  LVar *var;
  for (var = locals; var; var = var->next) {
    if (!memcmp(tok->str, var->name, tok->len)) {
      return var;
    }
  }
  return NULL;
}

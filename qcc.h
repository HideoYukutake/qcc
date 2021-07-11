#include <stdbool.h>

extern char *filename;

// tokenizer
/* トークンの種類 */
typedef enum {
  TK_RESERVED,   // 記号
  TK_NUM,        // 整数トークン
  TK_IDENTIFIER, // 識別子（変数など）
  TK_TYPE,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_EOF,
  TK_BLOCK_START,
  TK_BLOCK_END,
} TokenKind;

typedef struct _Token Token;
struct _Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

/* 変数を格納する連結リスト */
typedef struct _LVar LVar;
struct _LVar {
  LVar *next;
  char *name;
  int offset;
};

extern Token *token;
extern char *user_input;
extern LVar *locals;

/* 関数宣言 */
void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
bool startswith(char *p, char *q);
int is_alphabet(char c);
int is_alnum(char c);
Token *tokenize(char *p);
bool consume(char *op);
Token *consume_type();
Token *consume_identifier();
Token *consume_return();
Token *consume_reserved();
Token *consume_else();
Token *consume_block();
Token *consume_block_end();
void expect(char *op);
int expect_number();
LVar *find_lvar(LVar *locals, Token *tok);

// parser
/*! \enum NodeKind
 *
 *  抽象構文木のノードの種類
 */
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_EQ,
  ND_NE,
  ND_LT,
  ND_LE,
  ND_ASSIGN,
  ND_LVAR,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_FUNCTION_CALL,
  ND_FUNCTION,
  ND_ADDR,
  ND_DEREF,
} NodeKind;

typedef struct _Compounds Compounds;
typedef struct _Node Node;

/* 抽象構文木のノードの型 */
struct _Node {
  NodeKind kind;   /* ノードの型 */
  char *name;      /* 関数の場合のみ */
  int len;         /* name の長さ*/
  LVar *locals;    /* ローカル変数のリスト。関数の場合のみ */
  Node *cond;      /* if,while,forの場合のみ */
  Node *init;      /* forの場合のみ */
  Node *step;      /* forの場合のみ */
  Node *lhs;       /* 左辺 */
  Node *rhs;       /* 右辺 */
  Compounds *comp; /* Blockの場合のみ */
  int val;         /* kindがND_NUMの場合のみ */
  int offset;      /* kindがND_LVARの場合のみ */
};

/* Detailed description */
struct _Compounds {
  Compounds *next;
  Node *stmt;
};

extern Node *code[100];

/* 関数宣言 */
void program();
Node *function();
Node *stmt(LVar *locals);
Node *block(LVar *locals);
Node *expr(LVar *locals);
Node *assign(LVar *locals);
Node *equality(LVar *locals);
Node *relational(LVar *locals);
Node *add(LVar *locals);
Node *unary(LVar *locals);
Node *primary(LVar *locals);
Node *mul(LVar *locals);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);

// generator
void generate(Node *node);

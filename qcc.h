#include <stdbool.h>

//
/*! \enum TokenKind
 *
 *  トークンの種類
 */
typedef enum {
        TK_RESERVED,
        TK_NUM,
        TK_IDENT,     // 識別子（変数など）
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

typedef struct Token Token;


/*! \struct Token
 *  \brief Brief struct description
 *
 *  Detailed description
 */
struct Token {
        TokenKind kind;  // トークンの型
        Token *next;     // 次の入力トークン
        int val;         // kindがTK_NUMの場合、その数値
        char *str;       // トークン文字列
        int len;         // トークンの長さ
};

/*! \struct qcc_t
 *  \brief Brief struct description
 *
 *  変数を格納する連結リスト
 */
typedef struct _LVar LVar;
struct _LVar {
        LVar *next;
        char *name;
        int offset;
};

/*! \enum NodeKind
 *
 *  抽象構文木のノードの種類
 */
typedef enum
{
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
typedef struct Node Node;

/*! \struct Node
 *  \brief Brief struct description
 *
 *  抽象構文木のノードの型
 */
struct Node {
        NodeKind kind;      /* ノードの型 */
        char *name;         /* 関数の場合のみ */
        int len;            /* name の長さ*/
        LVar *locals;       /* ローカル変数のリスト。関数の場合のみ */
        Node *cond;         /* if,while,forの場合のみ */
        Node *init;         /* forの場合のみ */
        Node *step;         /* forの場合のみ */
        Node *lhs;          /* 左辺 */
        Node *rhs;          /* 右辺 */
        Compounds *comp;    /* Blockの場合のみ */
        int val;            /* kindがND_NUMの場合のみ */
        int offset;         /* kindがND_LVARの場合のみ */
};

/*! \struct Compounds
 *  \brief Brief struct description
 *
 *  Detailed description
 */
struct _Compounds {
        Compounds *next;  /*!< Description */
        Node *stmt;
} /* optional variable list */;

extern Token *token;
extern char *user_input;
extern Node *code[100];
extern LVar *locals;
extern char *filename;

void program();
Node *function();
Node *stmt(LVar *locals);
Node *block(LVar *locals);
Node *assign(LVar *locals);
Node *expr(LVar *locals);
Node *mul(LVar *locals);
Node *primary(LVar *locals);
Node *unary(LVar *locals);
Node *equality(LVar *locals);
Node *relational(LVar *locals);
Node *add(LVar *locals);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
bool consume(char *op);
Token *consume_ident();
void expect(char *op);
int expect_number();

void gen(Node *node);

Token *tokenize(char *p);

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

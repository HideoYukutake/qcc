#include <stdbool.h>

//
/*! \enum TokenKind
 *
 *  トークンの種類
 */
typedef enum {
        TK_RESERVED,
        TK_NUM,
        TK_IDENT,
        TK_RETURN,
        TK_EOF,
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
        int len;
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
} NodeKind;

typedef struct Node Node;

/*! \struct Node
 *  \brief Brief struct description
 *
 *  抽象構文木のノードの型
 */
struct Node {
        NodeKind kind; /* ノードの型 */
        Node *lhs;     /* 左辺 */
        Node *rhs;     /* 右辺 */
        int val;       /* kindがND_NUMの場合のみ */
        int offset;    /* kindがND_LVARの場合のみ */
};


extern Token *token;
extern char *user_input;
extern Node *code[100];
extern LVar *locals;

void program();
Node *stmt();
Node *assign();
Node *expr();
Node *mul();
Node *primary();
Node *unary();
Node *equality();
Node *relational();
Node *add();
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

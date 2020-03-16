#include <stdio.h>
#include "qcc.h"

long unique;
long stack_counter;

void gen_lval(Node *node) {
        if (node->kind != ND_LVAR) {
                error("代入の左辺値が変数ではありません");
        }

        printf("    mov rax, rbp\n");
        printf("    sub rax, %d\n", node->offset);
        printf("    push rax\n");
}

void gen(Node *node)
{
        Compounds *c;
        int argc;
        if (!node) {
                return;
        }

        switch (node->kind) {
                case ND_NUM:
                        printf("    push %d\n", node->val);
                        return;
                case ND_LVAR:
                        gen_lval(node);
                        printf("    pop rax\n");
                        printf("    mov rax, [rax]\n");
                        printf("    push rax\n");
                        return;
                case ND_ASSIGN:
                        gen_lval(node->lhs);
                        gen(node->rhs);
                        printf("    pop rdi\n");
                        printf("    pop rax\n");
                        printf("    mov [rax], rdi\n");
                        printf("    push rdi\n");
                        return;
                case ND_RETURN:
                        gen(node->lhs);
                        printf("    pop rax\n");
                        printf("    mov rsp, rbp\n");
                        printf("    pop rbp\n");
                        printf("    ret\n");
                        return;
                case ND_IF:
                        gen(node->cond);
                        printf("    pop rax\n");
                        printf("    cmp rax, 0\n");
                        if (node->rhs) {
                                printf("    je .Lelse%ld\n", unique);
                                gen(node->lhs);
                                printf("    jmp .Lend%ld\n", unique);
                                printf(".Lelse%ld:\n", unique);
                                gen(node->rhs);
                        } else {
                                printf("    je .Lend%ld\n", unique);
                                gen(node->lhs);
                        }
                        printf(".Lend%ld:\n", unique);
                        unique++;
                        return;
                case ND_WHILE:
                        printf(".Lbegin%ld:\n", unique);
                        gen(node->cond);
                        printf("    pop rax\n");
                        printf("    cmp rax, 0\n");
                        printf("    je .Lend%ld\n", unique);
                        gen(node->lhs);
                        printf("    jmp .Lbegin%ld\n", unique);
                        printf(".Lend%ld:\n", unique);
                        unique++;
                        return;
                case ND_FOR:
                        gen(node->init);
                        printf(".Lbegin%ld:\n", unique);
                        gen(node->cond);
                        printf("    pop rax\n");
                        printf("    cmp rax, 0\n");
                        printf("    je .Lend%ld\n", unique);
                        gen(node->lhs);
                        gen(node->step);
                        printf("    jmp .Lbegin%ld\n", unique);
                        printf(".Lend%ld:\n", unique);
                        unique++;
                        return;
                case ND_FUNCTION:
                        printf("%s:\n", node->name);

                        // Prologue(関数呼び出しの際の定型の命令)
                        printf("    push rbp\n");
                        printf("    mov rbp, rsp\n");

                        c = node->comp;
                        argc = 0;
                        if (c) {
                                c = c->next;
                        }
                        while (c) {
                                switch (argc) {
                                        case 0:
                                                // TODO
                                                // rdi にある値をローカル変数として定義する
                                                printf("    push rdi\n");
                                                break;
                                        case 1:
                                                // TODO
                                                // rsi にある値をローカル変数として定義する
                                                printf("    push rsi\n");
                                                break;
                                        case 2:
                                                // TODO
                                                // rdx にある値をローカル変数として定義する
                                                printf("    push rdx\n");
                                                break;
                                        case 3:
                                                // TODO
                                                // rcx にある値をローカル変数として定義する
                                                printf("    push rcx\n");
                                                break;
                                        default:
                                                printf("    push rax\n");
                                }
                                argc++;
                                c = c->next;
                        }

                        printf("    sub rsp, %d\n", 8*node->locals->len);

                        gen(node->lhs);

                        // Epilogue(関数の末尾に出力する定型の命令)
                        printf("    mov rsp, rbp\n");
                        printf("    pop rbp\n");
                        printf("    ret\n");
                        return;
                case ND_BLOCK:
                        c  = node->comp;
                        while ((c = c->next)) {
                                gen(c->stmt);
                                printf("    pop rax\n");
                        }
                        return;
                case ND_FUNCTION_CALL:
                        /*
                        if ((stack_counter % 2) == 1) {
                                printf("    sub rsp, 0x18\n");
                        }
                        printf("    mov rax, [rsp]\n");
                        printf("    mov rbx, 16\n");
                        printf("    div rbx\n");
                        printf("    cmp rdx, 0\n");
                        printf("    je .LfuncN%s\n", node->name);
                        printf("    sub rsp, 8\n");
                        printf(".LfuncN%s:\n", node->name);
                        */
                        c = node->comp;
                        argc = 0;
                        if (c) {
                                c = c->next;
                        }
                        while (c) {
                                switch (argc) {
                                        case 0:
                                                gen(c->stmt);
                                                printf("    pop rax\n");
                                                printf("    mov rdi, rax\n");
                                                break;
                                        case 1:
                                                gen(c->stmt);
                                                printf("    pop rax\n");
                                                printf("    mov rsi, rax\n");
                                                break;
                                        case 2:
                                                gen(c->stmt);
                                                printf("    pop rax\n");
                                                printf("    mov rdx, rax\n");
                                                break;
                                        case 3:
                                                gen(c->stmt);
                                                printf("    pop rax\n");
                                                printf("    mov rcx, rax\n");
                                                break;
                                        default:
                                                gen(c->stmt);
                                                printf("    pop rax\n");
                                                printf("    push rax\n");
                                }
                                argc++;
                                c = c->next;
                        }
                        printf("    call %s\n", node->name);
                        printf("    push rax\n");
                        return;
        }

        gen(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");

        switch (node->kind) {
                case ND_ADD:
                        printf("    add rax, rdi\n");
                        break;
                case ND_SUB:
                        printf("    sub rax, rdi\n");
                        break;
                case ND_MUL:
                        printf("    imul rax, rdi\n");
                        break;
                case ND_DIV:
                        printf("    cqo\n");
                        printf("    idiv rdi\n");
                        break;
                case ND_EQ:
                        printf("    cmp rax, rdi\n");
                        printf("    sete al\n");
                        printf("    movzb rax, al\n");
                        break;
                case ND_NE:
                        printf("    cmp rax, rdi\n");
                        printf("    setne al\n");
                        printf("    movzb rax, al\n");
                        break;
                case ND_LT:
                        printf("    cmp rax, rdi\n");
                        printf("    setl al\n");
                        printf("    movzb rax, al\n");
                        break;
                case ND_LE:
                        printf("    cmp rax, rdi\n");
                        printf("    setle al\n");
                        printf("    movzb rax, al\n");
                        break;
                case ND_NUM:
                        // 何もしない
                        break;
        }

        printf("    push rax\n");
}

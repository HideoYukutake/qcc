#include "qcc.h"
#include <stdio.h>

long unique;
long stack_counter;

void generate_lval(Node *node) {
  if (node->kind != ND_LVAR) {
    error("代入の左辺値が変数ではありません");
  }

  printf("    mov rax, rbp\n");
  printf("    sub rax, %d\n", node->offset);
  printf("    push rax\n");
}

void generate(Node *node) {
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
    generate_lval(node);
    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
    return;
  case ND_ADDR:
    generate_lval(node->lhs);
    return;
  case ND_DEREF:
    generate(node->lhs);
    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
    return;
  case ND_ASSIGN:
    generate_lval(node->lhs);
    generate(node->rhs);
    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    mov [rax], rdi\n");
    printf("    push rdi\n");
    return;
  case ND_RETURN:
    generate(node->lhs);
    printf("    pop rax\n");
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
  case ND_IF:
    generate(node->cond);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    if (node->rhs) {
      printf("    je .Lelse%ld\n", unique);
      generate(node->lhs);
      printf("    jmp .Lend%ld\n", unique);
      printf(".Lelse%ld:\n", unique);
      generate(node->rhs);
    } else {
      printf("    je .Lend%ld\n", unique);
      generate(node->lhs);
    }
    printf(".Lend%ld:\n", unique);
    unique++;
    return;
  case ND_WHILE:
    printf(".Lbegin%ld:\n", unique);
    generate(node->cond);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    je .Lend%ld\n", unique);
    generate(node->lhs);
    printf("    jmp .Lbegin%ld\n", unique);
    printf(".Lend%ld:\n", unique);
    unique++;
    return;
  case ND_FOR:
    generate(node->init);
    printf(".Lbegin%ld:\n", unique);
    generate(node->cond);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");
    printf("    je .Lend%ld\n", unique);
    generate(node->lhs);
    generate(node->step);
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
        printf("    push rdi\n");
        break;
      case 1:
        printf("    push rsi\n");
        break;
      case 2:
        printf("    push rdx\n");
        break;
      case 3:
        printf("    push rcx\n");
        break;
      default:
        printf("    push rax\n");
      }
      argc++;
      c = c->next;
    }

    int i = 0;
    for (LVar *lvar = node->locals->next; lvar; lvar = lvar->next) {
      i++;
    }
    printf("    sub rsp, %d\n", 8 * i);

    generate(node->lhs);

    // Epilogue(関数の末尾に出力する定型の命令)
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
  case ND_BLOCK:
    c = node->comp;
    while ((c = c->next)) {
      generate(c->stmt);
      // printf("    pop rax\n");
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
        generate(c->stmt);
        printf("    pop rax\n");
        printf("    mov rdi, rax\n");
        break;
      case 1:
        generate(c->stmt);
        printf("    pop rax\n");
        printf("    mov rsi, rax\n");
        break;
      case 2:
        generate(c->stmt);
        printf("    pop rax\n");
        printf("    mov rdx, rax\n");
        break;
      case 3:
        generate(c->stmt);
        printf("    pop rax\n");
        printf("    mov rcx, rax\n");
        break;
      default:
        generate(c->stmt);
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

  generate(node->lhs);
  generate(node->rhs);

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

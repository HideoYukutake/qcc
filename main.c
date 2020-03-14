#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "qcc.h"

Token *token;
char *user_input;
Node *code[100];
char *filename;

char *read_file(char *path)
{
        FILE *fp = fopen(path, "r");
        if (!fp) {
                error("cannot open %s: %s", path, strerror(errno));
        }

        if (fseek(fp, 0, SEEK_END) == -1) {
                error("%s: fseek: %s", path, strerror(errno));
        }

        size_t size = ftell(fp);
        if (fseek(fp, 0, SEEK_SET) == -1) {
                error("%s: fseek: %s", path, strerror(errno));
        }

        char *buf = calloc(1, size+2);
        fread(buf, size, 1, fp);

        if (size == 0 || buf[size-1] != '\n') {
                buf[size++] = '\n';
        }
        buf[size] = '\n';
        fclose(fp);
        return buf;
}


int main(int argc, char **argv)
{
        if (argc != 2) {
                error("引数の個数が正しくありません");
                return 1;
        }

        filename = argv[1];
        user_input = read_file(filename);
        token = tokenize(user_input);
        program();

        // アセンブリの前半部分を出力
        printf(".intel_syntax noprefix\n");
        printf(".global main\n");
        /*
        printf("main:\n");

        // Prologue(関数呼び出しの際の定型の命令)
        // 変数26個文の領域を確保する
        // 26 * 8 = 208
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, 208\n");
        */

        for (int i = 0; code[i]; i++) {
                gen(code[i]);
                printf("    pop rax\n");
        }

        /*
        // Epilogue(関数の末尾に出力する定型の命令)
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        */

        return 0;

}


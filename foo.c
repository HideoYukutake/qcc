#include <stdio.h>

int foo()
{
        printf("foo is called\n");
        return 0;
}

int hoge(int a)
{
        printf("hoge is called: %d\n", a);
        return 0;
}

int fuga(int a, int b)
{
        printf("fuga is called: %d\n", a+b);
        return a+b;
}

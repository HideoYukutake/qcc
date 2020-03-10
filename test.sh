#!/bin/bash

try(){
    expected="$1"
    input="$2"

    ./qcc "$input" > tmp.s
    if [[ "$expected" = 0 ]]; then
        gcc -o tmp tmp.s foo.o
    else
        gcc -o tmp tmp.s
    fi
    ./tmp
    actual="$?"

    if [[ "$actual" = "$expected" ]]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

try 0 "0;"
try 42 "42;"
try 21 "5+20-4;"
try 41 " 12 + 34 - 5 ;"
try 47 "5+6*7;"
try 15 "5*(9-6);"
try 4  "(3+5)/2;"
try 6  "-10+2*8;"
try 0  "0==1;"
try 0  "0 == 1;"
try 1  "42 == 42;"
try 1  "0 != 2;"
try 0  "42 != 42;"
try 1  "0 < 2;"
try 0  "42 < 42;"
try 0  "2 < 1;"
try 1  "0 <= 1;"
try 1  "1 <= 1;"
try 0  "2 <= 1;"
try 1  "1 > 0;"
try 0  "1 > 1;"
try 0  "1 > 2;"
try 1  "1 >= 0;"
try 1  "1 >= 1;"
try 0  "1 >= 2;"
try 3  "a = 3;"
try 14 "a = 3; b = 5 * 6 - 8; a + b / 2;"
try 3  "foo = 3;"
try 6  "foo = 1;bar = 2 + 3;foo + bar;"
try 14 "foo = 3; bar = 5 * 6 - 8; return foo + bar / 2;"
try 4  "i = 3; if ( i == 3 ) i = 4; return i;"
try 2  "i = 3; if ( i != 3 ) i = 4; else i = 2; return i;"
try 10 "i = 1; while ( i < 10 ) i = i + 1; return i;"
try 10 "for ( i = 1;i < 10; i = i + 1 ) i; return i;"
try 10 "for ( i = 1;i < 10; ) i = i + 1; return i;"
try 10 "i = 1; for ( ;i < 10; i = i + 1 ) i; return i;"
try 4  "i = 3; if ( i == 3 ) { i = 4; } return i;"
try 5  "i = 3; if ( i == 3 ) { i = 4; i = 5;} return i;"
try 0  "foo(); return 0;"
try 0  "i = 3; if ( i == 3 ) { foo(); foo(); } return 0;"
try 0  "hoge(1); return 0;"
try 0  "hoge(1); fuga(1, 2); return 0;"

echo OK

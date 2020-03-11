#!/bin/bash

try(){
    expected="$1"
    input="$2"

    ./qcc ./test/"$input" > tmp.s
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

try 0  "test01"
try 42 "test02"
try 21 "test03"
try 41 "test04"
try 47 "test05"
try 15 "test06"
try 4  "test07"
try 6  "test08"
try 0  "test09"
try 0  "test10"
try 1  "test11"
try 1  "test12"
try 0  "test13"
try 1  "test14"
try 0  "test15"
try 0  "test16"
try 1  "test17"
try 1  "test18"
try 0  "test19"
try 1  "test20"
try 0  "test21"
try 0  "test22"
try 1  "test23"
try 1  "test24"
try 0  "test25"
try 3  "test26"
try 14 "test27"
try 3  "test28"
try 6  "test29"
try 14 "test30"
try 4  "test31"
try 2  "test32"
try 10 "test33"
try 10 "test34"
try 10 "test35"
try 10 "test36"
try 4  "test37"
try 5  "test38"
try 0  "test39"
try 0  "test40"

echo OK

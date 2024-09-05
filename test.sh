#! /usr/bin/bash

for test in test/*; do 
    ./riscv_asm $test/input.s $test/output.hex
    if cmp -s $test/output.hex $test/expected.hex; then
        echo "$test: Passed" 
    else
        echo "$test: Failed"
    fi
    rm $test/output.hex
done
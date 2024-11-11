#! /usr/bin/bash

for i in test/*; do
    ./riscv_sim >/dev/null <<EOF
        cache_sim enable $i/config.txt
        load $i/input.s
        run
        exit
EOF

    if cmp -s "$i/expected.output" "$i/input.output"; then
        echo "$i: passed"
    else
        echo "$i: failed"
    fi
done
.data
.word 10
.word 20

.text
lui x3, 0x10
lw x7, 0(x3)
lw x8, 4(x3)
add x1, x7, x8
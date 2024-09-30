; Multiply two numbers in .data
; and put result in x1

.data
.word 20, 10
.dword 0x123456789abcdef0
.word 0x12345678, 0x9abcdef0
.half 0x1234, 0x5678, 0x9abc, 0xdef0
.byte 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0

.text
lui x3, 0x10
lw x7, 0(x3)
lw x8, 4(x3)

Loop:
    beq x0, x8, Exit    
    add x1, x1, x7
    addi x8, x8, -1
    beq x0, x0, Loop

Exit:
    add x0, x0, x0
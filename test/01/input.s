; Multiply two numbers in .data
; and put result in x1

.data
.word 20, 10

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
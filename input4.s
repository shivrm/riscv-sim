; Recursively finds sum of first n natural numbers
; n is defined in .data

.data
.word 100

.text
lui x3, 0x10
lui x2, 0x30
lw x10, 0(x3)

jal x1, Sum
beq x0, x0, Exit

Sum:
    beq x10, x0, Return
    sd x1, 0(x2)
    addi x2, x2, -8
    sd x10, 0(x2)
    addi x2, x2, -8

    addi x10, x10, -1
    jal x1, Sum

    addi x2, x2, 8
    ld x11, 0(x2)
    add x10, x10, x11
    addi x2, x2, 8
    ld x1, 0(x2)
 
    Return:
        jalr x0, 0(x1)

Exit:
    add x0, x0, x0
beq x1, x2, L1
jal x0, L2
ld x2, 1110(x1)
L1: xor a5, a3, a7
lui x9, 0x10000
L2: addi t1, x8, L1

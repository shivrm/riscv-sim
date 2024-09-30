.data
.word 5, 66, 99, 12, 18, 3, 4, 0, 5, 12, 21

.text
lui x3, 0x10
lui x4, 0x10
addi x4, x4, 512

lw x5, 0(x3)
addi x6, x3, 4
add x7, x0, x0
addi x13, x0, 1 

OVERALL_LOOP:
beq x7, x5, Exit
slli x8, x7, 3
slli x15, x7, 3
add x16, x6, x8 
addi x17, x16, 4
add x18, x4, x15 
lw x9, 0(x16)
lw x10, 0(x17) 
addi x7, x7, 1

GCD_LOOP:
bge x9, x10, Proceed
add x11, x0, x9 
add x12, x0, x10
sub x9, x9, x0
sub x10, x10, x0
add x9, x0, x12
add x10, x0, x11
Proceed:
beq x10, x0, Zero    
beq x9, x10, Multiple 
blt x9, x10, Swap_and_Repeat 
sub x9, x9, x10
beq x0, x0, GCD_LOOP

Multiple:
sd x9, 0(x18)
beq x0, x0, OVERALL_LOOP

Swap_and_Repeat:
beq x9, x13, One
add x11, x0, x9 
add x12, x0, x10
sub x9, x9, x0
sub x10, x10, x0
add x9, x0, x12
add x10, x0, x11 
beq x0, x0, GCD_LOOP

One: 
sd x13, 0(x18)
beq x0, x0, OVERALL_LOOP

Zero:
sd x0, 0(x18)
beq x0, x0, OVERALL_LOOP

Exit:
sub x0, x0, x0


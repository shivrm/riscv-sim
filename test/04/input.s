
;The following line initializes register x3 with 0x10000000 
;so that you can use x3 for referencing various memory locations. 
lui x3, 0x10000
;your code starts here

sub x10, x0, x0    ; Initialize 10 to zero

; Adding the first 10 numbers
; 1
ld x9, 0(x3)        ; Load the first number into x9
add x10, x10, x9    ; Add it to x10

; 2 
ld x9, 8(x3)
add x10, x10, x9

; 3
ld x9, 16(x3)
add x10, x10, x9

; 4
ld x9, 24(x3)
add x10, x10, x9

; 5
ld x9, 32(x3)
add x10, x10, x9

; 6
ld x9, 40(x3)
add x10, x10, x9

; 7
ld x9, 48(x3)
add x10, x10, x9

; 8
ld x9, 56(x3)
add x10, x10, x9

; 9
ld x9, 64(x3)
add x10, x10, x9

; 10
ld x9, 72(x3)
add x10, x10, x9

; Subtracting the next 5 numbers
; 1
ld x9, 80(x3)
sub x10, x10, x9

; 2
ld x9, 88(x3)
sub x10, x10, x9

; 3
ld x9, 96(x3)
sub x10, x10, x9

; 4
ld x9, 104(x3)
sub x10, x10, x9

; 5
ld x9, 112(x3)
sub x10, x10, x9

;The final result should be in register x10
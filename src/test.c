#include <stdio.h>
#include <limits.h>
int main(void){
    int32_t ins = 0b10011001100110011001100110011001;
    int imm = ((ins>>31)<< 11) +
            (((ins>>7)& 0b1) << 10) +
            (((ins >>25) & 0b111111) << 4) +
            ((ins>>8) & 0b1111);

    printf("%d\n", imm);
    printf("%d", 0b110011001001); // they should return the same
}

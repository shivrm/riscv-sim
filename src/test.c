#include <stdio.h>
#include <limits.h>
int main(void){
    int32_t ins = 0b10011001100110011001100110011001;
    /*int imm = ((ins>>31)<< 11) +
            (((ins>>7)& 0b1) << 10) +
            (((ins >>25) & 0b111111) << 4) +
            ((ins>>8) & 0b1111);*/
	    int    imm = ((ins>>31)<< 19) + // 20
            	(((ins>>12)& 0b11111111) << 11) + // 19:12
            	(((ins >>20) & 0b1) << 10) + //11
            	((ins>>21) & 0b1111111111); // 10:1

    printf("%d\n", imm);
    printf("%d\n", 0b11001100110011001100); // they should return the same

    int num = 0b111101;
    int16_t sixteen = (((int16_t)num) <<10)>>8;
    printf("%d %d\n", sixteen, num);
}

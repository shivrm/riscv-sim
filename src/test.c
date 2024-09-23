#include <stdio.h>
#include <limits.h>
int main(void){
    /*u_int8_t rs1 = 0b00000010, rs2 = 0b10001000;
    u_int8_t change = 0b11111111;
    change = change << (8 - rs1);
    u_int8_t rd = change | (rs2 >> rs1);
    printf("%d", rd);*/
    int8_t rs1 = 0b10010000, rs2 = 0b00000010, rd;
    printf("%d\n", rs1>>rs2);
    printf("%hhd\n", (int8_t) ((u_int8_t)(rs1) >> (u_int8_t)(rs2)));
    //int8_t rs3 = 0b10000000;
    //printf("%d", rs3);

}

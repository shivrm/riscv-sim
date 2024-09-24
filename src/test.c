#include <stdio.h>
#include <limits.h>
int main(void){
    int8_t num = 0b10001001;
    printf("%d\n", num >>2);
    if (num>>7 == -1){
        printf("%d\n", ((u_int8_t)num >> 2));
    }
    else{
        printf("%d\n", num>>2);
    }

}

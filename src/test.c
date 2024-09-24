#include <stdio.h>
#include <limits.h>
int main(void){
    int16_t num = 0b1001100110011001;
    u_int16_t unsigned_num = (u_int16_t) num;
    //int8_t smaller_num = (num << 8) >> 8;
    u_int8_t smaller_num = (num<<8) >>8;
    printf("%d\n", smaller_num);
    int64_t bigger_num = (int64_t) smaller_num; // bigger_num is still unsigned.
    //int64_t bigger_num = (int64_t)(int8_t)((num << 8) >>8); 
    printf("%lld\n", bigger_num);
    printf("%d %d", num, unsigned_num);

}

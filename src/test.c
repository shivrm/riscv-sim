#include <stdio.h>
#include <stdlib.h>

int main(void){
    uint16_t value = 0b1010101010111011;
    printf("%d %d %d\n", value, value % 0xff, value % 255);
}
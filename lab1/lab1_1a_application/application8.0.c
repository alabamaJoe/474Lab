// preprocessor directive to support printing to the display

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// the main program
int main(void)
{
    char data[] = "A B C D";
    unsigned long delay1 = 1;
    unsigned long delay2 = 3;

    while(1){
        f3DataClear(data, delay1, delay2);
    }

	return 0;
}

void delay(){
    uint32_t i;
    uint32_t oneSecond = 11200 * 22500;
    for (i = 0; i < oneSecond; i++);
}

void delaySec(unsigned long s) {
    int i;
    for (i = 0; i < s; i++) {
        delay();
    }
}

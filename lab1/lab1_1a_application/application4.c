// preprocessor directive to support printing to the display

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// the main program
int main(void)
{
    while(1){
        printf("A B C D");
        delaySec(2);
        system("cls");
        delaySec(3);
    }

	return 0;
}

void delay(){
    uint32_t i;
    uint32_t oneSecond = 11200 * 22500;
    for (i = 0; i < oneSecond; i++);
}

void delaySec(int s) {
    int i;
    for (i = 0; i < s; i++) {
        delay();
    }
}

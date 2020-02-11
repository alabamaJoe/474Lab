// preprocessor directive to support printing to the display

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// the main program
int main(void)
{
    unsigned long delay1 = 1;
    unsigned long delay2 = 1;

    while(1){
        f1Data(&delay1);
        f2Clear(&delay2);
    }

	return 0;
}

void delay(){
    uint32_t i;
    uint32_t oneSecond = 11200 * 22500;
    for (i = 0; i < oneSecond; i++);
}

void f1Data(unsigned long *delay1) {
    printf("A B C D");
    for (int i = 0; i < *delay1; i++) {
        delay();
    }
}

void f2Clear(unsigned long *delay2) {
    system("cls");
    for (int i = 0; i < *delay2; i++) {
        delay();
    }
}

    //print("*delay1 = %d\n", *delay1);
    //print("&delay1 = %d\n", &delay1);
    //print("delay1 = %d\n", delay1);

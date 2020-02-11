// preprocessor directive to support printing to the display

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// the main program
void delay(){
    uint32_t i;
    uint32_t oneSecond = 11200 * 22500;
    for (i = 0; i < oneSecond; i++);
}

int main(void)
{

    while(1){
        printf("A B C D");
        delay();
        system("cls");
        delay();
    }

	return 0;
}

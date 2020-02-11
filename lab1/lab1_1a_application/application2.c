// preprocessor directive to support printing to the display

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// the main program
int main(void)
{

    while(1){
        printf("A");
        delay();
        system("cls");
        printf("A B");
        delay();
        system("cls");
        printf("A B C");
        delay();
        system("cls");
        printf("A B C D");
        delay();
        system("cls");
        printf("  B C D");
        delay();
        system("cls");
        printf("    C D");
        delay();
        system("cls");
        printf("      D");
        delay();
        system("cls");
        printf("       ");
        delay();
        system("cls");
    }

	return 0;
}

void delay(){
    uint32_t i;
    uint32_t oneSecond = 11200 * 22500;
    for (i = 0; i < oneSecond; i++);
}

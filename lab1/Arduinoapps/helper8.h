#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


void delay(){ // Function takes care of approximately a one second time delay
    uint32_t i;
    uint32_t oneSecond = 220;
    for (i = 0; i < oneSecond; i++){  // For loop method
      Serial.println((int)i);
    }
}


void delaySec(unsigned long s) {  // Function that determines how long a delay is in seconds
    int i;
    for (i = 0; i < s; i++) {
        delay();
    }
}

void f3DataClear(char data[], unsigned long* delay1, unsigned long* delay2) { // Function that displays letters for some time and prints nothing for some time
    char clearSpace[2] = " ";
    tft.print(data);  // Print letters  
    delaySec(*delay1);  
    tft.fillScreen(BLACK);  
    tft.print(clearSpace);  // Print nothing
    delaySec(*delay2);
}

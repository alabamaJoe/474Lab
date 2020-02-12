#include <stdio.h>
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // Touch screen library
#include "Lab3.h"
#include "TimerOne.h"

// Digital pin macros
#define CONTACT_INDICATOR_PIN   23
#define HVIL_READ_PIN           24
#define MEASURE_Y_SPACING       30
#define MEASURE_TEXT_SIZE       2

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Parameters needed for touch capability
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

// These are the pins for the shield!
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// Pressure touchscreen parameters
#define MINPRESSURE 50
#define MAXPRESSURE 1000

//Different Screens for TFT
#define MEASUREMENT_SCREEN  0
#define ALARM_SCREEN  1
#define BATTERY_SCREEN  2

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

///////////////Global Variables////////////////////

///////////////TCB instantiations//////////////////
static TCB alarmTask;
static TCB contactTask;
static TCB socTask;
static TCB measureTask;
static TCB tftTask;
static TCB schedulerTask;
static TCB startupTask;
static TCB intraSysCommTask;

static TCB* taskArray[] = {&measureTask, &tftTask, &intraSysCommTask, &socTask, &contactTask, &alarmTask};   // TCB pointer queue for scheduler
static long waitTime;

static int screenDisplay = BATTERY_SCREEN;  // Variable that determines which screen to show during TFT task

String hvilState; 
String ocurrState;
static String hvorState;
static int soc;
int alarmCntr = 0;    // Counters to help determine which dummy value to output
static int socCntr = 0;
static int measureCntr = 0;
static int screenPressed = 0;

static int temp;  // Variables needed for measurement
static int currentHV;
static int voltageHV;
static long isolResHV;
static String modeHV;

/////////////////// taskDataPtr instantiations for each task ///////////////
struct alarmStruct{
  String* hvilPtr;
  String* ocurrPtr;
  String* hvorPtr;
}; typedef struct alarmStruct dataAlarm;

struct contactStruct{
  String *state;
  bool *control;
}; typedef struct contactStruct dataContact;

struct socStruct{
  int* socPtr;
}; typedef struct socStruct dataSOC;

struct measureStruct{
  int* tempPtr;
  int* currentHVPtr;
  int* voltageHVPtr;
  long* isolResHVPtr;
  String* modeHVPtr;
  String* hvilInputStatePtr;
}; typedef struct measureStruct dataMeasure;

struct tftStruct{
  int* socPtr;    // Measurement screen variables
  int* tempPtr;
  int* currentHVPtr;
  int* voltageHVPtr;
  long* isolResHVPtr;
  String* modeHVPtr;
  String* hvilInputStatePtr;
  String* hvilPtr;
  String* ocurrPtr;
  String* hvorPtr;
  int* screenPtr;
  bool* firstTimeScreenPtr;
  bool* controlPtr;
}; typedef struct tftStruct dataTft;

struct schedulerStruct{
  TCB* (*pointerToTCBArray)[6];
}; typedef struct schedulerStruct dataScheduler;

struct startupStruct{
  TCB* (*pointerToTCBArray)[6];
}; typedef struct startupStruct dataStartup;

volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt


///////////////////Flags/////////////////////
static String hvilInputState = "OPEN";
static String contactState = "OPEN";
static bool batteryTurnON = false;
static bool firstTimeScreen = true;
TSPoint p;

void setup() {
  // put your setup code here, to run once:

  initializeStructs();

  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));  //prints a msg that we should see when booting up


#ifdef USE_Elegoo_SHIELD_PINOUT
  Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));  // If we are using the shield, we want to see a msg printed out
#else
  Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
#endif

  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  tft.reset();

   uint16_t identifier = tft.readID();
   if(identifier == 0x9325) {                         //let's user know that a driver has been found
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else if(identifier==0x1111)
  {     
      identifier=0x9328;
       Serial.println(F("Found 0x9328 LCD driver"));
  }
  else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9328;
  
  }
  pinMode(CONTACT_INDICATOR_PIN, OUTPUT);   // Setup up GPIO output pin
  tft.begin(identifier);
  tft.setRotation(2);
  tft.fillScreen(BLACK);
  TSPoint p;    // create TSPoint object for touch capability
  
  tft.fillRect(10, 8.5*MEASURE_Y_SPACING, 50, 50, GREEN); //Draws Left Button for keypad
  tft.drawRect(10, 8.5*MEASURE_Y_SPACING, 50, 50, WHITE);

  tft.fillRect(180, 8.5*MEASURE_Y_SPACING, 50, 50, GREEN); //Draws Right Button for keypad
  tft.drawRect(180, 8.5*MEASURE_Y_SPACING, 50, 50, WHITE);
  *startupTask.myTask;  // call Startup task
}

void loop() {
  while(timeBaseFlag == 1){
    timeBaseFlag = 0;
    schedulerFunction(schedulerTask.taskDataPtr);  // Calls out scheduler 
  }
}

// Startup task that initalizes GPIOs, Timer interrupt, and task linkedlist
static void startupFunction(void* arg){   
  dataStartup* localDataPtr = (dataStartup*) arg;
  Timer1.initialize(100000); 
  Timer1.attachInterrupt(timerISR);
  Timer1.start();
  pinMode(CONTACT_INDICATOR_PIN, OUTPUT);   // Setup up GPIO output pin
  pinMode(HVIL_READ_PIN, INPUT_PULLUP);            // Setup up GPIO input pin
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);
//  measureTask.prev = NULL;
//  measureTask.next = tftTask.prev;    // Setting up TCB task linked list
//  tftTask.next = intraSysCommTask.prev;
//  intraSysCommTask.next = socTask.prev;
//  socTask.next = contactTask.prev;
//  contactTask.next = alarmTask.prev;
//  alarmTask.next = NULL;

  for (int i = 0; i < 5; i++){
    TCB* task = *localDataPtr->pointerToTCBArray[i];
    TCB* nextTask = *localDataPtr->pointerToTCBArray[i+1];
    task->next = nextTask->prev;
  }
}

// Scheduler task that uses task queue to perform a round robin scheduler
static void schedulerFunction(void* arg){
  dataScheduler* localDataPtr = (dataScheduler* ) arg;
 
  TCB* taskNode = *(localDataPtr->pointerToTCBArray[0]);
  for (int i = 0; i < 5; i++){   // For loop to ensure that each task gets called
    taskNode->myTask((taskNode->taskDataPtr));
    taskNode = taskNode->next;
  }

}

void intraSystemCommFunction(void* arg){
  Serial.println(2);
}

// TFT display + keypad task that is capable of displaying one of the three screens (measurement, alarm, batteryon/off) at a time
void tftFunction(void* arg){
  Serial.println(1);
/*
  screenPressed = 0;
  Serial.print("After: ");
  Serial.println(screenPressed);
  dataTft* localDataPtr = (dataTft* )arg;
  int state = *localDataPtr->screenPtr;
  //samplePress(localDataPtr);
  int firstTime = *localDataPtr->firstTimeScreenPtr;
  if(state == MEASUREMENT_SCREEN){  // Code to display the measurement screen
    if(firstTime){                  // Wipes the previous screens content when transtioning to a new screen. Each screen code block contains this if check
      tft.fillRect(0,0,240, 250, BLACK);
      *localDataPtr->firstTimeScreenPtr = false;
    }
    samplePress(localDataPtr);  // samples if we detect a touch screen
    tft.setCursor(20,0);
    samplePress(localDataPtr);
    tft.setTextColor(WHITE, BLACK); // prints measurement header
    samplePress(localDataPtr);
    tft.setTextSize(3);
    samplePress(localDataPtr);
    tft.print("Measurement");
    samplePress(localDataPtr);
  
    tft.setCursor(0, MEASURE_Y_SPACING);   // Displays SOC data
    tft.setTextSize(MEASURE_TEXT_SIZE);
    tft.print("State of Charge: ");
    samplePress(localDataPtr);
    if (!*(localDataPtr->socPtr)){
      tft.fillRect(228, MEASURE_Y_SPACING, 50, 15, BLACK);
    }
    tft.print(*(localDataPtr->socPtr));
    samplePress(localDataPtr);
  
    tft.setCursor(0, 2*MEASURE_Y_SPACING);   // Displays Temperature data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    tft.print("Temperature: ");
    samplePress(localDataPtr);
    if (*(localDataPtr->tempPtr) == 5){
      tft.fillRect(150, 2*MEASURE_Y_SPACING, 100, 15, BLACK);
    }
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->tempPtr));
    samplePress(localDataPtr);
   
    tft.setCursor(0, 3*MEASURE_Y_SPACING);   // Displays HV Current data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HV Current: ");
    samplePress(localDataPtr);
    if(!*(localDataPtr->currentHVPtr)){
      tft.fillRect(155, 3*MEASURE_Y_SPACING, 100, 15, BLACK);
    }
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->currentHVPtr));
    samplePress(localDataPtr);
  
    tft.setCursor(0, 4*MEASURE_Y_SPACING);   // Displays HV Voltage data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HV Voltage: ");
    samplePress(localDataPtr);
    if(*(localDataPtr->voltageHVPtr) == 10){
      tft.fillRect(167, 4*MEASURE_Y_SPACING, 100, 15, BLACK);
    }
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->voltageHVPtr));
    samplePress(localDataPtr);
  
    tft.setCursor(0, 5*MEASURE_Y_SPACING);   // Displays HV Isolation Resistance data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HVIRes: ");
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->isolResHVPtr));
    if(*(localDataPtr->isolResHVPtr) == 0){
      tft.fillRect(108, 5*MEASURE_Y_SPACING, 100, 15, BLACK);
    }
    samplePress(localDataPtr);
    
    tft.setCursor(0, 6*MEASURE_Y_SPACING);   // Displays HV Isolation Mode data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HVIMod: ");
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->modeHVPtr));
    samplePress(localDataPtr);
  
    tft.setCursor(0, 7*MEASURE_Y_SPACING);   // Displays HVIL data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HVIL Status: ");
    samplePress(localDataPtr);
    tft.print(*(localDataPtr->hvilInputStatePtr));
    samplePress(localDataPtr);
  }else if (state == ALARM_SCREEN){   // Code for displaying Alarm Screen
    if(firstTime){   
      tft.fillRect(0,0,240, 250, BLACK);
      *localDataPtr->firstTimeScreenPtr = false;
    }
    samplePress(localDataPtr);  // Displays Alarm header
    tft.setCursor(20,0);
    samplePress(localDataPtr);
    tft.setTextColor(WHITE, BLACK);
    samplePress(localDataPtr);
    tft.setTextSize(3);
    samplePress(localDataPtr);
    tft.print("Alarm");
    samplePress(localDataPtr);
  
    tft.setCursor(0, MEASURE_Y_SPACING);    // Displays HVI Alarm data
    samplePress(localDataPtr);
    tft.setTextSize(MEASURE_TEXT_SIZE);
    samplePress(localDataPtr);
    tft.print("HVIAlarm: ");
    samplePress(localDataPtr);
    tft.setCursor(0, 1.5*MEASURE_Y_SPACING);
    samplePress(localDataPtr);
    tft.print(*localDataPtr->hvilPtr);
    samplePress(localDataPtr);
  
    tft.setCursor(0, 3*MEASURE_Y_SPACING);   // Displays Overcurrent data
    samplePress(localDataPtr);
    tft.print("Overcurrent: ");
    samplePress(localDataPtr);
    tft.setCursor(0, 3.5*MEASURE_Y_SPACING);
    samplePress(localDataPtr);
    tft.print(*localDataPtr->ocurrPtr);
    samplePress(localDataPtr);
    
    tft.setCursor(0, 5*MEASURE_Y_SPACING);  // Displays HV out of range data
    samplePress(localDataPtr);
    tft.print("HVOutofRange: ");
    samplePress(localDataPtr);
    tft.setCursor(0, 5.5*MEASURE_Y_SPACING);
    samplePress(localDataPtr);
    tft.print(*localDataPtr->hvorPtr); 
    samplePress(localDataPtr);
  } else if (state == BATTERY_SCREEN){    // Displays battery screen
      tft.setCursor(40,0);
      if(firstTime){
        tft.fillRect(0,0,240, 250, BLACK);
        *localDataPtr->firstTimeScreenPtr = false;
      }
      samplePress(localDataPtr);    // Displays battery header
      tft.setTextColor(WHITE, BLACK);
      samplePress(localDataPtr);
      tft.setTextSize(2);
      samplePress(localDataPtr);
      tft.print("Battery ON/OFF");
      samplePress(localDataPtr);

      tft.fillRect(50, 1.4*MEASURE_Y_SPACING, 133, 70, GREEN);  // Draws green button for turning ON battery
      samplePress(localDataPtr);
      tft.drawRect(50, 1.4*MEASURE_Y_SPACING, 133, 70, WHITE);
      samplePress(localDataPtr);

      tft.fillRect(50, 4*MEASURE_Y_SPACING, 133, 70, RED);    // Draws red button for turning OFF battery
      samplePress(localDataPtr);
      tft.drawRect(50, 4*MEASURE_Y_SPACING, 133, 70, WHITE);
      samplePress(localDataPtr);
      
      tft.setCursor(12, 1.5*MEASURE_Y_SPACING);   // Prints Battery turn on label
      samplePress(localDataPtr);
      tft.setTextSize(MEASURE_TEXT_SIZE);
      samplePress(localDataPtr);
      tft.setTextColor(WHITE);
      tft.print("ON");
      samplePress(localDataPtr);
      
      tft.setCursor(12, 4.1*MEASURE_Y_SPACING);   // Prints Battery turn off label
      samplePress(localDataPtr);
      tft.setTextSize(MEASURE_TEXT_SIZE);
      samplePress(localDataPtr);
      tft.setTextColor(WHITE);
      samplePress(localDataPtr);
      tft.print("OFF");
      samplePress(localDataPtr);
  }*/
}

// Helper function that takes in touch pressure coordinates and determines whether or not the stylus actually touched a button
static void samplePress(void* arg){

  dataTft* sampDataPtr = (dataTft* )arg;
  digitalWrite(13, HIGH); // Lines that we need to get a touch coordinate
  p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  int state = *sampDataPtr->screenPtr;
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE && screenPressed == 0) { // Check if pressure is within the right bounds
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);   // Scale x coordinate
    p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));   // Scale y coordinate
    if (p.y > 255 && p.y < 305) {        // For changing screens
      if (p.x < 55 && p.x > 10) {   // If left keypad button is pressed
        *sampDataPtr->firstTimeScreenPtr = true;
        if(state == MEASUREMENT_SCREEN){
          *sampDataPtr->screenPtr = ALARM_SCREEN;
        } else if (state == ALARM_SCREEN){
          *sampDataPtr->screenPtr = BATTERY_SCREEN;
        } else if (state == BATTERY_SCREEN){
          *sampDataPtr->screenPtr = MEASUREMENT_SCREEN;
        }
      } else if (p.x < 225 && p.x > 180) {    // If right keypad button is pressed
        *sampDataPtr->firstTimeScreenPtr = true;
        if(state == MEASUREMENT_SCREEN){
          *sampDataPtr->screenPtr = BATTERY_SCREEN;
        } else if (state == BATTERY_SCREEN){
          *sampDataPtr->screenPtr = ALARM_SCREEN;
        } else if (state == ALARM_SCREEN){
          *sampDataPtr->screenPtr = MEASUREMENT_SCREEN;
        }
      }
      screenPressed = 1;
    } else if (p.y > 45 && p.y < 185 && p.x > 55 && p.x < 180){   // If the contactor buttons were pressed
      if (p.y < 115) {
        *sampDataPtr->controlPtr = true;
        Serial.println("Battery turned on");
      } else {
        *sampDataPtr->controlPtr = false;
        Serial.println("Battery turned off");
      }
      screenPressed = 1;
    }
  }
}

// measure task that determines and cycles through the dummy values of measurement data to be displayed
static void measureFunction(void* arg){
  Serial.println(3);
/*
  dataMeasure* localDataPtr = (dataMeasure* ) arg;
  *localDataPtr->tempPtr = 0;
  *localDataPtr->isolResHVPtr = 0;
  *localDataPtr->modeHVPtr = "0";

  if (measureCntr % 5 == 0){    // Handles dummy current values
    *(localDataPtr->currentHVPtr) = -20;
  } else{
    *(localDataPtr->currentHVPtr) += 10;
  }

  if (measureCntr % 9 < 3){   // Handles dummy voltage values
    *(localDataPtr->voltageHVPtr) = 10;
  } else if (measureCntr % 9 < 6){
    *(localDataPtr->voltageHVPtr) = 150;
  } else if (measureCntr % 9 < 9){
    *(localDataPtr->voltageHVPtr) = 450;
  }


  if (digitalRead(HVIL_READ_PIN)){    // Handles HVIL status data
    *(localDataPtr->hvilInputStatePtr) = "OPEN  ";
  } else{
    *(localDataPtr->hvilInputStatePtr) = "CLOSED";
  }

  measureCntr++;
  */

}

// SOC task that sets up the SOC dummy value to be displayed
static void socFunction(void* arg){
  Serial.println(6);
  /*
  dataSOC* localDataPtr = (dataSOC* ) arg;
  int remainder = socCntr % 5;
  *(localDataPtr->socPtr) = remainder * 25;
  socCntr++;*/

}

// Contacter task that applies contactor state diagram
static void contactFunction(void * arg){
  Serial.println(4);
/*
  dataContact* localDataPtr = (dataContact* ) arg;
  if(*(localDataPtr->control)){   // If Battery turn On button is pressed, we close the contactors
    *(localDataPtr->state) = "CLOSED";
    digitalWrite(CONTACT_INDICATOR_PIN, HIGH);
  } else{
    *(localDataPtr->state) = "OPEN";
    digitalWrite(CONTACT_INDICATOR_PIN, LOW);
  }
  */

}

// Alarm task that determines and cycles the alarm dummy values to be displayed 
void alarmFunction(void * arg){
  Serial.println(5);
  /*
  dataAlarm* localDataPtr = (dataAlarm* )arg;
  if (alarmCntr == 18){
    alarmCntr = 0;
  }
  
  if (alarmCntr % 3 == 0){    // Takes care of HV interlock alarm data
    *(localDataPtr->hvilPtr) = "NOT_ACTIVE             ";
  } else if (alarmCntr % 3 == 1){
    *(localDataPtr->hvilPtr) = "ACTIVE_NOT_ACKNOWLEDGED";
  } else if (alarmCntr % 3 == 2){
    *(localDataPtr->hvilPtr) = "ACTIVE_ACKNOWLEDGED    ";
  }

  if (alarmCntr % 6 < 2){   // Takes care of overcurrent data
    *(localDataPtr->ocurrPtr) = "NOT_ACTIVE             ";
  } else if (alarmCntr % 6 < 4){
    *(localDataPtr->ocurrPtr) = "ACTIVE_NOT_ACKNOWLEDGED";
  } else if (alarmCntr % 6 < 6){
    *(localDataPtr->ocurrPtr) = "ACTIVE_ACKNOWLEDGED    ";
  }

  if (alarmCntr % 9 < 3){   // Takes care of HV out of range data
    *(localDataPtr->hvorPtr) = "NOT_ACTIVE             ";
  } else if (alarmCntr % 9 < 6){
    *(localDataPtr->hvorPtr) = "ACTIVE_NOT_ACKNOWLEDGED";
  } else if (alarmCntr % 9 < 9){
    *(localDataPtr->hvorPtr) = "ACTIVE_ACKNOWLEDGED    ";
  }

//  Serial.println(*(localDataPtr->hvorPtr));
  alarmCntr++; 
  */

}

// Helper function that initializes the myTaskDataPtr members to their respective TCB task structs and initializes members to the addresses of the different global variables used
void initializeStructs(){
  /////////////////// ALARM ///////////////////
  alarmTask.myTask = &alarmFunction;
  dataAlarm alarmData; 
  alarmTask.taskDataPtr = &alarmData;
  alarmData.hvilPtr = &hvilState;
  alarmData.ocurrPtr = &ocurrState;
  alarmData.hvorPtr = &hvorState;
  alarmTask.next = NULL;
  alarmTask.prev = NULL;

  /////////////////// Contactor /////////////////
  contactTask.myTask = &contactFunction;
  dataContact contactData;
  contactTask.taskDataPtr = &contactData;
  contactData.state = &contactState;
  contactData.control = &batteryTurnON;
  contactTask.next = NULL;
  contactTask.prev = NULL;

  /////////////////// SOC ///////////////////////
  socTask.myTask = &socFunction;
  dataSOC socData;
  socTask.taskDataPtr = &socData;
  socData.socPtr = &soc;
  socTask.next = NULL;
  socTask.prev = NULL;

  ////////////////// Measure ////////////////////
  measureTask.myTask = &measureFunction;
  dataMeasure measureData;
  measureTask.taskDataPtr = &measureData;
  measureData.tempPtr = &temp;
  measureData.currentHVPtr = &currentHV;
  measureData.voltageHVPtr = &voltageHV;
  measureData.isolResHVPtr = &isolResHV;
  measureData.modeHVPtr = &modeHV;
  measureData.hvilInputStatePtr = &hvilInputState;
  measureTask.next = NULL;
  measureTask.prev = NULL;

  ////////////////// TFT //////////////////////////
  tftTask.myTask = &tftFunction;
  dataTft tftData;
  tftTask.taskDataPtr = &tftData;
  tftData.socPtr = &soc;
  tftData.tempPtr = &temp;
  tftData.currentHVPtr = &currentHV;
  tftData.voltageHVPtr = &voltageHV;
  tftData.isolResHVPtr = &isolResHV;
  tftData.modeHVPtr = &modeHV;
  tftData.hvilInputStatePtr = &hvilInputState;
  tftData.hvilPtr = &hvilState;
  tftData.ocurrPtr = &ocurrState;
  tftData.hvorPtr = &hvorState;
  tftData.screenPtr = &screenDisplay;
  tftData.firstTimeScreenPtr = &firstTimeScreen;
  tftData.controlPtr = &batteryTurnON;
  tftTask.next = NULL;
  tftTask.prev = NULL;

  Serial.println("Finish Initializing Structs");

  ////////////////// Scheduler ////////////////////
  schedulerTask.myTask = &schedulerFunction;
  dataScheduler schedulerData;
  schedulerTask.taskDataPtr = &schedulerData;
  schedulerData.pointerToTCBArray = &taskArray;
  schedulerTask.next = NULL;
  schedulerTask.prev = NULL;

  //////////////////Startup/////////////////////////
  startupTask.myTask = &startupFunction;
  dataStartup startupData;
  startupTask.taskDataPtr = &startupData;
  startupData.pointerToTCBArray = &taskArray;
  startupTask.next = NULL;
  startupTask.prev = NULL;
  
}

void timerISR(){
  timeBaseFlag = 1;
}

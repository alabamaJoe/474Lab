#include <stdio.h>
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // Touch screen library
#include "Lab3.h"
#include <TimerOne.h>

// Digital pin macros
#define CONTACT_INDICATOR_PIN   23
#define HVIL_READ_PIN           24
#define HVIL_ISR_PIN            21  // INT.1
#define HV_INPUT                // Still need to deside
#define MEASURE_Y_SPACING       30
#define MEASURE_TEXT_SIZE       2
#define UART_BYTE_LENGTH        2

// UART
#define RX                      19
#define TX                      18

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
#define MINPRESSURE 30
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
static TCB iSCTask;

static TCB* taskArray[] = {&measureTask, &tftTask, &socTask, &contactTask, &alarmTask};   // TCB pointer queue for scheduler
static long waitTime;

static int screenDisplay = MEASUREMENT_SCREEN;  // Variable that determines which screen to show during TFT task

volatile String HVIL_ISR_STATE;
String hvilState = "NOT_ACTIVE             "; 
String ocurrState = "NOT_ACTIVE             ";
static String hvorState = "NOT_ACTIVE             ";
static int soc = 0;   // SOC value, for now is just zero
//int alarmCntr = 0;    // Counters to help determine which dummy value to output
static int socCntr = 0;
static int measureCntr = 0;
static int screenPressed = 0;
static bool switchToAlarm = false;

static int temp;  // Variables needed for measurement
volatile static double currentHV;   // Stores HV current data
volatile static double voltageHV;   // Stores HV voltage data
volatile static byte command;  // Command to specify which data is desired
static long isolResHV;
static String modeHV;

/////////////////// taskDataPtr instantiations for each task ///////////////
struct alarmStruct{
  double* currentHVPtr;
  double* voltageHVPtr;
  String* hvilPtr;
  String* HVIL_ISR_STATE_PTR;
  String* ocurrPtr;
  String* hvorPtr;
  String* hvilInputStatePtr;
  bool* alarmsAcknowledgedPtr;
  bool* switchToAlarmPtr;
  bool* firstTimeScreenPtr;
  int* screenPtr;
}; typedef struct alarmStruct dataAlarm;

struct contactStruct{
  String *state;
  String *STATE_HVIL_ISR;
  bool *control;
  String *hvilPtr;
}; typedef struct contactStruct dataContact;

struct socStruct{
  int* socPtr;
}; typedef struct socStruct dataSOC;

struct tftStruct{
  int* socPtr;    // Measurement screen variables
  int* tempPtr;
  double* currentHVPtr;
  double* voltageHVPtr;
  long* isolResHVPtr;
  String* modeHVPtr;
  String* hvilInputStatePtr;
  String* hvilPtr;
  String* ocurrPtr;
  String* hvorPtr;
  int* screenPtr;
  bool* firstTimeScreenPtr;
  bool* controlPtr;
  bool* alarmsAcknowledgedPtr;
  bool* switchToAlarmPtr;
}; typedef struct tftStruct dataTft;

struct schedulerStruct{
  TCB* (*pointerToTCBArray)[5];
  TCB* firstNode;
  tftStruct* dataTftPtr;
}; typedef struct schedulerStruct dataScheduler;

struct startupStruct{
  TCB* (*pointerToTCBArray)[5];
}; typedef struct startupStruct dataStartup;

struct iSCStruct{
  volatile double* voltageHVPtr;
  volatile double* currentHVPtr;
  byte* commandPtr;
}; typedef struct iSCStruct dataiSC;

struct measureStruct{
  int* tempPtr;
  double* currentHVPtr;
  double* voltageHVPtr;
  long* isolResHVPtr;
  String* modeHVPtr;
  String* hvilInputStatePtr;
  iSCStruct* dataiSCPtr;
  bool* alarmsAcknowledgedPtr;
}; typedef struct measureStruct dataMeasure;

volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt


///////////////////Flags/////////////////////
static String hvilInputState = "OPEN  ";
volatile String contactState = "OPEN  ";
volatile bool batteryTurnON = false;
volatile bool firstTimeScreen = true;
volatile bool alarmsAcknowledged = false;
TSPoint p;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial1.setTimeout(1000);
  initializeStructs();
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
  
  startupTask.myTask(startupTask.taskDataPtr);  // call Startup task
  Timer1.initialize(100000);    // Sets up Timer Interrupt
  Timer1.attachInterrupt(timerISR);  

//   Initialize HVIL ISR
  pinMode(HVIL_ISR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HVIL_ISR_PIN), hvilISR, RISING);
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

  pinMode(CONTACT_INDICATOR_PIN, OUTPUT);   // Setup up GPIO output pin
  pinMode(HVIL_READ_PIN, INPUT_PULLUP);            // Setup up GPIO input pin
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW); 

  for (int i = 0; i < 4; i++){    // Sets up the double linked list of tasks
    TCB* task = taskArray[i];
    TCB* nextTask = taskArray[i+1];
    task->next = nextTask;
    nextTask->prev=task;
  }
  taskArray[0]->prev = NULL;
  taskArray[5]->next = NULL; 
  
  Serial.println("Finish startup");
}

// Scheduler task that uses task queue to perform a round robin scheduler
static void schedulerFunction(void* arg){
  dataScheduler* localDataPtr = (dataScheduler* ) arg;
 
  TCB* taskNode = localDataPtr->firstNode;
  while(taskNode != NULL){          
    taskNode->myTask((taskNode->taskDataPtr));    // Iterate through each node in the doubly linkedlist and call the respective task function
//    samplePress(localDataPtr);
    taskNode = taskNode->next;
  }
}

// Intra-system communication task that sends UART command to peripheral to obtain current and voltage readings
void intraSystemCommFunction(void* arg){
  dataiSC* localDataPtr = (dataiSC* ) arg;
  int dataSize;
  *(localDataPtr->commandPtr) = 0x00;
  Serial1.write(*(localDataPtr->commandPtr));
  if (*(localDataPtr->commandPtr) == 0) {
    // Receiving currentHV and voltageHV data
    dataSize = 2 * UART_BYTE_LENGTH;
  } else {
    // Receiving only currentHV or voltageHV data
    dataSize = UART_BYTE_LENGTH;
  }
  byte receivedData[dataSize]; 
  while (Serial1.available()) {
    int charactersRead = Serial1.readBytesUntil('\r', receivedData, dataSize);
  }
  uint16_t current = (receivedData[0]) | ((uint16_t)receivedData[1] << 2);
  uint16_t volt = (receivedData[2]) | ((uint16_t)receivedData[3] << 2);
  
  *localDataPtr->voltageHVPtr = (volt / 1024.0) * 450;     // Converting according to voltage range
  *localDataPtr->currentHVPtr = (current / 1024.0) * 50.0 - 25;    // Converting according to current range
  Serial.println(*localDataPtr->currentHVPtr);
  Serial.println(*localDataPtr->voltageHVPtr);
  Serial.println();
}

// TFT display + keypad task that is capable of displaying one of the three screens (measurement, alarm, batteryon/off) at a time
void tftFunction(void* arg){
  screenPressed = 0;
  dataTft* localDataPtr = (dataTft* )arg;
  int state = *localDataPtr->screenPtr;
  int firstTime = *localDataPtr->firstTimeScreenPtr;
//  if(*localDataPtr->switchToAlarmPtr == true){
//    state = ALARM_SCREEN;
//  }
  if(state == MEASUREMENT_SCREEN){  // Code to display the measurement screen
    if(firstTime){                  // Wipes the previous screens content when transtioning to a new screen. Each screen code block contains this if check
      tft.fillRect(0,0,240, 250, BLACK);
      *localDataPtr->firstTimeScreenPtr = false;
      tft.setCursor(20,0);
      tft.setTextColor(WHITE, BLACK); // prints measurement header
      tft.setTextSize(3);
      tft.print("Measurement");
      tft.setCursor(0, MEASURE_Y_SPACING);   // Displays SOC data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("State of Charge: ");
      tft.setCursor(0, 2*MEASURE_Y_SPACING);   // Displays Temperature data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("Temperature: ");
      tft.setCursor(0, 3*MEASURE_Y_SPACING);   // Displays HV Current data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HV Current: ");
      tft.setCursor(0, 4*MEASURE_Y_SPACING);   // Displays HV Voltage data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HV Voltage: ");
      tft.setCursor(0, 5*MEASURE_Y_SPACING);   // Displays HV Isolation Resistance data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HVIRes: ");
      tft.setCursor(0, 6*MEASURE_Y_SPACING);   // Displays HV Isolation Mode data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HVIMod: ");
      tft.setCursor(0, 7*MEASURE_Y_SPACING);   // Displays HVIL data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HVIL Status: ");
    }
    samplePress(localDataPtr);    // Updates the measurement screen values
    tft.setCursor(190, MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->socPtr));
  
    tft.setCursor(143, 2*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->tempPtr));
   
    tft.setCursor(135, 3*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->currentHVPtr));
  
    tft.setCursor(135, 4*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->voltageHVPtr));
  
    tft.setCursor(90, 5*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->isolResHVPtr));

    tft.setCursor(90, 6*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->modeHVPtr));
  
    tft.setCursor(150, 7*MEASURE_Y_SPACING);
    tft.print(*(localDataPtr->hvilInputStatePtr));
    samplePress(localDataPtr);
  }else if (state == ALARM_SCREEN){   // Code for displaying Alarm Screen
    if(firstTime){   
      Serial.println("ALARM FIRST TIME");
      tft.fillRect(0,0,240, 250, BLACK);
      *localDataPtr->firstTimeScreenPtr = false;
      tft.setCursor(20,0);    // Displays Alarm header
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(3);
      tft.print("Alarm");

      tft.setCursor(0, 2*MEASURE_Y_SPACING);    // Displays HVI Alarm data
      tft.setTextSize(MEASURE_TEXT_SIZE);
      tft.print("HVILAlarm: ");

      tft.setCursor(0, 4*MEASURE_Y_SPACING);   // Displays Overcurrent data
      tft.print("Overcurrent: ");

      tft.setCursor(0, 6*MEASURE_Y_SPACING);  // Displays HV out of range data
      tft.print("HVOutofRange: ");

      
      if(*localDataPtr->alarmsAcknowledgedPtr == false){
        tft.setCursor(0, 0.8*MEASURE_Y_SPACING);    // Displays HVI Alarm data
        tft.setTextSize(MEASURE_TEXT_SIZE);
        tft.setTextColor(RED, BLACK);
        tft.print("ACK change with     white button!");
        //tft.print("Press ACK button to acknowledge all     Alarm state changes");
        tft.fillRect(50, 7.25*MEASURE_Y_SPACING, 133, 30, RED);
        tft.setCursor(105, 7.5*MEASURE_Y_SPACING);
        tft.setTextColor(WHITE, RED);
        tft.print("ACK");
      }      
    }
    tft.setTextColor(WHITE, BLACK);
    samplePress(localDataPtr);    // Updates the alarm screen values
    tft.setCursor(0, 2.5*MEASURE_Y_SPACING);
    tft.print(*localDataPtr->hvilPtr);

    tft.setCursor(0, 4.5*MEASURE_Y_SPACING);
    tft.print(*localDataPtr->ocurrPtr);
    
    tft.setCursor(0, 6.5*MEASURE_Y_SPACING);
    tft.print(*localDataPtr->hvorPtr); 
    samplePress(localDataPtr);
    Serial.println(*localDataPtr->alarmsAcknowledgedPtr);
    
  } else if (state == BATTERY_SCREEN){    // Displays battery screen
      tft.setCursor(40,0);
      if(firstTime){
        tft.fillRect(0,0,240, 250, BLACK);
        *localDataPtr->firstTimeScreenPtr = false;
        tft.setTextColor(WHITE, BLACK);   // Displays battery header
        tft.setTextSize(2);
        tft.print("Battery ON/OFF");
        tft.fillRect(50, 1.4*MEASURE_Y_SPACING, 133, 70, GREEN);  // Draws green button for turning ON battery
        tft.drawRect(50, 1.4*MEASURE_Y_SPACING, 133, 70, WHITE);

        tft.fillRect(50, 4*MEASURE_Y_SPACING, 133, 70, RED);    // Draws red button for turning OFF battery
        tft.drawRect(50, 4*MEASURE_Y_SPACING, 133, 70, WHITE);

        tft.setCursor(12, 1.5*MEASURE_Y_SPACING);   // Prints Battery turn on label
        tft.setTextSize(MEASURE_TEXT_SIZE);
        tft.setTextColor(WHITE);
        tft.print("ON");
        
        tft.setCursor(12, 4.1*MEASURE_Y_SPACING);   // Prints Battery turn off label
        tft.setTextSize(MEASURE_TEXT_SIZE);
        tft.setTextColor(WHITE);
        tft.print("OFF");
      }
      samplePress(localDataPtr);
  }
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
    }else if (p.y < 252 && p.y > 222 && p.x > 55 && p.x < 180){
      *sampDataPtr->alarmsAcknowledgedPtr = true;
      *sampDataPtr->switchToAlarmPtr = false;
      *sampDataPtr->firstTimeScreenPtr = true;
      Serial.println("ACK TOUCHED");
    }
  }
}

// measure task that determines and cycles through the dummy values of measurement data to be displayed
static void measureFunction(void* arg){
  dataMeasure* localDataPtr = (dataMeasure* ) arg;
  *localDataPtr->tempPtr = 0;
  *localDataPtr->isolResHVPtr = 0;
  *localDataPtr->modeHVPtr = "0";
  
  if (digitalRead(HVIL_ISR_PIN)){    // Handles HVIL status data
    *(localDataPtr->hvilInputStatePtr) = "OPEN  ";
  } else{
    *(localDataPtr->hvilInputStatePtr) = "CLOSED";
  }
  intraSystemCommFunction(localDataPtr->dataiSCPtr);    // Calls the intra-system comm task to obtain current and voltage Uart data
}

// SOC task that sets up the SOC dummy value to be displayed
static void socFunction(void* arg){
  // SOC is const 0 for now. Global variable 'soc' is set to zero
  dataSOC* localDataPtr = (dataSOC* ) arg;
  *(localDataPtr->socPtr) = 0;
}

// Contacter task that applies contactor state diagram
static void contactFunction(void * arg){  
  dataContact* localDataPtr = (dataContact* ) arg;
  // If Battery turn On button is pressed, we close the contactors
  if(*(localDataPtr->control) && *(localDataPtr->hvilPtr) == "NOT_ACTIVE             "){ 
    *(localDataPtr->state) = "CLOSED";
    digitalWrite(CONTACT_INDICATOR_PIN, HIGH);
    Serial.println("LIGHT ON");
  } else{
    *(localDataPtr->state) = "OPEN  ";
    digitalWrite(CONTACT_INDICATOR_PIN, LOW);
    Serial.println("OFF");
  }
  if(*(localDataPtr->control)){
      Serial.println("FOSK");
  }
//  Serial.println(*(localDataPtr->hvilPtr));
}

// Alarm task that determines and cycles the alarm dummy values to be displayed 
void alarmFunction(void * arg){
  dataAlarm* localDataPtr = (dataAlarm* )arg;

  // Takes care of HVIL/HVIL Interrupt alarm data
  if ((*localDataPtr->hvilInputStatePtr) == "CLOSED"){   
    *(localDataPtr->hvilPtr) = "NOT_ACTIVE             ";
  } else if ((*localDataPtr->hvilInputStatePtr) == "OPEN  " && (*localDataPtr->alarmsAcknowledgedPtr) == true){
    *(localDataPtr->hvilPtr) = "ACTIVE_ACKNOWLEDGED    ";
  } else { 
    *(localDataPtr->hvilPtr) = "ACTIVE_NOT_ACKNOWLEDGED";
    (*localDataPtr->alarmsAcknowledgedPtr) = false;
  }

  // Takes care of overcurrent data
  if ((*localDataPtr->currentHVPtr) < 20.0 && (*localDataPtr->currentHVPtr) > -5.0){   
    *(localDataPtr->ocurrPtr) = "NOT_ACTIVE             ";
  } else if(((((*localDataPtr->currentHVPtr) <= -5.0 || (*localDataPtr->currentHVPtr) >= 20.0))) && (((*localDataPtr->alarmsAcknowledgedPtr) == true &&  *(localDataPtr->ocurrPtr) == "ACTIVE_NOT_ACKNOWLEDGED") || (*(localDataPtr->ocurrPtr) == "ACTIVE_ACKNOWLEDGED    "))){ 
    *(localDataPtr->ocurrPtr) = "ACTIVE_ACKNOWLEDGED    ";
  } else {
    *(localDataPtr->ocurrPtr) = "ACTIVE_NOT_ACKNOWLEDGED"; 
    (*localDataPtr->alarmsAcknowledgedPtr) = false;
  }
  
  // Takes care of HV out of range data
  if ((*localDataPtr->voltageHVPtr) < 405.0 && (*localDataPtr->voltageHVPtr) > 280.0){   
    *(localDataPtr->hvorPtr) = "NOT_ACTIVE             ";
  } else if (((((*localDataPtr->voltageHVPtr) <= 280.0 || (*localDataPtr->voltageHVPtr) >= 405.0))) && (((*localDataPtr->alarmsAcknowledgedPtr) == true &&  *(localDataPtr->hvorPtr) == "ACTIVE_NOT_ACKNOWLEDGED") || (*(localDataPtr->hvorPtr) == "ACTIVE_ACKNOWLEDGED    "))){ 
    *(localDataPtr->hvorPtr) = "ACTIVE_ACKNOWLEDGED    ";
  } else {
    *(localDataPtr->hvorPtr) = "ACTIVE_NOT_ACKNOWLEDGED";
    (*localDataPtr->alarmsAcknowledgedPtr) = false;
  }

  if ((*(localDataPtr->hvorPtr) == "ACTIVE_NOT_ACKNOWLEDGED") || (*(localDataPtr->ocurrPtr) == "ACTIVE_NOT_ACKNOWLEDGED") || (*(localDataPtr->hvilPtr) == "ACTIVE_NOT_ACKNOWLEDGED")){
    *(localDataPtr->switchToAlarmPtr) = true;
    if(*localDataPtr->screenPtr != ALARM_SCREEN || *(localDataPtr->alarmsAcknowledgedPtr) == true){
      *localDataPtr->firstTimeScreenPtr = true;
    } else{
      *localDataPtr->firstTimeScreenPtr = false;
    }
    *(localDataPtr->screenPtr) = ALARM_SCREEN;
  }else{
    *(localDataPtr->switchToAlarmPtr) = false;
  }  
}

// Helper function that initializes the myTaskDataPtr members to their respective TCB task structs and initializes members to the addresses of the different global variables used
void initializeStructs(){
  /////////////////// ALARM ///////////////////
  alarmTask.myTask = &alarmFunction;
  dataAlarm alarmData; 
  alarmTask.taskDataPtr = &alarmData;
  alarmData.currentHVPtr = &currentHV;
  alarmData.voltageHVPtr = &voltageHV;
  alarmData.hvilPtr = &hvilState;
  alarmData.HVIL_ISR_STATE_PTR = &HVIL_ISR_STATE;
  alarmData.hvilInputStatePtr = &HVIL_ISR_STATE;
  alarmData.ocurrPtr = &ocurrState;
  alarmData.hvorPtr = &hvorState;
  alarmData.alarmsAcknowledgedPtr = &alarmsAcknowledged;
  alarmData.switchToAlarmPtr = &switchToAlarm;
  alarmData.firstTimeScreenPtr = &firstTimeScreen;
  alarmData.screenPtr = &screenDisplay;
  alarmTask.next = NULL;
  alarmTask.prev = NULL;

  /////////////////// Contactor /////////////////
  contactTask.myTask = &contactFunction;
  dataContact contactData;
  contactTask.taskDataPtr = &contactData;
  contactData.state = &contactState;
  contactData.control = &batteryTurnON;
  contactData.STATE_HVIL_ISR = &HVIL_ISR_STATE;   
  contactData.hvilPtr = &hvilState;
  contactTask.next = NULL;
  contactTask.prev = NULL;

  /////////////////// SOC ///////////////////////
  socTask.myTask = &socFunction;
  dataSOC socData;
  socTask.taskDataPtr = &socData;
  socData.socPtr = &soc;
  socTask.next = NULL;
  socTask.prev = NULL;
  
  /////////////////Intra-SysComm////////////////////
  iSCTask.myTask = &intraSystemCommFunction;
  dataiSC iSCData;
  iSCTask.taskDataPtr = &iSCData;
  iSCData.voltageHVPtr = &voltageHV;
  iSCData.currentHVPtr = &currentHV;
  iSCData.commandPtr = &command;
  iSCTask.next = NULL;
  iSCTask.prev = NULL;
  
  ////////////////// Measure ////////////////////
  measureTask.myTask = &measureFunction;
  dataMeasure measureData;
  measureTask.taskDataPtr = &measureData;
  measureData.tempPtr = &temp;
  measureData.currentHVPtr = &currentHV;
  measureData.voltageHVPtr = &voltageHV;
  measureData.isolResHVPtr = &isolResHV;
  measureData.modeHVPtr = &modeHV;
  measureData.hvilInputStatePtr = &HVIL_ISR_STATE;
  measureData.dataiSCPtr = &iSCData;
  measureData.alarmsAcknowledgedPtr = &alarmsAcknowledged;
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
  tftData.hvilInputStatePtr = &HVIL_ISR_STATE;
  tftData.hvilPtr = &hvilState;
  tftData.ocurrPtr = &ocurrState;
  tftData.hvorPtr = &hvorState;
  tftData.screenPtr = &screenDisplay;
  tftData.firstTimeScreenPtr = &firstTimeScreen;
  tftData.controlPtr = &batteryTurnON;
  tftData.alarmsAcknowledgedPtr = &alarmsAcknowledged;
  tftData.switchToAlarmPtr = &switchToAlarm;
  tftTask.next = NULL;
  tftTask.prev = NULL;

  ////////////////// Scheduler ////////////////////
  schedulerTask.myTask = &schedulerFunction;
  dataScheduler schedulerData;
  schedulerTask.taskDataPtr = &schedulerData;
  schedulerData.pointerToTCBArray = &taskArray;
  schedulerData.firstNode = &measureTask;
  schedulerData.dataTftPtr = &tftData;
  schedulerTask.next = NULL;
  schedulerTask.prev = NULL;

  //////////////////Startup/////////////////////////
  startupTask.myTask = &startupFunction;
  dataStartup startupData;
  startupTask.taskDataPtr = &startupData;
  startupData.pointerToTCBArray = &taskArray;
  startupTask.next = NULL;
  startupTask.prev = NULL;
  Serial.println("Finish Initializing Structs");
}

// Timer Base 10Hz ISR
void timerISR(){
  timeBaseFlag = 1;
}

// High Voltage Interlock IRS
// Trigger when changing HVIL state from CLOSED to OPEN
// Action: set HVIL interrupt alarm to "ACTIVE, NOT ACKNOWLEDGED"
//         open contactors by toggling gpio directly
void hvilISR() {
  HVIL_ISR_STATE = "OPEN  ";
  alarmsAcknowledged = false;
  batteryTurnON = false;
  firstTimeScreen = true;
  digitalWrite(CONTACT_INDICATOR_PIN, LOW);
}

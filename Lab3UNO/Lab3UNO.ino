#include <stdio.h>
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // Touch screen library
#include "Lab3UNO.h"
#include "TimerOne.h"

// Analog pin macros
#define CIRCUIT_V             A0 // output (voltage) from the circuit 
#define CIRCUIT_C             A1 // output (current) from the circuit 

// Digital pin 
#define CONTACT_INDICATOR_PIN   13 // the contactor 
#define NUMBER_OF_DATUMS        2  // Number of data points that need to be measured

// pins go to ATMega
#define RX                      0
#define TX                      1

///////////////TCB instantiations//////////////////
static TCB measureTask;
static TCB schedulerTask;
static TCB startupTask;
static TCB intraTask;
static TCB* taskArray[] = {&measureTask};   // TCB pointer queue for scheduler

// Variables needed for measurement
uint8_t dataArray[2 * NUMBER_OF_DATUMS];
// int currentHV[16]; // volatile as its periodic
// int voltageHV[16]; // volatile as its periodic
volatile static int command;   // command from UART from mega
//volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  initializeStructs();
  Serial.setTimeout(1000);
  Serial.println(F("TFT LCD test"));  //prints a msg that we should see when booting up
  startupTask.myTask(startupTask.taskDataPtr);
  Timer1.initialize(1000000); 
  Timer1.attachInterrupt(timerISR);  
}

void loop() {
  // put your main code here, to run repeatedly:
  while(timeBaseFlag == 1){
    //Serial.println("In main loop");
    timeBaseFlag = 0;
    schedulerFunction(schedulerTask.taskDataPtr);  // Calls out scheduler 
  }
  
//  measureFunction(measureTask.taskDataPtr);
//  dataMeasure* test = (dataMeasure*) measureTask.taskDataPtr;
//  int v = *test->voltageHVPtr;
//  int c = *test->currentHVPtr;
//
//  Serial.println(v);
//  Serial.println(c);
//  for (int i = 0; i < 100; i++) {
//    Serial.println();
//  }
}

static void measureFunction(void* arg) {
  dataMeasure* localDataPtr = (dataMeasure* ) arg;
  int arrayData[NUMBER_OF_DATUMS];
  arrayData[0] = analogRead(CIRCUIT_V);
  arrayData[1] = analogRead(CIRCUIT_C);
  for (int i = 0; i < NUMBER_OF_DATUMS; i++) {
    (localDataPtr->dataArrayPtr[i+i]) = arrayData[i] & 0xFF;
    (localDataPtr->dataArrayPtr[i+i+1]) = ((arrayData[i] >> 2) & 0xC0);
  }
  for (int i = 0; i < sizeof(dataArray); i++) {
    //Serial.println(dataArray[i]);
  }
 
}

static void startupFunction(void* arg) {
  dataStartup* localDataPtr = (dataStartup*) arg;
  
//  for (int i = 0; i < 1; i++){
//    TCB* task = taskArray[i];
//    TCB* nextTask = taskArray[i+1];
//    task->next = nextTask;
//    nextTask->prev=task;
//  }

  taskArray[0]->prev = NULL;
  taskArray[0]->next = NULL; 
}

static void schedulerFunction(void* arg) {
  dataScheduler* localDataPtr = (dataScheduler* ) arg;
 
  TCB* taskNode = localDataPtr->firstNode;
  while(taskNode != NULL){
    taskNode->myTask((taskNode->taskDataPtr));
    taskNode = taskNode->next;
  }
}

void serialEvent() {
  //Serial.println("UART invoked");
  intraSystemCommFunction(intraTask.taskDataPtr);
}

static void intraSystemCommFunction(void* arg) {
  dataIntra* localDataPtr = (dataIntra* ) arg;
  //Serial.println("In ISC Function");
  *(localDataPtr->commandPtr) = (int) Serial.read();
  //Serial.println(*(localDataPtr->commandPtr));
  if (*(localDataPtr->commandPtr) == 0) { // pass v and c to UART
    /*
    Serial.print("Sending data for command, ");
    Serial.println(*(localDataPtr->commandPtr)); // chekcking if passing the right command 
    for (int i = 0; i < sizeof(dataArray); i++) {
      Serial.println(dataArray[i]);
    }
    */
    Serial.write(dataArray, 2 * NUMBER_OF_DATUMS);
    //Serial.println((uint16_t) (dataArray[0] | (uint16_t) dataArray[1] << 2));
    //Serial.println((uint16_t) (dataArray[2] | (uint16_t) dataArray[3] << 2));
    /*
    Serial.println((uint16_t) (dataArray[0] | (uint16_t) dataArray[1] << 2));
    Serial.println((uint16_t) (dataArray[2] | (uint16_t) dataArray[3] << 2));
    Serial.print("Done sending ");
    Serial.print(bytesSent);
    Serial.println(" bytes!");
    */
  } 
  /*else if (*(localDataPtr->commandPtr) == 1) {  // pass v to UART
    Serial.println(command);
    Serial.write(*(localDataPtr->voltageHVPtr));
  } else if (*(localDataPtr->commandPtr) == 2) {  // pass c to UART
    Serial.println(command);
    Serial.write(*(localDataPtr->currentHVPtr));
  }
  */
  //Serial.println();
  //Serial.println();
}

void initializeStructs(){
  ////////////////// Measure ////////////////////
  measureTask.myTask = &measureFunction;
  dataMeasure measureData;
  measureTask.taskDataPtr = &measureData;
  measureData.dataArrayPtr = &dataArray[0];
  //measureData.currentHVPtr = &dataArray[0];
  //measureData.voltageHVPtr = &dataArray[0];
  measureTask.next = NULL;
  measureTask.prev = NULL;
  
  ////////////////// Intra ////////////////////
  intraTask.myTask = &intraSystemCommFunction;
  dataIntra intraData;
  intraTask.taskDataPtr = &intraData;
  intraData.dataArrayPtr = &dataArray[0];
  //intraData.currentHVPtr = &dataArray[0];
  //intraData.voltageHVPtr = &dataArray[0];
  intraData.commandPtr = &command;
  intraTask.next = NULL;
  intraTask.prev = NULL;
//  Serial.println("Finish Initializing Structs");
//
  ////////////////// Scheduler ////////////////////
  schedulerTask.myTask = &schedulerFunction;
  dataScheduler schedulerData;
  schedulerTask.taskDataPtr = &schedulerData;
  schedulerData.pointerToTCBArray = &taskArray;
  schedulerData.firstNode = &measureTask;
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

void timerISR(){
  timeBaseFlag = 1;
}

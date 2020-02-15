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
volatile static int currentHV; // volatile as its periodic
volatile static int voltageHV; // volatile as its periodic
volatile static int command;   // command from UART from mega
//volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt

void setup() {
  // put your setup code here, to run once:
  initializeStructs();

  Serial.begin(9600);
  Serial.setTimeout(1000);
  Serial.println(F("TFT LCD test"));  //prints a msg that we should see when booting up
  startupTask.myTask(startupTask.taskDataPtr);
  Timer1.initialize(100000); 
  Timer1.attachInterrupt(timerISR);  
  Timer1.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  while(timeBaseFlag == 1){
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

  *(localDataPtr->voltageHVPtr) = analogRead(CIRCUIT_V);
  *(localDataPtr->currentHVPtr) = analogRead(CIRCUIT_C);
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
  intraSystemCommFunction(intraTask.taskDataPtr);
}

static void intraSystemCommFunction(void* arg) {
  dataIntra* localDataPtr = (dataIntra* ) arg;

  while (Serial.available()) {
    *(localDataPtr->commandPtr) = (int) Serial.read();

    if (*(localDataPtr->commandPtr) == 0) { // pass v and c to UART
      Serial.println(command); // chekcking if passing the right command 
//      int vc[2];
//      vc[0] = *(localDataPtr->voltageHVPtr);
//      vc[1] = *(localDataPtr->currentHVPtr);
      Serial.write(*(localDataPtr->voltageHVPtr));
      Serial.write(*(localDataPtr->currentHVPtr));
    } else if (*(localDataPtr->commandPtr) == 1) {  // pass v to UART
      Serial.println(command);
      Serial.write(*(localDataPtr->voltageHVPtr));
    } else if (*(localDataPtr->commandPtr) == 2) {  // pass c to UART
      Serial.println(command);
      Serial.write(*(localDataPtr->currentHVPtr));
    }
  }
  
}

void initializeStructs(){
  ////////////////// Measure ////////////////////
  measureTask.myTask = &measureFunction;
  dataMeasure measureData;
  measureTask.taskDataPtr = &measureData;
  measureData.currentHVPtr = &currentHV;
  measureData.voltageHVPtr = &voltageHV;
  measureTask.next = NULL;
  measureTask.prev = NULL;
  
  ////////////////// Intra ////////////////////
  intraTask.myTask = &intraSystemCommFunction;
  dataIntra intraData;
  intraTask.taskDataPtr = &intraData;
  intraData.currentHVPtr = &currentHV;
  intraData.voltageHVPtr = &voltageHV;
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
  
}

void timerISR(){
  timeBaseFlag = 1;
}

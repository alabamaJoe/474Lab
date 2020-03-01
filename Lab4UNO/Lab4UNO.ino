#include <stdio.h>
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>   // Touch screen library
#include "Lab4UNO.h"
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
static int taskSize = 1;
static TCB* head = taskArray[0];
static TCB* tail = taskArray[taskSize-1];

// Variables needed for measurement
uint8_t dataArray[2 * NUMBER_OF_DATUMS];
// int currentHV[16]; // volatile as its periodic
// int voltageHV[16]; // volatile as its periodic
volatile static int command;   // command from UART from mega
//volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt

// taskCounter for future time varying task support
volatile static int taskCounter = 0;


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
}

// Function that sets up "dynamic queue" that allows inserting/deleting tasks
static void startupFunction(void* arg) {
  dataStartup* localDataPtr = (dataStartup*) arg;

  for (int i = 0; i < (taskSize-1); i++){    // Sets up the double linked list of tasks
    TCB* task = taskArray[i];
    TCB* nextTask = taskArray[i+1];
    task->next = nextTask;
    nextTask->prev=task;
  }
  
  taskArray[0]->prev = NULL;
  taskArray[taskSize-1]->next = NULL; 
}

// Function that reads the current and voltage values from its analog input
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
  }
 
}

// Function that iterates through all the tasks in its "dyanmic queue" also add the dynamic insert/delete tasks
static void schedulerFunction(void* arg) {
  dataScheduler* localDataPtr = (dataScheduler* ) arg;
 
  TCB* taskNode = head;
  while(taskNode != NULL){          
    taskNode->myTask((taskNode->taskDataPtr));    // Iterate through each node in the doubly linkedlist and call the respective task function
    taskNode = taskNode->next;
  }
}

// insert desired task to the task linked list
static void insertFunction(TCB* node){
  if (head == NULL){
    head = node;
    tail = node;
  } else{
    tail->next = node;
    node->prev = tail;
    node->next = NULL;
    tail = node;
  }
}

// delete desired task to the task linked list
static void deleteFunction(TCB* node){
  TCB* temp = head;
  TCB* formerTail;
  if (head == NULL){
  } else if (head == tail){
    head = NULL;
    tail = NULL;
  } else if (head == node){
    temp = head->next;
    head->prev = NULL;
    head->next = NULL;
    head = temp;
  } else if (tail == node){
    formerTail = tail;
    temp = tail->prev;
    tail = temp;
    tail->next = NULL;
    formerTail->prev = NULL;
    formerTail->next = NULL;
  } else{
    while (temp != NULL){
      if (temp == node){
        TCB* nextNode = temp->next;
        TCB* prevNode = temp->prev;
        prevNode->next = nextNode;
        nextNode->prev = prevNode;
        temp->prev = NULL;
        temp->next = NULL;
      }
      temp = temp->next;
    } 
  }
}

// Serial ISR
void serialEvent() {
  //Serial.println("UART invoked");
  intraSystemCommFunction(intraTask.taskDataPtr);
}

// Function that facilitates UART communication on the peripheral side. Sends current and voltage readings
static void intraSystemCommFunction(void* arg) {
  dataIntra* localDataPtr = (dataIntra* ) arg;
  //Serial.println("In ISC Function");
  *(localDataPtr->commandPtr) = (int) Serial.read();
  //Serial.println(*(localDataPtr->commandPtr));
  if (*(localDataPtr->commandPtr) == 0) { // pass v and c to UART
    Serial.write(dataArray, 2 * NUMBER_OF_DATUMS);
  } 
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

  ////////////////// Scheduler ////////////////////
  schedulerTask.myTask = &schedulerFunction;
  dataScheduler schedulerData;
  schedulerTask.taskDataPtr = &schedulerData;
  schedulerData.pointerToTCBArray = &taskArray;
  schedulerData.firstNode = &measureTask;
  schedulerData.taskCounterPtr = &taskCounter;
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
  taskCounter++;
}

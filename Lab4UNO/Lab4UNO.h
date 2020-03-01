#ifndef _474Lab1_H    
#define _474Lab1_H 

struct MyStruct{
  void (*myTask)(void*);
  void* taskDataPtr;
  struct MyStruct* next;
  struct MyStruct* prev;
}; typedef struct MyStruct TCB;

/////////////////// taskDataPtr instantiations for each task ///////////////
struct measureStruct{
  uint8_t* dataArrayPtr;
  //uint8_t* currentHVPtr; 
  //uint8_t* voltageHVPtr; 
}; typedef struct measureStruct dataMeasure;

struct schedulerStruct{
  TCB* (*pointerToTCBArray)[1];
  TCB* firstNode;
  int* taskCounterPtr;
}; typedef struct schedulerStruct dataScheduler;

struct intraSysCommStruct{
  volatile int* commandPtr;
  uint8_t* dataArrayPtr;
  //uint8_t* currentHVPtr; 
  //uint8_t* voltageHVPtr; 
}; typedef struct intraSysCommStruct dataIntra;

struct startupStruct{
  TCB* (*pointerToTCBArray)[1];
}; typedef struct startupStruct dataStartup;

volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt


#endif //_474Lab1_H

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
  volatile int* currentHVPtr; //
  volatile int* voltageHVPtr; //
}; typedef struct measureStruct dataMeasure;

struct schedulerStruct{
  TCB* (*pointerToTCBArray)[1];
  TCB* firstNode;
}; typedef struct schedulerStruct dataScheduler;

struct intraSysCommStruct{
  volatile int* commandPtr;
  volatile int* currentHVPtr; //
  volatile int* voltageHVPtr; //
}; typedef struct intraSysCommStruct dataIntra;

struct startupStruct{
  TCB* (*pointerToTCBArray)[1];
}; typedef struct startupStruct dataStartup;

volatile int timeBaseFlag = 0; // Global time base flag set by Timer Interrupt


#endif //_474Lab1_H

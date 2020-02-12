#ifndef _474Lab1_H    
#define _474Lab1_H 

struct MyStruct{
  void (*myTask)(void*);
  void* taskDataPtr;
  struct MyStruct* next;
  struct MyStruct* prev;
}; typedef struct MyStruct TCB;

//enum alarm_state{
//  NOT_ACTIVE,
//  ACTIVE_NOT_ACKNOWLEDGED,
//  ACTIVE_ACKNOWLEDGED
//};
//const String* alarm_state[] = {"NOT_ACTIVE", "ACTIVE_NOT_ACKNOWLEDGED", "ACTIVE_ACKNOWLEDGED"};
//const String* basic_state[] = {"OPEN", "CLOSED"};
//const String* operating_state[] = {"NORMAL", "DEVICE_ERROR"};
//enum basic_state{
//  OPEN,
//  CLOSED
//};
//
//enum operating_state{
//  NORMAL,
//  DEVICE_ERROR
//};

#endif //_474Lab1_H

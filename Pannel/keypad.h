#ifndef KEYPAD_H
#define KEYPAD_H
#include <PortExpander_I2C.h>
#include <Arduino.h>
#include "Buzzer.h"

class Keypad {
  
  private:
    const String keys[4][4] = {{"D","#","0","*"},{"C","9","8","7"},{"B","6","5","4"},{"A","3","2","1"}};//array for decoding matrix position to keypress
    int antiBounceLoops = 5;
    int antiBounceCounter = 0;
    String key_antiBounceMemory_key = "";
    String key_number_buffer = "";
    bool key_number_buffer_active = false;
    PortExpander_I2C keyReader = NULL;
    Buzzer* buzzer = NULL;
    
  public:
    Keypad(byte pin, Buzzer *buzzer);
    void init();
    String monitorKeypad();
    String getKeyDown();
    void clearNumeric();
    void disableNumeric();
    int getNumeric();
    
};
#endif

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
  
  private:
    int buzzTimeout=0;
    int buzzThreshhold = 1;
    int buzzLoop = 1;
    int buzzLoopCurrent = 1;
    int buzzTone = 1;
    int buzzerPin = 0;
    
    
  public:
    Buzzer(int);
    void init();
    void buzz(byte, byte,int,int);
    void buzz(byte, byte);

    void loop();
    
};
#endif

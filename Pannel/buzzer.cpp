#include "Buzzer.h"
#include "Arduino.h"


Buzzer::Buzzer(int pin) {
  this->buzzerPin = pin;
}
void Buzzer::init() {
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin,LOW);
}

void Buzzer::loop(){
    if (this->buzzTimeout > 0) {
      this->buzzTimeout--;  
     
      if (this->buzzLoopCurrent < 0){
        this->buzzLoopCurrent = this->buzzLoop;
      }
      if (this->buzzLoopCurrent >= this->buzzThreshhold){
        //analogWrite(this->buzzerPin, this->buzzTone);
      } else {
        analogWrite(this->buzzerPin, 0);
      }
      this->buzzLoopCurrent--;
  } else {
      analogWrite(this->buzzerPin, 0);
  }
}

void Buzzer::buzz(byte tone, byte duration,int t,int l){
  this->buzzLoop = l;
  this->buzzThreshhold = t;
  this->buzzTone = tone;
  this->buzzTimeout = duration;
  //analogWrite(this->buzzerPin, tone);
}

void Buzzer::buzz(byte tone, byte duration){
  this->buzzLoop = 1;
  this->buzzThreshhold = 0;
  this->buzzTone = tone;
  this->buzzTimeout = duration;
 // analogWrite(this->buzzerPin, tone);
}

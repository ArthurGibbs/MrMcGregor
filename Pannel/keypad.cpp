#include "Keypad.h"
#include "Arduino.h"
#include <PortExpander_I2C.h>

Keypad::Keypad(uint8_t i2c_addr, Buzzer* buzzer) {
  this->keyReader = PortExpander_I2C(i2c_addr); 
  this->buzzer = buzzer;
}
void Keypad::init() {
  Serial.print("Initializing keypad...");
  this->keyReader.init();
  for(int idx = 0; idx < 4; idx++ ) {
    this->keyReader.pinMode(idx,OUTPUT);
    this->keyReader.digitalWrite(idx, 0); 
  }
  for(int idx = 4; idx < 8; idx++ ) {
    this->keyReader.digitalWrite(idx, 0); 
    this->keyReader.pinMode(idx,INPUT);
  }
  Serial.println("OK");
}
String Keypad::getKeyDown(){
  String currentKey = "";
   int currentKeysPressed = 0;
      for(int idx = 0; idx < 4; idx++ ) {
       this->keyReader.digitalWrite(idx, 0); 
      for(int idy = 0; idy < 4; idy++ ) {
        if ( this->keyReader.digitalRead(idy + 4) == 0) {   
             currentKey = keys[idx][idy];
             currentKeysPressed++;
        }
      }
       this->keyReader.digitalWrite(idx, 1); 
    }
    if (currentKeysPressed > 1){
      Serial.println("Multiple keypress!");
      buzzer->buzz(255, 10, 1, 3);
      return "";
    }
    if (currentKeysPressed == 0){
      return "";
    }
    if (currentKeysPressed == 1){
      buzzer->buzz(50, 10, 0, 1);
      return currentKey;
    }
}

String Keypad::monitorKeypad() {
  String key_active = "";
  if (antiBounceCounter > 0) {
    antiBounceCounter--;
  } 
  String currentKey = getKeyDown();
  if(currentKey != "" && currentKey != this->key_antiBounceMemory_key){
    //new key
    this->key_antiBounceMemory_key = currentKey;
    key_active = currentKey;
    this->antiBounceCounter = this->antiBounceLoops;
   // buzzFor(2, 1, 0, 1);
  } else if (this->antiBounceCounter == 0 && currentKey == "") {
    this->key_antiBounceMemory_key = "";
  }


  if (key_number_buffer_active && key_active != "" && key_active != "A"&& key_active != "B"&& key_active != "C"&& key_active != "D"&& key_active != "#"&& key_active != "*"){
    key_number_buffer = key_number_buffer + key_active;
      Serial.println(key_number_buffer);
  }
  return key_active;
}

void Keypad::clearNumeric(){
  key_number_buffer_active = true;
  key_number_buffer = "";
}

void Keypad::disableNumeric(){
  key_number_buffer_active = false;
}

int Keypad::getNumeric(){
  return key_number_buffer.toInt();
}

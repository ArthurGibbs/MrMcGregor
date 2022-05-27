#include <SI7021.h>

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <PortExpander_I2C.h>
#include <LiquidCrystal_I2C.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x60); // @60
SI7021 md; // @40
PortExpander_I2C relays(0x38); //@38
PortExpander_I2C keypad(0x39); //@39
LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); //@ 27

//array for decoding matrix position to keypress
String keys[4][4] = {{"D","#","0","*"},{"C","9","8","7"},{"B","6","5","4"},{"A","3","2","1"}};

float globalTemperature = 0;
int globalHumidity = 0;
int measureEveryCount = 0;
int globalDisplayIdxTimer;
int globalDisplayIdx;
int globalDisplayIdxMax = 2;
int cycleTimer = 0;

int tempOutTimer = 0;
int lastMillis=0;
int globalHumidityState=0;
int statusFlags = 0;

int buzsound=0;
int antiBounceCounter = 0;
String activeKey = "";

void setup() {
  Wire.begin();
  
  //setup relays and turn them all off
  relays.init();
  for(int idx = 0; idx < 8; idx++ ) {
    //1 is off for some reason
    relays.digitalWrite(idx, 1); 
  }

  keypad.init();
  for(int idx = 0; idx < 4; idx++ ) {
    keypad.pinMode(idx,OUTPUT);
    keypad.digitalWrite(idx, 0); 
  }
  for(int idx = 4; idx < 8; idx++ ) {
    keypad.digitalWrite(idx, 0); 
    keypad.pinMode(idx,INPUT);
  }

  //setup pwm output board
  pwm.begin();
  pwm.setPWMFreq(30);  // This is the maximum PWM frequency 

  //serial for dev and monitoring
  Serial.begin(9600);

  //activate LCD module
  lcd.begin (20,4); // for 16 x 2 LCD module
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // set cursor to 0,0
  lcd.print("  Mr McGregor V1.1  "); 
  lcd.setCursor (0,1);
  
  //IO pins
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(2,HIGH);
  digitalWrite(3,LOW);
  digitalWrite(4,LOW);
  digitalWrite(5,LOW);

  pinMode(11, OUTPUT);
  digitalWrite(11,LOW);
  
  lastMillis = millis();
}
 
void loop() {
  monitorKeypad();
  delay(10);
  if (activeKey != "") {
    Serial.println(activeKey);
  }

  if (measureEveryCount == 0) {
      monitorTemperature();
      
      Serial.println("Temp: " + (String)globalTemperature);
      lcd.home (); // set cursor to 0,0
      lcd.print("                    "); 
      lcd.setCursor (0,0);
      lcd.print("Temp: " + (String)globalTemperature); 
      measureEveryCount = 20;
  }
  measureEveryCount--;

}

void monitorKeypad() {
  int activeKeys = 0;
  if (buzsound > 0) {
          buzsound--;
          digitalWrite(11, HIGH);
  } else {
          digitalWrite(11, LOW);
  }
  if (antiBounceCounter > 0) {
    antiBounceCounter--;
  } 
  if (antiBounceCounter == 0 && activeKey == "") {
    for(int idx = 0; idx < 4; idx++ ) {
      keypad.digitalWrite(idx, 0); 
      for(int idy = 0; idy < 4; idy++ ) {
        if (keypad.digitalRead(idy + 4) == 0) {   
             activeKey = keys[idx][idy];
             antiBounceCounter = 10;
             activeKeys++;
        }
      }
      keypad.digitalWrite(idx, 1); 
    }
    if (activeKeys == 0){
      activeKey = "";
    }
    if (activeKeys == 1){
      buzsound = 20;
    }
    if (activeKeys > 1){
      Serial.println("Multiple keypress!");
      activeKey = "";
    }
  }
  if (activeKeys == 0){
     activeKey = "";
  }
}

void cycleLoop(int chunkSize) {
  globalDisplayIdxTimer+= chunkSize;
  if (globalDisplayIdxTimer > 1000){
    globalDisplayIdxTimer-=1000;
    globalDisplayIdx++;  
    if (globalDisplayIdx > globalDisplayIdxMax){
      globalDisplayIdx=0; 
    }
    //print page
    lcd.setCursor (17,3);
    lcd.print((String)(globalDisplayIdx+1)+"/"+(globalDisplayIdxMax+1)); 
  }
  String title="";
  String value="";
  
  if (globalDisplayIdx==0) {
    title = "  Air Temperature   "; 
    value = "  "+ (String)globalTemperature+(char)223;
  }
  if (globalDisplayIdx==1) {
    title = "  Humidity          "; 
    value = "  "+ (String)globalHumidity+"%";
  }
  if (globalDisplayIdx==2) {
    if (bitRead(statusFlags,1) == 0) {
      title = "  Filling bed in    ";
      value = "  "+ (String)(cycleTimer/1000)+" Seconds";
    }else {
      title = "  Draining bed in    ";
      value = "  "+ (String)((cycleTimer+5000)/1000)+" Seconds";
    } 
   
  }
  
  while( title.length() < 20){
     title+=" "; 
  }
  while( value.length() < 20){
     value+=" "; 
  }
  lcd.setCursor (0,1);
  lcd.print(title);
  lcd.setCursor (0,2);
  lcd.print(value);
  

 
  digitalWrite(13,HIGH);
  monitorTemperature();
 
  regulateTemperature();
  regulateFloodDrain(chunkSize);

  //temp humidity
  tempOutTimer += chunkSize;
  if (tempOutTimer > 1000) {
    Serial.println("DC:"+(String)globalTemperature);
    Serial.println("RH:"+(String)globalHumidity); 
    tempOutTimer = 0 ;  
  }
  
  if (globalHumidity > 80 and globalHumidityState == 0) {
    Serial.println("Opening window");
    globalHumidityState = 1;
    pwm.setPWM(0, 0, (4095/25)*2);
  } else if (globalHumidity < 78 and globalHumidityState == 1) {
    Serial.println("Closing window");
    globalHumidityState = 0;
    pwm.setPWM(0, 0, (4095/25)*1);
  } else if (globalHumidity < 65 and globalHumidityState == 0) {
    Serial.println("Starting Misting");
    globalHumidityState = 2;
  } else if (globalHumidity > 70 and globalHumidityState == 2) {
    Serial.println("Stoping Misting");
    globalHumidityState = 0;
  }
}

void regulateFloodDrain(int chunkSize) {
  cycleTimer-= chunkSize;
  if (cycleTimer <= 0) {
    if (bitRead(statusFlags,1) == 0) {
      if (readConductivitySensor(0) > 200){
        bitWrite(statusFlags,0,1);
        Serial.println("ERROR: Drain sensor triggered before fill!");
      }
      Serial.println("Starting Filling Cycle");
      bitWrite(statusFlags,1,1);
      relays.digitalWrite(1, 0);
    }  
    if (readConductivitySensor(0) > 200) {
      Serial.println("Filling Cycle Ended");
      bitWrite(statusFlags,1,0);
      relays.digitalWrite(1, 1); 
      cycleTimer = 10000;  
    }    
    if (cycleTimer < -5000) {
      Serial.println("Filling Cycle Timed Out");
      bitWrite(statusFlags,1,0);
      relays.digitalWrite(1, 1); 
      cycleTimer = 10000 ;  
    }
  }
}

void regulateTemperature() {
  if (globalTemperature > 20) {
     bitWrite(statusFlags,2,1); 
     relays.digitalWrite(0, 0);
  }else {
     bitWrite(statusFlags,2,0);
     relays.digitalWrite(0, 1);
  }
}

void monitorTemperature(){
  Wire.begin();
  md.begin();
  float tempTemp = (float)md.getCelsiusHundredths()/100;
  //int TempRh = md.getHumidityPercent();
  globalTemperature = tempTemp;
  //globalHumidity = TempRh;
}

int readConductivitySensor(int idx) {
    digitalWrite(8 ,bitRead(idx,0));
    digitalWrite(9 ,bitRead(idx,1));
    digitalWrite(10 ,bitRead(idx,2));
    digitalWrite(11,bitRead(idx,3));
    digitalWrite(12,LOW);
    //settle time
    delay(50); 
    int value = 1023 - analogRead(3);
    digitalWrite(12,HIGH);
    return value;
}


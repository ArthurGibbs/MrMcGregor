#include <Wire.h>
#include <PortExpander_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "Keypad.h"
#include "Buzzer.h"



LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); //@ 27
PortExpander_I2C  relays(0x38); //@[38 - 40 hex 8 spaces
PortExpander_I2C  switches(0x25); //@[20-28]

// PINS

byte led_pin_r = 3;
byte led_pin_g = 5;
byte led_pin_b = 6;

//*********************************************** Keypad
String key_active = "";

Buzzer buzzer = Buzzer(11);

Keypad keypad = Keypad(0x21, &buzzer); //@39

//flood drain states
int state_draining = 0;
int state_filling = 1;
int state_holding = 2;

//screen
int ScreenTimeout = 0;
int DefaultScreenTimeout = 5000;
String screenBuffer[4] = {"","","",""};

//**************************************** Menu
int menuStack[5] = {0,0,0,0,0}; //allows for 5 levels of menu
int menuLevel = 0; //current height of menu
String DrawnMenuStatus = ""; //used to remember if we need to draw new state to screen
String menuItemStatus = ""; 
int maxMenuIndex = 0; // used to limit generic loops

const String menuMap[8][4] = {
  {"0","Beds","C","00"},
    {"00","Bed1","C","101"},
      {"00","Bed1","C","101"},
        {"000","Fill Time","C","0000"},
        {"000","Drain Time","C","0001"},
    {"01","Bed2","C","101"},
    {"02","Bed3","C","101"},
  {"1","Air","C","10"}};


// memory positions int is 2 bytes
int memPOSbeds[5] = {0,2,4,6,8};
int memPOSbedDrain[5] = {12,14,16,18,20};

// logic
// flood drain/watering control
int bedFloodTimes[5] = {0,0,0,0,0};
int bedDrainTimes[5] = {0,0,0,0,0};


// air circulation control
int TempOm = "0";
int TempMax = "0";
int TempHistorisis = "1";
int HumidityOn = "0";
int HumidityMax = "0";
int HumidityHistorisis = "1";


//food
//future thing
//lighting

//********* Temp logic
long bedTimer[5] = {0,0,0,0,0};
byte bedState[5] = {0,0,0,0,0};

boolean fanStatus = false;
long lastTimestamp = 0;
int timestep = 0;
int secondParts = 0;

void setup() {
  Serial.begin(9600);//serial for dev and monitoring
  Serial.println("");
  Serial.println("Booting...");
  //led setup
  pinMode(led_pin_r, OUTPUT);
  pinMode(led_pin_g, OUTPUT);
  pinMode(led_pin_b, OUTPUT);
  led(255,0,0);

  


  
  Wire.begin();//for i2c
  Wire.setClock(1000UL);//slow it down for longer max distance
  
  keypad.init();
  setup_lcd();
  setupFloodDrain();
  
  loadMemory();
//    for(int idx = 0; idx < 5; idx++ ) {
//    bedState[idx] = 0;
//  }
  delay(50);
  lcd.clear();
  lcd.print("  Mr McGregor V1.2  ");
  delay(2000);
  led(0,0,0);
  buzzer.buzz(20, 10, 1, 4);
  lcd.clear();
}

void led(int r, int g, int b){
 analogWrite(led_pin_r, 255-r);  
 analogWrite(led_pin_g, 255-g);  
 analogWrite(led_pin_b, 255-b);  
}

void setup_lcd(){
  //activate LCD module
  lcd.begin (20,4); // for 16 x 2 LCD module
  lcd.setBacklightPin(3,POSITIVE);
  wakeScreen();
  lcd.home (); // set cursor to 0,0
  lcd.print("Booting");  
  delay(50);
}
void setupFloodDrain(){
  switches.init();
  for(int idx = 0; idx < 8; idx++ ) {
     switches.pinMode(idx,INPUT);
  }
  
  //relays.init();
  for(int idx = 0; idx < 8; idx++ ) {
    relays.digitalWrite(idx,1);  //1 is off for some reason
  }
}
void loadMemory(){
   Serial.print("Loading memory..");
	  //load in bed flood times
  for(int idx = 0; idx < 5; idx++ ) {
    EEPROM.get( memPOSbeds[idx], bedFloodTimes[idx]);
  }

  //load in bed drain times
  for(int idx = 0; idx < 5; idx++ ) {
    EEPROM.get( memPOSbedDrain[idx], bedDrainTimes[idx]);
  }
  Serial.println("OK");
}

void loop() {
  
  key_active = keypad.monitorKeypad();
  buzzer.loop();
  monitorScreen();
  monitorState();
 

  long now = millis();
  timestep = int(now - lastTimestamp);
  lastTimestamp = now;
 

  secondParts += timestep;
  if (secondParts  >= 1000){
    secondParts -= 1000;
    for(int idx = 0; idx < 5; idx++ ) {
      bedTimer[idx]++;
    }
    for(int idx = 0; idx < 1; idx++ ) {// 0 1 2 3 4
      if(int(bedState[idx]) > 2 || int(bedState[idx]) < 0) {
        Serial.println("bedState idx"+ String(idx)+" has value "+String(bedState[idx])+" error!!! BUFFER OVERFLOW!!!");
        bedState[idx] = state_draining;
       // buzzFor(255, 10, 1, 3);
      }
      if (bedState[idx] == state_draining){ //draining
        if (bedTimer[idx] >= bedDrainTimes[idx] && !isAnyBedFilling() && !isAnyBedHolding()){//prevent fill of other beds have water
         // Serial.println("none blocking going to fill");
          //for(int idx = 0; idx < 5; idx++ ) {
          //  Serial.println("bedState idx" + String(idx) + " : " + String(bedState[idx]));
        //  }
          
          if(idx < 3){ //beds idx 0-2 sense fill
            bedState[idx] = state_filling; 
            relays.digitalWrite(idx, 0); //close valve
          } else {
            bedState[idx] = state_holding; //valves idx 3+ jump to hold timer
          }
          relays.digitalWrite(idx+3, 0); //turn on water
          
          //buzzFor(50, 60, 5,10);
          Serial.println("Filling bed #" + String(idx+1));
          
          if(menuLevel ==0){
            bufferedWrite(2, 0, "Filling bed #" + String(idx + 1));
          }
        } 
      } else if (bedState[idx] == state_filling){ //filling
        if (switches.digitalRead(idx) == 1){//fill sensor
          Serial.println("sensor #" + String(idx+1));
          bedState[idx] = state_holding;
          bedTimer[idx] = 0;
          //buzzFor(100, 10, 3,6);
          relays.digitalWrite(idx+3, 1); //turn off water
          Serial.println("Holding bed #" + String(idx+1));
          if(menuLevel ==0){
            bufferedWrite(2, 0, "Holding bed #" + String(idx +1));
          }
          
        } 
      } else if (bedState[idx] == state_holding){ //holding
        if (bedTimer[idx] >= bedFloodTimes[idx]){
          bedState[idx] = state_draining;
          bedTimer[idx] = 0;
          //buzzFor(200, 30, 4,8);
          Serial.println("Draining bed #" + String(idx+1));
          relays.digitalWrite(idx+3, 1); // turn off water why
          if(idx < 3){
            relays.digitalWrite(idx, 1); //open valve to drain
          }
          if(menuLevel ==0){
            bufferedWrite(2, 0, "Draining bed #" + String(idx + 1));
          }
        } 
      }    
    }
   
  }

  
  delay(10);
}

bool isAnyBedFilling(){
  for(int idx = 0; idx < 5; idx++ ) {
    if (bedState[idx] == 1){ //filling
      return true;
    }
  }
  return false;
}

bool isAnyBedHolding(){
  for(int idx = 0; idx < 3; idx++ ) {
    if (bedState[idx] == 2){ //holding
      return true;
    }
  }
  return false;
}
void wakeScreen(){
  ScreenTimeout = DefaultScreenTimeout;
  lcd.setBacklight(HIGH);
}
void monitorScreen(){
  if (ScreenTimeout > 0) {
    ScreenTimeout--;
  }
  if (ScreenTimeout == 0) {
      lcd.setBacklight(LOW);
      menuLevel = 0;
  }
  if (key_active != ""){
    wakeScreen();
  }
  
}
void monitorState(){
  if(key_active == "B" && menuStack[menuLevel] < maxMenuIndex){
     menuStack[menuLevel]++;
  }
  if(key_active == "A" && menuStack[menuLevel] > 0){
     menuStack[menuLevel]--;
  }
  if ( menuStack[menuLevel] > maxMenuIndex){
      menuStack[menuLevel] = maxMenuIndex;
  }
  if (menuLevel < 0){
      menuLevel = 0;
  }
  if(key_active == "D" ){
    if (menuLevel > 0){
      menuLevel--;
    }
    menuItemStatus = "";
    DrawnMenuStatus = "";
    keypad.disableNumeric();
  }
  
  if (menuLevel == 0){//home
    if (DrawnMenuStatus != "HOME"){
      DrawnMenuStatus = "HOME";
      lcd.clear();
      
    }
    if(key_active == "C"){
      menuLevel = 1;
      menuStack[menuLevel] = 0;
      key_active = "";
    }
  }
  if (menuLevel == 1){//menu
    if (DrawnMenuStatus != "MENU"){
      DrawnMenuStatus = "MENU";
      bufferedWrite(0,0,"Menu");
      lcdClearRow(1);
      lcdClearRow(2);

      maxMenuIndex = 1;
    }
    if(menuStack[menuLevel] == 0){
      if(menuItemStatus != "FLOODDRAIN"){
        menuItemStatus = "FLOODDRAIN";
        bufferedWrite(1,0,"Beds");
        lcdClearRow(2);
      }
      if(key_active == "C"){
               menuLevel = 2;
               menuStack[menuLevel] = 0;
               key_active = "";
      }
    }
    if(menuStack[menuLevel] == 1){
      if(menuItemStatus != "AIR"){
        menuItemStatus = "AIR";
        bufferedWrite(1,0,"Air");
        lcdClearRow(2);
      }
      if(key_active == "C"){
               menuLevel = 2;
               menuStack[menuLevel] = 0;
               key_active = "";
      }
    }
  }
  if (menuStack[1] == 0 && menuLevel == 2){
    //FLOODDRAIN
    if (DrawnMenuStatus != getMenuId()){
      DrawnMenuStatus = getMenuId();
      lcd.clear();
      bufferedWrite(0,0,"Menu>Beds"); 
      maxMenuIndex = 4;
    }
    if(menuItemStatus != "BED" + String(menuStack[menuLevel]+1)){
      menuItemStatus = "BED" + String(menuStack[menuLevel]+1);
      bufferedWrite(1,2,"BED" + String(menuStack[menuLevel]+1)); 
    }
    if(key_active == "C"){
             menuLevel = 3;
             menuStack[menuLevel] = 0;
             key_active = "";
    }
  }
  if (menuStack[1] == 0 && menuLevel == 3){  //generic bedx menu
    //bed1
    if (DrawnMenuStatus != getMenuId()){
      DrawnMenuStatus = getMenuId();
      bufferedWrite(0,0,"Menu>Beds>" + String(menuStack[2]+1)); 
      maxMenuIndex = 1;
  	 // if (menuStack[2] > 2) {
  		//  maxMenuIndex = 0; //disable drain controles for bed 4-5
  	 // }
    }
    if(menuStack[menuLevel] == 0){
      if(menuItemStatus != "FLOODTIME"){
        menuItemStatus = "FLOODTIME";
        bufferedWrite(1,2,"Flood Time (s)");
        bufferedWrite(2,2,String(bedFloodTimes[menuStack[2]])); 
      }
    }
    if(menuStack[menuLevel] == 1){
      if(menuItemStatus != "DRAINTIME"){
        menuItemStatus = "DRAINTIME";
        bufferedWrite(1,2,"Drain Time (s)"); 
        bufferedWrite(2,2,String(bedDrainTimes[menuStack[2]])); 
      }
    }
    if(key_active == "C"){
             menuLevel = 4;
             menuStack[menuLevel] = 0;
             key_active = "";
    }
  }

  //              water  any bed     
  if (menuStack[1] == 0 &&  menuLevel == 4){  //generic bedx flood/drain page
    //bed1
    int* value;
    int* mempos;
    if (menuStack[3] == 0){
        value = &bedFloodTimes[menuStack[2]];
        mempos = &memPOSbeds[menuStack[2]];
    } else if(menuStack[3] == 1){
        value = &bedDrainTimes[menuStack[2]];
        mempos = &memPOSbedDrain[menuStack[2]];
    }
    
    if (DrawnMenuStatus != getMenuId()){
      DrawnMenuStatus = getMenuId();
      lcd.clear();
      if (menuStack[3] == 0){
        bufferedWrite(0,0,"Menu>Bed>" + String(menuStack[2]+1) + ">FT(s)");
      }
      if (menuStack[3] == 1){
        bufferedWrite(0,0,"Menu>Bed>" + String(menuStack[2]+1) + ">DT(s)");
      }
      maxMenuIndex = 1;
      keypad.clearNumeric();

      lcd.setCursor (2,1);
      lcd.print(*value); 
    }
  
    if(key_active == "A"){
            *value = *value +1;
            bufferedWrite(1,2,String(*value));
            EEPROM.put( *mempos, *value);
    }  
    if(key_active == "B"){
            *value = max(0,*value -1);
            bufferedWrite(1,2,String(*value));
            EEPROM.put( *mempos, *value);
    }  

    if (keypad.getNumeric() != 0) {
            bufferedWrite(1,2,String(keypad.getNumeric()));
            bufferedWrite(2,2,"Unsaved");
    }

    if(key_active == "C"){
            if (keypad.getNumeric() != 0) {
              *value = keypad.getNumeric();
              keypad.clearNumeric();
              EEPROM.put( *mempos, *value);
             
              bufferedWrite(1,2,String(*value));
              bufferedWrite(2,2,"Saved");
              Serial.println("Saved");
            } else {
              Serial.println("NAN, not saving");
            }

            
    }
  }
  
  if (menuStack[1] == 1 && menuLevel == 2){
    //Air
    if (DrawnMenuStatus != "AIR"){
      DrawnMenuStatus = "AIR";
      lcd.clear();
      lcd.print("Menu>Air"); 
    }
    if(menuStack[menuLevel] == 0){
      if(menuItemStatus != "TEMP"){
        menuItemStatus = "TEMP";
        bufferedWrite(1,0,"Temp");
        lcdClearRow(2);
      }
      if(key_active == "C"){
               menuLevel = 3;
               menuStack[menuLevel] = 0;
               key_active = "";
      }
    }
    if(menuStack[menuLevel] == 1){
      if(menuItemStatus != "HUMIDITY"){
        menuItemStatus = "HUMIDITY";
        bufferedWrite(1,0,"Humidity");
        lcdClearRow(2);
      }
      if(key_active == "C"){
               menuLevel = 3;
               menuStack[menuLevel] = 0;
               key_active = "";
      }
    }   
  }
}
void lcdClearRow(byte col){
        lcd.setCursor(0,col);
        lcd.print("                    ");
}


String getMenuId(){
  String out = "";
  for(int idx = 0; idx < menuLevel; idx++ ) {
    out = out + String(menuStack[idx]);
  }
  return out;
}
void bufferedWrite(int col, int start, String line){
  String newline = "                    ";
  for(int idx = 0; idx < line.length(); idx++ ) {
    if (idx+ start < 20){
      newline.setCharAt(idx+ start, line.charAt(idx));
    }
  }
  if (newline != screenBuffer[col]){
    lcd.setCursor (0,col);
    lcd.print(newline); 
  }
}

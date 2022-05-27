#include <Adafruit_BME280.h>
#include <SI7021.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h>


#define SEALEVELPRESSURE_HPA (1013.25)
SI7021 md; // @40
Adafruit_BME280 bme;
#define ONE_WIRE_BUS 2 
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
// variable to hold device addresses
DeviceAddress Thermometer;

int deviceCount = 0;

uint8_t sensor1[8] = { 0x28, 0xAA, 0xEB, 0xAD, 0x16, 0x13, 0x02, 0x8D };
uint8_t sensor2[8] = { 0x28, 0xCD, 0xAA, 0x58, 0x1E, 0x13, 0x01, 0xD8 };

void setup() {
  Wire.begin();
  
  md.begin();
  bme.begin(0x76);
  //serial for dev and monitoring
  Serial.begin(9600);
   sensors.begin(); 


     Serial.println("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
  
  Serial.println("Printing addresses...");
  for (int i = 0;  i < deviceCount;  i++)
  {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);
    printAddress(Thermometer);
  }
}

void printAddress(DeviceAddress deviceAddress)
{ 
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}
 
void loop() {
  float tempTemp = (float)md.getCelsiusHundredths()/100;
  int TempRh = md.getHumidityPercent();
  Serial.println("Temp " + String(tempTemp));
  Serial.println("RH " + String(TempRh));

    Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println("*C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println("hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println("m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println("%");

  Serial.println();

  sensors.requestTemperatures();
  
  Serial.print("Sensor 1: ");
  printTemperature(sensor1);  
  
  Serial.print("Sensor 2: ");
  printTemperature(sensor2);


  
  delay(1000);
}

void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(tempC);
  Serial.print((char)176);
  Serial.print("C  |  ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
  Serial.print((char)176);
  Serial.println("F");
}

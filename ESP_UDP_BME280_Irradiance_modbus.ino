// WELLPRO DAQ 8CH

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <time.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include "OLED.h"
#include <SoftwareSerial.h>
#include <ModbusMaster.h>



SoftwareSerial mySerial(13, 14); // RX(1), TX(4) MAX487 
#define MAX485_DE      12

ModbusMaster node;

void preTransmission()
{
  //digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  //digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

ESP8266WiFiMulti wifiMulti;
WiFiUDP Udp;

IPAddress destinationIP(35,xxx,xxx,xxx);  // Address of target machine
unsigned int destinationPort = xxxx;      // Port to send to

//#define MY_DEBUG
#define USE_OLED
#define USE_BME280

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C #define BME280_ADDRESS                (0x76)

OLED display(4, 5);

char buf[20];
char str[20];

// Config WiFi
#define WIFI_SSID     "TP-Link"     
#define WIFI_PASSWORD "00000000"   

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

int timezone = 7;
int dst = 0;

unsigned long lastUpdateEnergy = 0, lastUpdateFirebase = 0;

const int sleepTimeS = 1800;

//ADC_MODE(ADC_VCC);

void setup() {
  bool status;  
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_DE, 0);
  
  mySerial.begin(9600);
  node.begin(1, mySerial);
  delay(50);
 
  Serial.begin(9600);
  delay(100);

#ifdef USE_BME280 
 status = bme.begin();  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
#endif
    
#ifdef MY_DEBUG         
       Serial.println("BME280 sensor Ok!!!");
       Serial.print("connecting wifi.");
#endif

WiFiManager wifiManager;

wifiManager.autoConnect("AutoConnectAP");

  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");
  
  while (!time(nullptr)) {
    delay(1000);
  }
  
#ifdef USE_OLED    
  display.begin();
  display.print("Environment Log.");
  sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
  display.print(buf,2); 
  sprintf (buf, "Connected!");
  display.print(buf,3);    
#endif
  delay(5000);
}

float Temperature = 0.0;
float Humidity = 0.0;
int cnt=0;

int Log_flag=0;
long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 1.0;
float Volts; 
float ADC1,ADC2,ADC3,ADC4,ADC5,ADC6,ADC7;

time_t prevDisplay = 0;

static char mydata[255];
ADC_MODE(0);

void loop() {
  int index=0; uint8_t j, result;
  long now_t = millis();
  long x;  
  unsigned long DataHex; 

#ifdef USE_BME280    
  Temperature = bme.readTemperature();
  Humidity = bme.readHumidity();
#endif     

    cnt++;

    Measure_data();

#ifdef USE_OLED    
  sprintf (buf, "T:%2.1fC %RH:%2.1f%%", Temperature,Humidity);
  display.print(buf,2);
  sprintf (buf, "ADC1 %2.4f", ADC1);
  display.print(buf,3);   
  sprintf (buf, "ADC2 %2.4f", ADC2);
  display.print(buf,4); 
  sprintf (buf, "ADC3 %2.4f", ADC3);
  display.print(buf,5); 
  sprintf (buf, "ADC4 %2.4f", ADC4);
  display.print(buf,6);       
#endif          

#ifdef MY_DEBUG   
  Serial.print(" Temp:");Serial.print(Temperature,1);
  Serial.print(" Humid:");Serial.print(Humidity,0);  
  Serial.print(" ADC1:");Serial.println(ADC1,4);
#endif 

  time_t now = time(nullptr);
  struct tm* nowTime = localtime(&now);
  if (((nowTime->tm_sec % 30) == 0)&&(Log_flag == 0)){
    Log_flag=1;
    lastUpdateFirebase = millis();   
    String DataSend = "{\"id\":\"Irr-ESP\",\"data\":"+ String(cnt)+",\"temperature\":"+ String(Temperature)+",\"humidity\":"+ String(Humidity)+",\"analog_in_1\":"+ String(ADC1)+"}";      
    DataSend.toCharArray(mydata,255);
    Udp.beginPacket(destinationIP, destinationPort);
    Udp.write(mydata);
    Udp.endPacket();

#ifdef MY_DEBUG       
    Serial.println("Sent->influxdb");    
#endif    
    sprintf (buf, "Sent->influxdb");
    display.print(buf,1);
  }
  if((nowTime->tm_sec % 30)!=0){
     Log_flag=0;  
     sprintf (buf, "waiting.......");
     display.print(buf,1);     
  }  
  delay(100);  
}

void Measure_data(){
  uint8_t j,a, result;
  unsigned long DataHex;  
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
  result = node.readHoldingRegisters(0, 2); //readHoldingRegisters
  ESP.wdtEnable(1);

  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC1 = DataHex*0.002442; 
  }   

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

   ESP.wdtDisable();
   result = node.readHoldingRegisters(2, 2); //readHoldingRegisters
   ESP.wdtEnable(1);
     
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC2 = (DataHex*10)/4095; 
  } 

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(4, 2); //readHoldingRegisters
  ESP.wdtEnable(1);
     
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC3 = (DataHex*10)/4095;
  }   

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(6, 2); //readHoldingRegisters
  ESP.wdtEnable(1);
     
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC4 = (DataHex*10)/4095; 
  }
/*
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(8, 2); //readHoldingRegisters
  ESP.wdtEnable(1);   
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC5 = (DataHex*10)/4095; 
  }  
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(10, 2); //readHoldingRegisters
  ESP.wdtEnable(1);   
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC6 = (DataHex*10)/4095; 
  }

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(12, 2); //readHoldingRegisters
  ESP.wdtEnable(1);   
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC7 = (DataHex*10)/4095; 
  }

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  ESP.wdtDisable();
   result = node.readHoldingRegisters(14, 2); //readHoldingRegisters
  ESP.wdtEnable(1);   
  if (result == node.ku8MBSuccess)
  {
    ((byte*)&DataHex)[3]= node.getResponseBuffer(0x01)>> 8;
    ((byte*)&DataHex)[2]= node.getResponseBuffer(0x01)& 0xff;
    ((byte*)&DataHex)[1]= node.getResponseBuffer(0x00)>> 8;
    ((byte*)&DataHex)[0]= node.getResponseBuffer(0x00)& 0xff;   
    ADC8 = (DataHex*10)/4095; 
  }      
*/
  
}
  
String getTime() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  String tmpNow = "";
  tmpNow += String(newtime->tm_year + 1900);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mon + 1);
  tmpNow += "-";
  tmpNow += String(newtime->tm_mday);
  tmpNow += " ";
  tmpNow += String(newtime->tm_hour);
  tmpNow += ":";
  tmpNow += String(newtime->tm_min);
  tmpNow += ":";
  tmpNow += String(newtime->tm_sec);
  return tmpNow;
}

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
//#include <Adafruit_BME280.h>
//#include <Adafruit_GFX.h>
#include "OLED.h"
//#include <SoftwareSerial.h>
//#include <ModbusMaster.h>
#include <Arduino.h>
#include <max6675.h>

int thermoDO = 12;
int thermoCS = 13;
int thermoCLK = 14;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

String temperature;  // Values read from sensor


//SoftwareSerial mySerial(13, 14); // RX(1), TX(4) MAX487 
//#define MAX485_DE      12

//ModbusMaster node;
/*
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
*/
ESP8266WiFiMulti wifiMulti;
WiFiUDP Udp;

IPAddress destinationIP(35,201,5,171);  // Address of target machine
unsigned int destinationPort = 5051;      // Port to send to

#define MY_DEBUG
#define USE_OLED
//#define USE_BME280

#define SEALEVELPRESSURE_HPA (1013.25)

//Adafruit_BME280 bme; // I2C #define BME280_ADDRESS                (0x76)

OLED display(4, 5);

char buf[20];
char str[20];

// Config WiFi
#define WIFI_SSID     "TP-Link_FB54"     //"HUAWEIJAK"       "TP-Link_FB54"      "TP-LINK"   "HUAWEIP20"
#define WIFI_PASSWORD "80026983"   // "1234567890"     "80026983"  "123456789" "1234567890"

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
//  pinMode(MAX485_DE, OUTPUT);
//  digitalWrite(MAX485_DE, 0);
  
//  mySerial.begin(9600);
//  node.begin(1, mySerial);
  //node.preTransmission(preTransmission);
  //node.postTransmission(postTransmission);
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
/*
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
#ifdef MY_DEBUG     
  Serial.println(WiFi.localIP());
#endif

#ifdef USE_OLED  

#endif
*/
  
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");
  
  while (!time(nullptr)) {
    delay(1000);
  }
  
    //Serial.println(getTime());

#ifdef USE_OLED    
  display.begin();
  display.print("Environment Log.");
  sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
  display.print(buf,2); 
  sprintf (buf, "Connected!");
  display.print(buf,3);    
#endif
  thermocouple.begin(thermoCLK, thermoCS, thermoDO);
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

    cnt++;

#ifdef USE_OLED    
  sprintf (buf, "T:%2.1f ", Temperature);
  display.print(buf,5);       
#endif          

  time_t now = time(nullptr);
  struct tm* nowTime = localtime(&now);
  if (((nowTime->tm_sec % 30) == 0)&&(Log_flag == 0)){
    Temperature = float(thermocouple.readCelsius());
    Serial.println(Temperature);
    Log_flag=1;
    lastUpdateFirebase = millis();   
    String DataSend = "{\"id\":\"PVtemp-ESP\",\"data\":"+ String(cnt)+",\"temperature\":"+ String(Temperature)+"}";      
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

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <time.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include "OLED.h"

ESP8266WiFiMulti wifiMulti;
WiFiUDP Udp;

IPAddress destinationIP(35,xxx,xxx,xxx);  // Address of target machine
unsigned int destinationPort = xxxx;      // Port to send to

#define MY_DEBUG
#define USE_OLED

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C #define BME280_ADDRESS                (0x76)

OLED display(4, 5);

char buf[20];
char str[20];

// Config WiFi
#define WIFI_SSID     "HUAWEI"  
#define WIFI_PASSWORD "00000000"    

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

int timezone = 7;
int dst = 0;

unsigned long lastUpdateEnergy = 0, lastUpdateFirebase = 0;

const int sleepTimeS = 900;

//ADC_MODE(ADC_VCC);

void setup() {
  bool status;  

  Serial.begin(9600);
  delay(1000);

#ifdef USE_OLED    
  display.begin();
  display.print("Environment Log.");
#endif

    status = bme.begin();  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
    
#ifdef MY_DEBUG         
       Serial.println("BME280 sensor Ok!!!");
       Serial.print("connecting wifi......");
#endif

  //bool conn = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  //WiFi.mode(WIFI_STA);
  
  //wifiMulti.addAP("TP-LINK", "123456789");
  //wifiMulti.addAP("HUAWEIJAK", "1234567890");
  //wifiMulti.addAP("TP-Link_FB54", "80026983");
/*  
  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
*/

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
#ifdef MY_DEBUG     
  Serial.println(WiFi.localIP());
#endif

#ifdef USE_OLED  
  sprintf(buf, "IP:%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
  display.print(buf,2);
#endif
  
  configTime(timezone * 3600, dst * 0, "pool.ntp.org", "time.nist.gov");
  
#ifdef MY_DEBUG       
  Serial.println("Waiting for time");
#endif 
  
  while (!time(nullptr)) {
    delay(1000);
  }
  delay(5000);
}

float Temperature = 0.0;
float Humidity = 0.0;
int cnt=0;

time_t prevDisplay = 0;

static char mydata[255];

void loop() {
  int index=0; 
  long now_t = millis();
  int sensorValue = analogRead(A0);
  float volts = 3.30*(float)sensorValue/1023.00;
  //float get_Vcc = ESP.getVcc();

#ifdef MY_DEBUG   
  Serial.print("ADC: ");
  Serial.println(volts);  
#endif 

  Temperature = bme.readTemperature();
  Humidity = bme.readHumidity();

#ifdef USE_OLED    
  sprintf(buf,"Vbat:%2.3f",volts);
  display.print(buf,3);
  sprintf (buf, "T:%2.1fC %RH:%2.1f%%", Temperature,Humidity);
  display.print(buf,4);    
#endif            
  cnt++;
        
  String DataSend = "{\"id\":\"Soilmoisture-ESP\",\"data\":"+ String(cnt)+",\"temperature\":"+ String(Temperature)+",\"humidity\":"+ String(Humidity)+",\"analog_in_1\":"+ String(volts)+"}";      
  DataSend.toCharArray(mydata,255);
  
  Udp.beginPacket(destinationIP, destinationPort);
  Udp.write(mydata);
  Udp.endPacket();

    
#ifdef MY_DEBUG       
    //Serial.println("Sent and sleep for 15 Minute");
#endif       
    //ESP.deepSleep(sleepTimeS * 1000000); //ESP.deepSleep(0);
    delay(5000);
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

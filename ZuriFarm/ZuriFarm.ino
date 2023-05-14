#include <WiFi.h> 
#include<ThingSpeak.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "RTClib.h"

// Declaring the credentials 
#define WIFI_SSID "DevKB"
#define WIFI_PASS "engineer254"

unsigned long channelID = 2130961;
const char *API_KEY = "FJ33Y1PVN6SPFG96";
const char *READ_API_KEY = "W39DE3LXTLMQOQAJ";

// Declaring GPIO pins
#define dhtPin 4
#define DHTTYPE DHT11
#define MQ2pin 34
#define CSMpin 35
#define PUMP 18

//sleep time interval
#define sleep_time 180
#define micro_second 1000000ULL


// calibration factors
const int wetValue = 2800;
const int dryValue = 1640;

unsigned long last_time = 0;
unsigned long Delay = 180000;


// Parameters to be read
float Temperature, Humidity;
float c02;
float soilMoisture;
int moistureContent, moisturePercent;

// instanciating RTC module (DS3231)
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int onHour;
int onMin;
float onTime;

LiquidCrystal_I2C lcd(0x27,16,2); 
DHT dht(dhtPin, DHT11);
WiFiClient client;

void setup() {

  lcd.init();             
  lcd.backlight();
  Serial.begin(9600);
  Serial.println("Zuri-Farm Initializing...");
  lcd.setCursor(0,0);
  lcd.print("ZURI-FARM...");

  WiFi.mode(WIFI_STA);

  Serial.print("ZuriFarm connecting to: ");
  Serial.print(WIFI_SSID);

  WiFi.begin(WIFI_SSID,WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("...");
    }
    Serial.println("");
    Serial.print("Zuri-Farm connected to: ");
    Serial.println(WIFI_SSID);
    Serial.print("Zuri-Farm IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.print("CONNECTED...");

    //initialize RTC clock Module
    if (! rtc.begin()) {
    Serial.println("Could not find RTC! Check circuit.");

    lcd.clear();
    lcd.print("RTC FAILED!");
    while (1);
  }

  rtc.adjust(DateTime(__DATE__, __TIME__));
  delay(3000);
    
  ThingSpeak.begin(client);
  dht.begin();
  
  pinMode(MQ2pin,INPUT);
  pinMode(CSMpin, INPUT);
  pinMode(PUMP,OUTPUT);
  delay(2000);

}

void loop() {
  checkStatus();
  delay(2000);
  readData();
  
  if(millis()-last_time > Delay){
  publishData();
  last_time = millis();
  }

}

void readData(){
  Temperature = dht.readTemperature();
  Humidity = dht.readHumidity();
  c02= analogRead(MQ2pin);
  
  moistureContent = analogRead(CSMpin);
  moisturePercent = map(moistureContent,wetValue,dryValue,0,100);
  moisturePercent = constrain(moisturePercent, 0,100);
  
  if(isnan(Temperature) || isnan(Humidity)){
    Serial.println("DHT Reading error...");
    }
    Serial.print("Temperature: ");
    Serial.println(Temperature);
    Serial.print("Humidity: ");
    Serial.println(Humidity);
    Serial.print("Carbon Concentration: ");
    Serial.println(c02);
    Serial.print("Moisture Content: ");
    Serial.println(moisturePercent);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("TEMP: ");
    lcd.print(Temperature);
    lcd.setCursor(0,1);
    lcd.print("HUMIDITY: ");
    lcd.print(Humidity);
    
    delay(3000);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MOISTURE: ");
    lcd.print(moisturePercent);
    lcd.setCursor(0,1);
    lcd.print("Co2: ");
    lcd.print(c02);
    delay(3000);
    }
void publishData(){

  ThingSpeak.setField(1,Temperature);
  ThingSpeak.setField(2,Humidity);
  ThingSpeak.setField(3,moisturePercent);
  ThingSpeak.setField(4,c02);
  ThingSpeak.writeFields(channelID,API_KEY);
//
  int dataC = ThingSpeak.writeFields(channelID,API_KEY);

  Serial.println(dataC);


    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SENDING DATA...");
    delay(2000);

    if(dataC == -210){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("SEND OK!");
      }else{
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("SEND FAIL! ");
        }//
  }
void checkStatus(){
  
  DateTime now = rtc.now();
  Serial.print(now.hour(),DEC);
  Serial.print(":");
  Serial.println(now.minute(),DEC);

  onTime = ThingSpeak.readFloatField(channelID, 5, READ_API_KEY);
  onHour = (int)onTime;
  onMin = (onTime - onHour)*100;

 
  Serial.print("Irrigation Time = ");
  Serial.print(onHour);
  Serial.print(":");
  Serial.println(onMin);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("IRRIGATING AT:");
    lcd.setCursor(0,1);
    lcd.print(onHour);
    lcd.print(":");
    lcd.print(onMin);
    delay(1000);
   

    //check irrigation

    if(moisturePercent <20){
      digitalWrite(PUMP,HIGH);
      delay(5000);
      } else{
        Serial.println("Soil Moisture OK"); 
        digitalWrite(PUMP,LOW);
        }
  

  if((now.hour() == onHour) && (now.minute()== onMin)){
    Serial.println("Irrigation Triggered!");
    lcd.clear();
    lcd.print("PUMP ON!");
    digitalWrite(PUMP,HIGH); // Irrigates the Farm for a minute
    
    }else{
      digitalWrite(PUMP,LOW);
      }
  }

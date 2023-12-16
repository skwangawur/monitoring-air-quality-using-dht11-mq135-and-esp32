#define BLYNK_TEMPLATE_ID "###########"
#define BLYNK_TEMPLATE_NAME "Monitoring Kandang Ayam"
#define BLYNK_AUTH_TOKEN            "###########"
#define BLYNK_PRINT Serial
#include <esp_system.h>
#include <SimpleTimer.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// DHT11 Config
#include "DHT.h"       
#define DHTPIN 4
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);
// DHT11 Config End

//MQ135 Gas Sensor Config
#include <MQUnifiedsensor.h>
#define Board "ESP-32"
#define Voltage_Resolution 5
#define MQ135_Pin 32
#define MQType "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6
MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, MQ135_Pin, MQType);
//MQ135 Gas Sensor Config End

// Antares Config
#include <AntaresESP32HTTP.h>
#define ACCESSKEY "###########"  
#define WIFISSID "###########"                              
#define PASSWORD "###########"                           
#define applicationName "###########"  
#define deviceName "ESP32"                    
AntaresESP32HTTP antares(ACCESSKEY); 
// Antares Config End 

//pwm kipas
#define IN1 13 // deklarasi pin IN1
#define IN2 12  // deklarasi pin IN2
#define IN3 27  // deklarasi pin IN3
#define IN4 26  // deklarasi pin IN4
#define ENA 14 // deklarasi pin ENA
#define ENB 15  // deklarasi pin ENB
//pwm kipas end

// Relay Pin Config
#define LAMPU 5
#define KIPAS 17
#define MISTMAKER 16
// Relay Pin Config End

int exhaustFan;
float fanSpeed = 100;
float nh3;
float temp;
float hum;
bool activeHeater = false; 
bool activeFan = false ;
bool activeMistMaker = false;
int milliSecondToSecond = 1000 * 30;
int k  = 0;
float newNh3, newHum, newTemp;

BlynkTimer blynkTimer;
SimpleTimer timer;

BLYNK_WRITE(V4)
{
  fanSpeed = param.asInt() ;
}


BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V4); 
}

void sendDataToBlynk()
{
  Blynk.virtualWrite(V2,nh3); 
  Blynk.virtualWrite(V1, hum); 
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V5, fanSpeed);
}

void mainSystem(){
  exhaustFan = (fanSpeed/100)*255;

  // put your main code here, to run repeatedly:
  MQ135.update();
  nh3 = MQ135.readSensor();
  hum = dht.readHumidity();     
  temp = dht.readTemperature(); 

  if(int(temp) < 33){
    digitalWrite(LAMPU, LOW);
    digitalWrite(KIPAS, HIGH); 
    activeHeater= true;
    activeFan= false;
  }else{ 
    digitalWrite(LAMPU, HIGH);
    digitalWrite(KIPAS, LOW);
    activeHeater= false;
    activeFan= true;
  }

  if(hum <= 50 || temp > 36){
    digitalWrite(MISTMAKER, LOW); 
    activeMistMaker= true;   
  }else{
    digitalWrite(MISTMAKER, HIGH);  
    activeMistMaker= false;   
  }

  if(nh3 > 20 || hum > 70){
    digitalWrite(KIPAS, LOW);    
    activeFan= true;
  }else{
    if(temp > 33){
      digitalWrite(KIPAS, LOW);    
      activeFan= true;
    }else{
      digitalWrite(KIPAS, HIGH);   
      activeFan= false; 
    }
  }
  
  antares.add("temperature: ", temp);
  antares.add("humidity: ", hum);
  antares.add("amonia: ", nh3);
  antares.add("Fan is Active: ", activeFan);
  antares.add("Heater is Active: ", activeHeater);
  antares.add("Mist Maker is Active: ", activeMistMaker);
  antares.add("Fan Speed: ", fanSpeed);
  antares.send(applicationName, deviceName);

  Serial.println("");
  Serial.println("-------------------------------------------");
  Serial.println("          Monitoring Kandang Ayam          ");
  Serial.println("-------------------------------------------");
  Serial.println("jumlah i: " + (String)k);
  Serial.println("Humidity: " + (String)hum + " %");  // Print kelembapan (%)
  Serial.println("Temperature: " + (String)temp + " *C");
  Serial.println("Amonia: " + (String)nh3 + "PPM");
  Serial.println("Fan is Active: " + (String)activeFan);
  Serial.println("Heater is Active: " + (String)activeHeater);
  Serial.println("Mist Maker is Active: " + (String)activeMistMaker);
  Serial.println("Fan Speed: " + (String)fanSpeed);
  Serial.println("Fan Speed: " + (String)exhaustFan);
  Serial.println("-------------------------------------------");
}


void setup() {

  Serial.begin(9600);
  //Antares Config
  antares.setDebug(true);   // Nyalakan debug. Set menjadi "false" jika tidak ingin pesan-pesan tampil di serial monitor
  antares.wifiConnection(WIFISSID,PASSWORD);  // Mencoba untuk menyambungkan ke WiFi
  //Antares Config End 

  blynkTimer.setInterval(1000L, sendDataToBlynk);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFISSID, PASSWORD);
  timer.setInterval(30000, mainSystem);
  

  // Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();   

  MQ135.setRegressionMethod(1);
  MQ135.setA(102.2);
  MQ135.setB(-2.473);
  MQ135.init();
  Serial.print("Calibrating please wahit");
  float calcRO = 0;
  for(int i = 1 ; i <= 10 ; i++){
    MQ135.update();
    calcRO += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcRO/10);
  Serial.print("done..");
  if(isinf(calcRO)){
    Serial.println("please check the wiring supply");
  }
  if(calcRO == 0){
    Serial.println("R0 is zero");
  }
  MQ135.serialDebug(true);

  // Konfigurasi pin-pin sebagai Output
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  // end


  // Relay Config
  pinMode(LAMPU, OUTPUT);
  pinMode(KIPAS, OUTPUT);
  pinMode(MISTMAKER, OUTPUT);
  // Relay Config End
}


void loop() {
  // Motor A dan B berputar ke kiri 2000 ms (2 detik) dengan kecepatan penuh
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, exhaustFan); // Mengatur kecepatan motor A (0-255)
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, exhaustFan); // Mengatur kecepatan motor B (0-255)
  //end


  Blynk.run();
  timer.run();
  blynkTimer.run();
}
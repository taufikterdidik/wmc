#include <Arduino.h>
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPUpdateServer.h>
  #define STASSID "IoT_TaufikHouse"
  #define STAPSK "2a6a7b8e3d53dd0f28862997e52a5a4d!"
#endif
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#if defined(ESP8266)
uint32_t PIN_LED = 2;
uint32_t PIN_ZERO_CROSS = 13;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(STM32F1xx)
uint32_t PIN_LED = PC13;
uint32_t PIN_ZERO_CROSS = PC12;
#endif

bool ledtoogle = false;
volatile uint32_t freq = 0;
volatile uint32_t last_isr = 0;

LiquidCrystal_I2C display(PCF8574A_ADDR_A21_A11_A01, 4,5,6,16,11,12,13,14, POSITIVE);

ICACHE_RAM_ATTR void zero_crossing_isr() {
  if(millis() - last_isr > 5) {
    freq++;
    last_isr = millis();
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting sketch...");

  #if defined(ESP8266)
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  MDNS.begin("washing-machine");
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("Loaded ESP8266 routines");
  #endif
  
  Wire.begin();
  delay(1000);
  byte error, address, i;
  int i2cdevices = 0;
  for(address=1; address < 127; address++){
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if(error==0){
      Serial.print(F("I2C:"));
      Serial.println(address, HEX);
      for(i=1; i<20; i++){
        ledtoogle = !ledtoogle;
        if(ledtoogle){
          digitalWrite(PIN_LED, HIGH);
        } else {
          digitalWrite(PIN_LED, LOW);
        }
        delay(50);
      }
    }
    i2cdevices++;
  }

  if(i2cdevices==0) {
    while(true){
      ledtoogle = !ledtoogle;
      if(ledtoogle){
        digitalWrite(PIN_LED, HIGH);
      } else {
        digitalWrite(PIN_LED, LOW);
      }
      delay(200);
    }
  }

  while(display.begin(16, 2, LCD_5x8DOTS) != 1) {
    Serial.println("Display failed");
    delay(2000);
    ledtoogle = !ledtoogle;
    if(ledtoogle){
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
  }
  digitalWrite(PIN_LED, HIGH);

  display.print(F("Loading..."));
  delay(1000);
  attachInterrupt(PIN_ZERO_CROSS, zero_crossing_isr, RISING);
  //display.clear();
  display.setCursor(0,0);
  display.print(F("                "));
  display.setCursor(0,0);
  display.print(F("Washing Machine"));
  display.setCursor(0, 1);
  display.print(F("   by Taufik"));
  delay(1000);
  display.setCursor(0,1);
  display.print(F("                "));
}

void loop() {
  #if defined(ESP8266)
  httpServer.handleClient();
  MDNS.update();
  #endif

  static uint32_t freqtimer = millis();
  if(millis() > freqtimer + 1000) {
    display.setCursor(0,1);
    display.print(F("                "));
    display.setCursor(0,1);
    display.print(freq/2);
    display.print(F(" Hz"));
    freq = 0;
    freqtimer = millis();
  }
}
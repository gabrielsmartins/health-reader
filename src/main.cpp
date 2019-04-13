#include "SSD1306.h" 
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1000


SSD1306  display(0x3c, 5, 4);

PulseOximeter pox;

uint32_t tsLastReport = 0;

void setupOLEDDisplay();
void setupMAX30100();
void displayHeartRate();
void displaySPO2();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  setupOLEDDisplay();
  setupMAX30100();
}

void setupOLEDDisplay(){
   Serial.println("Initializing Health Reader ...\n");
   display.init();
   display.flipScreenVertically();
   display.setFont(ArialMT_Plain_16);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawStringMaxWidth(0, 0, 128, "Initializing Health Reader ..." );
   display.display();
   delay(10000);
}

void onBeatDetected()
{
   Serial.println("Beat!");
}

void setupMAX30100(){
  display.clear();
  Serial.print("Initializing pulse oximeter...\n");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128,"Initializing pulse oximeter..." );
  display.display();
  delay(1000);

  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_17_4MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

  display.clear();

  while(!pox.begin()){
    Serial.println("FAILED");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.clear();
    display.drawStringMaxWidth(0, 0, 128,"Trying to intilize MAX30100 sensor ..." );
    display.drawStringMaxWidth(0, 32, 128,"Failed ..." );
    display.display();
  }

  Serial.println("SUCCESS");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 2, "Success ... ");
  display.display();  
}




void readMAX30100(){
  
  pox.update();
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    display.clear();
    displayHeartRate();
    displaySPO2();
    tsLastReport = millis();
  }
}

void displayHeartRate(){
   Serial.print("Heart rate: ");
   Serial.print(String(pox.getHeartRate()) + " BPM \n");
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 0,"BPM : ..." + String(pox.getHeartRate()));
   display.display();
}

void displaySPO2(){
   Serial.print("BPM / SpO2: ");
   Serial.print(String(pox.getSpO2()) + "% \n");
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 32,"SP02 : ..." + String(pox.getSpO2()) + "%");
   display.display();
}



void loop() {
    readMAX30100();
}
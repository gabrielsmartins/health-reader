#include "SSD1306.h" 
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1000


SSD1306  display(0x3c, 5, 4);

PulseOximeter pox;

uint32_t tsLastReport = 0;

void setupOLEDDisplay();
void setupMAX30100();
void displayHeartRate();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  setupOLEDDisplay();
  setupMAX30100();
}

void setupOLEDDisplay(){
   Serial.println("Initializing Health Reader ...");
   display.init();
   display.flipScreenVertically();
   display.setFont(ArialMT_Plain_16);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawStringMaxWidth(0, 0, 128, "Initializing Health Reader ..." );
   display.display();
   delay(100);
}

void onBeatDetected()
{
   Serial.println("Beat!");
}

void setupMAX30100(){
  display.clear();
  Serial.print("Initializing pulse oximeter...");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128,"Initializing pulse oximeter...\n" );
  display.display();
  delay(100);

  display.clear();
  if (!pox.begin()){
    Serial.println("FAILED");
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "Failed ... ");
  for(;;);
  } else {
    Serial.println("SUCCESS");
     display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "Success ... ");
  }
  display.display();
   delay(100);
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected);

}




void readMAX30100(){
  
  pox.update();
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
  Serial.print("Heart rate:");
  Serial.print(pox.getHeartRate());
  displayHeartRate();
  Serial.print("bpm / SpO2:");
  Serial.print(pox.getSpO2());
  Serial.println("%");
  
  tsLastReport = millis();
  }
}

void displayHeartRate(){
   display.clear();
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_CENTER);
   display.drawStringMaxWidth(0, 0, 128,"Heart rate: ..." );
   display.display();
}



void loop() {
  //readMAX30100();
}
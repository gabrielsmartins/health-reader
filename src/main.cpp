#include "SSD1306.h" 
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1500


SSD1306  display(0x3c, 5, 4);

PulseOximeter pox;
uint32_t tsLastReport = 0;

const int analogIn = A0;
 
int rawValue= 0;
double voltage = 0;
double tempC = 0;
double tempF = 0;

void setupOLEDDisplay();
void setupMAX30100Sensor();
void displayHeartRate();
void readHealth();
void readTemperature();
void displaySPO2();
void displayTemperature();


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  setupOLEDDisplay();
  setupMAX30100Sensor();
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

void setupMAX30100Sensor(){
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
    display.drawStringMaxWidth(0, 16, 128,"Failed ..." );
    display.display();
  }

  Serial.println("SUCCESS");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 2, "Success ... ");
  display.display();  
}




void readHealth(){
  
  pox.update();
  readTemperature();
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    display.clear();
    displayHeartRate();
    displaySPO2();
    displayTemperature(); 
    tsLastReport = millis();
  }
}

void readTemperature(){
  rawValue= 0;
  voltage = 0;
  tempC = 0;
  tempF = 0;

  rawValue = analogRead(analogIn);
  voltage = (rawValue / 2048.0) * 3300; // 5000 to get millivots.
  tempC = voltage * 0.1;
  tempF = (tempC * 1.8) + 32; // conver to F
 
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
   display.drawString(0, 16,"SP02 : ..." + String(pox.getSpO2()) + "%");
   display.display();
}

void displayTemperature(){
  Serial.print("Raw Value = " ); // shows pre-scaled value
  Serial.print(rawValue);
  Serial.print("\t milli volts = "); // shows the voltage measured
  Serial.print(voltage,0); //
  Serial.print("\t Temperature in C = ");
  Serial.print(tempC,1);
  Serial.print("\t Temperature in F = ");
  Serial.println(tempF,1);

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 32,"Temp : ..." + String(tempC) + " C");
  display.drawString(0, 48,"Temp : ..." + String(tempF) + " F");
  display.display();
}


void loop() {
    readHealth();
}
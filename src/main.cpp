#include "SSD1306.h" 
#include "MAX30100_PulseOximeter.h"
#include "WiFi.h"
#include "WiFiUdp.h"
#include "NTPClient.h"
#include "HTTPClient.h"

#define REPORTING_PERIOD_MS 1000


/************************
***** Display OLED *****
************************/
SSD1306  display(0x3c, 5, 4);

/**************************
***** MAX30100 Sensor *****
***************************/
PulseOximeter pox;
uint32_t tsLastReport = 0;

/***************************************
***** Analog SVP PIN (Temperature) *****
****************************************/
const int analogIn = A0;
 
/****************************************
**** Variables SVP PIN (Temperature) ****
*****************************************/
int rawValue= 0;
double voltage = 0;
double tempC = 0;
double tempF = 0;

/***************************************
***** WiFI Connection Information *****
****************************************/
char* ssid = "NET_2G468822";
char* password = "51468822";

/***************************************
***** WiFI Information to get Datetime *****
****************************************/
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


/***************************************
******* Endpoint API - IoT Health ******
****************************************/
String endpoint = "https://api-iot-health.herokuapp.com/v1";

void setupOLEDDisplay();
void setupMAX30100Sensor();
void setupWifi();
void displayHeartRate(float heartRate);
void displaySPO2(float spO2);
void readHealth();
double readTemperatureCelsius();
double convertTemperatureToFahrenheit(double tempC);
void displayTemperature(double tempC, double tempF);
void sendHttpRequest(int patientId, String measurementType, String measurementUnit, double mesaurementValue);


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  setupOLEDDisplay();
  setupWifi();
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

void setupWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  WiFi.begin(ssid, password); 
 
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);

  while (WiFi.status() != WL_CONNECTED) { 
    delay(1000);
    Serial.println("Connecting to WiFi...");
    display.drawStringMaxWidth(0, 0, 128,"Connecting to WiFi..." );
    display.display();
       if(WiFi.status() == WL_CONNECT_FAILED){
          display.drawStringMaxWidth(0, 32, 128,"Failed to connect to WiFi..." );
          display.display();
       }
    Serial.println("Wifi Status ..." + String(WiFi.status()));
  }
  display.clear();
  Serial.println("Connected to the WiFi network.");
  display.drawStringMaxWidth(0, 0, 128,"Connected to the WiFi network." );
  display.display();
  delay(1000);
  display.clear();
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


void sendHttpRequest(int patientId, String measurementType, String measurementUnit, double mesaurementValue){

   if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

        HTTPClient httpClient;
        
        httpClient.begin(endpoint + "/measurement");  //Specify destination for HTTP request
        httpClient.addHeader("Content-Type", "application/json");             //Specify content-type header
        
        timeClient.begin();
        while(!timeClient.update()) {
           timeClient.forceUpdate();
        }
        String formattedDate = timeClient.getFormattedDate();

        Serial.println("Datetime :" + formattedDate);

        String body = "{ \"patient\" : " + String(patientId) +
                          "\"datetime\" :" + String() +
                          "\"measurement_type\" : " + measurementType + 
                          "\"measurement_value\" : " + String(mesaurementValue) + 
                          "\"measurement_unit\" : " + String(measurementUnit) +
                      "}";

        

        int httpResponseCode = httpClient.POST(body);   //Send the actual POST request
        
        if(httpResponseCode>0){
        
            String response = httpClient.getString();                       //Get the response to the request
        
            Serial.println(httpResponseCode);   //Print return code
            Serial.println(response);           //Print request answer
        
        }else{
        
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        
        }
        
        httpClient.end();  //Free resources
 
 }else{
 
    Serial.println("Error in WiFi connection");   
 
 }
 
  delay(10000);  //Send a request every 10 seconds
}



void readHealth(){
  
  pox.update();
  double tempC = readTemperatureCelsius();
  double tempF = convertTemperatureToFahrenheit(tempC);
  
  
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

    float hearRate = pox.getHeartRate();
    uint8_t spO2Value = pox.getSpO2();
    float spO2 = spO2Value;

     Serial.print("Heart rate: ");
     Serial.print(String(hearRate) + " BPM \t");
     Serial.print("BPM / SpO2: ");
     Serial.print(String(spO2) + "% \n");

      Serial.print("Raw Value = " ); // shows pre-scaled value
      Serial.print(rawValue);
      Serial.print("\t milli volts = "); // shows the voltage measured
      Serial.print(voltage,0); //
      Serial.print("\t Temperature in C = ");
      Serial.print(tempC,1);
      Serial.print("\t Temperature in F = ");
      Serial.println(tempF,1);

      display.clear();
      displayHeartRate(hearRate);
      displaySPO2(spO2);
      displayTemperature(tempC,tempF); 
 
      int patientId = 1;
      String measurementType = "HEARTRATE";
      String measurementUnit = "BPM";
      double measurementValue = hearRate;
      //sendHttpRequest(patientId, measurementType, measurementUnit, measurementValue);
      tsLastReport = millis();
  }
}

double readTemperatureCelsius(){
  rawValue = analogRead(analogIn);
  voltage = (rawValue / 2048.0) * 3300; // 5000 to get millivots.
  return voltage * 0.1;
}

double convertTemperatureToFahrenheit(double tempC){
   return tempF = (tempC * 1.8) + 32; // conver to F
}

void displayHeartRate(float heartRate){
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 0,"BPM : ..." + String(heartRate));
   display.display();
}

void displaySPO2(float spO2){
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 16,"SP02 : ..." + String(spO2) + "%");
   display.display();
}

void displayTemperature(double tempC, double tempF){
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 32,"Temp : ..." + String(tempC) + " C");
  display.drawString(0, 48,"Temp : ..." + String(tempF) + " F");
  display.display();
}


void loop() {
    readHealth();
}
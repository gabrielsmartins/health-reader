#include "SSD1306.h" 
#include "MAX30100_PulseOximeter.h"
#include "WiFi.h"
#include "WiFiUdp.h"
#include "NTPClient.h"
#include "HTTPClient.h"

#define REPORTING_DISPLAY_PERIOD_MS 1000
#define REPORTING_API_PERIOD_MS 3000
#define MAX_MEASUREMENTS_VALUES 100
#define MAX_REPORT_API_VALUES 10



/************************
***** Display OLED *****
************************/
SSD1306  display(0x3c, 5, 4);
uint32_t tsLastReport = 0;

/**************************
***** MAX30100 Sensor *****
***************************/
PulseOximeter pox;

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

typedef struct {
    int patientId;
    String dateTime;
    String measurementType;
    double measurementValue;
    String measurementUnit;
} Measurement;

Measurement measurementData[MAX_MEASUREMENTS_VALUES] = {0,"0000-00-00 00:00:00","",0,""};

int measurementIndex = 0;

void setupOLEDDisplay();
void setupMAX30100Sensor();
void setupWifi();
void setupNTPClientTime();
void displayHeartRate(float heartRate);
void displaySPO2(float spO2);
void readHealth();
double readTemperatureCelsius();
double convertTemperatureToFahrenheit(double tempC);
void displayTemperature(double tempC, double tempF);
void sendHttpRequest(Measurement measurement);
Measurement buildHeartRateMeasurement(double heartRateValue);
Measurement buildSpo2Measurement(double spO2Value);
Measurement buildTemperatureMeasurement(double tempC);
int fillMeasurementData(Measurement measurement);
void sendHealthData();
String getFormattedDateTime();
bool isValidMeasurement(Measurement measurement);
bool isDifferentTypeFromPrevious(Measurement measurement);



void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  setupOLEDDisplay();
  setupWifi();
  setupMAX30100Sensor();
  setupNTPClientTime();
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

void setupNTPClientTime(){
 timeClient.begin();
 timeClient.setTimeOffset(3600);
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
    display.drawStringMaxWidth(0, 0, 128,"Trying to initialize MAX30100 sensor ..." );
    display.drawStringMaxWidth(0, 16, 128,"Failed to initialize MAX30100 sensor ..." );
    display.display();
  }

  Serial.println("SUCCESS");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 2, "Success ... ");
  display.display();  
}

Measurement buildHeartRateMeasurement(double heartRateValue){

    Measurement measurement;
    measurement.patientId = 1;
    measurement.dateTime = getFormattedDateTime();
    measurement.measurementType = "HEARTRATE";
    measurement.measurementValue = heartRateValue;
    measurement.measurementUnit = "BPM";
    
    return measurement;
}

Measurement buildSpo2Measurement(double spO2Value){

    Measurement measurement;
    measurement.patientId = 1;
    measurement.dateTime = getFormattedDateTime();
    measurement.measurementType = "OXYGEN SATURATION";
    measurement.measurementValue = spO2Value;
    measurement.measurementUnit = "%";
    
    return measurement;
}

Measurement buildTemperatureMeasurement(double tempC){
   
  
    Measurement measurement;
    measurement.patientId = 1;
    measurement.dateTime = getFormattedDateTime();
    measurement.measurementType = "TEMPERATURE";
    measurement.measurementValue = tempC;
    measurement.measurementUnit = "C";
    
    return measurement;
}

String getFormattedDateTime(){
    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }
    String formattedDateTime = timeClient.getFormattedDate();
    int splitT = formattedDateTime.indexOf("T");
    String formattedDate = formattedDateTime.substring(0,splitT);
    String timeStamp = formattedDateTime.substring(splitT+1, formattedDateTime.length()-1);
    String dateTime = formattedDate + " " + timeStamp;
    return dateTime;
}

void sendHttpRequest(Measurement measurement){

   if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status

        HTTPClient httpClient;
        
        httpClient.begin(endpoint + "/measurement");  //Specify destination for HTTP request
        httpClient.addHeader("Content-Type", "application/json");             //Specify content-type header
  
        String body = "{ \"patient_id\" : " + String(measurement.patientId) + ","
                           "\"datetime\" : \"" + String(measurement.dateTime) + "\","
                           "\"measurement_type\" :  \"" + measurement.measurementType + "\","
                           "\"measurement_value\" : " + String(measurement.measurementValue) + ","
                           "\"measurement_unit\" :  \"" + String(measurement.measurementUnit) + "\"" +
                      "}";

        Serial.println("JSON :" + body);

        int httpResponseCode = httpClient.POST(body);   //Send the actual POST request
        Serial.println("Http Response Code : " + String(httpResponseCode));

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

  float heartRate = pox.getHeartRate();
  uint8_t spO2Value = pox.getSpO2();
  float spO2 = spO2Value;
  double tempC = readTemperatureCelsius();
  double tempF = convertTemperatureToFahrenheit(tempC);

  if (millis() - tsLastReport > REPORTING_DISPLAY_PERIOD_MS) {
      display.clear();
      displayHeartRate(heartRate);
      displaySPO2(spO2);
      displayTemperature(tempC,tempF); 

      fillMeasurementData(buildHeartRateMeasurement(heartRate));
      fillMeasurementData(buildSpo2Measurement(spO2));
      fillMeasurementData(buildTemperatureMeasurement(tempC));
      tsLastReport = millis();
  }

    
  
}

int fillMeasurementData(Measurement measurement){
   Serial.println("Measurement Index :" + String(measurementIndex));
   if( isValidMeasurement(measurement) && isDifferentTypeFromPrevious(measurement)){
     measurementIndex = measurementData[measurementIndex].patientId == 0 ? measurementIndex : measurementIndex++;
     measurementIndex = measurementIndex >= MAX_MEASUREMENTS_VALUES ? 0 : measurementIndex;
     measurementData[measurementIndex] = measurement;
     measurementIndex = measurementIndex+1;
   }
   return measurementIndex;
}

bool isValidMeasurement(Measurement measurement){
    if(measurement.measurementType =="HEARTRATE" && measurement.measurementValue > 0.00)
       return true;
    
    if(measurement.measurementType =="OXYGEN SATURATION" && measurement.measurementValue > 0.00)
       return true;
    
    if(measurement.measurementType == "TEMPERATURE" &&  measurement.measurementValue > 30)
         return true;
           
    return false;
}

bool isDifferentTypeFromPrevious(Measurement measurement){
  int previousIndex = measurementIndex >= MAX_MEASUREMENTS_VALUES ? 0 : measurementIndex;
  if(measurementData[previousIndex].measurementType.equals(measurement.measurementType)){
    return false;
  }
  return true;
}


void sendHealthData(){
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);

   for(int i=0; i < MAX_REPORT_API_VALUES; i++){
      display.clear();
      display.drawString(0, 0, "Storing health data : ..." + String(i+1) + "/" + String(MAX_MEASUREMENTS_VALUES));
      display.display();
      Measurement measurement = measurementData[i];
      sendHttpRequest(measurement);
   }
}

double readTemperatureCelsius(){
  rawValue = analogRead(analogIn);
  voltage = (rawValue / 2048.0) * 3300; // 5000 to get millivots.
  return voltage * 0.1;
}

double convertTemperatureToFahrenheit(double tempC){
   double tempF = (tempC * 1.8) + 32; // convert to F
   return tempF;
}

void displayHeartRate(float heartRate){
   Serial.print("\tHeart rate: ");
   Serial.print(String(heartRate) + " BPM \t");
   
   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 0,"BPM : ..." + String(heartRate));
   display.display();
}

void displaySPO2(float spO2){
   Serial.print("BPM / SpO2: ");
   Serial.print(String(spO2) + "% \n");

   display.setFont(ArialMT_Plain_10);
   display.setTextAlignment(TEXT_ALIGN_LEFT);
   display.drawString(0, 16,"SP02 : ..." + String(spO2) + "%");
   display.display();
}

void displayTemperature(double tempC, double tempF){

  Serial.print("\t Temperature in C = ");
  Serial.print(tempC,1);
  Serial.print("\t Temperature in F = ");
  Serial.print(tempF,1);



  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 32,"Temp : ..." + String(tempC) + " C");
  display.drawString(0, 48,"Temp : ..." + String(tempF) + " F");
  display.display();
}


void loop() {
  readHealth();
  if(measurementIndex == MAX_MEASUREMENTS_VALUES){
    sendHealthData();
  }
}
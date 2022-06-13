#include <Arduino.h>
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h" 
#include <ESP8266WiFi.h>       // WiFi library
 
// Define settings
const char ssid[]     = ""; // WiFi SSID
const char pass[]     = ""; // WiFi password
const char domain[]   = "xxx.tmep.cz";  // domain.tmep.cz
const char guid[]     = "mereni"; // tmep variable
const byte sleep      = 1; // How often send data to the server. In minutes
 
SCD30 airSensor;
const u_int16_t measurementInterval = 30;

void setParameters() {

 //Change number of seconds between measurements: 2 to 1800 (30 minutes), stored in non-volatile memory of SCD30
  airSensor.setMeasurementInterval(measurementInterval);

  //While the setting is recorded, it is not immediately available to be read.
  delay(200);

  int interval = airSensor.getMeasurementInterval();
  Serial.print("Measurement Interval: ");
  Serial.println(interval);

  Serial.print("Auto calibration set to ");
  if (airSensor.getAutoSelfCalibration() == true)
      Serial.println("true");
  else
      Serial.println("false");
        
          
  uint16_t settingVal; // The settings will be returned in settingVal
    
  if (airSensor.getForcedRecalibration(&settingVal) == true) // Get the setting
  {
    Serial.print("Forced recalibration factor (ppm) is ");
    Serial.println(settingVal);
  }
  else
  {
    Serial.print("getForcedRecalibration failed! Freezing...");
    while (1)
      ; // Do nothing more
  }

  if (airSensor.getMeasurementInterval(&settingVal) == true) // Get the setting
  {
    Serial.print("Measurement interval (s) is ");
    Serial.println(settingVal);
  }
  else
  {
    Serial.print("getMeasurementInterval failed! Freezing...");
    while (1)
      ; // Do nothing more
  }

  if (airSensor.getFirmwareVersion(&settingVal) == true) // Get the setting
  {
    Serial.print("Firmware version is 0x");
    Serial.println(settingVal, HEX);
  }
  else
  {
    Serial.print("getFirmwareVersion! Freezing...");
    while (1)
      ; // Do nothing more
  }
}

void sendData(uint16_t co2Value, uint16_t teplota, uint16_t vlhkost) {
  WiFiClient client;

  char host[50];            // Joining two chars is little bit difficult. Make new array, 50 bytes long
  strcpy(host, domain);     // Copy /domain/ in to the /host/
  strcat(host, ".tmep.cz"); // Add ".tmep.cz" at the end of the /host/. /host/ is now "/domain/.tmep.cz"

  Serial.print(F("Connecting to ")); Serial.println(host);
  if (!client.connect(host, 80)) {
    Serial.println(F("Connection failed"));
    delay(1000);
    return;
  }
  Serial.println(F("Client connected"));

  // Make an url. We need: /?guid=t
  String url = "/?";
        url += guid;
        url += "=";
        url += teplota;
        url += "&humV=";
        url += vlhkost;
        url += "&CO2=";
        url += co2Value;

  Serial.print(F("Requesting URL: ")); Serial.println(url);

  // Make a HTTP GETrequest.
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" + 
              "Connection: close\r\n\r\n");

  // Workaroud for timeout
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(F(">>> Client Timeout !"));
      client.stop();
      delay(1000);
      return;
    }
  }
}

void setup() {
 
  // Start serial
  Serial.begin(115200);
  delay(1000);
  Serial.println();

  Wire.begin();

  if (airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }

  setParameters();
 
  // Connect to the WiFi
  Serial.print(F("Connecting to ")); Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: ")); Serial.println(WiFi.localIP());
  Serial.println();

  delay(5000);
}
 
void loop() {
  if (airSensor.dataAvailable())
  {
    Serial.println();

    Serial.print("co2(ppm):");
    uint16_t co2Value = airSensor.getCO2();
    Serial.print(co2Value);

    Serial.print(" temp(C):");
    uint16_t teplota = airSensor.getTemperature();
    Serial.print(teplota, 1);

    Serial.print(" humidity(%):");
    uint16_t vlhkost = airSensor.getHumidity();
    Serial.print(vlhkost, 1);

    Serial.print(" ");

    sendData(co2Value, teplota, vlhkost);
  }
  
  delay(sleep*60000);
}
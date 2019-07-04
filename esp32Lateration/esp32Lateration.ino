#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <BLEUtils.h>
#include <BLEDevice.h>
#include <BLEBeacon.h>
#include "time.h"
#include <ctime>
#include <sstream>

/* --- GLOBAL VARIABLES --- */

Preferences preferences;
const int ARDUINO_N = 1;
const int BUTTON_KEY = 0;
const int LED_BUILTIN = 2;

/* --- WIFI CREDENTIALS VARAIBLES --- */

String recv_ssid;
String recv_pass;

IPAddress serverIP; int serverPort;

/* --- TIME VARIABLES --- */

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

/* --- CONNECTION VARIABLES --- */

WiFiClient wifiClient;
PubSubClient client(wifiClient);

const std::string mqttUsers[] = {"tk3_esp1", "tk3_esp2", "tk3_esp3", "tk3_esp4"};
const std::string mqttPswrd[] = {"esp1mqtt", "esp2mqtt", "esp3mqtt", "esp4mqtt"};

bool wifiConnected = false;
bool foundMQTTBroker = false;
bool mqttConnected = false;

/* --- BLE VARIABLES --- */

const std::string uuids[] = {"178a09a8-200f-422c-ac98-ddea53704f19", 
                             "c0db5460-16e1-4754-b56a-63cc5c042f47",
                             "bc8131f0-b76b-4362-b4f5-a95bc6b4fa71",
                             "df3a4416-8737-424c-b795-5066aacbff94"};

const std::string positions[4][2] = {{"0", "0"}, {"0", "1"}, {"1", "0"}, {"1", "1"}};

BLEAdvertising *bleAdv;

/* --- AUXILIARY METHODS --- */

void ledOn(){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOn(int nTime){
  pinMode(LED_BUILTIN, OUTPUT);
  int millis = nTime * 1000;
  digitalWrite(LED_BUILTIN, HIGH); delay(millis); digitalWrite(LED_BUILTIN, LOW);
}

void ledOff(){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void ledBlink(){
  pinMode(LED_BUILTIN, OUTPUT);
  if(digitalRead(LED_BUILTIN) == HIGH)
    digitalWrite(LED_BUILTIN, LOW);
  else
    digitalWrite(LED_BUILTIN, HIGH);
}

void ledBlink(int nTimes, int nTimeAppart){
  pinMode(LED_BUILTIN, OUTPUT);
  int millis = nTimeAppart * 1000;
  for(int i = 0; i <= nTimes - 1; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(millis); digitalWrite(LED_BUILTIN, LOW); delay(millis);
  }
}

time_t updateTime(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  return mktime(&timeinfo);
}

void sendMessage(std::string message, std::string topic){
  client.publish(topic.c_str(), message.c_str());
  Serial.print("Message sent ["); Serial.print(topic.c_str()); Serial.print("] "); 
  Serial.println(message.c_str());
}

/* --- SETUP & SETUP_AUX METHODS --- */

bool receiveCredentials(){
  Serial.println();
  Serial.println("Waiting for WiFi Credentials");
  /*SSID input*/
  Serial.print("SSID: ");

  int count = 0;
  while(Serial.available() == 0) {
    delay(1000);
    if(count == 10)
      return false;
    
    count++;
  }

  recv_ssid = Serial.readString();
  if(recv_ssid[recv_ssid.length() - 1] == '\n')
    recv_ssid[recv_ssid.length() - 1] = '\0';
  Serial.println(recv_ssid);

  /*Password input*/
  Serial.print("Password: ");
  while (Serial.available() == 0);
  recv_pass = Serial.readString();
  if(recv_pass[recv_pass.length() - 1] == '\n')
    recv_pass[recv_pass.length() - 1] = '\0';

  return true;
}

void storeCredentials(){
  preferences.clear();
  preferences.putString("ssid", recv_ssid);
  preferences.putString("pass", recv_pass);
  Serial.println("Credentials successfully stored in memory");
}

String getStoredSSID(){
  String ssid = preferences.getString("ssid");
  Serial.println("Stored SSID: " + ssid);
  return ssid;
}

String getStoredPassword(){
  String pass = preferences.getString("pass");
  Serial.println("Stored Password: " + pass);
  return pass;
}

bool initWiFiConn(String ssid, String pass){
  /*Connecting to WiFi*/
  WiFi.begin(ssid.c_str(), pass.c_str());

  /*A delay is needed so we only continue with the program until the ESP32 is effectively connected to the WiFi network*/
  int count = 0;
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    if(count == 20) {
      Serial.println();
      break;
    }
    count++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected.");     
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    ledBlink(2, 0.5);
    wifiConnected = true; return true;
  }
  else {
    Serial.println("Connection Attempt Failed");
    ledBlink(2, 1);
    wifiConnected = false; return false;
  }
}

bool browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("No services found");
        foundMQTTBroker = false; return false;
    }
    else {
        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i)); serverIP = MDNS.IP(i);
            Serial.print(":");
            Serial.print(MDNS.port(i)); serverPort = MDNS.port(i);
            Serial.println(")");
        }
        foundMQTTBroker = true; return true;
    }
}

bool mqttConnect(){
  String id = "ESP32_K"; id += ARDUINO_N; 
  if(client.connect(id.c_str(), mqttUsers[ARDUINO_N-1].c_str(), mqttPswrd[ARDUINO_N-1].c_str())) {
    Serial.println("Client " + id + " connected to server");
    Serial.println();
    mqttConnected = true; return true;
  }
  else {
    Serial.println("Unable to connect to server");
    mqttConnected = false; return false;
  }
}

void enableBeacon(){
  BLEBeacon beacon = BLEBeacon();
  beacon.setManufacturerId(0x4C00); 
  beacon.setProximityUUID(BLEUUID(uuids[ARDUINO_N-1]));
  beacon.setMajor(0x4B02);              // 19202 0x‭4B02‬
  beacon.setMinor(0x0451);              // 1105  0x0451
  beacon.setSignalPower(-57);

  BLEAdvertisementData advData = BLEAdvertisementData();
  BLEAdvertisementData scanRespData = BLEAdvertisementData();

  advData.setFlags(0x04);

  std::string serviceData = "";
  serviceData += (char)26;              // Len
  serviceData += (char)0xFF;            // Type
  serviceData += beacon.getData(); 
  advData.addData(serviceData);

  bleAdv->setAdvertisementData(advData);
  bleAdv->setScanResponseData(scanRespData);
}

void setup(){
  Serial.begin(115200);
  preferences.begin("iotk", false);
  
  delay(2000);
  if(receiveCredentials() == true) {
    storeCredentials();
    ESP.restart();
  }
  else {
    Serial.println("No WiFi Credentials were received");
    Serial.println("Using WiFi Credentials stored in memory");
  }

  String ssid = getStoredSSID();
  String pass = getStoredPassword();

  for(int i = 0; !initWiFiConn(ssid, pass); i++){ if(i == 1) break; }
  if(wifiConnected){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println(updateTime());
    if(!MDNS.begin("ESP")){
      Serial.println("Error setting up mDNS responder");
    }
    else {
      Serial.println("Finished setup of mDNS");
      for(int i = 0; !browseService("mqtt", "tcp"); i++){ if(i == 2) break; }
      if(foundMQTTBroker){
        client.setServer(serverIP, serverPort);
        for(int i = 0; !mqttConnect(); i++){ if(i == 2) break; }
        if(!mqttConnected)
          WiFi.disconnect();
      }
      else
        WiFi.disconnect();
    }
  }

  BLEDevice::init("");
  // BLEDevice::setPower(ESP_PWR_LVL_N14);
  bleAdv = BLEDevice::getAdvertising();
  enableBeacon();

  /* Close the Preferences */
  preferences.end();
}

/* --- LOOP VARIABLES --- */

std::string mqttTopic = "/laterator/beacons/" + uuids[ARDUINO_N-1];

/* --- LOOP & LOOP_AUX METHODS --- */

void loop(){
  if(mqttConnected) {
    client.loop();

    bleAdv->start();
    delay(500);
    bleAdv->stop();

    std::string topic = mqttTopic + "/name";
    sendMessage(mqttUsers[ARDUINO_N-1], topic);

    topic = mqttTopic + "/lastActivity";
    std::time_t unixTimestamp = std::time(nullptr);
    std::stringstream ss; ss << unixTimestamp;
    sendMessage(ss.str(), topic);

    topic = mqttTopic + "/x";
    sendMessage(positions[ARDUINO_N-1][0], topic);

    topic = mqttTopic + "/y";
    sendMessage(positions[ARDUINO_N-1][1], topic);
  }
}
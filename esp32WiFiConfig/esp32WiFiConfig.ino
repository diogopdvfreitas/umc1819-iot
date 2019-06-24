#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <PubSubClient.h>

Preferences preferences;
const int BUTTON_KEY = 0;
const int LED_BUILTIN = 2;
String recv_ssid;
String recv_pass;

IPAddress myIP;
IPAddress serverIP; int serverPort;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

String mqttUsers[] = {"tk3_esp1", "tk3_esp2", "tk3_esp3", "tk3_esp4"};
String mqttPswrd[] = {"esp1mqtt", "esp2mqtt", "esp3mqtt", "esp4mqtt"};
const String mqttTopic = "mensapoll";

bool mqttConnected = false;

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

  bool foundMQTTBroker = false;
  if(initWiFiConn(ssid, pass)) {
    if(!MDNS.begin("ESP"))
      Serial.println("Error setting up mDNS responder");
    else {
      Serial.println("Finished setup of mDNS");
      foundMQTTBroker = browseService("mqtt", "tcp");
    }
  }

  if(foundMQTTBroker) {
    client.setServer(serverIP, serverPort);
    client.setCallback(callback);
    if(mqttConnect()) {
      mqttConnected = true;
      client.subscribe(mqttTopic.c_str());
      Serial.print("Client subscribed to topic "); Serial.print(mqttTopic);
      Serial.println(" - Awaiting messages...");
      Serial.println("Press button > 3s to initiate poll");
      pinMode(BUTTON_KEY, INPUT);
    }
  }
  
  /* Close the Preferences */
  preferences.end();
}

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
    delay(1000);
    Serial.print(".");

    if(count == 20) {
      Serial.println();
      break;
    }
    count++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected.");     
    Serial.print("IP Address: ");
    myIP = WiFi.localIP();
    Serial.println(myIP);
    ledOn(10);
    return true;
  }
  else {
    Serial.println("Connection Attempt Failed");
    ledBlink(10, 1);
    return false;
  }
}

bool browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("No services found");
        return false;
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
        return true;
    }
}

bool mqttConnect(){
  int i = 3;
  String id = "ESP32_K"; id += i; 
  if(client.connect(id.c_str(), mqttUsers[i-1].c_str(), mqttPswrd[i-1].c_str())) {
    Serial.println("Client " + id + " connected to server");
    Serial.println();
    return true;
  }
  else {
    Serial.println("Unable to connect to server");
    return false;
  }
}

bool messageReceived = false;
String payloadMsgRecv = "";

const int LONG_PRESS = 3000;
const int NO_COUNT_INPUT = 2000;

int currButtonState = 0;          // Button's current state
int lastButtonState = 1;          // Button's state in the last loop
int timeStartPressed = 0;         // Time the button was pressed
int timeEndPressed = 0;           // Time the button was released
int timeHold = 0;                 // How long the button was held
int timeReleased = 0;             // How long the button released

bool initiator = false;           // Activates when the client is the one o initiates a poll
bool pollActive = false;          // Activated when the client first realizes a poll is active
bool initPoll = false;            // Activated when the client initiates the configuration of a poll
bool countPresses = false;        // Activated when the client is deciding the minimum number of positives for poll success
bool blinkEndInitPoll = false;    // Activated when the poll configuration finishes and the client must now wait for a decision
bool decided = false;             // Activated when the client either accepts or denies the poll
bool decisionMade = false;        // Activated when the poll has been completed and a decision made

int nPollAccept = 0;              // Minimum number of positives for poll success
int nAccept = 0;                  // Number of clients who accepted the poll
int timeDecision = 0;             // Time the poll decision was made 

void loop(){
  if(mqttConnected) {
    client.loop();

    if(initiator && decisionMade) {
      int decisionElapsedTime = millis() - timeDecision;
      if(decisionElapsedTime >= 300000) {
        sendMessage("CLEAR");
        clearState();
      }
    }

    if(messageReceived){
      char *token;
      char msg_a[payloadMsgRecv.length()]; payloadMsgRecv.toCharArray(msg_a, payloadMsgRecv.length());
      String split[2];
      token = strtok(msg_a, " ");
      for(int i = 0; token != NULL; i++) {
        split[i] = token;
        token = strtok(NULL, " ");
      }
      
      String command(split[0]);
      if(command == "POLL" && !pollActive){
        Serial.println("Poll was initiated by another client");
        Serial.println("To accept poll press button, to deny long press");
        pollActive = true;
        nPollAccept = atoi(split[1].c_str());
        ledBlink(1);
      }
      if(command == "ACCEPT"){
        nAccept++;
        if(nAccept == nPollAccept) {
          Serial.println("Decision made - Poll Accepted");
          ledBlink(0.5);
          decisionMade = true; pollActive = false;
          timeDecision = millis();
        }
      }
      if(command == "CLEAR")
        clearState();

      messageReceived = false;
      payloadMsgRecv = "";
    }

    currButtonState = digitalRead(BUTTON_KEY);
    if(currButtonState != lastButtonState) {
      updateButtonState();
      if(initPoll) {
        initPoll = false;
        countPresses = true;
        Serial.println("Press the number of positives needed for poll success");
      }
      if(countPresses && currButtonState == LOW){
        nPollAccept++;
        Serial.print("N Times Pressed: "); Serial.println(nPollAccept);
      }
      if(pollActive && !decided && currButtonState == LOW){
        sendMessage("ACCEPT");
        Serial.println("Voted yes on poll - Awaiting decision");
        decided = true;
        ledOn();
      }
    }
    else {
      updateButtonCounter();
      if(timeHold >= LONG_PRESS && !pollActive && !initPoll && !countPresses) {
        initPoll = true;
        Serial.println("Initiating Poll Configuration");
      }
      if(timeReleased >= NO_COUNT_INPUT && countPresses) {
        //if(nPollAccept != 0) {}
        countPresses = false;
        Serial.print("Total Times Pressed: "); Serial.println(nPollAccept);
        ledBlink(nPollAccept, 1);
        Serial.println("Poll Configuration Finished");

        String message = "POLL "; message += nPollAccept;
        sendMessage(message);
        Serial.println("Poll Initiated - Awaiting decision");
        initiator = true; pollActive = true; decided = true;
        blinkEndInitPoll = true;
      }
      if(blinkEndInitPoll)
        ledBlink(1);
      if(timeHold >= LONG_PRESS && pollActive && !decided) {
        Serial.println("Voted no on poll - Awaiting decision");
        ledOff();
        decided = true;
      }
      if(timeHold >= LONG_PRESS && pollActive && initiator) {
        Serial.println("Cancelled Poll");
        sendMessage("CLEAR");
        clearState();
      }
    }

    lastButtonState = currButtonState;
  }
}

void updateButtonState(){
  /*  LOW == Button Pressed 
      HIGH == Button Released */
  if(currButtonState == LOW) {
    timeStartPressed = millis();                        // Registers the time the button was pressed
    timeReleased = timeStartPressed - timeEndPressed;   // Registers how long the button was released
  }
  else {
    timeEndPressed = millis();                          // Registers the time the button was released
    timeHold = timeEndPressed - timeStartPressed;       // Registers how long the button was held down
  }
}

void updateButtonCounter(){
  if(currButtonState == LOW)
    timeHold = millis() - timeStartPressed;             // Registers for how long the button has been pressed currently
  else
    timeReleased = millis() - timeEndPressed;           // Registers for how long the button has been released currently
}

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

void ledBlink(int nTimeAppart){
  pinMode(LED_BUILTIN, OUTPUT);
  int millis = nTimeAppart * 1000;
  digitalWrite(LED_BUILTIN, HIGH); delay(millis); digitalWrite(LED_BUILTIN, LOW); delay(millis);
}

void ledBlink(int nTimes, int nTimeAppart){
  pinMode(LED_BUILTIN, OUTPUT);
  int millis = nTimeAppart * 1000;
  for(int i = 0; i <= nTimes - 1; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(millis); digitalWrite(LED_BUILTIN, LOW); delay(millis);
  }
}

void sendMessage(String message){
  client.publish(mqttTopic.c_str(), message.c_str());
  Serial.print("Message sent ["); Serial.print(mqttTopic); Serial.print("] "); 
  Serial.println(message);
}

void clearState(){
  bool initiator = false;          
  bool pollActive = false;         
  bool initPoll = false;           
  bool countPresses = false;       
  bool blinkEndInitPoll = false;   
  bool decided = false;            
  bool decisionMade = false;       

  int nPollAccept = 0;             
  int nAccept = 0;                 
  int timeDecision = 0;

  ledOff();

  Serial.println();
  Serial.println("Press button > 3s to initiate poll");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for(int i = 0; i < length; i++) {
    char c = (char)payload[i];
    payloadMsgRecv += c;
    Serial.print(c);
  }
  Serial.println();
  messageReceived = true;
}
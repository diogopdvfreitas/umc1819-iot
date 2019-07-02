#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <PubSubClient.h>

/* --- GLOBAL VARIABLES --- */

Preferences preferences;
const int ARDUINO_N = 3;
const int BUTTON_KEY = 0;
const int LED_BUILTIN = 2;

/* --- WIFI CREDENTIALS VARAIBLES --- */

String recv_ssid;
String recv_pass;

IPAddress myIP;
IPAddress serverIP; int serverPort;

/* --- CONNECTION VARIABLES --- */

WiFiClient wifiClient;
PubSubClient client(wifiClient);

const String mqttUsers[] = {"tk3_esp1", "tk3_esp2", "tk3_esp3", "tk3_esp4"};
const String mqttPswrd[] = {"esp1mqtt", "esp2mqtt", "esp3mqtt", "esp4mqtt"};
const String mqttTopic = "mensapoll";

bool wifiConnected = false;
bool foundMQTTBroker = false;
bool mqttConnected = false;

/* --- POLL VARIABLES --- */

enum State{
  NOPOLL, INITPOLL, AWAITVOTES, VOTE, VOTED, DECIDED
};

bool messageReceived = false;
String payloadMsgRecv = "";

const int LONG_PRESS = 3000;
const int NO_COUNT_INPUT = 2000;

int currButtonState = 0;              // Button's current state
int lastButtonState = 1;              // Button's state in the last loop
int timeStartPressed = 0;             // Time the button was pressed
int timeEndPressed = 0;               // Time the button was released
int timeHold = 0;                     // How long the button was held
int timeReleased = 0;                 // How long the button released
int timeLedChange = 0;

bool flagLEDBlink0 = false;
bool flagLEDBlink1 = false;
bool flagLEDBlink2 = false;

int nPollAccept = 0;
int votes = 0;
int decisionTime = 0;

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
  timeLedChange = millis();
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

/* --- POLL CLASSES --- */

class PollState{
  public:
    PollState(){}
    virtual void shortPress();
    virtual void longPress();
    virtual void finish();
    virtual State getValue() = 0;
};

PollState* state;
bool initiator = false;

void changeState(State stt);

class State_NoPoll : public PollState{
  public:
    State_NoPoll(){
      ledOff();
      initiator = false;
      nPollAccept = 0; votes = 0; decisionTime = 0;
      flagLEDBlink0 = false; flagLEDBlink1 = false; flagLEDBlink2 = false;
      Serial.println("Press button > 3s to initiate poll");
    }
    void shortPress(){}
    void longPress(){
      changeState(INITPOLL);
    }
    void finish(){}
    State getValue(){ return NOPOLL; }
};

class State_InitPoll : public PollState{
  private:
    int presses = 0;
  public:
    State_InitPoll(){
      initiator = true;
      Serial.println("Initiating Poll Configuration");
      Serial.println("Press the number of positives needed for poll success");
    }
    void shortPress(){
      presses++;
      Serial.print("N Times Pressed: "); Serial.println(presses);
    }
    void longPress(){}
    void finish(){
      Serial.print("Total Times Pressed: "); Serial.println(presses);
      nPollAccept = presses;
      ledBlink(presses, 1);
      Serial.println("Poll Configuration Finished");
      changeState(AWAITVOTES);
    }
    State getValue(){ return INITPOLL; }
};

class State_AwaitVotes : public PollState{
  public:
    State_AwaitVotes(){
      String message = "POLL "; message += nPollAccept;
      sendMessage(message);
      Serial.println("Poll Initiated - Awaiting decision");
      ledBlink(); flagLEDBlink2 = true;
    }
    void shortPress(){}
    void longPress(){
      Serial.println("Cancelling Poll");
      sendMessage("CLEAR");
    }
    void finish(){}
    State getValue(){ return AWAITVOTES; }
};

class State_Vote : public PollState{
  public:
    State_Vote(){
      Serial.println("Poll was initiated by another client");
      Serial.println("To accept poll press button, to deny long press");
      ledBlink(); flagLEDBlink1 = true;
    }
    void shortPress(){
      sendMessage("ACCEPT");
      Serial.println("Voted yes on poll - Awaiting decision");
      ledOn(); flagLEDBlink1 = false;
      changeState(VOTED);
    }
    void longPress(){
      Serial.println("Voted no on poll - Awaiting decision");
      ledOff(); flagLEDBlink1 = false;
      changeState(VOTED);
    }
    void finish(){}
    State getValue(){ return VOTE; }
};

class State_Voted : public PollState{
  public:
    State_Voted(){}
    void shortPress(){}
    void longPress(){}
    void finish(){}
    State getValue(){ return VOTED; }
};

class State_Decided : public PollState{
  public:
    State_Decided(){
      Serial.println("Decision made - Poll Accepted");
      decisionTime = millis();
      ledBlink(); flagLEDBlink0 = true;
    }
    void shortPress(){}
    void longPress(){
      if(initiator){
        Serial.println("Cancelling Poll");
        sendMessage("CLEAR");
      }
    }
    void finish(){}
    State getValue(){ return DECIDED; }
};

/* --- SETUP & SETUP_AUX METHODS --- */

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
    if(!MDNS.begin("ESP")){
      Serial.println("Error setting up mDNS responder");
    }
    else {
      Serial.println("Finished setup of mDNS");
      for(int i = 0; !browseService("mqtt", "tcp"); i++){ if(i == 2) break; }
      if(foundMQTTBroker){
        client.setServer(serverIP, serverPort);
        client.setCallback(callback);
        for(int i = 0; !mqttConnect(); i++){ if(i == 2) break; }
        if(mqttConnected){
          client.subscribe(mqttTopic.c_str());
          Serial.print("Client subscribed to topic "); Serial.print(mqttTopic);
          Serial.println(" - Awaiting messages...");
          pinMode(BUTTON_KEY, INPUT);
          changeState(NOPOLL);
        }
        else
          WiFi.disconnect();
      }
      else
        WiFi.disconnect();
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
    Serial.println();
    Serial.println("WiFi Connected.");     
    Serial.print("IP Address: ");
    myIP = WiFi.localIP();
    Serial.println(myIP);
    ledOn(10);
    wifiConnected = true; return true;
  }
  else {
    Serial.println("Connection Attempt Failed");
    ledBlink(10, 1);
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

/* --- LOOP & LOOP_AUX METHODS --- */

void loop(){
  if(mqttConnected) {
    client.loop();

    currButtonState = digitalRead(BUTTON_KEY);
    if(currButtonState != lastButtonState) {
      updateButtonState();
      if(currButtonState == HIGH && timeHold <= LONG_PRESS){
        state->shortPress();
      }
      if(currButtonState == HIGH && timeHold >= LONG_PRESS){
        state->longPress();
      }
    }
    else {
      updateButtonCounter();
      if(state->getValue() == INITPOLL && timeReleased >= NO_COUNT_INPUT){
        state->finish();
      }
    }

    int ledChangeElapsedTime = millis() - timeLedChange;
    if( (flagLEDBlink0 && ledChangeElapsedTime == 500) ||
        (flagLEDBlink1 && ledChangeElapsedTime == 1000) ||
        (flagLEDBlink2 && ledChangeElapsedTime == 2000) ){
      ledBlink();
    }


    if(messageReceived){
      if(payloadMsgRecv.substring(0,4) == "POLL" && state->getValue() == NOPOLL){
        nPollAccept = payloadMsgRecv.substring(5).toInt();
        changeState(VOTE);
      }
      if(payloadMsgRecv == "ACCEPT"){
        incrementVotes();
      }
      if(payloadMsgRecv == "CLEAR"){
        Serial.println("Poll Cancelled by the host");
        changeState(NOPOLL);
      }

      messageReceived = false;
      payloadMsgRecv = "";
    }

    if(initiator && state->getValue() == DECIDED){
      int decisionElapsedTime = millis() - decisionTime;
      if(decisionElapsedTime >= 300000) {
        sendMessage("CLEAR");
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

void changeState(State stt){
  switch(stt){
    case NOPOLL:
      state = new State_NoPoll();
      break;
    case INITPOLL:
      state = new State_InitPoll();
      break;
    case AWAITVOTES:
      state = new State_AwaitVotes();
      break;
    case VOTE:
      state = new State_Vote();
      break;
    case VOTED:
      state = new State_Voted();
      break;
    case DECIDED:
      state = new State_Decided();
      break;
   }
}

void incrementVotes(){
  votes++;
  Serial.print("Current Number of votes: "); Serial.println(votes);
  if(votes == nPollAccept) {
    changeState(DECIDED);
  }
}
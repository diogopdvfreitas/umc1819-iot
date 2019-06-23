#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;

int LED_BUILTIN = 2;
String recv_ssid;
String recv_pass;

void setup(){
  Serial.begin(115200);
  preferences.begin("iotk", false);
  
  delay(2000);
  if(receiveCredentials() == true){
    storeCredentials();
    ESP.restart();
  }
  else{
    Serial.println("No WiFi Credentials were received");
    Serial.println("Using WiFi Credentials stored in memory");
  }

  String ssid = getStoredSSID();
  String pass = getStoredPassword();

  char ssid_a[ssid.length()];
  convertString(ssid, ssid_a);
  char pass_a[pass.length()];
  convertString(pass, pass_a);

  /*Connecting to WiFi*/
  WiFi.begin(ssid_a, pass_a);

  /*A delay is needed so we only continue with the program untill the ESP32 is effectively connected to the WiFi network*/
  int count = 0;
  
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");

    if(count == 20){
      Serial.println();
      break;
    }
    count++;
  }
  
  if (WiFi.status() == WL_CONNECTED){	
  	Serial.println("WiFi Connected.");     
 	  Serial.print("IP Address: ");
	  Serial.println(WiFi.localIP());

    onLed_10s();
  }
  else {
    Serial.println("Connection Attempt Failed");
  	blinkLed_10s();
  }

  /* Close the Preferences */
  preferences.end();
}

bool receiveCredentials(){
  Serial.println("Waiting for WiFi Credentials");
  /*SSID input*/
  Serial.print("SSID: ");

  int count = 0;
  while (Serial.available() == 0){
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

void convertString(String str, char* res){
  int len = str.length();
  //for(int i = 0; i < len - 1; i++){
  for(int i = 0; i < len; i++){
    res[i] = str[i];
  }
}

void onLed_10s(){
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH); delay(10000); digitalWrite(LED_BUILTIN, LOW);
}

void blinkLed_10s(){
	pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i <= 9; i++){
    digitalWrite(LED_BUILTIN, HIGH); delay(500); digitalWrite(LED_BUILTIN, LOW); delay(500);
  }
}

void loop(){

}

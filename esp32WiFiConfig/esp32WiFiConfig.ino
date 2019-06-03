#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;

void setup(){
  Serial.begin(115200);
  Serial.println();
    
  String ssid;
  String pass;

  //preferences.clear();

  /* Start a namespace "iotk" in Read-Write mode: set second parameter to false 
  Note: Namespace name is limited to 15 chars */
  preferences.begin("iotk", false);

  if (preferences.getString("SSID") == null){
    /* SSID and Password inputs */
    Serial.print("SSID: ");
    while (Serial.available() == 0);
    ssid = Serial.readString();
    Serial.println(ssid);

    Serial.print("Password: ");
    while (Serial.available() == 0);
    pass = Serial.readString();
    Serial.println(pass);

    /* Store credentials to the Preferences */
    preferences.putString("SSID", ssid);
    preferences.putString("Pass", pass);
  }

  ssid = preferences.getString("SSID");
  pass = preferences.getString("Pass")

  /* Converting a string into a char array
  Length (it already including the null terminator) */
  int ssid_len = ssid.length();
  int pass_len = pass.length();

  /* Prepare the character array */
  char ssid_array[ssid_len];
  char pass_array[pass_len];

  for(int i = 0; i < ssid_len-1; i++)
    ssid_array[i] = ssid[i];
      
  ssid_array[ssid_len-1] = '\0';

  for(int i = 0; i < pass_len-1; i++)
    pass_array[i] = pass[i];
      
  pass_array[ssid_len-1] = '\0';

  /* Connecting to WiFi */
  WiFi.begin(ssid_array, pass_array);

  /* A delay is needed so we only continue with the program until 
  the ESP32 is effectively connected to the WiFi network */
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }

  if (WiFi.status() == WL_CONNECTED){
  	onLed_10s();
  	Serial.println("WiFi Connected.");

 	Serial.print("IP Address: ");
	Serial.println(WiFi.localIP());
  }
  else {
  	blinkLed_10s();
  }

  /* Close the Preferences */
  preferences.end();
}

void onLed_10s(){
	pinMode(LED1, OUTPUT);
	digitalWrite(LED, HIGH); delay(10000); digitalWrite(LED, LOW);
}

void blinkLed_10s(){
	pinMode(LED1, OUTPUT);
  for(int i = 0; i == 9; i++)
	   digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
}

void loop(){}
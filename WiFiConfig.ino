#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;

void setup() 
{
  Serial.begin(115200);
  Serial.println();
    
  String ssid;
  String password;

  preferences.clear();

  /* Start a namespace "iotk"
  in Read-Write mode: set second parameter to false 
  Note: Namespace name is limited to 15 chars */
  preferences.begin("iotk", false);

  /*SSID and Password inputs*/
  Serial.print("SSID: ");
  while (Serial.available() == 0);
  ssid = Serial.readString();
  Serial.println(ssid);

  Serial.print("Password: ");
  while (Serial.available() == 0);
  password = Serial.readString();
  Serial.println(password);

  /* Store credentials to the Preferences */
  preferences.putString("SSID", ssid);
  preferences.putString("Password", password);

  /*Converting a string into a char array*/
  // Length (it already including the null terminator)
  int ssid_len = ssid.length();
  int password_len = password.length();

  // Prepare the character array
  char ssid_array[ssid_len];
  char password_array[password_len];

  for(int i = 0; i < ssid_len-1; i++)
    ssid_array[i] = ssid[i];
      
  ssid_array[ssid_len-1] = '\0';

  for(int i = 0; i < password_len-1; i++)
    password_array[i] = password[i];
      
  password_array[ssid_len-1] = '\0';

  /*Connecting to WiFi*/
  WiFi.begin(ssid_array, password_array);

  /*A delay is needed so we only continue with the program untill the ESP32 is effectively connected to the WiFi network*/
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi...");
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
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
	digitalWrite(LED, HIGH); delay(500); digitalWrite(LED, LOW); delay(500);
}

void loop() {

}

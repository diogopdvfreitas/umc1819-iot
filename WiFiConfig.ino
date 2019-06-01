#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;

void setup() 
{
  Serial.begin(115200);
  Serial.println();

  String ssid;
  String password;

  /* Start a namespace "iotk"
  in Read-Write mode: set second parameter to false 
  Note: Namespace name is limited to 15 chars */
  preferences.begin("iotk", false);

  Serial.println("SSID: ");
  while (Serial.available() == 0);
  
  // read the incoming:
  ssid = Serial.readString();
  Serial.println(ssid);

  Serial.println("Password: ");
  while (Serial.available() == 0);
  
  // read the incoming:
  password = Serial.readString();
  Serial.println(password);

  /* Store credentials to the Preferences */
  preferences.putString("SSID", ssid);
  preferences.putString("Password", password);

  WiFi.begin(preferences.getString("SSID").c_str(), preferences.getString("Password").c_str());
 
  Serial.println("WiFi Connected.");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  /* Close the Preferences */
  preferences.end();
}

void loop() {
  // put your main code here, to run repeatedly:

}

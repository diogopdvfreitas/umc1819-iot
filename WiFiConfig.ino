#include <WiFi.h>
#include <Preferences.h>
Preferences preferences;

void setup() 
{
  Serial.begin(115200);
  Serial.println();

  /* Start a namespace "iotk"
  in Read-Write mode: set second parameter to false 
  Note: Namespace name is limited to 15 chars */
  preferences.begin("iotk", false);

  Serial.println("SSID: ");
  String ssid = Serial.readString();
  Serial.println("Password: ");
  String password = Serial.readString();

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

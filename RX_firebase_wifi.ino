#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <string>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
// #define WIFI_SSID "Sammy Barasa"
// #define WIFI_PASSWORD "$KetepaSasiniPride#"


// #define WIFI_SSID "Galaxy J6+ Kesa"
// #define WIFI_PASSWORD "fakefake2"

#define WIFI_SSID "Clio Mobile"
#define WIFI_PASSWORD "Dimsdale01"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAFXP8IfhtgO7oXQ_DLKK-vBuXnfcw2hUo"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "dronemonitoringproject@gmail.com"
#define USER_PASSWORD "dronemonitoringproject"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://drone-monitoring-project-f6fab-default-rtdb.firebaseio.com/"

#define ss 5
#define rst 14
#define dio0 2

#define max 8
// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes

String timePath = "/timestamp";
String latPath = "/lat";
String lonPath = "/lan";
String yPath = "/y";    // yaw
String pPath = "/p";   //pitch
String rPath = "/r";   //roll
String deviceidPath = "/id";   //roll
String hPath = "/h";   //roll

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;

float latd = -1.091182;
float longtd = 37.023082;
float alt =0;
float y=0;
float p=0;
float r=0;
char device_id[] = "KDMD_001";
float tstamp =0;
float h = 0;

// Timer variables (send new readings every 10 seconds)
unsigned long sendDataPrevMillis = millis();
unsigned long timerDelay = 10000;
// unsigned long prevTimeTask2 = sendDataPrevMillis;

// unsigned long intervalTask2 = 100;

using namespace std;

string strings[max]; // define max string

// length of the string
int len(string str)
{
  int length = 0;
  for (int i = 0; str[i] != '\0'; i++)
  {
    length++;
  }
  return length;
}


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  // Serial.println(WiFi.localIP());
  // Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

// create custom split() function
void split(string str, char seperator)
{
  int currIndex = 0, i = 0;
  int startIndex = 0, endIndex = 0;
  while (i <= len(str))
  {
    if (str[i] == seperator || i == len(str))
    {
      endIndex = i;
      string subStr = "";
      subStr.append(str, startIndex, endIndex - startIndex);
      strings[currIndex] = subStr;
      currIndex += 1;
      startIndex = endIndex + 1;
    }
    i++;
  }
}

void loraSetUp(){
  Serial.println("LoRa Receiver");

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);

  // replace the LoRa.begin(---E-) argument with your location's frequency
  // 433E6 for Asia
  // 866E6 for Europe
  // 915E6 for North America
  while (!LoRa.begin(866E6))
  {
    Serial.println(".");
    delay(500);
  }
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF6);
  Serial.println("LoRa Initializing OK!");
}

void setup(){
  Serial.begin(9600);
 Serial.print("Starting ...");
  initWiFi();
  timeClient.begin();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  // Serial.print("User UID: ");
  // Serial.println(uid);

  // Update database path
  databasePath = "/DroneData/" + uid + "/readings";

  loraSetUp();
}

void loop(){
   
 
    // try to parse packet
      int packetSize = LoRa.parsePacket();
      if (packetSize)
      {
      //   // received a packet
        Serial.print("Received packet '");
      //   // read packet
        while (LoRa.available())
        {
          String LoRaData = LoRa.readString();
          Serial.print(LoRaData);

      //     // split string by comma
          char seperator = ','; // comma
          split(LoRaData.c_str(), seperator);
          string splitStrings[8];
          for (int i = 0; i < max; i++)
          {
            string newLine = strings[i];
            splitStrings[i] = newLine;
          }

          latd =  stof(splitStrings[0]);
          longtd = stof(splitStrings[1]);
          alt = stof(splitStrings[2]);
          y = stof(splitStrings[3]);
          p = stof(splitStrings[4]);
          r = stof(splitStrings[5]);
          h = stof(splitStrings[6]);  
          // device_id = stoi(splitStrings[7]);           
              
          Serial.print("' with RSSI ");
          Serial.println(LoRa.packetRssi());    
        }                       
      }    

      // Send new readings to database
      if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay )){
        
        //Get current timestamp
        timestamp = getTime();
        Serial.print ("time: ");
        Serial.println (timestamp);

        parentPath= databasePath + "/" + String(timestamp);
        
        json.set(latPath.c_str(), String(latd,6));
        json.set(lonPath.c_str(), String(longtd,6));
        json.set(timePath.c_str(), String(timestamp));
        json.set(yPath.c_str(), String(y));
        json.set(pPath.c_str(), String(p));  
        json.set(rPath.c_str(), String(r)); 
        json.set(hPath.c_str(), String(h)); 
        json.set(deviceidPath.c_str(), String(device_id)); 
        
        Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
        sendDataPrevMillis = millis();    
      }
      delay(1000);
      Serial.flush();
}

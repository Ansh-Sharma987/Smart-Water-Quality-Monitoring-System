#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// including the files related to each sensors.
#include "DS18B20Sensor.h"
#include "PhSensor.h"
#include "Tds.h"


// firebase libraries
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>


// CONFIG OF FIREBASE
/* 1. Define the WiFi credentials */
#define WIFI_SSID "******" //Your SSID
#define WIFI_PASSWORD "*****" //Your password

/* 2. Define the API Key */
#define API_KEY "******************"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "*****"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "******"
#define USER_PASSWORD "****"






// oled display configurations
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Declare SSD1306 object


struct WaterParameters {
  float temperature;
  float ph;
  int tds;
  float tempInF;
  String pHStatus;
};


// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long dataMillis = 0;
bool createDoc = false;


String upLoadStatus = "Unknown";





void setup() {
  Serial.begin(115200);
  setupDS18B20Sensor();
  setupPhSensor();
  setupTdsSensor();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  // fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // // Limit the size of response payload to be collected in FirebaseData
  // fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Water Quality");
  display.println("Measurement System");
  display.display();
  delay(3000);  // Pause for 3 seconds
  display.clearDisplay();
}

void loop() {

  // Get temperature readings
  float tempInC = readTemperature();
  // float tempInC = getTemperature();
  float tempInF = (tempInC * 9 / 5) + 32;
  // Get pH sensor data
  int rawPhReading = rawPhData();
  float pHValue = readPh();
  // get tds
  float tdsValue = readTDS(tempInC);
  WaterParameters waterData;
  waterData.temperature = tempInC;
  waterData.ph = pHValue;
  waterData.tds = tdsValue;
  waterData.tempInF = tempInF;
  waterData.pHStatus = getPhStatus(pHValue);

  sendDataToFireStore(waterData);
  // Display water params on Oled
  displayDataOled(tempInC, tempInF, tdsValue, rawPhReading, pHValue);

}




// Function to display wtaer params on OLED display
void displayDataOled(float tempCelsius, float tempFahrenheit, float tdsValue, int rawPhReading, float pHValue) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setCursor(0, 0);
  display.print("Temp (C): ");
  display.println(tempCelsius, 1);
  display.print("Temp (F): ");
  display.println(tempFahrenheit, 1);
  display.print("TDS: ");
  display.print(tdsValue, 0);
  display.println(" ppm");
  display.print("Raw pH Data: ");
  display.println(rawPhReading, 0);
  display.print("pH : ");
  display.println(pHValue, 1);
  getPhStatus(pHValue);
  display.print("Upload: ");
  display.println(upLoadStatus);
  display.display();
  // delay(4000);
}

// Function to display pH status on OLED display
String getPhStatus(float pHValue) {
  String status;

  switch (int(pHValue)) {
    case 0 ... 5:
      status = "Acidic";
      break;
    case 6 ... 8:
      status = "Neutral";
      break;
    case 9 ... 14:
      status = "Basic";
      break;
    default:
      status = "Unknown";
      break;
  }

  display.print("pH Status: ");
  display.println(status);
  return status;
}

void sendDataToFireStore(WaterParameters waterData) {
  String userId = auth.token.uid.c_str();
  Serial.print("User Id is :");
  Serial.println(userId);

  if (Firebase.ready() && (millis() - dataMillis > 5000 || dataMillis == 0)) {
    dataMillis = millis();
    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Edit_Parse/Create_Edit_Parse.ino
    FirebaseJson content;
    // it is the collection id, after / is the document id.
    String documentPath = "Devices/" + userId;

    content.clear();
    content.set("fields/tds/doubleValue", String(waterData.tds).c_str());
    content.set("fields/pH/doubleValue", String(waterData.ph, 2).c_str());
    content.set("fields/temperature/doubleValue", String(waterData.temperature, 2).c_str());
    content.set("fields/phStatus/stringValue", String(waterData.pHStatus).c_str());
    content.set("fields/tempInF/doubleValue", String(waterData.tempInF, 2).c_str());
    Serial.print("Update a document... ");

    /** if updateMask contains the field name that exists in the remote document and
         * this field name does not exist in the document (content), that field will be deleted from remote document
         */

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(), "" /* updateMask */)) {
  // If the Firestore patchDocument operation was successful
     upLoadStatus = "Success";
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str()); // Print success message to Serial monitor
    } else {
        // If the Firestore patchDocument operation failed
        upLoadStatus = "Failed";
    Serial.println(fbdo.errorReason()); // Print error reason to Serial monitor
    }
  }
}


#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Wire.h>
#include <Adafruit_SH110X.h>

#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, OLED_RESET);

// Wi-Fi Credentials
#define WIFI_SSID "-------------"  //WiFi Name
#define WIFI_PASSWORD "-------------" //WiFi Password

// Firebase Info
#define API_KEY "------------------" //Your API Key
#define DATABASE_URL "---------------------" //Your Firebase URL

// Relay Pins
#define RELAY1_PIN 27
#define RELAY2_PIN 26
#define RELAY3_PIN 25
#define RELAY4_PIN 33

// Button Pins
#define BUTTON1_PIN 12
#define BUTTON2_PIN 14
#define BUTTON3_PIN 32
#define BUTTON4_PIN 18

// Firebase Variables
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Relay States
int stateRelay1 = 0;
int stateRelay2 = 0;
int stateRelay3 = 0;
int stateRelay4 = 0;

// Firebase Paths
String relayPath1 = "/Button/SW1";
String relayPath2 = "/Button/SW2";
String relayPath3 = "/Button/SW3";
String relayPath4 = "/Button/SW4";

// Timing
unsigned long sendDataPrevMillis = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  // WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi");

  // Firebase config
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp successful");
    signupOK = true;
  } else {
    Serial.printf("Firebase SignUp failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Pin setup
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  // Default OFF
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  // OLED setup
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 25);
  display.println("Connecting to Firebase...");
  display.display();

  delay(2000);

  // Sync initial states from Firebase
  syncRelay(relayPath1, stateRelay1, RELAY1_PIN);
  syncRelay(relayPath2, stateRelay2, RELAY2_PIN);
  syncRelay(relayPath3, stateRelay3, RELAY3_PIN);
  syncRelay(relayPath4, stateRelay4, RELAY4_PIN);

  updateOLED();
}

void loop() {
  // Manual Buttons
  checkButton(BUTTON1_PIN, stateRelay1, RELAY1_PIN, relayPath1);
  checkButton(BUTTON2_PIN, stateRelay2, RELAY2_PIN, relayPath2);
  checkButton(BUTTON3_PIN, stateRelay3, RELAY3_PIN, relayPath3);
  checkButton(BUTTON4_PIN, stateRelay4, RELAY4_PIN, relayPath4);

  // Firebase Sync
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 300)) {
    sendDataPrevMillis = millis();

    syncRelay(relayPath1, stateRelay1, RELAY1_PIN);
    syncRelay(relayPath2, stateRelay2, RELAY2_PIN);
    syncRelay(relayPath3, stateRelay3, RELAY3_PIN);
    syncRelay(relayPath4, stateRelay4, RELAY4_PIN);
  }
}

// Update OLED Display with All States
void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(stateRelay1 ? "Appliance1: ON" : "Appliance1: OFF");
  display.println(stateRelay2 ? "Appliance2: ON" : "Appliance2: OFF");
  display.println(stateRelay3 ? "Appliance3: ON" : "Appliance3: OFF");
  display.println(stateRelay4 ? "Appliance4: ON" : "Appliance4: OFF");
  display.display();
}

// Sync Relay from Firebase
void syncRelay(String path, int &state, int pin) {
  if (Firebase.RTDB.getInt(&fbdo, path.c_str())) {
    int fbState = 0;
  if (fbdo.dataType() == "string") {            // I set this for the ESP32 not giving the same output to the database with Mobile App
  fbState = fbdo.stringData().toInt();
  }
  else {
  fbState = fbdo.intData();
  }
  if (fbState != state) {
      state = fbState;
      digitalWrite(pin, state);
      updateOLED();
  }
  } 
  else {
    Serial.print("Firebase error: ");
    Serial.println(fbdo.errorReason());
  }
}

// Check Button Presses
void checkButton(int btnPin, int &relayState, int relayPin, String path) {
  if (digitalRead(btnPin) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      relayState = !relayState;
      digitalWrite(relayPin, relayState);
      Firebase.RTDB.setInt(&fbdo, path.c_str(), relayState);
      updateOLED();
      lastDebounceTime = millis();
    }
  }
}

// YourProject.ino - STEP 1: LED Pins + Remove Hardcoded CHAT_ID
#include <ESP32Servo.h>
#include <WiFi.h>
#include <CTBot.h>
#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <RTClib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

RTC_DS3231 rtc;

// --- WiFi & Bot Credentials ---
const char* WIFI_SSID = "Oneplus";
const char* WIFI_PASS = "12345678";
const char* BOT_TOKEN = "8279099635:AAEz_aSfiZYaiW2O4-JG7w27GHUrdN_yFq8";
// REMOVED: long long CHAT_ID = 948598949LL;  // We'll fetch this from JSON

CTBot myBot;

// --- Hardware Pins ---
const int NUM_COMPARTMENTS = 2;
const int SERVO_PINS[NUM_COMPARTMENTS] = {13, 26}; 
const int IR_PINS[NUM_COMPARTMENTS] = {14, 25};

// **ADDED: LED and BUZZER pins for each compartment**
const int LED_PINS[NUM_COMPARTMENTS] = {32, 33};      // LEDs for A and B
const int BUZZER_PINS[NUM_COMPARTMENTS] = {4, 5};     // Buzzers for A and B

#define RX_PIN 16 
#define TX_PIN 17 

Servo myServos[NUM_COMPARTMENTS];

// --- Fingerprint Setup ---
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// --- Servo Commands ---
const int SERVO_STOP = 90;
const int SERVO_SPIN_FORWARD = 40;
const int SERVO_SPIN_REVERSE = 140;
const int turnTime = 201;

// --- Wi-Fi state ---
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;

// **ADDED: JSON Server URL**
const char* JSON_SERVER = "http://10.189.36.122:5000/list";

// **ADDED: Structure to hold schedule data**
struct Schedule {
  int hour;
  int minute;
  String chatID;
  bool active;
};

Schedule scheduleA = {-1, -1, "", false};
Schedule scheduleB = {-1, -1, "", false};
unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 60000; // Fetch every 60 seconds

// **ADDED: Alarm state tracking for each compartment**
struct AlarmState {
  bool alarmActive;           // Is alarm (buzzer+LED) currently on?
  bool fingerprintScanned;    // Was fingerprint scanned during alarm?
  bool systemActive;          // Is 10-sec dispensing window active?
  bool anyMedicineTaken;      // Was ANY medicine taken during window? (for tracking)
  unsigned long alarmStartTime;
  unsigned long systemStartTime;
  int lastTriggeredMinute;    // To prevent repeated triggers in same minute
  unsigned long lastIRTriggerTime; // To debounce IR sensor
};

AlarmState stateA = {false, false, false, false, 0, 0, -1, 0};
AlarmState stateB = {false, false, false, false, 0, 0, -1, 0};

const unsigned long alarmDuration = 10000UL;   // 10 seconds for buzzer
const unsigned long systemDuration = 10000UL;  // 10 seconds for dispensing
const unsigned long irDebounceTime = 1000UL;   // 1 second debounce between dispenses

// --- Function Prototypes ---
void checkWifiConnection();
void sendTelegramAlert(String message, long long chatID);
int getFingerprintID();
void triggerServo(int index);
void fetchScheduleFromJSON();
void handleCompartment(int index, Schedule &sched, AlarmState &state, DateTime now);

void setup() {
  Serial.begin(115200);
  delay(10);

  // RTC Initialization
  Wire.begin(21, 22);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC! Check wiring.");
  } else {
    Serial.println("RTC found.");
  }
  // Get the compile time and add 30 seconds
DateTime compileTime = DateTime(__DATE__, __TIME__);
rtc.adjust(DateTime(compileTime.unixtime()+30));

  // **ADDED: Initialize LEDs and Buzzers**
  for (int i = 0; i < NUM_COMPARTMENTS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    pinMode(BUZZER_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
    digitalWrite(BUZZER_PINS[i], LOW);
  }

  // Setup Servos and IR sensors
  Serial.println("Setting up Servos and IR Sensors...");
  for (int i = 0; i < NUM_COMPARTMENTS; i++) {
    pinMode(IR_PINS[i], INPUT_PULLUP);
    myServos[i].attach(SERVO_PINS[i]);
    myServos[i].write(SERVO_STOP);
    delay(250);
  }
  Serial.println("Hardware ready.");

  // Fingerprint UART
  fingerSerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(50);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor detected!");
  } else {
    Serial.println("Fingerprint sensor not found. Check wiring.");
  }

  // Start Wi-Fi connection
  Serial.print("Starting Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println("\nSystem ready. Waiting for scheduled alarms or fingerprint.");
}

void loop() {
  DateTime now = rtc.now();
  Serial.print("RTC Time = ");
  Serial.print(now.year()); Serial.print('/');
  Serial.print(now.month()); Serial.print('/');
  Serial.print(now.day()); Serial.print(' ');
  Serial.print(now.hour()); Serial.print(':');
  Serial.print(now.minute()); Serial.print(':');
  Serial.println(now.second());
  delay(1000);
  checkWifiConnection();

  // Fetch schedule periodically
  if (millis() - lastFetchTime > fetchInterval) {
    fetchScheduleFromJSON();
    lastFetchTime = millis();
  }

  // **Check for fingerprint at ANY time**
  int fingerprintID = getFingerprintID();
  if (fingerprintID > 0) {
    Serial.printf("Fingerprint ID #%d detected!\n", fingerprintID);
    
    // If alarm A is active, stop it and start dispensing window
    if (stateA.alarmActive) {
      digitalWrite(BUZZER_PINS[0], LOW);
      digitalWrite(LED_PINS[0], LOW);
      stateA.alarmActive = false;
      stateA.fingerprintScanned = true;
      Serial.println("Alarm A stopped by fingerprint");
    }
    
    // If alarm B is active, stop it and start dispensing window
    if (stateB.alarmActive) {
      digitalWrite(BUZZER_PINS[1], LOW);
      digitalWrite(LED_PINS[1], LOW);
      stateB.alarmActive = false;
      stateB.fingerprintScanned = true;
      Serial.println("Alarm B stopped by fingerprint");
    }
    
    // Activate both compartments for dispensing for 10 seconds
    if (!stateA.systemActive) {
      stateA.systemActive = true;
      stateA.systemStartTime = millis();
      stateA.anyMedicineTaken = false;
      stateA.lastIRTriggerTime = 0;  // Reset debounce
      digitalWrite(LED_PINS[0], HIGH);
      Serial.println("Compartment A: 10-sec dispensing window ACTIVE");
    }
    
    if (!stateB.systemActive) {
      stateB.systemActive = true;
      stateB.systemStartTime = millis();
      stateB.anyMedicineTaken = false;
      stateB.lastIRTriggerTime = 0;  // Reset debounce
      digitalWrite(LED_PINS[1], HIGH);
      Serial.println("Compartment B: 10-sec dispensing window ACTIVE");
    }
  }

  // **Handle Compartment A**
  handleCompartment(0, scheduleA, stateA, now);
  
  // **Handle Compartment B**
  handleCompartment(1, scheduleB, stateB, now);

  delay(100);
}

void triggerServo(int index) {
  if (index < 0 || index >= NUM_COMPARTMENTS) return;

  Serial.print("Running servo #");
  Serial.println(index + 1);

  myServos[index].write(SERVO_SPIN_REVERSE);
  delay(turnTime);
  myServos[index].write(SERVO_STOP);
  delay(200);
  myServos[index].write(SERVO_SPIN_FORWARD);
  delay(turnTime);
  myServos[index].write(SERVO_STOP);

  Serial.println("Servo action complete.");
}

int getFingerprintID() {
  // Test sensor connection first
  if (!finger.verifyPassword()) {
    Serial.println("[FP] ERROR: Sensor not responding or password failed!");
    return -1;
  }

  uint8_t p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) {
    // Don't spam - finger not on sensor (normal)
    return 0;
  }
  if (p == FINGERPRINT_IMAGEFAIL) {
    Serial.println("[FP] ERROR: Imaging error");
    return -1;
  }
  if (p != FINGERPRINT_OK) {
    Serial.print("[FP] ERROR: Unknown error in getImage: ");
    Serial.println(p);
    return -1;
  }
  
  Serial.println("[FP] Image captured! Converting...");

  p = finger.image2Tz();
  if (p == FINGERPRINT_IMAGEMESS) {
    Serial.println("[FP] ERROR: Image too messy");
    return -1;
  }
  if (p == FINGERPRINT_FEATUREFAIL) {
    Serial.println("[FP] ERROR: Could not find fingerprint features");
    return -1;
  }
  if (p != FINGERPRINT_OK) {
    Serial.print("[FP] ERROR: Unknown error in image2Tz: ");
    Serial.println(p);
    return -1;
  }

  Serial.println("[FP] Image converted! Searching database...");

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("[FP] Fingerprint NOT FOUND in database!");
    return -1;
  }
  if (p != FINGERPRINT_OK) {
    Serial.print("[FP] ERROR: Unknown error in fingerFastSearch: ");
    Serial.println(p);
    return -1;
  }

  Serial.print("[FP] ✓ MATCH FOUND! ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence ");
  Serial.println(finger.confidence);
  
  return finger.fingerID;
}

void checkWifiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("WiFi disconnected!");
      wifiConnected = false;
    }
    if (millis() - lastWifiCheck > 5000) {
      Serial.print(".");
      lastWifiCheck = millis();
    }
  } else {
    if (!wifiConnected) {
      Serial.println("\nWiFi connected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      myBot.setTelegramToken(BOT_TOKEN);
      if (myBot.testConnection()) {
        Serial.println("Telegram bot connected!");
      } else {
        Serial.println("Telegram bot connection failed.");
      }
      wifiConnected = true;
    }
  }
}

void sendTelegramAlert(String message, long long chatID) {
  if (!wifiConnected) {
    Serial.println("Can't send alert - WiFi not connected");
    return;
  }
  
  if (myBot.sendMessage(chatID, message)) {
    Serial.println("Telegram alert sent: " + message);
  } else {
    Serial.println("Failed to send Telegram alert");
  }
}

// **ADDED: Function to fetch schedule from JSON server**
// **FIXED: Function to fetch schedule from JSON server**
void fetchScheduleFromJSON() {
  if (!wifiConnected) return;

  HTTPClient http;
  String url = String(JSON_SERVER);
  http.begin(url);
  http.addHeader("User-Agent", "ESP32");
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Fetched JSON: " + payload);

    // ArduinoJson 5 syntax - parse array
    DynamicJsonBuffer jsonBuffer(2048);
    JsonArray& scheduleArray = jsonBuffer.parseArray(payload);

    if (!scheduleArray.success()) {
      Serial.println("JSON parse error!");
      http.end();
      return;
    }

    // Reset schedules before parsing
    scheduleA.active = false;
    scheduleB.active = false;

    // Parse each schedule entry in the array
    for (int i = 0; i < scheduleArray.size(); i++) {
      JsonObject& entry = scheduleArray[i];
      
      const char* compartment = entry["compartment"];
      const char* time = entry["time"];
      const char* telegram = entry["telegram"];
      
      if (compartment == nullptr || time == nullptr || telegram == nullptr) {
        Serial.printf("Entry %d: Missing required fields\n", i);
        continue;
      }

      String timeStr = String(time);
      String chatID = String(telegram);
      
      // Parse time format "HH:MM"
      int colonPos = timeStr.indexOf(':');
      if (colonPos <= 0) {
        Serial.printf("Entry %d: Invalid time format\n", i);
        continue;
      }
      
      int hour = timeStr.substring(0, colonPos).toInt();
      int minute = timeStr.substring(colonPos + 1).toInt();

      // Assign to appropriate compartment
      if (strcmp(compartment, "A") == 0) {
        scheduleA.hour = hour;
        scheduleA.minute = minute;
        scheduleA.chatID = chatID;
        scheduleA.active = true;
        Serial.printf("Schedule A: %02d:%02d, ChatID: %s\n", hour, minute, chatID.c_str());
      } 
      else if (strcmp(compartment, "B") == 0) {
        scheduleB.hour = hour;
        scheduleB.minute = minute;
        scheduleB.chatID = chatID;
        scheduleB.active = true;
        Serial.printf("Schedule B: %02d:%02d, ChatID: %s\n", hour, minute, chatID.c_str());
      }
    }

    Serial.printf("Parsed %d schedule entries\n", scheduleArray.size());

  } else {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
  }

  http.end();
}

// **ADDED: Handle each compartment's alarm and dispensing logic**
void handleCompartment(int index, Schedule &sched, AlarmState &state, DateTime now) {
  // Check if scheduled time matches current time
  if (sched.active && 
      now.hour() == sched.hour && 
      now.minute() == sched.minute &&
      now.minute() != state.lastTriggeredMinute) {
    
    // Start alarm (buzzer + LED)
    digitalWrite(BUZZER_PINS[index], HIGH);
    digitalWrite(LED_PINS[index], HIGH);
    state.alarmActive = true;
    state.alarmStartTime = millis();
    state.fingerprintScanned = false;
    state.lastTriggeredMinute = now.minute();
    
    Serial.printf(">>> ALARM ON for Compartment %c\n", 'A' + index);
  }

  // Stop alarm after 10 seconds if fingerprint NOT scanned
  if (state.alarmActive && (millis() - state.alarmStartTime > alarmDuration)) {
    digitalWrite(BUZZER_PINS[index], LOW);
    digitalWrite(LED_PINS[index], LOW);
    state.alarmActive = false;
    
    Serial.printf(">>> ALARM OFF for Compartment %c\n", 'A' + index);
    
    // If fingerprint was NOT scanned during alarm, send Telegram alert
    if (!state.fingerprintScanned && sched.chatID.length() > 0) {
      String message = "⚠️ ALERT: Medicine not taken from Compartment ";
      message += (char)('A' + index);
      message += " at scheduled time!";
      
      long long chatID = sched.chatID.toInt();
      sendTelegramAlert(message, chatID);
    }
  }

  // Handle dispensing window (10 seconds after fingerprint)
  if (state.systemActive) {
    // Check if medicine was taken via IR sensor
    // ALLOW MULTIPLE TRIGGERS during the window with debounce
    if (digitalRead(IR_PINS[index]) == LOW) {
      unsigned long currentTime = millis();
      
      // Only trigger if enough time has passed since last trigger (debounce)
      if (currentTime - state.lastIRTriggerTime > irDebounceTime) {
        state.anyMedicineTaken = true;  // Mark that at least one medicine was taken
        state.lastIRTriggerTime = currentTime;
        
        Serial.printf("Medicine detected from Compartment %c! Dispensing...\n", 'A' + index);
        triggerServo(index);
        
        // DO NOT deactivate system - allow more triggers during 10-sec window
      }
    }

    // End dispensing window after 10 seconds
    if (millis() - state.systemStartTime > systemDuration) {
      state.systemActive = false;
      digitalWrite(LED_PINS[index], LOW);
      Serial.printf("Dispensing window closed for Compartment %c\n", 'A' + index);
      
      if (state.anyMedicineTaken) {
        Serial.printf("✓ Medicines were dispensed from Compartment %c\n", 'A' + index);
      }
    }
  }

  // Reset trigger when minute changes
  if (now.minute() != state.lastTriggeredMinute && !state.alarmActive) {
    state.lastTriggeredMinute = -1;
  }
}
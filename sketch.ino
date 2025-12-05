// Smart Retail Shelf - ESP32 + 4 ultrasonic sensors + 4 buttons + Blynk
// Shows distance in cm on Blynk
// Sends phone notifications when stock level changes

// Blynk setup
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID   "TMPL5GuNmM-t-"
#define BLYNK_TEMPLATE_NAME "ESP32RetailShelfSystem"
#define BLYNK_AUTH_TOKEN    "bq9jIrd-GhansZO2a5dhLejUpg7tykC-"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// WiFi for Wokwi
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Pin numbers for the 4 ultrasonic sensors
// Slot 1: TRIG 13, ECHO 12
// Slot 2: TRIG 14, ECHO 27
// Slot 3: TRIG 26, ECHO 25
// Slot 4: TRIG 33, ECHO 32
const int trigPins[4] = {13, 14, 26, 33};
const int echoPins[4] = {12, 27, 25, 32};

// Pin numbers for the 4 buttons (wrong product simulation)
// Slot 1: 23, Slot 2: 22, Slot 3: 19, Slot 4: 18
const int buttonPins[4] = {23, 22, 19, 18};

// Stock levels used inside the code
// 0 = Empty
// 1 = Low
// 2 = Moderate
// 3 = Full
// 4 = Wrong product
int  slotState[4]    = {0, 0, 0, 0};      // current level for each shelf
long slotDistance[4] = {0, 0, 0, 0};      // last distance in cm for each shelf
int  lastState[4]    = {-1, -1, -1, -1};  // previous level for each shelf
// Thresholds for stock levels (cm)
// distance <= fullMaxCm        -> Full
// distance <= moderateMaxCm    -> Moderate
// distance <= lowMaxCm         -> Low
// distance > lowMaxCm          -> Empty
int fullMaxCm     = 4;   // default Full max distance
int moderateMaxCm = 8;   // default Moderate max distance
int lowMaxCm      = 13;  // default Low max distance

// Read one ultrasonic sensor and return distance in cm
long readDistanceCm(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); //timeout for 30ms

  // use float for better precision, then round
  float distanceFloat = duration * 0.034f / 2.0f;
  long distance = lround(distanceFloat);  // round to nearest cm

  return distance;
}


// Turn distance into stock level using thresholds from Blynk
int classifyStockState(long d) 
{
  if (d <= 0 || d > 17)  // anything out of the main range -> Empty
  {
    return 0;  // Empty 
  }
  if (d <= fullMaxCm)  // closest range -> Full
  {
    return 3;  // Full
  }
  if (d <= moderateMaxCm) // next range -> Moderate
  {
    return 2;  // Moderate
  }
  if (d <= lowMaxCm) // next range -> Low
  {
    return 1;  // Low
  }
  return 0;    // beyond lowMaxCm but still under 17 -> Empty
}


// Turn level number into text for Serial Monitor
const char* stateToText(int s) {
  switch (s) {
    case 0: return "Empty";
    case 1: return "Low";
    case 2: return "Moderate";
    case 3: return "Full";
    case 4: return "Wrong product";
    default: return "Unknown";
  }
}

// Blynk slider for Full max distance (V4)
BLYNK_WRITE(V4) {
  fullMaxCm = param.asInt();
  Serial.print("FullMax from Blynk: ");
  Serial.println(fullMaxCm);
}

// Blynk slider for Moderate max distance (V5)
BLYNK_WRITE(V5) {
  moderateMaxCm = param.asInt();
  Serial.print("ModerateMax from Blynk: ");
  Serial.println(moderateMaxCm);
}

// Blynk slider for Low max distance (V6)
BLYNK_WRITE(V6) {
  lowMaxCm = param.asInt();
  Serial.print("LowMax from Blynk: ");
  Serial.println(lowMaxCm);
}



// Read all 4 sensors and buttons, update distance and stock level
void updateShelfStates() {
  for (int i = 0; i < 4; i++) {
    // read distance in cm for this shelf
    long distance = readDistanceCm(trigPins[i], echoPins[i]);
    slotDistance[i] = distance;

    // get level from distance (Empty, Low, Moderate, Full)
    int baseState = classifyStockState(distance);

    // read button for this shelf
    int buttonVal = digitalRead(buttonPins[i]);

    // start with level from distance
    int finalState = baseState;

    // if button is pressed (LOW), mark as Wrong product
    if (buttonVal == LOW) {
      finalState = 4; // Wrong product
    }

    // save final level
    slotState[i] = finalState;

    // print info to Serial Monitor
    Serial.print("Shelf ");
    Serial.print(i + 1);
    Serial.print("  Dist: ");
    Serial.print(distance);
    Serial.print(" cm   Level: ");
    Serial.println(stateToText(finalState));
  }

  Serial.println("---------------------------");
}

// Check for changes in level and send Blynk notifications
// Uses Custom Event in Blynk with code: shelf_alert_testing and template: {{message}}
void checkAndNotify() {
  for (int i = 0; i < 4; i++) {
    int currentState = slotState[i];
    long d           = slotDistance[i];

    // only do something if the level changed since last time
    if (currentState != lastState[i]) {
      Serial.print("Level change on shelf ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(stateToText(lastState[i]));
      Serial.print(" -> ");
      Serial.println(stateToText(currentState));

      // update lastState so we remember the new level
      lastState[i] = currentState;

      // build message text for phone notification
      String msg;

      if (currentState == 0) {       // Empty
        msg = "Shelf " + String(i + 1) +
              " is EMPTY (distance " + String(d) + " cm)";
      }
      else if (currentState == 1) {  // Low
        msg = "Shelf " + String(i + 1) +
              " is LOW (distance " + String(d) + " cm)";
      }
      else if (currentState == 2) {  // Moderate
        msg = "Shelf " + String(i + 1) +
              " is MODERATE (distance " + String(d) + " cm)";
      }
      else if (currentState == 3) {  // Full
        msg = "Shelf " + String(i + 1) +
              " is FULL (distance " + String(d) + " cm)";
      }
      else if (currentState == 4) {  // Wrong product
        msg = "Shelf " + String(i + 1) +
              " has WRONG PRODUCT (distance " + String(d) + " cm)";
      } else {
        // unknown state, do not send anything
        msg = "";
      }

      // send notification only if we built a message
      if (msg.length() > 0) {
        Serial.print("Sending event: ");
        Serial.println(msg);
        Blynk.logEvent("shelf_alert_testing", msg);
      }
    }
  }
}

// Send one line per shelf to V7..V10
void sendStatusSummary() {
  for (int i = 0; i < 4; i++) {
    String line = "Shelf ";
    line += String(i + 1);
    line += ": ";
    line += stateToText(slotState[i]);
    line += " (";
    line += String(slotDistance[i]);
    line += " cm)";

    // send to V7, V8, V9, V10
    Blynk.virtualWrite(V7 + i, line);
  }
}



// Send cm distances to Blynk gauges V0, V1, V2, V3
void sendToBlynk() {
  for (int i = 0; i < 4; i++) {
    Blynk.virtualWrite(V0 + i, slotDistance[i]);
  }
}

void setup() {
  // 115200 is the speed of Serial in bits per second
  // it must match the Serial Monitor setting
  Serial.begin(115200);

  // set pin modes for all sensors and buttons
  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  Serial.println("Connecting to Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  // keep Blynk connection working
  Blynk.run();

  // main steps that run once per second
  updateShelfStates();   // read sensors and buttons
  checkAndNotify();      // send phone alerts on changes
  sendToBlynk();         // send cm values to dashboard
  sendStatusSummary();  // send text summary to V7

  delay(2500);           // wait 2.5 second
}

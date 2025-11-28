// Smart Retail Shelf - 4 ultrasonic sensors on your ESP32 pin layout
// Slot to pin mapping
// Slot 1: TRIG 13, ECHO 12
// Slot 2: TRIG 14, ECHO 27
// Slot 3: TRIG 26, ECHO 25
// Slot 4: TRIG 33, ECHO 32

const int trigPins[4] = {13, 14, 26, 33};
const int echoPins[4] = {12, 27, 25, 32};

// 0 = Full, 1 = Low, 2 = Empty
int slotState[4] = {0, 0, 0, 0};

long readDistanceCm(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30 ms
  long distance = duration * 0.034 / 2;           // speed of sound
  return distance;
}

// Adjust these thresholds later if you change your shelf depth
int classifyStockState(long distanceCm) {
  if (distanceCm == 0) {
    return 2;          // treat no echo as empty
  } else if (distanceCm < 3) {
    return 0;          // Full
  } else if (distanceCm < 7) {
    return 1;          // Low
  } else {
    return 2;          // Empty
  }
}

const char* stateToText(int stateCode) {
  switch (stateCode) {
    case 0: return "Full";
    case 1: return "Low";
    case 2: return "Empty";
    default: return "Unknown";
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }

  Serial.println("Smart Retail Shelf - 4 slot test starting...");
}

void loop() {
  for (int i = 0; i < 4; i++) {
    long distance = readDistanceCm(trigPins[i], echoPins[i]);
    int state = classifyStockState(distance);

    slotState[i] = state;

    Serial.print("Slot ");
    Serial.print(i + 1);
    Serial.print("  Distance: ");
    Serial.print(distance);
    Serial.print(" cm   State: ");
    Serial.println(stateToText(state));
  }

  Serial.println("---------------------------");
  delay(1000);
}

// Single HC-SR04 on ESP32 test
// Sensor pins: TRIG -> GPIO 13, ECHO -> GPIO 12

const int trigPin = 13;
const int echoPin = 12;

long readDistanceCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);  // timeout 30 ms
  long distance = duration * 0.034 / 2;
  return distance;
}

int classifyStockState(long distanceCm) {
  if (distanceCm == 0) {
    return 2;          // Empty if no echo
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

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.println("Single shelf test starting...");
}

void loop() {
  long distance = readDistanceCm();
  int state = classifyStockState(distance);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm   State: ");
  Serial.println(stateToText(state));

  delay(1000);
}

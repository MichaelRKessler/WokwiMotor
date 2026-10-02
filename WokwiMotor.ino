// DC motor control through an L293D H-bridge (channels 1 & 2)
// Potentiometer sets speed, pushbutton toggles forward/reverse.
const int EN12_PIN = 26;  // L293D EN1,2 - PWM sets speed
const int IN1_PIN = 25;   // L293D 1A
const int IN2_PIN = 27;   // L293D 2A
const int POT_PIN = 36;   // Speed potentiometer wiper (VP, ADC1 - works with WiFi on)
const int BTN_PIN = 23;   // Forward/Reverse button to GND

const int PWM_FREQ = 1000;
const int PWM_BITS = 8;
const unsigned long DEBOUNCE_MS = 30;

bool forward = true;
int lastDuty = -1;
bool lastBtnReading = HIGH;
bool btnState = HIGH;
unsigned long lastBtnChange = 0;

// speed: -255 (full reverse) .. 255 (full forward), 0 = brake
void setMotor(int speed) {
  speed = constrain(speed, -255, 255);
  digitalWrite(IN1_PIN, speed > 0 ? HIGH : LOW);
  digitalWrite(IN2_PIN, speed < 0 ? HIGH : LOW);
  ledcWrite(EN12_PIN, abs(speed));
}

// Returns true once per debounced press.
bool buttonPressed() {
  bool reading = digitalRead(BTN_PIN);
  if (reading != lastBtnReading) {
    lastBtnChange = millis();
    lastBtnReading = reading;
  }
  if (millis() - lastBtnChange > DEBOUNCE_MS && reading != btnState) {
    btnState = reading;
    return btnState == LOW;
  }
  return false;
}

void printStatus(int duty) {
  Serial.printf("%s, speed %d%%\n", forward ? "Forward" : "Reverse", duty * 100 / 255);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 L293D DC Motor - pot = speed, button = direction");
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  ledcAttach(EN12_PIN, PWM_FREQ, PWM_BITS);
  setMotor(0);
}

void loop() {
  if (buttonPressed()) {
    forward = !forward;
    lastDuty = -1;  // force a status print
  }

  int duty = map(analogRead(POT_PIN), 0, 4095, 0, 255);
  setMotor(forward ? duty : -duty);

  if (lastDuty < 0 || abs(duty - lastDuty) >= 13) {  // ~5% steps
    printStatus(duty);
    lastDuty = duty;
  }
  delay(10);
}

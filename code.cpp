// Particle Counter with Nernst Equation Display
// Arduino Uno R3 — Tinkercad Compatible
// 16x2 LCD display + Two HC-SR04 ultrasonic sensors
//
// Row 1: C1:xx    C2:xx   (remaining particles, counting down)
// Row 2: N1:xx    N2:xx   (Nernst voltage results)

#include <LiquidCrystal.h>
#include <math.h>

// --- Pin Configuration ---

// LCD pins (6 pins: RS, EN, D4–D7)
const int LCD_RS = 2;
const int LCD_EN = 3;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6;
const int LCD_D7 = 7;

// HC-SR04 Sensor 1
const int TRIG_PIN1 = 8;
const int ECHO_PIN1 = 9;

// HC-SR04 Sensor 2
const int TRIG_PIN2 = 10;
const int ECHO_PIN2 = 11;

// --- Sensor Configuration ---
const int TARGET_DISTANCE_CM = 5;
const int DISTANCE_TOLERANCE_CM = 1;
const int DEBOUNCE_DELAY = 100; // ms to ignore repeated triggers

// --- Counter Configuration ---
const int START_VALUE = 99; // Initial particle count (tweak per experiment)
const int MIN_VALUE = 1;    // Floor to avoid division by zero
const int STEP_1 = 6;
const int STEP_2 = 9;

// --- Nernst Constants (tweak per particle type) ---
const float NERNST_NUMERATOR = 59.2; // 0.0592 V converted to mV
const float LOG10_Q = 0.60206;       // log10(4), reaction quotient

// --- LCD Object ---
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// --- State ---
int counter1 = START_VALUE;
int counter2 = START_VALUE;

bool lastSensor1State = false;
bool lastSensor2State = false;

unsigned long lastTriggerTime1 = 0;
unsigned long lastTriggerTime2 = 0;

// --- Functions ---

int measureDistance(int trigPin, int echoPin)
{
  // Send 10µs trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse (timeout 30 ms ≈ ~5 m max range)
  unsigned long duration = pulseIn(echoPin, HIGH, 30000UL);

  if (duration == 0)
  {
    return -1; // No echo received
  }

  // Round-trip: ~58 µs/cm
  return (int)(duration / 58UL);
}

// Nernst equation: E = (0.0592 V / counter_mV) * log10(Q)
// With mV conversion: (59.2 / counter) * log10(4)
// Result clamped to 0–99 for display
int computeNernst(int counterVal)
{
  float result = (NERNST_NUMERATOR / (float)counterVal) * LOG10_Q;
  int rounded = (int)round(result);

  // Clamp to displayable range
  if (rounded < 0) rounded = 0;
  if (rounded > 99) rounded = 99;

  return rounded;
}

// Format a 2-digit number with leading zero (e.g. 5 → "05")
void printPadded(int value)
{
  if (value < 10) lcd.print('0');
  lcd.print(value);
}

// Update the LCD with current counters and Nernst results
void updateDisplay()
{
  int nernst1 = computeNernst(counter1);
  int nernst2 = computeNernst(counter2);

  // Row 1: counters
  // Format: "C1:99    C2:99  "
  lcd.setCursor(0, 0);
  lcd.print("VNa:");
  printPadded(counter1);
  lcd.print("    ");
  lcd.print("VK:");
  printPadded(counter2);

  // Row 2: Nernst results
  // Format: "N1:00    N2:00  "
  lcd.setCursor(0, 1);
  lcd.print("Na:");
  printPadded(nernst1);
  lcd.print("    ");
  lcd.print("K:");
  printPadded(nernst2);
}

void setup()
{
  // Setup HC-SR04 sensor pins
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // Setup LCD (16 columns, 2 rows)
  lcd.begin(16, 2);

  Serial.begin(9600);
  Serial.println("Particle Counter with Nernst Equation");
  Serial.print("Start: ");
  Serial.print(START_VALUE);
  Serial.print(" | Min: ");
  Serial.println(MIN_VALUE);

  // Show initial values
  updateDisplay();
}

void loop()
{
  // Measure distances
  int distance1 = measureDistance(TRIG_PIN1, ECHO_PIN1);
  int distance2 = measureDistance(TRIG_PIN2, ECHO_PIN2);

  // Check if object is within target range
  bool sensor1Triggered = (distance1 >= 0) &&
                          (abs(distance1 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM);
  bool sensor2Triggered = (distance2 >= 0) &&
                          (abs(distance2 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM);

  unsigned long now = millis();
  bool changed = false;

  // Sensor 1: rising edge + debounce → count down
  if (sensor1Triggered && !lastSensor1State && (now - lastTriggerTime1 > DEBOUNCE_DELAY))
  {
    if (counter1 > MIN_VALUE)
    {
      counter1 = counter1 - STEP_1;
      changed = true;
      lastTriggerTime1 = now;
      Serial.print("Counter 1: ");
      Serial.print(counter1);
      Serial.print(" | Nernst: ");
      Serial.println(computeNernst(counter1));
    }
  }
  lastSensor1State = sensor1Triggered;

  // Sensor 2: rising edge + debounce → count down
  if (sensor2Triggered && !lastSensor2State && (now - lastTriggerTime2 > DEBOUNCE_DELAY))
  {
    if (counter2 > MIN_VALUE)
    {
      counter2 = counter2 - STEP_2;
      changed = true;
      lastTriggerTime2 = now;
      Serial.print("Counter 2: ");
      Serial.print(counter2);
      Serial.print(" | Nernst: ");
      Serial.println(computeNernst(counter2));
    }
  }
  lastSensor2State = sensor2Triggered;

  // Only refresh LCD when a counter changes
  if (changed)
  {
    updateDisplay();
  }
}
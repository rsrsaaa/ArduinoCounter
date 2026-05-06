// Particle Counter with Nernst Equation Display
// Arduino Uno R3
// I2C 16x2 LCD display + Two HC-SR04 ultrasonic sensors
//
// Row 1: C1:xx    C2:xx   (remaining particles, counting down)
// Row 2: N1:xx    N2:xx   (Nernst voltage results)

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <math.h>

// --- Pin Configuration ---

// LCD I2C address (0x27 is the most common default for PCF8574 backpacks;
// if nothing shows on screen, try 0x3F instead)
const int LCD_I2C_ADDR = 0x27;

// HC-SR04 Sensor 1
const int TRIG_PIN1 = 8;
const int ECHO_PIN1 = 9;

// HC-SR04 Sensor 2
const int TRIG_PIN2 = 10;
const int ECHO_PIN2 = 11;

// --- Sensor Configuration ---
const int TARGET_DISTANCE_CM = 5;
const int DISTANCE_TOLERANCE_CM = 1;
const int DEBOUNCE_DELAY = 30; // ms to ignore repeated triggers

// --- Counter Configuration ---
const int START_VALUE = 99; // Initial particle count (tweak per experiment)

// --- LCD Object (I2C) ---
// SDA → A4, SCL → A5 on Arduino Uno (hardware I2C, no pin config needed)
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

// --- State ---
int counter1 = START_VALUE;
int counter2 = START_VALUE;

bool lastSensor1State = false;
bool lastSensor2State = false;

unsigned long lastTriggerTime1 = 0;
unsigned long lastTriggerTime2 = 0;

// --- Functions ---

// Trigger both HC-SR04 sensors and read echoes simultaneously.
// Uses manual polling instead of blocking pulseIn() to read both in parallel.
// Total time: ~3.2ms worst case (vs ~6ms sequential)
void measureBothDistances(int &dist1, int &dist2)
{
  // Trigger sensor 1
  digitalWrite(TRIG_PIN1, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN1, LOW);

  // Small gap to avoid ultrasonic crosstalk between sensors
  delayMicroseconds(200);

  // Trigger sensor 2
  digitalWrite(TRIG_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN2, LOW);

  // Poll both echo pins simultaneously
  unsigned long startTime = micros();
  const unsigned long TIMEOUT = 3000UL; // 3ms ≈ ~50 cm max range

  unsigned long rise1 = 0, fall1 = 0;
  unsigned long rise2 = 0, fall2 = 0;
  bool echo1High = false, echo2High = false;
  bool done1 = false, done2 = false;

  while (micros() - startTime < TIMEOUT)
  {
    unsigned long now = micros();

    // Track echo pin 1 rising/falling edges
    if (!done1)
    {
      if (digitalRead(ECHO_PIN1))
      {
        if (!echo1High) { rise1 = now; echo1High = true; }
      }
      else if (echo1High)
      {
        fall1 = now; done1 = true;
      }
    }

    // Track echo pin 2 rising/falling edges
    if (!done2)
    {
      if (digitalRead(ECHO_PIN2))
      {
        if (!echo2High) { rise2 = now; echo2High = true; }
      }
      else if (echo2High)
      {
        fall2 = now; done2 = true;
      }
    }

    // Exit early if both echoes received
    if (done1 && done2) break;
  }

  // Convert durations to cm (round-trip: ~58 µs/cm)
  dist1 = done1 ? (int)((fall1 - rise1) / 58UL) : -1;
  dist2 = done2 ? (int)((fall2 - rise2) / 58UL) : -1;
}

// Format a 2-digit number with leading zero (e.g. 5 → "05")
void printPadded(int value)
{
  if (value < 10) lcd.print('0');
  lcd.print(value);
}

// Partial LCD update: only rewrite the 2 digits for counter 1 and its Nernst result
// Row 0 col 4–5 = counter1 digits, Row 1 col 3–4 = nernst1 digits
void updateCounter1Display()
{
  lcd.setCursor(4, 0);                          // "VNa:XX"
  printPadded(counter1);
  lcd.setCursor(3, 1);
  if (counter1 < 99)                          // "Na:XX"
    printPadded((35 + counter1 - 1) / counter1);
  else
    printPadded(0);  
}

// Partial LCD update: only rewrite the 2 digits for counter 2 and its Nernst result
// Row 0 col 13–14 = counter2 digits, Row 1 col 11–12 = nernst2 digits
void updateCounter2Display()
{
  lcd.setCursor(12, 0);                         // "VK:XX"
  printPadded(counter2);
  lcd.setCursor(11, 1);
  if (counter2 < 99)                          // "K:XX"
    printPadded((35 + counter2 - 1) / counter2);
  else
    printPadded(0);  
}

void setup()
{
  // Setup HC-SR04 sensor pins
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // Setup LCD (I2C)
  lcd.init();
  lcd.backlight();

  // Paint static labels once (they never change)
  // Row 0: "VNa:99    VK:99"
  lcd.setCursor(0, 0);
  lcd.print("VNa:     VK:");
  // Row 1: "Na:00     K:00 "
  lcd.setCursor(0, 1);
  lcd.print("Na:      K:");

  Serial.begin(9600);

  // Show initial digit values
  updateCounter1Display();
  updateCounter2Display();
}

void loop()
{
  // Measure both sensors simultaneously (interleaved)
  int distance1, distance2;
  measureBothDistances(distance1, distance2);

  // Check if object is within target range
  bool sensor1Triggered = (distance1 >= 0) &&
                          (abs(distance1 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM);
  bool sensor2Triggered = (distance2 >= 0) &&
                          (abs(distance2 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM);

  unsigned long now = millis();

  // Sensor 1: rising edge + debounce → count down
  if (sensor1Triggered && !lastSensor1State && (now - lastTriggerTime1 > DEBOUNCE_DELAY))
  {
    if (counter1 > 9)
    {
      counter1 = counter1 - 9;
      lastTriggerTime1 = now;
      updateCounter1Display(); // Only refresh the 4 chars that changed
    }
  }
  lastSensor1State = sensor1Triggered;

  // Sensor 2: rising edge + debounce → count down
  if (sensor2Triggered && !lastSensor2State && (now - lastTriggerTime2 > DEBOUNCE_DELAY))
  {
    if (counter2 > 6)
    {
      counter2 = counter2 - 6;
      lastTriggerTime2 = now;
      updateCounter2Display(); // Only refresh the 4 chars that changed
    }
  }
  lastSensor2State = sensor2Triggered;
}
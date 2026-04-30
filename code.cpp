// Particle Counter with Nernst Equation Display
// Arduino Uno R3
// Two TM1637 4-digit displays + Two HC-SR04 ultrasonic sensors
//
// Display 1: counter1 : counter2   (remaining particles)
// Display 2: result1  : result2    (Nernst voltage)

#include <TM1637Display.h>
#include <math.h>

// --- Pin Configuration ---

// TM1637 Display 1 (shows counters)
const int DISP1_CLK = 2;
const int DISP1_DIO = 3;

// TM1637 Display 2 (shows Nernst results)
const int DISP2_CLK = 4;
const int DISP2_DIO = 5;

// HC-SR04 Sensor 1
const int TRIG_PIN1 = 6;
const int ECHO_PIN1 = 7;

// HC-SR04 Sensor 2
const int TRIG_PIN2 = 8;
const int ECHO_PIN2 = 9;

// --- Sensor Configuration ---
const int TARGET_DISTANCE_CM = 5;
const int DISTANCE_TOLERANCE_CM = 1;
const int DEBOUNCE_DELAY = 300; // ms to ignore repeated triggers

// --- Counter Configuration ---
const int START_VALUE = 99; // Initial particle count (tweak later per experiment)
const int MIN_VALUE = 1;    // Floor to avoid division by zero

// --- Nernst Constants (tweak later per particle type) ---
const float NERNST_NUMERATOR = 59.2; // 0.0592 V converted to mV
const float LOG10_Q = 0.60206;       // log10(4), reaction quotient

// --- Display Objects ---
TM1637Display dispCounters(DISP1_CLK, DISP1_DIO);
TM1637Display dispResults(DISP2_CLK, DISP2_DIO);

// Colon bitmask for XX:XX format
const uint8_t COLON_ON = 0b01000000;

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
// Result clamped to 0–99 for 2-digit display
int computeNernst(int counterVal)
{
  float result = (NERNST_NUMERATOR / (float)counterVal) * LOG10_Q;
  int rounded = (int)round(result);

  // Clamp to displayable range
  if (rounded < 0) rounded = 0;
  if (rounded > 99) rounded = 99;

  return rounded;
}

// Update both TM1637 displays
void updateDisplays()
{
  // Display 1: counter1:counter2
  int counterDisplay = counter1 * 100 + counter2;
  dispCounters.showNumberDecEx(counterDisplay, COLON_ON, true);

  // Display 2: result1:result2
  int result1 = computeNernst(counter1);
  int result2 = computeNernst(counter2);
  int resultDisplay = result1 * 100 + result2;
  dispResults.showNumberDecEx(resultDisplay, COLON_ON, true);
}

void setup()
{
  // Setup HC-SR04 sensor pins
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // Setup TM1637 displays (brightness 0–7)
  dispCounters.setBrightness(5);
  dispResults.setBrightness(5);

  Serial.begin(9600);
  Serial.println("Particle Counter with Nernst Equation");
  Serial.print("Start: ");
  Serial.print(START_VALUE);
  Serial.print(" | Min: ");
  Serial.println(MIN_VALUE);

  // Show initial values
  updateDisplays();
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
      counter1--;
      changed = true;
      lastTriggerTime1 = now;
      Serial.print("Counter 1: ");
      Serial.print(counter1);
      Serial.print(" → Nernst: ");
      Serial.println(computeNernst(counter1));
    }
  }
  lastSensor1State = sensor1Triggered;

  // Sensor 2: rising edge + debounce → count down
  if (sensor2Triggered && !lastSensor2State && (now - lastTriggerTime2 > DEBOUNCE_DELAY))
  {
    if (counter2 > MIN_VALUE)
    {
      counter2--;
      changed = true;
      lastTriggerTime2 = now;
      Serial.print("Counter 2: ");
      Serial.print(counter2);
      Serial.print(" → Nernst: ");
      Serial.println(computeNernst(counter2));
    }
  }
  lastSensor2State = sensor2Triggered;

  // Only refresh displays when a counter changes
  if (changed)
  {
    updateDisplays();
  }
}
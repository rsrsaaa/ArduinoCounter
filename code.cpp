// 7-Segment Display Counter with Two HC-SR04 Ultrasonic Sensors
// Arduino Uno R3

// pins 13-7 for segments a-g of display 1
// pins 6-2 + A2, A3 for segments a-g of display 2

const int segPins[2][7] = {{13, 12, 11, 10, 9, 8, 7}, 
                           {6, 5, 4, 3, 2, A2, A3}};

// HC-SR04 Sensor 1 pins
const int TRIG_PIN1 = A0;
const int ECHO_PIN1 = A1;

// HC-SR04 Sensor 2 pins
const int TRIG_PIN2 = A4;
const int ECHO_PIN2 = A5;

const int TARGET_DISTANCE_CM = 5;
const int DISTANCE_TOLERANCE_CM = 1;
const int DEBOUNCE_DELAY = 300;   // ms to ignore repeated triggers

// Digit patterns for 0-9 (segments: a, b, c, d, e, f, g)
// 1 = segment ON, 0 = segment OFF
const byte digits[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}, // 9
};

int counter1 = 0;
bool lastSensor1State = false;
unsigned long lastTriggerTime1 = 0;

int counter2 = 0;
bool lastSensor2State = false;
unsigned long lastTriggerTime2 = 0;

void displayDigit(int n, int disp) 
{
  n = n % 10; // Keep within 0-9
  for (int i = 0; i < 7; i++) 
  {
    digitalWrite(segPins[disp][i], digits[n][i]);
  }
}

// Measures distance in cm using an HC-SR04 sensor.
// Returns -1 if no echo is received (out of range / timeout).
int measureDistance(int trigPin, int echoPin) 
{
  // Send 10µs trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse duration (timeout 30 ms ≈ ~5 m max range)
  unsigned long duration = pulseIn(echoPin, HIGH, 30000UL);

  if (duration == 0) {
    return -1; // No echo received
  }

  // Speed of sound ≈ 343 m/s → ~29.1 µs/cm one-way, 58.2 µs/cm round-trip
  return (int)(duration / 58UL);
}

void setup() 
{
  // Setup display pins
  for (int i = 0; i < 7; i++) 
  {
    for (int j = 0; j < 2; j++) 
    {
      pinMode(segPins[j][i], OUTPUT);
    }
  }
  
  // Setup HC-SR04 sensor 1 pins
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);

  // Setup HC-SR04 sensor 2 pins
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  Serial.begin(9600);

  displayDigit(0, 0);
  displayDigit(0, 1);

  Serial.println("Counter 1 ready.");
  Serial.println("Counter 2 ready.");
}

void loop() {
  // Measure both sensors
  int distance1 = measureDistance(TRIG_PIN1, ECHO_PIN1);
  int distance2 = measureDistance(TRIG_PIN2, ECHO_PIN2);

  // Trigger when an object is detected within target distance ± tolerance
  bool sensor1Triggered = distance1 >= 0 && abs(distance1 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM;
  bool sensor2Triggered = distance2 >= 0 && abs(distance2 - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM;

  unsigned long now = millis();

  // Sensor 1: detect rising edge + debounce
  if (sensor1Triggered && !lastSensor1State && (now - lastTriggerTime1 > DEBOUNCE_DELAY)) {
    counter1++;
    lastTriggerTime1 = now;
    Serial.print("Counter 1: ");
    Serial.println(counter1);
    displayDigit(counter1, 0);
  }
  lastSensor1State = sensor1Triggered;

  // Sensor 2: detect rising edge + debounce
  if (sensor2Triggered && !lastSensor2State && (now - lastTriggerTime2 > DEBOUNCE_DELAY)) {
    counter2++;
    lastTriggerTime2 = now;
    Serial.print("Counter 2: ");
    Serial.println(counter2);
    displayDigit(counter2, 1);
  }
  lastSensor2State = sensor2Triggered;
}
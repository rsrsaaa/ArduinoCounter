// 7-Segment Display Counter with Motion Sensor

// pins 13-7 for segments a-g of display 1
// pins 6-2 + A2, A3 for segments a-g of display 2

const int segPins[2][7] = {{13, 12, 11, 10, 9, 8, 7}, 
                           {6, 5, 4, 3, 2, A2, A3}};

const int SENSOR_PIN1 = A0;
const int SENSOR_PIN2 = A1;

const int SENSOR_THRESHOLD = 5 * 58; // Adjust based on your sensor (0-1023)
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
  
  // Setup sensor pins
  pinMode(SENSOR_PIN1, INPUT);
  Serial.begin(9600);
  displayDigit(0, 0);
  Serial.println("Counter 1 ready.");
  
  pinMode(SENSOR_PIN2, INPUT);
  Serial.begin(9600);
  displayDigit(0, 1);
  Serial.println("Counter 2 ready.");
}

void loop() {
  int sensor1Value = analogRead(SENSOR_PIN1);
  bool sensor1Triggered = sensor1Value > SENSOR_THRESHOLD;

  pinMode(SENSOR_PIN2, OUTPUT);
  digitalWrite(SENSOR_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(SENSOR_PIN2, HIGH);
  delayMicroseconds(5);
  digitalWrite(SENSOR_PIN2, LOW);
  pinMode(SENSOR_PIN2, INPUT);
  unsigned long sensor2Duration = pulseIn(SENSOR_PIN2, HIGH, 30000UL);
  int sensor2Distance = sensor2Duration > 0 ? (int)(sensor2Duration / 58UL) : -1;
  bool sensor2Triggered = sensor2Distance >= 0 &&
                          abs(sensor2Distance - TARGET_DISTANCE_CM) <= DISTANCE_TOLERANCE_CM;
  
  unsigned long now = millis();

  // Detect rising edge + debounce
  if (sensor1Triggered && !lastSensor1State && (now - lastTriggerTime1 > DEBOUNCE_DELAY)) {
    counter1++;
    lastTriggerTime1 = now;
    Serial.print("Count: ");
    Serial.println(counter1);
    displayDigit(counter1, 0);
  }

  lastSensor1State = sensor1Triggered;
  
  if (sensor2Triggered && !lastSensor2State && (now - lastTriggerTime2 > DEBOUNCE_DELAY)) {
    counter2++;
    lastTriggerTime2 = now;
    Serial.print("Count: ");
    Serial.println(counter2);
    displayDigit(counter2, 1);
  }
  lastSensor2State = sensor2Triggered;
}
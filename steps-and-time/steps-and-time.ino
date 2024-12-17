#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Create an OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Step counter variables
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
unsigned long stepPreviousMillis = 0;
const long stepInterval = 1000;
long stepCount = 0;
unsigned long stepCurrentMillis;
int stepFlag = 0;

// Time variables
unsigned long timePreviousMillis = 0;
int seconds = 0;
int minutes = 24;
int hours = 20;

// BPM
const int sensorPin = A0; 
int sensorValue;

const int threshold = 520; // todo: play with the it
unsigned long lastBeatTime = 0; // time of last detected peak
int bpm = 0;

void setup() {
  Serial.begin(9600);
  // Serial.begin(115200); 
  Wire.begin();

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Stop execution if the display is not detected
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(2000); // Show a starting message

  setupMPU(); // Initialize MPU
}

void loop() {
  recordAccelRegisters();
  calculateSteps();
  updateTime();
  BPM();
  updateDisplay();
  delay(100);
}

void BPM(){
  Serial.print(F(","));

  // int sensorValue = analogRead(A0);  
  // Serial.print("HeartRate: ");
  Serial.println(sensorValue);       
  delay(10);

  sensorValue = analogRead(A0);

  if (sensorValue > threshold && (millis() - lastBeatTime > 300)) { 
    // Detect peak only if 300ms (2 Hz) has passed to avoid double-counting
    unsigned long currentTime = millis();
    int beatInterval = currentTime - lastBeatTime;
    lastBeatTime = currentTime;

    bpm = 60000 / beatInterval;
    // Serial.print("BPM: ");
    Serial.print(F(","));
    Serial.println(bpm);
  }

  delay(10);
}

void setupMPU() {
  Wire.beginTransmission(0x68); // I2C address of the MPU
  Wire.write(0x6B);             // Power Management register
  Wire.write(0b00000000);       // Wake up the MPU
  Wire.endTransmission();
}

void recordAccelRegisters() {
  Wire.beginTransmission(0x68); // I2C address of the MPU
  Wire.write(0x3B);             // Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0x68, 6);    // Request Accel Registers
  while (Wire.available() < 6);
  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();
  processAccelData();
}

void processAccelData() {
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0;
  gForceZ = accelZ / 16384.0;
}

void calculateSteps() {
  if (gForceY > 0.5) {
    stepFlag = 1;
    stepPreviousMillis = millis();
    stepCurrentMillis = millis();
  }

  if ((stepCurrentMillis - stepPreviousMillis <= stepInterval) && (stepFlag)) {
    if (gForceY < -0.5) {
      stepCount++;
      stepFlag = 0;
    }
  }

  stepCurrentMillis = millis();
  if (stepCurrentMillis - stepPreviousMillis > stepInterval) {
    stepFlag = 0;
  }

  // Serial.print("Steps = ");
  Serial.print(stepCount);
}

void updateTime() {
  unsigned long currentMillis = millis();
  if (currentMillis - timePreviousMillis >= 1000) {
    timePreviousMillis = currentMillis;
    seconds++;

    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
      }
    }
  }
}

void updateDisplay() {
  display.clearDisplay(); 

  // Display the step count
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Steps: ");
  display.print(stepCount);

  display.setCursor(50, 0);
  display.print("BPM: ");
  display.print(bpm);

  // Display the time
  display.setTextSize(1);
  display.setCursor(0, 20);
  if (hours < 10) display.print("0");
  display.print(hours);
  display.print(":");
  if (minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if (seconds < 10) display.print("0");
  display.print(seconds);

  display.display(); // Push the buffer to the screen
}
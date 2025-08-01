// Cube World Arduino Firmware (Core Logic)
// Designed for Seeed XIAO RP2040
// This sketch sets up random action triggering after random delays.
// This version reads the ICM-42688-P accelerometer and gyroscope manually via I2C
// and initializes an OLED display via I2C.

// --- Includes ---
#include <Wire.h> // Required for I2C communication
#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_SSD1306.h> // Hardware-specific library for the OLED

// --- Global Variables ---
unsigned long previousMillis = 0;   // Stores last time an action was triggered
long interval = 0;                  // Stores the current random interval for the next action

// Define I2C address and register addresses for the ICM-42688-P
const int ICM_ADDR = 0x68; // 7-bit I2C address (when AD0 is LOW/GND)

// Power Management Register
const int PWR_MGMT0 = 0x4E;
const int PWR_MGMT0_MODE_LN = 0b00000011; // Low-noise mode for accel and gyro

// Sensor Configuration Registers
const int ACCEL_CONFIG0 = 0x50;
const int ACCEL_CONFIG0_RANGE_4G = 0b00000100; // Set accelerometer range to +/- 4G (16384 LSB/g)
const int ACCEL_CONFIG0_RATE_12_5_HZ = 0b01100000; // 12.5 Hz ODR

const int GYRO_CONFIG0 = 0x4F;
const int GYRO_CONFIG0_RANGE_500DPS = 0b00001000; // Set gyro range to +/- 500 dps (65.5 LSB/dps)
const int GYRO_CONFIG0_RATE_12_5_HZ = 0b01100000; // 12.5 Hz ODR

// Sensor Data Registers (start reading from here)
const int TEMP_DATA0 = 0x1F;

// Button Pin Definitions
// These pins are connected to buttons with external pull-down resistors
const int BUTTON_ADDPEEP_PIN = 27; // GP27 / A1 on the XIAO RP2040
const int BUTTON_RESET_PIN = 26;   // GP26 / A0 on the XIAO RP2040

// OLED Display Definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
const int OLED_I2C_ADDR = 0x3C; // The datasheet gives 0x78, which is the 8-bit address. The 7-bit is 0x3C.

// Create an instance of the OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Function Prototypes ---
void actionA();
void actionB();
void actionC();
void actionD();
void actionE();
void resetGame();

void setRandomInterval();
void triggerRandomAction();
void configureICM42688();
void readSensorData();
void checkButtons();

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("Cube World Firmware Starting...");

  randomSeed(analogRead(A0)); 

  // --- Initialize I2C, Accelerometer, and OLED ---
  Wire.begin(); // Start I2C bus on the XIAO RP2040 (SDA=GP6, SCL=GP7)
  
  configureICM42688();

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  
  // Clear the buffer and show the splash screen (default behavior)
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay(); // Clear the buffer after the splash screen
  Serial.println("OLED Display Initialized.");

  // --- Initialize Buttons ---
  pinMode(BUTTON_ADDPEEP_PIN, INPUT);
  pinMode(BUTTON_RESET_PIN, INPUT);

  setRandomInterval();
}

// --- Main Loop Function ---
void loop() {
  unsigned long currentMillis = millis();

  // Check for the timed action trigger
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    triggerRandomAction();
    setRandomInterval();
  }
  
  readSensorData();
  checkButtons();
  
  delay(50);
}

// --- Helper Functions ---

// Sets a new random interval for the next action.
void setRandomInterval() {
  interval = random(5000, 10001);
  Serial.print("Next action in: ");
  Serial.print(interval);
  Serial.println(" ms");
}

// Triggers one of the defined action functions randomly.
void triggerRandomAction() {
  int actionChoice = random(0, 5);

  Serial.print("Triggering action: ");
  switch (actionChoice) {
    case 0:
      Serial.println("Action A!");
      actionA();
      break;
    case 1:
      Serial.println("Action B!");
      actionB();
      break;
    case 2:
      Serial.println("Action C!");
      actionC();
      break;
    case 3:
      Serial.println("Action D!");
      actionD();
      break;
    case 4:
      Serial.println("Action E!");
      actionE();
      break;
    default:
      Serial.println("Unknown action choice!");
      break;
  }
}

// Manually configures the ICM-42688-P via I2C register writes.
void configureICM42688() {
  Serial.println("Configuring ICM42688-P...");
  
  // Power up the sensor and set to low-noise mode for accel and gyro
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(PWR_MGMT0); // Register address
  Wire.write(PWR_MGMT0_MODE_LN); // Data to write
  Wire.endTransmission();
  delay(100); // Give the sensor time to power up

  // Configure accelerometer range and data rate
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(ACCEL_CONFIG0); // Register address
  Wire.write(ACCEL_CONFIG0_RATE_12_5_HZ | ACCEL_CONFIG0_RANGE_4G); // Data to write
  Wire.endTransmission();
  delay(100);

  // Configure gyroscope range and data rate
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(GYRO_CONFIG0); // Register address
  Wire.write(GYRO_CONFIG0_RATE_12_5_HZ | GYRO_CONFIG0_RANGE_500DPS); // Data to write
  Wire.endTransmission();
  delay(100);

  Serial.println("ICM42688-P Configuration Complete!");
}

// Manually reads the accelerometer and gyroscope data from the sensor.
void readSensorData() {
  // Raw 16-bit values from sensor
  int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;

  // Start a read sequence
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(TEMP_DATA0); // Start reading from the Temp data register
  Wire.endTransmission(false); // Do not send a stop bit, this is a repeated start condition
  
  // Request 14 bytes of data (2 for temp, 6 for accel, 6 for gyro)
  Wire.requestFrom(ICM_ADDR, 14);

  if (Wire.available() == 14) {
    // Read and discard temp data (2 bytes)
    Wire.read(); 
    Wire.read();
    
    // Read Accel X (high byte, then low byte)
    accelX = Wire.read() << 8 | Wire.read();
    // Read Accel Y
    accelY = Wire.read() << 8 | Wire.read();
    // Read Accel Z
    accelZ = Wire.read() << 8 | Wire.read();

    // Read Gyro X
    gyroX = Wire.read() << 8 | Wire.read();
    // Read Gyro Y
    gyroY = Wire.read() << 8 | Wire.read();
    // Read Gyro Z
    gyroZ = Wire.read() << 8 | Wire.read();

    // --- Convert raw values to physical units ---
    // The scale factor for +/- 4G range is 16384 LSB/g
    float accelX_g = accelX / 16384.0;
    float accelY_g = accelY / 16384.0;
    float accelZ_g = accelZ / 16384.0;
    
    // The scale factor for +/- 500 DPS range is 65.5 LSB/dps
    float gyroX_dps = gyroX / 65.5;
    float gyroY_dps = gyroY / 65.5;
    float gyroZ_dps = gyroZ / 65.5;

    // Print acceleration data
    Serial.print("Accel X: ");
    Serial.print(accelX_g, 2);
    Serial.print(" g, Y: ");
    Serial.print(accelY_g, 2);
    Serial.print(" g, Z: ");
    Serial.print(accelZ_g, 2);
    Serial.println(" g");

    // Print gyroscope data
    Serial.print("Gyro X: ");
    Serial.print(gyroX_dps, 2);
    Serial.print(" dps, Y: ");
    Serial.print(gyroY_dps, 2);
    Serial.print(" dps, Z: ");
    Serial.print(gyroZ_dps, 2);
    Serial.println(" dps");

  } else {
    Serial.println("I2C read error!");
  }
}

// Checks the state of the buttons and performs actions.
void checkButtons() {
  // Debouncing is not included here for simplicity, but is recommended for real-world applications.
  if (digitalRead(BUTTON_ADDPEEP_PIN) == HIGH) {
    Serial.println("AddPeep button pressed!");
    triggerRandomAction();
    // A small delay to prevent rapid-fire triggering while the button is held down
    delay(200); 
  }
  
  if (digitalRead(BUTTON_RESET_PIN) == HIGH) {
    Serial.println("Reset button pressed!");
    resetGame();
    // A small delay to prevent rapid-fire triggering while the button is held down
    delay(200);
  }
}

// --- Action Functions (Empty Placeholders) ---
void actionA() {}
void actionB() {}
void actionC() {}
void actionD() {}
void actionE() {}

// --- Game Logic Functions ---
void resetGame() {
  Serial.println("--- Resetting Game State ---");
  // Your game reset logic will go here.
  // This could involve:
  // - Clearing the OLED display
  // - Resetting game variables (e.g., score, level)
  // - Re-initializing game objects
}
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

unsigned long previousMillis = 0;
long interval = 0;

const int ICM_ADDR = 0x68;

const int PWR_MGMT0 = 0x4E;
const int PWR_MGMT0_MODE_LN = 0b00000011;

const int ACCEL_CONFIG0 = 0x50;
const int ACCEL_CONFIG0_RANGE_4G = 0b00000100;
const int ACCEL_CONFIG0_RATE_12_5_HZ = 0b01100000;

const int GYRO_CONFIG0 = 0x4F;
const int GYRO_CONFIG0_RANGE_500DPS = 0b00001000;
const int GYRO_CONFIG0_RATE_12_5_HZ = 0b01100000;

const int TEMP_DATA0 = 0x1F;

const int BUTTON_ADDPEEP_PIN = 27;
const int BUTTON_RESET_PIN = 26;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
const int OLED_I2C_ADDR = 0x3C;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum GameState {
  IDLE,
  BUG_SCENARIO_WAIT,
  BUG_SCENARIO_STOMP,
  DOG_SCENARIO_LICK_WALK,
  DOG_SCENARIO_BITE_RUNIN,
  DOG_SCENARIO_BITE_BITTEN,
  DOG_SCENARIO_BITE_RUNOUT,
  DOG_SCENARIO_BITE_RUNBACKIN,
  DOG_SCENARIO_BITE_JUMP,
  PERSON_SCENARIO_RUNIN,
  PERSON_SCENARIO_FIGHT,
  PERSON_SCENARIO_DRAG,
  PERSON_SCENARIO_FRIENDS
};
GameState currentGameState = IDLE;

int characterX = SCREEN_WIDTH / 2;
int characterY = 40;
int characterDirection = 1;

int bugX, bugY;
int dogX, dogY;
int person2X, person2Y;

int punchCount = 0;
const int MAX_PUNCHES = 10;
const int MIN_PUNCHES = 5;

float tiltAngle = 0.0;
int tiltRating = 0;
float gravity = 0.0;
int gravityX = 0;
int gravityY = 1;

void drawCharacter(int x, int y, int direction, bool hasBiteMarks);
void drawMeditating();
void drawReading();
void drawThinking();
void drawWaving();
void drawLiftingWeights();
void drawBug(int x, int y);
void drawDog(int x, int y);
void drawPerson2(int x, int y, bool isKnockedOut);
void drawStompEffect(int x, int y);
void drawDogLick(int x, int y, int direction);
void drawCharacterHoldingDog(int x, int y, int direction);

void handleBugScenario();
void handleDogLickScenario();
void handleDogBiteScenario();
void handlePersonFightScenario();
void handlePersonFriendsScenario();
void resetGame();

void setRandomInterval();
void triggerRandomAction();
void configureICM42688();
void readSensorData();
void checkButtons();
void handleGravity();

void actionA();
void actionB();
void actionC();
void actionD();
void actionE();

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("Cube World Firmware Starting...");

  randomSeed(analogRead(A0));

  Wire.begin();
  configureICM42688();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);

  pinMode(BUTTON_ADDPEEP_PIN, INPUT);
  pinMode(BUTTON_RESET_PIN, INPUT);

  drawCharacter(characterX, characterY, characterDirection, false);
  Serial.println("OLED Display Initialized.");
  setRandomInterval();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentGameState == IDLE) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      triggerRandomAction();
      setRandomInterval();
    }
  }

  switch (currentGameState) {
    case BUG_SCENARIO_WAIT:
    case BUG_SCENARIO_STOMP:
      handleBugScenario();
      break;
    case DOG_SCENARIO_LICK_WALK:
      handleDogLickScenario();
      break;
    case DOG_SCENARIO_BITE_RUNIN:
    case DOG_SCENARIO_BITE_BITTEN:
    case DOG_SCENARIO_BITE_RUNOUT:
    case DOG_SCENARIO_BITE_RUNBACKIN:
    case DOG_SCENARIO_BITE_JUMP:
      handleDogBiteScenario();
      break;
    case PERSON_SCENARIO_RUNIN:
    case PERSON_SCENARIO_FIGHT:
    case PERSON_SCENARIO_DRAG:
      handlePersonFightScenario();
      break;
    case PERSON_SCENARIO_FRIENDS:
      handlePersonFriendsScenario();
      break;
    default:
      break;
  }

  readSensorData();
  handleGravity();
  checkButtons();

  delay(50);
}

void setRandomInterval() {
  interval = random(5000, 10001);
  Serial.print("Next action in: ");
  Serial.print(interval);
  Serial.println(" ms");
}

void triggerRandomAction() {
  int actionChoice = random(0, 5);
  Serial.print("Triggering random action: ");
  switch (actionChoice) {
    case 0:
      Serial.println("Action A: Meditating");
      actionA();
      break;
    case 1:
      Serial.println("Action B: Reading");
      actionB();
      break;
    case 2:
      Serial.println("Action C: Thinking");
      actionC();
      break;
    case 3:
      Serial.println("Action D: Waving");
      actionD();
      break;
    case 4:
      Serial.println("Action E: Lifting Weights");
      actionE();
      break;
    default:
      Serial.println("Initial Pose");
      drawCharacter(characterX, characterY, characterDirection, false);
      break;
  }
}

void configureICM42688() {
  Serial.println("Configuring ICM42688-P...");
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(PWR_MGMT0);
  Wire.write(PWR_MGMT0_MODE_LN);
  Wire.endTransmission();
  delay(100);

  Wire.beginTransmission(ICM_ADDR);
  Wire.write(ACCEL_CONFIG0);
  Wire.write(ACCEL_CONFIG0_RATE_12_5_HZ | ACCEL_CONFIG0_RANGE_4G);
  Wire.endTransmission();
  delay(100);

  Wire.beginTransmission(ICM_ADDR);
  Wire.write(GYRO_CONFIG0);
  Wire.write(GYRO_CONFIG0_RATE_12_5_HZ | GYRO_CONFIG0_RANGE_500DPS);
  Wire.endTransmission();
  delay(100);
  Serial.println("ICM42688-P Configuration Complete!");
}

void readSensorData() {
  int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(TEMP_DATA0);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, 14);

  if (Wire.available() == 14) {
    Wire.read();
    Wire.read();
    accelX = Wire.read() << 8 | Wire.read();
    accelY = Wire.read() << 8 | Wire.read();
    accelZ = Wire.read() << 8 | Wire.read();
    gyroX = Wire.read() << 8 | Wire.read();
    gyroY = Wire.read() << 8 | Wire.read();
    gyroZ = Wire.read() << 8 | Wire.read();

    float accelX_g = accelX / 16384.0;
    float accelY_g = accelY / 16384.0;
    float accelZ_g = accelZ / 16384.0;

    tiltAngle = atan2(-accelX_g, accelY_g) * 180.0 / PI;

    if (tiltAngle < 0) {
      tiltAngle += 360;
    }

    tiltRating = floor(tiltAngle / 10.0);
    if (tiltRating > 36) {
        tiltRating = 36;
    }
    
    if (tiltRating < 3) {
      gravity = 0.0;
    } else {
      gravity = (float)(tiltRating - 2) / 8.0; 
      if (gravity > 1.0) gravity = 1.0;
    }

    if (tiltAngle >= 315 || tiltAngle < 45) {
        gravityX = 0; gravityY = 1;
    } else if (tiltAngle >= 45 && tiltAngle < 135) {
        gravityX = 1; gravityY = 0;
    } else if (tiltAngle >= 135 && tiltAngle < 225) {
        gravityX = 0; gravityY = -1;
    } else if (tiltAngle >= 225 && tiltAngle < 315) {
        gravityX = -1; gravityY = 0;
    }

    Serial.print("Tilt Angle: "); Serial.print(tiltAngle);
    Serial.print(" degrees, Tilt Rating: "); Serial.print(tiltRating);
    Serial.print(", Gravity: "); Serial.println(gravity, 2);
  } else {
    Serial.println("I2C read error!");
  }
}

void handleGravity() {
    if (gravity > 0) {
        characterX += gravity * gravityX;
        characterY += gravity * gravityY;
        
        if (characterX < 0) characterX = 0;
        if (characterX > SCREEN_WIDTH-1) characterX = SCREEN_WIDTH-1;
        if (characterY < 0) characterY = 0;
        if (characterY > SCREEN_HEIGHT-1) characterY = SCREEN_HEIGHT-1;
    }
}

void checkButtons() {
  static unsigned long lastAddPeepPress = 0;
  static unsigned long lastResetPress = 0;
  const unsigned long debounceDelay = 50;

  if (digitalRead(BUTTON_ADDPEEP_PIN) == HIGH) {
    unsigned long now = millis();
    if (now - lastAddPeepPress > debounceDelay) {
      lastAddPeepPress = now;
      Serial.println("AddPeep button pressed! Starting a new scenario...");
      int scenarioChoice = random(0, 3);
      if (currentGameState != IDLE) {
          Serial.println("Cannot start new scenario, game is not IDLE.");
          return;
      }
      
      if (scenarioChoice == 0) {
          Serial.println("Spawning a bug!");
          int bugSpawnSide = random(0, 2);
          if (bugSpawnSide == 0) {
              bugX = -10;
              characterDirection = 1;
          } else {
              bugX = SCREEN_WIDTH + 10;
              characterDirection = -1;
          }
          bugY = SCREEN_HEIGHT - 5;
          currentGameState = BUG_SCENARIO_WAIT;
      } else if (scenarioChoice == 1) {
          Serial.println("Spawning a dog!");
          int dogSpawnSide = random(0, 2);
          if (dogSpawnSide == 0) {
              dogX = -15;
          } else {
              dogX = SCREEN_WIDTH + 15;
          }
          dogY = SCREEN_HEIGHT - 10;
          int dogInteraction = random(0, 2);
          if (dogInteraction == 0) {
              currentGameState = DOG_SCENARIO_LICK_WALK;
          } else {
              currentGameState = DOG_SCENARIO_BITE_RUNIN;
          }
      } else {
          Serial.println("Spawning a new person!");
          int personSpawnSide = random(0, 2);
          if (personSpawnSide == 0) {
              person2X = -10;
          } else {
              person2X = SCREEN_WIDTH + 10;
          }
          person2Y = 40;
          int personInteraction = random(0, 2);
          if (personInteraction == 0) {
              punchCount = random(MIN_PUNCHES, MAX_PUNCHES + 1);
              currentGameState = PERSON_SCENARIO_RUNIN;
          } else {
              currentGameState = PERSON_SCENARIO_FRIENDS;
          }
      }
    }
  }

  if (digitalRead(BUTTON_RESET_PIN) == HIGH) {
    unsigned long now = millis();
    if (now - lastResetPress > debounceDelay) {
      lastResetPress = now;
      Serial.println("Reset button pressed!");
      resetGame();
    }
  }
}

void drawCharacter(int x, int y, int direction, bool hasBiteMarks) {
  display.clearDisplay();

  display.drawCircle(x, y - 10, 6, SSD1306_WHITE);
  display.drawRect(x - 4, y - 4, 8, 18, SSD1306_WHITE);
  
  if (hasBiteMarks) {
    display.drawCircle(x - 2, y + 2, 1, SSD1306_BLACK);
    display.drawCircle(x + 2, y + 2, 1, SSD1306_BLACK);
    display.drawCircle(x - 2, y + 6, 1, SSD1306_BLACK);
    display.drawCircle(x + 2, y + 6, 1, SSD1306_BLACK);
  }

  display.drawLine(x, y - 2, x - 12, y + 8, SSD1306_WHITE);
  display.drawLine(x, y - 2, x + 12, y + 8, SSD1306_WHITE);

  display.drawLine(x - 2, y + 14, x - 2, y + 28, SSD1306_WHITE);
  display.drawLine(x + 2, y + 14, x + 2, y + 28, SSD1306_WHITE);

  display.drawLine(x - 5, y + 28, x + 1, y + 28, SSD1306_WHITE);
  display.drawLine(x + 1, y + 28, x + 7, y + 28, SSD1306_WHITE);
  display.display();
}

void resetGame() {
  Serial.println("--- Resetting Game State ---");
  currentGameState = IDLE;
  characterX = SCREEN_WIDTH / 2;
  characterY = 40;
  characterDirection = 1;
  drawCharacter(characterX, characterY, characterDirection, false);
}

void drawMeditating() {
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, 12, 6, SSD1306_WHITE);
  display.drawRect(58, 18, 12, 14, SSD1306_WHITE);
  display.drawLine(60, 26, 62, 32, SSD1306_WHITE);
  display.drawLine(68, 26, 66, 32, SSD1306_WHITE);
  display.drawCircle(64, 40, 8, SSD1306_WHITE);
  display.display();
}

void drawReading() {
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, 12, 6, SSD1306_WHITE);
  display.drawRect(60, 18, 8, 18, SSD1306_WHITE);
  display.drawLine(60, 24, 52, 32, SSD1306_WHITE);
  display.drawLine(68, 24, 76, 32, SSD1306_WHITE);
  display.drawRect(52, 32, 24, 16, SSD1306_WHITE);
  display.drawLine(62, 36, 62, 50, SSD1306_WHITE);
  display.drawLine(66, 36, 66, 50, SSD1306_WHITE);
  display.drawLine(59, 50, 65, 50, SSD1306_WHITE);
  display.drawLine(63, 50, 69, 50, SSD1306_WHITE);
  display.display();
}

void drawThinking() {
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, 12, 6, SSD1306_WHITE);
  display.drawRect(60, 18, 8, 18, SSD1306_WHITE);
  display.drawLine(64, 20, 52, 30, SSD1306_WHITE);
  display.drawLine(64, 20, 70, 20, SSD1306_WHITE);
  display.drawLine(70, 20, 70, 15, SSD1306_WHITE);
  display.drawCircle(78, 8, 4, SSD1306_WHITE);
  display.drawPixel(70, 12, SSD1306_WHITE);
  display.drawPixel(74, 10, SSD1306_WHITE);
  display.drawLine(62, 36, 62, 50, SSD1306_WHITE);
  display.drawLine(66, 36, 66, 50, SSD1306_WHITE);
  display.drawLine(59, 50, 65, 50, SSD1306_WHITE);
  display.drawLine(63, 50, 69, 50, SSD1306_WHITE);
  display.display();
}

void drawWaving() {
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, 12, 6, SSD1306_WHITE);
  display.drawRect(60, 18, 8, 18, SSD1306_WHITE);
  display.drawLine(64, 20, 52, 10, SSD1306_WHITE);
  display.drawLine(64, 20, 76, 30, SSD1306_WHITE);
  display.drawLine(62, 36, 62, 50, SSD1306_WHITE);
  display.drawLine(66, 36, 66, 50, SSD1306_WHITE);
  display.drawLine(59, 50, 65, 50, SSD1306_WHITE);
  display.drawLine(63, 50, 69, 50, SSD1306_WHITE);
  display.display();
}

void drawLiftingWeights() {
  display.clearDisplay();
  display.drawCircle(SCREEN_WIDTH / 2, 12, 6, SSD1306_WHITE);
  display.drawRect(60, 18, 8, 18, SSD1306_WHITE);
  display.drawLine(64, 20, 64, 30, SSD1306_WHITE);
  display.drawLine(64, 20, 64, 30, SSD1306_WHITE);
  display.drawLine(50, 30, 78, 30, SSD1306_WHITE);
  display.drawCircle(50, 30, 2, SSD1306_WHITE);
  display.drawCircle(78, 30, 2, SSD1306_WHITE);
  display.drawLine(62, 36, 62, 50, SSD1306_WHITE);
  display.drawLine(66, 36, 66, 50, SSD1306_WHITE);
  display.drawLine(59, 50, 65, 50, SSD1306_WHITE);
  display.drawLine(63, 50, 69, 50, SSD1306_WHITE);
  display.display();
}

void drawBug(int x, int y) {
  display.drawPixel(x, y, SSD1306_WHITE);
  display.drawPixel(x+1, y, SSD1306_WHITE);
  display.drawPixel(x-1, y, SSD1306_WHITE);
  display.drawPixel(x, y-1, SSD1306_WHITE);
  display.drawPixel(x, y+1, SSD1306_WHITE);
}

void drawDog(int x, int y) {
  display.fillRect(x, y, 10, 5, SSD1306_WHITE);
  display.fillRect(x+10, y, 3, 3, SSD1306_WHITE);
}

void drawPerson2(int x, int y, bool isKnockedOut) {
  if (isKnockedOut) {
    display.drawCircle(x, y + 10, 6, SSD1306_WHITE);
    display.drawRect(x - 10, y + 16, 18, 8, SSD1306_WHITE);
  } else {
    drawCharacter(x, y, -1, false);
  }
}

void drawStompEffect(int x, int y) {
  display.fillCircle(x, y, 5, SSD1306_WHITE);
}

void drawDogLick(int x, int y, int direction) {
  display.drawLine(x, y, x + (10 * direction), y, SSD1306_WHITE);
}

void drawCharacterHoldingDog(int x, int y, int direction) {
  display.clearDisplay();
  drawCharacter(x, y, direction, false);
  drawDog(x + (15 * direction), y + 10);
  display.display();
}

void actionA() {
  drawMeditating();
}

void actionB() {
  drawReading();
}

void actionC() {
  drawThinking();
}

void actionD() {
  drawWaving();
}

void actionE() {
  drawLiftingWeights();
}

void handleBugScenario() {
  static bool bugSpawned = false;
  static int bugDirection;
  
  if (!bugSpawned) {
    bugDirection = (bugX < SCREEN_WIDTH / 2) ? 1 : -1;
    characterDirection = bugDirection;
    bugSpawned = true;
    currentGameState = BUG_SCENARIO_WAIT;
  }
  
  if (currentGameState == BUG_SCENARIO_WAIT) {
    display.clearDisplay();
    drawCharacter(characterX, characterY, characterDirection, false);
    drawBug(bugX, bugY);
    display.display();
    
    bugX += bugDirection;
    
    if (abs(bugX - characterX) < 10) {
      currentGameState = BUG_SCENARIO_STOMP;
      previousMillis = millis();
    }
  }
  
  if (currentGameState == BUG_SCENARIO_STOMP) {
    display.clearDisplay();
    drawCharacter(characterX, characterY, characterDirection, false);
    drawStompEffect(bugX, bugY);
    display.display();
    
    if (millis() - previousMillis > 500) {
      bugSpawned = false;
      resetGame();
    }
  }
}

void handleDogLickScenario() {
  static int dogDirection = (dogX < SCREEN_WIDTH / 2) ? 1 : -1;
  static int phase = 0;
  static unsigned long lastUpdate = 0;
  const int updateInterval = 100;

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    if (phase == 0) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawDog(dogX, dogY);
      display.display();
      dogX += (5 * dogDirection);
      if (abs(dogX - characterX) < 20) {
        phase = 1;
        lastUpdate = millis();
      }
    } else if (phase == 1) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawDog(dogX, dogY);
      drawDogLick(dogX, dogY - 5, -dogDirection);
      display.display();
      if (millis() - lastUpdate > 1000) {
        phase = 2;
        lastUpdate = millis();
      }
    } else if (phase == 2) {
      dogDirection = -dogDirection;
      characterDirection = dogDirection;
      drawCharacterHoldingDog(characterX, characterY, characterDirection);
      characterX += (2 * characterDirection);
      dogX = characterX + (15 * characterDirection);
      if (characterX < -20 || characterX > SCREEN_WIDTH + 20) {
        phase = 3;
        lastUpdate = millis();
      }
    } else if (phase == 3) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      dogX += (10 * dogDirection);
      drawDog(dogX, dogY);
      display.display();
      if (dogX < -20 || dogX > SCREEN_WIDTH + 20) {
        phase = 4;
      }
    } else if (phase == 4) {
      phase = 0;
      resetGame();
    }
  }
}

void handleDogBiteScenario() {
  static int dogDirection = (dogX < SCREEN_WIDTH / 2) ? 1 : -1;
  static int phase = 0;
  static unsigned long lastUpdate = 0;
  const int updateInterval = 100;

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    if (phase == 0) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawDog(dogX, dogY);
      display.display();
      dogX += (5 * dogDirection);
      if (abs(dogX - characterX) < 10) {
        phase = 1;
        lastUpdate = millis();
      }
    } else if (phase == 1) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, true);
      drawDog(dogX, dogY);
      display.display();
      if (millis() - lastUpdate > 1000) {
        phase = 2;
      }
    } else if (phase == 2) {
      characterDirection = -dogDirection;
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, true);
      drawDog(dogX, dogY);
      display.display();
      characterX += (5 * characterDirection);
      dogX += (5 * characterDirection);
      if (characterX < -20 || characterX > SCREEN_WIDTH + 20) {
        phase = 3;
        lastUpdate = millis();
      }
    } else if (phase == 3) {
      characterX = (characterDirection == 1) ? -20 : SCREEN_WIDTH + 20;
      dogX = characterX + (10 * characterDirection);
      phase = 4;
      lastUpdate = millis();
    } else if (phase == 4) {
      display.clearDisplay();
      drawCharacter(characterX, characterY - 10, characterDirection, true);
      drawDog(dogX, dogY);
      display.display();
      dogX += (5 * characterDirection);
      if (abs(characterX - dogX) < 10) {
        phase = 5;
      }
    } else if (phase == 5) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, true);
      drawDog(dogX, dogY);
      display.display();
      dogX += (5 * characterDirection);
      if (dogX < -20 || dogX > SCREEN_WIDTH + 20) {
        phase = 6;
      }
    } else if (phase == 6) {
      phase = 0;
      resetGame();
    }
  }
}

void handlePersonFightScenario() {
  static int person2Direction;
  static int phase = 0;
  static unsigned long lastUpdate = 0;
  const int updateInterval = 100;

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    if (phase == 0) {
      person2Direction = (person2X < SCREEN_WIDTH / 2) ? 1 : -1;
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawPerson2(person2X, person2Y, false);
      display.display();
      person2X += (5 * person2Direction);
      if (abs(person2X - characterX) < 20) {
        phase = 1;
      }
    } else if (phase == 1) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawPerson2(person2X, person2Y, false);
      display.display();
      punchCount--;
      if (punchCount <= 0) {
        phase = 2;
        lastUpdate = millis();
      }
    } else if (phase == 2) {
      display.clearDisplay();
      drawPerson2(person2X, person2Y, false);
      drawPerson2(characterX, characterY, true);
      display.display();
      if (millis() - lastUpdate > 1000) {
        phase = 3;
      }
    } else if (phase == 3) {
      display.clearDisplay();
      drawPerson2(person2X, person2Y, false);
      drawPerson2(characterX, characterY, true);
      display.display();
      person2X += (5 * person2Direction);
      characterX += (5 * person2Direction);
      if (person2X < -20 || person2X > SCREEN_WIDTH + 20) {
        phase = 4;
      }
    } else if (phase == 4) {
      person2X = (person2Direction == 1) ? -20 : SCREEN_WIDTH + 20;
      display.clearDisplay();
      drawPerson2(person2X, person2Y, false);
      display.display();
      if (person2X > SCREEN_WIDTH / 2 - 10 && person2X < SCREEN_WIDTH / 2 + 10) {
        phase = 5;
        lastUpdate = millis();
      }
    } else if (phase == 5) {
      phase = 0;
      resetGame();
    }
  }
}

void handlePersonFriendsScenario() {
  static int person2Direction;
  static int phase = 0;
  static unsigned long lastUpdate = 0;
  const int updateInterval = 100;

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    if (phase == 0) {
      person2Direction = (person2X < SCREEN_WIDTH / 2) ? 1 : -1;
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawPerson2(person2X, person2Y, false);
      display.display();
      person2X += (5 * person2Direction);
      if (abs(person2X - characterX) < 20) {
        phase = 1;
        lastUpdate = millis();
      }
    } else if (phase == 1) {
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawPerson2(person2X, person2Y, false);
      display.display();
      if (millis() - lastUpdate > 500) {
        phase = 2;
        lastUpdate = millis();
      }
    } else if (phase == 2) {
      person2Direction = -person2Direction;
      display.clearDisplay();
      drawCharacter(characterX, characterY, characterDirection, false);
      drawPerson2(person2X, person2Y, false);
      display.display();
      person2X += (10 * person2Direction);
      if (person2X < -20 || person2X > SCREEN_WIDTH + 20) {
        phase = 3;
      }
    } else if (phase == 3) {
      phase = 0;
      resetGame();
    }
  }
}
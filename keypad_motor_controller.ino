// RBT173 - LAB5.2
// DAVID GARCIA
// DAVGARCI@UAT.EDU

// Keypad Conveyor Controller
// Controls:
//   0..9 = set speed level (0=stop)
//   #    = start/pause
//   A    = toggle direction (FWD/REV)
//   *    = stop & clear
//

#include <Keypad.h>              // matrix keypad support
#include <LiquidCrystal.h>       // LCD support

// ---- Pins (match your wiring) ----
const byte PIN_IN1 = 5;          // L293D IN1 (PWM) - forward drive
const byte PIN_IN2 = 6;          // L293D IN2 (PWM) - reverse drive
const byte PIN_R   = 9;          // RGB LED: Red channel (PWM)
const byte PIN_G   = 10;         // RGB LED: Green channel (PWM)

// LCD(rs, enable, d4, d5, d6, d7) — using 4-bit mode on A0..A3
LiquidCrystal lcd(12, 8, A0, A1, A2, A3);

// ---- Keypad wiring/map ----
const byte ROWS = 4, COLS = 4;
char KEYS[ROWS][COLS] = {         // layout matches physical keypad
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte ROW_PINS[ROWS] = {2, 3, 4, 7};       // R1..R4 - D2, D3, D4, D7
byte COL_PINS[COLS] = {11, 13, A4, A5};   // C1..C4 - D11, D13, A4, A5
Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);

// ---- State ----
bool running    = false;          // true = motor should spin
bool forwardDir = true;           // true = forward, false = reverse
byte speedLevel = 0;              // 0..9 user speed (0 means stop)

// ---- LED helpers ----
void ledRed()   { analogWrite(PIN_R, 255); analogWrite(PIN_G, 0);   } // show STOP
void ledGreen() { analogWrite(PIN_R, 0);   analogWrite(PIN_G, 255); } // show RUN
void ledOff()   { analogWrite(PIN_R, 0);   analogWrite(PIN_G, 0);   } // not used often

// ---- Motor helpers ----
void motorStop() {
  analogWrite(PIN_IN1, 0);        // no forward drive
  analogWrite(PIN_IN2, 0);        // no reverse drive
}

byte pwmFromLevel(byte lvl) {
  if (lvl == 0) return 0;         // level 0 = fully stopped
  return (byte)min(255, lvl * 28);// map 1..9 - ~28..252 (simple linear map)
}

void applyMotor() {
  byte pwm = pwmFromLevel(speedLevel);  // convert level to PWM duty

  if (!running || pwm == 0) {     // paused or zero speed
    motorStop();
    return;
  }

  if (forwardDir) {               // forward: drive IN1, hold IN2 low
    analogWrite(PIN_IN2, 0);
    analogWrite(PIN_IN1, pwm);
  } else {                        // reverse: drive IN2, hold IN1 low
    analogWrite(PIN_IN1, 0);
    analogWrite(PIN_IN2, pwm);
  }
}

// ---- LCD helpers ----
void print2(const char* l1, const char* l2) {
  lcd.clear();                    // clear previous text
  lcd.setCursor(0,0); lcd.print(l1);
  lcd.setCursor(0,1); lcd.print(l2);
}

void showStatus() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(running ? "RUN " : "STOP");     // show run/stop
  lcd.print(forwardDir ? " FWD " : " REV "); // show direction
  lcd.print("SPD:"); lcd.print(speedLevel); // show speed level
  lcd.setCursor(0,1);
  lcd.print("0-9 spd  # start  A dir");     // quick help line
}

// ---- Actions (called from key handling) ----
void setSpeed(byte lvl) {
  speedLevel = (lvl > 9) ? 9 : lvl;   // clamp to 0..9
  if (speedLevel == 0) running = false; // choosing 0 also pauses
  applyMotor();                        // push state to hardware
  if (running) ledGreen(); else ledRed();
  showStatus();
}

void toggleRun() {
  running = !running;                  // flip run/pause
  if (running && speedLevel == 0) {    // if starting at 0, pick a default low speed
    speedLevel = 3;
  }
  applyMotor();
  if (running) ledGreen(); else ledRed();
  showStatus();
}

void toggleDir() {
  forwardDir = !forwardDir;            // flip direction
  applyMotor();                        // reapply PWM on the other input
  showStatus();
}

void hardStop() {
  running = false;                     // force stop
  speedLevel = 0;                      // clear speed
  motorStop();                         // cut both inputs
  ledRed();                            // show stopped
  showStatus();
}

// ---- Arduino setup/loop ----
void setup() {
  pinMode(PIN_IN1, OUTPUT);            // motor input pins as outputs
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_R,   OUTPUT);            // RGB channels as PWM outputs
  pinMode(PIN_G,   OUTPUT);

  motorStop();                         // start safe
  ledRed();                            // indicate STOP at power-up

  lcd.begin(16,2);                     // init 16x2 LCD
  showStatus();                        // draw initial screen
}

void loop() {
  char k = keypad.getKey();            // key read
  if (!k) return;                      // no key - nothing to do

  if (k >= '0' && k <= '9') {          // speed selection
    setSpeed(k - '0');                 // convert ASCII digit to number 0..9
    return;
  }
  if (k == '#') {                      // start/pause toggle
    toggleRun();
    return;
  }
  if (k == 'A') {                      // direction toggle
    toggleDir();
    return;
  }
  if (k == '*') {                      // emergency stop / clear
    hardStop();
    return;
  }
}

#include <AFMotor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "GM65_scanner.h"

const auto hardware = []() {
  struct {
    LiquidCrystal_I2C lcd{0x27, 16, 2};
    AF_DCMotor motors[4]{
      AF_DCMotor(1, MOTOR12_1KHZ),
      AF_DCMotor(2, MOTOR12_1KHZ),
      AF_DCMotor(3, MOTOR34_1KHZ),
      AF_DCMotor(4, MOTOR34_1KHZ)
    };

      const char keys[4][4] = {
      {'1','2','3','A'},
      {'4','5','6','B'},
      {'7','8','9','C'},
      {'*','0','#','D'}
    };
    byte rowPins[4] = {44, 42, 40, 38};
    byte colPins[4] = {36, 34, 32, 30};

    Keypad keypad = Keypad(makeKeymap(keys),   
      rowPins,
      colPins,
      4,
      4
    );

    GM65_scanner scanner{&Serial1};
  } hw;

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  Serial1.begin(9600);

  hw.lcd.init();
  hw.lcd.backlight();

  hw.scanner.init();
  hw.scanner.enable_setting_code();
  return hw;
}();

auto state = []() {
  struct {
    const String barcodes[8] = {
      "", 
      "3857291046", 
      "7902431857", 
      "6415072389", 
      "1246893057", 
      "9083761542", 
      "3569817204", 
      "7024189365"  
    };
    int selected = 0;
    String target = "";
    String scanned = "";
    enum {IDLE, SEARCHING, FOUND} status_enum;
    int status = IDLE;

    hardware.lcd.print("Select Item 1-7");
  } s;
  return s;
}();
  void updateLCD(String l1, String l2 = "") {
    hardware.lcd.clear();
    hardware.lcd.setCursor(0,0);
    hardware.lcd.print(l1);
    if (l2 != "") {
      hardware.lcd.setCursor(0,1);
      hardware.lcd.print(l2);
    }
  }

  void controlMotors(int m1, int m2, int m3, int m4) {
    for (int i=0; i<4; i++) {
      hardware.motors[i].run((i==0)?m1:(i==1)?m2:(i==2)?m3:m4);
      hardware.motors[i].setSpeed(80);
    }
  }
  void handleInput(char key) {
    if (!key) return;

    if (state.status == state.IDLE && key >= '1' && key <= '7') {
      state.selected = key - '0';
      state.target = state.barcodes[state.selected];
      updateLCD("Item Selected:", state.target);
      delay(1500);
      updateLCD("Searching...");
      state.status = state.SEARCHING;
    }

    else if (key == 'D') {
      controlMotors(RELEASE, RELEASE, RELEASE, RELEASE);
      state.status = state.IDLE;
      state.target = "";
      updateLCD("Select Item 1-7");
    }
    else if (key == 'C' && state.status == state.SEARCHING) {
      controlMotors(RELEASE, RELEASE, RELEASE, RELEASE);
      updateLCD("Paused");
    }
  }
  void followLine() {
    int left = digitalRead(A0);
    int right = digitalRead(A1);
    if (!left && !right) {
      controlMotors(FORWARD, FORWARD, FORWARD, FORWARD);
    } else if (!left && right) {
      controlMotors(FORWARD, FORWARD, BACKWARD, BACKWARD);
    } else if (left && !right) {
      controlMotors(BACKWARD, BACKWARD, FORWARD, FORWARD);
    } else {
      controlMotors(RELEASE, RELEASE, RELEASE, RELEASE);
    }
  }

  void checkBarcode() {
    state.scanned = hardware.scanner.get_info();
    state.scanned.trim();
    if (state.scanned.length() && state.scanned == state.target) {
      state.status = state.FOUND;
      updateLCD("Item Found!", state.scanned);
    }
  }
}

void setup() {
}

void loop() {
  Robot::handleInput(hardware.keypad.getKey());
  if (state.status == state.SEARCHING) {
    Robot::followLine();
    Robot::checkBarcode();
  }
}

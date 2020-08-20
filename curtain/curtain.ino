// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
#define INIT_KEY 13

#define MOTOR_COUNT 2

// Дребезг, случайные замыкания
#define button_debounce 20
// Клик
#define button_hold 1000

#include <EEPROM.h>
#include "IRremote.h"
#include "Motor.h"

const int ir_pin = A0;
enum irEvent {Close, Open, Stop, MenuSwitch, MotorSwitch};
const unsigned long minus = 16769055;
const unsigned long pause = 16761405;
const unsigned long plus = 16754775;
const unsigned long next = 16712445;
const unsigned long prev = 16720605;
const unsigned long zero = 16738455;

IRrecv irrecv(ir_pin);
decode_results results;

enum motorActiveState {First, Second, Both};

motorActiveState mtrActiveState = Both;
// События
enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitSleep};

enum buttonState {Sleep, PreClick, Click, Hold, LongHold, ForcedSleep};

enum buttonState btnState = Sleep;

unsigned long pressTimestamp;

enum motorManagerMode mtrMngMode = Auto;

// Указываем пины, к которым подключен драйвера шаговых двигателей
Motor firstMotor(Left, 8, 9, 10, 11);

Motor secondMotor(Right, 4, 5, 6, 7);

Motor motorList[] = {firstMotor, secondMotor};

void printStructList() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Serial.print(i+1);
    Serial.print(" Motor:\n");
    motorList[i].print();
  }
}

int btPin = 13;
int piezoPin = 2;
int FIRST_LED = 12;
int SECOND_LED = 3;

void ledOn(int ledPin) {
  digitalWrite(ledPin, HIGH);
}

void ledOff(int ledPin) {
  digitalWrite(ledPin, LOW);
}

//короткий звук
void beep() {
  tone(piezoPin, 800, 100);
}

//длинный звук
void longBeep() {
  tone(piezoPin, 1000, 500);
}

void setup() {
  Serial.begin(9600);
  Serial.print("Setup\n");

  pinMode(piezoPin, OUTPUT);
  pinMode(FIRST_LED, OUTPUT);
  pinMode(SECOND_LED, OUTPUT);

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);

  bool firstInit = false;
  if (EEPROM.read(INIT_ADDR) != INIT_KEY) {
    Serial.print("First init\n");
    // Записали ключ
    EEPROM.write(INIT_ADDR, INIT_KEY);
    firstInit = true;
  }

  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].initData(firstInit, i);
    motorList[i].initStepper();
  }

  mtrActiveState = getCurrentActiveState();
  setMotorActivities();

  printStructList();
}

void saveAllData() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].saveData();
  }
}

void loop() {
  controlManagerLoop();
  motorManagerLoop();
  releaseMotorData();
}

void motorManagerLoop() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].loop(mtrMngMode);
  }
}

void releaseMotorData() {
  bool needSave = motorList[0].needSave || motorList[1].needSave;

  if (needSave) {
    motorState firstCurState = motorList[0].getCurState();
    motorState firstPrevState = motorList[0].getPrevState();
    motorState secondCurState = motorList[1].getCurState();
    motorState secondPrevState = motorList[1].getPrevState();
    if (firstCurState == Idle && firstPrevState == firstCurState
      && secondCurState == Idle && secondPrevState == secondCurState) {
        Serial.print("Save ALL DATA\n");
        saveAllData();
    }
  }
}

void doEvent(enum irEvent e, Motor *mtr) {
  switch (e) {
    case Close: {
      if (mtr->getCurState() != Down) {
        mtr->changeState(Down);
        Serial.print("Close\n");
        beep();
      }
      break;
    }
    case Open: {
      if (mtr->getCurState() != Up) {
        mtr->changeState(Up);
        Serial.print("Open\n");
        beep();
      }
      break;
    }
    case Stop: {
      if (mtr->getCurState() != Idle) {
        mtr->changeState(Idle);
        Serial.print("Stop\n");
        beep();
      }
      break;
    }
    case MenuSwitch: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Calibration;
        for (int i = 0; i < MOTOR_COUNT; i++) {
          if (motorList[i].getCurState() != Idle) {
            doEvent(Stop, &motorList[i]);
          }
        }
        mtrActiveState = First;
        setMotorActivities();
        Serial.print("Calibration mode\n");
      } else if (mtrMngMode == Calibration) {
        mtrMngMode = Auto;
        Serial.print("Auto mode\n");
      }
      longBeep();
      break;
    }
    case MotorSwitch: {
      switchMotors();
      Serial.print("Motor switched\n");
      printStructList();
      break;
    }
  }
}

void switchMotors() {
  if(mtrMngMode == Auto) {
    if (mtrActiveState == First) {
      mtrActiveState = Second;
    } else if (mtrActiveState == Second) {
      mtrActiveState = Both;
    } else if (mtrActiveState == Both) {
      mtrActiveState = First;
    }
  } else if (mtrMngMode == Calibration) {
    if (mtrActiveState == First) {
      mtrActiveState = Second;
    } else if (mtrActiveState == Second) {
      mtrActiveState = First;
    }
  }
  setMotorActivities();
  if (mtrMngMode == Auto) {
    saveAllData();
  }
}

void setMotorActivities() {
  switch (mtrActiveState) {
    case First:
      motorList[0].setActive(1);
      motorList[1].setActive(0);
      ledOn(FIRST_LED);
      ledOff(SECOND_LED);
      break;
    case Second:
      motorList[0].setActive(0);
      motorList[1].setActive(1);
      ledOff(FIRST_LED);
      ledOn(SECOND_LED);
      break;
    case Both:
      motorList[0].setActive(1);
      motorList[1].setActive(1);
      ledOn(FIRST_LED);
      ledOn(SECOND_LED);
      break;
  }
}

motorActiveState getCurrentActiveState() {
  bool firstActive = motorList[0].isActive();
  bool secondActive = motorList[1].isActive();

  if (firstActive && secondActive) {
    return Both;
  } else if (firstActive) {
    return First;
  } else if (secondActive) {
    return Second;
  }
}

void controlManagerLoop() {
  irControl();
  buttonControl();
}

void irControl() {
  if (irrecv.decode(&results)) {
    // Serial.print(results.value);
    // Serial.print("\n");
    switch (results.value) {
      case next:
        doEvent(MenuSwitch, &motorList[0]);
        break;
      case prev:
        doEvent(MotorSwitch, &motorList[0]);
        break;
      case zero:
        printStructList();
        break;
      default: {
        for (int i = 0; i < MOTOR_COUNT; i++) {
          controlLoop(results.value, &motorList[i]);
        }
      }
    }
    irrecv.resume();
  }
}

void controlLoop(unsigned long value, Motor *mtr) {
  if (mtr->isActive()) {
    switch (value) {
      case plus:
        mtr->getCurState() == Up ? doEvent(Stop, mtr) : doEvent(Open, mtr);
        break;
      case minus:
        mtr->getCurState() == Down ? doEvent(Stop, mtr) : doEvent(Close, mtr);
        break;
      case pause:
        doEvent(Stop, mtr);
        break;
    }
  }
}

void buttonControl() {
  unsigned long mls = millis();

  if (digitalRead(btPin)) {
    doButtonEvent(Press);
  } else {
    doButtonEvent(Release);
  }

  if (mls - pressTimestamp > button_debounce) {
    doButtonEvent(WaitDebounce);
  }
  if (mls - pressTimestamp > button_hold) {
    doButtonEvent(WaitHold);
  }
}

void doButtonEvent(enum event e) {
  switch (e) {
    case Press:
      if (btnState == Sleep) {
        btnState =  PreClick;
        pressTimestamp = millis();
      }
      break;
    case Release:
      onClick(btnState);
      btnState = Sleep;
      break;
    case WaitDebounce:
      if (btnState == PreClick) {
        btnState = Click;
      }
      break;
    case WaitHold:
      if (btnState == Click) {
        btnState = Hold;
      }
      break;
  }
}

void onClick(enum buttonState s) {
  switch (s) {
    case Click:
      Serial.print("Click\n");
      doEvent(MotorSwitch, &motorList[0]);
      break;
    case Hold:
      Serial.print("Hold\n");
      doEvent(MenuSwitch, &motorList[0]);
      break;
  }
}

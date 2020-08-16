// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
#define INIT_KEY 6

#define MOTOR_COUNT 2

// // Дребезг, случайные замыкания
// #define button_debounce 20
// // Клик
// #define button_hold 250
// // Долгое нажатие
// #define button_long 3000
// // Ошибка, на кнопку сели
// #define button_idle 5000

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
// enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitIdle};

// enum buttonState {Idle, PreClick, Click, Hold, LongHold, ForcedIdle};

// enum buttonState btnState = Idle;

// unsigned long pressTimestamp;

// enum motorManagerMode {Auto, Calibration};

enum motorManagerMode mtrMngMode = Auto;

// Указываем пины, к которым подключен драйвера шаговых двигателей
Motor firstMotor(8, 9, 10, 11);

Motor secondMotor(4, 5, 6, 7);

Motor motorList[] = {firstMotor, secondMotor};

void printStructList() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Serial.print(i+1);
    Serial.print(" Motor:\n");
    motorList[i].print();
  }
}

void setup() {
  Serial.begin(9600);
  Serial.print("Setup\n");

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

  printStructList();

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);
}

void loop() {
  controlManagerLoop();
  motorManagerLoop();
}

void motorManagerLoop() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].loop(mtrMngMode);
  }
}


void doEvent(enum irEvent e, Motor *mtr) {
  switch (e) {
    case Close: {
      if (mtr->getCurState() != Down) {
        mtr->changeState(Down);
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (mtr->getCurState() != Up) {
        mtr->changeState(Up);
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (mtr->getCurState() != Idle) {
        mtr->changeState(Idle);
        Serial.print("Stop\n");
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
        Serial.print("Calibration mode\n");
      } else if (mtrMngMode == Calibration) {
        mtrMngMode = Auto;
        Serial.print("Auto mode\n");
      }
      break;
    }
    case MotorSwitch: {
      // if (motorList[i].getCurState() != Idle) {
      //   continue;
      // }
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
      motorList[0].setActive(0);
      motorList[1].setActive(1);
    } else if (mtrActiveState == Second) {
      mtrActiveState = Both;
      motorList[0].setActive(1);
      motorList[1].setActive(1);
    } else if (mtrActiveState == Both) {
      mtrActiveState = First;
      motorList[0].setActive(1);
      motorList[1].setActive(0);
    }
  } else if (mtrMngMode == Calibration) {
    if (mtrActiveState == First) {
      mtrActiveState = Second;
      motorList[0].setActive(0);
      motorList[1].setActive(1);
    } else if (mtrActiveState == Second) {
      mtrActiveState = First;
      motorList[0].setActive(1);
      motorList[1].setActive(0);
    }
  }
}

void controlManagerLoop() {
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
  // unsigned long mls = millis();

  // if (digitalRead(btPin)) {
  //   doEvent(Press);
  // } else {
  //   doEvent(Release);
  // }

  // if (mls - pressTimestamp > button_debounce) {
  //   doEvent(WaitDebounce);
  // }
  // if (mls - pressTimestamp > button_hold) {
  //   doEvent(WaitHold);
  // }
  // if (mls - pressTimestamp > button_long) {
  //   doEvent(WaitLongHold);
  // }
  // if (mls - pressTimestamp > button_idle) {
  //   doEvent(WaitIdle);
  // }
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

// void doEvent(enum event e) {
//   switch (e) {
//     case Press:
//       if (btnState == Idle) {
//         btnState =  PreClick;
//         pressTimestamp = millis();
//       }
//       break;
//     case Release:
//       onClick(btnState);
//       btnState = Idle;
//       break;
//     case WaitDebounce:
//       if (btnState == PreClick) {
//         btnState = Click;
//       }
//       break;
//     case WaitHold:
//       if (btnState == Click) {
//         btnState = Hold;
//       }
//       break;
//     case WaitLongHold:
//       if (btnState == Hold) {
//         btnState = LongHold;
//       }
//       break;
//     case WaitIdle:
//       if (btnState == LongHold) {
//         // btnState = Idle;
//       }
//       break;
//   }
// }

// void onClick(enum buttonState s) {
//   switch (s) {
//     case Click:
//       firstMtr.curState = firstMtr.curState == RunCCW ? RunCW : RunCCW;
//       stepper.setDirection(STOP);
//       Serial.print("Click\n");
//       break;
//     case Hold:
//       firstMtr.curState = Stop;
//       Serial.print("Hold\n");
//       break;
//     case LongHold:
//       Serial.print("LongHold\n");
//       break;
//     default:
//       break;
//   }
// }

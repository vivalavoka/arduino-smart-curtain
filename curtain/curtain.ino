// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
// #define INIT_KEY 253
#define INIT_KEY 254

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

// События
// enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitIdle};

// enum buttonState {Idle, PreClick, Click, Hold, LongHold, ForcedIdle};

// enum buttonState btnState = Idle;

// unsigned long pressTimestamp;

enum motorManagerMode {Auto, Calibration};

enum motorManagerMode mtrMngMode = Auto;

byte pins[4] = {8, 9, 10, 11};
// Указываем пины, к которым подключен драйвера шаговых двигателей
Motor firstMotor(pins);

void printStructList() {
  Serial.print("First Motor:\n");
  firstMotor.print();
}

void setup() {
  Serial.begin(9600);
  Serial.print("Setup\n");

  firstMotor.initData();

  printStructList();

  firstMotor.initStepper();

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);
}

void loop() {
  controlManagerLoop();
  motorManagerLoop();
}

void motorManagerLoop() {
    firstMotor.loop();
    // if (mtrMngMode == Calibration) {
    //   // calibrationLoop(&motorList[i], &stepperList[i]);
    // } else if (mtrMngMode == Auto) {
    //   autoLoop();
    // }
}

void calibrationLoop(MotorStruct *mtr, CustomStepper *stepper) {
  // if ((mtr->curState == Idle && mtr->prevState == Idle) || !mtr->active) {
  //   return;
  // }

  // if (stepper->isDone()) {
  //   // Для разовой подачи команды на смену направления
  //   if (mtr->curState == Up && mtr->prevState != Up) {
  //     stepper->setDirection(_UP);
  //     mtr->prevState = mtr->curState;
  //   } else if (mtr->curState == Down && mtr->prevState != Down) {
  //     stepper->setDirection(_DOWN);
  //     mtr->curPosition = MIN_POSITION;

  //     mtr->prevState = mtr->curState;
  //   } else if (mtr->curState == Idle && mtr->prevState != Idle) {
  //     stepper->setDirection(STOP);
  //     if (mtr->prevState == Down) {
  //       mtr->maxPosition = mtr->curPosition;
  //       Serial.print("Save max turnover: ");
  //       Serial.print(mtr->maxPosition);
  //       Serial.print("\n");

  //       EEPROM.put((int)&motorList_addr, motorList);

  //       doEvent(MenuSwitch, mtr);
  //     }
  //     mtr->prevState = mtr->curState;
  //     return;
  //   }

  //   // Изменение текущего шага мотора
  //   if (mtr->curState == Up) {
  //     mtr->curPosition -= min_step;
  //   } else if (mtr->curState == Down) {
  //     mtr->curPosition += min_step;
  //   }

  //   Serial.print(mtr->curPosition);
  //   Serial.print("\n");
  //   stepper->rotateDegrees(min_degress);
  // }
}

void doEvent(enum irEvent e) {
  switch (e) {
    case Close: {
      if (firstMotor.getCurState() != Down) {
        firstMotor.changeState(Down);
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (firstMotor.getCurState() != Up) {
        firstMotor.changeState(Up);
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (firstMotor.getCurState() != Idle) {
        firstMotor.changeState(Idle);
        Serial.print("Stop\n");
      }
      break;
    }
    case MenuSwitch: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Calibration;
        if (firstMotor.getCurState() != Idle) {
          doEvent(Stop);
        }
        Serial.print("Calibration mode\n");
      } else if (mtrMngMode == Calibration) {
        mtrMngMode = Auto;
        Serial.print("Auto mode\n");
      }
      break;
    }
    case MotorSwitch: {
      doEvent(Stop);
      firstMotor.toggleActive();
      firstMotor.saveData();
      Serial.print("Motor switched\n");
      printStructList();
      break;
    }
  }
}

void controlManagerLoop() {
  if (irrecv.decode(&results)) {
    // Serial.print(results.value);
    // Serial.print("\n");
    switch (results.value) {
      case next:
        doEvent(MenuSwitch);
        break;
      case prev:
        doEvent(MotorSwitch);
        break;
      case zero:
        printStructList();
        break;
      default: {
        for (int i = 0; i < MOTOR_COUNT; i++) {
          controlLoop(results.value);
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

void controlLoop(unsigned long value) {
  if (firstMotor.isActive()) {
    switch (value) {
      case plus:
        firstMotor.getCurState() == Up ? doEvent(Stop) : doEvent(Open);
        break;
      case minus:
        firstMotor.getCurState() == Down ? doEvent(Stop) : doEvent(Close);
        break;
      case pause:
        doEvent(Stop);
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



// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
#define INIT_KEY 253
// #define INIT_KEY 254

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
#include <CustomStepper.h>
#include "IRremote.h"

const int _UP = CW;
const int _DOWN = CCW;
const float MIN_POSITION=1.0;
// количество оборотов до полного закрытия шторы
const float DEFAULT_MAX_POSITION=2.0;

enum motorState {Idle, Up, Down};

struct MotorStruct {
  bool active;
  enum motorState curState;
  enum motorState prevState;
  float curPosition;
  float maxPosition;
};

MotorStruct firstMtr;
MotorStruct secondMtr;

MotorStruct EEMEM motorList_addr[MOTOR_COUNT];

MotorStruct motorList[] = {firstMtr, secondMtr};

const int ir_pin = A0;
enum irEvent {Close, Open, Stop, SwitchMenu};
const unsigned long minus = 16769055;
const unsigned long pause = 16761405;
const unsigned long plus = 16754775;
const unsigned long next = 16712445;
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

// 18;
int min_degress = 18;
// 0.05
float min_step = (float)min_degress / 360.0;

// Указываем пины, к которым подключен драйвера шаговых двигателей
// CustomStepper firstStepper(8, 9, 10, 11);
CustomStepper firstStepper(4, 5, 6, 7);

// CustomStepper stepperList[MOTOR_COUNT] = {firstStepper};

bool similar(float A, float B, float epsilon = 0.005f) {
  return (fabs(A - B) < epsilon);
}

void printStructList() {
  Serial.print("First Motor:\n");
  printStruct(motorList[0]);
  Serial.print("Second Motor:\n");
  printStruct(motorList[1]);
}

void printStruct(MotorStruct mtr) {
  Serial.print("Active: ");
  Serial.print(mtr.active);
  Serial.print("\n");

  Serial.print("Cur position: ");
  Serial.print(mtr.curPosition);
  Serial.print("\n");

  Serial.print("Max position: ");
  Serial.print(mtr.maxPosition);
  Serial.print("\n");

  Serial.print("Cur state: ");
  Serial.print(mtr.curState);
  Serial.print("\n");
  Serial.print("\n");
}

void initMotors() {
  if (EEPROM.read(INIT_ADDR) != INIT_KEY) {
    // Записали ключ
    EEPROM.write(INIT_ADDR, INIT_KEY);

    Serial.print("First init\n");

    for (int i = 0; i < MOTOR_COUNT; i++) {
      motorList[i].active = true;
      motorList[i].curState = Idle;
      motorList[i].prevState = Idle;
      motorList[i].curPosition = MIN_POSITION;
      motorList[i].maxPosition = DEFAULT_MAX_POSITION;
    }

    EEPROM.put((int)&motorList_addr, motorList);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.print("\n");

  initMotors();

  // initMotot((int)&firstMotorAddr, firstMtr);
  EEPROM.get((int)&motorList_addr, motorList);

  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].curState = Idle;
    motorList[i].prevState = Idle;
  }

  printStructList();

  // for (int i = 0; i < MOTOR_COUNT; i++) {
    // Устанавливаем кол-во оборотов в минуту
    firstStepper.setRPM(12);
    // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
    firstStepper.setSPR(4075.7728395);
    firstStepper.setDirection(STOP);
    firstStepper.rotate();
  // }

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);
}

void loop() {
  controlLoop();
  motorManagerLoop();
}

void motorManagerLoop() {
  if (mtrMngMode == Calibration) {
    calibrationLoop();
  } else if (mtrMngMode == Auto) {
    for (int i = 0; i < MOTOR_COUNT; i++) {
      autoLoop(&motorList[i]);
    }
  }
  // for (int i = 0; i < MOTOR_COUNT; i++) {
    firstStepper.run();
  // }
}

void calibrationLoop() {
  if (motorList[0].curState == Idle && motorList[0].prevState == Idle) {
    return;
  }
  // if (stepperList[0].isDone()) {
  //   // Для разовой подачи команды на смену направления
  //   if (motorList[0].curState == Up && motorList[0].prevState != Up) {
  //     stepperList[0].setDirection(_UP);
  //     motorList[0].prevState = motorList[0].curState;
  //   } else if (motorList[0].curState == Down && motorList[0].prevState != Down) {
  //     stepperList[0].setDirection(_DOWN);
  //     motorList[0].curPosition = MIN_POSITION;

  //     motorList[0].prevState = motorList[0].curState;
  //   } else if (motorList[0].curState == Idle && motorList[0].prevState != Idle) {
  //     stepperList[0].setDirection(STOP);
  //     if (motorList[0].prevState == Down) {
  //       motorList[0].maxPosition = motorList[0].curPosition;
  //       Serial.print("Save max turnover: ");
  //       Serial.print(motorList[0].maxPosition);
  //       Serial.print("\n");

  //       EEPROM.put((int)&motorList_addr, motorList);

  //       doEvent(SwitchMenu, motorList[0]);
  //     }
  //     motorList[0].prevState = motorList[0].curState;
  //     return;
  //   }

  //   // Изменение текущего шага мотора
  //   if (motorList[0].curState == Up) {
  //     motorList[0].curPosition -= min_step;
  //   } else if (motorList[0].curState == Down) {
  //     motorList[0].curPosition += min_step;
  //   }

  //   Serial.print(motorList[0].curPosition);
  //   Serial.print("\n");
  //   stepperList[0].rotateDegrees(min_degress);
  // }
}

void autoLoop(MotorStruct *mtr) {

  if (mtr->curState == Idle && mtr->prevState == Idle) {
    return;
  }
  if (firstStepper.isDone()) {
    // Для разовой подачи команды на смену направления
    if (mtr->curState == Up && mtr->prevState != Up) {
      firstStepper.setDirection(_UP);
      mtr->prevState = mtr->curState;
      EEPROM.put((int)&motorList_addr, motorList);
    } else if (mtr->curState == Down && mtr->prevState != Down) {
      firstStepper.setDirection(_DOWN);
      mtr->prevState = mtr->curState;
      EEPROM.put((int)&motorList_addr, motorList);
    } else if (mtr->curState == Idle && mtr->prevState != Idle) {
      firstStepper.setDirection(STOP);
      mtr->prevState = mtr->curState;

      EEPROM.put((int)&motorList_addr, motorList);
      return;
    }

    // Проверка на остановку
    if (mtr->curState == Up && similar(mtr->curPosition, MIN_POSITION)) {
      Serial.print(mtr->curPosition);
      Serial.print(" go min ");
      Serial.print(MIN_POSITION);
      mtr->curPosition = MIN_POSITION;
      doEvent(Stop, mtr);
      return;
    } else if (mtr->curState == Down && similar(mtr->curPosition, mtr->maxPosition)) {
      Serial.print(mtr->curPosition);
      Serial.print(" go max ");
      Serial.print(mtr->maxPosition);
      mtr->curPosition = mtr->maxPosition;
      doEvent(Stop, mtr);
      return;
    }

    // Изменение текущего шага мотора
    if (mtr->curState == Up) {
      mtr->curPosition -= min_step;
    } else if (mtr->curState == Down) {
      mtr->curPosition += min_step;
    }

    Serial.print(mtr->curPosition);
    Serial.print("\n");
    firstStepper.rotateDegrees(min_degress);
  }
}

void doEvent(enum irEvent e, MotorStruct *mtr) {
  switch (e) {
    case Close: {
      if (mtr->curState != Down) {
        mtr->prevState = mtr->curState;
        mtr->curState = Down;
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (mtr->curState != Up) {
        mtr->prevState = mtr->curState;
        mtr->curState = Up;
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (mtr->curState != Idle) {
        mtr->prevState = mtr->curState;
        mtr->curState = Idle;
        Serial.print("Stop\n");
      }
      break;
    }
    case SwitchMenu: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Calibration;
        if (mtr->curState != Idle) {
          doEvent(Stop, mtr);
        }
        Serial.print("Calibration mode\n");
      } else if (mtrMngMode == Calibration) {
        mtrMngMode = Auto;
        Serial.print("Auto mode\n");
      }
      break;
    }
  }
}

void controlLoop() {
  if (irrecv.decode(&results)) {
    // Serial.print(results.value);
    // Serial.print("\n");
    if (mtrMngMode == Auto || mtrMngMode == Calibration) {
      switch (results.value) {
        case plus:
          motorList[0].curState == Up ? doEvent(Stop, &motorList[0]) : doEvent(Open, &motorList[0]);
          break;
        case minus:
          motorList[0].curState == Down ? doEvent(Stop, &motorList[0]) : doEvent(Close, &motorList[0]);
          break;
        case pause:
          doEvent(Stop, &motorList[0]);
          break;
        case next:
          doEvent(SwitchMenu, &motorList[0]);
        case zero:
          printStructList();
          break;
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

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

// 18;
int min_degress = 18;
// 0.05
float min_step = (float)min_degress / 360.0;

// Указываем пины, к которым подключен драйвера шаговых двигателей
CustomStepper firstStepper(8, 9, 10, 11);
CustomStepper secondStepper(4, 5, 6, 7);
CustomStepper stepperList[] = {firstStepper, secondStepper};

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
      motorList[i].active = i == 0;
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

  for (int i = 0; i < MOTOR_COUNT; i++) {
    // Устанавливаем кол-во оборотов в минуту
    stepperList[i].setRPM(12);
    // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
    stepperList[i].setSPR(4075.7728395);
    stepperList[i].setDirection(STOP);
    stepperList[i].rotate();
  }

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);
}

void loop() {
  controlManagerLoop();
  motorManagerLoop();
}

void motorManagerLoop() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    if (!motorList[i].active) {
      continue;
    }

    if (mtrMngMode == Calibration) {
      calibrationLoop(&motorList[i], &stepperList[i]);
    } else if (mtrMngMode == Auto) {
      autoLoop(&motorList[i], &stepperList[i]);
    }

    stepperList[i].run();
  }
}

void calibrationLoop(MotorStruct *mtr, CustomStepper *stepper) {
  if ((mtr->curState == Idle && mtr->prevState == Idle) || !mtr->active) {
    return;
  }

  if (stepper->isDone()) {
    // Для разовой подачи команды на смену направления
    if (mtr->curState == Up && mtr->prevState != Up) {
      stepper->setDirection(_UP);
      mtr->prevState = mtr->curState;
    } else if (mtr->curState == Down && mtr->prevState != Down) {
      stepper->setDirection(_DOWN);
      mtr->curPosition = MIN_POSITION;

      mtr->prevState = mtr->curState;
    } else if (mtr->curState == Idle && mtr->prevState != Idle) {
      stepper->setDirection(STOP);
      if (mtr->prevState == Down) {
        mtr->maxPosition = mtr->curPosition;
        Serial.print("Save max turnover: ");
        Serial.print(mtr->maxPosition);
        Serial.print("\n");

        EEPROM.put((int)&motorList_addr, motorList);

        doEvent(MenuSwitch, mtr);
      }
      mtr->prevState = mtr->curState;
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
    stepper->rotateDegrees(min_degress);
  }
}

void autoLoop(MotorStruct *mtr, CustomStepper *stepper) {
  if ((mtr->curState == Idle && mtr->prevState == Idle) || !mtr->active) {
    return;
  }

  if (stepper->isDone()) {
    // Для разовой подачи команды на смену направления
    if (mtr->curState == Up && mtr->prevState != Up) {
      stepper->setDirection(_UP);
      mtr->prevState = mtr->curState;
      EEPROM.put((int)&motorList_addr, motorList);
    } else if (mtr->curState == Down && mtr->prevState != Down) {
      stepper->setDirection(_DOWN);
      mtr->prevState = mtr->curState;
      EEPROM.put((int)&motorList_addr, motorList);
    } else if (mtr->curState == Idle && mtr->prevState != Idle) {
      stepper->setDirection(STOP);
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
    stepper->rotateDegrees(min_degress);
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
    case MenuSwitch: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Calibration;
        for (int i = 0; i < MOTOR_COUNT; i++) {
          if (motorList[i].curState != Idle) {
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
      for (int i = 0; i < MOTOR_COUNT; i++) {
        doEvent(Stop, &motorList[i]);
        motorList[i].active = !motorList[i].active;
      }
      EEPROM.put((int)&motorList_addr, motorList);
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
          controlLoop(&motorList[i], results.value);
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

void controlLoop(MotorStruct *mtr, unsigned long value) {
  if (mtr->active) {
    switch (value) {
      case plus:
        mtr->curState == Up ? doEvent(Stop, mtr) : doEvent(Open, mtr);
        break;
      case minus:
        mtr->curState == Down ? doEvent(Stop, mtr) : doEvent(Close, mtr);
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

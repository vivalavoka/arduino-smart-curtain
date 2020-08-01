// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
// #define INIT_KEY 253
#define INIT_KEY 254

// Дребезг, случайные замыкания
#define button_debounce 20
// Клик
#define button_hold 250
// Долгое нажатие
#define button_long 3000
// Ошибка, на кнопку сели
#define button_idle 5000

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

// Адрес в памяти для хранения данных по первому мотору
MotorStruct EEMEM firstMotorAddr;

// Адрес в памяти для хранения данных по второму мотору
MotorStruct EEMEM secondMotorAddr;

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

// Указываем пины, к которым подключен драйвер шагового двигателя
CustomStepper stepper(8, 9, 10, 11);

bool similar(float A, float B, float epsilon = 0.005f) {
  return (fabs(A - B) < epsilon);
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

void setup() {
  Serial.begin(9600);
  Serial.print("\n");

  // Первый запуск
  if (EEPROM.read(INIT_ADDR) != INIT_KEY) {
    // Записали ключ
    EEPROM.write(INIT_ADDR, INIT_KEY);

    Serial.print("First init\n");
    firstMtr.active = true;
    firstMtr.curState = Idle;
    firstMtr.prevState = Idle;
    firstMtr.curPosition = MIN_POSITION;
    firstMtr.maxPosition = DEFAULT_MAX_POSITION;

    EEPROM.put((int)&firstMotorAddr, firstMtr);
    // eeprom_write_block((void*)&firstMtr, (const void*)&firstMotorAddr, sizeof(firstMtr));
  }

  EEPROM.get((int)&firstMotorAddr, firstMtr);
  // eeprom_read_block((void*)&firstMtr, (const void*)&firstMotorAddr, sizeof(firstMtr));

  firstMtr.curState = Idle;
  firstMtr.prevState = Idle;

  printStruct(firstMtr);
  // firstMtr.active = (bool)EEPROM.read(0);
  // EEPROM.get(1, firstMtr.maxPosition);
  // EEPROM.get(5, firstMtr.curPosition);

  // Устанавливаем кол-во оборотов в минуту
  stepper.setRPM(12);
  // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
  stepper.setSPR(4075.7728395);

  stepper.setDirection(STOP);
  stepper.rotate();

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
    autoLoop();
  }
  stepper.run();
}

void calibrationLoop() {
  if (firstMtr.curState == Idle && firstMtr.prevState == Idle) {
    return;
  }
  if (stepper.isDone()) {
    // Для разовой подачи команды на смену направления
    if (firstMtr.curState == Up && firstMtr.prevState != Up) {
      stepper.setDirection(_UP);
      firstMtr.prevState = firstMtr.curState;
    } else if (firstMtr.curState == Down && firstMtr.prevState != Down) {
      stepper.setDirection(_DOWN);
      firstMtr.curPosition = MIN_POSITION;

      firstMtr.prevState = firstMtr.curState;
    } else if (firstMtr.curState == Idle && firstMtr.prevState != Idle) {
      stepper.setDirection(STOP);
      if (firstMtr.prevState == Down) {
        firstMtr.maxPosition = firstMtr.curPosition;
        Serial.print("Save max turnover: ");
        Serial.print(firstMtr.maxPosition);
        Serial.print("\n");

        EEPROM.put((int)&firstMotorAddr, firstMtr);

        doEvent(SwitchMenu);
      }
      firstMtr.prevState = firstMtr.curState;
      return;
    }

    // Изменение текущего шага мотора
    if (firstMtr.curState == Up) {
      firstMtr.curPosition -= min_step;
    } else if (firstMtr.curState == Down) {
      firstMtr.curPosition += min_step;
    }

    Serial.print(firstMtr.curPosition);
    Serial.print("\n");
    stepper.rotateDegrees(min_degress);
  }
}

void autoLoop() {
  if (firstMtr.curState == Idle && firstMtr.prevState == Idle) {
    return;
  }
  if (stepper.isDone()) {
    // Для разовой подачи команды на смену направления
    if (firstMtr.curState == Up && firstMtr.prevState != Up) {
      stepper.setDirection(_UP);
      firstMtr.prevState = firstMtr.curState;
      EEPROM.put((int)&firstMotorAddr, firstMtr);
    } else if (firstMtr.curState == Down && firstMtr.prevState != Down) {
      stepper.setDirection(_DOWN);
      firstMtr.prevState = firstMtr.curState;
      EEPROM.put((int)&firstMotorAddr, firstMtr);
    } else if (firstMtr.curState == Idle && firstMtr.prevState != Idle) {
      stepper.setDirection(STOP);
      firstMtr.prevState = firstMtr.curState;
      EEPROM.put((int)&firstMotorAddr, firstMtr);
      return;
    }

    // Проверка на остановку
    if (firstMtr.curState == Up && similar(firstMtr.curPosition, MIN_POSITION)) {
      Serial.print(firstMtr.curPosition);
      Serial.print(" go min ");
      Serial.print(MIN_POSITION);
      firstMtr.curPosition = MIN_POSITION;
      doEvent(Stop);
      return;
    } else if (firstMtr.curState == Down && similar(firstMtr.curPosition, firstMtr.maxPosition)) {
      Serial.print(firstMtr.curPosition);
      Serial.print(" go max ");
      Serial.print(firstMtr.maxPosition);
      firstMtr.curPosition = firstMtr.maxPosition;
      doEvent(Stop);
      return;
    }

    // Изменение текущего шага мотора
    if (firstMtr.curState == Up) {
      firstMtr.curPosition -= min_step;
    } else if (firstMtr.curState == Down) {
      firstMtr.curPosition += min_step;
    }

    Serial.print(firstMtr.curPosition);
    Serial.print("\n");
    stepper.rotateDegrees(min_degress);
  }
}

void doEvent(enum irEvent e) {
  switch (e) {
    case Close: {
      if (firstMtr.curState != Down) {
        firstMtr.prevState = firstMtr.curState;
        firstMtr.curState = Down;
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (firstMtr.curState != Up) {
        firstMtr.prevState = firstMtr.curState;
        firstMtr.curState = Up;
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (firstMtr.curState != Idle) {
        firstMtr.prevState = firstMtr.curState;
        firstMtr.curState = Idle;
        Serial.print("Stop\n");
      }
      break;
    }
    case SwitchMenu: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Calibration;
        if (firstMtr.curState != Idle) {
          doEvent(Stop);
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
          firstMtr.curState == Up ? doEvent(Stop) : doEvent(Open);
          break;
        case minus:
          firstMtr.curState == Down ? doEvent(Stop) : doEvent(Close);
          break;
        case pause:
          doEvent(Stop);
          break;
        case next:
          doEvent(SwitchMenu);
        case zero:
          printStruct(firstMtr);
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

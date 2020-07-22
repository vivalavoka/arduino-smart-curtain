// Дребезг, случайные замыкания
#define button_debounce 20
// Клик
#define button_hold 250
// Долгое нажатие
#define button_long 3000
// Ошибка, на кнопку сели
#define button_idle 5000

#include <avr/eeprom.h>
#include <CustomStepper.h>
#include "IRremote.h"

const int ir_pin = A0;

// События
// enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitIdle};

// enum buttonState {Idle, PreClick, Click, Hold, LongHold, ForcedIdle};

// enum buttonState btnState = Idle;

enum motorState {Idle, RunCW, RunCCW};

enum motorState mtrState = Idle;
enum motorState prevMtrState = Idle;

enum motorManagerMode {Auto, Manual, Calibration};

enum motorManagerMode mtrMngMode = Manual;

enum irEvent {Close, Open, Stop, SwitchMenu};

// unsigned long pressTimestamp;

const unsigned long minus = 16769055;
const unsigned long pause = 16761405;
const unsigned long plus = 16754775;
const unsigned long next = 16712445;

IRrecv irrecv(ir_pin);
decode_results results;

float mtr_min=0;
// количество оборотов до полного закрытия шторы
float mtr_max=3;
float mtr_cur;

// int min_degress = 18;
int min_degress = 18;
float min_step = (float)min_degress / 360.0;

// Указываем пины, к которым подключен драйвер шагового двигателя
CustomStepper stepper(8, 9, 10, 11);

void setup() {
  if (eeprom_read_float(0) != 0) {
    _EEGET(mtr_max, 0);
  }

  Serial.print(mtr_max);

  mtr_cur = mtr_min;

  // Устанавливаем кол-во оборотов в минуту
  stepper.setRPM(12);
  // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
  stepper.setSPR(4075.7728395);

  stepper.setDirection(STOP);
  stepper.rotate();

  irrecv.enableIRIn();
  pinMode(ir_pin, INPUT);

  Serial.begin(9600);
}

void loop() {
  controlLoop();
  motorManagerLoop();
}

void motorManagerLoop() {
  if (mtrMngMode == Calibration) {

  } else if (mtrMngMode == Manual) {
    manualLoop();
  }
  stepper.run();
}

void manualLoop() {
  if (mtrState == Idle && prevMtrState == Idle) {
    return;
  }
  if (stepper.isDone()) {
    // Для разовой подачи команды на смену направления
    if (mtrState == RunCW && prevMtrState != RunCW) {
      stepper.setDirection(CW);
      prevMtrState = mtrState;
    } else if (mtrState == RunCCW && prevMtrState != RunCCW) {
      stepper.setDirection(CCW);
      prevMtrState = mtrState;
    } else if (mtrState == Idle && prevMtrState != Idle) {
      stepper.setDirection(STOP);
      prevMtrState = mtrState;
      return;
    }

    // Изменение текущего шага мотора
    if (mtrState == RunCW) {
      mtr_cur += min_step;
    } else if (mtrState == RunCCW) {
      mtr_cur -= min_step;
    }

    if (mtr_cur > mtr_max) {
      mtr_cur = mtr_max;
      doEvent(Stop);
      return;
    } else if (mtr_cur < mtr_min) {
      mtr_cur = mtr_min; 
      doEvent(Stop);
      return;
    }

    Serial.print(mtr_cur);
    Serial.print("\n");
    stepper.rotateDegrees(min_degress);
  }
}

void doEvent(enum irEvent e) {
  switch (e) {
    case Close: {
      if (mtrState != RunCCW) {
        prevMtrState = mtrState;
        mtrState = RunCCW;
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (mtrState != RunCW) {
        prevMtrState = mtrState;
        mtrState = RunCW;
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (mtrState != Idle) {
        prevMtrState = mtrState;
        mtrState = Idle;
        Serial.print("Stop\n");
      }
      break;
    }
    case SwitchMenu: {
      if (mtrMngMode == Auto) {
        mtrMngMode = Manual;
        Serial.print("Manual mode\n");
      } else if (mtrMngMode == Manual) {
        mtrMngMode = Calibration;
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
      Serial.print(results.value);
      Serial.print("\n");
      switch (results.value) {
        case plus:
          doEvent(Open);
          break;
        case minus:
          doEvent(Close);
          break;
        case pause:
          doEvent(Stop);
          break;
        case next:
          doEvent(SwitchMenu);
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
//       mtrState = mtrState == RunCCW ? RunCW : RunCCW;
//       stepper.setDirection(STOP);
//       Serial.print("Click\n");
//       break;
//     case Hold:
//       mtrState = Stop;
//       Serial.print("Hold\n");
//       break;
//     case LongHold:
//       Serial.print("LongHold\n");
//       break;
//     default:
//       break;
//   }
// }

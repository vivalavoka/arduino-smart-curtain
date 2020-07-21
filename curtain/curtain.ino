// Дребезг, случайные замыкания
#define button_debounce 20
// Клик
#define button_hold 250
// Долгое нажатие
#define button_long 3000
// Ошибка, на кнопку сели
#define button_idle 5000

#include <CustomStepper.h>
#include "IRremote.h"

const int RECEIVE_PIN = 2;
IRrecv irrecv(A0);
decode_results results;

// Указываем пины, к которым подключен драйвер шагового двигателя
CustomStepper stepper(8, 9, 10, 11);

// События
// enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitIdle};

// enum buttonState {Idle, PreClick, Click, Hold, LongHold, ForcedIdle};

// enum buttonState btnState = Idle;

enum motorState {Idle, RunCW, RunCCW};

enum motorState mtrState = Idle;

enum irEvent {Close, Open, Stop};

unsigned long pressTimestamp;

const unsigned long minus = 16753245;
const unsigned long neutral = 16736925;
const unsigned long plus = 16769565;

void setup() {
  // Устанавливаем кол-во оборотов в минуту
  stepper.setRPM(12);
  // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
  stepper.setSPR(4075.7728395);

  stepper.setDirection(STOP);
  stepper.rotate();

  irrecv.enableIRIn();
  pinMode(A0, INPUT);

  Serial.begin(9600);
}

void loop() {
  controlLoop();

  motorManagerLoop();
}

void motorManagerLoop() {
  stepper.run();
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
        case neutral:
          doEvent(Stop);
          break;
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


void doEvent(enum irEvent e) {
  switch (e) {
    case Close: {
      if (mtrState != RunCCW) {
        stepper.setDirection(CCW);
        stepper.rotate();
        mtrState = RunCCW;
        Serial.print("Close\n");
      }
      break;
    }
    case Open: {
      if (mtrState != RunCW) {
        stepper.setDirection(CW);
        stepper.rotate();
        mtrState = RunCW;
        Serial.print("Open\n");
      }
      break;
    }
    case Stop: {
      if (mtrState != Idle) {
        stepper.setDirection(STOP);
        mtrState = Idle;
        Serial.print("Stop\n");
      }
      break;
    }
  }
}

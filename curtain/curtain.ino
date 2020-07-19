// Дребезг, случайные замыкания
#define button_debounce 20
// Клик
#define button_hold 250
// Долгое нажатие
#define button_long 3000
// Ошибка, на кнопку сели
#define button_idle 5000

#include <CustomStepper.h>

// Указываем пины, к которым подключен драйвер шагового двигателя
CustomStepper stepper(8, 9, 10, 11);

// События
enum event {Press, Release, WaitDebounce, WaitHold, WaitLongHold, WaitIdle};

enum buttonState {Idle, PreClick, Click, Hold, LongHold, ForcedIdle};

enum buttonState btnState = Idle;

enum motorState {RunCW, RunCCW, Stop};

enum motorState mtrState = Stop;

unsigned long pressTimestamp;

const int btPin = 2;

void setup() {
  stepper.setRPM(12);                 // Устанавливаем кол-во оборотов в минуту
  stepper.setSPR(4075.7728395);       // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395

  // Кнопка
  pinMode(2, INPUT);

  Serial.begin(9600);
}

void loop() {
  unsigned long mls = millis();

  if (digitalRead(btPin)) {
    doEvent(Press);
  } else {
    doEvent(Release);
  }

  if (mls - pressTimestamp > button_debounce) {
    doEvent(WaitDebounce);
  }
  if (mls - pressTimestamp > button_hold) {
    doEvent(WaitHold);
  }
  if (mls - pressTimestamp > button_long) {
    doEvent(WaitLongHold);
  }
  if (mls - pressTimestamp > button_idle) {
    doEvent(WaitIdle);
  }

  if (mtrState == Stop) {
    stepper.setDirection(STOP);
  }

  if (stepper.isDone()) {
    if (mtrState == RunCW) {
      Serial.print("Rotate CW\n");
      stepper.setDirection(CW);
    } else if (mtrState == RunCCW) {
      Serial.print("Rotate CCW\n");
      stepper.setDirection(CCW);
    } else if (mtrState == Stop) {
      stepper.setDirection(STOP);
    }
    stepper.rotate();
  }
  stepper.run();
}

void doEvent(enum event e) {
  switch (e) {
    case Press:
      if (btnState == Idle) {
        btnState =  PreClick;
        pressTimestamp = millis();
      }
      break;
    case Release:
      onClick(btnState);
      btnState = Idle;
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
    case WaitLongHold:
      if (btnState == Hold) {
        btnState = LongHold;
      }
      break;
    case WaitIdle:
      if (btnState == LongHold) {
        // btnState = Idle;
      }
      break;
  }
}

void onClick(enum buttonState s) {
  switch (s) {
    case Click:
      mtrState = mtrState == RunCCW ? RunCW : RunCCW;
      stepper.setDirection(STOP);
      Serial.print("Click\n");
      break;
    case Hold:
      mtrState = Stop;
      Serial.print("Hold\n");
      break;
    case LongHold:
      Serial.print("LongHold\n");
      break;
    default:
      break;
  }
}

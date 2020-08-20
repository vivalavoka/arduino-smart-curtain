// Адрес ячейки, в которой будет хранится ключ первого запуска
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
#define INIT_KEY 1

// Количество моторов
#define MOTOR_COUNT 2

// Всё, что до 20 мс - дребезг, случайные замыкания
// От 20 мс до 1000 мс - клик
// Дольше - долгое нажатие
#define BUTTON_DEBOUNCE 20
#define BUTTON_HOLD 1000

#include <EEPROM.h>
#include "IRremote.h"
#include "Motor.h"

// Пин пьезоэлемента
const int piezoPin = 2;
// Пин кнопки
const int btPin = 13;
// Пин IR-приемника
const int irPin = A0;

// ----- Моторы ------
/*
* Left - расположение мотора относительно шторы
* 8, 9, 10, 11 - 4 пина для драйвера
* FIRST_LED - Пин для лампочки
*/
int FIRST_LED = 12;
Motor firstMotor(Left, 8, 9, 10, 11, FIRST_LED);

/*
* Right - расположение мотора относительно шторы
* 4, 5, 6, 7 - 4 пина для драйвера
* SECOND_LED - Пин для лампочки
*/
int SECOND_LED = 3;
Motor secondMotor(Right, 4, 5, 6, 7, SECOND_LED);

// Список моторов
Motor motorList[] = {firstMotor, secondMotor};
// -----------


// ------ Менеджер моторов ------
// Режимы работы моторов - Рабочий/Калибровка
// По умолчанию - Рабочий режим
enum motorManagerMode mtrMngMode = Work;

// Количество рабочих моторов - Первый/Второй/Оба
enum motorActiveState {First, Second, Both};

// По умолчанию - оба активны
motorActiveState mtrActiveState = Both;

// События доступные менеджеру моторов
enum motorManagerEvent {Close, Open, Stop, MenuSwitch, MotorSwitch};
// -----------


// ------ IR-приемник ------
// Код кнопки для закрытия
const unsigned long closeCode = 16769055;
// Код кнопки для открытия
const unsigned long openCode = 16754775;

// Ниже представлены дополнительные кнопки управления
// Код кнопки для остановки
const unsigned long stopCode = 16761405;
// Код кнопки для переключения меню
// const unsigned long menuCode = 16712445;
// Код кнопки для переключения моторов
// const unsigned long motorCode = 16720605;
// Код кнопки для вывода в лог полной информации о моторах
// const unsigned long infoCode = 16738455;

// Настройки ir-приемника
IRrecv irrecv(irPin);
decode_results results;
// -----------


// ------ Кнопка -------
// Временная метка нажатия кнопки
unsigned long pressTimestamp;

// События доступные кнопке
enum buttonEvent {Press, Release, WaitDebounce, WaitHold};

// Состояния кнопки
enum buttonState {Sleep, PreClick, Click, Hold};

// По умолчанию - режим ожидания
enum buttonState btnState = Sleep;
// -----------

// ===== Установка =====
void setup() {
  Serial.begin(9600);
  Serial.print("Setup\n");

  pinMode(piezoPin, OUTPUT);
  pinMode(FIRST_LED, OUTPUT);
  pinMode(SECOND_LED, OUTPUT);

  irrecv.enableIRIn();
  pinMode(irPin, INPUT);

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

// ===== Основной цикл программы =====
void loop() {
  controlManagerLoop();
  motorManagerLoop();
  releaseMotorData();
}

// ===== Цикл работы моторов =====
void motorManagerLoop() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].loop(mtrMngMode);
  }
}

// ===== Блок управления ======
// ---- Основной метод управления -----
void controlManagerLoop() {
  irControl();
  buttonControl();
}

// ----- Обработчик ir-сигналов ------
void irControl() {
  if (irrecv.decode(&results)) {
    // Serial.print(results.value);
    // Serial.print("\n");
    switch (results.value) {
      // case menuCode:
      //   doEvent(MenuSwitch, &motorList[0]);
      //   break;
      // case motorCode:
      //   doEvent(MotorSwitch, &motorList[0]);
      //   break;
      // case infoCode:
      //   printStructList();
      //   break;
      default: {
        for (int i = 0; i < MOTOR_COUNT; i++) {
          motorControlLoop(results.value, &motorList[i]);
        }
      }
    }
    irrecv.resume();
  }
}

// ----- Блок управления конкретным мотором ------
void motorControlLoop(unsigned long value, Motor *mtr) {
  if (mtr->isActive()) {
    switch (value) {
      case openCode:
        mtr->getCurState() == Up ? doEvent(Stop, mtr) : doEvent(Open, mtr);
        break;
      case closeCode:
        mtr->getCurState() == Down ? doEvent(Stop, mtr) : doEvent(Close, mtr);
        break;
      case stopCode:
        doEvent(Stop, mtr);
        break;
    }
  }
}

// ------ Обработчик сигналов с кнопки ------
void buttonControl() {
  unsigned long mls = millis();

  if (digitalRead(btPin)) {
    doButtonEvent(Press);
  } else {
    doButtonEvent(Release);
  }

  if (mls - pressTimestamp > BUTTON_DEBOUNCE) {
    doButtonEvent(WaitDebounce);
  }
  if (mls - pressTimestamp > BUTTON_HOLD) {
    doButtonEvent(WaitHold);
  }
}

// Обработчик событий по конкретному нажатию
void doButtonEvent(enum buttonEvent e) {
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

// Обработчик нажатия
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

// ===== Блок управления моторами ======

// Сохранение показателей моторов в постоянную память
void saveAllData() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    motorList[i].saveData();
  }
}

// Принятие решение о возможности сохранить показатели
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

// Обработчик события изменения состояния мотора
void doEvent(enum motorManagerEvent e, Motor *mtr) {
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
      if (mtrMngMode == Work) {
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
        mtrMngMode = Work;
        Serial.print("Work mode\n");
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

// Преключение режимов менеджера моторов
void switchMotors() {
  if(mtrMngMode == Work) {
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
  if (mtrMngMode == Work) {
    saveAllData();
  }
}

// Выставление активности моторов в зависимости от режима
void setMotorActivities() {
  switch (mtrActiveState) {
    case First:
      motorList[0].setActive(1);
      motorList[1].setActive(0);
      break;
    case Second:
      motorList[0].setActive(0);
      motorList[1].setActive(1);
      break;
    case Both:
      motorList[0].setActive(1);
      motorList[1].setActive(1);
      break;
  }
}

// Определение текущего режима мотором
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

// ===== Доп методы ======

// короткий бип
void beep() {
  // 800 - тональность, чем меньше тем ниже
  // 100 - протяжность, 100мс
  tone(piezoPin, 800, 100);
}

// длинный бииип
void longBeep() {
  // 1000 - тональность, чем больше тем выше
  // 500 - протяжность, 500мс
  tone(piezoPin, 1000, 500);
}

// Вывод в консоль информации о моторах
void printStructList() {
  for (int i = 0; i < MOTOR_COUNT; i++) {
    Serial.print(i+1);
    Serial.print(" Motor:\n");
    motorList[i].print();
  }
}
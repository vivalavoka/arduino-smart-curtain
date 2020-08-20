#include "Motor.h"

// минимальная позиция (открытое состоения шторы)
const float MIN_POSITION=1.0;
// количество оборотов до полного закрытия шторы
const float DEFAULT_MAX_POSITION=2.0;

// 18;
int min_degress = 18;
// 0.05
float min_step = (float)min_degress / 360.0;

Motor::Motor(motorLocation location, byte pinA, byte pinB, byte pinC, byte pinD, int ledPin) {
    this->_stepper = new CustomStepper(pinA, pinB, pinC, pinD);
    this->_curState = Idle;
    this->_prevState = Idle;

    this->_UP = location == Left ? CCW : CW;
    this->_DOWN = location == Left ? CW : CCW;

    this->_LED_PIN = ledPin;
}

void Motor::initData(bool firstInit, int index) {
    this->_eeAddress = index * sizeof(MotorStruct);

    if (firstInit) {
        this->_data.active = true;
        this->_data.curPosition = MIN_POSITION;
        this->_data.maxPosition = DEFAULT_MAX_POSITION;

        this->saveData();
    }

    EEPROM.get(this->_eeAddress, this->_data);
    this->needSave = false;
}

void Motor::initStepper() {
    // Устанавливаем кол-во оборотов в минуту
    this->_stepper->setRPM(12);
    // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395
    this->_stepper->setSPR(4075.7728395);
    this->_stepper->setDirection(STOP);
    this->_stepper->rotate();
}

void Motor::print() {
    Serial.print("Active: ");
    Serial.print(this->_data.active);
    Serial.print("\n");

    Serial.print("Cur position: ");
    Serial.print(this->_data.curPosition);
    Serial.print("\n");

    Serial.print("Max position: ");
    Serial.print(this->_data.maxPosition);
    Serial.print("\n");

    Serial.print("Cur state: ");
    Serial.print(this->_curState);
    Serial.print("\n");
    Serial.print("\n");
}


bool Motor::_isSimilar(float A, float B) {
    return (fabs(A - B) < 0.005f);
}

void Motor::loop(motorManagerMode mtrMngMode) {
    if (!this->isActive() || (this->_curState == Idle && this->_prevState == Idle)) {
      return;
    }

    if (this->_stepper->isDone()) {
        // Для разовой подачи команды на смену направления
        if (this->_prevState != this->_curState) {
            this->_switchStepperDirection(mtrMngMode);
            this->_prevState = this->_curState;
        }

        if (mtrMngMode == Work) {
            this->_checkEndValues();
        }

        this->_changePosition();
    }

    this->_stepper->run();
}

void Motor::_switchStepperDirection(motorManagerMode mtrMngMode) {
    switch (this->_curState) {
        case Up:
            this->_stepper->setDirection(this->_UP);
            break;
        case Down:
            this->_stepper->setDirection(this->_DOWN);
            if (mtrMngMode == Calibration) {
                this->_data.curPosition = MIN_POSITION;
            }
            break;
        case Idle:
            this->needSave = true;
            this->_stepper->setDirection(STOP);
            if (mtrMngMode == Calibration && this->_prevState == Down) {
                this->_data.maxPosition = this->_data.curPosition;
                Serial.print("Save max turnover: ");
                Serial.print(this->_data.maxPosition);
                Serial.print("\n");
            }
            break;
    }
}

void Motor::_checkEndValues() {
    // Проверка на остановку
    if (this->_curState == Up && this->_isSimilar(this->_data.curPosition, MIN_POSITION)) {
        Serial.print("Completely open\n");
        this->_data.curPosition = MIN_POSITION;
        this->changeState(Idle);
    } else if (this->_curState == Down && this->_isSimilar(this->_data.curPosition, this->_data.maxPosition)) {
        Serial.print("Completely closed\n");
        this->_data.curPosition = this->_data.maxPosition;
        this->changeState(Idle);
    }
}

void Motor::_changePosition() {
    // Изменение текущего шага мотора
    if (this->_curState == Up) {
        this->_data.curPosition -= min_step;
        this->_stepper->rotateDegrees(min_degress);
        Serial.print(this->_data.curPosition);
        Serial.print("\n");
    } else if (this->_curState == Down) {
        this->_data.curPosition += min_step;
        this->_stepper->rotateDegrees(min_degress);
        Serial.print(this->_data.curPosition);
        Serial.print("\n");
    }
}

void Motor::changeState(motorState newState) {
    this->_prevState = this->_curState;
    this->_curState = newState;
}

bool Motor::isActive() {
    return this->_data.active;
}

motorState Motor::getCurState() {
    return this->_curState;
}

motorState Motor::getPrevState() {
    return this->_prevState;
}

void Motor::setActive(bool state) {
    this->_data.active = state;
    this->_setupLed();
}

void Motor::saveData() {
    EEPROM.put(this->_eeAddress, this->_data);
    this->needSave = false;
}

void Motor::_setupLed() {
    if (this->_data.active) {
        this->_ledOn();
    } else {
        this->_ledOff();
    }
}

void Motor::_ledOn() {
    digitalWrite(this->_LED_PIN, HIGH);
}

void Motor::_ledOff() {
    digitalWrite(this->_LED_PIN, LOW);
}
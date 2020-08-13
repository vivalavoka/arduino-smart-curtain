#include "Motor.h"

// крутить по часовой, чтобы поднять штору
const int _UP = CW;
// крутить против часовой, чтобы опустить штору
const int _DOWN = CCW;
// минимальная позиция (открытое состоения шторы)
const float MIN_POSITION=1.0;
// количество оборотов до полного закрытия шторы
const float DEFAULT_MAX_POSITION=2.0;

// 18;
int min_degress = 18;
// 0.05
float min_step = (float)min_degress / 360.0;

Motor::Motor(byte pinA, byte pinB, byte pinC, byte pinD) {
    this->_stepper = new CustomStepper(pinA, pinB, pinC, pinD);
    this->_curState = Idle;
    this->_prevState = Idle;
}

void Motor::initData(bool firstInit, int index) {
    this->_eeAddress = index * sizeof(MotorStruct);

    if (firstInit) {
        this->_data.active = index == 0;
        this->_data.curPosition = MIN_POSITION;
        this->_data.maxPosition = DEFAULT_MAX_POSITION;

        this->saveData();
        // EEPROM.put(this->_eeAddress, this->_data);
    }

    EEPROM.get(this->_eeAddress, this->_data);
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


bool Motor::isSimilar(float A, float B) {
    return (fabs(A - B) < 0.005f);
}

void Motor::loop(motorManagerMode mtrMngMode) {
    if (!this->isActive() || (this->_curState == Idle && this->_prevState == Idle)) {
      return;
    }

    if (this->_stepper->isDone()) {
        // Для разовой подачи команды на смену направления
        if (this->_prevState != this->_curState) {
            switch (this->_curState) {
                case Up:
                    this->_stepper->setDirection(_UP);
                    break;
                case Down:
                    this->_stepper->setDirection(_DOWN);
                    if (mtrMngMode == Calibration) {
                        this->_data.curPosition = MIN_POSITION;
                    }
                    break;
                case Idle:
                    this->_stepper->setDirection(STOP);
                    if (mtrMngMode == Calibration && this->_prevState == Down) {
                        this->_data.maxPosition = this->_data.curPosition;
                        Serial.print("Save max turnover: ");
                        Serial.print(this->_data.maxPosition);
                        Serial.print("\n");
                        this->saveData();
                    } 
                    break;
            }
            this->_prevState = this->_curState;
            // this->saveData();
            if (this->_curState == Idle) {
                return;
            }
        }

        if (mtrMngMode == Auto) {
            // Проверка на остановку
            if (this->_curState == Up && this->isSimilar(this->_data.curPosition, MIN_POSITION)) {
                Serial.print("Completely open\n");
                this->_data.curPosition = MIN_POSITION;
                this->changeState(Idle);
                return;
            } else if (this->_curState == Down && this->isSimilar(this->_data.curPosition, this->_data.maxPosition)) {
                Serial.print("Completely closed\n");
                this->_data.curPosition = this->_data.maxPosition;
                this->changeState(Idle);
                return;
            }
        }

        // Изменение текущего шага мотора
        if (this->_curState == Up) {
            this->_data.curPosition -= min_step;
        } else if (this->_curState == Down) {
            this->_data.curPosition += min_step;
        }

        Serial.print(this->_data.curPosition);
        Serial.print("\n");
        this->_stepper->rotateDegrees(min_degress);
    }

    this->_stepper->run();
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
}

void Motor::saveData() {
    Serial.print("SAVE\n");
    // EEPROM.put(this->_eeAddress, this->_data);
}

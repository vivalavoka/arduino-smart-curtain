// Адрес ячейки
#define INIT_ADDR 1023
// Ключ первого запуска. 0-254
#define INIT_KEY 253

#include "Motor.h"

const int _UP = CW;
const int _DOWN = CCW;
const float MIN_POSITION=1.0;
// количество оборотов до полного закрытия шторы
const float DEFAULT_MAX_POSITION=2.0;

MotorStruct EEMEM motorStruct_addr;

// 18;
int min_degress = 18;
// 0.05
float min_step = (float)min_degress / 360.0;

Motor::Motor() {
    this->_stepper = new CustomStepper(8, 9, 10, 11);
    this->_curState = Idle;
    this->_prevState = Idle;
    this->_data.active = true;
    this->_data.curPosition = MIN_POSITION;
    this->_data.maxPosition = DEFAULT_MAX_POSITION;
}

void Motor::initData() {
   if (EEPROM.read(INIT_ADDR) != INIT_KEY) {
    // Записали ключ
    EEPROM.write(INIT_ADDR, INIT_KEY);

    Serial.print("First init\n");

    this->_data.active = true;
    this->_data.curPosition = MIN_POSITION;
    this->_data.maxPosition = DEFAULT_MAX_POSITION;

    EEPROM.put((int)&motorStruct_addr, this->_data);
  }

  EEPROM.get((int)&motorStruct_addr, this->_data);
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

void Motor::loop() {
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
                    break;
                case Idle:
                    this->_stepper->setDirection(STOP);
                    break;
            }
            this->_prevState = this->_curState;
            this->saveData();
            if (this->_curState == Idle) {
                return;
            }
        }

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
    Serial.print("State changed\n");
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

bool Motor::toggleActive() {
    this->_data.active = !this->_data.active;
    return this->_data.active;
}

void Motor::saveData() {
    EEPROM.put((int)&motorStruct_addr, this->_data);
}

#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <CustomStepper.h>
#include <EEPROM.h>

enum motorState {Idle, Up, Down};

struct MotorStruct {
  bool active;
  float curPosition;
  float maxPosition;
};

class Motor {
  private:
    CustomStepper &_stepper;
    MotorStruct _data;
    enum motorState _curState;
    enum motorState _prevState;

  public:
    Motor(byte pins[]);
    bool isSimilar(float A, float B);
    void initData();
    void initStepper();
    void print();
    void loop();
    motorState getCurState();
    motorState getPrevState();
    bool isActive();

    void changeState(motorState newState);
    bool toggleActive();
    void saveData();
};

#endif
#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <CustomStepper.h>
#include <EEPROM.h>

enum motorState {Idle, Up, Down};
enum motorManagerMode {Auto, Calibration};

struct MotorStruct {
  bool active;
  float curPosition;
  float maxPosition;
};

class Motor {
  private:
    CustomStepper *_stepper;
    enum motorState _curState;
    enum motorState _prevState;
    int _eeAddress;

  public:
    MotorStruct _data;
    Motor(byte pinA, byte pinB, byte pinC, byte pinD);
    bool isSimilar(float A, float B);
    void initData(bool firstInit, MotorStruct data);
    void initStepper();
    void print();
    bool loop(motorManagerMode mtrMngMode);
    motorState getCurState();
    motorState getPrevState();
    bool isActive();

    void changeState(motorState newState);
    void setActive(bool state);
    void saveData();
};

#endif
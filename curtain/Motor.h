#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <CustomStepper.h>
#include <EEPROM.h>

enum motorState {Idle, Up, Down};
enum motorManagerMode {Auto, Calibration};
enum motorLocation {Left, Right};

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
    MotorStruct _data;
    int _eeAddress;
    int _UP;
    int _DOWN;
    void _switchStepperDirection(motorManagerMode mtrMngMode);
    void _checkEndValues();
    void _changePosition();
    bool _isSimilar(float A, float B);

  public:
    bool needSave;
    Motor(motorLocation location, byte pinA, byte pinB, byte pinC, byte pinD);
    void initData(bool firstInit, int index);
    void initStepper();
    void print();
    void loop(motorManagerMode mtrMngMode);
    motorState getCurState();
    motorState getPrevState();
    bool isActive();

    void changeState(motorState newState);
    void setActive(bool state);
    void saveData();
};

#endif
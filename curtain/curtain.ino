// Видеообзоры и уроки работы с ARDUINO на YouTube-канале IOMOIO: https://www.youtube.com/channel/UCmNXABaTjX_iKH28TTJpiqA

#include <CustomStepper.h>            // Подключаем библиотеку управления шаговым двигателем. По умолчанию настроена на двигатель 28BYJ-48-5V
CustomStepper stepper(8, 9, 10, 11);  // Указываем пины, к которым подключен драйвер шагового двигателя

const int buttonPin = 2;

int example = 1;
int buttonState = 0;
int touched = 0;

void setup()
{
  stepper.setRPM(12);                 // Устанавливаем кол-во оборотов в минуту
  stepper.setSPR(4075.7728395);       // Устанавливаем кол-во шагов на полный оборот. Максимальное значение 4075.7728395

  // pinMode(2, INPUT);
  // Serial.begin(9600);
}

void loop()
{
  // buttonState = digitalRead(buttonPin);

  // if (buttonState == HIGH && touched == 0) {
  //   touched = 1;
  //   Serial.print("Start\n");
  //   stepper.setDirection(CW);
  //   stepper.rotate();

  // } else if (buttonState == LOW && touched == 1) {
  //   touched = 0;

  //   Serial.print("Stop\n");
  //   stepper.setDirection(STOP);
  // }

  // stepper.run();

  if (stepper.isDone() and example == 1)  // Когда предыдущая команда выполнена (см. ниже), метод stepper.isDone() возвращает true
  {
    Serial.print("Example 1\n");
    stepper.setDirection(CW);         // Устанавливает направление вращения. Может принимать 3 значения: CW - по часовой, CCW - против часовой, STOP
    stepper.rotate(1);                // Устанавливает вращение на заданное кол-во оборотов
    example = 2;
  }
  if (stepper.isDone() and example == 2)
  {
    Serial.print("Example 2\n");
    stepper.setDirection(CCW);
    stepper.rotateDegrees(90);        // Поворачивает вал на заданное кол-во градусов. Можно указывать десятичную точность (например 90.5), но не принимаются отрицательные значения
    example = 3;
  }
  if (stepper.isDone() and example == 3)
  {
    Serial.print("Example 3\n");
    stepper.setDirection(CW);
    stepper.rotate();
    // Будет вращать пока не получит команду о смене направления или пока не получит директиву STOP
  }
  stepper.run();                      // Этот метод обязателен в блоке loop. Он инициирует работу двигателя, когда это необходимо
}

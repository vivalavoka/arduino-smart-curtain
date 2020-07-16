// #include <CyberLib.h>

// void setup()
// {
//   D8_Out;
//   D9_Out;
//   D10_Out;
//   D11_Out;
// }

// void loop()
// {
//   D8_High;
//   D9_High;
//   D10_Low;
//   D11_Low;
//   delay_ms(2000);
//   // D8_Low;
//   // D9_Low;
//   // D10_Low;
//   // D11_Low;
//   // delay_ms(2000);
// }

#include <Stepper.h>

// указываем количество шагов нашего мотора - в нашем случае 64
#define STEPS 64

// Создаем экземпляр класса Stepper, и указываем 
// количество шагов и пины подключния
Stepper stepper(STEPS, 8, 9, 10, 11);

// переменная для запоминания предыдущего значения
int previous = 0;

void setup() {
  // Устанавливаем скорость вращения в об/мин
  stepper.setSpeed(90);
}

void loop() {
  // новое значение полученное с датчика
  int val = analogRead(0); // При подключении датчика, 0 заменить на пин датчика

  // Прокрутить на разницу шагов между новым полученным значением и предыдущим
  stepper.step(val - previous);

  // Запоминаем новое полученное значение
  previous = val;
}
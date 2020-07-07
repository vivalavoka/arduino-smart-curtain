// CyberLib - https://istarik.ru/blog/arduino/1.html

//**********  (РЕЖИМ 1-ШАГ) Крутим с помощью сдвига --рабочая - крутит туда сюда без потери шагов - ускоряет - отключает катушки после бездействия
//PORTB - 8, 9, 10, 11 - Шаговик. && 12-кнопка Menu \ 13-Светодиод 1й штор.
//PORTD - 4, 5, 6, 7 - Шаговик. && 2-кнопка UP \ 3-кнопка DOWN
// А0-Светодиод 2й штор..

#include <EEPROM.h> 
#include <CyberLib.h>

#define uskor 3 //Задаем ускорение

//Здесь устанавливаем максимальное кол. шагов для штор
#define max_Sht1 28900
#define max_Sht2 27500

byte n1 = 1;// Сдвигаем 1 бит
byte n3 = 0x10;
bool f_timX = 0;
bool f_timZ = 0;

byte sped_Xt = uskor;//Начальная скорость
byte sped_X = 0; //Ускорение
byte sped_Zt = uskor;//Начальная скорость
byte sped_Z = 0; //Ускорение

bool f_run1X = 0;//Сброс после остановки 2 переменные.
unsigned long no_runX; //Переменная для прошедшего периода

bool f_run1Z = 0;//Сброс после остановки 2 переменные.
unsigned long no_runZ; //Переменная для прошедшего периода

// Переменные для кн.Menu
bool kn_menu;
bool f_menu;
bool f_Mpush;
bool f_Md;
uint8_t Menu = 1;
uint8_t temp_Menu;
bool kalib;
unsigned long menu_dreb;
unsigned long menu_klik;

// Переменные для кн.Up
bool kn_Up;
bool f_Up;
bool state_Up;
bool _UP;
uint32_t Up_dreb;

// Переменные для кн.Down
bool kn_Down;
bool f_Down;
bool state_Down;
bool _Down;
uint32_t Down_dreb;

//сбрасываем UP\Down
bool f_sbrosZ = 1;
bool f_sbrosX = 1;

uint32_t cur_time; //Миллис читаем один раз
uint32_t blink_time; //Мигаем в режиме калибровки

int Shtora1;
int Shtora2;

const int ton = 16; //пищалка

// чтение
int EEPROM_int_read(int addr) {
  byte raw[2];

  for(byte i = 0; i < 2; i++) {
    raw[i] = EEPROM.read(addr+i);
  }

  int &num = (int&)raw;
  return num;
}

// запись
void EEPROM_int_write(int addr, int num) {
  if (EEPROM_int_read(addr)!= num) {//если сохраняемое отличается
    byte raw[2];
    (int&)raw = num;
    for(byte i = 0; i < 2; i++) EEPROM.write(addr+i, raw[i]);
  }
}

void setup() {
  // DDRB пин, 8,9,10,11 - Шаговик, pin 12- вход, 13- Выход
  DDRB = DDRB | B00101111;

  // pin 8,9,10,11,13 - выход в 0 а входы pin 12, - 0
  PORTB = B00000000;

  //  DDRD, подключенный к двигателю pin 4,5,6,7 как выход,pin2,3 Выход
  DDRD = DDRD | B11110000;

  // pin 4,5,6,7 - выход в 0 а входы pin 2,3 - 0
  PORTD = B00000000;//;

  D14_Out;  // Led_1
  D14_Low;

  D15_Out;  // Led_2
  D15_Low;

  pinMode(ton, OUTPUT); //пищалка

  Serial.begin(9600);

  // Инициализировать TIMER1

  StartTimer1(Time_Closk, 2200);

  Shtora1 = EEPROM_int_read(0);

  Shtora2 = EEPROM_int_read(2);
}

void Beep() //короткий звук, переключение шторы
{
  tone(ton,500,500);
  delay(200);
  noTone(ton);
}

void Beep_long()  //длинный звук, вход и выход из режима калибровки "0"
{
  tone(ton,200,1000);
  delay(200);
  noTone(ton);
}

void Time_Closk() // Функция прерывания таймера
{
  f_timX = 1;
  f_timZ = 1;
}

//Функция вращения  dvigatel 1
void RunX (bool perem_X)
{
  f_sbrosX = 0;
  f_timX = 0;

  if (perem_X) {
    n1 = (n1 >> 1) | (n1 << 3); //Сдвигаем
    --Shtora1;
  }
  else {
    n1 = (n1 << 1) | (n1 >> 3); //Сдвигаем
    ++Shtora1;
  }

  n1 = 0x0F & n1; //Маскируем кнопки

  PORTB = (PORTB & 0xF0) | n1; //Очищаем Младшие биты и пишем туда n1
}

//Функция dvigatel 3
void RunZ (bool perem_Z)
{
  f_sbrosZ = 0;

  f_timZ = 0;

  if (perem_Z) {
    n3 = (n3 >> 1) | (n3 << 3); //Сдвигаем
    --Shtora2;
  }
  else {
    n3 = (n3 << 1) | (n3 >> 3); //Сдвигаем
    ++Shtora2;
  }

  n3 = 0xF0 & n3;//Маскируем

  PORTD = (PORTD & 0xF) | n3; //Очищаем старшие биты и пишем туда n3
}

void loop() {  
  cur_time = millis();

  ///////////////////Проверка кнопки МЕНЮ - АНТИДРЕБЕЗГ
  if (D12_Read && !f_menu)
  {
    menu_dreb = cur_time;
    f_menu = 1;
  }

  if (!D12_Read)
  {
    kn_menu = 0;
    f_menu = 0;
  }

  if (f_menu && cur_time - menu_dreb >= 20)
  {
    kn_menu = 1;

    if (!f_Mpush) {
      menu_klik = cur_time;
      f_Mpush = 1;
    }
  }

//Был клик

if (f_Mpush && !kn_menu && cur_time - menu_klik < 1000) {
   if (_UP || _Down) {
    _UP = 0;
    _Down = 0;
  }
  else Menu++, Beep();     // Меняем

  if (Menu > 3) Menu = 1;
  f_Mpush = 0;
}

//Было нажатие

else if (f_Mpush && !kn_menu && cur_time - menu_klik > 1000) {
  Beep_long();
  kalib = !kalib; // Включаем калибровку
  f_Mpush = 0;
}  ///////////////////////Проверка кнопки UP - АНТИДРЕБЕЗГ

  if (D2_Read != f_Up) { //Если сигнал изменился
    f_Up = !f_Up;
    Up_dreb = cur_time;
  }
  if (kn_Up == D2_Read && cur_time - Up_dreb > 20) kn_Up = !D2_Read;

  // Было нажатие
  if (kn_Up && !state_Up) {
    if (_UP || _Down) {
      _UP = 0;
      _Down = 0;
      state_Up = 1;
    }
    else {
      _UP = !_UP;    // Меняем значение
      state_Up = 1;
    }
  }

  if (!kn_Up)state_Up = 0; // Отпустили
  ///////////////////////Проверка кнопки DOWN - АНТИДРЕБЕЗГ

  if (D3_Read != f_Down) { //Если сигнал изменился
    f_Down = !f_Down;
    Down_dreb = cur_time;
  }

  if (kn_Down == D3_Read && cur_time - Down_dreb > 20) kn_Down = !D3_Read;

  // Было нажатие

  if (kn_Down && !state_Down) {
    if (_UP || _Down) {
      _UP = 0;
      _Down = 0;
      state_Down = 1;
    }
    else {
      _Down = !_Down;  // Меняем значение
      state_Down = 1;
    }
  }

  if (!kn_Down)state_Down = 0; // Отпустили

  //////////////////////// С кнопками закончили продолжаем логику
  if (!kalib)
  {
    if (Menu != temp_Menu) {
      temp_Menu = Menu;
      switch (Menu) {
        case 1: D14_High; D15_Low; break;
        case 2: D14_Low; D15_High; break;
        case 3: D14_High; D15_High; break;
      }
    }
  }
  else {
    if (cur_time - blink_time >= 300)
    {
      blink_time = cur_time;

      switch (Menu) {
        case 1: D14_Inv; D15_Low; break;
        case 2: D14_Low; D15_Inv; break;
        case 3: D14_Inv; D15_Inv; break;
      }
    }
  }

  /////////////////////////// Запуск 1го двигателя если " perem_X != 0 "

  if (f_timX) {
    if (_UP || _Down) {
      if (Menu != 2) {
        if (!sped_X) {
          if (!kalib) { //Калибровка отк.
            if (_Down && Shtora1 < max_Sht1) {
              RunX(0);
            }

            if (_UP && Shtora1 > 0) {
              RunX(1);
            }
          }
          else { //Калибровка вкл.
            if (_Down) {
              Shtora1 = 0;
              RunX(0);
            }
            if (_UP) {
              Shtora1 = 1;
              RunX(1);
            }
          }

          //Три поверки для разгона
          if (sped_Xt > 0) {
            --sped_Xt;
            sped_X = sped_Xt;

            // f_timX = 0;
          }
        }
        else {
          --sped_X;
          f_timX = 0;
        }
      }
    }////

    else {
      sped_Xt = uskor;
      sped_X = 0;
    }
  }

  else { //Для снятия напряжения
    f_run1X = 1;

    no_runX = cur_time;
  }

  /////////////////Запуск 3го двигателя если " perem_Z != 0 "
  if (f_timZ) {
    if (_UP || _Down) {
      if (Menu != 1) {
        if (!sped_Z) {
          if (!kalib)//Калибровка отк.
          {
            if (_UP && Shtora2 < max_Sht2) {
              RunZ(0);
            }
            if (_Down && Shtora2 > 0) {
              RunZ(1);
            }
          }
          else {//Калибровка Вкл.
            if (_UP) {
              Shtora2 = 0;
              RunZ(0);
            }
            if (_Down) {
              Shtora2 = 1;
              RunZ(1);
            }
          }

          //Три поверки для разгона
          if (sped_Zt > 0) {
            --sped_Zt;
            sped_Z = sped_Zt;
            f_timZ = 0;
          }
        }
        else {
          --sped_Z;
          f_timZ = 0;
        }
      }
    }////
    else {
      sped_Zt = uskor;
      sped_Z = 0;
    }
  }
  else { //Для снятия напряжения
    f_run1Z = 1;
    no_runZ = cur_time;
  }

  //Возврат по достижению конечной точки
  if (f_sbrosZ && f_sbrosX) {
    _UP = 0;
    _Down = 0;
  }
  //Снимаем напряжение с катушек двигателя X
  if (f_run1X && cur_time - no_runX >= 1000)
  {
    f_run1X = 0;
    PORTB &= 0xF0;
    f_sbrosX = 1;
    if (EEPROM_int_read(0)!=Shtora1)
    {
     EEPROM_int_write(0, Shtora1); // адрес 0 (+2 байта)
    }
  }

  //Снимаем напряжение с катушек двигателя Z
  if (f_run1Z && cur_time - no_runZ >= 1000)
  {
    f_run1Z = 0;

    PORTD &= 0xF;
    f_sbrosZ = 1;
    if (EEPROM_int_read(2)!=Shtora2)
    {
     EEPROM_int_write(2, Shtora2); // адрес 0 (+2 байта)
    }
  }
}



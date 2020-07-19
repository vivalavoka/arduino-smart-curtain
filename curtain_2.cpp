#include <EEPROM.h>

#include <SoftwareSerial.h>//для порта
 
// Определение пинов для управления двигателем
#define motorPin1  22 // IN1 на 1-м драйвере ULN2003
#define motorPin2  23 // IN2 на 1-м драйвере ULN2003
#define motorPin3  24 // IN3 на 1-м драйвере ULN2003
#define motorPin4  25 // IN4 на 1-м драйвере ULN2003
#define motorPin5  26 // IN1 на 2-м драйвере ULN2003
#define motorPin6  27 // IN2 на 2-м драйвере ULN2003
#define motorPin7  28 // IN3 на 2-м драйвере ULN2003
#define motorPin8  29 // IN4 на 2-м драйвере ULN2003

#define kn_menu 9   //кнопка меню (выбор шторы)
#define kn_open 10  //кнопка "открыть"
#define kn_close 11 //кнопка "закрыть"

const int led1 = 7; //индикатор шторы 1
const int led2 = 6; //индикатор шторы 2

const int ton = 12;//пищалка
//Таймеры
long previousMillisSerial = 0; // время, когда состояние обновлялось
long intervalSerial = 1000; // интервал для вывода данных

//Переменные
int reg=1;  //Режим работы штор (1 или 2 штора)
int reg_k=0;  //Режим калибровки "0" (1 или 2 штора)
int flag_o=0; // чтобы исключить ошибочное нажатие кнопки "вверх"
int flag_c=0; // чтобы исключить ошибочное нажатие кнопки "вниз"
int motorSpeed = 1000; // пауза между шагами двигателя, чем меньше, тем быстрей скорость (1000 оптимально)
boolean start = false;  //если "True" двигатель работает, иначе остановлен
boolean kalibr=false; // если "True" включена калибровка "0"
bool     button_state      = false; //для короткого нажатия кнопки "меню"
bool     button_long_state = false; //для длинного нажатия кнопки "меню"
uint32_t ms_button = 0; // таймер для отслеживания длительности нажатия кнопки "меню"
// переменные для счета шагов двигателя
int r=0;
int pos1=0;
int max1=7185;// количество шагов для полного закрытия шторы 1
int min1=0;        
int pos2=0;
int max2=6950;// количество шагов для полного закрытия шторы 2
int min2=0;        
//команды для управления двигателями
int lookup_1[8] = {
  B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};
int lookup_2[8] = {
  B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};

// чтение
int EEPROM_int_read(int addr) {   
  byte raw[2];
  for(byte i = 0; i < 2; i++) raw[i] = EEPROM.read(addr+i);
  int &num = (int&)raw;
  return num;
}
// запись
void EEPROM_int_write(int addr, int num) {
  if (EEPROM_int_read(addr)!= num){//если сохраняемое отличается
  byte raw[2];
  (int&)raw = num;
  for(byte i = 0; i < 2; i++) EEPROM.write(addr+i, raw[i]);
}
}

void setup(){
                             //назначение пинов для кнопок
    pinMode(kn_menu,INPUT);
    pinMode(kn_open,INPUT);
    pinMode(kn_close,INPUT);
                             //назначение пинов для светодиодов
    pinMode(led1,OUTPUT);   
    pinMode(led2,OUTPUT);   
    digitalWrite(led1,LOW);
    digitalWrite(led2,LOW);
                             //назначение пинов для двигателей
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(motorPin3, OUTPUT);
    pinMode(motorPin4, OUTPUT);
    pinMode(motorPin5, OUTPUT);
    pinMode(motorPin6, OUTPUT);
    pinMode(motorPin7, OUTPUT);
    pinMode(motorPin8, OUTPUT);

    pinMode(ton,OUTPUT);//пищалка
    Serial.begin(9600);
    if (EEPROM_int_read(4)!=1)
    {
      pos1=0;
      pos2=0;
    }
    else
    {
      pos1 = EEPROM_int_read(0);
      pos2 = EEPROM_int_read(2);
    }
}

// функция поворачивает мотор 1 против часовой стрелки.
void anticlockwise_1()
{
  for(int i = 0; i < 8; i++)
  {
    setOutput_1(i);
    delayMicroseconds(motorSpeed);
  }
}

// функция поворачивает мотор 1 по часовой стрелке.
void clockwise_1()
{
  for(int i = 7; i >= 0; i--)
  {
    setOutput_1(i);
    delayMicroseconds(motorSpeed);
  }
}
// функция поворачивает мотор 2 против часовой стрелки.
void anticlockwise_2()
{
  for(int j = 0; j < 8; j++)
  {
    setOutput_2(j);
    delayMicroseconds(motorSpeed);
  }
}

// функция поворачивает мотор 2 по часовой стрелке.
void clockwise_2()
{
  for(int j = 7; j >= 0; j--)
  {
    setOutput_2(j);
    delayMicroseconds(motorSpeed);
  }
}

 void setOutput_1(int out)
{
  digitalWrite(motorPin1, bitRead(lookup_1[out], 0));
  digitalWrite(motorPin2, bitRead(lookup_1[out], 1));
  digitalWrite(motorPin3, bitRead(lookup_1[out], 2));
  digitalWrite(motorPin4, bitRead(lookup_1[out], 3));
}

void setOutput_2(int out)
{
  digitalWrite(motorPin5, bitRead(lookup_2[out], 0));
  digitalWrite(motorPin6, bitRead(lookup_2[out], 1));
  digitalWrite(motorPin7, bitRead(lookup_2[out], 2));
  digitalWrite(motorPin8, bitRead(lookup_2[out], 3));
}


void Stop_motor_1()
{
            digitalWrite(motorPin1, LOW); //останавливаем мотор
            digitalWrite(motorPin2, LOW);
            digitalWrite(motorPin3, LOW);
            digitalWrite(motorPin4, LOW);  
}
void Stop_motor_2()
{
            digitalWrite(motorPin5, LOW); //останавливаем мотор
            digitalWrite(motorPin6, LOW);
            digitalWrite(motorPin7, LOW);
            digitalWrite(motorPin8, LOW);  
}
void Beep() //короткий звук, переключение шторы
{
  tone(ton,500,500);
  delay(200);
  noTone(ton);
}
void Beep_long()  //длинный звук, вход и выход из режима калибровки "0"
{
  tone(ton,200,1000);
  delay(200);
  noTone(ton);
}
 
void loop(){
//----------------------------------------------------------------------------------------------------------------
//Калибровка "0", Выбор шторы (1 или 2)
//----------------------------------------------------------------------------------------------------------------
   uint32_t ms    = millis();
   bool pin_state = digitalRead(kn_menu);
// Фиксируем нажатие кнопки "меню" (переключение шторы)  
   if( pin_state  == HIGH && !button_state && !kalibr && ( ms - ms_button ) > 50 ){
      button_state      = true;
      button_long_state = false;  
      ms_button         = ms;
   }
// Фиксируем длинное нажатие кнопки "меню"(вход в режим калибровки "0")  
   if( pin_state  == HIGH && !button_long_state && !kalibr && ( ms - ms_button ) > 2000 ){
      button_long_state = true;
      kalibr=true;  //включаем режим калибровки "0"
      EEPROM_int_write(4, 0); // адрес 4 (+2 байта)если калибровка не проведена, сохраняем 0
      reg=0;
      reg_k=0;
      Beep_long();
      Serial.println("Long press key");    
   }
// Фиксируем отпускание кнопки (переключение меню в работе)   
   if( pin_state == LOW && button_state && !kalibr && ( ms - ms_button ) > 50  ){
      button_state      = false;   
      ms_button         = ms;
      reg++;
      Beep();
      delay(200);
      if (reg>3)
      {reg=1;}
      if( !button_long_state )Serial.println("Press key");
   }
// Фиксируем нажатие кнопки "меню" (переключение штор в режиме калибровки)  
   if( pin_state  == HIGH && !button_state && kalibr && ( ms - ms_button ) > 50 ){
      button_state      = true;
      button_long_state = false;  
      ms_button         = ms;
   }
// Фиксируем длинное нажатие кнопки (выход из режима калибровки "0")  
   if( pin_state  == HIGH && !button_long_state && kalibr && ( ms - ms_button ) > 2000 ){
      button_long_state = true;
      kalibr=false;
      EEPROM_int_write(4, 1); // адрес 4 (+2 байта)если калибровка проведена, сохраняем 1
      reg=0;
      reg_k=0;
      Beep_long();
      pos1=0; // устанавливаем позицию шторы 1 как "0" 
      pos2=0; // устанавливаем позицию шторы 2 как "0"
      Serial.println("Long press key kalibr");    
   }
   //переключение меню в режиме калибровки "0"
   if( pin_state == LOW && button_state && kalibr && ( ms - ms_button ) > 50  ){
      button_state      = false;   
      ms_button         = ms;
      reg_k++;
      Beep();
      delay(200);
      if (reg_k>2)
      {reg_k=1;}
      if( !button_long_state )Serial.println("Press key kalibr");
   }

  
//----------------------------------------------------------------------------------------------------------------
//Выбор шторы (1 или 2)
//----------------------------------------------------------------------------------------------------------------
  if (reg==1)
    {                                                           //включаем 1 светодиод и выключаем 2
      digitalWrite(led1,HIGH);
      digitalWrite(led2,LOW);
    }
  if (reg==2)
    {                                                       //включаем 2 светодиод и выключаем 1
      digitalWrite(led2,HIGH);
      digitalWrite(led1,LOW);
    }
  if (reg==3)
    {                                                       //включаем 2 светодиода
      digitalWrite(led2,HIGH);
      digitalWrite(led1,HIGH);
    }
    if (reg_k==1)
    {                                                       //включаем 1 светодиод и выключаем 2
      digitalWrite(led1,HIGH);
      digitalWrite(led2,LOW);      
    }
  if (reg_k==2)
    {                                                       //включаем 2 светодиод и выключаем 1
      digitalWrite(led2,HIGH);
      digitalWrite(led1,LOW);
    }

//--------------------------------------------------------------------------
//Управляем двигателями в режиме работа
//--------------------------------------------------------------------------
//закрываем шторы
  if (digitalRead(kn_open)==1 && start==false && flag_o==0 && !kalibr)
      {
       flag_o=1; 
       start=true;
       delay(500);
        switch (reg)
        {
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Закрываем первую штору. Если во время движения нажимается любая кнопка, то штора останавливается, иначе закрывается до максимального значения
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 1:
          for (int r=pos1; r<max1+1;r++)
          {
          anticlockwise_1(); // Мотор 1 крутим влево.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            start=false;
            break;
          }
          else 
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
          }
          start=false;
          }
          break;
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Закрываем вторую штору. Если во время движения нажимается любая кнопка, то штора останавливается, иначе закрывается до максимального значения
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 2:
          for (int r=pos2; r<max2+1;r++)
          {
          clockwise_2(); //Мотор 2 крутим вправо.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else 
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
          start=false;
          }
          break;
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Закрываем обе шторы. Если во время движения нажимается любая кнопка, то шторы останавливаются, иначе закрываются до максимального значения.
          //Вторая штора останавливается первой, потому что для неё максимальное значение меньше чем у первой. Вторая штора продолжает движение до своего
          //максимального значения.
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 3:
          //---------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1<pos2)//если вторая штора ниже первой, то даем команду первой шторе сравняться со второй
          {
            for (int r=pos1; r<pos2+1;r++)
            {
              anticlockwise_1();
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            start=false;
            break;
          }
          else 
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 2 (+2 байта)
          }
              
            }
          }
          //---------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1>pos2)//если первая штора ниже второй, то даем команду второй шторе сравняться с первой
          {
            for (int r=pos2; r<pos1+1;r++)
            {
              clockwise_2();
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else 
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
              
            }
          }
          //---------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1==pos2)//если шторы находятся на одном уровне
          {
          for (int r=pos1; r<max2+1;r++)//даем команду на оба мотора
          {
            anticlockwise_1();
            clockwise_2();
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)//если нажата любая кнопка, останавливаем моторы
          {
            Stop_motor_1();//останавливаем мотор
            Stop_motor_2();//останавливаем мотор
            //сохраняем позиции штор
            pos1=r;
            pos2=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else 
          {
            Stop_motor_1();//останавливаем мотор
            Stop_motor_2();//останавливаем мотор
            pos1=r;
            pos2=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
            start=false;
          }
          }
          //-------------------------------------------------------------------------------------
          if (pos1==max2 || pos2==max2)//если шторы достигли максимального значения второй шторы
          {
          for (int r=pos1; r<max1+1;r++)//даем команду первому мотору на закрытие
          {
            anticlockwise_1();
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)  
            start=false;
            break;
          }
          else 
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
          }
            start=false;
          }
            
          }
          break;
        }
      }
  if (digitalRead(kn_open)==0 && flag_o==1 && !kalibr)
    {
      flag_o=0;
      flag_c=0;
      delay(500);
    }


//открываем шторы
  if (digitalRead(kn_close)==1 && start==false && flag_c==0 && !kalibr)
      {
       flag_c=1; 
       start=true;
       delay(500);
        switch (reg)
        {
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Открываем первую штору. Если во время движения нажимается любая кнопка, то штора останавливается, иначе открывается до минимального значения
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 1:
          for (int r=pos1; r>min1-1;r--)
          {
          clockwise_1(); // Мотор 1 крутим вправо.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            start=false;
            break;
          }
          else
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
          }
            start=false;
          }
          break;
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Открываем вторую штору. Если во время движения нажимается любая кнопка, то штора останавливается, иначе открывается до минимального значения
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 2:
          for (int r=pos2; r>min2-1;r--)
          {
          anticlockwise_2(); //Мотор 2 крутим влево.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
            start=false;
          }
          break;
          //---------------------------------------------------------------------------------------------------------------------------------------------
          //Открываем обе шторы. Если во время движения нажимается любая кнопка, то шторы останавливаются, иначе открываются до минимального значения.
          //Первая штора сравнивается со второй и потом обе открываются одновременно до минимального значения.
          //---------------------------------------------------------------------------------------------------------------------------------------------
          case 3:
          //---------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1>pos2)
          {
          for (int r=pos1; r>pos2-1;r--)
          {
          clockwise_1(); // Мотор 1 крутим вправо.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            start=false;
            break;
          }
          else
          {
            Stop_motor_1();//останавливаем мотор
            pos1=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
          }
            start=false;
          }
          }
          //-------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1<pos2)
          {
          for (int r=pos2; r>pos1-1;r--)
          {
          anticlockwise_2(); //Мотор 2 крутим влево.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else
          {
            Stop_motor_2();//останавливаем мотор
            pos2=r;
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
            start=false;
          }
            
          }
          //-------------------------------------------------------------------------------------------------------------------------------------------
          if (pos1==pos2)
          {
          for (int r=pos1; r>min1-1;r--)
          {
          clockwise_1(); // Мотор 1 крутим вправо.
          anticlockwise_2(); //Мотор 2 крутим влево.
          if (digitalRead(kn_open)==1 || digitalRead(kn_close)==1 || digitalRead(kn_menu)==1)
          {
            Stop_motor_1();//останавливаем мотор
            Stop_motor_2();//останавливаем мотор
            pos1=r;
            pos2=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
            start=false;
            break;
          }
          else
          {
            Stop_motor_1();//останавливаем мотор
            Stop_motor_2();//останавливаем мотор
            pos1=r;
            pos2=r;
            EEPROM_int_write(0, pos1); // адрес 0 (+2 байта)
            EEPROM_int_write(2, pos2); // адрес 2 (+2 байта)
          }
            start=false;
          }
            
          }
          //-------------------------------------------------------------------------------------------------------------------------------------------
          break;
        }
      }
  if (digitalRead(kn_close)==0 && flag_c==1 && !kalibr)
    {
      flag_o=0;
      flag_c=0;
      delay(500);
    }


//--------------------------------------------------------------------------
//Управляем двигателями в режиме калибровка
//--------------------------------------------------------------------------
//закрываем шторы
  if (digitalRead(kn_open)==1 && kalibr && reg_k==1)  //пока кнопка "вверх" нажата в режиме калибровки "0", поднимаем штору 1
      {
          anticlockwise_1(); // Мотор 1 крутим влево.
          }
  if (digitalRead(kn_open)==1 && kalibr && reg_k==2)  //пока кнопка "вверх" нажата в режиме калибровки "0", поднимаем штору 2
      {
          clockwise_2(); // Мотор 2 крутим вправо.
          }
  if (digitalRead(kn_close)==1 && kalibr && reg_k==1) //пока кнопка "вниз" нажата в режиме калибровки "0", опускаем штору 1
      {
          clockwise_1(); // Мотор 1 крутим вправо.
          }
  if (digitalRead(kn_close)==1 && kalibr && reg_k==2) //пока кнопка "вверх" нажата в режиме калибровки "0", опускаем штору 2
      {
          anticlockwise_2(); // Мотор 2 крутим влево.
          }
  if (digitalRead(kn_close)==0 && digitalRead(kn_open)==0 && kalibr)  //если обе кнопки отжаты, останавливаем двигатели
  {
            Stop_motor_1();//останавливаем мотор
            Stop_motor_2();//останавливаем мотор
          }


//  вывод информации в порт (для отслеживания позиций двигателей в режиме отладки и подсчета максимальных позиций штор)
unsigned long currentMillisSerial = millis();                       // текущее время в миллисекундах
  
  if(currentMillisSerial - previousMillisSerial > intervalSerial)
  {
                                                                 // сохраняем последний момент, когда делался замер
    previousMillisSerial = currentMillisSerial;
     Serial.begin(9600);
     Serial.print("Позиция_1 ");
     Serial.print(pos1);
     Serial.print(" Позиция_2 ");
     Serial.print(pos2);
     Serial.print(" Ячейка_1 ");
     Serial.print(EEPROM_int_read(0));
     Serial.print(" Ячейка_2 ");
     Serial.print(EEPROM_int_read(2));
     Serial.print(" Калибровка ");
     Serial.println(EEPROM_int_read(4));

  }    
}
[/code]
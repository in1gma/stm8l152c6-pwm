#include "iostm8l152c6.h"
#include <intrinsics.h>
#include <inttypes.h>
#include <math.h>

#define FClock 16000000 // значение частоты процессора

// значения по умолчанию
uint16_t _frequency = 20; // частота
uint32_t _counter = 0; // счетчик
uint32_t _pwm = 0; // шим
uint8_t _pressed = 0; // кнопка
double _percentage = 20.0; // 1/процент

// инициализируем выходы шима
void int_out()
{
  PD_DDR_bit.DDR4 = 1;
  PD_CR1_bit.C14 = 1;
  PD_CR2_bit.C24 = 1;
  PD_ODR_bit.ODR4 = 0;
}

void init_button()
{
  // кнопки вниз PA7; вверх PA6
  PA_CR1_bit.C16 = 1; // порт в pull-push
  PA_CR1_bit.C17 = 1; // порт в pull-push
}

// функция задает коэффициенты таймера по заданной частоте
int set_frequency(uint16_t frequency)
{
  uint32_t current_clock_frequency = static_cast<uint32_t>(FClock)/static_cast<uint16_t>(frequency);

  uint16_t d_max = 65535;
  uint16_t c_max = 65535;
  
  // ищем коэффиценты подбором
  for (uint16_t divider = 1; divider <= d_max; divider++)
  {
    double tmp = static_cast<double>(current_clock_frequency)/static_cast<double>(divider);
    if (floor(tmp) == tmp && tmp <= c_max)
    {
      uint32_t counter = static_cast<uint32_t> (tmp);
      _counter = counter;
      
      // записываем коэффициенты в регистры
      TIM1_PSCRH = divider >> 8;
      TIM1_PSCRL = divider & 0xFF;
      
      TIM1_ARRH = counter >> 8;
      TIM1_ARRL = counter & 0xFF;
      
      // шим по умолчанию 50%
      uint16_t pwm = static_cast<uint32_t>(counter/2);
      _pwm = pwm;
      
      TIM1_CCR2H = pwm >> 8;
      TIM1_CCR2L = pwm & 0xFF;
      
      return 0;
    }
  }
  
  return 1;
}

// инициализация clock
void clock_init()
{
  CLK_CKDIVR = 0; // предделитель 0
  CLK_PCKENR2 = 0xFF; 
}

// инициализация таймера
void init_timer()
{
    set_frequency(_frequency); // задаем частоту
    
    // настройки шима
    TIM1_BKR = 0x80; // регистр остановки
    TIM1_CCMR2 = 0x68; // регистр сравнения
    TIM1_CCER1_bit.CC2P = 0; 
    TIM1_CCER1_bit.CC2E = 1; // канал 2
}

// запуск таймера
void timer_start()
{
  TIM1_IER_bit.UIE = 1; // Разрешаем прерывание по переполнению
  TIM1_CR1_bit.CEN = 1; // Запускаем таймер
}

// увеличиваем значение шима в %
void set_pwm(double percentage)
{
    // прибавляем
    int32_t tmp = _pwm + static_cast<int32_t>(_counter/percentage);
    
    // проверяем границы
    tmp = tmp >= 0 ? tmp : 0;
    _pwm = tmp <= _counter ? tmp : _counter;
    
    // пишем в регистры
    TIM1_CCR2H = _pwm >> 8;
    TIM1_CCR2L = _pwm & 0xFF;
}

void pressed()
{
  // PA7 0x83
  // PA6 0x43
  // PA6 && PA7 0x??

  // проверяем нажатие кнопки
  if (!(PA_IDR_bit.IDR7) || !(PA_IDR_bit.IDR6)) // какая-то кнопка нажата
  {
    _pressed = PA_IDR;
  }
  else // отжата
  {
    // если была нажата
    if (_pressed != 0)
    { 
      // выбираем по нажатой кнопке действие
      switch(_pressed)
      {
        case 0x83: // PA6
          {
            set_pwm(_percentage);
            break;
          }
        case 0x43: // PA7
          {
            set_pwm(-1*_percentage);
            break;
          }
      }
      
      _pressed = 0;
    }
  }
}

// main
int main()
{
  // запускм все фукнции
  clock_init(); 
  int_out();
  init_button();
  
  init_timer();
  timer_start();
  
  // в цикле проверяем нажатие кнопки
  while (1)
  {
    __no_operation();
    pressed();
  }
  
  return 0;
}
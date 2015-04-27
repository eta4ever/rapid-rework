#include <avr/io.h>
#include <util/delay.h>
#define F_CPU 1200000UL

// Минимальная и максимальная задержки ударного импульса
#define MINDELAY 3
#define MAXDELAY 5

// Педаль на размыкание подключена к PB4. Активный уровень - низкий.
#define PEDAL PB4

// Выход - PB3. Активный уровень - высокий.
#define FIRE PB3 

void delaymsMod(unsigned char ms)
// Компилятор хочет, чтобы аргументом _delay_ms была константа.
// Поэтому приходится делать велокостыль.
{
	for (unsigned char counter=0; counter<ms; counter++)
	{
		_delay_ms(1);
	}
}


unsigned char fireDelay(void)
// Отключить компаратор. Включить АЦП. Получить уставку с переменного резистора.
// Вычислить время, которое будет между переходом через 0 и подачей ударного импульса
// Чем меньше это время, тем больший участок полупериода подается на электромагнит,
// соответственно, сильнее удар. Выключить АЦП.
{
	// ACD - отключение аналогового компаратора
	ACSR |= (1 << ACD); 

	// бит 5 - ADLAR, ориентация данных. Чтобы использовать только 8 бит ADCH
	// биты 0-1 - настройка входа. 01 соотв. ADC1 (PB2)
	ADMUX |= 0b00100001;  

	// ADPS2-1-0 задает делитель тактирования АЦП. 011 соотв. делителю 8
	// Результирующая частота АЦП 150 кГц (1.2 МГц / 8)
	ADCSRA |= (1 << ADPS1) | (1 << ADPS0);
	//ADCSRA &= (0 << ADPS2);

	// Включить АЦП
	ADCSRA |= (1 << ADEN);

	// Начать преобразование
	ADCSRA |= (1 << ADSC);

	// Подождать, пока не сбросится бит преобразования
	while (ADCSRA & (1 << ADSC));

	// Получить 8 бит данных
	unsigned char rawADC = ADCH;

	// Отключить АЦП
	ADCSRA &= ~(1 << ADEN);

	// Вычислить задержку
	return MINDELAY + rawADC * (MAXDELAY - MINDELAY) / 255;
}

void waitForZero(void)
// Включить компаратор. Дождаться перехода через ноль. Выключить компаратор.
{
	ACSR &= ~(1 << ACD);
	_delay_ms(1);
	while ((ACSR & 0b00100000) == 0b00100000);
	ACSR |= (1 << ACD);
}

unsigned char pedal(void)
// Проверить нажатие педали. С подавлением дребезга. Педаль работает на размыкание.
{
	unsigned char pressed = 0; // флаг нажатия
	
	// время, в течение которого должно не прерываться нажатие
	unsigned char pressedTime = 100; 

	if (PINB & (1 << PEDAL)) // педаль нажата, цепь разомкнута, на входе 1
	{
		pressed = 1;

		for (unsigned char counter=0; counter < pressedTime; counter++)
		{
			
			// если в течение заданного интервала педаль отпущена, цепь
			// замкнута, на входе 0 - сбросить флаг нажатия
			if ( !(PINB & (1 << PEDAL)))
			{
				pressed = 0;
			}

			_delay_ms(1);
		}

	}

	return pressed;
}

void fire(void)
// ударный импульс длительностью 1 мс
{
	PORTB |= (1 << FIRE);
	_delay_ms(1);
	PORTB &= ~(1 << FIRE);
}

void setup(void)
{
	// режим порта
	DDRB=0b00001000;
}

int main(void)
{
	setup();

	while (1)
	{
		if (pedal())
		{
			waitForZero();
			delaymsMod(fireDelay());
			fire();

			while(pedal());
		}

	}
	return -1; // Никогда
}
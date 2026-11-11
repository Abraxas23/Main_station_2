#include "ds18b20_RTOS.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;

// переменные под Dallas
uint8_t dt_1[8];
uint16_t raw_temper_1;
int ID_dallas;
uint8_t status = 0;

#define resolution_9bit  0x1F          //    0.5C ,    100ms
#define resolution_10bit 0x3F		   //    0.25C ,   190ms
#define resolution_11bit 0x5F          //    0.125C ,  380ms
#define resolution_12bit 0x7F          //    0.0625C , 750ms

//  АДРЕС КАК СЧИТЫВАЕТСЯ ПО ПОРЯДКУ ТАК СЮДА И ПИШЕМ


uint8_t  D1[8] = {0x28, 0x84, 0xA2, 0x2D, 0x06, 0x00, 0x00, 0xA4};  //  1
uint8_t  D2[8] = {0x28, 0x10, 0xAD, 0xAB, 0x0D, 0x00, 0x00, 0xB7};  //  2
uint8_t  D3[8] = {0x28, 0x78, 0xCD, 0xAA, 0x0D, 0x00, 0x00, 0x4B};  //  3

// -----------------------------------------------

float DS18B20_ReadOne(uint8_t *Dx);   // Чтение по ROM

void DallasPort_Init()
{
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
	GPIOA->CRL |= GPIO_CRL_MODE1;
	GPIOA->CRL |= GPIO_CRL_CNF1_0;
	GPIOA->CRL &= ~GPIO_CRL_CNF1_1;
}

void Dallas_writebit(uint8_t bit)
{
	//DQ_0();
	GPIOA->ODR &= ~GPIO_ODR_ODR1; // low
	Delay_us(bit ? 3:65);
	//DQ_1();
	GPIOA->ODR |= GPIO_ODR_ODR1; // high
	Delay_us(bit ? 65:3);
}

void Dallas_writebyte(uint8_t dt)
{
	for(uint8_t i=0; i<8; i++)
	{
		Dallas_writebit(dt >> i&1);
		Delay_us(5);
	}
}

uint8_t Dallas_read_BIT(void)
{
	uint8_t bit = 0;
	//DQ_0();
	GPIOA->ODR &= ~GPIO_ODR_ODR1; // low
	Delay_us(2);
	//DQ_1();
	GPIOA->ODR |= GPIO_ODR_ODR1; // high
	Delay_us(13);
	bit = (GPIOA-> IDR & GPIO_IDR_IDR1 ? 1:0);
	Delay_us(45);
	return bit;
}

uint8_t Dallas_read_BYTE(void)
{
	uint8_t data = 0;
	for(uint8_t i=0; i<=7; i++)
	{
		data += Dallas_read_BIT() << i;
	}
	return data;
}


uint8_t Dallas_presence(void)  // Старт диалога (типа сброс)
{
	uint16_t status;
	//DQ_0();
	GPIOA->ODR &= ~GPIO_ODR_ODR1; // low
	Delay_us(500);
	//DQ_1();
	GPIOA->ODR |= GPIO_ODR_ODR1; // high
	Delay_us(65);
	status = GPIOA->IDR & GPIO_IDR_IDR1;  // проверяем уровень на пине
	Delay_us(500);
	return (status ? 1:0); //
}

uint8_t Init_Dallas_12bit()
{
	if(Dallas_presence()) return 1;

		Dallas_writebyte(0xCC); // обращение к единственному устр на линии
		Dallas_writebyte(0x4E); // Запись в регистры
		Dallas_writebyte(0x7D); // верх порог тревоги 125
		Dallas_writebyte(0x9E); // нижн порог тревоги -30
		Dallas_writebyte(resolution_12bit); // разрешение

	return 0;
}

uint8_t Init_Dallas_10bit()
{
	if(Dallas_presence()) return 1;

		Dallas_writebyte(0xCC); // обращение к единственному устр на линии
		Dallas_writebyte(0x4E); // Запись в регистры
		Dallas_writebyte(0x7D); // верх порог тревоги 125     0x64 - 100 град
		Dallas_writebyte(0x9E); // нижн порог тревоги -30
		Dallas_writebyte(resolution_10bit); // разрешение

	return 0;
}

void Dallas_ReadROM()
{
	Dallas_presence();
    Dallas_writebyte(0x33);	// Считываем ID единственного датчика на линии.
}


void Dallas_Read_mesure(uint8_t *Data)
{
	uint8_t i;
	Dallas_presence();

    Dallas_writebyte(0xCC);

	Dallas_writebyte(0xBE);
	for(i=0; i<8; i++)
	{
		Data[i] = Dallas_read_BYTE();
	}
}

void Dallas_Read_mesure_ONE_ID(uint8_t *Data, uint8_t *Dx)
{
	Dallas_presence();

		Dallas_writebyte(0x55);       // MATCH ROM
		for(uint8_t i=0; i<8; i++)
		{
			Dallas_writebyte(Dx[i]);
		}

	Dallas_writebyte(0xBE);
	for(uint8_t i=0; i<8; i++)
	{
		Data[i] = Dallas_read_BYTE();
	}
}


void Dallas_Read_ID(uint8_t *Data_1)
{
	uint8_t h;

	Dallas_writebyte(0xBE);
	for(h=0; h<8; h++)
	{
		Data_1[h] = Dallas_read_BYTE();
	}
}

/*float Dallas_Convert_to_T(uint16_t dt)
{
	float t;
	t = (float) ((dt&0x07FF) >> 4);  // отбрасываем знаковые и дробные биты
	t += (float) (dt&0x000F) / 16.0f;
	//t = dt*0.0625;
	return t;
}*/

float Dallas_Convert_to_T(int16_t dt)   //  принимает ЗНАКОВОЕ целое
{
    return dt / 16.0f;
}


void DS18B20_StartAll(void)
{
	Dallas_presence();
	Dallas_writebyte(0xCC);           // SKIP ROM
	Dallas_writebyte(0x44);           //  Конвертировать Температуру
    osDelay(200);
}

float DS18B20_ReadOne(uint8_t *Dx)
{
    uint8_t dt[8];
    Dallas_Read_mesure_ONE_ID(dt, Dx);

    int16_t raw = ((int16_t)dt[1] << 8) | dt[0];
    float temp = Dallas_Convert_to_T(raw);
    return temp;
}

void DS18b20_ASK_ID()
{
	Dallas_ReadROM();
	//Delay_ms(200);
	Dallas_Read_ID(dt_1);
	raw_temper_1 = ((uint16_t) dt_1[0] << 8) | dt_1[0];
	ID_dallas = raw_temper_1;
}

void DallasThread(void *argument)
{
	DallasPort_Init();                 	   // Init_PORTfor_DAllas() + настройка разрешения
	status = Init_Dallas_10bit();


  for(;;)
  {
    DS18B20_StartAll();                    // Запуск конверсии

    float t1 = DS18B20_ReadOne(D1);
    float t2 = DS18B20_ReadOne(D2);
    float t3 = DS18B20_ReadOne(D3);

    // Отправка по UART для отладки
    char buf[128];
    sprintf(buf, "A: %.1f  B: %.1f  C: %.1f    \r\n", t1, t2, t3);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 100);

    osDelay(1170);    // Период опроса
  }
}

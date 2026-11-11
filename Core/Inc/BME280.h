#ifndef INC_BME280_H_
#define INC_BME280_H_

// Здесь управление BME280

#include "cmsis_os.h"

// BME280
void BME280_Init(void);

// Задача FreeRTOS
void I2C_Thread(void *argument);

#endif /* INC_BME280_H_ */

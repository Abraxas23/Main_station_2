#ifndef INC_SX1278_H_
#define INC_SX1278_H_

#include "stm32f1xx_hal.h"

// Адрес регистра версии микросхемы (для проверки коннекта)
#define SX1278_REG_VERSION    0x42

// Ожидаемая версия SX1278          (для проверки коннекта)
#define SX1278_VERSION        0x12

// Инициализация аппаратного сброса
void SX1278_Reset(void);

// Чтение регистра
HAL_StatusTypeDef SX1278_ReadRegister(uint8_t address,
                                      uint8_t *data);

// Запись регистра
HAL_StatusTypeDef SX1278_WriteRegister(uint8_t address,
                                       uint8_t data);

#endif /* INC_SX1278_H_ */

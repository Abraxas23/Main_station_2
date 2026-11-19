
#include "sx1278.h"

// SPI1, созданный CubeMX
extern SPI_HandleTypeDef hspi1;


// Управление NSS
#define SX1278_NSS_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define SX1278_NSS_HIGH() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

// Управление RESET
#define SX1278_RESET_LOW()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET)
#define SX1278_RESET_HIGH() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET)


/* Аппаратный сброс SX1278 */
void SX1278_Reset(void)
{
    SX1278_RESET_LOW();

    HAL_Delay(10);

    SX1278_RESET_HIGH();

    HAL_Delay(10);
}


/* Чтение регистра SX1278 */
HAL_StatusTypeDef SX1278_ReadRegister(uint8_t address,
                                      uint8_t *data)
{
    if (data == NULL)
        return HAL_ERROR;

    uint8_t tx[2];
    uint8_t rx[2];

    // MSB = 0 означает чтение для SX1278
    tx[0] = address & 0x7F;
    tx[1] = 0x00;

    SX1278_NSS_LOW();

    HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(&hspi1,
                                tx,
                                rx,
                                2,
                                100);

    SX1278_NSS_HIGH();

    if (status == HAL_OK)
    {
        *data = rx[1];
    }

    return status;
}


/* Запись регистра SX1278 */
HAL_StatusTypeDef SX1278_WriteRegister(uint8_t address,
                                       uint8_t data)
{
    uint8_t tx[2];

    // MSB = 1 означает запись для SX1278
    tx[0] = address | 0x80;
    tx[1] = data;

    SX1278_NSS_LOW();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1,
                         tx,
                         2,
                         100);

    SX1278_NSS_HIGH();

    return status;
}

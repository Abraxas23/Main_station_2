#ifndef INC_SX1278_H_
#define INC_SX1278_H_

#include "stm32f1xx_hal.h"


/* ============================================================
 *                  НАСТРОЙКИ ПОДКЛЮЧЕНИЯ
 * ============================================================ */

// SPI для общения с SX1278
extern SPI_HandleTypeDef hspi1;

// NSS — выбор радиочипа (PA4)

// RESET — аппаратный сброс радиочипа (PB0)

// DIO0 — прерывание от радиочипа (PB1)


/* ============================================================
 *                  ОСНОВНЫЕ РЕГИСТРЫ LoRa
 * ============================================================ */

// FIFO
#define SX1278_REG_FIFO                  0x00

// Управление режимом работы
#define SX1278_REG_OP_MODE               0x01

// Настройка частоты
#define SX1278_REG_FRF_MSB               0x06
#define SX1278_REG_FRF_MID               0x07
#define SX1278_REG_FRF_LSB               0x08

// Настройка передатчика
#define SX1278_REG_PA_CONFIG             0x09
#define SX1278_REG_PA_RAMP               0x0A
#define SX1278_REG_OCP                   0x0B
#define SX1278_REG_LNA                   0x0C

// FIFO и приём
#define SX1278_REG_FIFO_ADDR_PTR         0x0D
#define SX1278_REG_FIFO_TX_BASE_ADDR     0x0E
#define SX1278_REG_FIFO_RX_BASE_ADDR     0x0F
#define SX1278_REG_FIFO_RX_CURRENT_ADDR  0x10

// Прерывания
#define SX1278_REG_IRQ_FLAGS_MASK        0x11
#define SX1278_REG_IRQ_FLAGS             0x12

// Информация о принятом пакете
#define SX1278_REG_RX_NB_BYTES           0x13
#define SX1278_REG_PKT_SNR_VALUE         0x19
#define SX1278_REG_PKT_RSSI_VALUE        0x1A
#define SX1278_REG_RSSI_VALUE            0x1B

// Настройки LoRa-модема
#define SX1278_REG_MODEM_CONFIG_1        0x1D
#define SX1278_REG_MODEM_CONFIG_2        0x1E
#define SX1278_REG_SYMB_TIMEOUT_LSB      0x1F

// Преамбула и длина пакета
#define SX1278_REG_PREAMBLE_MSB          0x20
#define SX1278_REG_PREAMBLE_LSB          0x21
#define SX1278_REG_PAYLOAD_LENGTH        0x22

// Дополнительная конфигурация
#define SX1278_REG_MODEM_CONFIG_3        0x26
#define SX1278_REG_SYNC_WORD             0x39

// Версия микросхемы
#define SX1278_REG_VERSION               0x42


/* ============================================================
 *                  КОНСТАНТЫ SX1278
 * ============================================================ */

// Ожидаемая версия SX1278
#define SX1278_VERSION                  0x12

// Опорная частота кварца радиочипа, Гц
#define SX1278_XTAL_FREQ                32000000UL


/* ============================================================
 *                  РЕЖИМЫ РАБОТЫ
 * ============================================================ */

// Младшие три бита RegOpMode определяют режим работы.
// Старшие биты будут формироваться отдельно.

typedef enum
{
    SX1278_MODE_SLEEP = 0,
    SX1278_MODE_STDBY,
    SX1278_MODE_FSTX,
    SX1278_MODE_TX,
    SX1278_MODE_FSRX,
    SX1278_MODE_RX_CONTINUOUS,
    SX1278_MODE_RX_SINGLE,
    SX1278_MODE_CAD

} SX1278_Mode_t;


/* ============================================================
 *              ИНДЕКСЫ ПОЛОСЫ ПРОПУСКАНИЯ LoRa
 * ============================================================ */

// Значения соответствуют битам BW[3:0] регистра RegModemConfig1.
// Тут не частота, а код выбора полосы пропускания.

typedef enum
{
    SX1278_BW_7_8_KHZ   = 0,
    SX1278_BW_10_4_KHZ  = 1,
    SX1278_BW_15_6_KHZ  = 2,
    SX1278_BW_20_8_KHZ  = 3,
    SX1278_BW_31_25_KHZ = 4,
    SX1278_BW_41_7_KHZ  = 5,
    SX1278_BW_62_5_KHZ  = 6,
    SX1278_BW_125_KHZ   = 7,
    SX1278_BW_250_KHZ   = 8,
    SX1278_BW_500_KHZ   = 9

} SX1278_Bandwidth_t;


/* ============================================================
 *                  КОНФИГУРАЦИЯ РАДИОКАНАЛА
 * ============================================================ */

typedef struct
{
    uint32_t frequency;              // Частота, Гц

    uint8_t spreading_factor;        // SF7 ... SF12

    SX1278_Bandwidth_t bandwidth;    // Полоса пропускания

    uint8_t coding_rate;             // 1 = 4/5, 2 = 4/6, 3 = 4/7, 4 = 4/8

    uint16_t preamble_length;        // Длина преамбулы

    uint8_t sync_word;               // Синхрослово

    uint8_t crc_enable;              // 1 = CRC включён

} SX1278_Config_t;




/* ============================================================
 *                  ПУБЛИЧНЫЕ ФУНКЦИИ
 * ============================================================ */

// Аппаратный сброс
void SX1278_Reset(void);

// Чтение и запись регистров
HAL_StatusTypeDef SX1278_ReadRegister(uint8_t address, uint8_t *data);

HAL_StatusTypeDef SX1278_WriteRegister(uint8_t address, uint8_t data);

// Переключение режима работы
HAL_StatusTypeDef SX1278_SetMode(SX1278_Mode_t mode);

// Настройка частоты
HAL_StatusTypeDef SX1278_SetFrequency(uint32_t frequency);

// Инициализация радиомодуля
HAL_StatusTypeDef SX1278_Init(const SX1278_Config_t *config);



#endif /* INC_SX1278_H_ */

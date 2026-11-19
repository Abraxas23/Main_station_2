
#include "sx1278.h"
#include "main.h"

// SPI1
extern SPI_HandleTypeDef hspi1;

/* УПРАВЛЕНИЕ РЕЖИМАМИ SX1278 */

// Базовая конфигурация RegOpMode:

// Bit 7 = 1 — LoRa Mode
// Bit 3 = 1 — Low Frequency Mode (433 МГц)

// 1000 1000 = 0x88

#define SX1278_OPMODE_LORA_BASE    0x88


// Управление NSS
#define SX1278_NSS_LOW()  HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_RESET)
#define SX1278_NSS_HIGH() HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET)

// Управление RESET
#define SX1278_RESET_LOW()  HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_RESET)
#define SX1278_RESET_HIGH() HAL_GPIO_WritePin(RESET_GPIO_Port, RESET_Pin, GPIO_PIN_SET)


/* Аппаратный сброс SX1278 */
void SX1278_Reset(void)
{
    SX1278_RESET_LOW();
    Delay_ms(10);

    SX1278_RESET_HIGH();
    Delay_ms(10);
}


/* Чтение регистра SX1278 */
HAL_StatusTypeDef SX1278_ReadRegister(uint8_t address, uint8_t *data)
{
    if (data == NULL)
        return HAL_ERROR;

    uint8_t tx[2];
    uint8_t rx[2];

    // MSB = 0 - чтение для SX1278
    tx[0] = address & 0x7F;
    tx[1] = 0x00;

    SX1278_NSS_LOW();

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, 100);

    SX1278_NSS_HIGH();

    if (status == HAL_OK)
    {
        *data = rx[1];
    }

    return status;
}


/* Запись регистра SX1278 */
HAL_StatusTypeDef SX1278_WriteRegister(uint8_t address, uint8_t data)
{
    uint8_t tx[2];

    // MSB = 1 - запись для SX1278
    tx[0] = address | 0x80;
    tx[1] = data;

    SX1278_NSS_LOW();

    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, tx, 2, 100);

    SX1278_NSS_HIGH();

    return status;
}

/*
  Функция: SX1278_SetMode()

  Переключение радиочипа между режимами SLEEP, STANDBY, TX, RX и CAD.

  mode — значение из SX1278_Mode_t.

 * Важно:
  Функция предназначена для работы после перевода
  микросхемы в LoRa Mode.
 */

HAL_StatusTypeDef SX1278_SetMode(SX1278_Mode_t mode)
{
    uint8_t op_mode;

    // Проверяем корректность переданного режима.
    if (mode > SX1278_MODE_CAD)
    {
        return HAL_ERROR;
    }

    // Формируем RegOpMode:

    // Старшие биты — LoRa и диапазон 433 МГц.
    // Младшие 3 бита — выбранный режим.

    // Например:
    // 0x88 | 0x01 = 0x89 (STANDBY)
    // 0x88 | 0x03 = 0x8B (TX)
    // 0x88 | 0x05 = 0x8D (RX Continuous)

    op_mode = SX1278_OPMODE_LORA_BASE | (uint8_t)mode;

    // Записываем новое состояние радиочипа.
    return SX1278_WriteRegister(SX1278_REG_OP_MODE, op_mode);
}


 /* НАСТРОЙКА РАБОЧЕЙ ЧАСТОТЫ */

/*
  Функция: SX1278_SetFrequency()  ---   frequency — требуемая частота в Герцах.

  пример:
  433000000 = 433 МГц.

  Используем опорную частоту кварца SX1278: 32 МГц.

  Полученное значение FRF записываем в три регистра: RegFrfMsb, RegFrfMid, RegFrfLsb.
 */

HAL_StatusTypeDef SX1278_SetFrequency(uint32_t frequency)
{
    uint32_t frf;

    HAL_StatusTypeDef status;

    // Расчёт FRF.

    // Используем uint64_t, поскольку при умножении частоты на 2^19 промежуточное значение может не поместиться в uint32_t.
    // Добавляем половину делителя для округления до ближайшего целого значения.

    frf = (uint32_t)((((uint64_t)frequency << 19) + (SX1278_XTAL_FREQ / 2)) / SX1278_XTAL_FREQ);

    // Старшие 8 бит FRF.
    status = SX1278_WriteRegister(SX1278_REG_FRF_MSB, (uint8_t)(frf >> 16));

    if (status != HAL_OK)
        return status;


    // Средние 8 бит FRF.
    status = SX1278_WriteRegister(SX1278_REG_FRF_MID, (uint8_t)(frf >> 8));

    if (status != HAL_OK)
        return status;


    // Младшие 8 бит FRF.
    status = SX1278_WriteRegister(SX1278_REG_FRF_LSB, (uint8_t)(frf));

    if (status != HAL_OK)
        return status;

    return HAL_OK;
}

/*
   ============================================================
   =                ИНИЦИАЛИЗАЦИЯ SX1278                      =
   ============================================================


 Функция: SX1278_Init()

  Полная первоначальная настройка радиочипа для работы в режиме LoRa.

  config — указатель на структуру с параметрами радиоканала.

  Возвращает:
 * HAL_OK    — инициализация успешна.
 * HAL_ERROR — ошибка конфигурации или версии чипа.
 * HAL_BUSY  — если SPI занят.
 * HAL_TIMEOUT — если произошёл таймаут SPI.

*/

HAL_StatusTypeDef SX1278_Init(const SX1278_Config_t *config)
{
    uint8_t version;

    uint8_t modem_config_1;
    uint8_t modem_config_2;
    uint8_t modem_config_3;

    HAL_StatusTypeDef status;



    //  1. ПРОВЕРКА ПЕРЕДАННОЙ КОНФИГУРАЦИИ

    // Проверяем, что передан корректный указатель.
    if (config == NULL)
        return HAL_ERROR;

    // Частотный диапазон самого модуля XL1278-SMT.
    if (config->frequency < 410000000UL || config->frequency > 460000000UL)
        return HAL_ERROR;

    // Для первого драйвера используем SF7...SF12.
    // SF6 требует дополнительных специальных настроек.
    if (config->spreading_factor < 7 || config->spreading_factor > 12)
        return HAL_ERROR;

    // Проверяем допустимость индекса BW.
    if (config->bandwidth > SX1278_BW_500_KHZ)
        return HAL_ERROR;

    // Coding Rate:
    // 1 = 4/5; 2 = 4/6; 3 = 4/7; 4 = 4/8

    if (config->coding_rate < 1 || config->coding_rate > 4)
        return HAL_ERROR;

    // Преамбула не должна быть нулевой.
    if (config->preamble_length == 0)
        return HAL_ERROR;

    // CRC может быть только 0 или 1.
    if (config->crc_enable > 1)
        return HAL_ERROR;


     //  2. АППАРАТНЫЙ СБРОС РАДИОЧИПА

    SX1278_Reset();


     //  3. ПРОВЕРКА ВЕРСИИ SX1278


    status = SX1278_ReadRegister(SX1278_REG_VERSION,&version);

    if (status != HAL_OK)
        return status;

    // Убеждаемся, что подключён ожидаемый радиочип.
    if (version != SX1278_VERSION)
        return HAL_ERROR;


     //  4. ПЕРЕКЛЮЧЕНИЕ FSK -> LoRa

     // После сброса радиочип находится в режиме FSK.

     // Переключение FSK/LoRa разрешено только в режиме SLEEP.

     // Сначала отправляем микросхему в FSK SLEEP.

    status = SX1278_WriteRegister(SX1278_REG_OP_MODE,0x00);

    if (status != HAL_OK)
        return status;

    Delay_ms(1);

    // Теперь устанавливаем: Bit 7 = 1 — LoRa Mode
    //                       Bit 3 = 1 — Low Frequency Mode
    //                       Bits 2:0 = 000 — SLEEP

    // 1000 1000 = 0x88

    status = SX1278_WriteRegister(SX1278_REG_OP_MODE, 0x88);

    if (status != HAL_OK)
        return status;

    Delay_ms(1);



     // 5. ПЕРЕХОД В STANDBY

    // В STANDBY можно безопасно изменять параметры радиоканала и работать с FIFO.

    status = SX1278_SetMode(SX1278_MODE_STDBY);

    if (status != HAL_OK)
        return status;



     // 6. НАСТРОЙКА РАБОЧЕЙ ЧАСТОТЫ

    status = SX1278_SetFrequency(config->frequency);

    if (status != HAL_OK)
        return status;



     // 7. ФОРМИРОВАНИЕ REG_MODEM_CONFIG1

    // RegModemConfig1:  Bits 7:4 — Bandwidth
    //                   Bits 3:1 — Coding Rate
    //                   Bit 0    — Implicit Header Mode

    // Мы используем Explicit Header, поэтому Bit 0 = 0.

    modem_config_1 =((uint8_t)config->bandwidth << 4) | (config->coding_rate << 1);

    status = SX1278_WriteRegister(SX1278_REG_MODEM_CONFIG_1, modem_config_1);

    if (status != HAL_OK)
        return status;



     // 8. ФОРМИРОВАНИЕ REG_MODEM_CONFIG2


    // RegModemConfig2:  Bits 7:4 — Spreading Factor
    //                   Bit 3    — TxContinuousMode
    //                   Bit 2    — RxPayloadCrcOn
    //                   Bits 1:0 — SymbTimeout[9:8]

    // Непрерывную передачу не используем.
    // CRC включаем согласно конфигурации.
    // Старшие биты таймаута символов пока оставим 0.

    modem_config_2 = (config->spreading_factor << 4) | (config->crc_enable << 2);

    status = SX1278_WriteRegister(SX1278_REG_MODEM_CONFIG_2, modem_config_2);

    if (status != HAL_OK)
        return status;



     // 9. ФОРМИРОВАНИЕ REG_MODEM_CONFIG3


    // RegModemConfig3:  Bit 3 — LowDataRateOptimize
    //                   Bit 2 — AgcAutoOn

    // При SF7 и BW125:  LDRO = 0.

    // Автоматическая регулировка усиления включена: AGC = 1.

    modem_config_3 = 0x04;

    status = SX1278_WriteRegister(SX1278_REG_MODEM_CONFIG_3, modem_config_3);

    if (status != HAL_OK)
        return status;



     // 10. НАСТРОЙКА ДЛИНЫ ПРЕАМБУЛЫ

    // Длина преамбулы хранится в двух регистрах.

    // RegPreambleMsb — старшие 8 бит.
    // RegPreambleLsb — младшие 8 бит.

    status = SX1278_WriteRegister(SX1278_REG_PREAMBLE_MSB, (uint8_t)(config->preamble_length >> 8));

    if (status != HAL_OK)
        return status;

    status = SX1278_WriteRegister(SX1278_REG_PREAMBLE_LSB, (uint8_t)config->preamble_length);

    if (status != HAL_OK)
        return status;



     // 11. НАСТРОЙКА SYNC WORD

    // Синхрослово используется для идентификации нужной LoRa-сети, не является шифрованием, любой у кого параметры настроены будут такие же подключиться.

    status = SX1278_WriteRegister(SX1278_REG_SYNC_WORD, config->sync_word);

    if (status != HAL_OK)
        return status;



     // 12. ПОДГОТОВКА FIFO


    // Начальные адреса TX и RX FIFO.
    // Используем начало общего буфера.

    status = SX1278_WriteRegister(SX1278_REG_FIFO_TX_BASE_ADDR, 0x00);

    if (status != HAL_OK)
        return status;

    status = SX1278_WriteRegister(SX1278_REG_FIFO_RX_BASE_ADDR, 0x00);

    if (status != HAL_OK)
        return status;

    // Устанавливаем указатель FIFO в начало.
    status = SX1278_WriteRegister(SX1278_REG_FIFO_ADDR_PTR, 0x00);

    if (status != HAL_OK)
        return status;



     // 13. НАСТРОЙКА ПРЕРЫВАНИЙ

    // RegIrqFlagsMask:  Bit = 1 — соответствующее прерывание замаскировано.

    // Пока маскируем все аппаратные IRQ.
    // Пока будем проверять флаги опросом.

    // Позже настроим DIO0 и размаскируем
    // необходимые события RxDone / TxDone.

    status = SX1278_WriteRegister(SX1278_REG_IRQ_FLAGS_MASK, 0xFF);

    if (status != HAL_OK)
        return status;



     // 14. ОЧИСТКА ФЛАГОВ ПРЕРЫВАНИЙ


    // В SX1278 флаги IRQ очищаются записью единицы в соответствующие биты.

    // Записываем 0xFF, чтобы очистить все флаги.

    status = SX1278_WriteRegister(SX1278_REG_IRQ_FLAGS, 0xFF);

    if (status != HAL_OK)
        return status;



     // 15. ЗАВЕРШЕНИЕ ИНИЦИАЛИЗАЦИИ

    // Оставляем радиочип в STANDBY. Он готов к дальнейшей работе.

    status = SX1278_SetMode(SX1278_MODE_STDBY);

    if (status != HAL_OK)
        return status;

    return HAL_OK;
}

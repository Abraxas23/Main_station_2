#include "BME280.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define BME280_ADDR    0x76
#define REG_ID         0xD0
#define REG_CTRL_HUM   0xF2
#define REG_CTRL_MEAS  0xF4
#define REG_CONFIG     0xF5
#define REG_PRESS_MSB  0xF7

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

/* ---------- Калибровочные коэффициенты ---------- */
static uint16_t dig_T1, dig_P1;
static int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;
static int32_t  t_fine;

/* ---------- Глобальные буферы для I2C ---------- */
static uint8_t calib_88[24];
static uint8_t calib_E1[7];
static char uart_buf[80];


static HAL_StatusTypeDef I2C_WriteReg(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1, (uint16_t)(BME280_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}

static HAL_StatusTypeDef I2C_ReadRegs(uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, (uint16_t)(BME280_ADDR << 1), reg, I2C_MEMADD_SIZE_8BIT, data, len, 100);
}

/* ---------- Чтение калибровки ---------- */
static void BME280_ReadCalibration(void)
{
    if (I2C_ReadRegs(0x88, calib_88, 24) != HAL_OK)
    {
        // обработка ошибки
        return;
    }

    dig_T1 = (calib_88[1] << 8) | calib_88[0];
    dig_T2 = (int16_t)((calib_88[3] << 8) | calib_88[2]);
    dig_T3 = (int16_t)((calib_88[5] << 8) | calib_88[4]);
    dig_P1 = (calib_88[7] << 8) | calib_88[6];
    dig_P2 = (int16_t)((calib_88[9] << 8) | calib_88[8]);
    dig_P3 = (int16_t)((calib_88[11] << 8) | calib_88[10]);
    dig_P4 = (int16_t)((calib_88[13] << 8) | calib_88[12]);
    dig_P5 = (int16_t)((calib_88[15] << 8) | calib_88[14]);
    dig_P6 = (int16_t)((calib_88[17] << 8) | calib_88[16]);
    dig_P7 = (int16_t)((calib_88[19] << 8) | calib_88[18]);
    dig_P8 = (int16_t)((calib_88[21] << 8) | calib_88[20]);
    dig_P9 = (int16_t)((calib_88[23] << 8) | calib_88[22]);

    I2C_ReadRegs(0xA1, &dig_H1, 1);
    I2C_ReadRegs(0xE1, calib_E1, 7);
    dig_H2 = (int16_t)((calib_E1[1] << 8) | calib_E1[0]);
    dig_H3 = calib_E1[2];
    dig_H4 = (int16_t)((calib_E1[3] << 4) | (calib_E1[4] & 0x0F));
    dig_H5 = (int16_t)((calib_E1[5] << 4) | (calib_E1[4] >> 4));
    dig_H6 = (int8_t)calib_E1[6];
}

/* ---------- Инициализация BME280 ---------- */
void BME280_Init(void)
{
    uint8_t id;
    I2C_ReadRegs(REG_ID, &id, 1);
    sprintf(uart_buf, "ID: 0x%02X\r\n", id);
    HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 100);

    I2C_WriteReg(0xE0, 0xB6);
    osDelay(10);

    BME280_ReadCalibration();

    // Настройка: T x1, P x1, H x1, нормальный режим
    I2C_WriteReg(REG_CTRL_HUM, 0x01);   // humidity oversampling x1
    I2C_WriteReg(REG_CTRL_MEAS, 0x27);  // T x1, P x1, normal mode
    I2C_WriteReg(REG_CONFIG, 0x00);     // standby 0.5ms, filter off
}

/* ---------- Компенсация температуры (возвращает *100) ---------- */
static int32_t BME280_compensate_T(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

/* ---------- Компенсация давления (возвращает Па * 256) ---------- */
static uint32_t BME280_compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)p;
}

/* ---------- Компенсация влажности (возвращает %RH * 1024) ---------- */
static uint32_t BME280_compensate_H(int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) +
                  ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
                     (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (uint32_t)(v_x1_u32r >> 12);
}

/* ---------- Задача FreeRTOS ---------- */
void BME280_Thread(void *argument)
{
    BME280_Init();

    for (;;)
    {
        uint8_t raw[8];
        if (I2C_ReadRegs(REG_PRESS_MSB, raw, 8) != HAL_OK)
        {
            strcpy(uart_buf, "Read err\r\n");
            //HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 100);
            //osDelay(200);
            //continue;
        }

        int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
        int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
        int32_t adc_H = ((int32_t)raw[6] << 8)  | raw[7];

        int32_t  temp100  = BME280_compensate_T(adc_T);
        uint32_t press256 = BME280_compensate_P(adc_P);
        uint32_t hum1024  = BME280_compensate_H(adc_H);

        // Преобразование в удобные единицы
        int16_t temp_int = temp100 / 100;
        uint8_t temp_dec = abs(temp100) % 100;

        uint32_t press_hpa_x100 = (press256 * 100) / 25600;
        uint16_t press_int = press_hpa_x100 / 100 / 1.33322;     //  без 1.33322 будет в hPa а так в мм рт.ст
        uint8_t press_dec = press_hpa_x100 % 100;

        uint32_t hum_x100 = (hum1024 * 100) / 1024;
        uint16_t hum_int = hum_x100 / 100;
        uint8_t hum_dec = hum_x100 % 100;

        // Формируем строку для вывода
        sprintf(uart_buf,
                "T:%c%d.%02dC P:%d.%02dmm.rt H:%d.%d%% \r\n",
                temp100 < 0 ? '-' : ' ',
                abs(temp_int), temp_dec,
                press_int, press_dec,
                hum_int, hum_dec);
        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 100);

        osDelay(2300);  // для теста; потом можно ускорить
    }
}

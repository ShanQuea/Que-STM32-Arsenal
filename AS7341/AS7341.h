/**
  ******************************************************************************
  * @file           : AS7341.h
  * @author         : ShanQue
  * @brief          : AS7341 11通道光谱传感器驱动库
  * @date           : 2025/07/24
  ******************************************************************************
  */

#ifndef _AS7341_H
#define _AS7341_H

/* 头文件包含 */

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* 宏定义 */

#define AS7341_I2CADDR_DEFAULT 0x39
#define AS7341_CHIP_ID 0x09
#define AS7341_WHOAMI 0x92

// 主要控制寄存器
#define AS7341_ENABLE 0x80
#define AS7341_ATIME 0x81
#define AS7341_CONFIG 0x70
#define AS7341_LED 0x74
#define AS7341_STATUS 0x93
#define AS7341_CFG0 0xA9
#define AS7341_CFG1 0xAA
#define AS7341_CFG6 0xAF
#define AS7341_CFG9 0xB2
#define AS7341_CFG12 0xB5
#define AS7341_ASTEP_L 0xCA
#define AS7341_ASTEP_H 0xCB

// ADC通道数据寄存器
#define AS7341_CH0_DATA_L 0x95
#define AS7341_CH0_DATA_H 0x96
#define AS7341_CH1_DATA_L 0x97
#define AS7341_CH1_DATA_H 0x98
#define AS7341_CH2_DATA_L 0x99
#define AS7341_CH2_DATA_H 0x9A
#define AS7341_CH3_DATA_L 0x9B
#define AS7341_CH3_DATA_H 0x9C
#define AS7341_CH4_DATA_L 0x9D
#define AS7341_CH4_DATA_H 0x9E
#define AS7341_CH5_DATA_L 0x9F
#define AS7341_CH5_DATA_H 0xA0

#define AS7341_TIMEOUT_MS 100

/* 枚举类型定义 */

typedef enum {
    AS7341_GAIN_0_5X = 0,
    AS7341_GAIN_1X = 1,
    AS7341_GAIN_2X = 2,
    AS7341_GAIN_4X = 3,
    AS7341_GAIN_8X = 4,
    AS7341_GAIN_16X = 5,
    AS7341_GAIN_32X = 6,
    AS7341_GAIN_64X = 7,
    AS7341_GAIN_128X = 8,
    AS7341_GAIN_256X = 9,
    AS7341_GAIN_512X = 10,
} as7341_gain_t;

typedef enum {
    AS7341_SMUX_CMD_ROM_RESET,
    AS7341_SMUX_CMD_READ,
    AS7341_SMUX_CMD_WRITE,
} as7341_smux_cmd_t;

typedef enum {
    AS7341_ADC_CHANNEL_0,
    AS7341_ADC_CHANNEL_1,
    AS7341_ADC_CHANNEL_2,
    AS7341_ADC_CHANNEL_3,
    AS7341_ADC_CHANNEL_4,
    AS7341_ADC_CHANNEL_5,
} as7341_adc_channel_t;

typedef enum {
    AS7341_CHANNEL_415nm_F1,
    AS7341_CHANNEL_445nm_F2,
    AS7341_CHANNEL_480nm_F3,
    AS7341_CHANNEL_515nm_F4,
    AS7341_CHANNEL_CLEAR_0,
    AS7341_CHANNEL_NIR_0,
    AS7341_CHANNEL_555nm_F5,
    AS7341_CHANNEL_590nm_F6,
    AS7341_CHANNEL_630nm_F7,
    AS7341_CHANNEL_680nm_F8,
    AS7341_CHANNEL_CLEAR,
    AS7341_CHANNEL_NIR,
} as7341_color_channel_t;

typedef enum {
    AS7341_WAITING_START,
    AS7341_WAITING_LOW,
    AS7341_WAITING_HIGH,
    AS7341_WAITING_DONE,
} as7341_waiting_t;

/* 结构体定义 */

typedef struct {
    I2C_HandleTypeDef *i2c_handle;
    uint8_t i2c_address;
    uint16_t channel_readings[12];
    as7341_waiting_t reading_state;
    bool initialized;
} as7341_handle_t;

/* 函数声明 */

// 初始化和配置
bool AS7341_Init(as7341_handle_t *handle, I2C_HandleTypeDef *i2c_handle, uint8_t i2c_address, int32_t sensor_id);
bool AS7341_SetASTEP(as7341_handle_t *handle, uint16_t astep_value);
bool AS7341_SetATIME(as7341_handle_t *handle, uint8_t atime_value);
bool AS7341_SetGain(as7341_handle_t *handle, as7341_gain_t gain_value);

// 数据获取
uint16_t AS7341_GetASTEP(as7341_handle_t *handle);
uint8_t AS7341_GetATIME(as7341_handle_t *handle);
as7341_gain_t AS7341_GetGain(as7341_handle_t *handle);
uint32_t AS7341_GetTINT(as7341_handle_t *handle);

// 数据读取
bool AS7341_ReadAllChannels(as7341_handle_t *handle);
bool AS7341_ReadAllChannels_Blocking(as7341_handle_t *handle);
uint16_t AS7341_ReadChannel(as7341_handle_t *handle, as7341_adc_channel_t channel);
uint16_t AS7341_GetChannel(as7341_handle_t *handle, as7341_color_channel_t channel);

// 控制功能
bool AS7341_PowerEnable(as7341_handle_t *handle, bool enable_power);
bool AS7341_EnableSpectralMeasurement(as7341_handle_t *handle, bool enable_measurement);
bool AS7341_EnableLED(as7341_handle_t *handle, bool enable_led);
bool AS7341_SetLEDCurrent(as7341_handle_t *handle, uint16_t led_current_ma);
bool AS7341_GetIsDataReady(as7341_handle_t *handle);

/**
 * 使用说明：
 * 1. 使用AS7341_Init()初始化传感器
 * 2. 使用AS7341_SetGain()和AS7341_SetATIME()配置参数
 * 3. 使用AS7341_ReadAllChannels_Blocking()读取所有通道数据
 * 4. 使用AS7341_GetChannel()获取特定波长通道数据
 */

#endif /* _AS7341_H */
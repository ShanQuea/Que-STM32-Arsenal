/**
  ******************************************************************************
  * @file           : TCA9548A.h
  * @author         : ShanQue
  * @brief          : TCA9548A 8通道I2C多路复用器驱动库
  * @date           : 2025/07/24
  ******************************************************************************
  */

#ifndef _TCA9548A_H
#define _TCA9548A_H

/* 头文件包含 */

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* 宏定义 */

#define TCA9548A_I2C_ADDR_DEFAULT   0x70
#define TCA9548A_I2C_ADDR_MIN       0x70
#define TCA9548A_I2C_ADDR_MAX       0x77

#define TCA9548A_CHANNEL_0          0x01
#define TCA9548A_CHANNEL_1          0x02
#define TCA9548A_CHANNEL_2          0x04
#define TCA9548A_CHANNEL_3          0x08
#define TCA9548A_CHANNEL_4          0x10
#define TCA9548A_CHANNEL_5          0x20
#define TCA9548A_CHANNEL_6          0x40
#define TCA9548A_CHANNEL_7          0x80
#define TCA9548A_CHANNEL_ALL        0xFF
#define TCA9548A_CHANNEL_NONE       0x00

#define TCA9548A_TIMEOUT_MS         100

/* 枚举类型定义 */

typedef enum {
    TCA9548A_OK = 0,
    TCA9548A_ERROR_INVALID_PARAM,
    TCA9548A_ERROR_I2C_TIMEOUT,
    TCA9548A_ERROR_I2C_ERROR,
    TCA9548A_ERROR_DEVICE_NOT_FOUND,
    TCA9548A_ERROR_CHANNEL_INVALID,
    TCA9548A_ERROR_NOT_INITIALIZED
} tca9548a_error_t;

typedef enum {
    TCA9548A_CH0 = 0,
    TCA9548A_CH1 = 1,
    TCA9548A_CH2 = 2,
    TCA9548A_CH3 = 3,
    TCA9548A_CH4 = 4,
    TCA9548A_CH5 = 5,
    TCA9548A_CH6 = 6,
    TCA9548A_CH7 = 7,
    TCA9548A_CH_MAX = 8
} tca9548a_channel_t;

/* 结构体定义 */

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t device_address;
    uint8_t current_channels;
    bool initialized;
    uint32_t timeout_ms;
} tca9548a_handle_t;

/* 函数声明 */

// 初始化和控制
tca9548a_error_t TCA9548A_Init(tca9548a_handle_t *handle, I2C_HandleTypeDef *hi2c, uint8_t device_address);
tca9548a_error_t TCA9548A_IsDeviceReady(tca9548a_handle_t *handle);
tca9548a_error_t TCA9548A_Reset(tca9548a_handle_t *handle);

// 通道控制
tca9548a_error_t TCA9548A_SelectChannel(tca9548a_handle_t *handle, tca9548a_channel_t channel);
tca9548a_error_t TCA9548A_SelectChannels(tca9548a_handle_t *handle, uint8_t channel_mask);
tca9548a_error_t TCA9548A_DisableChannel(tca9548a_handle_t *handle, tca9548a_channel_t channel);
tca9548a_error_t TCA9548A_DisableAllChannels(tca9548a_handle_t *handle);

// 状态查询
tca9548a_error_t TCA9548A_GetChannelStatus(tca9548a_handle_t *handle, uint8_t *channel_status);
tca9548a_error_t TCA9548A_IsChannelActive(tca9548a_handle_t *handle, tca9548a_channel_t channel, bool *is_active);

// 设备扫描
tca9548a_error_t TCA9548A_ScanChannel(tca9548a_handle_t *handle, tca9548a_channel_t channel, 
                                      uint8_t *device_addresses, uint8_t max_devices, uint8_t *found_count);

// 工具函数
uint8_t TCA9548A_ChannelToMask(tca9548a_channel_t channel);
tca9548a_channel_t TCA9548A_MaskToChannel(uint8_t mask);
bool TCA9548A_IsValidChannel(tca9548a_channel_t channel);
const char* TCA9548A_GetErrorString(tca9548a_error_t error);

/**
 * 使用说明：
 * 1. 使用TCA9548A_Init()初始化多路复用器
 * 2. 使用TCA9548A_SelectChannel()选择要通信的I2C通道
 * 3. 在选定通道上进行I2C设备通信
 * 4. 使用TCA9548A_DisableAllChannels()关闭所有通道
 */

#endif /* _TCA9548A_H */
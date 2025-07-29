/**
  ******************************************************************************
  * @file           : TCA9548A.c
  * @author         : ShanQue
  * @brief          : TCA9548A 8通道I2C多路复用器驱动库
  * @date           : 2025/07/24
  ******************************************************************************
  */


#include "TCA9548A.h"
#include <string.h>

// 静态函数声明
static tca9548a_error_t TCA9548A_WriteRegister(tca9548a_handle_t *handle, uint8_t data);
static tca9548a_error_t TCA9548A_ReadRegister(tca9548a_handle_t *handle, uint8_t *data);
static HAL_StatusTypeDef TCA9548A_HALStatusToError(HAL_StatusTypeDef hal_status);

// 核心函数实现

/**
 * @brief 初始化TCA9548A设备
 */
tca9548a_error_t TCA9548A_Init(tca9548a_handle_t *handle, 
                               I2C_HandleTypeDef *hi2c, 
                               uint8_t device_address)
{
    // 参数验证
    if (handle == NULL || hi2c == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    if (!TCA9548A_IsValidAddress(device_address)) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    // 初始化句柄
    memset(handle, 0, sizeof(tca9548a_handle_t));
    handle->hi2c = hi2c;
    handle->device_address = device_address << 1; // HAL库需要左移1位
    handle->current_channels = TCA9548A_CHANNEL_NONE;
    handle->timeout_ms = TCA9548A_TIMEOUT_MS;
    
    // 检测设备是否存在
    tca9548a_error_t error = TCA9548A_IsDeviceReady(handle);
    if (error != TCA9548A_OK) {
        return error;
    }
    
    // 复位设备（关闭所有通道）
    error = TCA9548A_Reset(handle);
    if (error != TCA9548A_OK) {
        return error;
    }
    
    handle->initialized = true;
    return TCA9548A_OK;
}

/**
 * @brief 检测TCA9548A设备是否存在
 */
tca9548a_error_t TCA9548A_IsDeviceReady(tca9548a_handle_t *handle)
{
    if (handle == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(handle->hi2c, 
                                                         handle->device_address, 
                                                         3, 
                                                         handle->timeout_ms);
    
    if (hal_status == HAL_OK) {
        return TCA9548A_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return TCA9548A_ERROR_I2C_TIMEOUT;
    } else {
        return TCA9548A_ERROR_DEVICE_NOT_FOUND;
    }
}

/**
 * @brief 选择单个通道
 */
tca9548a_error_t TCA9548A_SelectChannel(tca9548a_handle_t *handle, 
                                        tca9548a_channel_t channel)
{
    if (handle == NULL || !handle->initialized) {
        return TCA9548A_ERROR_NOT_INITIALIZED;
    }
    
    if (!TCA9548A_IsValidChannel(channel)) {
        return TCA9548A_ERROR_CHANNEL_INVALID;
    }
    
    uint8_t channel_mask = TCA9548A_ChannelToMask(channel);
    return TCA9548A_SelectChannels(handle, channel_mask);
}

/**
 * @brief 选择多个通道
 */
tca9548a_error_t TCA9548A_SelectChannels(tca9548a_handle_t *handle, 
                                         uint8_t channel_mask)
{
    if (handle == NULL || !handle->initialized) {
        return TCA9548A_ERROR_NOT_INITIALIZED;
    }
    
    tca9548a_error_t error = TCA9548A_WriteRegister(handle, channel_mask);
    if (error == TCA9548A_OK) {
        handle->current_channels = channel_mask;
    }
    
    return error;
}

/**
 * @brief 关闭单个通道
 */
tca9548a_error_t TCA9548A_DisableChannel(tca9548a_handle_t *handle, 
                                         tca9548a_channel_t channel)
{
    if (handle == NULL || !handle->initialized) {
        return TCA9548A_ERROR_NOT_INITIALIZED;
    }
    
    if (!TCA9548A_IsValidChannel(channel)) {
        return TCA9548A_ERROR_CHANNEL_INVALID;
    }
    
    uint8_t channel_mask = TCA9548A_ChannelToMask(channel);
    uint8_t new_channels = handle->current_channels & (~channel_mask);
    
    return TCA9548A_SelectChannels(handle, new_channels);
}

/**
 * @brief 关闭所有通道
 */
tca9548a_error_t TCA9548A_DisableAllChannels(tca9548a_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return TCA9548A_ERROR_NOT_INITIALIZED;
    }
    
    return TCA9548A_SelectChannels(handle, TCA9548A_CHANNEL_NONE);
}

/**
 * @brief 获取当前激活的通道状态
 */
tca9548a_error_t TCA9548A_GetChannelStatus(tca9548a_handle_t *handle, 
                                           uint8_t *channel_status)
{
    if (handle == NULL || !handle->initialized || channel_status == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    return TCA9548A_ReadRegister(handle, channel_status);
}

/**
 * @brief 检查指定通道是否激活
 */
tca9548a_error_t TCA9548A_IsChannelActive(tca9548a_handle_t *handle, 
                                          tca9548a_channel_t channel, 
                                          bool *is_active)
{
    if (handle == NULL || !handle->initialized || is_active == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    if (!TCA9548A_IsValidChannel(channel)) {
        return TCA9548A_ERROR_CHANNEL_INVALID;
    }
    
    uint8_t channel_status;
    tca9548a_error_t error = TCA9548A_GetChannelStatus(handle, &channel_status);
    if (error != TCA9548A_OK) {
        return error;
    }
    
    uint8_t channel_mask = TCA9548A_ChannelToMask(channel);
    *is_active = (channel_status & channel_mask) != 0;
    
    return TCA9548A_OK;
}

/**
 * @brief 复位TCA9548A设备
 */
tca9548a_error_t TCA9548A_Reset(tca9548a_handle_t *handle)
{
    if (handle == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    tca9548a_error_t error = TCA9548A_WriteRegister(handle, TCA9548A_CHANNEL_NONE);
    if (error == TCA9548A_OK) {
        handle->current_channels = TCA9548A_CHANNEL_NONE;
    }
    
    return error;
}

/**
 * @brief 扫描指定通道上的I2C设备
 */
tca9548a_error_t TCA9548A_ScanChannel(tca9548a_handle_t *handle, 
                                      tca9548a_channel_t channel,
                                      uint8_t *device_addresses, 
                                      uint8_t max_devices, 
                                      uint8_t *found_count)
{
    if (handle == NULL || !handle->initialized || 
        device_addresses == NULL || found_count == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    if (!TCA9548A_IsValidChannel(channel)) {
        return TCA9548A_ERROR_CHANNEL_INVALID;
    }
    
    // 保存当前通道状态
    uint8_t original_channels = handle->current_channels;
    
    // 选择要扫描的通道
    tca9548a_error_t error = TCA9548A_SelectChannel(handle, channel);
    if (error != TCA9548A_OK) {
        return error;
    }
    
    // 扫描I2C设备
    *found_count = 0;
    for (uint8_t addr = 0x08; addr < 0x78 && *found_count < max_devices; addr++) {
        if (addr == (handle->device_address >> 1)) {
            continue; // 跳过TCA9548A自身的地址
        }
        
        HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(handle->hi2c, 
                                                             addr << 1, 
                                                             1, 
                                                             10);
        if (hal_status == HAL_OK) {
            device_addresses[*found_count] = addr;
            (*found_count)++;
        }
    }
    
    // 恢复原始通道状态
    TCA9548A_SelectChannels(handle, original_channels);
    
    return TCA9548A_OK;
}

/**
 * @brief 设置操作超时时间
 */
tca9548a_error_t TCA9548A_SetTimeout(tca9548a_handle_t *handle, 
                                     uint32_t timeout_ms)
{
    if (handle == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    handle->timeout_ms = timeout_ms;
    return TCA9548A_OK;
}

// 辅助函数实现

/**
 * @brief 将通道编号转换为通道掩码
 */
uint8_t TCA9548A_ChannelToMask(tca9548a_channel_t channel)
{
    if (channel >= TCA9548A_CH_MAX) {
        return 0;
    }
    
    return (1 << channel);
}

/**
 * @brief 将通道掩码转换为通道编号
 */
tca9548a_channel_t TCA9548A_MaskToChannel(uint8_t mask)
{
    for (tca9548a_channel_t ch = TCA9548A_CH0; ch < TCA9548A_CH_MAX; ch++) {
        if (mask == (1 << ch)) {
            return ch;
        }
    }
    return TCA9548A_CH_MAX;
}

/**
 * @brief 检查通道编号是否有效
 */
bool TCA9548A_IsValidChannel(tca9548a_channel_t channel)
{
    return (channel >= TCA9548A_CH0 && channel < TCA9548A_CH_MAX);
}

/**
 * @brief 检查I2C地址是否有效
 */
bool TCA9548A_IsValidAddress(uint8_t address)
{
    return (address >= TCA9548A_I2C_ADDR_MIN && address <= TCA9548A_I2C_ADDR_MAX);
}

/**
 * @brief 获取错误描述字符串
 */
const char* TCA9548A_GetErrorString(tca9548a_error_t error)
{
    switch (error) {
        case TCA9548A_OK:
            return "Success";
        case TCA9548A_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case TCA9548A_ERROR_I2C_TIMEOUT:
            return "I2C timeout";
        case TCA9548A_ERROR_I2C_ERROR:
            return "I2C communication error";
        case TCA9548A_ERROR_DEVICE_NOT_FOUND:
            return "Device not found";
        case TCA9548A_ERROR_CHANNEL_INVALID:
            return "Invalid channel";
        case TCA9548A_ERROR_NOT_INITIALIZED:
            return "Device not initialized";
        default:
            return "Unknown error";
    }
}

// 静态函数实现

/**
 * @brief 写入寄存器数据
 */
static tca9548a_error_t TCA9548A_WriteRegister(tca9548a_handle_t *handle, uint8_t data)
{
    if (handle == NULL || handle->hi2c == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(handle->hi2c, 
                                                           handle->device_address, 
                                                           &data, 
                                                           1, 
                                                           handle->timeout_ms);
    
    if (hal_status == HAL_OK) {
        return TCA9548A_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return TCA9548A_ERROR_I2C_TIMEOUT;
    } else {
        return TCA9548A_ERROR_I2C_ERROR;
    }
}

/**
 * @brief 读取寄存器数据
 */
static tca9548a_error_t TCA9548A_ReadRegister(tca9548a_handle_t *handle, uint8_t *data)
{
    if (handle == NULL || handle->hi2c == NULL || data == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(handle->hi2c, 
                                                          handle->device_address, 
                                                          data, 
                                                          1, 
                                                          handle->timeout_ms);
    
    if (hal_status == HAL_OK) {
        return TCA9548A_OK;
    } else if (hal_status == HAL_TIMEOUT) {
        return TCA9548A_ERROR_I2C_TIMEOUT;
    } else {
        return TCA9548A_ERROR_I2C_ERROR;
    }
}

/**
 * @brief 将HAL状态转换为TCA9548A错误代码
 */
static HAL_StatusTypeDef TCA9548A_HALStatusToError(HAL_StatusTypeDef hal_status)
{
    switch (hal_status) {
        case HAL_OK:
            return TCA9548A_OK;
        case HAL_TIMEOUT:
            return TCA9548A_ERROR_I2C_TIMEOUT;
        case HAL_ERROR:
        case HAL_BUSY:
        default:
            return TCA9548A_ERROR_I2C_ERROR;
    }
}

/**
 * @brief 扫描I2C总线上的所有TCA9548A设备
 */
tca9548a_error_t TCA9548A_ScanBus(I2C_HandleTypeDef *hi2c, 
                                  uint8_t *found_addresses, 
                                  uint8_t max_devices, 
                                  uint8_t *found_count)
{
    if (hi2c == NULL || found_addresses == NULL || found_count == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    *found_count = 0;
    
    // 扫描TCA9548A地址范围 (0x70-0x77)
    for (uint8_t addr = TCA9548A_I2C_ADDR_MIN; addr <= TCA9548A_I2C_ADDR_MAX && *found_count < max_devices; addr++) {
        HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(hi2c, 
                                                             addr << 1,  // HAL库需要左移1位
                                                             3, 
                                                             100);
        if (hal_status == HAL_OK) {
            found_addresses[*found_count] = addr;
            (*found_count)++;
        }
    }
    
    return TCA9548A_OK;
}

/**
 * @brief 扫描I2C总线上的所有设备
 */
tca9548a_error_t TCA9548A_ScanAllDevices(I2C_HandleTypeDef *hi2c, 
                                         uint8_t *found_addresses, 
                                         uint8_t max_devices, 
                                         uint8_t *found_count)
{
    if (hi2c == NULL || found_addresses == NULL || found_count == NULL) {
        return TCA9548A_ERROR_INVALID_PARAM;
    }
    
    *found_count = 0;
    
    // 扫描I2C地址范围 (0x08-0x77)
    for (uint8_t addr = 0x08; addr <= 0x77 && *found_count < max_devices; addr++) {
        HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(hi2c, 
                                                             addr << 1,
                                                             3, 
                                                             50);
        if (hal_status == HAL_OK) {
            found_addresses[*found_count] = addr;
            (*found_count)++;
        }
    }
    
    return TCA9548A_OK;
}
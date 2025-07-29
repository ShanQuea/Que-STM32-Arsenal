/**
  ******************************************************************************
  * @file           : AS7341.c
  * @author         : ShanQue
  * @brief          : AS7341 11通道光谱传感器驱动库
  * @date           : 2025/07/24
  ******************************************************************************
  */

#include "AS7341.h"
#include "app.h"
#include <string.h>


// 静态函数声明
static bool AS7341_WriteRegister(as7341_handle_t *handle, uint8_t mem_addr, uint8_t *val, uint16_t size);
static bool AS7341_WriteRegisterByte(as7341_handle_t *handle, uint8_t mem_addr, uint8_t val);
static bool AS7341_ReadRegister(as7341_handle_t *handle, uint16_t mem_addr, uint8_t *dest, uint16_t size);
static uint8_t AS7341_ReadRegisterByte(as7341_handle_t *handle, uint16_t mem_addr);
static bool AS7341_ModifyRegisterBit(as7341_handle_t *handle, uint16_t reg, bool value, uint8_t pos);
static uint8_t AS7341_CheckRegisterBit(as7341_handle_t *handle, uint16_t reg, uint8_t pos);
static uint8_t AS7341_ModifyBitInByte(uint8_t var, uint8_t value, uint8_t pos);
static bool AS7341_ModifyRegisterMultipleBit(as7341_handle_t *handle, uint16_t reg, uint8_t value, uint8_t pos, uint8_t bits);
static bool AS7341_EnableSMUX(as7341_handle_t *handle);
static bool AS7341_SetSMUXCommand(as7341_handle_t *handle, as7341_smux_cmd_t command);
static void AS7341_SetSMUXLowChannels(as7341_handle_t *handle, bool f1_f4);
static bool AS7341_InitDevice(as7341_handle_t *handle, int32_t sensor_id);

// 核心函数实现

/**
 * @brief 初始化AS7341设备
 */
bool AS7341_Init(as7341_handle_t *handle, I2C_HandleTypeDef *i2c_handle, 
                 uint8_t i2c_address, int32_t sensor_id)
{
    // 参数验证
    if (handle == NULL || i2c_handle == NULL) {
        return false;
    }
    
    // 初始化句柄
    memset(handle, 0, sizeof(as7341_handle_t));
    handle->i2c_handle = i2c_handle;
    handle->i2c_address = i2c_address << 1; 
    handle->reading_state = AS7341_WAITING_START;
    
    // 初始化设备
    bool init_result = AS7341_InitDevice(handle, sensor_id);
    if (init_result) {
        handle->initialized = true;
    }
    
    return init_result;
}

/**
 * @brief 设置积分步长值
 */
bool AS7341_SetASTEP(as7341_handle_t *handle, uint16_t astep_value)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_WriteRegister(handle, AS7341_ASTEP_L, (uint8_t*)&astep_value, 2);
}

/**
 * @brief 设置ADC积分时间
 */
bool AS7341_SetATIME(as7341_handle_t *handle, uint8_t atime_value)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_WriteRegisterByte(handle, AS7341_ATIME, atime_value);
}

/**
 * @brief 设置ADC增益
 */
bool AS7341_SetGain(as7341_handle_t *handle, as7341_gain_t gain_value)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_WriteRegisterByte(handle, AS7341_CFG1, gain_value);
}

/**
 * @brief 获取积分步长值
 */
uint16_t AS7341_GetASTEP(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    
    uint8_t data[2];
    AS7341_ReadRegister(handle, AS7341_ASTEP_L, data, 2);
    // 修正位运算错误：应该使用OR而不是AND
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief 获取ADC积分时间
 */
uint8_t AS7341_GetATIME(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    
    return AS7341_ReadRegisterByte(handle, AS7341_ATIME);
}

/**
 * @brief 获取ADC增益
 */
as7341_gain_t AS7341_GetGain(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return AS7341_GAIN_1X;
    }
    
    return (as7341_gain_t)AS7341_ReadRegisterByte(handle, AS7341_CFG1);
}

/**
 * @brief 获取总积分时间
 */
uint32_t AS7341_GetTINT(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    
    uint16_t astep = AS7341_GetASTEP(handle);
    uint8_t atime = AS7341_GetATIME(handle);
    
    return (uint32_t)((atime + 1) * (astep + 1) * 2.78f / 1000.0f);
}

/**
 * @brief 将原始ADC值转换为基本计数值
 */
float AS7341_ToBasicCounts(as7341_handle_t *handle, uint16_t raw)
{
    if (handle == NULL || !handle->initialized) {
        return 0.0f;
    }
    
    float gain_val = 0.0f;
    as7341_gain_t gain = AS7341_GetGain(handle);
    
    switch (gain) {
        case AS7341_GAIN_0_5X:  gain_val = 0.5f; break;
        case AS7341_GAIN_1X:    gain_val = 1.0f; break;
        case AS7341_GAIN_2X:    gain_val = 2.0f; break;
        case AS7341_GAIN_4X:    gain_val = 4.0f; break;
        case AS7341_GAIN_8X:    gain_val = 8.0f; break;
        case AS7341_GAIN_16X:   gain_val = 16.0f; break;
        case AS7341_GAIN_32X:   gain_val = 32.0f; break;
        case AS7341_GAIN_64X:   gain_val = 64.0f; break;
        case AS7341_GAIN_128X:  gain_val = 128.0f; break;
        case AS7341_GAIN_256X:  gain_val = 256.0f; break;
        case AS7341_GAIN_512X:  gain_val = 512.0f; break;
        default: gain_val = 1.0f; break;
    }
    
    return raw / (gain_val * (AS7341_GetATIME(handle) + 1) * (AS7341_GetASTEP(handle) + 1) * 2.78f / 1000.0f);
}

/**
 * @brief 读取所有通道数据
 */
bool AS7341_ReadAllChannels(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_ReadAllChannelsToBuffer(handle, handle->channel_readings);
}

/**
 * @brief 读取所有通道数据到指定缓冲区
 */
bool AS7341_ReadAllChannelsToBuffer(as7341_handle_t *handle, uint16_t *readings_buffer)
{
    if (handle == NULL || !handle->initialized || readings_buffer == NULL) {
        return false;
    }
    
    AS7341_SetSMUXLowChannels(handle, true);        // 配置SMUX读取低通道
    AS7341_EnableSpectralMeasurement(handle, true); 
    AS7341_DelayForData(handle, 0);                
    
    bool low_success = AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)readings_buffer, 12);
    
    AS7341_SetSMUXLowChannels(handle, false);       // 配置SMUX读取高通道
    AS7341_EnableSpectralMeasurement(handle, true); 
    AS7341_DelayForData(handle, 0);                
    
    return low_success && AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)&readings_buffer[6], 12);
}

/**
 * @brief 等待数据就绪
 */
void AS7341_DelayForData(as7341_handle_t *handle, uint32_t wait_time)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }
    
    if (wait_time == 0) {
        uint32_t timeout_ms = 200;
        uint32_t elapsed_ms = 0;

        while (!AS7341_GetIsDataReady(handle) && elapsed_ms < timeout_ms) {
            HAL_Delay(1);
            elapsed_ms++;
            // 每50ms刷新一次看门狗，防止超时 (这里可以不需要 存在的原因是因为我开发时候使用看门狗检测系统运行状态)
            if (elapsed_ms % 50 == 0) {
                extern void System_WatchdogRefresh(void);
                System_WatchdogRefresh();
            }
        }
        return;
    }
    
    if (wait_time > 0) { // 等待指定时间
        uint32_t elapsed_millis = 0;
        while ((!AS7341_GetIsDataReady(handle)) && (elapsed_millis < wait_time)) {
            HAL_Delay(1);
            elapsed_millis++;
        }
        return;
    }
}

/**
 * @brief 读取指定ADC通道数据
 */
uint16_t AS7341_ReadChannel(as7341_handle_t *handle, as7341_adc_channel_t channel)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    
    // 每个通道有两个字节，所以偏移量为通道号*2
    uint8_t data[2];
    AS7341_ReadRegister(handle, (uint16_t)(AS7341_CH0_DATA_L + 2 * channel), data, 2);
    return (((uint16_t)data[1]) << 8) | data[0];
}

/**
 * @brief 获取指定颜色通道数据
 */
uint16_t AS7341_GetChannel(as7341_handle_t *handle, as7341_color_channel_t channel)
{
    if (handle == NULL || !handle->initialized || channel >= 12) {
        return 0;
    }
    
    return handle->channel_readings[channel];
}

/**
 * @brief 开始异步读取
 */
bool AS7341_StartReading(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    handle->reading_state = AS7341_WAITING_START; // 开始测量
    AS7341_CheckReadingProgress(handle);          // 调用检查函数开始
    return true;
}

/**
 * @brief 检查异步读取进度
 */
bool AS7341_CheckReadingProgress(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    if (handle->reading_state == AS7341_WAITING_START) {
        AS7341_SetSMUXLowChannels(handle, true);        // 配置SMUX读取低通道
        AS7341_EnableSpectralMeasurement(handle, true); 
        handle->reading_state = AS7341_WAITING_LOW;
        return false;
    }
    
    if (!AS7341_GetIsDataReady(handle) || handle->reading_state == AS7341_WAITING_DONE) {
        return false;
    }
    
    if (handle->reading_state == AS7341_WAITING_LOW) { // 检查getIsDataReady已完成
        AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)handle->channel_readings, 12);
        
        AS7341_SetSMUXLowChannels(handle, false);       // 配置SMUX读取高通道
        AS7341_EnableSpectralMeasurement(handle, true); 
        handle->reading_state = AS7341_WAITING_HIGH;
        return false;
    }
    
    if (handle->reading_state == AS7341_WAITING_HIGH) { // 检查getIsDataReady已完成
        handle->reading_state = AS7341_WAITING_DONE;
        AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)&handle->channel_readings[6], 12);
        return true;
    }
    
    return false;
}

/**
 * @brief 获取所有通道数据
 */
bool AS7341_GetAllChannels(as7341_handle_t *handle, uint32_t *readings_buffer)
{
    if (handle == NULL || !handle->initialized || readings_buffer == NULL) {
        return false;
    }
    
    for (int i = 0; i < 12; i++) {
        readings_buffer[i] = handle->channel_readings[i];
    }
    return true;
}

/**
 * @brief 阻塞式读取所有通道数据（简化版）
 * 基于Adafruit原版逻辑，去除异步处理，使用HAL阻塞函数
 * 这个函数会完整读取一次所有12个通道数据
 */
bool AS7341_ReadAllChannels_Blocking(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    AS7341_PowerEnable(handle, true);  // 启用PON位
    HAL_Delay(5); // 等待电源稳定

    AS7341_SetSMUXLowChannels(handle, true);        // 配置SMUX读取低通道
    AS7341_EnableSpectralMeasurement(handle, true); 
    

    AS7341_DelayForData(handle, 0);

    if (!AS7341_GetIsDataReady(handle)) {
        app_printf("   低通道数据未就绪\r\n");
        return false;
    }
    
    // 读取低通道数据（6个通道，12字节）
    if (!AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)handle->channel_readings, 12)) {
        app_printf("   低通道数据读取失败\r\n");
        return false;
    }
    
    // 第二步：读取高通道组 (F5-F8, Clear, NIR)
    AS7341_SetSMUXLowChannels(handle, false);       // 配置SMUX读取高通道
    AS7341_EnableSpectralMeasurement(handle, true); 

    AS7341_DelayForData(handle, 0);
    
    // 检查数据是否就绪
    if (!AS7341_GetIsDataReady(handle)) {
        app_printf("   高通道数据未就绪\r\n");
        return false;
    }
    
    // 读取高通道数据（6个通道，12字节）
    if (!AS7341_ReadRegister(handle, AS7341_CH0_DATA_L, (uint8_t*)&handle->channel_readings[6], 12)) {
        app_printf("   高通道数据读取失败\r\n");
        return false;
    }
    
    return true;
}

/**
 * @brief 检测闪烁频率
 */

/**
 * @brief 配置F1-F4、Clear和NIR通道
 */
void AS7341_Setup_F1F4_Clear_NIR(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }
    
    // F1,F2,F3,F4,NIR,Clear的SMUX配置
    AS7341_WriteRegisterByte(handle, 0x00, 0x30); // F3左连接到ADC2
    AS7341_WriteRegisterByte(handle, 0x01, 0x01); // F1左连接到ADC0
    AS7341_WriteRegisterByte(handle, 0x02, 0x00); // 保留或禁用
    AS7341_WriteRegisterByte(handle, 0x03, 0x00); // F8左禁用
    AS7341_WriteRegisterByte(handle, 0x04, 0x00); // F6左禁用
    AS7341_WriteRegisterByte(handle, 0x05, 0x42); // F4左连接到ADC3/F2左连接到ADC1
    AS7341_WriteRegisterByte(handle, 0x06, 0x00); // F5左禁用
    AS7341_WriteRegisterByte(handle, 0x07, 0x00); // F7左禁用
    AS7341_WriteRegisterByte(handle, 0x08, 0x50); // CLEAR连接到ADC4
    AS7341_WriteRegisterByte(handle, 0x09, 0x00); // F5右禁用
    AS7341_WriteRegisterByte(handle, 0x0A, 0x00); // F7右禁用
    AS7341_WriteRegisterByte(handle, 0x0B, 0x00); // 保留或禁用
    AS7341_WriteRegisterByte(handle, 0x0C, 0x20); // F2右连接到ADC1
    AS7341_WriteRegisterByte(handle, 0x0D, 0x04); // F4右连接到ADC3
    AS7341_WriteRegisterByte(handle, 0x0E, 0x00); // F6/F8右禁用
    AS7341_WriteRegisterByte(handle, 0x0F, 0x30); // F3右连接到ADC2
    AS7341_WriteRegisterByte(handle, 0x10, 0x01); // F1右连接到ADC0
    AS7341_WriteRegisterByte(handle, 0x11, 0x50); // CLEAR右连接到ADC4
    AS7341_WriteRegisterByte(handle, 0x12, 0x00); // 保留或禁用
    AS7341_WriteRegisterByte(handle, 0x13, 0x06); // NIR连接到ADC5
}

/**
 * @brief 配置F5-F8、Clear和NIR通道
 */
void AS7341_Setup_F5F8_Clear_NIR(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }
    
    // F5,F6,F7,F8,NIR,Clear的SMUX配置
    AS7341_WriteRegisterByte(handle, 0x00, 0x00); // F3左禁用
    AS7341_WriteRegisterByte(handle, 0x01, 0x00); // F1左禁用
    AS7341_WriteRegisterByte(handle, 0x02, 0x00); // 保留/禁用
    AS7341_WriteRegisterByte(handle, 0x03, 0x40); // F8左连接到ADC3
    AS7341_WriteRegisterByte(handle, 0x04, 0x02); // F6左连接到ADC1
    AS7341_WriteRegisterByte(handle, 0x05, 0x00); // F4/F2禁用
    AS7341_WriteRegisterByte(handle, 0x06, 0x10); // F5左连接到ADC0
    AS7341_WriteRegisterByte(handle, 0x07, 0x03); // F7左连接到ADC2
    AS7341_WriteRegisterByte(handle, 0x08, 0x50); // CLEAR连接到ADC4
    AS7341_WriteRegisterByte(handle, 0x09, 0x10); // F5右连接到ADC0
    AS7341_WriteRegisterByte(handle, 0x0A, 0x03); // F7右连接到ADC2
    AS7341_WriteRegisterByte(handle, 0x0B, 0x00); // 保留或禁用
    AS7341_WriteRegisterByte(handle, 0x0C, 0x00); // F2右禁用
    AS7341_WriteRegisterByte(handle, 0x0D, 0x00); // F4右禁用
    AS7341_WriteRegisterByte(handle, 0x0E, 0x24); // F8右连接到ADC2/F6右连接到ADC1
    AS7341_WriteRegisterByte(handle, 0x0F, 0x00); // F3右禁用
    AS7341_WriteRegisterByte(handle, 0x10, 0x00); // F1右禁用
    AS7341_WriteRegisterByte(handle, 0x11, 0x50); // CLEAR右连接到ADC4
    AS7341_WriteRegisterByte(handle, 0x12, 0x00); // 保留或禁用
    AS7341_WriteRegisterByte(handle, 0x13, 0x06); // NIR连接到ADC5
}

/**
 * @brief 启用/禁用AS7341电源 (PON位)
 */
bool AS7341_PowerEnable(as7341_handle_t *handle, bool enable_power)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    // 读取当前ENABLE寄存器值
    uint8_t enable_reg = AS7341_ReadRegisterByte(handle, AS7341_ENABLE);
    
    // 修改PON位 (bit 0)
    enable_reg = AS7341_ModifyBitInByte(enable_reg, (uint8_t)enable_power, 0);
    
    // 写入修改后的值
    return AS7341_WriteRegisterByte(handle, AS7341_ENABLE, enable_reg);
}

/**
 * @brief 启用/禁用光谱测量
 */
bool AS7341_EnableSpectralMeasurement(as7341_handle_t *handle, bool enable_measurement)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    uint8_t enable_reg = AS7341_ReadRegisterByte(handle, AS7341_ENABLE);
    enable_reg = AS7341_ModifyBitInByte(enable_reg, (uint8_t)enable_measurement, 1);
    
    return AS7341_WriteRegisterByte(handle, AS7341_ENABLE, enable_reg);
}



/**
 * @brief 启用/禁用LED
 */
bool AS7341_EnableLED(as7341_handle_t *handle, bool enable_led)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    AS7341_SetBank(handle, true); // 访问0x60-0x74
    bool result = AS7341_ModifyRegisterBit(handle, AS7341_CONFIG, enable_led, 3) &&
                  AS7341_ModifyRegisterBit(handle, AS7341_LED, enable_led, 7);
    AS7341_SetBank(handle, false); // 访问0x80及以上寄存器（默认）
    return result;
}

/**
 * @brief 设置LED电流
 */
bool AS7341_SetLEDCurrent(as7341_handle_t *handle, uint16_t led_current_ma)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    // 检查是否在允许范围内
    if (led_current_ma > 258) {
        return false;
    }
    if (led_current_ma < 4) {
        led_current_ma = 4;
    }
    
    AS7341_SetBank(handle, true); // 访问0x60-0x74
    
    bool result = AS7341_ModifyRegisterMultipleBit(handle, AS7341_LED,
                                                   (uint8_t)((led_current_ma - 4) / 2), 0, 7);
    AS7341_SetBank(handle, false); // 访问0x80及以上寄存器（默认）
    return result;
}

/**
 * @brief 禁用所有功能
 */
void AS7341_DisableAll(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return;
    }
    
    AS7341_WriteRegisterByte(handle, AS7341_ENABLE, 0);
}

/**
 * @brief 检查数据是否就绪
 */
bool AS7341_GetIsDataReady(as7341_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_CheckRegisterBit(handle, AS7341_STATUS2, 6);
}

/**
 * @brief 设置寄存器组
 */
bool AS7341_SetBank(as7341_handle_t *handle, bool low)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }
    
    return AS7341_ModifyRegisterBit(handle, AS7341_CFG0, low, 4);
}


// 静态函数实现

/**
 * @brief 初始化设备内部函数
 */
static bool AS7341_InitDevice(as7341_handle_t *handle, int32_t sensor_id)
{
    if (handle == NULL) {
        return false;
    }
    
    // 等待设备初始化完成
    HAL_Delay(1);
    
    // 检查设备ID
    uint8_t chip_id = AS7341_ReadRegisterByte(handle, AS7341_WHOAMI);
    
    if ((chip_id & 0xFC) != (AS7341_CHIP_ID << 2)) {
        return false;  // 设备ID验证失败
    }
    
    // 启用电源 (PON = 1)
    uint8_t enable_reg = AS7341_ReadRegisterByte(handle, AS7341_ENABLE);
    enable_reg = AS7341_ModifyBitInByte(enable_reg, 1, 0);
    bool power_result = AS7341_WriteRegisterByte(handle, AS7341_ENABLE, enable_reg);
    
    if (!power_result) {
        return false;
    }
    
    // 等待设备切换到IDLE状态
    HAL_Delay(2);
    
    return true;
}


/**
 * @brief 启用SMUX
 */
static bool AS7341_EnableSMUX(as7341_handle_t *handle)
{
    if (handle == NULL) {
        return false;
    }
    
    bool success = AS7341_ModifyRegisterBit(handle, AS7341_ENABLE, true, 4);
    
    int timeout = 1000; // 超时值，如果超过1000毫秒则认为出错
    int count = 0;
    while (AS7341_CheckRegisterBit(handle, AS7341_ENABLE, 4) && count < timeout) {
        HAL_Delay(1);
        count++;
    }
    
    if (count >= timeout) {
        return false;
    } else {
        return success;
    }
}



/**
 * @brief 设置SMUX命令
 */
static bool AS7341_SetSMUXCommand(as7341_handle_t *handle, as7341_smux_cmd_t command)
{
    if (handle == NULL) {
        return false;
    }
    
    return AS7341_ModifyRegisterMultipleBit(handle, AS7341_CFG6, command, 3, 2);
}

/**
 * @brief 设置SMUX低通道
 */
static void AS7341_SetSMUXLowChannels(as7341_handle_t *handle, bool f1_f4)
{
    if (handle == NULL) {
        return;
    }
    
    AS7341_EnableSpectralMeasurement(handle, false);
    AS7341_SetSMUXCommand(handle, AS7341_SMUX_CMD_WRITE);
    if (f1_f4) {
        AS7341_Setup_F1F4_Clear_NIR(handle);
    } else {
        AS7341_Setup_F5F8_Clear_NIR(handle);
    }
    AS7341_EnableSMUX(handle);

    HAL_Delay(10);
}

/**
 * @brief 写入寄存器
 */
static bool AS7341_WriteRegister(as7341_handle_t *handle, uint8_t mem_addr, uint8_t *val, uint16_t size)
{
    if (handle == NULL || val == NULL) {
        return false;
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(handle->i2c_handle, handle->i2c_address, 
                                                 mem_addr, 1, val, size, AS7341_TIMEOUT_MS);
    return (status == HAL_OK);
}

/**
 * @brief 写入单个寄存器字节
 */
static bool AS7341_WriteRegisterByte(as7341_handle_t *handle, uint8_t mem_addr, uint8_t val)
{
    if (handle == NULL) {
        return false;
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(handle->i2c_handle, handle->i2c_address, 
                                                 mem_addr, 1, &val, 1, AS7341_TIMEOUT_MS);
    return (status == HAL_OK);
}

/**
 * @brief 读取寄存器
 */
static bool AS7341_ReadRegister(as7341_handle_t *handle, uint16_t mem_addr, uint8_t *dest, uint16_t size)
{
    if (handle == NULL || dest == NULL) {
        return false;
    }
    
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(handle->i2c_handle, handle->i2c_address, 
                                                mem_addr, 1, dest, size, AS7341_TIMEOUT_MS);
    return (status == HAL_OK);
}

/**
 * @brief 读取单个寄存器字节
 */
static uint8_t AS7341_ReadRegisterByte(as7341_handle_t *handle, uint16_t mem_addr)
{
    if (handle == NULL) {
        return 0;
    }
    
    uint8_t data = 0;
    HAL_I2C_Mem_Read(handle->i2c_handle, handle->i2c_address, mem_addr, 1, &data, 1, AS7341_TIMEOUT_MS);
    return data;
}

/**
 * @brief 修改寄存器中的单个位
 */
static bool AS7341_ModifyRegisterBit(as7341_handle_t *handle, uint16_t reg, bool value, uint8_t pos)
{
    if (handle == NULL) {
        return false;
    }
    
    uint8_t register_value = AS7341_ReadRegisterByte(handle, reg);
    register_value = AS7341_ModifyBitInByte(register_value, (uint8_t)value, pos);
    
    return AS7341_WriteRegisterByte(handle, reg, register_value);
}

/**
 * @brief 检查寄存器中的单个位
 */
static uint8_t AS7341_CheckRegisterBit(as7341_handle_t *handle, uint16_t reg, uint8_t pos)
{
    if (handle == NULL) {
        return 0;
    }
    
    return (uint8_t)((AS7341_ReadRegisterByte(handle, reg) >> pos) & 0x01);
}

/**
 * @brief 修改字节中的单个位
 */
static uint8_t AS7341_ModifyBitInByte(uint8_t var, uint8_t value, uint8_t pos)
{
    uint8_t mask = 1 << pos;
    return ((var & ~mask) | (value << pos));
}

/**
 * @brief 修改寄存器中的多个位
 */
static bool AS7341_ModifyRegisterMultipleBit(as7341_handle_t *handle, uint16_t reg, 
                                             uint8_t value, uint8_t pos, uint8_t bits)
{
    if (handle == NULL) {
        return false;
    }
    
    uint8_t register_value = AS7341_ReadRegisterByte(handle, reg);
    
    uint8_t mask = (1 << bits) - 1;
    value &= mask;
    
    mask <<= pos;
    register_value &= ~mask;          // 移除当前位置的数据
    register_value |= value << pos;   // 添加新数据
    
    return AS7341_WriteRegisterByte(handle, reg, register_value);
}
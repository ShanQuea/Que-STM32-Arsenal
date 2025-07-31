/**
 * @file    comm.c
 * @brief   通信库核心API实现
 * @author  ShanQue
 * @version 2.0
 * @date    2025-07-31
 */

#include "comm.h"
#include "comm_internal.h"
#include "comm_protocol.h"
#include "comm_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


bool comm_send_raw(comm_instance_t *instance, const char *data, uint16_t length);


/**
 * @brief  初始化通信库
 * @param  None
 * @retval None
 */
void comm_init(void)
{
    // 调用管理器的初始化函数
    comm_manager_init();
}

/**
 * @brief  添加UART到通信库
 * @param  huart: UART句柄指针
 * @retval true: 添加成功, false: 添加失败  
 */
bool comm_add_uart(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return false;
    }
    
    // 检查是否已存在
    if (comm_find_instance(huart) != NULL) {
        return true;
    }
    
    // 创建实例并自动初始化
    comm_instance_t *instance = comm_create_uart_instance(huart, 1000, 3);
    if (instance == NULL) {
        return false;
    }
    
    // 启动UART中断接收 - 使用实例内部的rx_byte
    HAL_UART_Receive_IT(huart, &instance->rx_byte, 1);
    
    // 启用调试输出
    instance->debug_enabled = true;
    
    return true;
}

/**
 * @brief  注册命令回调函数
 * @param  huart: UART句柄指针
 * @param  cmd: 命令字符串
 * @param  callback: 回调函数指针
 * @retval true: 注册成功, false: 注册失败
 */
bool comm_register_command_callback(UART_HandleTypeDef *huart, 
                                   const char *cmd, 
                                   comm_callback_t callback)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        COMM_DEBUG_ERROR("未找到UART实例");
        COMM_ERROR_OUTPUT("UART操作失败: 未找到UART实例 %p", huart);
        return false;
    }

    return comm_register_callback(instance, cmd, callback);
}

/**
 * @brief  设置失败回调函数
 * @param  huart: UART句柄指针
 * @param  callback: 失败回调函数指针
 * @retval true: 设置成功, false: 设置失败
 */
bool comm_register_fail_callback(UART_HandleTypeDef *huart, 
                                comm_fail_callback_t callback)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        COMM_DEBUG_ERROR("未找到UART实例");
        COMM_ERROR_OUTPUT("UART操作失败: 未找到UART实例 %p", huart);
        return false;
    }

    comm_set_fail_callback(instance, callback);
    return true;
}

/**
 * @brief  设置状态变化回调函数
 * @param  huart: UART句柄指针
 * @param  callback: 状态变化回调函数指针
 * @retval true: 设置成功, false: 设置失败
 */
bool comm_register_state_change_callback(UART_HandleTypeDef *huart, 
                                        comm_state_change_callback_t callback)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        COMM_DEBUG_ERROR("未找到UART实例");
        COMM_ERROR_OUTPUT("UART操作失败: 未找到UART实例 %p", huart);
        return false;
    }

    instance->state_change_callback = callback;
    return true;
}

/**
 * @brief  发送命令（异步）
 * @param  huart: UART句柄指针
 * @param  cmd: 命令字符串
 * @param  data: 数据字符串
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_send_command(UART_HandleTypeDef *huart, const char *cmd, const char *data)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        COMM_DEBUG_ERROR("未找到UART实例");
        COMM_ERROR_OUTPUT("UART操作失败: 未找到UART实例 %p", huart);
        return false;
    }

    if (!comm_instance_is_ready(instance)) {
        return false;
    }

    if (cmd == NULL || data == NULL) {
        return false;
    }

    if (strlen(cmd) >= COMM_MAX_CMD_LENGTH || strlen(data) >= COMM_MAX_DATA_LENGTH) {
        return false;
    }

    // 保存当前命令和数据（用于重试）
    strncpy(instance->current_cmd, cmd, sizeof(instance->current_cmd) - 1);
    instance->current_cmd[sizeof(instance->current_cmd) - 1] = '\0';
    
    strncpy(instance->current_data, data, sizeof(instance->current_data) - 1);
    instance->current_data[sizeof(instance->current_data) - 1] = '\0';
    instance->retry_count = 0;

    uint16_t frame_len;
    if (!comm_build_frame(instance, cmd, data, instance->tx_buffer, &frame_len)) {
        return false;
    }

    instance->tx_length = frame_len;
    

    if (strlen(instance->pending_frame.cmd) > 0) {
        memset(&instance->pending_frame, 0, sizeof(comm_frame_t));
    }

    char send_copy[COMM_TX_BUFFER_SIZE];
    memcpy(send_copy, instance->tx_buffer, frame_len);
    send_copy[frame_len] = '\0';
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(instance->huart, 
                                                (uint8_t*)send_copy, 
                                                frame_len, 1000);  // 1秒超时

    if (status == HAL_OK) {
        comm_set_state(instance, COMM_STATE_WAIT_ACK);
        instance->last_send_time = HAL_GetTick();

        return true;
    } else {
        return false;
    }
}

/**
 * @brief  发送PING命令测试连通性
 * @param  huart: UART句柄指针
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_ping(UART_HandleTypeDef *huart)
{
    return comm_send_command(huart, COMM_CMD_PING, "TEST");
}

/**
 * @brief  检查UART实例是否就绪
 * @param  huart: UART句柄指针
 * @retval true: 就绪, false: 未就绪
 */
bool comm_is_ready(UART_HandleTypeDef *huart)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        return false;
    }

    return comm_instance_is_ready(instance);
}

/**
 * @brief  获取UART实例的详细状态信息
 * @param  huart: UART句柄指针
 * @retval 状态字符串
 */
const char* comm_get_state_string(UART_HandleTypeDef *huart)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        return "NOT_FOUND";
    }
    
    switch (instance->state) {
        case COMM_STATE_IDLE:       return "IDLE";
        case COMM_STATE_SENDING:    return "SENDING";
        case COMM_STATE_WAIT_ACK:   return "WAIT_ACK";
        case COMM_STATE_RETRY:      return "RETRY";
        case COMM_STATE_RECEIVING:  return "RECEIVING";
        case COMM_STATE_PROCESSING: return "PROCESSING";
        case COMM_STATE_ERROR:      return "ERROR";
        default:                    return "UNKNOWN";
    }
}

/**
 * @brief  获取UART实例的重试次数
 * @param  huart: UART句柄指针
 * @retval 当前重试次数
 */
uint8_t comm_get_retry_count(UART_HandleTypeDef *huart)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        return 0;
    }
    
    return instance->retry_count;
}


/**
 * @brief  定时处理函数（在定时器中断中调用）
 * @param  None
 * @retval None
 */
void comm_tick(void)
{
    uint8_t count = comm_get_instance_count();
    for (uint8_t i = 0; i < count; i++) {
        comm_instance_t *instance = comm_get_instance_by_index(i);
        if (instance == NULL) continue;

        if (comm_is_timeout(instance)) {
            comm_handle_timeout(instance);
        }

        if (comm_is_frame_timeout(instance)) {
            comm_handle_frame_timeout(instance);
        }

        // 处理完整帧
        if (instance->new_frame_available) {
            comm_handle_complete_frame(instance, &instance->pending_frame);
            instance->new_frame_available = false;
            memset(&instance->pending_frame, 0, sizeof(comm_frame_t));
        }
    }
}


/* =============================================================================
 * 统一HAL回调处理函数
 * =============================================================================
 */

/**
 * @brief  统一UART接收回调处理 - 在HAL_UART_RxCpltCallback中调用
 * @param  huart: UART句柄指针
 * @retval None
 */
void comm_uart_rx_callback(UART_HandleTypeDef *huart)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        return;
    }

    uint32_t uart_errors = huart->ErrorCode;
    if (uart_errors != HAL_UART_ERROR_NONE) {
        huart->ErrorCode = HAL_UART_ERROR_NONE;

        COMM_DEBUG_INSTANCE(instance, "UART错误: 0x%08lx", uart_errors);

        if (uart_errors & HAL_UART_ERROR_ORE) {
            HAL_UART_AbortReceive_IT(huart);
            instance->parse_state = FRAME_STATE_IDLE;  // 重置解析状态
            instance->rx_index = 0;
        }
    }
    
    // 处理接收的字节 - 使用实例内部的rx_byte
    comm_process_byte_in_interrupt(instance, instance->rx_byte);
    
    // 立即启动下一个字节接收，避免overrun
    HAL_UART_Receive_IT(huart, &instance->rx_byte, 1);
}

/**
 * @brief  统一UART发送回调处理 - 不再使用(暂时留着 还有新想法 下次更新)
 * @param  huart: UART句柄指针
 * @retval None
 */
void comm_uart_tx_callback(UART_HandleTypeDef *huart)
{

}

/**
 * @brief  统一UART错误回调处理 - 在HAL_UART_ErrorCallback中调用
 * @param  huart: UART句柄指针
 * @retval None
 */
void comm_uart_error_callback(UART_HandleTypeDef *huart)
{
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance != NULL) {
        HAL_UART_Receive_IT(huart, &instance->rx_byte, 1);
    }
}
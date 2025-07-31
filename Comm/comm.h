/**
  ******************************************************************************
  * @file           : comm.h
  * @author         : ShanQue
  * @brief          : STM32串口通信协议库
  * @date           : 2025/07/31
  * @version        : 2.0.0
  ******************************************************************************
  *
  * 协议格式: {CMD:DATA#SEQ#CRC}
  * 使用示例:
  *   comm_init();
  *   comm_add_uart(&huart2);
  *   comm_add_uart(&huart3);
  *   comm_register_command_callback(&huart2, "TEST", my_callback);
  *   comm_send_command(&huart2, "GET", "TEMP");
  * 
  ******************************************************************************
  */

#ifndef COMM_H
#define COMM_H


#include "stm32f4xx_hal.h"
#include "usart.h"
#include <stdbool.h>
#include <stdint.h>

struct comm_stats_t;
typedef struct comm_stats_t comm_stats_t;
struct comm_instance;
typedef struct comm_instance comm_instance_t;

typedef void (*comm_callback_t)(const char *cmd, const char *data);
typedef void (*comm_fail_callback_t)(const char* cmd, const char* data, const char* reason);
typedef void (*comm_state_change_callback_t)(UART_HandleTypeDef *huart, 
                                            const char* from_state, 
                                            const char* to_state, 
                                            uint8_t retry_count);

/* =============================================================================
 * 核心API
 * =============================================================================
 */

/**
 * @brief  初始化通信库
 * @param  None
 * @retval None
 */
void comm_init(void);

/**
 * @brief  添加UART到通信库（自动处理所有细节）
 * @param  huart: UART句柄指针
 * @retval true: 添加成功, false: 添加失败  
 * @note   自动创建实例、启动中断接收、处理HAL回调
 */
bool comm_add_uart(UART_HandleTypeDef *huart);

/**
 * @brief  发送命令（异步）
 * @param  huart: UART句柄指针
 * @param  cmd: 命令字符串
 * @param  data: 数据字符串
 * @retval true: 发送成功, false: 发送失败
 * @note   异步发送，支持自动重试和ACK确认
 */
bool comm_send_command(UART_HandleTypeDef *huart, const char* cmd, const char* data);

/**
 * @brief  注册命令回调函数
 * @param  huart: UART句柄指针
 * @param  cmd: 要监听的命令字符串
 * @param  callback: 回调函数指针
 * @retval true: 注册成功, false: 注册失败
 */
bool comm_register_command_callback(UART_HandleTypeDef *huart, const char* cmd, comm_callback_t callback);

/**
 * @brief  注册失败回调函数
 * @param  huart: UART句柄指针
 * @param  callback: 失败回调函数指针
 * @retval true: 注册成功, false: 注册失败
 */
bool comm_register_fail_callback(UART_HandleTypeDef *huart, comm_fail_callback_t callback);

/**
 * @brief  注册状态变化回调函数
 * @param  huart: UART句柄指针
 * @param  callback: 状态变化回调函数指针
 * @retval true: 注册成功, false: 注册失败
 * @note   当通信状态发生变化时自动调用此回调，无需应用层轮询
 */
bool comm_register_state_change_callback(UART_HandleTypeDef *huart, comm_state_change_callback_t callback);

/**
 * @brief  处理通信事务（在定时器中断中调用）
 * @param  None
 * @retval None
 * @note   必须在定时器中断中调用（建议1ms间隔）
 */
void comm_tick(void);

/* =============================================================================
 * 辅助函数
 * =============================================================================
 */

/**
 * @brief  发送PING命令
 * @param  huart: UART句柄指针
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_ping(UART_HandleTypeDef *huart);

/**
 * @brief  检查UART是否就绪
 * @param  huart: UART句柄指针
 * @retval true: 就绪, false: 未就绪
 */
bool comm_is_ready(UART_HandleTypeDef *huart);

/**
 * @brief  获取UART实例的详细状态信息
 * @param  huart: UART句柄指针
 * @retval 状态字符串（"IDLE", "SENDING", "WAIT_ACK", "RETRY", "ERROR"）
 */
const char* comm_get_state_string(UART_HandleTypeDef *huart);

/**
 * @brief  获取UART实例的重试次数
 * @param  huart: UART句柄指针
 * @retval 当前重试次数
 */
uint8_t comm_get_retry_count(UART_HandleTypeDef *huart);

/* =============================================================================
 * HAL回调集成函数 - 在HAL回调中调用
 * =============================================================================
 */

/**
 * @brief  UART接收完成回调
 * @param  huart: UART句柄指针
 * @retval None
 * @note   在HAL_UART_RxCpltCallback中调用此函数
 */
void comm_uart_rx_callback(UART_HandleTypeDef *huart);

/**
 * @brief  UART发送完成回调
 * @param  huart: UART句柄指针
 * @retval None
 * @note   在HAL_UART_TxCpltCallback中调用此函数
 */
void comm_uart_tx_callback(UART_HandleTypeDef *huart);

/**
 * @brief  UART错误回调
 * @param  huart: UART句柄指针
 * @retval None
 * @note   在HAL_UART_ErrorCallback中调用此函数
 */
void comm_uart_error_callback(UART_HandleTypeDef *huart);

#endif /* COMM_H */
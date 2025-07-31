/**
 ******************************************************************************
 * @file           : comm_manager.h
 * @author         : ShanQue
 * @brief          : STM32串口通信实例管理层
 * @date           : 2025/07/31
 * @version        : 2.0.0
 ******************************************************************************
 * 
 * 实例管理层 - 负责UART实例管理、回调处理、超时重试
 * 
 ******************************************************************************
 */

#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#include "comm_internal.h"

/* =============================================================================
 * 全局管理器函数
 * =============================================================================
 */

/**
 * @brief  初始化全局管理器
 * @param  None
 * @retval None
 */
void comm_manager_init(void);

/**
 * @brief  获取实例计数
 * @param  None
 * @retval 当前实例数量
 */
uint8_t comm_get_instance_count(void);

/**
 * @brief  获取指定索引的实例
 * @param  index: 实例索引
 * @retval 实例指针，索引无效返回NULL
 */
comm_instance_t* comm_get_instance_by_index(uint8_t index);

/* =============================================================================
 * 实例管理函数
 * =============================================================================
 */

/**
 * @brief  初始化实例
 * @param  instance: 实例指针
 * @param  huart: UART句柄指针
 * @param  timeout_ms: 超时时间（毫秒）
 * @param  max_retry: 最大重试次数
 * @retval true: 初始化成功, false: 初始化失败
 */
bool comm_instance_init(comm_instance_t *instance, 
                       UART_HandleTypeDef *huart,
                       uint32_t timeout_ms, 
                       uint8_t max_retry);

/**
 * @brief  检查实例是否就绪
 * @param  instance: 实例指针
 * @retval true: 就绪, false: 未就绪
 */
bool comm_instance_is_ready(comm_instance_t *instance);

/**
 * @brief  获取实例当前状态
 * @param  instance: 实例指针
 * @retval 状态值
 */
uint8_t comm_instance_get_state(comm_instance_t *instance);

/**
 * @brief  重置实例状态
 * @param  instance: 实例指针
 * @retval None
 */
void comm_instance_reset(comm_instance_t *instance);

/* =============================================================================
 * 回调管理函数
 * =============================================================================
 */

/**
 * @brief  注册命令回调函数
 * @param  instance: 实例指针
 * @param  cmd: 命令字符串
 * @param  callback: 回调函数指针
 * @retval true: 注册成功, false: 注册失败
 */
bool comm_register_callback(comm_instance_t *instance, const char *cmd, comm_callback_t callback);

/**
 * @brief  查找并调用命令回调函数
 * @param  instance: 实例指针
 * @param  cmd: 命令字符串
 * @param  data: 数据字符串
 * @retval true: 找到并调用了回调, false: 未找到回调
 */
bool comm_call_callback(comm_instance_t *instance, const char *cmd, const char *data);

/**
 * @brief  设置失败回调函数
 * @param  instance: 实例指针
 * @param  callback: 失败回调函数指针
 * @retval None
 */
void comm_set_fail_callback(comm_instance_t *instance, comm_fail_callback_t callback);

/**
 * @brief  调用失败回调函数
 * @param  instance: 实例指针
 * @param  cmd: 命令字符串
 * @param  data: 数据字符串
 * @param  reason: 失败原因
 * @retval None
 */
void comm_call_fail_callback(comm_instance_t *instance, const char *cmd, const char *data, const char *reason);

/* =============================================================================
 * 超时和重试管理函数
 * =============================================================================
 */

/**
 * @brief  检查是否超时
 * @param  instance: 实例指针
 * @retval true: 已超时, false: 未超时
 */
bool comm_is_timeout(comm_instance_t *instance);

/**
 * @brief  处理超时事件
 * @param  instance: 实例指针
 * @retval None
 */
void comm_handle_timeout(comm_instance_t *instance);

/**
 * @brief  检查帧接收是否超时
 * @param  instance: 实例指针
 * @retval true: 已超时, false: 未超时
 */
bool comm_is_frame_timeout(comm_instance_t *instance);

/**
 * @brief  处理帧接收超时事件
 * @param  instance: 实例指针
 * @retval None
 */
void comm_handle_frame_timeout(comm_instance_t *instance);

/**
 * @brief  设置实例状态
 * @param  instance: 实例指针
 * @param  new_state: 新状态
 * @retval None
 */
void comm_set_state(comm_instance_t *instance, uint8_t new_state);

/**
 * @brief  发送原始数据（阻塞方式）
 * @param  instance: 实例指针
 * @param  data: 数据指针
 * @param  length: 数据长度
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_send_raw(comm_instance_t *instance, const char *data, uint16_t length);

/* =============================================================================
 * 全局实例查找函数
 * =============================================================================
 */

/**
 * @brief  根据UART句柄查找实例
 * @param  huart: UART句柄指针
 * @retval 实例指针，未找到返回NULL
 */
comm_instance_t* comm_find_instance(UART_HandleTypeDef *huart);

/**
 * @brief  创建新的UART实例
 * @param  huart: UART句柄指针
 * @param  timeout_ms: 超时时间（毫秒）
 * @param  max_retry: 最大重试次数
 * @retval 实例指针，创建失败返回NULL
 */
comm_instance_t* comm_create_uart_instance(UART_HandleTypeDef *huart, 
                                          uint32_t timeout_ms, 
                                          uint8_t max_retry);

#endif /* COMM_MANAGER_H */
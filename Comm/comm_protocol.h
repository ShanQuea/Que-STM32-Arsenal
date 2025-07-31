/**
 ******************************************************************************
 * @file           : comm_protocol.h
 * @author         : ShanQue
 * @brief          : STM32串口通信协议处理层
 * @date           : 2025/07/31
 * @version        : 2.0.0
 ******************************************************************************
 * 
 * 协议处理层 - 负责帧构建、解析、CRC计算、序列号管理
 * 
 ******************************************************************************
 */

#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include "comm_internal.h"

/* =============================================================================
 * CRC计算函数
 * =============================================================================
 */

/**
 * @brief  计算CRC8校验码
 * @param  data: 数据指针
 * @param  length: 数据长度
 * @retval CRC8校验码
 */
uint8_t comm_crc8_calculate(const uint8_t *data, uint16_t length);

/**
 * @brief  验证CRC8校验码
 * @param  data: 数据指针
 * @param  length: 数据长度
 * @param  expected_crc: 期望的CRC值
 * @retval true: 校验通过, false: 校验失败
 */
bool comm_crc8_verify(const uint8_t *data, uint16_t length, uint8_t expected_crc);

/* =============================================================================
 * 序列号管理函数
 * =============================================================================
 */

/**
 * @brief  获取下一个发送序列号
 * @param  instance: 实例指针
 * @retval 序列号
 */
uint8_t comm_get_next_tx_sequence(comm_instance_t *instance);

/**
 * @brief  检查接收序列号的有效性
 * @param  instance: 实例指针
 * @param  rx_seq: 接收到的序列号
 * @retval true: 有效, false: 无效/重复
 */
bool comm_is_valid_rx_sequence(comm_instance_t *instance, uint8_t rx_seq);

/**
 * @brief  更新接收序列号
 * @param  instance: 实例指针
 * @param  rx_seq: 接收到的序列号
 */
void comm_update_rx_sequence(comm_instance_t *instance, uint8_t rx_seq);

/* =============================================================================
 * 帧处理函数
 * =============================================================================
 */

/**
 * @brief  构建完整的通信帧
 * @param  instance: 实例指针
 * @param  cmd: 命令字符串
 * @param  data: 数据字符串
 * @param  frame_buffer: 帧缓冲区
 * @param  frame_length: 帧长度指针
 * @retval true: 构建成功, false: 构建失败
 */
bool comm_build_frame(comm_instance_t *instance, const char *cmd, const char *data, 
                     char *frame_buffer, uint16_t *frame_length);

/**
 * @brief  在中断中处理接收的字节
 * @param  instance: 实例指针
 * @param  byte: 接收到的字节
 * @retval None
 * @note   此函数在中断上下文中调用，应保持简短
 */
void comm_process_byte_in_interrupt(comm_instance_t *instance, uint8_t byte);

/**
 * @brief  处理完整的接收帧
 * @param  instance: 实例指针
 * @param  frame: 完整帧指针
 * @retval None
 * @note   在主循环或定时器中断中调用
 */
void comm_handle_complete_frame(comm_instance_t *instance, const comm_frame_t *frame);

/**
 * @brief  发送ACK确认帧
 * @param  instance: 实例指针
 * @param  ack_seq: 确认的序列号
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_send_ack(comm_instance_t *instance, uint8_t ack_seq);

/**
 * @brief  发送NAK否认帧
 * @param  instance: 实例指针
 * @param  nak_seq: 否认的序列号
 * @param  reason: 错误原因
 * @retval true: 发送成功, false: 发送失败
 */
bool comm_send_nak(comm_instance_t *instance, uint8_t nak_seq, const char *reason);

#endif /* COMM_PROTOCOL_H */
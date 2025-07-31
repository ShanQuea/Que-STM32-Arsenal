/**
 * @file    comm_protocol.c
 * @brief   通信库协议处理层 - 帧构建、解析、CRC计算、序列号管理
 * @author  ShanQue
 * @version 2.0
 * @date    2025-07-31
 */


#include "comm_protocol.h"
#include "comm_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if COMM_ENABLE_FAST_CRC
/* CRC8-CCITT查表法（0x07多项式） */
static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};
#endif

/* =============================================================================
 * CRC计算函数实现
 * =============================================================================
 */

uint8_t comm_crc8_calculate(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return 0;
    }
    
#if COMM_ENABLE_FAST_CRC
    // 查表法计算CRC8-CCITT（高速）
    uint8_t crc = 0;
    for (uint16_t i = 0; i < length; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
#else
    // 直接计算法CRC8-CCITT（节省空间）
    uint8_t crc = 0;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;  // 0x07是CRC8-CCITT多项式
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
#endif
}

bool comm_crc8_verify(const uint8_t *data, uint16_t length, uint8_t expected_crc)
{
    uint8_t calculated_crc = comm_crc8_calculate(data, length);
    return (calculated_crc == expected_crc);
}

/* =============================================================================
 * 序列号管理函数实现
 * =============================================================================
 */

uint8_t comm_get_next_tx_sequence(comm_instance_t *instance)
{
    if (instance == NULL) {
        return 0;
    }
    
    instance->tx_sequence++;
    
    // 序列号为0时跳过（保留给特殊用途）
    if (instance->tx_sequence == 0) {
        instance->tx_sequence = 1;
    }
    
    return instance->tx_sequence;
}

bool comm_is_valid_rx_sequence(comm_instance_t *instance, uint8_t rx_seq)
{
    if (instance == NULL) {
        return false;
    }

    int16_t diff = (int16_t)rx_seq - (int16_t)instance->rx_sequence;

    if (diff < -128) {
        diff += 256;  // 正向回绕：255->1
    } else if (diff > 128) {
        diff -= 256;
    }
    
    // 接受策略：
    // 1. 序列号递增1（正常情况）
    // 2. 序列号递增2-10（允许少量丢包）
    // 3. 拒绝重复序列号（diff <= 0）
    // 4. 拒绝跳跃过大的序列号（diff > 10）
    
    if (diff >= 1 && diff <= 10) {
        return true;
    } else if (diff == 0) {
        // 重复序列号，需要重发ACK
        #if COMM_ENABLE_STATS
        instance->stats.rx_seq_error++;
        #endif
        COMM_DEBUG_INSTANCE(instance, "重复序列号: %d", rx_seq);
        return false;
    } else if (diff < 0) {
        // 序列号倒退，拒绝
        #if COMM_ENABLE_STATS
        instance->stats.rx_seq_error++;
        #endif
        COMM_DEBUG_INSTANCE(instance, "序列号倒退: %d -> %d (差值=%d)", 
                           instance->rx_sequence, rx_seq, diff);
        return false;
    } else {
        // 序列号跳跃过大，拒绝
        #if COMM_ENABLE_STATS
        instance->stats.rx_seq_error++;
        #endif
        COMM_DEBUG_INSTANCE(instance, "序列号跳跃过大: %d -> %d (差值=%d)", 
                           instance->rx_sequence, rx_seq, diff);
        return false;
    }
}

void comm_update_rx_sequence(comm_instance_t *instance, uint8_t rx_seq)
{
    if (instance != NULL) {
        instance->rx_sequence = rx_seq;
    }
}

/* =============================================================================
 * 帧处理函数实现
 * =============================================================================
 */

bool comm_build_frame(comm_instance_t *instance, const char *cmd, const char *data, 
                     char *frame_buffer, uint16_t *frame_length)
{
    if (instance == NULL || cmd == NULL || data == NULL || frame_buffer == NULL || frame_length == NULL) {
        return false;
    }
    
    // 检查命令和数据长度
    if (strlen(cmd) > COMM_MAX_CMD_LENGTH || strlen(data) > COMM_MAX_DATA_LENGTH) {
        COMM_DEBUG_INSTANCE(instance, "帧构建失败: 命令或数据过长");
        return false;
    }
    
    // 获取序列号（如果是重试，使用保存的序列号；否则获取新序列号）
    uint8_t seq;
    if (instance->retry_count > 0) {
        // 重试时使用保存的序列号
        seq = instance->current_sequence;
    } else {
        // 新发送时获取新序列号并保存
        seq = comm_get_next_tx_sequence(instance);
        instance->current_sequence = seq;
    }

    char frame_content[COMM_TX_BUFFER_SIZE];
    int content_len = snprintf(frame_content, sizeof(frame_content), 
                              "%c%s%c%s%c%02X", 
                              COMM_FRAME_START, cmd, COMM_CMD_DATA_SEPARATOR, 
                              data, COMM_FIELD_SEPARATOR, seq);
    
    if (content_len < 0 || content_len >= sizeof(frame_content)) {
        COMM_DEBUG_INSTANCE(instance, "帧构建失败: 格式化错误");
        return false;
    }
    
    // 计算CRC（只对内容部分，不包括起始符）
    uint8_t crc = comm_crc8_calculate((uint8_t*)(frame_content + 1), content_len - 1);
    
    // 构建完整帧
    int total_len = snprintf(frame_buffer, COMM_TX_BUFFER_SIZE, 
                            "%s%c%02X%c",
                            frame_content, COMM_FIELD_SEPARATOR, crc, COMM_FRAME_END);
    
    if (total_len < 0 || total_len >= COMM_TX_BUFFER_SIZE) {
        COMM_DEBUG_INSTANCE(instance, "帧构建失败: 输出缓冲区溢出");
        return false;
    }
    
    *frame_length = (uint16_t)total_len;
    instance->expected_ack_seq = seq;  // 期望收到确认这个序列号的ACK
    
    COMM_DEBUG_INSTANCE(instance, "设置expected_ack_seq=%d (发送seq=%d)", 
                       instance->expected_ack_seq, seq);
    
    return true;
}

void comm_process_byte_in_interrupt(comm_instance_t *instance, uint8_t byte)
{
    if (instance == NULL) {
        return;
    }
    

    if (instance->new_frame_available) {
        return;
    }
    
    switch (instance->parse_state) {
        case FRAME_STATE_IDLE:
            if (byte == COMM_FRAME_START) {
                instance->parse_state = FRAME_STATE_CMD;
                instance->rx_index = 0;
                instance->frame_timeout = HAL_GetTick() + COMM_FRAME_TIMEOUT_MS;
                // 确保清空pending_frame缓冲区
                memset(&instance->pending_frame, 0, sizeof(comm_frame_t));
            }
            break;
            
        case FRAME_STATE_CMD:
            if (byte == COMM_CMD_DATA_SEPARATOR) {
                instance->pending_frame.cmd[instance->rx_index] = '\0';
                instance->parse_state = FRAME_STATE_DATA;
                instance->rx_index = 0;
            } else if (instance->rx_index < COMM_MAX_CMD_LENGTH) {
                instance->pending_frame.cmd[instance->rx_index++] = byte;
            } else {
                // 命令过长，重置
                instance->parse_state = FRAME_STATE_IDLE;
            }
            break;
            
        case FRAME_STATE_DATA:
            if (byte == COMM_FIELD_SEPARATOR) {
                instance->pending_frame.data[instance->rx_index] = '\0';
                instance->parse_state = FRAME_STATE_SEQ;
                instance->rx_index = 0;
            } else if (instance->rx_index < COMM_MAX_DATA_LENGTH) {
                instance->pending_frame.data[instance->rx_index++] = byte;
            } else {
                // 数据过长，重置
                instance->parse_state = FRAME_STATE_IDLE;
            }
            break;
            
        case FRAME_STATE_SEQ:
            if (byte == COMM_FIELD_SEPARATOR) {
                // 解析序列号（十六进制字符串）
                instance->rx_buffer[instance->rx_index] = '\0';
                instance->pending_frame.sequence = (uint8_t)strtol(instance->rx_buffer, NULL, 16);
                instance->parse_state = FRAME_STATE_CRC;
                instance->rx_index = 0;  // 重置索引为CRC解析做准备
            } else if (instance->rx_index < 2) { // 序列号最多2个十六进制字符
                instance->rx_buffer[instance->rx_index++] = byte;
            } else {
                // 序列号格式错误，重置
                instance->parse_state = FRAME_STATE_IDLE;
            }
            break;
            
        case FRAME_STATE_CRC:
            if (byte == COMM_FRAME_END) {
                // 解析CRC（十六进制字符串）
                instance->rx_buffer[instance->rx_index] = '\0';
                instance->pending_frame.crc = (uint8_t)strtol(instance->rx_buffer, NULL, 16);
                
                // 验证CRC
                char frame_for_crc[COMM_RX_BUFFER_SIZE];
                int crc_len = snprintf(frame_for_crc, sizeof(frame_for_crc), 
                                      "%s%c%s%c%02X",
                                      instance->pending_frame.cmd, COMM_CMD_DATA_SEPARATOR,
                                      instance->pending_frame.data, COMM_FIELD_SEPARATOR,
                                      instance->pending_frame.sequence);
                
                uint8_t calculated_crc = comm_crc8_calculate((uint8_t*)frame_for_crc, crc_len);
                
                if (crc_len > 0 && comm_crc8_verify((uint8_t*)frame_for_crc, crc_len, instance->pending_frame.crc)) {
                    instance->pending_frame.is_valid = true;
                    instance->new_frame_available = true;
                } else {
                    instance->pending_frame.is_valid = false;
                }
                
                instance->parse_state = FRAME_STATE_IDLE;
                instance->rx_index = 0;
            } else if (instance->rx_index < 2) { // CRC最多2个十六进制字符
                instance->rx_buffer[instance->rx_index++] = byte;
            } else {
                // CRC格式错误，重置
                instance->parse_state = FRAME_STATE_IDLE;
            }
            break;
            
        default:
            instance->parse_state = FRAME_STATE_IDLE;
            break;
    }
}

void comm_handle_complete_frame(comm_instance_t *instance, const comm_frame_t *frame)
{
    if (instance == NULL || frame == NULL) {
        return;
    }

    if (!frame->is_valid) {
        char frame_for_crc[COMM_RX_BUFFER_SIZE];
        int crc_len = snprintf(frame_for_crc, sizeof(frame_for_crc), 
                              "%s%c%s%c%02X",
                              frame->cmd, COMM_CMD_DATA_SEPARATOR,
                              frame->data, COMM_FIELD_SEPARATOR,
                              frame->sequence);
        
        uint8_t calculated_crc = comm_crc8_calculate((uint8_t*)frame_for_crc, crc_len);
        COMM_DEBUG_INSTANCE(instance, "CRC校验失败: 接收=%02X, 计算=%02X, 数据='%s'", 
                           frame->crc, calculated_crc, frame_for_crc);
        return;  // CRC失败，不处理帧
    }

    if (strcmp(frame->cmd, COMM_CMD_ACK) == 0) {
        uint8_t ack_seq = (uint8_t)strtol(frame->data, NULL, 16);
        
        if (ack_seq == instance->expected_ack_seq && instance->state == COMM_STATE_WAIT_ACK) {
            comm_set_state(instance, COMM_STATE_IDLE);
            instance->retry_count = 0;
            
            #if COMM_ENABLE_STATS
            comm_update_tx_stats(instance, true);
            uint32_t delay = HAL_GetTick() - instance->last_send_time;
            if (delay < instance->stats.min_delay_ms) instance->stats.min_delay_ms = delay;
            if (delay > instance->stats.max_delay_ms) instance->stats.max_delay_ms = delay;
            #endif
        } else {
            COMM_DEBUG_INSTANCE(instance, "ACK不匹配: ack_seq=%d vs expected=%d, state=%d", 
                               ack_seq, instance->expected_ack_seq, instance->state);
        }
        return;
    }
    
    if (strcmp(frame->cmd, COMM_CMD_NAK) == 0) {
        uint8_t nak_seq = (uint8_t)strtol(frame->data, NULL, 16);
        
        if (nak_seq == instance->expected_ack_seq && instance->state == COMM_STATE_WAIT_ACK) {
            COMM_DEBUG_INSTANCE(instance, "收到NAK否认，seq=%d", nak_seq);
            comm_handle_timeout(instance);
        }
        return;
    }
    
    // 处理普通命令帧
    if (comm_is_valid_rx_sequence(instance, frame->sequence)) {
        if (instance->state == COMM_STATE_WAIT_ACK && 
            frame->sequence == instance->expected_ack_seq) {
            COMM_DEBUG_INSTANCE(instance, "软件防回环：忽略seq=%d的%s帧", 
                               frame->sequence, frame->cmd);
            return;
        }

        comm_update_rx_sequence(instance, frame->sequence);
        comm_send_ack(instance, frame->sequence);
        
        // 调用用户回调函数
        if (comm_call_callback(instance, frame->cmd, frame->data)) {
        } else {
            COMM_DEBUG_INSTANCE(instance, "忽略未注册命令: %s", frame->cmd);
        }
        
        #if COMM_ENABLE_STATS
        comm_update_rx_stats(instance, true, COMM_ERR_NONE);
        #endif
    } else {
        // 检查是否为重复序列号
        int16_t diff = (int16_t)frame->sequence - (int16_t)instance->rx_sequence;
        if (diff < -128) diff += 256;
        else if (diff > 128) diff -= 256;
        
        if (diff == 0) {
            // 重复序列号：重发ACK但不更新序列号，不调用回调
            COMM_DEBUG_INSTANCE(instance, "重复序列号，重发ACK: %d", frame->sequence);
            comm_send_ack(instance, frame->sequence);
        } else {
            COMM_DEBUG_INSTANCE(instance, "序列号错误，发送NAK: %d", frame->sequence);
            comm_send_nak(instance, frame->sequence, "SEQ_ERROR");
        }
        
        #if COMM_ENABLE_STATS
        comm_update_rx_stats(instance, false, COMM_ERR_SEQUENCE_ERROR);
        #endif
    }
}

bool comm_send_ack(comm_instance_t *instance, uint8_t ack_seq)
{
    if (instance == NULL) {
        return false;
    }
    
    COMM_DEBUG_INSTANCE(instance, "准备发送ACK: 目标seq=%d", ack_seq);
    
    char ack_data[8];
    snprintf(ack_data, sizeof(ack_data), "%02X", ack_seq);
    
    // ACK帧直接构建，不参与序列号管理
    // 格式: {ACK:01#00#CRC}，序列号固定为00
    char frame_content[COMM_TX_BUFFER_SIZE];
    int content_len = snprintf(frame_content, sizeof(frame_content), 
                              "%s%c%s%c00",
                              COMM_CMD_ACK, COMM_CMD_DATA_SEPARATOR,
                              ack_data, COMM_FIELD_SEPARATOR);
    
    if (content_len < 0 || content_len >= sizeof(frame_content)) {
        return false;
    }
    
    // 计算CRC
    uint8_t crc = comm_crc8_calculate((uint8_t*)frame_content, content_len);
    
    // 构建完整ACK帧
    char ack_frame[COMM_TX_BUFFER_SIZE];
    int total_len = snprintf(ack_frame, sizeof(ack_frame),
                            "%c%s%c%02X%c",
                            COMM_FRAME_START, frame_content, 
                            COMM_FIELD_SEPARATOR, crc, COMM_FRAME_END);
    
    if (total_len < 0 || total_len >= sizeof(ack_frame)) {
        return false;
    }
    
    COMM_DEBUG_INSTANCE(instance, "发送ACK帧: %s", ack_frame);
    COMM_DEBUG_INSTANCE(instance, "*** 即将发送ACK，UART地址=0x%08lx", (uint32_t)instance->huart);
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(instance->huart, 
                                                (uint8_t*)ack_frame, 
                                                total_len, 500);  // 500ms超时
    
    if (status == HAL_OK) {
        COMM_DEBUG_INSTANCE(instance, "ACK发送成功");
        return true;
    } else {
        COMM_DEBUG_INSTANCE(instance, "ACK发送失败，状态: %d", status);
        return false;
    }
}

bool comm_send_nak(comm_instance_t *instance, uint8_t nak_seq, const char *reason)
{
    if (instance == NULL) {
        return false;
    }
    
    char nak_data[16];
    snprintf(nak_data, sizeof(nak_data), "%02X", nak_seq);
    
    // NAK帧直接构建，不参与序列号管理  
    // 格式: {NAK:01#00#CRC}，序列号固定为00
    char frame_content[COMM_TX_BUFFER_SIZE];
    int content_len = snprintf(frame_content, sizeof(frame_content),
                              "%s%c%s%c00", 
                              COMM_CMD_NAK, COMM_CMD_DATA_SEPARATOR,
                              nak_data, COMM_FIELD_SEPARATOR);
    
    if (content_len < 0 || content_len >= sizeof(frame_content)) {
        return false;
    }
    
    // 计算CRC
    uint8_t crc = comm_crc8_calculate((uint8_t*)frame_content, content_len);
    
    // 构建完整NAK帧
    char nak_frame[COMM_TX_BUFFER_SIZE];
    int total_len = snprintf(nak_frame, sizeof(nak_frame),
                            "%c%s%c%02X%c",
                            COMM_FRAME_START, frame_content,
                            COMM_FIELD_SEPARATOR, crc, COMM_FRAME_END);
                            
    if (total_len < 0 || total_len >= sizeof(nak_frame)) {
        return false;
    }
    
    COMM_DEBUG_INSTANCE(instance, "发送NAK: %s (原因: %s)", nak_frame, reason);
    

    return comm_send_raw(instance, nak_frame, total_len);
}
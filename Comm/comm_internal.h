/**
  ******************************************************************************
  * @file           : comm_internal.h
  * @author         : ShanQue
  * @brief          : STM32串口通信协议库 - 内部定义和配置
  * @date           : 2025/07/31
  * @version        : 2.0.0
  ******************************************************************************
  * 
  * 内部使用的数据结构、状态机、错误码等定义
  * 
  ******************************************************************************
  */

#ifndef COMM_INTERNAL_H
#define COMM_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* =============================================================================
 * 基本配置参数
 * =============================================================================
 */

/** @brief 最大支持的UART实例数量 */
#define COMM_MAX_INSTANCES          8

/** @brief 接收缓冲区大小（字节） */
#define COMM_RX_BUFFER_SIZE         256

/** @brief 发送缓冲区大小（字节） */
#define COMM_TX_BUFFER_SIZE         128

/** @brief 每个UART最大回调函数数量 */
#define COMM_MAX_CALLBACKS          8

/* =============================================================================
 * 协议格式配置
 * =============================================================================
 */

/** @brief 帧起始符 */
#define COMM_FRAME_START            '{'

/** @brief 帧结束符 */
#define COMM_FRAME_END              '}'

/** @brief 命令和数据分隔符 */
#define COMM_CMD_DATA_SEPARATOR     ':'

/** @brief 字段分隔符（数据、序列号、CRC之间） */
#define COMM_FIELD_SEPARATOR        '#'

/** @brief 最大命令长度 */
#define COMM_MAX_CMD_LENGTH         16

/** @brief 最大数据长度 */
#define COMM_MAX_DATA_LENGTH        64

/* =============================================================================
 * 通信参数配置
 * =============================================================================
 */

/** @brief 默认超时基准时间（毫秒） */
#define COMM_DEFAULT_TIMEOUT_BASE   200

/** @brief 每字节额外超时时间（毫秒） */
#define COMM_TIMEOUT_PER_BYTE       1

/** @brief 默认最大重试次数 */
#define COMM_DEFAULT_MAX_RETRY      3

/** @brief 重试间隔时间（毫秒） */
#define COMM_RETRY_INTERVAL         10

/** @brief 帧接收超时时间（毫秒） */
#define COMM_FRAME_TIMEOUT_MS       100

/** @brief 序列号最大值（8位） */
#define COMM_MAX_SEQUENCE           255

/* =============================================================================
 * 特殊命令定义
 * =============================================================================
 */

/** @brief ACK确认命令 */
#define COMM_CMD_ACK                "ACK"

/** @brief NAK否认命令 */
#define COMM_CMD_NAK                "NAK"

/** @brief PING心跳命令 */
#define COMM_CMD_PING               "PING"

/** @brief PONG心跳响应命令 */
#define COMM_CMD_PONG               "PONG"

/* =============================================================================
 * 调试和性能配置
 * =============================================================================
 */

/** @brief 启用全局调试输出 */
#define COMM_ENABLE_DEBUG           0

/** @brief 启用统计功能 */
#define COMM_ENABLE_STATS           0

/** @brief 启用快速CRC计算 */
#define COMM_ENABLE_FAST_CRC        1

/** @brief 启用错误回调 */
#define COMM_ENABLE_ERROR_CALLBACK  1

/** @brief 启用帧错误统计 */
#define COMM_ENABLE_ERROR_STATS     1

/* 回调函数类型定义 */
typedef void (*comm_callback_t)(const char *cmd, const char *data);
typedef void (*comm_fail_callback_t)(const char* cmd, const char* data, const char* reason);
typedef void (*comm_state_change_callback_t)(UART_HandleTypeDef *huart, 
                                            const char* from_state, 
                                            const char* to_state, 
                                            uint8_t retry_count);

/* =============================================================================
 * 内部错误码定义
 * =============================================================================
 */

typedef enum {
    COMM_ERR_NONE = 0,              /**< 无错误 */
    COMM_ERR_INVALID_PARAM,         /**< 无效参数 */
    COMM_ERR_NOT_INITIALIZED,       /**< 未初始化 */
    COMM_ERR_BUFFER_FULL,           /**< 缓冲区满 */
    COMM_ERR_TIMEOUT,               /**< 超时 */
    COMM_ERR_CRC_MISMATCH,          /**< CRC校验失败 */
    COMM_ERR_FRAME_FORMAT,          /**< 帧格式错误 */
    COMM_ERR_SEQUENCE_ERROR,        /**< 序列号错误 */
    COMM_ERR_UART_ERROR,            /**< UART硬件错误 */
    COMM_ERR_NO_MEMORY,             /**< 内存不足 */
    COMM_ERR_CALLBACK_NOT_FOUND,    /**< 回调函数未找到 */
    COMM_ERR_MAX                    /**< 错误码上限 */
} comm_error_t;


/* =============================================================================
 * 状态机定义
 * =============================================================================
 */

typedef enum {
    COMM_STATE_IDLE = 0,            /**< 空闲状态 */
    COMM_STATE_SENDING,             /**< 发送中 */
    COMM_STATE_WAIT_ACK,            /**< 等待ACK确认 */
    COMM_STATE_RETRY,               /**< 重试中 */
    COMM_STATE_RECEIVING,           /**< 接收中 */
    COMM_STATE_PROCESSING,          /**< 处理接收数据中 */
    COMM_STATE_ERROR                /**< 错误状态 */
} comm_state_t;

/* =============================================================================
 * 帧解析状态
 * =============================================================================
 */

typedef enum {
    FRAME_STATE_IDLE = 0,           /**< 等待帧开始 '{' */
    FRAME_STATE_CMD,                /**< 解析命令部分 */
    FRAME_STATE_WAIT_COLON,         /**< 等待冒号 ':' */
    FRAME_STATE_DATA,               /**< 解析数据部分 */
    FRAME_STATE_WAIT_HASH1,         /**< 等待第一个井号 '#' */
    FRAME_STATE_SEQ,                /**< 解析序列号 */
    FRAME_STATE_WAIT_HASH2,         /**< 等待第二个井号 '#' */
    FRAME_STATE_CRC,                /**< 解析CRC */
    FRAME_STATE_WAIT_END,           /**< 等待帧结束 '}' */
    FRAME_STATE_COMPLETE,           /**< 帧解析完成 */
    FRAME_STATE_ERROR               /**< 解析错误状态 */
} frame_parse_state_t;

/* =============================================================================
 * 回调函数管理
 * =============================================================================
 */

typedef struct {
    char cmd[COMM_MAX_CMD_LENGTH + 1];      /**< 命令字符串 */
    comm_callback_t callback;               /**< 回调函数指针 */
    bool is_used;                           /**< 是否已使用 */
} comm_handler_t;

/* =============================================================================
 * 统计信息结构
 * =============================================================================
 */

#if COMM_ENABLE_STATS
struct comm_stats_t {
    /* 发送统计 */
    uint32_t tx_count;                      /**< 发送总次数 */
    uint32_t tx_success;                    /**< 发送成功次数 */
    uint32_t tx_failed;                     /**< 发送失败次数 */
    uint32_t tx_retry;                      /**< 重传次数 */
    uint32_t tx_timeout;                    /**< 发送超时次数 */
    
    /* 接收统计 */
    uint32_t rx_count;                      /**< 接收总次数 */
    uint32_t rx_success;                    /**< 接收成功次数 */
    uint32_t rx_error;                      /**< 接收错误次数 */
    uint32_t rx_crc_error;                  /**< CRC错误次数 */
    uint32_t rx_frame_error;                /**< 帧格式错误次数 */
    uint32_t rx_seq_error;                  /**< 序列号错误次数 */
    
    /* 性能统计 */
    uint32_t avg_delay_ms;                  /**< 平均响应延迟(ms) */
    uint32_t max_delay_ms;                  /**< 最大响应延迟(ms) */
    uint32_t min_delay_ms;                  /**< 最小响应延迟(ms) */
    
    /* 其他统计 */
    uint32_t ping_count;                    /**< PING次数 */
    uint32_t ping_success;                  /**< PING成功次数 */
};
typedef struct comm_stats_t comm_stats_t;
#else
/* 如果禁用统计功能，提供空的结构定义 */
struct comm_stats_t {
    uint8_t dummy;
};
typedef struct comm_stats_t comm_stats_t;
#endif

/* =============================================================================
 * 帧数据结构
 * =============================================================================
 */

typedef struct {
    char cmd[COMM_MAX_CMD_LENGTH + 1];      /**< 命令字符串 */
    char data[COMM_MAX_DATA_LENGTH + 1];    /**< 数据字符串 */
    uint8_t sequence;                       /**< 序列号 */
    uint8_t crc;                            /**< CRC校验值 */
    bool is_valid;                          /**< 帧是否有效 */
} comm_frame_t;

/* =============================================================================
 * 环形缓冲区（可选）
 * =============================================================================
 */

#if COMM_ENABLE_RING_BUFFER
typedef struct {
    uint8_t *buffer;                        /**< 缓冲区指针 */
    uint16_t size;                          /**< 缓冲区大小 */
    uint16_t head;                          /**< 写指针 */
    uint16_t tail;                          /**< 读指针 */
    bool is_full;                           /**< 缓冲区是否满 */
} comm_ring_buffer_t;
#endif

/* =============================================================================
 * UART实例管理结构
 * =============================================================================
 */

typedef struct comm_instance {
    /* UART句柄和基本信息 */
    UART_HandleTypeDef *huart;              /**< UART句柄指针 */
    
    /* 状态管理 */
    comm_state_t state;                     /**< 当前状态 */
    frame_parse_state_t parse_state;        /**< 帧解析状态 */
    
    /* 序列号管理 */
    uint8_t tx_sequence;                    /**< 发送序列号 */
    uint8_t rx_sequence;                    /**< 接收序列号 */
    uint8_t expected_ack_seq;               /**< 期望的ACK序列号 */
    
    /* 缓冲区管理 */
    char rx_buffer[COMM_RX_BUFFER_SIZE];    /**< 接收缓冲区 */
    char tx_buffer[COMM_TX_BUFFER_SIZE];    /**< 发送缓冲区 */
    uint16_t rx_index;                      /**< 接收缓冲区索引 */
    uint16_t tx_length;                     /**< 发送数据长度 */
    
    /* 中断接收相关 */
    uint8_t rx_byte;                        /**< 单字节接收缓冲 */
    volatile bool new_frame_available;      /**< 新帧可用标志 */
    comm_frame_t pending_frame;             /**< 待处理的完整帧 */
    uint32_t frame_timeout;                 /**< 帧接收超时时间 */
    
#if COMM_ENABLE_RING_BUFFER
    comm_ring_buffer_t ring_buffer;         /**< 环形缓冲区 */
#endif
    
    /* 回调函数管理 */
    comm_handler_t handlers[COMM_MAX_CALLBACKS];  /**< 回调函数数组 */
    uint8_t handler_count;                  /**< 已注册回调数量 */
    
    /* 超时和重试管理 */
    uint32_t timeout_ms;                    /**< 超时时间 */
    uint8_t max_retry;                      /**< 最大重试次数 */
    uint8_t retry_count;                    /**< 当前重试次数 */
    uint32_t last_send_time;                /**< 上次发送时间 */
    
    /* 当前发送任务信息 - 用于失败回调 */
    char current_cmd[COMM_MAX_CMD_LENGTH];  /**< 当前发送的命令 */
    char current_data[COMM_MAX_DATA_LENGTH]; /**< 当前发送的数据 */
    uint8_t current_sequence;               /**< 当前发送任务的序列号 */
    
    /* 调试和统计 */
    bool debug_enabled;                     /**< 调试开关 */
    comm_stats_t stats;                     /**< 统计信息 */
    
    /* 回调函数 */
    comm_fail_callback_t fail_callback;    /**< 发送失败回调 */
    comm_state_change_callback_t state_change_callback; /**< 状态变化回调 */
    
#if COMM_ENABLE_ERROR_CALLBACK
    comm_callback_t error_callback;         /**< 错误处理回调 */
#endif
    
} comm_instance_t;

/* =============================================================================
 * 全局管理结构
 * =============================================================================
 */

typedef struct {
    comm_instance_t instances[COMM_MAX_INSTANCES];  /**< 实例数组 */
    uint8_t instance_count;                         /**< 已使用实例数量 */
} comm_manager_t;


/* 统计功能 */
#if COMM_ENABLE_STATS
void comm_update_tx_stats(comm_instance_t *instance, bool success);
void comm_update_rx_stats(comm_instance_t *instance, bool success, comm_error_t error);
#else
#define comm_update_tx_stats(instance, success) ((void)0)
#define comm_update_rx_stats(instance, success, error) ((void)0)
#endif

/* 调试输出 */
#if COMM_ENABLE_DEBUG
void comm_debug_print_frame(const comm_instance_t *instance, const char *prefix, const comm_frame_t *frame);
void comm_debug_print_raw(const comm_instance_t *instance, const char *prefix, const char *data, uint16_t length);
void comm_debug_print_state(const comm_instance_t *instance);
#else
#define comm_debug_print_frame(instance, prefix, frame) ((void)0)
#define comm_debug_print_raw(instance, prefix, data, length) ((void)0)
#define comm_debug_print_state(instance) ((void)0)
#endif

/* 环形缓冲区操作（可选） */
#if COMM_ENABLE_RING_BUFFER
bool comm_ring_buffer_init(comm_ring_buffer_t *rb, uint8_t *buffer, uint16_t size);
bool comm_ring_buffer_put(comm_ring_buffer_t *rb, uint8_t data);
bool comm_ring_buffer_get(comm_ring_buffer_t *rb, uint8_t *data);
uint16_t comm_ring_buffer_available(const comm_ring_buffer_t *rb);
void comm_ring_buffer_reset(comm_ring_buffer_t *rb);
#endif


/* =============================================================================
 * 调试宏定义
 * =============================================================================
 */

// 错误输出串口配置 - 独立于DEBUG开关，用于重要错误信息
// 可以通过修改这里的 huart1 来指定不同的串口用于错误输出
#include "uart.h"  // 包含USARTx_printf函数
extern UART_HandleTypeDef huart1;  // 外部声明错误输出UART

// 错误输出串口宏 - 可以修改为 huart2 或 huart3
#define COMM_ERROR_UART huart1

#define COMM_ERROR_PRINTF(fmt, ...) USARTx_printf(COMM_ERROR_UART, fmt, ##__VA_ARGS__)

// 错误输出宏 - 始终启用，不受DEBUG开关影响
#define COMM_ERROR_OUTPUT(fmt, ...) \
    COMM_ERROR_PRINTF("[COMM-ERROR] " fmt "\r\n", ##__VA_ARGS__)

#if COMM_ENABLE_DEBUG
    // 调试输出函数 - 使用相同的UART进行调试输出
    #define COMM_DEBUG_PRINTF(fmt, ...) USARTx_printf(COMM_ERROR_UART, fmt, ##__VA_ARGS__)
    
    #define COMM_DEBUG_INSTANCE(instance, fmt, ...) \
        do { \
            if ((instance) && (instance)->debug_enabled) { \
                COMM_DEBUG_PRINTF("[COMM-%p] " fmt "\r\n", (instance)->huart, ##__VA_ARGS__); \
            } \
        } while(0)
    
    #define COMM_DEBUG_ERROR(fmt, ...) \
        COMM_DEBUG_PRINTF("[COMM-ERROR] " fmt "\r\n", ##__VA_ARGS__)
    
    #define COMM_DEBUG_INFO(fmt, ...) \
        COMM_DEBUG_PRINTF("[COMM-INFO] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define COMM_DEBUG_INSTANCE(instance, fmt, ...) ((void)0)
    #define COMM_DEBUG_ERROR(fmt, ...) ((void)0)
    #define COMM_DEBUG_INFO(fmt, ...) ((void)0)
#endif

#endif /* COMM_INTERNAL_H */


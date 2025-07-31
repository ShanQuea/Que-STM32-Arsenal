/**
 * @file    comm_manager.c
 * @brief   通信库实例管理层 - UART实例管理、回调处理、超时重试
 * @author  ShanQue
 * @version 2.0
 * @date    2025-07-31
 */

#include "comm_manager.h"
#include "comm_protocol.h"
#include <string.h>

static comm_manager_t g_comm_manager = {0};

/* Private function prototypes -----------------------------------------------*/
static int comm_find_callback_index(comm_instance_t *instance, const char *cmd);
static comm_instance_t* comm_create_instance(UART_HandleTypeDef *huart);
static const char* comm_state_to_string(uint8_t state);

/* =============================================================================
 * 全局管理器函数实现
 * =============================================================================
 */

void comm_manager_init(void)
{
    memset(&g_comm_manager, 0, sizeof(comm_manager_t));
}

uint8_t comm_get_instance_count(void)
{
    return g_comm_manager.instance_count;
}

comm_instance_t* comm_get_instance_by_index(uint8_t index)
{
    if (index >= g_comm_manager.instance_count) {
        return NULL;
    }
    return &g_comm_manager.instances[index];
}

/* =============================================================================
 * 实例管理函数实现
 * =============================================================================
 */

bool comm_instance_init(comm_instance_t *instance, 
                       UART_HandleTypeDef *huart,
                       uint32_t timeout_ms, 
                       uint8_t max_retry)
{
    if (instance == NULL || huart == NULL) {
        return false;
    }
    
    // 清零实例结构
    memset(instance, 0, sizeof(comm_instance_t));
    
    // 设置基本参数
    instance->huart = huart;
    instance->timeout_ms = timeout_ms;
    instance->max_retry = max_retry;
    
    // 初始化状态
    instance->state = COMM_STATE_IDLE;
    instance->parse_state = FRAME_STATE_IDLE;
    
    // 重置序列号
    instance->tx_sequence = 0;
    instance->rx_sequence = 0;
    instance->expected_ack_seq = 0;
    instance->current_sequence = 0;
    
    // 清除所有回调
    for (int i = 0; i < COMM_MAX_CALLBACKS; i++) {
        memset(instance->handlers[i].cmd, 0, COMM_MAX_CMD_LENGTH);
        instance->handlers[i].callback = NULL;
    }
    instance->fail_callback = NULL;
    instance->state_change_callback = NULL;
    
    #if COMM_ENABLE_STATS
    // 初始化统计信息
    memset(&instance->stats, 0, sizeof(instance->stats));
    instance->stats.min_delay_ms = UINT32_MAX;
    #endif
    
    COMM_DEBUG_INSTANCE(instance, "通信实例初始化成功");
    return true;
}

bool comm_instance_is_ready(comm_instance_t *instance)
{
    if (instance == NULL) {
        return false;
    }
    
    return (instance->state == COMM_STATE_IDLE);
}

uint8_t comm_instance_get_state(comm_instance_t *instance)
{
    if (instance == NULL) {
        return COMM_STATE_IDLE;
    }
    
    return instance->state;
}

void comm_instance_reset(comm_instance_t *instance)
{
    if (instance == NULL) {
        return;
    }
    
    // 保存必要的配置
    UART_HandleTypeDef *huart = instance->huart;
    uint32_t timeout_ms = instance->timeout_ms;
    uint8_t max_retry = instance->max_retry;
    
    #if COMM_ENABLE_STATS
    comm_stats_t stats = instance->stats;
    #endif
    
    // 重新初始化
    comm_instance_init(instance, huart, timeout_ms, max_retry);
    
    #if COMM_ENABLE_STATS
    // 恢复统计信息
    instance->stats = stats;
    #endif
    
    COMM_DEBUG_INSTANCE(instance, "通信实例已重置");
}

/* =============================================================================
 * 回调管理函数实现
 * =============================================================================
 */

bool comm_register_callback(comm_instance_t *instance, const char *cmd, comm_callback_t callback)
{
    if (instance == NULL || cmd == NULL || callback == NULL) {
        return false;
    }
    
    // 检查命令长度
    if (strlen(cmd) >= COMM_MAX_CMD_LENGTH) {
        COMM_DEBUG_INSTANCE(instance, "命令过长，无法注册: %s", cmd);
        return false;
    }
    
    // 检查是否已存在该命令的回调
    int existing_index = comm_find_callback_index(instance, cmd);
    if (existing_index >= 0) {
        // 更新现有回调
        instance->handlers[existing_index].callback = callback;
        COMM_DEBUG_INSTANCE(instance, "更新回调: %s", cmd);
        return true;
    }
    
    // 寻找空闲位置
    for (int i = 0; i < COMM_MAX_CALLBACKS; i++) {
        if (instance->handlers[i].callback == NULL) {
            strncpy(instance->handlers[i].cmd, cmd, COMM_MAX_CMD_LENGTH - 1);
            instance->handlers[i].cmd[COMM_MAX_CMD_LENGTH - 1] = '\0';
            instance->handlers[i].callback = callback;
            
            COMM_DEBUG_INSTANCE(instance, "注册回调成功: %s (位置 %d)", cmd, i);
            return true;
        }
    }
    
    COMM_DEBUG_INSTANCE(instance, "回调表已满，无法注册: %s", cmd);
    return false;
}

bool comm_call_callback(comm_instance_t *instance, const char *cmd, const char *data)
{
    if (instance == NULL || cmd == NULL || data == NULL) {
        return false;
    }
    
    int index = comm_find_callback_index(instance, cmd);
    if (index >= 0 && instance->handlers[index].callback != NULL) {
        // 执行用户回调
        instance->handlers[index].callback(cmd, data);
        
        COMM_DEBUG_INSTANCE(instance, "执行回调: %s -> %s", cmd, data);
        return true;
    }
    
    COMM_DEBUG_INSTANCE(instance, "未找到命令回调: %s", cmd);
    return false;
}

void comm_set_fail_callback(comm_instance_t *instance, comm_fail_callback_t callback)
{
    if (instance == NULL) {
        return;
    }
    
    instance->fail_callback = callback;
    
    if (callback != NULL) {
        COMM_DEBUG_INSTANCE(instance, "设置失败回调函数");
    } else {
        COMM_DEBUG_INSTANCE(instance, "清除失败回调函数");
    }
}

void comm_call_fail_callback(comm_instance_t *instance, const char *cmd, const char *data, const char *reason)
{
    if (instance == NULL || cmd == NULL || data == NULL || reason == NULL) {
        return;
    }
    
    if (instance->fail_callback != NULL) {
        instance->fail_callback(cmd, data, reason);
        COMM_DEBUG_INSTANCE(instance, "调用失败回调: %s:%s - %s", cmd, data, reason);
    }
}

/* =============================================================================
 * 超时和重试管理函数实现
 * =============================================================================
 */

bool comm_is_timeout(comm_instance_t *instance)
{
    if (instance == NULL || instance->state != COMM_STATE_WAIT_ACK) {
        return false;
    }
    
    uint32_t current_time = HAL_GetTick();
    uint32_t elapsed = current_time - instance->last_send_time;
    
    return (elapsed >= instance->timeout_ms);
}

void comm_handle_timeout(comm_instance_t *instance)
{
    if (instance == NULL) {
        return;
    }
    
    COMM_DEBUG_INSTANCE(instance, "超时发生，重试次数: %d", instance->retry_count);
    
    #if COMM_ENABLE_STATS
    instance->stats.tx_timeout++;
    #endif
    
    if (instance->retry_count < instance->max_retry) {
        // 开始重试
        instance->retry_count++;
        
        COMM_DEBUG_INSTANCE(instance, "开始第%d次重试", instance->retry_count);
        
        #if COMM_ENABLE_STATS
        instance->stats.tx_retry++;
        #endif
        
        // 重新发送
        HAL_StatusTypeDef status = HAL_UART_Transmit_IT(instance->huart, 
                                                       (uint8_t*)instance->tx_buffer, 
                                                       instance->tx_length);
        
        if (status == HAL_OK) {
            // 直接转到等待ACK状态，不经过SENDING状态
            comm_set_state(instance, COMM_STATE_WAIT_ACK);
            instance->last_send_time = HAL_GetTick();
            
            COMM_DEBUG_INSTANCE(instance, "重试发送启动成功");
        } else {
            // 非阻塞发送失败，回退到阻塞方式
            if (comm_send_raw(instance, instance->tx_buffer, instance->tx_length)) {
                comm_set_state(instance, COMM_STATE_WAIT_ACK);
                COMM_DEBUG_INSTANCE(instance, "重试使用阻塞发送成功");
            } else {
                COMM_DEBUG_INSTANCE(instance, "重试发送失败");
            }
        }
    } else {
        COMM_DEBUG_INSTANCE(instance, "达到最大重试次数，放弃");
        COMM_ERROR_OUTPUT("通信失败: UART %p, 命令 %s:%s, 重试 %d 次后放弃", 
                         instance->huart, instance->current_cmd, 
                         instance->current_data, instance->max_retry);
        
        // 触发失败回调
        comm_call_fail_callback(instance, instance->current_cmd, 
                               instance->current_data, "超时重试失败");

        comm_set_state(instance, COMM_STATE_IDLE);
        instance->retry_count = 0;
        
        #if COMM_ENABLE_STATS
        instance->stats.tx_failed++;
        #endif
    }
}

bool comm_is_frame_timeout(comm_instance_t *instance)
{
    if (instance == NULL || instance->parse_state == FRAME_STATE_IDLE) {
        return false;
    }
    
    uint32_t current_time = HAL_GetTick();
    return (current_time >= instance->frame_timeout);
}

void comm_handle_frame_timeout(comm_instance_t *instance)
{
    if (instance == NULL) {
        return;
    }
    
    COMM_DEBUG_INSTANCE(instance, "帧接收超时，重置解析状态");
    
    // 重置帧解析状态
    instance->parse_state = FRAME_STATE_IDLE;
    instance->rx_index = 0;
    instance->new_frame_available = false;
    
    #if COMM_ENABLE_STATS
    instance->stats.rx_error++;
    #endif
}

void comm_set_state(comm_instance_t *instance, uint8_t new_state)
{
    if (instance == NULL) {
        return;
    }
    
    if (instance->state != new_state) {
        // 保存旧状态用于回调
        const char* old_state_str = comm_state_to_string(instance->state);
        const char* new_state_str = comm_state_to_string(new_state);
        
        COMM_DEBUG_INSTANCE(instance, "状态变更: %s -> %s", old_state_str, new_state_str);
        
        // 更新状态
        instance->state = new_state;
        
        // 调用状态变化回调（如果已注册）
        if (instance->state_change_callback != NULL) {
            instance->state_change_callback(instance->huart, 
                                           old_state_str, 
                                           new_state_str, 
                                           instance->retry_count);
        }
    }
}

bool comm_send_raw(comm_instance_t *instance, const char *data, uint16_t length)
{
    if (instance == NULL || data == NULL || length == 0) {
        return false;
    }
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(instance->huart, 
                                                (uint8_t*)data, 
                                                length, 
                                                1000);  // 1秒超时
    
    if (status == HAL_OK) {
        instance->last_send_time = HAL_GetTick();
        return true;
    } else {
        COMM_DEBUG_INSTANCE(instance, "发送失败，状态码: %d", status);
        return false;
    }
}

/* =============================================================================
 * 全局实例查找函数实现
 * =============================================================================
 */

comm_instance_t* comm_find_instance(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return NULL;
    }
    
    for (uint8_t i = 0; i < g_comm_manager.instance_count; i++) {
        if (g_comm_manager.instances[i].huart == huart) {
            return &g_comm_manager.instances[i];
        }
    }
    
    return NULL;
}

comm_instance_t* comm_create_uart_instance(UART_HandleTypeDef *huart, 
                                          uint32_t timeout_ms, 
                                          uint8_t max_retry)
{
    if (huart == NULL) {
        return NULL;
    }
    
    // 查找或创建实例
    comm_instance_t *instance = comm_find_instance(huart);
    if (instance == NULL) {
        instance = comm_create_instance(huart);
        if (instance == NULL) {
            return NULL;
        }
    }
    
    // 初始化实例
    if (!comm_instance_init(instance, huart, timeout_ms, max_retry)) {
        COMM_DEBUG_ERROR("实例初始化失败");
        COMM_ERROR_OUTPUT("UART实例初始化失败: %p", huart);
        return NULL;
    }
    
    return instance;
}


static int comm_find_callback_index(comm_instance_t *instance, const char *cmd)
{
    if (instance == NULL || cmd == NULL) {
        return -1;
    }
    
    for (int i = 0; i < COMM_MAX_CALLBACKS; i++) {
        if (instance->handlers[i].callback != NULL &&
            strcmp(instance->handlers[i].cmd, cmd) == 0) {
            return i;
        }
    }
    
    return -1;
}

static comm_instance_t* comm_create_instance(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
        return NULL;
    }
    
    // 检查是否已存在
    comm_instance_t *existing = comm_find_instance(huart);
    if (existing != NULL) {
        return existing;
    }
    
    // 检查是否有空闲位置
    if (g_comm_manager.instance_count >= COMM_MAX_INSTANCES) {
        COMM_DEBUG_ERROR("达到最大实例数: %d", COMM_MAX_INSTANCES);
        COMM_ERROR_OUTPUT("无法创建更多UART实例，已达上限: %d", COMM_MAX_INSTANCES);
        return NULL;
    }
    
    // 创建新实例
    comm_instance_t *instance = &g_comm_manager.instances[g_comm_manager.instance_count];
    g_comm_manager.instance_count++;
    
    COMM_DEBUG_INFO("为UART %p创建实例，实例总数：%d", 
                    huart, g_comm_manager.instance_count);
    
    return instance;
}

static const char* comm_state_to_string(uint8_t state)
{
    switch (state) {
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
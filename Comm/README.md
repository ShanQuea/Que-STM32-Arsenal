# STM32 COMM库使用指南
### 步骤1: 添加库文件到项目
将以下文件添加到项目中：
```
Lib/Comm/
├── comm.h
├── comm.c
├── comm_protocol.h
├── comm_protocol.c
├── comm_manager.h
├── comm_manager.c
└── comm_internal.h
```

### 步骤2: 在main.c中添加必要的HAL回调函数
```c
/* USER CODE BEGIN 4 */

// HAL UART中断回调函数 - 必须添加这些回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    // 统一处理所有UART接收回调
    comm_uart_rx_callback(huart);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    // 统一处理所有UART发送回调
    comm_uart_tx_callback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    // 统一处理所有UART错误回调
    comm_uart_error_callback(huart);
}

/* USER CODE END 4 */
```

### 步骤3: 在定时器中断中添加通信处理
在 `stm32f4xx_it.c` 中：
```c
/* USER CODE BEGIN 1 */

// 定时器周期回调函数 - 必须添加用于通信处理
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim6) {  // 使用1ms定时器
        // 每1ms处理通信超时和重试
        comm_tick();
    }
}

/* USER CODE END 1 */
```

### 步骤4: 在main.c中初始化和使用
```c
#include "comm.h"

// 命令回调函数
void walk_callback(const char* cmd, const char* data) {
    printf("执行移动: %s\n", data);
}

void ping_callback(const char* cmd, const char* data) {
    printf("收到PING: %s\n", data);
}

// 失败回调
void comm_fail_callback(const char* cmd, const char* data, const char* reason) {
    printf("通信失败: %s:%s - %s\n", cmd, data, reason);
}

// 状态变化回调 - 自动监控通信状态变化
void comm_state_change_callback(UART_HandleTypeDef *huart, 
                               const char* from_state, 
                               const char* to_state, 
                               uint8_t retry_count) {
    // 只在非正常状态时显示警告
    if (strcmp(to_state, "IDLE") != 0) {
        printf("UART状态变化: %s -> %s", from_state, to_state);
        if (retry_count > 0) {
            printf(" (重试:%d/3)", retry_count);
        }
        printf("\n");
    }
}

int main(void) {
    // 硬件初始化...
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_TIM6_Init();
    
    // 1. 初始化通信库
    comm_init();
    
    // 2. 添加UART（自动处理接收中断）
    comm_add_uart(&huart2);
    comm_add_uart(&huart3);
    
    // 3. 注册命令回调
    comm_register_command_callback(&huart2, "WALK", walk_callback);
    comm_register_command_callback(&huart2, "PING", ping_callback);
    comm_register_fail_callback(&huart2, comm_fail_callback);
    
    // 4. 注册状态变化回调 - 自动监控通信状态（推荐）
    comm_register_state_change_callback(&huart2, comm_state_change_callback);
    comm_register_state_change_callback(&huart3, comm_state_change_callback);
    
    // 5. 启动定时器（必须！用于处理超时重试）
    HAL_TIM_Base_Start_IT(&htim6);
    
    while (1) {
        // 6. 发送命令
        if (comm_is_ready(&huart2)) {
            comm_send_command(&huart2, "WALK", "D1 V100 T5000");
        }
        
        HAL_Delay(1000);
    }
}
```

## 必需的硬件配置

### 定时器配置
需要配置一个1ms定时器用于通信处理：
```c
// 在CubeMX中配置TIM6：
// Prescaler: 83 (对于84MHz时钟)
// Counter Period: 999
// 生成1ms中断
```

### UART配置
```c
// 典型UART配置：
// Baud Rate: 115200
// Word Length: 8 Bits
// Stop Bits: 1
// Parity: None
// Hardware Flow Control: None
```

## 协议格式

**帧格式**: `{CMD:DATA#SEQ#CRC}`

- `{` - 帧开始符
- `CMD` - 命令（最长16字符）
- `:` - 命令数据分隔符
- `DATA` - 数据（最长64字符）
- `#` - 字段分隔符
- `SEQ` - 序列号（2位十六进制）
- `#` - 字段分隔符
- `CRC` - CRC8校验（2位十六进制）
- `}` - 帧结束符

**示例**:
- `{WALK:D1 V100 T5000#01#1F}`
- `{PING:TEST#04#5A}`
- `{ACK:01#00#85}`

## 主要API

| 函数 | 功能 | 返回值 |
|------|------|--------|
| `comm_init()` | 初始化通信库 | void |
| `comm_add_uart(huart)` | 添加UART实例 | bool |
| `comm_send_command(huart, cmd, data)` | 发送命令 | bool |
| `comm_register_command_callback(huart, cmd, callback)` | 注册命令回调 | bool |
| `comm_register_fail_callback(huart, callback)` | 注册失败回调 | bool |
| `comm_register_state_change_callback(huart, callback)` | 注册状态变化回调 | bool |
| `comm_is_ready(huart)` | 检查是否就绪 | bool |
| `comm_get_state_string(huart)` | 获取详细状态信息 | const char* |
| `comm_get_retry_count(huart)` | 获取当前重试次数 | uint8_t |
| `comm_ping(huart)` | 发送PING测试 | bool |
| `comm_tick()` | 定时处理（在定时器中断中调用） | void |

## 错误输出配置

COMM库会自动输出重要的错误信息（如重试失败、实例创建失败等），这些错误输出**独立于DEBUG开关**，始终启用。

### 配置错误输出串口

默认使用UART1输出错误信息，可以通过修改 `comm_internal.h` 来指定不同的串口：

```c
// 在 comm_internal.h 第398行修改错误输出串口
#define COMM_ERROR_UART huart1  // 改为 huart2 或 huart3

// 也需要声明对应的外部UART句柄
extern UART_HandleTypeDef huart2;  // 如果使用huart2
```

### 错误输出示例

当通信出现问题时，库会自动输出：
```
[COMM-ERROR] 通信失败: UART 0x40004400, 命令 WALK:D1 V100 T5000, 重试 3 次后放弃
[COMM-ERROR] UART操作失败: 未找到UART实例 0x40004800
[COMM-ERROR] 无法创建更多UART实例，已达上限: 8
```

这些错误信息**无需开启DEBUG**，会自动输出到指定的错误输出串口。

## 状态监控

库内置了两种状态监控方式：

### 方式1: 自动状态变化回调（推荐）

注册状态变化回调后，库会在状态发生变化时自动通知应用层，无需轮询：

```c
// 注册状态变化回调
comm_register_state_change_callback(&huart2, comm_state_change_callback);

// 回调函数实现
void comm_state_change_callback(UART_HandleTypeDef *huart, 
                               const char* from_state, 
                               const char* to_state, 
                               uint8_t retry_count) {
    printf("通信状态变化: %s -> %s\n", from_state, to_state);
    if (retry_count > 0) {
        printf("重试次数: %d/3\n", retry_count);
    }
}
```

### 方式2: 手动状态查询（不推荐）

```c
// 手动检查通信状态（仅在必要时使用）
if (!comm_is_ready(&huart2)) {
    printf("UART2状态: %s (重试:%d/3)\n", 
           comm_get_state_string(&huart2), 
           comm_get_retry_count(&huart2));
}
```

**状态类型**：
- `IDLE` - 空闲状态，可以发送新命令
- `SENDING` - 正在发送数据
- `WAIT_ACK` - 等待ACK确认
- `RETRY` - 重试中
- `RECEIVING` - 接收数据中
- `PROCESSING` - 处理接收的数据
- `ERROR` - 错误状态

## 集成检查清单

在使用库之前，请确保完成以下步骤：

- [ ] 添加所有库文件到项目
- [ ] 在main.c中添加HAL UART回调函数
- [ ] 在定时器中断中添加comm_tick()调用
- [ ] 配置1ms定时器并启动中断
- [ ] 调用comm_init()初始化
- [ ] 使用comm_add_uart()添加UART
- [ ] 注册必要的命令回调函数
- [ ] 启动定时器中断

## 常见问题

### Q: 发送失败或超时
A: 检查定时器中断是否正常工作，确保comm_tick()被调用

### Q: 收不到数据
A: 检查HAL_UART_RxCpltCallback是否正确调用comm_uart_rx_callback()

### Q: CRC校验失败
A: 检查UART配置，确保波特率、数据位、停止位配置一致

### Q: 编译错误
A: 确保包含了uart.h（用于调试输出）和所有库文件


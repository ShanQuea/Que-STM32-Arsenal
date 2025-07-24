# 📡 Uart - 串口通信库

STM32串口发送通信库，专注于发送功能，支持格式化输出和多种数据类型发送。

## 🚀 快速使用

### 1. 基础发送功能

```c
#include "uart.h"

// 格式化输出
USARTx_printf(huart1, "温度: %.2f°C\n", 25.67);

// 16进制数据发送
uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
USARTx_SendHexDatas(huart1, data, sizeof(data));
```

### 2. 带前缀发送

```c
// 发送带前缀的数据
UART_SendIntWithPrefix(huart1, "温度", 25);        // 输出: 温度: 25
UART_SendFloatWithPrefix(huart1, "电压", 3.3, 2);  // 输出: 电压: 3.30
UART_SendHexWithPrefix(huart1, "状态", 0x5A);      // 输出: 状态: 0x5A
```

### 3. 格式化16进制显示

```c
uint8_t buffer[32] = {0x00, 0x01, 0x02, ...};

// 每16字节换行显示
UART_SendHexFormatted(huart1, buffer, sizeof(buffer), 16);

// 每8字节换行显示
UART_SendHexFormatted(huart1, buffer, sizeof(buffer), 8);

// 不换行显示
UART_SendHexFormatted(huart1, buffer, sizeof(buffer), 0);
```

## ⚙️ 配置参数

```c
#define APP_TX_DATA_SIZE    512     // 发送字符串最大长度
```

## 💡 使用说明

- **函数返回值**: 所有函数返回`HAL_StatusTypeDef`状态，便于错误处理


---

*如有问题欢迎提Issue，一起完善这个小库~ 🎉*
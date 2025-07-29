# 🔀 TCA9548A - I2C多路复用器库

TCA9548A 8通道I2C多路复用器驱动库，支持多个相同地址I2C设备连接。

## 🚀 快速使用

### 1. 初始化复用器

```c
#include "TCA9548A.h"

tca9548a_handle_t tca9548a;

// 初始化复用器
TCA9548A_Init(&tca9548a, &hi2c1, TCA9548A_I2C_ADDR_DEFAULT);
```

### 2. 通道选择和通信

```c
// 选择通道0
TCA9548A_SelectChannel(&tca9548a, TCA9548A_CH0);

// 在通道0上与I2C设备通信
// HAL_I2C_Mem_Read(&hi2c1, device_addr, ...);

// 选择通道3
TCA9548A_SelectChannel(&tca9548a, TCA9548A_CH3);

// 关闭所有通道
TCA9548A_DisableAllChannels(&tca9548a);
```

### 3. 设备扫描

```c
uint8_t found_devices[10];
uint8_t device_count;

// 扫描指定通道上的设备
TCA9548A_ScanChannel(&tca9548a, TCA9548A_CH0, found_devices, 10, &device_count);
```

## ⚙️ 主要功能

- **8通道切换**: 支持0-7共8个独立I2C通道
- **多通道同开**: 支持同时开启多个通道
- **设备扫描**: 扫描各通道上的I2C设备

## 💡 使用说明

- **I2C地址**: 0x70-0x77（默认0x70）
- **通道掩码**: 位0-7对应通道0-7
- **超时设置**: 默认100ms I2C操作超时
- **硬件配置**: 在STM32CubeMX中配置I2C外设

---

*如有问题欢迎提Issue，一起完善这个小库~ 🎉*
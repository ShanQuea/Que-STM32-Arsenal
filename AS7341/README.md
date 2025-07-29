# 🌈 AS7341 - 11通道光谱传感器库

AS7341 11通道光谱传感器驱动库，支持F1-F8、Clear、NIR通道检测。

## 🚀 快速使用

### 1. 初始化传感器

```c
#include "AS7341.h"

as7341_handle_t as7341;

// 初始化传感器
AS7341_Init(&as7341, &hi2c1, AS7341_I2CADDR_DEFAULT, 0);
```

### 2. 配置参数

```c
// 设置增益和积分时间
AS7341_SetGain(&as7341, AS7341_GAIN_64X);
AS7341_SetATIME(&as7341, 100);
AS7341_SetASTEP(&as7341, 999);
```

### 3. 读取光谱数据

```c
// 阻塞式读取所有通道数据
if(AS7341_ReadAllChannels_Blocking(&as7341)) {
    // 获取特定波长数据
    uint16_t f1_415nm = AS7341_GetChannel(&as7341, AS7341_CHANNEL_415nm_F1);
    uint16_t f5_555nm = AS7341_GetChannel(&as7341, AS7341_CHANNEL_555nm_F5);
    uint16_t clear = AS7341_GetChannel(&as7341, AS7341_CHANNEL_CLEAR);
    uint16_t nir = AS7341_GetChannel(&as7341, AS7341_CHANNEL_NIR);
}
```

## ⚙️ 主要功能

- **11通道检测**: F1(415nm)-F8(680nm)、Clear、NIR
- **可调增益**: 0.5x-512x增益设置
- **LED控制**: 内置LED照明控制
- **阻塞/非阻塞读取**: 支持两种读取模式

## 💡 使用说明

- **I2C地址**: 默认0x39
- **电源控制**: 支持低功耗模式
- **数据格式**: 16位ADC数据
- **硬件配置**: 在STM32CubeMX中配置I2C外设

---

*如有问题欢迎提Issue，一起完善这个小库~ 🎉*
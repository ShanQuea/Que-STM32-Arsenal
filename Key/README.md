# 🔘 Key - 按键检测库

基于状态机实现的STM32按键检测库，支持短按和长按功能。

## 🚀 快速使用

### 1. 初始化和注册按键

```c
#include "key.h"

KeyTypeDef key1;
KeyTypeDef *keys[10];

// 回调函数
void key1_short_press(void) { printf("短按\n"); }
void key1_long_press(void) { printf("长按\n"); }

// 可选：全局回调（每次按键扫描后调）
void global_callback(void) { 
    // 比如更新LED状态、处理其他任务等
}

// 初始化并注册按键
Key_Init(global_callback);  // 如果不需要传入NULL
RegKey(&key1, GPIOA, GPIO_PIN_0, 0, key_low, 
       key1_short_press, key1_long_press, keys);
```

### 2. 添加定时器支持

在 `stm32f4xx_it.c` 的 `SysTick_Handler` 中添加：

```c
void SysTick_Handler(void) {
    HAL_IncTick();
    KeySysTickAddCount();  // 按键计时
}
```

### 3. 主循环扫描

```c
while(1) {
    Key_Loop(keys);  // 建议10ms调用一次
    HAL_Delay(10);
}
```

## ⚙️ 配置参数

```c
#define LONG_PRESS_TIME     800     // 长按时间阈值 (ms)
#define KEY_DEBOUNCE_TIME   30      // 按键消抖时间 (ms)  
#define KEY_RELEASE_DELAY   50      // 按键释放后延时 (ms)
```

## 💡 使用说明

- **有效电平**: `key_low`(按下为0) 或 `key_high`(按下为1)
- **回调函数**: 短按和长按回调可以为NULL
- **按键ID**: 从0开始，用于索引按键数组
- **硬件配置**: 在STM32CubeMX中配置GPIO为输入模式，建议添加上拉/下拉电阻

---

*如有问题欢迎提Issue，一起完善这个小库~ 🎉*
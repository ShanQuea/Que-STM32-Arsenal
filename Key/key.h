/**
  ******************************************************************************
  * @file           : key.h
  * @author         : ShanQue
  * @brief          : 按键检测库头文件，支持短按和长按
  * @date           : 2025/07/24
  ******************************************************************************
  */

#ifndef KEY_H
#define KEY_H


/* 头文件包含 */

#include "main.h"
#include "stdbool.h"

/* 枚举类型定义 */

typedef enum {
	key_low = 0,    // 低电平有效
	key_high,       // 高电平有效
}KeyActiveLevelTypeDef;

typedef enum {
	Release_,       // 释放状态
	ShortPress_,    // 短按状态  
	LongPress_,     // 长按状态
}KeyStateTypeDef;

typedef enum {
	KEY_IDLE,           // 空闲状态
	KEY_DEBOUNCE,       // 消抖状态
	KEY_PRESSED,        // 已按下状态
	KEY_LONG_TRIGGERED, // 长按已触发状态
}KeyInternalStateTypeDef;

/* 结构体定义 */

typedef struct {
	GPIO_TypeDef* Key_GpioPort;             // GPIO端口
	uint16_t Key_GpioPin;                   // GPIO引脚
	uint8_t Key_Number;                     // 按键编号
	KeyStateTypeDef Key_State;              // 按键状态（对外）
	KeyInternalStateTypeDef internal_state; // 内部状态（状态机用）
	KeyActiveLevelTypeDef level;            // 有效电平
	uint32_t press_time;                    // 按键按下时间计数器
	void (*ShortPressF)(void);              // 短按回调函数
	void (*LongPressF)(void);               // 长按回调函数
}KeyTypeDef;

/* 时间配置宏定义 */

#define LONG_PRESS_TIME     800     // 长按时间阈值 (ms)
#define KEY_DEBOUNCE_TIME   30      // 按键消抖时间 (ms)
#define KEY_RELEASE_DELAY   50      // 按键释放后延时 (ms)

/* 函数声明 */

void Key_Init(void (*callback)(void));
void KeySysTickAddCount(void);
void RegKey(KeyTypeDef *key, GPIO_TypeDef *GpioPort, uint16_t GpioPin, uint8_t key_id, 
           KeyActiveLevelTypeDef level, void (*ShortPressF)(void), void (*LongPressF)(void), KeyTypeDef **keys);
void Key_Loop(KeyTypeDef **keys);


#endif //KEY_H

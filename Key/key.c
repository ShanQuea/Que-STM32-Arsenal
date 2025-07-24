/**
  ******************************************************************************
  * @file           : key.c
  * @author         : ShanQue
  * @brief          : 按键检测库，支持短按和长按，基于状态机实现
  * @date           : 2025/07/24
  ******************************************************************************
  */

#include "key.h"


// 全局变量
volatile uint32_t systick_1ms_counter = 0;      // 1ms计数器
uint8_t registered_key_count = 0;               // 已注册的按键数量

// 用户回调函数指针
typedef void (*KeyLoopCallbackFunc)(void);
KeyLoopCallbackFunc user_callback = NULL;

/**
  * @brief  注册按键
  * @param  key          按键结构体指针
  * @param  GpioPort     GPIO端口
  * @param  GpioPin      GPIO引脚
  * @param  key_id       按键ID
  * @param  level        按键有效电平
  * @param  ShortPressF  短按回调函数
  * @param  LongPressF   长按回调函数
  * @param  keys         按键数组指针
  * @retval None
  */
void RegKey(KeyTypeDef *key, GPIO_TypeDef *GpioPort, uint16_t GpioPin, uint8_t key_id, 
           KeyActiveLevelTypeDef level, void (*ShortPressF)(void), void (*LongPressF)(void), KeyTypeDef **keys)
{
    key->Key_GpioPort = GpioPort;
    key->Key_GpioPin = GpioPin;
    key->Key_Number = key_id;
    key->level = level;
    key->LongPressF = LongPressF;
    key->ShortPressF = ShortPressF;
    key->Key_State = Release_;
    key->internal_state = KEY_IDLE;
    key->press_time = 0;
    keys[key_id] = key;
    
    // 动态更新按键数量
    if(key_id >= registered_key_count) {
        registered_key_count = key_id + 1;
    }
}

/**
  * @brief  按键库初始化
  * @param  callback  用户回调函数
  * @retval None
  */
void Key_Init(void (*callback)(void))
{
    user_callback = callback;
    systick_1ms_counter = 0;
}

/**
  * @brief  读取按键GPIO状态
  * @param  key  按键结构体指针
  * @retval 1-按键按下，0-按键释放
  */
static uint8_t Key_ReadPin(KeyTypeDef *key)
{
    return (HAL_GPIO_ReadPin(key->Key_GpioPort, key->Key_GpioPin) == key->level) ? 1 : 0;
}

/**
  * @brief  按键状态机处理函数
  * @param  key  按键结构体指针
  * @retval None
  */
static void Key_StateMachine(KeyTypeDef *key)
{
    uint8_t key_pressed = Key_ReadPin(key);
    
    switch(key->internal_state)
    {
        case KEY_IDLE:
            if(key_pressed) {
                key->internal_state = KEY_DEBOUNCE;
                key->press_time = 0;
            }
            break;
            
        case KEY_DEBOUNCE:
            if(key_pressed) {
                key->press_time++;
                if(key->press_time >= KEY_DEBOUNCE_TIME) {
                    // 消抖完成，确认按键按下
                    key->internal_state = KEY_PRESSED;
                    key->press_time = 0;
                }
            } else {
                // 消抖期间释放，误触发
                key->internal_state = KEY_IDLE;
                key->press_time = 0;
            }
            break;
            
        case KEY_PRESSED:
            if(key_pressed) {
                key->press_time++;
                if(key->press_time >= LONG_PRESS_TIME) {
                    // 达到长按时间，触发长按
                    if(key->LongPressF != NULL) {
                        key->LongPressF();
                    }
                    key->internal_state = KEY_LONG_TRIGGERED;
                }
            } else {
                // 按键释放，触发短按（仅当未触发长按时）
                if(key->ShortPressF != NULL) {
                    key->ShortPressF();
                }
                key->internal_state = KEY_IDLE;
                key->press_time = 0;
            }
            break;
            
        case KEY_LONG_TRIGGERED:
            if(!key_pressed) {
                // 长按后释放，返回空闲状态
                key->internal_state = KEY_IDLE;
                key->press_time = 0;
            }
            // 长按触发后，继续按住不做任何操作
            break;
            
        default:
            key->internal_state = KEY_IDLE;
            key->press_time = 0;
            break;
    }
}

/**
  * @brief  按键扫描循环（应在主循环中调用）
  * @param  keys  按键数组指针
  * @retval None
  */
void Key_Loop(KeyTypeDef **keys)
{
    for(int i = 0; i < registered_key_count; i++) {
        // 检查指针是否有效
        if(keys[i] == NULL) continue;
        
        // 处理按键状态机
        Key_StateMachine(keys[i]);
    }
    
    // 调用用户回调函数
    if(user_callback != NULL) {
        user_callback();
    }
}

/**
  * @brief  SysTick中断中调用的计数函数（1ms调用一次）
  * @retval None
  */
void KeySysTickAddCount(void)
{
    systick_1ms_counter++;
}
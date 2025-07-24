/**
  ******************************************************************************
  * @file           : uart.h
  * @author         : ShanQue
  * @brief          : UART串口通信库
  * @date           : 2025/05/28
  ******************************************************************************
  */

#ifndef DIPAN_UART_H
#define DIPAN_UART_H

/* 头文件包含 */

#include "main.h"
#include "usart.h"
#include "stdio.h"

/* 宏定义 */

#define APP_TX_DATA_SIZE			512		// 发送字符串最大长度
#define USARTx_SendHexData(huartx, data)  USARTx_SendHexDatas(huartx, data, sizeof(data))

/* 函数声明 */

HAL_StatusTypeDef USARTx_printf(UART_HandleTypeDef huartx, const char *format, ...);
HAL_StatusTypeDef USARTx_SendHexDatas(UART_HandleTypeDef huartx, uint8_t *data, uint16_t length);

HAL_StatusTypeDef UART_SendString(UART_HandleTypeDef huartx, char *str);
HAL_StatusTypeDef UART_SendIntWithPrefix(UART_HandleTypeDef huartx, const char *prefix, int32_t num);
HAL_StatusTypeDef UART_SendFloatWithPrefix(UART_HandleTypeDef huartx, const char *prefix, float num, uint8_t precision);
HAL_StatusTypeDef UART_SendHexWithPrefix(UART_HandleTypeDef huartx, const char *prefix, uint32_t num);
HAL_StatusTypeDef UART_SendNewLine(UART_HandleTypeDef huartx);
HAL_StatusTypeDef UART_SendHexFormatted(UART_HandleTypeDef huartx, uint8_t *data, uint16_t length, uint8_t bytes_per_line);



#endif //DIPAN_UART_H
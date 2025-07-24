/**
  ******************************************************************************
  * @file           : uart.c
  * @author         : ShanQue
  * @brief          : UART串口通信库
  * @date           : 2025/05/28
  ******************************************************************************
  */

#include "uart.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/**
  * @brief  串口格式化输出
  * @param  huartx  串口句柄指针
  * @param  format  格式化字符串
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef USARTx_printf(UART_HandleTypeDef huartx, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int length = vsnprintf((char *)UserTxBufferFS, APP_TX_DATA_SIZE, format, args);
	va_end(args);
	
	// 防止缓冲区溢出
	if (length >= APP_TX_DATA_SIZE) {
		length = APP_TX_DATA_SIZE - 1;
	}
	
	return HAL_UART_Transmit(&huartx, UserTxBufferFS, length, HAL_MAX_DELAY);
}

/**
  * @brief  发送16进制数据
  * @param  huartx  串口句柄指针
  * @param  data    数据指针
  * @param  length  数据长度
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef USARTx_SendHexDatas(UART_HandleTypeDef huartx, uint8_t *data, uint16_t length)
{
	return HAL_UART_Transmit(&huartx, data, length, HAL_MAX_DELAY);
}

/**
  * @brief  发送字符串
  * @param  huartx  串口句柄指针
  * @param  str     字符串指针
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendString(UART_HandleTypeDef huartx, char *str)
{
	return HAL_UART_Transmit(&huartx, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

/**
  * @brief  发送带前缀的整数
  * @param  huartx  串口句柄指针
  * @param  prefix  前缀字符串
  * @param  num     整数值
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendIntWithPrefix(UART_HandleTypeDef huartx, const char *prefix, int32_t num)
{
	return USARTx_printf(huartx, "%s: %d\r\n", prefix, num);
}

/**
  * @brief  发送带前缀的浮点数
  * @param  huartx     串口句柄指针
  * @param  prefix     前缀字符串
  * @param  num        浮点数值
  * @param  precision  小数位数
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendFloatWithPrefix(UART_HandleTypeDef huartx, const char *prefix, float num, uint8_t precision)
{
	char format[16];
	snprintf(format, sizeof(format), "%%s: %%.%df\r\n", precision);
	return USARTx_printf(huartx, format, prefix, num);
}

/**
  * @brief  发送带前缀的16进制值
  * @param  huartx  串口句柄指针
  * @param  prefix  前缀字符串
  * @param  num     数值
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendHexWithPrefix(UART_HandleTypeDef huartx, const char *prefix, uint32_t num)
{
	return USARTx_printf(huartx, "%s: 0x%02X\r\n", prefix, num);
}

/**
  * @brief  发送换行符
  * @param  huartx  串口句柄指针
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendNewLine(UART_HandleTypeDef huartx)
{
	return HAL_UART_Transmit(&huartx, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
}

/**
  * @brief  发送16进制数据并格式化显示
  * @param  huartx      串口句柄指针
  * @param  data        数据指针
  * @param  length      数据长度
  * @param  bytes_per_line 每行显示的字节数（0表示不换行）
  * @retval HAL_StatusTypeDef 发送状态
  */
HAL_StatusTypeDef UART_SendHexFormatted(UART_HandleTypeDef huartx, uint8_t *data, uint16_t length, uint8_t bytes_per_line)
{
	HAL_StatusTypeDef status = HAL_OK;
	
	for(uint16_t i = 0; i < length; i++) {
		status = USARTx_printf(huartx, "%02X ", data[i]);
		if(status != HAL_OK) return status;
		
		// 根据设置的每行字节数换行
		if(bytes_per_line > 0 && (i + 1) % bytes_per_line == 0) {
			status = UART_SendNewLine(huartx);
			if(status != HAL_OK) return status;
		}
	}
	
	// 如果启用了换行且最后一行不完整，补充换行
	if(bytes_per_line > 0 && length % bytes_per_line != 0) {
		status = UART_SendNewLine(huartx);
	}
	
	return status;
}
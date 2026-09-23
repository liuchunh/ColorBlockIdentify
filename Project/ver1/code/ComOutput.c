/*
 * ComOutput.c
 *
 *  Created on: 2026年9月18日
 *      Author: Lenovo
 */


#include "ComOutput.h"

/**
 * @brief 向 uart 串口打印保留 precision 小数位数的 num
 *
 * @param uart 串口
 * @param num 要输出的数字
 * @param precision 要保留的小数位数
 *
 * @note 要保留的 num 小数整数不能超过 255
 *
 * @example uart_write_float(UART_0, 3.14159, 2);
 */
void uart_write_float(uart_index_enum uart, float num, unsigned char precision){
    char buf[8] = {0};
    if (num < 0){
        uart_write_byte(uart, '-');
        num = -num;
    }
    unsigned int index = 0;
    unsigned int intPart = (int)num;
    float decimalPart = num - intPart;
    if (intPart == 0){ // 整数部分为 0
        uart_write_byte(uart, '0');
    }
    else{
        while (intPart > 0){
            buf[index ++] = (intPart % 10) + '0';
            intPart /= 10;
        }
        for (signed char i = index - 1; i >= 0; i -- ){
            uart_write_byte(uart, buf[i]);
        }
    }
    uart_write_byte(uart, '.');
    unsigned char count = 0;
    while (count < precision && decimalPart != (int)decimalPart){
        decimalPart *= 10;
        uart_write_byte(uart, (int)decimalPart % 10 + '0');
        count ++;
    }
}

/**
 * @brief 向 uart 串口打印整数 num
 *
 * @param uart 串口
 * @param num 要输出的数字
 *
 * @note num 不能超过 255
 * @note 不再使用 snprintf / vsnprintf , 其实现内部过于复杂, 会占用大量 ram
 *
 * @example uart_write_integer(UART_0, 123);
 */
void uart_write_integer(uart_index_enum uart, int num){
    if (num == 0){
        uart_write_byte(uart, '0');
        return;
    }

    char buf[8] = {0};
    if (num < 0){
        uart_write_byte(uart, '-');
        num = -num;
    }
    unsigned char index = 0;
    while (num > 0){
        buf[index ++] = (num % 10) + '0';
        num /= 10;
    }
    for (signed char i = index - 1; i >= 0; i -- ){
        uart_write_byte(uart, buf[i]);
    }
}

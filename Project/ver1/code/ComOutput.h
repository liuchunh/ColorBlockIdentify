/*
 * ComOutput.h
 *
 *  Created on: 2026Äê9ÔÂ18ÈÕ
 *      Author: Lenovo
 */

#ifndef CODE_COMOUTPUT_H_
#define CODE_COMOUTPUT_H_

#include <string.h>
#include "zf_driver_uart.h"
#include "zf_common_debug.h"

void uart_write_float(uart_index_enum uart, float num, unsigned char precision);

void uart_write_integer(uart_index_enum uart, int num);

#endif /* CODE_COMOUTPUT_H_ */

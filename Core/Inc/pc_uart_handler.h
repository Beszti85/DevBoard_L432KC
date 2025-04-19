/*
 * pcba_uart_handler.h
 *
 *  Created on: Apr 14, 2025
 *      Author: drCsabesz
 */

#ifndef INC_PC_UART_HANDLER_H_
#define INC_PC_UART_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void PCUART_ProcessRxCmd( uint8_t* cmdbuffer );

#ifdef __cplusplus
}
#endif

#endif /* INC_PC_UART_HANDLER_H_ */

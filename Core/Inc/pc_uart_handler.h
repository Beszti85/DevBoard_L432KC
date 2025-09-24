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

typedef enum
{
  PCUART_CONNECT = 0u,
  PCUART_READ_DATA,
  PCUART_WRITE_DATA,
  PCUART_CMD_EXEC,
  PCUART_NUM_OF_CMD
} PCUART_ServiceCommand_e;


uint8_t PCUART_ProcessRxCmd( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer );

#ifdef __cplusplus
}
#endif

#endif /* INC_PC_UART_HANDLER_H_ */

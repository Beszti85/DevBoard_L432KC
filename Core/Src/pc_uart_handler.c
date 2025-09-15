/*
 * pc_uart_handler.c
 *
 *  Created on: Apr 14, 2025
 *      Author: drCsabesz
 */

#include "pc_uart_handler.h"
#include "esp8266_at.h"
#include "flash.h"
#include "pc_act_cmd.h"
#include "pc_data_readwrite.h"

extern UART_HandleTypeDef huart1;

uint8_t PCUART_EspAtCmdNum = 0u;
static uint8_t ResponseLength = 0u;
static uint8_t PcUartConn = 0u;

static uint8_t PcUartCrc8( uint8_t crc8, uint8_t const* ptrBuffer, uint8_t size )
{
  while( size != 0u )
  {
    crc8 += *ptrBuffer;
    ptrBuffer++;
    size--;
  }

  return crc8;
}

static void PcUartProtHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{
  PCUART_ServiceCommand_e cmd = (PCUART_ServiceCommand_e)ptrRxBuffer[0];
  // Check first byte:
  switch (cmd)
  {
    // Connect
    case PCUART_CONNECT:
      PcUartConn = 1;
      *ptrTxBuffer = 0x12u;
      ptrTxBuffer++;
      break;
    // Execute Cmd
    case PCUART_READ_DATA:
      PC_ExecCmdHandler(&ptrRxBuffer[1], ptrTxBuffer);
      break;
    // Read Data
    case PCUART_CMD_EXEC:
      PC_ReadDataHandler(ptrRxBuffer[1], ptrTxBuffer);
    default:
      break;
  }
}

void PCUART_ProcessRxCmd( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{
  uint8_t cmdLength = 0u;
  uint8_t calcCrc8  = 0u;
  // First byte: 0xBE
  if(   (ptrRxBuffer[0] == 0xBEu)
     && (ptrRxBuffer[1] == ptrRxBuffer[2])
	   && (ptrRxBuffer[3] == ptrRxBuffer[0])
     && (ptrRxBuffer[4 + ptrRxBuffer[1] + 1u] == 0x27u) )
  {
    // save length info
    cmdLength = ptrRxBuffer[1];
    // check CRC
    calcCrc8 = PcUartCrc8( 0u, &ptrRxBuffer[4], cmdLength );
    if( calcCrc8 == ptrRxBuffer[4 + cmdLength] )
    {
      // CRC OK: process data frame
      PcUartProtHandler(&ptrRxBuffer[4], ptrTxBuffer);
    }
  }
}

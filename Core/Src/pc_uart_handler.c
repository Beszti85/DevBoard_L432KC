/*
 * pc_uart_handler.c
 *
 *  Created on: Apr 14, 2025
 *      Author: drCsabesz
 */

#include "pc_uart_handler.h"
#include "esp8266_at.h"

extern UART_HandleTypeDef huart1;

uint8_t PCUART_RxBuffer[80u];

uint8_t PCUART_EspAtCmdNum = 0u;
static uint8_t ResponseLength = 0uj;

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

static void PC_ExecCmdHandler( uint8_t* ptrBuffer )
{

}

static void PC_ReadDataHandler( uint8_t* ptrBuffer )
{

}

static void PcUartProtHandler( uint8_t* ptrBuffer )
{
  uint8_t cmd = ptrBuffer[0];
  // Check first byte:
  switch (cmd)
  {
    case 1:
      PC_ExecCmdHandler(&ptrBuffer[1]);
      break;
    case 2:
      PC_ReadDataHandler(&ptrBuffer[1]);
    default:
      break;
  }
}

void PCUART_ProcessRxCmd( uint8_t* ptrBuffer )
{
  uint8_t cmdLength = 0u;
  uint8_t calcCrc8  = 0u;
  // First byte: 0xBE
  if(   (ptrBuffer[0] == 0xBEu)
     && (ptrBuffer[1] == ptrBuffer[2])
	 && (ptrBuffer[3] == ptrBuffer[0])
     && (ptrBuffer[4 + ptrBuffer[1]] == 0x27u) )
  {
    // save length info
    cmdLength = ptrBuffer[1];
    // check CRC
    calcCrc8 = PcUartCrc8( 0u, &ptrBuffer[4], cmdLength );
    if( calcCrc8 == ptrBuffer[4 + cmdLength] )
    {
      // CRC OK: process data frame
    }
  }
}

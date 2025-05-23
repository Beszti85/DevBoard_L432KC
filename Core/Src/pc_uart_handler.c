/*
 * pc_uart_handler.c
 *
 *  Created on: Apr 14, 2025
 *      Author: drCsabesz
 */

#include "pc_uart_handler.h"
#include "esp8266_at.h"
#include "bme280.h"

extern UART_HandleTypeDef huart1;
extern BME280_PhysValues_t BME280_PhysicalValues;

uint8_t PCUART_EspAtCmdNum = 0u;
static uint8_t ResponseLength = 0u;

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

static void PC_ExecCmdHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{

}

static void PC_ReadDataHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{
  switch (ptrRxBuffer[0])
  {
    // Read Temperature BME280
    case 1:
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Temperature, sizeof(BME280_PhysicalValues.Temperature));
      break;
    // Read Humidity BME280
    case 2:
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Humidity, sizeof(BME280_PhysicalValues.Humidity));
      break;
    // Read Humidity BME280
    case 3:
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Pressure, sizeof(BME280_PhysicalValues.Pressure));
      break;
    default:
      break;
  }
}

static void PcUartProtHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{
  uint8_t cmd = ptrRxBuffer[0];
  // Check first byte:
  switch (cmd)
  {
    case 1:
      PC_ExecCmdHandler(&ptrRxBuffer[1], ptrTxBuffer);
      break;
    case 2:
      PC_ReadDataHandler(&ptrRxBuffer[1], ptrTxBuffer);
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
     && (ptrRxBuffer[4 + ptrRxBuffer[1]] == 0x27u) )
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

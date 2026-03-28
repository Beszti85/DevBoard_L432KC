/*
 * pc_act_cmd.c
 *
 *  Created on: Sep 7, 2025
 *      Author: drCsabesz
 */

#include "pc_act_cmd.h"
#include "flash.h"
#include "ds1307.h"
#include "led.h"
#include "nrf24l01.h"

extern FLASH_Handler_t FlashHandler;
extern DS1307_Handler_t DS1307_Handle;
extern LED_IO_Handler_s LED_Yellow;
extern LED_PWM_Handler_s LED_Red;
extern NRF24L01_Handler_t RFHandler;

void PC_ExecCmdHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
{
  uint8_t cmd = ptrRxBuffer[0];
  uint32_t tmpU32 = 0u;
  uint16_t tmpU16 = 0u;
  // Cehck first byte
  switch (cmd)
  {
    // Read external flash
    case PC_CMD_FLASH_READ:
      tmpU32 = ( (uint32_t)ptrRxBuffer[1] << 16u ) |
               ( (uint32_t)ptrRxBuffer[2] << 8u )  |
               ( (uint32_t)ptrRxBuffer[3] );
      tmpU16 = ( (uint16_t)ptrRxBuffer[4] << 8u )  |
               ( (uint16_t)ptrRxBuffer[5] );
      FLASH_Read(&FlashHandler, tmpU32, ptrTxBuffer, tmpU16);
      break;
    // Write external flash
    case PC_CMD_FLASH_WRITE:
      tmpU32 = ( (uint32_t)ptrRxBuffer[1] << 16u ) |
               ( (uint32_t)ptrRxBuffer[2] << 8u )  |
               ( (uint32_t)ptrRxBuffer[3] );
      tmpU16 = ( (uint16_t)ptrRxBuffer[4] << 8u ) |
               ( (uint16_t)ptrRxBuffer[5] );
      FLASH_Write( &FlashHandler, tmpU32, &ptrRxBuffer[6], tmpU16 );
      break;
    // Erase external flash
    case PC_CMD_FLASH_ERASE:
      FLASH_Erase( &FlashHandler );
      break;
    // Start DS1307
    case PC_CMD_DS1307_START:
      DS1307_TimeDate_t dateTime;
      memcpy(&dateTime, ptrRxBuffer, sizeof(dateTime));
      DS1307_Init( &DS1307_Handle, &dateTime );
      break;
    // Control the DS1307 output control signal
    case PC_CMD_DS1307_CTRL_SQW:
      DS1307_SquareWaveOutput( &DS1307_Handle, (DS1307_Frequency_e)ptrRxBuffer[1] );
      break;
    // Control basic IO LED
    case LED_CTRL_TOGGLE:
      LED_Toggle( &LED_Yellow );
      break;
    // Control PWM-driven LED
    case LED_PWM_CTRL:
      LED_SetPwmDuty( &LED_Red, ptrRxBuffer[1]);
      break;
    case NRF24_READ_REG_1BYTE:
      NRF24L01_ReadRegister1Byte( &RFHandler, (NRF24L01_RegParam_e)ptrRxBuffer[1] );
      memcpy( ptrTxBuffer, &RFHandler.RxBuffer, 1u );
      break;
    case NRF24_WRITE_REG_1BYTE:
      NRF24L01_WriteRegister1Byte( &RFHandler, (NRF24L01_RegParam_e)ptrRxBuffer[1u], ptrRxBuffer[2u] );
      break;
  }
}

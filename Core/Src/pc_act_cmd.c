/*
 * pc_act_cmd.c
 *
 *  Created on: Sep 7, 2025
 *      Author: drCsabesz
 */

#include "pc_act_cmd.h"
#include "flash.h"
#include "ds1307.h"

extern FLASH_Handler_t FlashHandler;
extern DS1307_Handler_t DS1307_Handle;

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
      FLASH_Write(&FlashHandler, tmpU32, &ptrRxBuffer[6], tmpU16);
      break;
    // Erase external flash
    case PC_CMD_FLASH_ERASE:
      FLASH_Erase(&FlashHandler);
    // Start DS1307
    case PC_CMD_DS1307_START:
      DS1307_TimeDate_t dateTime;
      memcpy(&dateTime, ptrRxBuffer, sizeof(dateTime));
      DS1307_Init(&DS1307_Handle, &dateTime);
  }
}

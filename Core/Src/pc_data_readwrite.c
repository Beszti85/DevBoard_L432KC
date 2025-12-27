/*
 * pc_data_readwrite.c
 *
 *  Created on: Sep 7, 2025
 *      Author: drCsabesz
 */

#include "pc_data_readwrite.h"
#include "bme280.h"
#include "flash.h"

// TODO: restructure the extern variables
extern BME280_PhysValues_t BME280_PhysicalValues;
extern FLASH_Handler_t FlashHandler;
extern float ADC_Voltage[5u];
extern uint32_t TIM1_PwmDutyCycle;

// Static module variables
uint32_t PcExtFlashReadAddress = 0u;
uint32_t PcExtFlashReadLength = 0u;

// Data read handler
uint8_t PC_ReadDataHandler( uint8_t readId, uint8_t* ptrTxBuffer )
{
  // return value is the length of the response
  uint8_t retval = 0u;

  switch (readId)
  {
    // Read board name
    case BOARD_ID:
      memcpy(ptrTxBuffer, "NucleoL432KC_DevBoard", sizeof("NucleoL432KC_DevBoard"));
      retval = sizeof("NucleoL432KC_DevBoard");
      break;
    // Read BME280 physical values
    case BME280_PHYSICAL_VALUES:
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Temperature, sizeof(BME280_PhysicalValues.Temperature));
      ptrTxBuffer += sizeof(BME280_PhysicalValues.Temperature);
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Humidity, sizeof(BME280_PhysicalValues.Humidity));
      ptrTxBuffer += sizeof(BME280_PhysicalValues.Humidity);
      memcpy(ptrTxBuffer, &BME280_PhysicalValues.Pressure, sizeof(BME280_PhysicalValues.Pressure));
      ptrTxBuffer += sizeof(BME280_PhysicalValues.Pressure);
      retval = 12u;
      break;
    // Read ADC data
    case ADC_PHY_VALUES:
      memcpy(ptrTxBuffer, ADC_Voltage, sizeof(ADC_Voltage));
      retval = 20u;
      break;
    // Read Flash ID
    case FLASH_ID:
      memcpy(ptrTxBuffer, &FlashHandler.DetectedFlash, sizeof(FlashHandler.DetectedFlash));
      retval = 1u;
      break;
    // Read Flash data
    case FLASH_READ:
      FLASH_Read(&FlashHandler, PcExtFlashReadAddress, ptrTxBuffer, PcExtFlashReadLength);
      retval = PcExtFlashReadLength;
      break;
    case LED_PWM:
      memcpy(ptrTxBuffer, &TIM1_PwmDutyCycle, sizeof(TIM1_PwmDutyCycle));
      retval = 4u;
      break;
    default:
      break;
  }

  return retval;
}

// Data write handler
void PC_WriteDataHandler( uint8_t* ptrTxBuffer )
{
  uint8_t writeId = ptrTxBuffer[0];
  switch (writeId)
  {
    // Set flash read address
    case FLASH_CFG_WRITE:
      memcpy(&PcExtFlashReadAddress, ptrTxBuffer, sizeof(PcExtFlashReadAddress));
      memcpy(&PcExtFlashReadLength, ptrTxBuffer + sizeof(PcExtFlashReadAddress), sizeof(PcExtFlashReadLength));
      break;
    default:
      break;
  }
}
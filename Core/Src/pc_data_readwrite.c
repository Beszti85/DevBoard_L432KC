/*
 * pc_data_readwrite.c
 *
 *  Created on: Sep 7, 2025
 *      Author: drCsabesz
 */

#include "pc_data_readwrite.h"
#include "bme280.h"

extern BME280_PhysValues_t BME280_PhysicalValues;

void PC_ReadDataHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer )
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

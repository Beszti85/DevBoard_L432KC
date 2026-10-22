/*
 * pc_data_readwrite.h
 *
 *  Created on: Sep 1, 2025
 *      Author: drCsabesz
 */

#ifndef INC_PC_DATA_READWRITE_H_
#define INC_PC_DATA_READWRITE_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PC_RD_BOARD_ID                 0u
#define PC_RD_BME280_PHYSICAL_VALUES   1u
#define PC_RD_ADC_PHY_VALUES           2u
#define PC_RD_FLASH_ID                 3u
#define PC_RW_LED_PWM                  4u
#define PC_RD_FLASH_READ               5u
#define PC_RW_RTC_READ_TIME            6u
#define PC_WR_DAC_OUTPUT               7u

#define FLASH_CFG_WRITE          1u

#define DATA_ID_NOT_FOUND        0xFEu
#define DATA_INTERNAL_ERROR      0xFDu

uint8_t PC_ReadDataHandler( uint8_t readId, uint8_t* ptrTxBuffer );
void PC_WriteDataHandler( uint8_t* ptrTxBuffer );

#endif /* INC_PC_DATA_READWRITE_H_ */

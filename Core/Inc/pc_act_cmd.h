/*
 * pc_act_cmd.h
 *
 *  Created on: Sep 1, 2025
 *      Author: drCsabesz
 */

#ifndef INC_PC_ACT_CMD_H_
#define INC_PC_ACT_CMD_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define PC_CMD_FLASH_READ           0x01u
#define PC_CMD_FLASH_WRITE          0x02u
#define PC_CMD_FLASH_ERASE          0x03u
#define PC_CMD_DS1307_START         0x04u
#define LED_CTRL_TOGGLE             0x05u
#define LED_PWM_CTRL                0x06u
#define PC_CMD_DS1307_CTRL_SQW      0x11u
#define NRF24_READ_REG_1BYTE        0x15u
#define NRF24_WRITE_REG_1BYTE       0x16u

void PC_ExecCmdHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer );

#endif /* INC_PC_ACT_CMD_H_ */

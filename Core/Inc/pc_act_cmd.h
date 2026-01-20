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

#define PC_CMD_FLASH_READ    0x01u
#define PC_CMD_FLASH_WRITE   0x02u
#define PC_CM_FLASH_ERASE    0x03u

void PC_ExecCmdHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer );

#endif /* INC_PC_ACT_CMD_H_ */

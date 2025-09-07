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

void PC_ExecCmdHandler( uint8_t* ptrRxBuffer, uint8_t* ptrTxBuffer );

#endif /* INC_PC_ACT_CMD_H_ */

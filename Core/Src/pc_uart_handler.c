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

void PCUART_ProcessRxCmd( uint8_t* ptrBuffer )
{
  if( !strncmp( ptrBuffer, "LED_TOGGLE", sizeof("LED_TOGGLE") - 1 ) )
  {
    //PCA9685_ToggleOutputEnable(&LedDriverHandle);
  }
  else
  if( !strncmp( ptrBuffer, "ESP_AT", sizeof("ESP_AT") - 1 ) )
  {
    // go to the end of the cmd prefix
    ptrBuffer += sizeof("ESP_AT")-1;
    // Check the end of the string
    if( *ptrBuffer != ' ' )
    {
      // Get the number
      PCUART_EspAtCmdNum = *ptrBuffer - '0';
      ptrBuffer++;
      // Any further characters - number > 9
      if( *ptrBuffer != ' ' )
      {
        PCUART_EspAtCmdNum *= 10u;
        PCUART_EspAtCmdNum += (*ptrBuffer - '0');
      }
      ESP8266_ProcessAtCmd( &huart1, (ESP8266_CMD_ID)PCUART_EspAtCmdNum );
    }
  }
}

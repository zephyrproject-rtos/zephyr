/**
  ******************************************************************************
  * @file    es_wifi_io.h
  * @author  MCD Application Team
  * @brief   This file contains the functions prototypes for es_wifi IO operations.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
#ifndef __WIFI_IO__
#define __WIFI_IO__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define WIFI_RESET_MODULE()                do{\
                                            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET );\
                                            HAL_Delay(10);\
                                            HAL_GPIO_WritePin( GPIOE, GPIO_PIN_8, GPIO_PIN_SET );\
                                            HAL_Delay(500);\
                                             }while(0);


#define WIFI_ENABLE_NSS()                  do{ \
                                             HAL_GPIO_WritePin( GPIOE, GPIO_PIN_0, GPIO_PIN_RESET );\
                                             HAL_Delay(10);\
                                             }while(0);

#define WIFI_DISABLE_NSS()                 do{ \
                                             HAL_GPIO_WritePin( GPIOE, GPIO_PIN_0, GPIO_PIN_SET );\
                                             HAL_Delay(10);\
                                             }while(0);

#define WIFI_IS_CMDDATA_READY()            (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_SET)

    

/* Exported functions ------------------------------------------------------- */ 
void    SPI_WIFI_MspInit(SPI_HandleTypeDef* hspi);
int8_t  SPI_WIFI_DeInit(void);
int8_t  SPI_WIFI_Init(void);
int16_t SPI_WIFI_ReceiveData(uint8_t *pData, uint16_t len, uint32_t timeout);
int16_t SPI_WIFI_SendData( uint8_t *pData, uint16_t len, uint32_t timeout);
void    SPI_WIFI_Delay(uint32_t Delay);
    
#ifdef __cplusplus
}
#endif

#endif /* __WIFI_IO__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/    

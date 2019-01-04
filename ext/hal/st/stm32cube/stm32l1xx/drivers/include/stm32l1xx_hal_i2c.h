/**
  ******************************************************************************
  * @file    stm32l1xx_hal_i2c.h
  * @author  MCD Application Team
  * @brief   Header file of I2C HAL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32L1xx_HAL_I2C_H
#define __STM32L1xx_HAL_I2C_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal_def.h"

/** @addtogroup STM32L1xx_HAL_Driver
  * @{
  */

/** @addtogroup I2C
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup I2C_Exported_Types I2C Exported Types
  * @{
  */

/** @defgroup I2C_Configuration_Structure_definition I2C Configuration Structure definition
  * @brief  I2C Configuration Structure definition
  * @{
  */
typedef struct
{
  uint32_t ClockSpeed;       /*!< Specifies the clock frequency.
                                  This parameter must be set to a value lower than 400kHz */

  uint32_t DutyCycle;        /*!< Specifies the I2C fast mode duty cycle.
                                  This parameter can be a value of @ref I2C_duty_cycle_in_fast_mode */

  uint32_t OwnAddress1;      /*!< Specifies the first device own address.
                                  This parameter can be a 7-bit or 10-bit address. */

  uint32_t AddressingMode;   /*!< Specifies if 7-bit or 10-bit addressing mode is selected.
                                  This parameter can be a value of @ref I2C_addressing_mode */

  uint32_t DualAddressMode;  /*!< Specifies if dual addressing mode is selected.
                                  This parameter can be a value of @ref I2C_dual_addressing_mode */

  uint32_t OwnAddress2;      /*!< Specifies the second device own address if dual addressing mode is selected
                                  This parameter can be a 7-bit address. */

  uint32_t GeneralCallMode;  /*!< Specifies if general call mode is selected.
                                  This parameter can be a value of @ref I2C_general_call_addressing_mode */

  uint32_t NoStretchMode;    /*!< Specifies if nostretch mode is selected.
                                  This parameter can be a value of @ref I2C_nostretch_mode */

}I2C_InitTypeDef;

/**
  * @}
  */

/** @defgroup HAL_state_structure_definition HAL state structure definition
  * @brief  HAL State structure definition  
  * @note  HAL I2C State value coding follow below described bitmap :
  *          b7-b6  Error information 
  *             00 : No Error
  *             01 : Abort (Abort user request on going)
  *             10 : Timeout
  *             11 : Error
  *          b5     IP initilisation status
  *             0  : Reset (IP not initialized)
  *             1  : Init done (IP initialized and ready to use. HAL I2C Init function called)
  *          b4     (not used)
  *             x  : Should be set to 0
  *          b3
  *             0  : Ready or Busy (No Listen mode ongoing)
  *             1  : Listen (IP in Address Listen Mode)
  *          b2     Intrinsic process state
  *             0  : Ready
  *             1  : Busy (IP busy with some configuration or internal operations)
  *          b1     Rx state
  *             0  : Ready (no Rx operation ongoing)
  *             1  : Busy (Rx operation ongoing)
  *          b0     Tx state
  *             0  : Ready (no Tx operation ongoing)
  *             1  : Busy (Tx operation ongoing)
  * @{
  */
typedef enum
{
  HAL_I2C_STATE_RESET             = 0x00U,   /*!< Peripheral is not yet Initialized         */
  HAL_I2C_STATE_READY             = 0x20U,   /*!< Peripheral Initialized and ready for use  */
  HAL_I2C_STATE_BUSY              = 0x24U,   /*!< An internal process is ongoing            */
  HAL_I2C_STATE_BUSY_TX           = 0x21U,   /*!< Data Transmission process is ongoing      */
  HAL_I2C_STATE_BUSY_RX           = 0x22U,   /*!< Data Reception process is ongoing         */
  HAL_I2C_STATE_LISTEN            = 0x28U,   /*!< Address Listen Mode is ongoing            */
  HAL_I2C_STATE_BUSY_TX_LISTEN    = 0x29U,   /*!< Address Listen Mode and Data Transmission
                                                 process is ongoing                         */
  HAL_I2C_STATE_BUSY_RX_LISTEN    = 0x2AU,   /*!< Address Listen Mode and Data Reception
                                                 process is ongoing                         */
  HAL_I2C_STATE_ABORT             = 0x60U,   /*!< Abort user request ongoing                */
  HAL_I2C_STATE_TIMEOUT           = 0xA0U,   /*!< Timeout state                             */
  HAL_I2C_STATE_ERROR             = 0xE0U    /*!< Error                                     */

}HAL_I2C_StateTypeDef;

/**
  * @}
  */

/** @defgroup HAL_mode_structure_definition HAL mode structure definition
  * @brief  HAL Mode structure definition
  * @note  HAL I2C Mode value coding follow below described bitmap :
  *          b7     (not used)
  *             x  : Should be set to 0
  *          b6
  *             0  : None
  *             1  : Memory (HAL I2C communication is in Memory Mode)
  *          b5
  *             0  : None
  *             1  : Slave (HAL I2C communication is in Slave Mode)
  *          b4
  *             0  : None
  *             1  : Master (HAL I2C communication is in Master Mode)
  *          b3-b2-b1-b0  (not used)
  *             xxxx : Should be set to 0000
  * @{
  */
typedef enum
{
  HAL_I2C_MODE_NONE               = 0x00U,   /*!< No I2C communication on going             */
  HAL_I2C_MODE_MASTER             = 0x10U,   /*!< I2C communication is in Master Mode       */
  HAL_I2C_MODE_SLAVE              = 0x20U,   /*!< I2C communication is in Slave Mode        */
  HAL_I2C_MODE_MEM                = 0x40U    /*!< I2C communication is in Memory Mode       */

}HAL_I2C_ModeTypeDef;

/** 
  * @}
  */

/** @defgroup I2C_Error_Code_definition I2C Error Code definition
  * @brief  I2C Error Code definition  
  * @{
  */ 
#define HAL_I2C_ERROR_NONE       (0x00000000U)    /*!< No error           */
#define HAL_I2C_ERROR_BERR       (0x00000001U)    /*!< BERR error         */
#define HAL_I2C_ERROR_ARLO       (0x00000002U)    /*!< ARLO error         */
#define HAL_I2C_ERROR_AF         (0x00000004U)    /*!< AF error           */
#define HAL_I2C_ERROR_OVR        (0x00000008U)    /*!< OVR error          */
#define HAL_I2C_ERROR_DMA        (0x00000010U)    /*!< DMA transfer error */
#define HAL_I2C_ERROR_TIMEOUT    (0x00000020U)    /*!< Timeout Error      */
/**
  * @}
  */

/** @defgroup I2C_handle_Structure_definition I2C handle Structure definition 
  * @brief  I2C handle Structure definition   
  * @{
  */ 
typedef struct
{
  I2C_TypeDef                *Instance;      /*!< I2C registers base address               */

  I2C_InitTypeDef            Init;           /*!< I2C communication parameters             */

  uint8_t                    *pBuffPtr;      /*!< Pointer to I2C transfer buffer           */

  uint16_t                   XferSize;       /*!< I2C transfer size                        */

  __IO uint16_t              XferCount;      /*!< I2C transfer counter                     */

  __IO uint32_t              XferOptions;    /*!< I2C transfer options                     */

  __IO uint32_t              PreviousState;  /*!< I2C communication Previous state and mode
                                                  context for internal usage               */

  DMA_HandleTypeDef          *hdmatx;        /*!< I2C Tx DMA handle parameters             */

  DMA_HandleTypeDef          *hdmarx;        /*!< I2C Rx DMA handle parameters             */

  HAL_LockTypeDef            Lock;           /*!< I2C locking object                       */

  __IO HAL_I2C_StateTypeDef  State;          /*!< I2C communication state                  */

  __IO HAL_I2C_ModeTypeDef   Mode;           /*!< I2C communication mode                   */

  __IO uint32_t              ErrorCode;      /*!< I2C Error code                           */

  __IO uint32_t              Devaddress;     /*!< I2C Target device address                */

  __IO uint32_t              Memaddress;     /*!< I2C Target memory address                */

  __IO uint32_t              MemaddSize;     /*!< I2C Target memory address  size          */

  __IO uint32_t              EventCount;     /*!< I2C Event counter                        */

}I2C_HandleTypeDef;

/**
  * @}
  */

/**
  * @}
  */  
/* Exported constants --------------------------------------------------------*/
/** @defgroup I2C_Exported_Constants I2C Exported Constants
  * @{
  */

/** @defgroup I2C_duty_cycle_in_fast_mode I2C duty cycle in fast mode
  * @{
  */
#define I2C_DUTYCYCLE_2                 (0x00000000U)
#define I2C_DUTYCYCLE_16_9              I2C_CCR_DUTY
/**
  * @}
  */

/** @defgroup I2C_addressing_mode I2C addressing mode
  * @{
  */
#define I2C_ADDRESSINGMODE_7BIT         (0x00004000U)
#define I2C_ADDRESSINGMODE_10BIT        (I2C_OAR1_ADDMODE | (0x00004000U))
/**
  * @}
  */

/** @defgroup I2C_dual_addressing_mode  I2C dual addressing mode
  * @{
  */
#define I2C_DUALADDRESS_DISABLE         (0x00000000U)
#define I2C_DUALADDRESS_ENABLE          I2C_OAR2_ENDUAL
/**
  * @}
  */

/** @defgroup I2C_general_call_addressing_mode I2C general call addressing mode
  * @{
  */
#define I2C_GENERALCALL_DISABLE         (0x00000000U)
#define I2C_GENERALCALL_ENABLE          I2C_CR1_ENGC
/**
  * @}
  */

/** @defgroup I2C_nostretch_mode I2C nostretch mode
  * @{
  */
#define I2C_NOSTRETCH_DISABLE           (0x00000000U)
#define I2C_NOSTRETCH_ENABLE            I2C_CR1_NOSTRETCH
/**
  * @}
  */

/** @defgroup I2C_Memory_Address_Size I2C Memory Address Size
  * @{
  */
#define I2C_MEMADD_SIZE_8BIT            (0x00000001U)
#define I2C_MEMADD_SIZE_16BIT           (0x00000010U)
/**
  * @}
  */

/** @defgroup I2C_XferDirection_definition I2C XferDirection definition Master Point of View
  * @{
  */
#define I2C_DIRECTION_RECEIVE           (0x00000000U) 
#define I2C_DIRECTION_TRANSMIT          (0x00000001U)
/**
  * @}
  */

/** @defgroup I2C_XferOptions_definition I2C XferOptions definition
  * @{
  */
#define  I2C_FIRST_FRAME                (0x00000001U)
#define  I2C_NEXT_FRAME                 (0x00000002U)
#define  I2C_FIRST_AND_LAST_FRAME       (0x00000004U)
#define  I2C_LAST_FRAME                 (0x00000008U)
/**
  * @}
  */

/** @defgroup I2C_Interrupt_configuration_definition I2C Interrupt configuration definition
  * @brief I2C Interrupt definition
  *        Elements values convention: 0xXXXXXXXX
  *           - XXXXXXXX  : Interrupt control mask
  * @{
  */
#define I2C_IT_BUF                      I2C_CR2_ITBUFEN
#define I2C_IT_EVT                      I2C_CR2_ITEVTEN
#define I2C_IT_ERR                      I2C_CR2_ITERREN
/**
  * @}
  */

/** @defgroup I2C_Flag_definition I2C Flag definition
  * @{
  */
#define I2C_FLAG_OVR                    ((uint32_t)(1U << 16U | I2C_SR1_OVR))
#define I2C_FLAG_AF                     ((uint32_t)(1U << 16U | I2C_SR1_AF))
#define I2C_FLAG_ARLO                   ((uint32_t)(1U << 16U | I2C_SR1_ARLO))
#define I2C_FLAG_BERR                   ((uint32_t)(1U << 16U | I2C_SR1_BERR))
#define I2C_FLAG_TXE                    ((uint32_t)(1U << 16U | I2C_SR1_TXE))
#define I2C_FLAG_RXNE                   ((uint32_t)(1U << 16U | I2C_SR1_RXNE))
#define I2C_FLAG_STOPF                  ((uint32_t)(1U << 16U | I2C_SR1_STOPF))
#define I2C_FLAG_ADD10                  ((uint32_t)(1U << 16U | I2C_SR1_ADD10))
#define I2C_FLAG_BTF                    ((uint32_t)(1U << 16U | I2C_SR1_BTF))
#define I2C_FLAG_ADDR                   ((uint32_t)(1U << 16U | I2C_SR1_ADDR))
#define I2C_FLAG_SB                     ((uint32_t)(1U << 16U | I2C_SR1_SB))
#define I2C_FLAG_DUALF                  ((uint32_t)(2U << 16U | I2C_SR2_DUALF))
#define I2C_FLAG_GENCALL                ((uint32_t)(2U << 16U | I2C_SR2_GENCALL))
#define I2C_FLAG_TRA                    ((uint32_t)(2U << 16U | I2C_SR2_TRA))
#define I2C_FLAG_BUSY                   ((uint32_t)(2U << 16U | I2C_SR2_BUSY))
#define I2C_FLAG_MSL                    ((uint32_t)(2U << 16U | I2C_SR2_MSL))
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup I2C_Exported_Macros I2C Exported Macros
  * @{
  */

/** @brief Reset I2C handle state.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @retval None
  */
#define __HAL_I2C_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = HAL_I2C_STATE_RESET)

/** @brief  Enable the specified I2C interrupt.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @param  __INTERRUPT__: specifies the interrupt source to enable.
  *         This parameter can be one of the following values:
  *            @arg I2C_IT_BUF: Buffer interrupt enable
  *            @arg I2C_IT_EVT: Event interrupt enable
  *            @arg I2C_IT_ERR: Error interrupt enable
  * @retval None
  */
#define __HAL_I2C_ENABLE_IT(__HANDLE__, __INTERRUPT__)   SET_BIT((__HANDLE__)->Instance->CR2,(__INTERRUPT__))

/** @brief  Disable the specified I2C interrupt.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @param  __INTERRUPT__: specifies the interrupt source to disable.
  *        This parameter can be one of the following values:
  *            @arg I2C_IT_BUF: Buffer interrupt enable
  *            @arg I2C_IT_EVT: Event interrupt enable
  *            @arg I2C_IT_ERR: Error interrupt enable
  *   
  * @retval None
  */
#define __HAL_I2C_DISABLE_IT(__HANDLE__, __INTERRUPT__)  CLEAR_BIT((__HANDLE__)->Instance->CR2, (__INTERRUPT__))

/** @brief  Check whether the specified I2C interrupt source is enabled or not.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @param  __INTERRUPT__: specifies the I2C interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg I2C_IT_BUF: Buffer interrupt enable
  *            @arg I2C_IT_EVT: Event interrupt enable
  *            @arg I2C_IT_ERR: Error interrupt enable
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#define __HAL_I2C_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((((__HANDLE__)->Instance->CR2 & (__INTERRUPT__)) == (__INTERRUPT__)) ? SET : RESET)

/** @brief  Check whether the specified I2C flag is set or not.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @param  __FLAG__ specifies the flag to check.
  *         This parameter can be one of the following values:
  *            @arg I2C_FLAG_OVR: Overrun/Underrun flag
  *            @arg I2C_FLAG_AF: Acknowledge failure flag
  *            @arg I2C_FLAG_ARLO: Arbitration lost flag
  *            @arg I2C_FLAG_BERR: Bus error flag
  *            @arg I2C_FLAG_TXE: Data register empty flag
  *            @arg I2C_FLAG_RXNE: Data register not empty flag
  *            @arg I2C_FLAG_STOPF: Stop detection flag
  *            @arg I2C_FLAG_ADD10: 10-bit header sent flag
  *            @arg I2C_FLAG_BTF: Byte transfer finished flag
  *            @arg I2C_FLAG_ADDR: Address sent flag
  *                                Address matched flag
  *            @arg I2C_FLAG_SB: Start bit flag
  *            @arg I2C_FLAG_DUALF: Dual flag
  *            @arg I2C_FLAG_GENCALL: General call header flag
  *            @arg I2C_FLAG_TRA: Transmitter/Receiver flag
  *            @arg I2C_FLAG_BUSY: Bus busy flag
  *            @arg I2C_FLAG_MSL: Master/Slave flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_I2C_GET_FLAG(__HANDLE__, __FLAG__) ((((uint8_t)((__FLAG__) >> 16U)) == 0x01U)?(((((__HANDLE__)->Instance->SR1) & ((__FLAG__) & I2C_FLAG_MASK)) == ((__FLAG__) & I2C_FLAG_MASK)) ? SET : RESET): \
                                                 (((((__HANDLE__)->Instance->SR2) & ((__FLAG__) & I2C_FLAG_MASK)) == ((__FLAG__) & I2C_FLAG_MASK)) ? SET : RESET))

/** @brief  Clear the I2C pending flags which are cleared by writing 0 in a specific bit.
  * @param  __HANDLE__ specifies the I2C Handle.
  * @param  __FLAG__ specifies the flag to clear.
  *         This parameter can be any combination of the following values:
  *            @arg I2C_FLAG_OVR: Overrun/Underrun flag (Slave mode)
  *            @arg I2C_FLAG_AF: Acknowledge failure flag
  *            @arg I2C_FLAG_ARLO: Arbitration lost flag (Master mode)
  *            @arg I2C_FLAG_BERR: Bus error flag
  * @retval None
  */
#define __HAL_I2C_CLEAR_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->SR1 = ~((__FLAG__) & I2C_FLAG_MASK))

/** @brief  Clears the I2C ADDR pending flag.
  * @param  __HANDLE__: specifies the I2C Handle.
  * @retval None
  */
#define __HAL_I2C_CLEAR_ADDRFLAG(__HANDLE__)    \
    do{                                         \
    __IO uint32_t tmpreg = 0x00U;               \
      tmpreg = (__HANDLE__)->Instance->SR1;     \
      tmpreg = (__HANDLE__)->Instance->SR2;     \
      UNUSED(tmpreg);                           \
    }while(0)

/** @brief  Clears the I2C STOPF pending flag.
  * @param  __HANDLE__: specifies the I2C Handle.
  * @retval None
  */
#define __HAL_I2C_CLEAR_STOPFLAG(__HANDLE__)            \
    do{                                                 \
    __IO uint32_t tmpreg = 0x00U;               \
      tmpreg = (__HANDLE__)->Instance->SR1;             \
      SET_BIT((__HANDLE__)->Instance->CR1, I2C_CR1_PE); \
      UNUSED(tmpreg);                                   \
    }while(0)

/** @brief  Enable the I2C peripheral.
  * @param  __HANDLE__: specifies the I2C Handle.
  *         This parameter can be I2Cx where x: 1 or 2  to select the I2C peripheral.
  * @retval None
  */
#define __HAL_I2C_ENABLE(__HANDLE__)                             SET_BIT((__HANDLE__)->Instance->CR1, I2C_CR1_PE)

/** @brief  Disable the I2C peripheral.
  * @param  __HANDLE__: specifies the I2C Handle.
  *         This parameter can be I2Cx where x: 1 or 2  to select the I2C peripheral.
  * @retval None
  */
#define __HAL_I2C_DISABLE(__HANDLE__)                            CLEAR_BIT((__HANDLE__)->Instance->CR1, I2C_CR1_PE)

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup I2C_Exported_Functions
  * @{
  */

/** @addtogroup I2C_Exported_Functions_Group1
  * @{
  */
/* Initialization and de-initialization functions******************************/
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_DeInit (I2C_HandleTypeDef *hi2c);
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c);

/**
  * @}
  */

/** @addtogroup I2C_Exported_Functions_Group2
  * @{
  */
/* IO operation functions  ****************************************************/
/******* Blocking mode: Polling */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Slave_Transmit(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Slave_Receive(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);

/******* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Slave_Transmit_IT(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Slave_Receive_IT(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size);

HAL_StatusTypeDef HAL_I2C_Master_Sequential_Transmit_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t XferOptions);
HAL_StatusTypeDef HAL_I2C_Master_Sequential_Receive_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t XferOptions);
HAL_StatusTypeDef HAL_I2C_Slave_Sequential_Transmit_IT(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t XferOptions);
HAL_StatusTypeDef HAL_I2C_Slave_Sequential_Receive_IT(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size, uint32_t XferOptions);
HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef *hi2c, uint16_t DevAddress);
HAL_StatusTypeDef HAL_I2C_EnableListen_IT(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_DisableListen_IT(I2C_HandleTypeDef *hi2c);

/******* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Slave_Transmit_DMA(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Slave_Receive_DMA(I2C_HandleTypeDef *hi2c, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Mem_Write_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Mem_Read_DMA(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size);
/**
  * @}
  */ 

/** @addtogroup I2C_IRQ_Handler_and_Callbacks IRQ Handler and Callbacks
 * @{
 */   
/******* I2C IRQHandler and Callbacks used in non blocking modes (Interrupt and DMA) */
void HAL_I2C_EV_IRQHandler(I2C_HandleTypeDef *hi2c);
void HAL_I2C_ER_IRQHandler(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode);
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c);
/**
  * @}
  */

/** @addtogroup I2C_Exported_Functions_Group3
  * @{
  */
/* Peripheral State, Mode and Error functions  *********************************/
HAL_I2C_StateTypeDef HAL_I2C_GetState(I2C_HandleTypeDef *hi2c);
HAL_I2C_ModeTypeDef  HAL_I2C_GetMode(I2C_HandleTypeDef *hi2c);
uint32_t             HAL_I2C_GetError(I2C_HandleTypeDef *hi2c);

/**
  * @}
  */ 
/**
  * @}
  */

/* Private constants ---------------------------------------------------------*/
/** @defgroup I2C_Private_Constants I2C Private Constants
  * @{
  */
#define I2C_FLAG_MASK  (0x0000FFFFU)
/**
  * @}
  */ 

/* Private macros ------------------------------------------------------------*/
/** @defgroup I2C_Private_Macro I2C Private Macros
  * @{
  */

#define IS_I2C_ADDRESSING_MODE(MODE) (((MODE) == I2C_ADDRESSINGMODE_7BIT) || \
                                         ((MODE) == I2C_ADDRESSINGMODE_10BIT))


#define IS_I2C_DUAL_ADDRESS(ADDRESS) (((ADDRESS) == I2C_DUALADDRESS_DISABLE) || \
                                      ((ADDRESS) == I2C_DUALADDRESS_ENABLE))

#define IS_I2C_OWN_ADDRESS2(ADDRESS2) (((ADDRESS2) & (0xFFFFFF01U)) == 0U)
#define IS_I2C_OWN_ADDRESS1(ADDRESS1) (((ADDRESS1) & (0xFFFFFC00U)) == 0U)
#define IS_I2C_GENERAL_CALL(CALL) (((CALL) == I2C_GENERALCALL_DISABLE) || \
                                   ((CALL) == I2C_GENERALCALL_ENABLE))

#define IS_I2C_NO_STRETCH(STRETCH) (((STRETCH) == I2C_NOSTRETCH_DISABLE) || \
                                    ((STRETCH) == I2C_NOSTRETCH_ENABLE))

#define IS_I2C_MEMADD_SIZE(SIZE) (((SIZE) == I2C_MEMADD_SIZE_8BIT) || \
                                  ((SIZE) == I2C_MEMADD_SIZE_16BIT))


#define I2C_MEM_ADD_MSB(__ADDRESS__)                       ((uint8_t)((uint16_t)(((uint16_t)((__ADDRESS__) & (uint16_t)(0xFF00U))) >> 8U)))
#define I2C_MEM_ADD_LSB(__ADDRESS__)                       ((uint8_t)((uint16_t)((__ADDRESS__) & (uint16_t)(0x00FFU))))


#define I2C_FREQ_RANGE(__PCLK__)                           ((__PCLK__)/1000000U)
#define I2C_RISE_TIME(__FREQRANGE__, __SPEED__)            (((__SPEED__) <= 100000U) ? ((__FREQRANGE__) + 1U) : ((((__FREQRANGE__) * 300U) / 1000U) + 1U))

#define IS_I2C_TRANSFER_OPTIONS_REQUEST(REQUEST)      (((REQUEST) == I2C_FIRST_FRAME)              || \
                                                       ((REQUEST) == I2C_NEXT_FRAME)               || \
                                                       ((REQUEST) == I2C_FIRST_AND_LAST_FRAME)     || \
                                                       ((REQUEST) == I2C_LAST_FRAME))
/**
  * @}
  */ 

/* Private Functions ---------------------------------------------------------*/
/** @defgroup I2C_Private_Functions I2C Private Functions
  * @{
  */
/* Private functions are defined in stm32f0xx_hal_i2c.c file */
/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif


#endif /* __STM32L1xx_HAL_I2C_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/


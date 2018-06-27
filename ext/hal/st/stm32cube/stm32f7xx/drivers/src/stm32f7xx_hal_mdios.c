/**
  ******************************************************************************
  * @file    stm32f7xx_hal_mdios.c
  * @author  MCD Application Team
  * @brief   MDIOS HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities of the MDIOS Peripheral.
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions
  *           
  @verbatim       
 ===============================================================================
                        ##### How to use this driver #####
 ===============================================================================
    [..]
    The MDIOS HAL driver can be used as follows:

    (#) Declare a MDIOS_HandleTypeDef handle structure.

    (#) Initialize the MDIOS low level resources by implementing the HAL_MDIOS_MspInit() API:
        (##) Enable the MDIOS interface clock.
        (##) MDIOS pins configuration:
            (+++) Enable clocks for the MDIOS GPIOs.
            (+++) Configure the MDIOS pins as alternate function.
        (##) NVIC configuration if you need to use interrupt process:
            (+++) Configure the MDIOS interrupt priority.
            (+++) Enable the NVIC MDIOS IRQ handle.

    (#) Program the Port Address and the Preamble Check in the Init structure.

    (#) Initialize the MDIOS registers by calling the HAL_MDIOS_Init() API.

    (#) Perform direct slave read/write operations using the following APIs:
        (##) Read the value of a DINn register: HAL_MDIOS_ReadReg()
        (##) Write a value to a DOUTn register: HAL_MDIOS_WriteReg()

    (#) Get the Master read/write operations flags using the following APIs:
        (##) Bit map of DOUTn registers read by Master: HAL_MDIOS_GetReadRegAddress()
        (##) Bit map of DINn registers written by Master : HAL_MDIOS_GetWrittenRegAddress()

    (#) Clear the read/write flags using the following APIs:
        (##) Clear read flags of a set of registers: HAL_MDIOS_ClearReadRegAddress()
        (##) Clear write flags of a set of registers: HAL_MDIOS_ClearWriteRegAddress()

    (#) Enable interrupts on events using HAL_MDIOS_EnableEvents(), when called
        the MDIOS will generate an interrupt in the following cases: 
        (##) a DINn register written by the Master
        (##) a DOUTn register read by the Master
        (##) an error occur

       -@@- A callback is executed for each genereted interrupt, so the driver provides the following 
            HAL_MDIOS_WriteCpltCallback(), HAL_MDIOS_ReadCpltCallback() and HAL_MDIOS_ErrorCallback()
       -@@- HAL_MDIOS_IRQHandler() must be called from the MDIOS IRQ Handler, to handle the interrupt
            and execute the previous callbacks
   
    (#) Reset the MDIOS peripheral and all related ressources by calling the HAL_MDIOS_DeInit() API.
        (##) HAL_MDIOS_MspDeInit() must be implemented to reset low level ressources 
            (GPIO, Clocks, NVIC configuration ...)


  @endverbatim
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/** @addtogroup STM32F7xx_HAL_Driver
  * @{
  */

/** @defgroup MDIOS MDIOS
  * @brief HAL MDIOS module driver
  * @{
  */
#ifdef HAL_MDIOS_MODULE_ENABLED
    
#if defined (MDIOS)
    
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MDIOS_PORT_ADDRESS_SHIFT        ((uint32_t)8)
#define	MDIOS_ALL_REG_FLAG	        ((uint32_t)0xFFFFFFFFU)
#define	MDIOS_ALL_ERRORS_FLAG           ((uint32_t)(MDIOS_SR_PERF | MDIOS_SR_SERF | MDIOS_SR_TERF))

#define MDIOS_DIN_BASE_ADDR             (MDIOS_BASE + 0x100)
#define MDIOS_DOUT_BASE_ADDR            (MDIOS_BASE + 0x180)

/* Private macro -------------------------------------------------------------*/ 
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup MDIOS_Exported_Functions MDIOS Exported Functions
  * @{
  */

/** @defgroup MDIOS_Exported_Functions_Group1 Initialization/de-initialization functions 
  *  @brief    Initialization and Configuration functions 
  *
@verbatim                                               
===============================================================================
            ##### Initialization and Configuration functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to initialize the MDIOS
      (+) The following parameters can be configured: 
        (++) Port Address
        (++) Preamble Check

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the MDIOS according to the specified parameters in 
  *         the MDIOS_InitTypeDef and creates the associated handle .
  * @param  hmdios pointer to a MDIOS_HandleTypeDef structure that contains
  *         the configuration information for MDIOS module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_Init(MDIOS_HandleTypeDef *hmdios)
{
  uint32_t tmpcr = 0;

  /* Check the MDIOS handle allocation */
  if(hmdios == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Check the parameters */
  assert_param(IS_MDIOS_ALL_INSTANCE(hmdios->Instance));
  assert_param(IS_MDIOS_PORTADDRESS(hmdios->Init.PortAddress));
  assert_param(IS_MDIOS_PREAMBLECHECK(hmdios->Init.PreambleCheck));
  
  /* Process Locked */
  __HAL_LOCK(hmdios);
  
  if(hmdios->State == HAL_MDIOS_STATE_RESET)
  {
    /* Init the low level hardware */
    HAL_MDIOS_MspInit(hmdios);
  }
  
  /* Change the MDIOS state */
  hmdios->State = HAL_MDIOS_STATE_BUSY;
  
  /* Get the MDIOS CR value */
  tmpcr = hmdios->Instance->CR;
  
  /* Clear PORT_ADDRESS, DPC and EN bits */
  tmpcr &= ((uint32_t)~(MDIOS_CR_EN | MDIOS_CR_DPC | MDIOS_CR_PORT_ADDRESS));
  
  /* Set MDIOS control parametrs and enable the peripheral */
  tmpcr |=  (uint32_t)(((hmdios->Init.PortAddress) << MDIOS_PORT_ADDRESS_SHIFT)    |\
                        (hmdios->Init.PreambleCheck) | \
                        (MDIOS_CR_EN));
  
  /* Write the MDIOS CR */
  hmdios->Instance->CR = tmpcr;
  
  /* Change the MDIOS state */
  hmdios->State = HAL_MDIOS_STATE_READY;
  
  /* Release Lock */
  __HAL_UNLOCK(hmdios);
  
  /* Return function status */
  return HAL_OK;

}

/**
  * @brief  DeInitializes the MDIOS peripheral.
  * @param  hmdios MDIOS handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_DeInit(MDIOS_HandleTypeDef *hmdios)
{
  /* Check the MDIOS handle allocation */
  if(hmdios == NULL)
  {
    return HAL_ERROR;
  }
  
  /* Check the parameters */
  assert_param(IS_MDIOS_ALL_INSTANCE(hmdios->Instance));
  
  /* Change the MDIOS state */
  hmdios->State = HAL_MDIOS_STATE_BUSY;
  
  /* Disable the Peripheral */
  __HAL_MDIOS_DISABLE(hmdios);
  
  /* DeInit the low level hardware */
  HAL_MDIOS_MspDeInit(hmdios);
  
  /* Change the MDIOS state */
  hmdios->State = HAL_MDIOS_STATE_RESET;
  
  /* Release Lock */
  __HAL_UNLOCK(hmdios);
  
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  MDIOS MSP Init
  * @param  hmdios mdios handle
  * @retval None
  */
 __weak void HAL_MDIOS_MspInit(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MDIOS_MspInit can be implemented in the user file
   */ 
}

/**
  * @brief  MDIOS MSP DeInit
  * @param  hmdios mdios handle
  * @retval None
  */
 __weak void HAL_MDIOS_MspDeInit(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MDIOS_MspDeInit can be implemented in the user file
   */ 
}
/**
  * @}
  */

/** @defgroup MDIOS_Exported_Functions_Group2 IO operation functions 
  *  @brief MDIOS Read/Write functions 
  *
@verbatim   
 ===============================================================================
                      ##### IO operation functions #####
 ===============================================================================
    This subsection provides a set of functions allowing to manage the MDIOS 
    read and write operations.

    (#) APIs that allow to the MDIOS to read/write from/to the 
        values of one of the DINn/DOUTn registers:
        (+) Read the value of a DINn register: HAL_MDIOS_ReadReg()
        (+) Write a value to a DOUTn register: HAL_MDIOS_WriteReg()

    (#) APIs that provide if there are some Slave registres have been 
        read or written by the Master:
        (+) DOUTn registers read by Master: HAL_MDIOS_GetReadRegAddress()
        (+) DINn registers written by Master : HAL_MDIOS_GetWrittenRegAddress()

    (#) APIs that Clear the read/write flags:
        (+) Clear read registers flags: HAL_MDIOS_ClearReadRegAddress()
        (+) Clear write registers flags: HAL_MDIOS_ClearWriteRegAddress()

    (#) A set of Callbacks are provided:
        (+) HAL_MDIOS_WriteCpltCallback()
        (+) HAL_MDIOS_ReadCpltCallback()
        (+) HAL_MDIOS_ErrorCallback() 

@endverbatim
  * @{
  */

/**
  * @brief  Writes to an MDIOS output register
  * @param  hmdios mdios handle
  * @param  RegNum MDIOS input register number    
  * @param  Data Data to write
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_WriteReg(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum, uint16_t Data)
{
  uint32_t tmpreg;
  
  /* Check the parameters */
  assert_param(IS_MDIOS_REGISTER(RegNum));
      
  /* Process Locked */
  __HAL_LOCK(hmdios);
  
  /* Get the addr of output register to be written by the MDIOS */
  tmpreg = MDIOS_DOUT_BASE_ADDR + (4 * RegNum);
    
  /* Write to DOUTn register */
  *((uint32_t *)tmpreg) = Data;  
        
    /* Process Unlocked */
  __HAL_UNLOCK(hmdios);
        
  return HAL_OK;   
}
      
/**
  * @brief  Reads an MDIOS input register
  * @param  hmdios mdios handle
  * @param  RegNum MDIOS input register number
  * @param  pData pointer to Data
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_ReadReg(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum, uint16_t *pData)
{
  uint32_t tmpreg;
  
  /* Check the parameters */
  assert_param(IS_MDIOS_REGISTER(RegNum));

  /* Process Locked */
  __HAL_LOCK(hmdios);
  
  /* Get the addr of input register to be read by the MDIOS */
  tmpreg = MDIOS_DIN_BASE_ADDR + (4 * RegNum);

  /* Read DINn register */
  *pData = (uint16_t)(*((uint32_t *)tmpreg));

  /* Process Unlocked */
  __HAL_UNLOCK(hmdios);

  return HAL_OK;
}

/**
  * @brief  Gets Written registers by MDIO master
  * @param  hmdios mdios handle
  * @retval bit map of written registers addresses
  */
uint32_t HAL_MDIOS_GetWrittenRegAddress(MDIOS_HandleTypeDef *hmdios)
{        
  return hmdios->Instance->WRFR;   
}

/**
  * @brief  Gets Read registers by MDIO master
  * @param  hmdios mdios handle
  * @retval bit map of read registers addresses
  */
uint32_t HAL_MDIOS_GetReadRegAddress(MDIOS_HandleTypeDef *hmdios)
{        
  return hmdios->Instance->RDFR;   
}

/**
  * @brief  Clears Write registers flag
  * @param  hmdios mdios handle
  * @param  RegNum registers addresses to be cleared
  * @retval HAL status 
  */
HAL_StatusTypeDef HAL_MDIOS_ClearWriteRegAddress(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum)
{
  /* Check the parameters */
  assert_param(IS_MDIOS_REGISTER(RegNum));
  
  /* Process Locked */
  __HAL_LOCK(hmdios);
         
  /* Clear write registers flags */
  hmdios->Instance->CWRFR |= (RegNum);
 
  /* Release Lock */
  __HAL_UNLOCK(hmdios);
  
  return HAL_OK;  
}

/**
  * @brief  Clears Read register flag
  * @param  hmdios mdios handle
  * @param  RegNum registers addresses to be cleared
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_ClearReadRegAddress(MDIOS_HandleTypeDef *hmdios, uint32_t RegNum)
{
  /* Check the parameters */
  assert_param(IS_MDIOS_REGISTER(RegNum));
  
  /* Process Locked */
  __HAL_LOCK(hmdios);
          
  /* Clear read registers flags */
  hmdios->Instance->CRDFR |= (RegNum); 
  
  /* Release Lock */
  __HAL_UNLOCK(hmdios);
  
  return HAL_OK;    
}

/**
  * @brief  Enables Events for MDIOS peripheral 
  * @param  hmdios mdios handle
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_MDIOS_EnableEvents(MDIOS_HandleTypeDef *hmdios)
{        
  /* Process Locked */
  __HAL_LOCK(hmdios);
  
  /* Enable MDIOS interrupts: Register Write, Register Read and Error ITs */
  __HAL_MDIOS_ENABLE_IT(hmdios, (MDIOS_IT_WRITE | MDIOS_IT_READ | MDIOS_IT_ERROR));
  
  /* Process Unlocked */
  __HAL_UNLOCK(hmdios);
    
  return HAL_OK;   
}

/**
  * @brief This function handles MDIOS interrupt request.
  * @param hmdios MDIOS handle
  * @retval None
  */
void HAL_MDIOS_IRQHandler(MDIOS_HandleTypeDef *hmdios)
{
  /* Write Register Interrupt enabled ? */
  if(__HAL_MDIOS_GET_IT_SOURCE(hmdios, MDIOS_IT_WRITE) != RESET)
  {
    /* Write register flag */
    if(HAL_MDIOS_GetWrittenRegAddress(hmdios) != RESET)
    {
      /* Write callback function */
      HAL_MDIOS_WriteCpltCallback(hmdios);
      
      /* Clear write register flag */
      HAL_MDIOS_ClearWriteRegAddress(hmdios, MDIOS_ALL_REG_FLAG);
    }
  }
  
  /* Read Register Interrupt enabled ? */
  if(__HAL_MDIOS_GET_IT_SOURCE(hmdios, MDIOS_IT_READ) != RESET)
  {
    /* Read register flag */
    if(HAL_MDIOS_GetReadRegAddress(hmdios) != RESET)
    {
      /* Read callback function  */
      HAL_MDIOS_ReadCpltCallback(hmdios);
      
      /* Clear read register flag */
      HAL_MDIOS_ClearReadRegAddress(hmdios, MDIOS_ALL_REG_FLAG);
    }
  }
  
  /* Error Interrupt enabled ? */
  if(__HAL_MDIOS_GET_IT_SOURCE(hmdios, MDIOS_IT_ERROR) != RESET)
  {
    /* All Errors Flag */
    if(__HAL_MDIOS_GET_ERROR_FLAG(hmdios, MDIOS_ALL_ERRORS_FLAG) !=RESET)
    {
      /* Error Callback */
      HAL_MDIOS_ErrorCallback(hmdios);
      
      /* Clear errors flag */
      __HAL_MDIOS_CLEAR_ERROR_FLAG(hmdios, MDIOS_ALL_ERRORS_FLAG);
    }
  }
   
  /* check MDIOS WAKEUP exti flag */
  if(__HAL_MDIOS_WAKEUP_EXTI_GET_FLAG() != RESET)
  {
    /* MDIOS WAKEUP interrupt user callback */
    HAL_MDIOS_WakeUpCallback(hmdios);
    
    /* Clear MDIOS WAKEUP Exti pending bit */
    __HAL_MDIOS_WAKEUP_EXTI_CLEAR_FLAG();
  }
}

/**
  * @brief  Write Complete Callback
  * @param  hmdios mdios handle
  * @retval None
  */
 __weak void HAL_MDIOS_WriteCpltCallback(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MDIOS_WriteCpltCallback can be implemented in the user file
   */ 
}

/**
  * @brief  Read Complete Callback
  * @param  hmdios mdios handle
  * @retval None
  */
 __weak void HAL_MDIOS_ReadCpltCallback(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MDIOS_ReadCpltCallback can be implemented in the user file
   */ 
}

/**
  * @brief Error Callback
  * @param hmdios mdios handle
  * @retval None
  */
 __weak void HAL_MDIOS_ErrorCallback(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_MDIOS_ErrorCallback can be implemented in the user file
   */ 
}

/**
  * @brief  MDIOS WAKEUP interrupt callback
  * @param hmdios mdios handle
  * @retval None
  */
__weak void HAL_MDIOS_WakeUpCallback(MDIOS_HandleTypeDef *hmdios)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hmdios);
  
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_MDIOS_WakeUpCallback could be implemented in the user file
   */ 
}

/**
  * @}
  */

/** @defgroup MDIOS_Exported_Functions_Group3 Peripheral Control functions 
  *  @brief   MDIOS control functions 
  *
@verbatim   
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to control the MDIOS.
     (+) HAL_MDIOS_GetState() API, helpful to check in run-time the state. 
     (+) HAL_MDIOS_GetError() API, returns the errors occured during data transfer. 
        
@endverbatim
  * @{
  */

/**
  * @brief  Gets MDIOS error flags 
  * @param  hmdios mdios handle
  * @retval bit map of occured errors 
  */
uint32_t HAL_MDIOS_GetError(MDIOS_HandleTypeDef *hmdios)
{
  /* return errors flags on status register */
  return hmdios->Instance->SR;
}

/**
  * @brief  Return the MDIOS HAL state
  * @param  hmdios mdios handle
  * @retval MDIOS state
  */
HAL_MDIOS_StateTypeDef HAL_MDIOS_GetState(MDIOS_HandleTypeDef *hmdios)
{
  /* Return MDIOS state */
  return hmdios->State;
}

/**
  * @}
  */ 

/**
  * @}
  */ 
#endif /* MDIOS */
#endif /* HAL_MDIOS_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

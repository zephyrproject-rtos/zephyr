/**
  ******************************************************************************
  * @file    stm32l4xx_hal_gfxmmu.c
  * @author  MCD Application Team
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of the Graphic MMU (GFXMMU) peripheral:
  *           + Initialization and De-initialization.
  *           + LUT configuration.
  *           + Modify physical buffer adresses.
  *           + Error management.
  *         
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
  [..]
    *** Initialization ***
    ======================
    [..]
      (#) As prerequisite, fill in the HAL_GFXMMU_MspInit() :
        (++) Enable GFXMMU clock interface with __HAL_RCC_GFXMMU_CLK_ENABLE().
        (++) If interrupts are used, enable and configure GFXMMU global
            interrupt with HAL_NVIC_SetPriority() and HAL_NVIC_EnableIRQ().
      (#) Configure the number of blocks per line, default value, physical 
          buffer addresses and interrupts using the HAL_GFXMMU_Init() function.

    *** LUT configuration ***
    =========================
    [..]
      (#) Use HAL_GFXMMU_DisableLutLines() to deactivate all LUT lines (or a
          range of lines).
      (#) Use HAL_GFXMMU_ConfigLut() to copy LUT from flash to look up RAM.
      (#) Use HAL_GFXMMU_ConfigLutLine() to configure one line of LUT.

    *** Modify physical buffer adresses ***
    =======================================
    [..]    
      (#) Use HAL_GFXMMU_ModifyBuffers() to modify physical buffer addresses.

    *** Error management ***
    ========================
    [..]
      (#) If interrupts are used, HAL_GFXMMU_IRQHandler() will be called when
          an error occurs. This function will call HAL_GFXMMU_ErrorCallback().
          Use HAL_GFXMMU_GetError() to get the error code.

    *** De-initialization ***
    =========================
    [..]    
      (#) As prerequisite, fill in the HAL_GFXMMU_MspDeInit() :
        (++) Disable GFXMMU clock interface with __HAL_RCC_GFXMMU_CLK_ENABLE().
        (++) If interrupts has been used, disable GFXMMU global interrupt with
             HAL_NVIC_DisableIRQ().
      (#) De-initialize GFXMMU using the HAL_GFXMMU_DeInit() function.

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
#include "stm32l4xx_hal.h"

/** @addtogroup STM32L4xx_HAL_Driver
  * @{
  */
#ifdef HAL_GFXMMU_MODULE_ENABLED
#if defined(GFXMMU)
/** @defgroup GFXMMU GFXMMU
  * @brief GFXMMU HAL driver module
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define GFXMMU_LUTXL_FVB_OFFSET     8U
#define GFXMMU_LUTXL_LVB_OFFSET     16U
#define GFXMMU_CR_ITS_MASK          0x1FU
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup GFXMMU_Exported_Functions GFXMMU Exported Functions
  * @{
  */

/** @defgroup GFXMMU_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and de-initialization functions 
 *
@verbatim
  ==============================================================================
          ##### Initialization and de-initialization functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) Initialize the GFXMMU.
      (+) De-initialize the GFXMMU.
@endverbatim
  * @{
  */

/**
  * @brief  Initialize the GFXMMU according to the specified parameters in the
  *         GFXMMU_InitTypeDef structure and initialize the associated handle.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_Init(GFXMMU_HandleTypeDef *hgfxmmu)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check GFXMMU handle */
  if(hgfxmmu == NULL)
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check parameters */
    assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
    assert_param(IS_GFXMMU_BLOCKS_PER_LINE(hgfxmmu->Init.BlocksPerLine));
    assert_param(IS_GFXMMU_BUFFER_ADDRESS(hgfxmmu->Init.Buffers.Buf0Address));
    assert_param(IS_GFXMMU_BUFFER_ADDRESS(hgfxmmu->Init.Buffers.Buf1Address));
    assert_param(IS_GFXMMU_BUFFER_ADDRESS(hgfxmmu->Init.Buffers.Buf2Address));
    assert_param(IS_GFXMMU_BUFFER_ADDRESS(hgfxmmu->Init.Buffers.Buf3Address));
    assert_param(IS_FUNCTIONAL_STATE(hgfxmmu->Init.Interrupts.Activation));
    
    /* Call GFXMMU MSP init function */
    HAL_GFXMMU_MspInit(hgfxmmu);
    
    /* Configure blocks per line and interrupts parameters on GFXMMU_CR register */
    hgfxmmu->Instance->CR &= ~(GFXMMU_CR_B0OIE | GFXMMU_CR_B1OIE | GFXMMU_CR_B2OIE | GFXMMU_CR_B3OIE |
                               GFXMMU_CR_AMEIE | GFXMMU_CR_192BM);
    hgfxmmu->Instance->CR |= (hgfxmmu->Init.BlocksPerLine);
    if(hgfxmmu->Init.Interrupts.Activation == ENABLE)
    {
      assert_param(IS_GFXMMU_INTERRUPTS(hgfxmmu->Init.Interrupts.UsedInterrupts));
      hgfxmmu->Instance->CR |= hgfxmmu->Init.Interrupts.UsedInterrupts;
    }
    
    /* Configure default value on GFXMMU_DVR register */
    hgfxmmu->Instance->DVR = hgfxmmu->Init.DefaultValue;
    
    /* Configure physical buffer adresses on GFXMMU_BxCR registers */
    hgfxmmu->Instance->B0CR = hgfxmmu->Init.Buffers.Buf0Address;
    hgfxmmu->Instance->B1CR = hgfxmmu->Init.Buffers.Buf1Address;
    hgfxmmu->Instance->B2CR = hgfxmmu->Init.Buffers.Buf2Address;
    hgfxmmu->Instance->B3CR = hgfxmmu->Init.Buffers.Buf3Address;
    
    /* Reset GFXMMU error code */
    hgfxmmu->ErrorCode = GFXMMU_ERROR_NONE;
    
    /* Set GFXMMU to ready state */
    hgfxmmu->State = HAL_GFXMMU_STATE_READY;
  }
  /* Return function status */
  return status;
}

/**
  * @brief  De-initialize the GFXMMU.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_DeInit(GFXMMU_HandleTypeDef *hgfxmmu)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check GFXMMU handle */
  if(hgfxmmu == NULL)
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Check parameters */
    assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
    
    /* Disable all interrupts on GFXMMU_CR register */
    hgfxmmu->Instance->CR &= ~(GFXMMU_CR_B0OIE | GFXMMU_CR_B1OIE | GFXMMU_CR_B2OIE | GFXMMU_CR_B3OIE |
                               GFXMMU_CR_AMEIE);
    
    /* Call GFXMMU MSP de-init function */
    HAL_GFXMMU_MspDeInit(hgfxmmu);
    
    /* Set GFXMMU to reset state */
    hgfxmmu->State = HAL_GFXMMU_STATE_RESET;
  }
  /* Return function status */
  return status;
}

/**
  * @brief  Initialize the GFXMMU MSP.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval None.
  */
__weak void HAL_GFXMMU_MspInit(GFXMMU_HandleTypeDef *hgfxmmu)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hgfxmmu);
  
  /* NOTE : This function should not be modified, when the function is needed,
            the HAL_GFXMMU_MspInit could be implemented in the user file.
   */
}

/**
  * @brief  De-initialize the GFXMMU MSP.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval None.
  */
__weak void HAL_GFXMMU_MspDeInit(GFXMMU_HandleTypeDef *hgfxmmu)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hgfxmmu);
  
  /* NOTE : This function should not be modified, when the function is needed,
            the HAL_GFXMMU_MspDeInit could be implemented in the user file.
   */
}

/**
  * @}
  */

/** @defgroup GFXMMU_Exported_Functions_Group2 Operations functions
 *  @brief    GFXMMU operation functions
 *
@verbatim
  ==============================================================================
                      ##### Operation functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure LUT.
      (+) Modify physical buffer adresses.
      (+) Manage error.
@endverbatim
  * @{
  */

/**
  * @brief  This function allows to copy LUT from flash to look up RAM.
  * @param  hgfxmmu : GFXMMU handle.
  * @param  FirstLine : First line enabled on LUT.
  *         This parameter must be a number between Min_Data = 0 and Max_Data = 1023.
  * @param  LinesNumber : Number of lines enabled on LUT.
  *         This parameter must be a number between Min_Data = 1 and Max_Data = 1024.
  * @param  Address : Start address of LUT in flash.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_ConfigLut(GFXMMU_HandleTypeDef *hgfxmmu,
                                       uint32_t FirstLine,
                                       uint32_t LinesNumber,
                                       uint32_t Address)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check parameters */
  assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
  assert_param(IS_GFXMMU_LUT_LINE(FirstLine));
  assert_param(IS_GFXMMU_LUT_LINES_NUMBER(LinesNumber));
  
  /* Check GFXMMU state and coherent parameters */
  if((hgfxmmu->State != HAL_GFXMMU_STATE_READY) || ((FirstLine + LinesNumber) > 1024U))
  {
    status = HAL_ERROR;
  }
  else
  {
    uint32_t current_address, current_line, lutxl_address, lutxh_address;
    
    /* Initialize local variables */
    current_address = Address;
    current_line    = 0U;
    lutxl_address   = (uint32_t) &(hgfxmmu->Instance->LUT[2U * FirstLine]);
    lutxh_address   = (uint32_t) &(hgfxmmu->Instance->LUT[(2U * FirstLine) + 1U]);
    
    /* Copy LUT from flash to look up RAM */
    while(current_line < LinesNumber)
    {
      *((uint32_t *)lutxl_address) = *((uint32_t *)current_address);
      current_address += 4U;
      *((uint32_t *)lutxh_address) = *((uint32_t *)current_address);
      current_address += 4U;
      lutxl_address += 8U;
      lutxh_address += 8U;
      current_line++;
    }
  }
  /* Return function status */
  return status;
}

/**
  * @brief  This function allows to disable a range of LUT lines.
  * @param  hgfxmmu : GFXMMU handle.
  * @param  FirstLine : First line to disable on LUT.
  *         This parameter must be a number between Min_Data = 0 and Max_Data = 1023.
  * @param  LinesNumber : Number of lines to disable on LUT.
  *         This parameter must be a number between Min_Data = 1 and Max_Data = 1024.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_DisableLutLines(GFXMMU_HandleTypeDef *hgfxmmu,
                                             uint32_t FirstLine,
                                             uint32_t LinesNumber)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check parameters */
  assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
  assert_param(IS_GFXMMU_LUT_LINE(FirstLine));
  assert_param(IS_GFXMMU_LUT_LINES_NUMBER(LinesNumber));
  
  /* Check GFXMMU state and coherent parameters */
  if((hgfxmmu->State != HAL_GFXMMU_STATE_READY) || ((FirstLine + LinesNumber) > 1024U))
  {
    status = HAL_ERROR;
  }
  else
  {
    uint32_t current_line, lutxl_address, lutxh_address;
    
    /* Initialize local variables */
    current_line    = 0U;
    lutxl_address   = (uint32_t) &(hgfxmmu->Instance->LUT[2U * FirstLine]);
    lutxh_address   = (uint32_t) &(hgfxmmu->Instance->LUT[(2U * FirstLine) + 1U]);
    
    /* Disable LUT lines */
    while(current_line < LinesNumber)
    {
      *((uint32_t *)lutxl_address) = 0U;
      *((uint32_t *)lutxh_address) = 0U;
      lutxl_address += 8U;
      lutxh_address += 8U;
      current_line++;
    }
  }
  /* Return function status */
  return status;
}

/**
  * @brief  This function allows to configure one line of LUT.
  * @param  hgfxmmu : GFXMMU handle.
  * @param  lutLine : LUT line parameters.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_ConfigLutLine(GFXMMU_HandleTypeDef *hgfxmmu, GFXMMU_LutLineTypeDef *lutLine)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check parameters */
  assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
  assert_param(IS_GFXMMU_LUT_LINE(lutLine->LineNumber));
  assert_param(IS_GFXMMU_LUT_LINE_STATUS(lutLine->LineStatus));
  assert_param(IS_GFXMMU_LUT_BLOCK(lutLine->FirstVisibleBlock));
  assert_param(IS_GFXMMU_LUT_BLOCK(lutLine->LastVisibleBlock));
  assert_param(IS_GFXMMU_LUT_LINE_OFFSET(lutLine->LineOffset));
  
  /* Check GFXMMU state */
  if(hgfxmmu->State != HAL_GFXMMU_STATE_READY)
  {
    status = HAL_ERROR;
  }
  else
  {
    uint32_t lutxl_address, lutxh_address;
    
    /* Initialize local variables */
    lutxl_address   = (uint32_t) &(hgfxmmu->Instance->LUT[2U * lutLine->LineNumber]);
    lutxh_address   = (uint32_t) &(hgfxmmu->Instance->LUT[(2U * lutLine->LineNumber) + 1U]);
    
    /* Configure LUT line */
    if(lutLine->LineStatus == GFXMMU_LUT_LINE_ENABLE)
    {
      /* Enable and configure LUT line */
      *((uint32_t *)lutxl_address) = (lutLine->LineStatus | 
                                     (lutLine->FirstVisibleBlock << GFXMMU_LUTXL_FVB_OFFSET) | 
                                     (lutLine->LastVisibleBlock << GFXMMU_LUTXL_LVB_OFFSET));
      *((uint32_t *)lutxh_address) = lutLine->LineOffset;
    }
    else
    {
      /* Disable LUT line */
      *((uint32_t *)lutxl_address) = 0U;
      *((uint32_t *)lutxh_address) = 0U;
    }
  }
  /* Return function status */
  return status;
}

/**
  * @brief  This function allows to modify physical buffer addresses.
  * @param  hgfxmmu : GFXMMU handle.
  * @param  Buffers : Buffers parameters.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_GFXMMU_ModifyBuffers(GFXMMU_HandleTypeDef *hgfxmmu, GFXMMU_BuffersTypeDef *Buffers)
{
  HAL_StatusTypeDef status = HAL_OK;
  
  /* Check parameters */
  assert_param(IS_GFXMMU_ALL_INSTANCE(hgfxmmu->Instance));
  assert_param(IS_GFXMMU_BUFFER_ADDRESS(Buffers->Buf0Address));
  assert_param(IS_GFXMMU_BUFFER_ADDRESS(Buffers->Buf1Address));
  assert_param(IS_GFXMMU_BUFFER_ADDRESS(Buffers->Buf2Address));
  assert_param(IS_GFXMMU_BUFFER_ADDRESS(Buffers->Buf3Address));
  
  /* Check GFXMMU state */
  if(hgfxmmu->State != HAL_GFXMMU_STATE_READY)
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Modify physical buffer adresses on GFXMMU_BxCR registers */
    hgfxmmu->Instance->B0CR = Buffers->Buf0Address;
    hgfxmmu->Instance->B1CR = Buffers->Buf1Address;
    hgfxmmu->Instance->B2CR = Buffers->Buf2Address;
    hgfxmmu->Instance->B3CR = Buffers->Buf3Address;
  }
  /* Return function status */
  return status;
}

/**
  * @brief  This function handles the GFXMMU interrupts.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval None.
  */
void HAL_GFXMMU_IRQHandler(GFXMMU_HandleTypeDef *hgfxmmu)
{
  uint32_t flags, interrupts, error;
  
  /* Read current flags and interrupts and determine which error occurs */
  flags = hgfxmmu->Instance->SR;
  interrupts = (hgfxmmu->Instance->CR & GFXMMU_CR_ITS_MASK);
  error = (flags & interrupts);
  
  if(error != 0U)
  {
    /* Clear flags on GFXMMU_FCR register */
    hgfxmmu->Instance->FCR = error;
    
    /* Update GFXMMU error code */
    hgfxmmu->ErrorCode |= error;
    
    /* Call GFXMMU error callback */
    HAL_GFXMMU_ErrorCallback(hgfxmmu);
  }
}

/**
  * @brief  Error callback. 
  * @param  hgfxmmu : GFXMMU handle.
  * @retval None.
  */
__weak void HAL_GFXMMU_ErrorCallback(GFXMMU_HandleTypeDef *hgfxmmu)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hgfxmmu);
  
  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_GFXMMU_ErrorCallback could be implemented in the user file.
   */
}

/**
  * @}
  */

/** @defgroup GFXMMU_Exported_Functions_Group3 State functions
 *  @brief    GFXMMU state functions
 *
@verbatim
  ==============================================================================
                         ##### State functions #####
  ==============================================================================
    [..]  This section provides functions allowing to:
      (+) Get GFXMMU handle state.
      (+) Get GFXMMU error code.
@endverbatim
  * @{
  */

/**
  * @brief  This function allows to get the current GFXMMU handle state.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval GFXMMU state.
  */
HAL_GFXMMU_StateTypeDef HAL_GFXMMU_GetState(GFXMMU_HandleTypeDef *hgfxmmu)
{
  /* Return GFXMMU handle state */
  return hgfxmmu->State;
}

/**
  * @brief  This function allows to get the current GFXMMU error code.
  * @param  hgfxmmu : GFXMMU handle.
  * @retval GFXMMU error code.
  */
uint32_t HAL_GFXMMU_GetError(GFXMMU_HandleTypeDef *hgfxmmu)
{
  uint32_t error_code;
  
  /* Enter in critical section */
  __disable_irq();  
  
  /* Store and reset GFXMMU error code */
  error_code = hgfxmmu->ErrorCode;
  hgfxmmu->ErrorCode = GFXMMU_ERROR_NONE;
  
  /* Exit from critical section */
  __enable_irq();
  
  /* Return GFXMMU error code */
  return error_code;
}

/**
  * @}
  */

/**
  * @}
  */
/* End of exported functions -------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* End of private functions --------------------------------------------------*/

/**
  * @}
  */
#endif /* GFXMMU */
#endif /* HAL_GFXMMU_MODULE_ENABLED */
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

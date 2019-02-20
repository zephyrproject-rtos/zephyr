/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_cryp_ex.c
  * @author  MCD Application Team
  * @brief   Extended CRYP HAL module driver
  *          This file provides firmware functions to manage the following
  *          functionalities of CRYP extension peripheral:
  *           + Extended AES processing functions
  *
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The CRYP extension HAL driver can be used after AES-GCM or AES-CCM
    Encryption/Decryption to get the authentication messages.

  @endverbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */
#if defined (CRYP1) || defined (CRYP2)
/** @defgroup CRYPEx CRYPEx
  * @brief CRYP Extension HAL module driver.
  * @{
  */

#ifdef HAL_CRYP_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup CRYPEx_Private_Defines
  * @{
  */

#define CRYP_PHASE_INIT                 0x00000000U
#define CRYP_PHASE_HEADER               CRYP_CR_GCM_CCMPH_0
#define CRYP_PHASE_PAYLOAD              CRYP_CR_GCM_CCMPH_1
#define CRYP_PHASE_FINAL                CRYP_CR_GCM_CCMPH

#define CRYP_OPERATINGMODE_ENCRYPT      0x00000000U
#define CRYP_OPERATINGMODE_DECRYPT      CRYP_CR_ALGODIR

#define  CRYPEx_PHASE_PROCESS       0x02U     /*!< CRYP peripheral is in processing phase */
#define  CRYPEx_PHASE_FINAL         0x03U     /*!< CRYP peripheral is in final phase this is relevant only with CCM and GCM modes */

/*  CTR0 information to use in CCM algorithm */
#define CRYP_CCM_CTR0_0            0x07FFFFFFU
#define CRYP_CCM_CTR0_3            0xFFFFFF00U

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/



/* Exported functions---------------------------------------------------------*/
/** @addtogroup CRYPEx_Exported_Functions
  * @{
  */

/** @defgroup CRYPEx_Exported_Functions_Group1 Extended AES processing functions
  *  @brief    CRYPEx Extended processing functions.
  *
@verbatim
  ==============================================================================
              ##### Extended AES processing functions #####
  ==============================================================================
    [..]  This section provides functions allowing to generate the authentication
          TAG in Polling mode
      (+)HAL_CRYPEx_AESGCM_GenerateAuthTAG
      (+)HAL_CRYPEx_AESCCM_GenerateAuthTAG
         they should be used after Encrypt/Decrypt operation.

@endverbatim
  * @{
  */


/**
  * @brief  generate the GCM authentication TAG.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  AuthTag: Pointer to the authentication buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout)
{
  uint32_t tickstart;
  uint64_t headerlength = (uint64_t)(hcryp->Init.HeaderSize) * 32U; /* Header length in bits */
  uint64_t inputlength = (uint64_t)(hcryp->Size) * 8U; /* input length in bits */
  uint32_t tagaddr = (uint32_t)AuthTag;

  if (hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process locked */
    __HAL_LOCK(hcryp);

    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hcryp->Phase == CRYPEx_PHASE_PROCESS)
    {
      /* Change the CRYP phase */
      hcryp->Phase = CRYPEx_PHASE_FINAL;
    }
    else /* Initialization phase has not been performed*/
    {
      /* Disable the Peripheral */
      __HAL_CRYP_DISABLE(hcryp);

      /* Sequence error code field */
      hcryp->ErrorCode |= HAL_CRYP_ERROR_AUTH_TAG_SEQUENCE;

      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;

      /* Process unlocked */
      __HAL_UNLOCK(hcryp);
      return HAL_ERROR;
    }

    /* Disable CRYP to start the final phase */
    __HAL_CRYP_DISABLE(hcryp);

    /* Select final phase */
    MODIFY_REG(hcryp->Instance->CR, CRYP_CR_GCM_CCMPH, CRYP_PHASE_FINAL);

    /*ALGODIR bit must be set to ‘0’.*/
    hcryp->Instance->CR &=  ~CRYP_CR_ALGODIR;

    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);

    /* Write the number of bits in header (64 bits) followed by the number of bits
    in the payload */
    hcryp->Instance->DIN = 0U;
    hcryp->Instance->DIN = (uint32_t)(headerlength);
    hcryp->Instance->DIN = 0U;
    hcryp->Instance->DIN = (uint32_t)(inputlength);
    /* Wait for OFNE flag to be raised */
    tickstart = HAL_GetTick();
    while (HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_OFNE))
    {
      /* Check for the Timeout */
      if (Timeout != HAL_MAX_DELAY)
      {
        if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U))
        {
          /* Disable the CRYP Peripheral Clock */
          __HAL_CRYP_DISABLE(hcryp);

          /* Change state */
          hcryp->ErrorCode |= HAL_CRYP_ERROR_TIMEOUT;
          hcryp->State = HAL_CRYP_STATE_READY;

          /* Process unlocked */
          __HAL_UNLOCK(hcryp);
          return HAL_ERROR;
        }
      }
    }

    /* Read the authentication TAG in the output FIFO */
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;

    /* Disable the peripheral */
    __HAL_CRYP_DISABLE(hcryp);

    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_READY;

    /* Process unlocked */
    __HAL_UNLOCK(hcryp);
  }
  else
  {
    /* Busy error code field */
    hcryp->ErrorCode |= HAL_CRYP_ERROR_BUSY;
    return HAL_ERROR;
  }
  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  AES CCM Authentication TAG generation.
  * @param  hcryp: pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  AuthTag: Pointer to the authentication buffer
  * @param  Timeout: Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout)
{
  uint32_t tagaddr = (uint32_t)AuthTag;
  uint32_t ctr0 [4] = {0};
  uint32_t ctr0addr = (uint32_t)ctr0;
  uint32_t tickstart;

  if (hcryp->State == HAL_CRYP_STATE_READY)
  {
    /* Process locked */
    __HAL_LOCK(hcryp);

    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_BUSY;

    /* Check if initialization phase has already been performed */
    if (hcryp->Phase == CRYPEx_PHASE_PROCESS)
    {
      /* Change the CRYP phase */
      hcryp->Phase = CRYPEx_PHASE_FINAL;
    }
    else /* Initialization phase has not been performed*/
    {
      /* Disable the peripheral */
      __HAL_CRYP_DISABLE(hcryp);

      /* Sequence error code field */
      hcryp->ErrorCode |= HAL_CRYP_ERROR_AUTH_TAG_SEQUENCE;

      /* Change the CRYP peripheral state */
      hcryp->State = HAL_CRYP_STATE_READY;

      /* Process unlocked */
      __HAL_UNLOCK(hcryp);
      return HAL_ERROR;
    }

    /* Disable CRYP to start the final phase */
    __HAL_CRYP_DISABLE(hcryp);

    /* Select final phase & ALGODIR bit must be set to ‘0’. */
    MODIFY_REG(hcryp->Instance->CR, CRYP_CR_GCM_CCMPH | CRYP_CR_ALGODIR, CRYP_PHASE_FINAL | CRYP_OPERATINGMODE_ENCRYPT);

    /* Enable the CRYP peripheral */
    __HAL_CRYP_ENABLE(hcryp);

    /* Write the counter block in the IN FIFO, CTR0 information from B0
    data has to be swapped according to the DATATYPE*/
    ctr0[0] = (hcryp->Init.B0[0]) & CRYP_CCM_CTR0_0;
    ctr0[1] = hcryp->Init.B0[1];
    ctr0[2] = hcryp->Init.B0[2];
    ctr0[3] = hcryp->Init.B0[3] &  CRYP_CCM_CTR0_3;

    hcryp->Instance->DIN = *(uint32_t *)(ctr0addr);
    ctr0addr += 4U;
    hcryp->Instance->DIN = *(uint32_t *)(ctr0addr);
    ctr0addr += 4U;
    hcryp->Instance->DIN = *(uint32_t *)(ctr0addr);
    ctr0addr += 4U;
    hcryp->Instance->DIN = *(uint32_t *)(ctr0addr);
    /* Wait for OFNE flag to be raised */
    tickstart = HAL_GetTick();
    while (HAL_IS_BIT_CLR(hcryp->Instance->SR, CRYP_FLAG_OFNE))
    {
      /* Check for the Timeout */
      if (Timeout != HAL_MAX_DELAY)
      {
        if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U))
        {
          /* Disable the CRYP peripheral Clock */
          __HAL_CRYP_DISABLE(hcryp);

          /* Change state */
          hcryp->ErrorCode |= HAL_CRYP_ERROR_TIMEOUT;
          hcryp->State = HAL_CRYP_STATE_READY;

          /* Process unlocked */
          __HAL_UNLOCK(hcryp);
          return HAL_ERROR;
        }
      }
    }

    /* Read the Auth TAG in the IN FIFO */
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUT;

    /* Change the CRYP peripheral state */
    hcryp->State = HAL_CRYP_STATE_READY;

    /* Process unlocked */
    __HAL_UNLOCK(hcryp);

    /* Disable CRYP  */
    __HAL_CRYP_DISABLE(hcryp);
  }
  else
  {
    /* Busy error code field */
    hcryp->ErrorCode = HAL_CRYP_ERROR_BUSY;
    return HAL_ERROR;
  }
  /* Return function status */
  return HAL_OK;
}

/**
  * @}
  */


#endif /* HAL_CRYP_MODULE_ENABLED */

/**
  * @}
  */
#endif /* CRYP1 || CRYP2 */
/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

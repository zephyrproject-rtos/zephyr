/**
  ******************************************************************************
  * @file    stm32g0xx_hal_cryp_ex.c
  * @author  MCD Application Team
  * @brief   CRYPEx HAL module driver.
  *          This file provides firmware functions to manage the extended
  *          functionalities of the Cryptography (CRYP) peripheral.
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
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
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup CRYPEx
  * @{
  */

#if defined(AES)

#ifdef HAL_CRYP_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup CRYPEx_Private_Defines
  * @{
  */

#define CRYP_PHASE_INIT                              0x00000000U             /*!< GCM/GMAC (or CCM) init phase */
#define CRYP_PHASE_HEADER                            AES_CR_GCMPH_0          /*!< GCM/GMAC or CCM header phase */
#define CRYP_PHASE_PAYLOAD                           AES_CR_GCMPH_1          /*!< GCM(/CCM) payload phase   */
#define CRYP_PHASE_FINAL                             AES_CR_GCMPH            /*!< GCM/GMAC or CCM  final phase  */

#define CRYP_OPERATINGMODE_ENCRYPT                   0x00000000U             /*!< Encryption mode   */
#define CRYP_OPERATINGMODE_KEYDERIVATION             AES_CR_MODE_0           /*!< Key derivation mode  only used when performing ECB and CBC decryptions  */
#define CRYP_OPERATINGMODE_DECRYPT                   AES_CR_MODE_1           /*!< Decryption       */
#define CRYP_OPERATINGMODE_KEYDERIVATION_DECRYPT     AES_CR_MODE             /*!< Key derivation and decryption only used when performing ECB and CBC decryptions  */

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
 *  @brief   Extended processing functions.
 *
@verbatim
  ==============================================================================
              ##### Extended AES processing functions #####
  ==============================================================================
    [..]  This section provides functions allowing to generate the authentication
          TAG in Polling mode
      (#)HAL_CRYPEx_AESGCM_GenerateAuthTAG
      (#)HAL_CRYPEx_AESCCM_GenerateAuthTAG
         they should be used after Encrypt/Decrypt operation.

@endverbatim
  * @{
  */

/**
  * @brief  generate the GCM authentication TAG.
  * @param  hcryp pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  AuthTag Pointer to the authentication buffer
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESGCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout)
{
  uint32_t tickstart;
  uint64_t headerlength = (uint64_t)hcryp->Init.HeaderSize * 32U; /* Header length in bits */
  uint64_t inputlength = (uint64_t)hcryp->Size * 8U; /* input length in bits */
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

    /* Select final phase */
    MODIFY_REG(hcryp->Instance->CR, AES_CR_GCMPH, CRYP_PHASE_FINAL);

    /* Set the encrypt operating mode*/
    MODIFY_REG(hcryp->Instance->CR, AES_CR_MODE, CRYP_OPERATINGMODE_ENCRYPT);

    /*TinyAES IP from V3.1.1 : data has to be inserted normally (no swapping)*/
    /* Write into the AES_DINR register the number of bits in header (64 bits)
    followed by the number of bits in the payload */

    hcryp->Instance->DINR = 0U;
    hcryp->Instance->DINR = (uint32_t)(headerlength);
    hcryp->Instance->DINR = 0U;
    hcryp->Instance->DINR = (uint32_t)(inputlength);

    /* Wait for CCF flag to be raised */
    tickstart = HAL_GetTick();
    while (HAL_IS_BIT_CLR(hcryp->Instance->SR, AES_SR_CCF))
    {
      /* Check for the Timeout */
      if (Timeout != HAL_MAX_DELAY)
      {
        if (((HAL_GetTick() - tickstart) > Timeout)||(Timeout == 0U))
        {
          /* Disable the CRYP peripheral clock */
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
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;

    /* Clear CCF flag */
    __HAL_CRYP_CLEAR_FLAG(hcryp, CRYP_CCF_CLEAR);

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
  * @param  hcryp pointer to a CRYP_HandleTypeDef structure that contains
  *         the configuration information for CRYP module
  * @param  AuthTag Pointer to the authentication buffer
  * @param  Timeout Timeout duration
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_CRYPEx_AESCCM_GenerateAuthTAG(CRYP_HandleTypeDef *hcryp, uint32_t *AuthTag, uint32_t Timeout)
{
  uint32_t tagaddr = (uint32_t)AuthTag;
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
    /* Select final phase */
    MODIFY_REG(hcryp->Instance->CR, AES_CR_GCMPH, CRYP_PHASE_FINAL);

    /* Set encrypt  operating mode*/
    MODIFY_REG(hcryp->Instance->CR, AES_CR_MODE, CRYP_OPERATINGMODE_ENCRYPT);

    /* Wait for CCF flag to be raised */
    tickstart = HAL_GetTick();
    while (HAL_IS_BIT_CLR(hcryp->Instance->SR, AES_SR_CCF))
    {
      /* Check for the Timeout */
      if (Timeout != HAL_MAX_DELAY)
      {
        if (((HAL_GetTick() - tickstart) > Timeout) ||(Timeout == 0U))
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

    /* Read the authentication TAG in the output FIFO */
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;
    tagaddr += 4U;
    *(uint32_t *)(tagaddr) = hcryp->Instance->DOUTR;

    /* Clear CCF Flag */
    __HAL_CRYP_CLEAR_FLAG(hcryp, CRYP_CCF_CLEAR);


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

/** @defgroup CRYPEx_Exported_Functions_Group2 Extended AES Key Derivations functions
  * @brief   Extended Key Derivations functions.
  *
@verbatim
  ==============================================================================
              ##### Key Derivation functions #####
  ==============================================================================
    [..]  This section provides functions allowing to Enable or Disable the
          the AutoKeyDerivation parameter in CRYP_HandleTypeDef structure
          These function are allowed only in TinyAES IP.
@endverbatim
  * @{
  */

/**
  * @brief  AES enable key derivation functions
  * @param  hcryp pointer to a CRYP_HandleTypeDef structure.
  */
void  HAL_CRYPEx_EnableAutoKeyDerivation(CRYP_HandleTypeDef *hcryp)
{
  if (hcryp->State == HAL_CRYP_STATE_READY)
  {
    hcryp->AutoKeyDerivation = ENABLE;
  }
  else
  {
    /* Busy error code field */
    hcryp->ErrorCode = HAL_CRYP_ERROR_BUSY;
  }
}
/**
  * @brief  AES disable key derivation functions
  * @param  hcryp pointer to a CRYP_HandleTypeDef structure.
  */
void  HAL_CRYPEx_DisableAutoKeyDerivation(CRYP_HandleTypeDef *hcryp)
{
  if (hcryp->State == HAL_CRYP_STATE_READY)
  {
    hcryp->AutoKeyDerivation = DISABLE;
  }
  else
  {
    /* Busy error code field */
    hcryp->ErrorCode = HAL_CRYP_ERROR_BUSY;
  }
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_CRYP_MODULE_ENABLED */

#endif /* AES */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

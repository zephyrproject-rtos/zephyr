/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_dcmi.c
  * @author  MCD Application Team
  * @brief   DCMI HAL module driver
  *          This file provides firmware functions to manage the following
  *          functionalities of the Digital Camera Interface (DCMI) peripheral:
  *           + Initialization and de-initialization functions
  *           + IO operation functions
  *           + Peripheral Control functions
  *           + Peripheral State and Error functions
  *
  @verbatim
  ==============================================================================
                        ##### How to use this driver #####
  ==============================================================================
  [..]
      The sequence below describes how to use this driver to capture image
      from a camera module connected to the DCMI Interface.
      This sequence does not take into account the configuration of the
      camera module, which should be made before to configure and enable
      the DCMI to capture images.

    (#) Program the required configuration through following parameters:
        horizontal and vertical polarity, pixel clock polarity, Capture Rate,
        Synchronization Mode, code of the frame delimiter and data width
        using HAL_DCMI_Init() function.

    (#) Configure the selected DMA stream to transfer Data from DCMI DR
        register to the destination memory buffer.

    (#) Program the required configuration through following parameters:
        DCMI mode, destination memory Buffer address and the data length
        and enable capture using HAL_DCMI_Start_DMA() function.

    (#) Optionally, configure and Enable the CROP feature to select a rectangular
        window from the received image using HAL_DCMI_ConfigCrop()
        and HAL_DCMI_EnableCrop() functions

    (#) The capture can be stopped using HAL_DCMI_Stop() function.

    (#) To control DCMI state you can use the function HAL_DCMI_GetState().

     *** Callback registration ***
     =============================================

     The compilation flag USE_HAL_DCMI_REGISTER_CALLBACKS when set to 1
     allows the user to configure dynamically the driver callbacks.
     Use Functions HAL_DCMI_RegisterCallback() to register an interrupt callback.

     Function HAL_DCMI_RegisterCallback() allows to register following callbacks:
       (+) LineEventCallback    : callback for DCMI line event.
       (+) FrameEventCallback   : callback for DCMI Frame event.
       (+) VsyncEventCallback   : callback for DCMI Vsync event.
       (+) ErrorCallback        : callback for error detection.
       (+) MspInitCallback      : callback for Msp Init.
       (+) MspDeInitCallback    : callback for Msp DeInit.
     This function takes as parameters the HAL peripheral handle, the Callback ID
     and a pointer to the user callback function.

     Use function HAL_DCMI_UnRegisterCallback to reset a callback to the default weak function.
     HAL_DCMI_UnRegisterCallback takes as parameters the HAL peripheral handle and the Callback ID.
     This function allows to reset following callbacks:
       (+) LineEventCallback    : callback for DCMI line event.
       (+) FrameEventCallback   : callback for DCMI Frame event.
       (+) VsyncEventCallback   : callback for DCMI Vsync event.
       (+) ErrorCallback        : callback for error detection.
       (+) MspInitCallback      : callback for Msp Init.
       (+) MspDeInitCallback    : callback for Msp DeInit.

     By default, after the HAL_DCMI_Init() and when the state is HAL_DCMI_STATE_RESET
     all callbacks are set to the corresponding weak functions:
     examples HAL_DCMI_LineEventCallback(), HAL_DCMI_FrameEventCallback().
     Exception done for MspInit and MspDeInit functions that are
     reset to the legacy weak functions in the HAL_DCMI_Init()/ HAL_DCMI_DeInit() only when
     these callbacks are null (not registered beforehand).
     If MspInit or MspDeInit are not null, the HAL_DCMI_Init()/ HAL_DCMI_DeInit()
     keep and use the user MspInit/MspDeInit callbacks (registered beforehand) whatever the state.

     Callbacks can be registered/unregistered in HAL_DCMI_STATE_READY state only.
     Exception done MspInit/MspDeInit functions that can be registered/unregistered
     in HAL_DCMI_STATE_READY or HAL_DCMI_STATE_RESET state,
     thus registered (user) MspInit/DeInit callbacks can be used during the Init/DeInit.
     In that case first register the MspInit/MspDeInit user callbacks
     using HAL_DCMI_RegisterCallback() before calling HAL_DCMI_DeInit() or HAL_DCMI_Init() function.

     When the compilation flag USE_HAL_DCMI_REGISTER_CALLBACKS is set to 0 or
     not defined, the callback registration feature is not available and all callbacks
     are set to the corresponding weak functions.

     *** DCMI HAL driver macros list ***
     =============================================
     [..]
       Below the list of most used macros in DCMI HAL driver.

      (+) __HAL_DCMI_ENABLE: Enable the DCMI peripheral.
      (+) __HAL_DCMI_DISABLE: Disable the DCMI peripheral.
      (+) __HAL_DCMI_GET_FLAG: Get the DCMI pending flags.
      (+) __HAL_DCMI_CLEAR_FLAG: Clear the DCMI pending flags.
      (+) __HAL_DCMI_ENABLE_IT: Enable the specified DCMI interrupts.
      (+) __HAL_DCMI_DISABLE_IT: Disable the specified DCMI interrupts.
      (+) __HAL_DCMI_GET_IT_SOURCE: Check whether the specified DCMI interrupt has occurred or not.

     [..]
       (@) You can refer to the DCMI HAL driver header file for more useful macros

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
/** @defgroup DCMI DCMI
  * @brief DCMI HAL module driver
  * @{
  */

#ifdef HAL_DCMI_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define HAL_TIMEOUT_DCMI_STOP    ((uint32_t)1000) /* Set timeout to 1s  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void       DCMI_DMAXferCplt(DMA_HandleTypeDef *hdma);
static void       DCMI_DMAError(DMA_HandleTypeDef *hdma);

/* Exported functions --------------------------------------------------------*/

/** @defgroup DCMI_Exported_Functions DCMI Exported Functions
  * @{
  */

/** @defgroup DCMI_Exported_Functions_Group1 Initialization and Configuration functions
 *  @brief   Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
                ##### Initialization and Configuration functions #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Initialize and configure the DCMI
      (+) De-initialize the DCMI

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the DCMI according to the specified
  *         parameters in the DCMI_InitTypeDef and create the associated handle.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_Init(DCMI_HandleTypeDef *hdcmi)
{
  /* Check the DCMI peripheral state */
  if(hdcmi == NULL)
  {
     return HAL_ERROR;
  }

  /* Check function parameters */
  assert_param(IS_DCMI_ALL_INSTANCE(hdcmi->Instance));
  assert_param(IS_DCMI_PCKPOLARITY(hdcmi->Init.PCKPolarity));
  assert_param(IS_DCMI_VSPOLARITY(hdcmi->Init.VSPolarity));
  assert_param(IS_DCMI_HSPOLARITY(hdcmi->Init.HSPolarity));
  assert_param(IS_DCMI_SYNCHRO(hdcmi->Init.SynchroMode));
  assert_param(IS_DCMI_CAPTURE_RATE(hdcmi->Init.CaptureRate));
  assert_param(IS_DCMI_EXTENDED_DATA(hdcmi->Init.ExtendedDataMode));
  assert_param(IS_DCMI_MODE_JPEG(hdcmi->Init.JPEGMode));

  assert_param(IS_DCMI_BYTE_SELECT_MODE(hdcmi->Init.ByteSelectMode));
  assert_param(IS_DCMI_BYTE_SELECT_START(hdcmi->Init.ByteSelectStart));
  assert_param(IS_DCMI_LINE_SELECT_MODE(hdcmi->Init.LineSelectMode));
  assert_param(IS_DCMI_LINE_SELECT_START(hdcmi->Init.LineSelectStart));

#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
  if(hdcmi->State == HAL_DCMI_STATE_RESET)
  {
    /* Reset callback pointers to the weak predefined callbacks */
    hdcmi->LineEventCallback = HAL_DCMI_LineEventCallback;
    hdcmi->FrameEventCallback = HAL_DCMI_FrameEventCallback;
    hdcmi->VsyncEventCallback = HAL_DCMI_VsyncEventCallback;
    hdcmi->ErrorCallback = HAL_DCMI_ErrorCallback;

    if(hdcmi->MspInitCallback == NULL)
    {
      hdcmi->MspInitCallback = HAL_DCMI_MspInit;
    }
    /* Init the low level hardware */
    hdcmi->MspInitCallback(hdcmi);
  }

#else
  if(hdcmi->State == HAL_DCMI_STATE_RESET)
  {
    /* Init the low level hardware */
    HAL_DCMI_MspInit(hdcmi);
  }
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */

  /* Change the DCMI state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;
                          /* Configures the HS, VS, DE and PC polarity */
  hdcmi->Instance->CR &= ~(DCMI_CR_PCKPOL | DCMI_CR_HSPOL  | DCMI_CR_VSPOL  | DCMI_CR_EDM_0 |\
                           DCMI_CR_EDM_1  | DCMI_CR_FCRC_0 | DCMI_CR_FCRC_1 | DCMI_CR_JPEG  |\
                           DCMI_CR_ESS | DCMI_CR_BSM_0 | DCMI_CR_BSM_1 | DCMI_CR_OEBS |\
                           DCMI_CR_LSM | DCMI_CR_OELS);

  hdcmi->Instance->CR |=  (uint32_t)(hdcmi->Init.SynchroMode | hdcmi->Init.CaptureRate |\
                                     hdcmi->Init.VSPolarity  | hdcmi->Init.HSPolarity  |\
                                     hdcmi->Init.PCKPolarity | hdcmi->Init.ExtendedDataMode |\
                                     hdcmi->Init.JPEGMode | hdcmi->Init.ByteSelectMode |\
                                     hdcmi->Init.ByteSelectStart | hdcmi->Init.LineSelectMode |\
                                     hdcmi->Init.LineSelectStart);

  if(hdcmi->Init.SynchroMode == DCMI_SYNCHRO_EMBEDDED)
  {
    hdcmi->Instance->ESCR = (((uint32_t)hdcmi->Init.SyncroCode.FrameStartCode)    |\
                             ((uint32_t)hdcmi->Init.SyncroCode.LineStartCode << DCMI_ESCR_LSC_Pos)|\
                             ((uint32_t)hdcmi->Init.SyncroCode.LineEndCode << DCMI_ESCR_LEC_Pos) |\
                             ((uint32_t)hdcmi->Init.SyncroCode.FrameEndCode << DCMI_ESCR_FEC_Pos));

  }

  /* Enable the Line, Vsync, Error and Overrun interrupts */
  __HAL_DCMI_ENABLE_IT(hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_ERR | DCMI_IT_OVR);

  /* Update error code */
  hdcmi->ErrorCode = HAL_DCMI_ERROR_NONE;

  /* Initialize the DCMI state*/
  hdcmi->State  = HAL_DCMI_STATE_READY;

  return HAL_OK;
}

/**
  * @brief  Deinitializes the DCMI peripheral registers to their default reset
  *         values.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */

HAL_StatusTypeDef HAL_DCMI_DeInit(DCMI_HandleTypeDef *hdcmi)
{
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
  if(hdcmi->MspDeInitCallback == NULL)
  {
    hdcmi->MspDeInitCallback = HAL_DCMI_MspDeInit;
  }

  /* DeInit the low level hardware */
  hdcmi->MspDeInitCallback(hdcmi);
#else
  /* DeInit the low level hardware */
  HAL_DCMI_MspDeInit(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */

  /* Update error code */
  hdcmi->ErrorCode = HAL_DCMI_ERROR_NONE;

  /* Initialize the DCMI state*/
  hdcmi->State = HAL_DCMI_STATE_RESET;

  /* Release Lock */
  __HAL_UNLOCK(hdcmi);

  return HAL_OK;
}

#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
/**
  * @brief  Register a User DCMI Callback
  *         To be used instead of the weak predefined callback
  * @param  hdcmi DCMI handle
  * @param  CallbackID ID of the callback to be registered
  *         This parameter can be one of the following values:
  *          @arg @ref HAL_DCMI_LINE_EVENT_CB_ID Line Event callback ID
  *          @arg @ref HAL_DCMI_FRAME_EVENT_CB_ID Frame Event callback ID
  *          @arg @ref HAL_DCMI_VSYNC_EVENT_CB_ID Vsync Event callback ID
  *          @arg @ref HAL_DCMI_ERROR_CB_ID Error callback ID
  *          @arg @ref HAL_DCMI_MSPINIT_CB_ID MspInit callback ID
  *          @arg @ref HAL_DCMI_MSPDEINIT_CB_ID MspDeInit callback ID
  * @param  pCallback pointer to the Callback function
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_RegisterCallback(DCMI_HandleTypeDef *hdcmi, HAL_DCMI_CallbackIDTypeDef CallbackID, pDCMI_CallbackTypeDef pCallback)
{
  HAL_StatusTypeDef status = HAL_OK;

  if(pCallback == NULL)
  {
    /* Update the error code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
    return HAL_ERROR;
  }
  /* Process locked */
  __HAL_LOCK(hdcmi);

  if(HAL_DCMI_STATE_READY == hdcmi->State)
  {
    switch (CallbackID)
    {
    case HAL_DCMI_LINE_EVENT_CB_ID :
      hdcmi->LineEventCallback = pCallback;
      break;

    case HAL_DCMI_FRAME_EVENT_CB_ID :
      hdcmi->FrameEventCallback = pCallback;
      break;

    case HAL_DCMI_VSYNC_EVENT_CB_ID :
      hdcmi->VsyncEventCallback = pCallback;
      break;

    case HAL_DCMI_ERROR_CB_ID :
      hdcmi->ErrorCallback = pCallback;
      break;

    case HAL_DCMI_MSPINIT_CB_ID :
      hdcmi->MspInitCallback = pCallback;
      break;

    case HAL_DCMI_MSPDEINIT_CB_ID :
      hdcmi->MspDeInitCallback = pCallback;
      break;

    default :
      /* Update the error code */
      hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
     /* Return error status */
      status =  HAL_ERROR;
      break;
    }
  }
  else if(HAL_DCMI_STATE_RESET == hdcmi->State)
  {
    switch (CallbackID)
    {
    case HAL_DCMI_MSPINIT_CB_ID :
      hdcmi->MspInitCallback = pCallback;
      break;

    case HAL_DCMI_MSPDEINIT_CB_ID :
      hdcmi->MspDeInitCallback = pCallback;
      break;

    default :
      /* Update the error code */
      hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
     /* Return error status */
      status =  HAL_ERROR;
      break;
    }
  }
  else
  {
    /* Update the error code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
    /* Return error status */
    status =  HAL_ERROR;
  }

  /* Release Lock */
  __HAL_UNLOCK(hdcmi);
  return status;
}

/**
  * @brief  Unregister a DCMI Callback
  *         DCMI callabck is redirected to the weak predefined callback
  * @param  hdcmi DCMI handle
  * @param  CallbackID ID of the callback to be registered
  *         This parameter can be one of the following values:
  *          @arg @ref HAL_DCMI_LINE_EVENT_CB_ID Line Event callback ID
  *          @arg @ref HAL_DCMI_FRAME_EVENT_CB_ID Frame Event callback ID
  *          @arg @ref HAL_DCMI_VSYNC_EVENT_CB_ID Vsync Event callback ID
  *          @arg @ref HAL_DCMI_ERROR_CB_ID Error callback ID
  *          @arg @ref HAL_DCMI_MSPINIT_CB_ID MspInit callback ID
  *          @arg @ref HAL_DCMI_MSPDEINIT_CB_ID MspDeInit callback ID
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_UnRegisterCallback(DCMI_HandleTypeDef *hdcmi, HAL_DCMI_CallbackIDTypeDef CallbackID)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Process locked */
  __HAL_LOCK(hdcmi);

  if(HAL_DCMI_STATE_READY == hdcmi->State)
  {
    switch (CallbackID)
    {
    case HAL_DCMI_LINE_EVENT_CB_ID :
      hdcmi->LineEventCallback = HAL_DCMI_LineEventCallback;
      break;

    case HAL_DCMI_FRAME_EVENT_CB_ID :
      hdcmi->FrameEventCallback = HAL_DCMI_FrameEventCallback;
      break;

    case HAL_DCMI_VSYNC_EVENT_CB_ID :
      hdcmi->VsyncEventCallback = HAL_DCMI_VsyncEventCallback;
      break;

    case HAL_DCMI_ERROR_CB_ID :
      hdcmi->ErrorCallback = HAL_DCMI_ErrorCallback;
      break;

    case HAL_DCMI_MSPINIT_CB_ID :
      hdcmi->MspInitCallback = HAL_DCMI_MspInit;
      break;

    case HAL_DCMI_MSPDEINIT_CB_ID :
      hdcmi->MspDeInitCallback = HAL_DCMI_MspDeInit;
      break;

    default :
      /* Update the error code */
      hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
     /* Return error status */
      status =  HAL_ERROR;
      break;
    }
  }
  else if(HAL_DCMI_STATE_RESET == hdcmi->State)
  {
    switch (CallbackID)
    {
    case HAL_DCMI_MSPINIT_CB_ID :
      hdcmi->MspInitCallback = HAL_DCMI_MspInit;
      break;

    case HAL_DCMI_MSPDEINIT_CB_ID :
      hdcmi->MspDeInitCallback = HAL_DCMI_MspDeInit;
      break;

    default :
      /* Update the error code */
      hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
     /* Return error status */
      status =  HAL_ERROR;
      break;
    }
  }
  else
  {
    /* Update the error code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_INVALID_CALLBACK;
    /* Return error status */
    status =  HAL_ERROR;
  }

  /* Release Lock */
  __HAL_UNLOCK(hdcmi);
  return status;
}
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
/**
  * @brief  Initializes the DCMI MSP.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_MspInit(DCMI_HandleTypeDef* hdcmi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmi);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_MspInit could be implemented in the user file
   */
}

/**
  * @brief  DeInitializes the DCMI MSP.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_MspDeInit(DCMI_HandleTypeDef* hdcmi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmi);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_MspDeInit could be implemented in the user file
   */
}

/**
  * @}
  */
/** @defgroup DCMI_Exported_Functions_Group2 IO operation functions
 *  @brief   IO operation functions
 *
@verbatim
 ===============================================================================
                      #####  IO operation functions  #####
 ===============================================================================
    [..]  This section provides functions allowing to:
      (+) Configure destination address and data length and
          Enables DCMI DMA request and enables DCMI capture
      (+) Stop the DCMI capture.
      (+) Handles DCMI interrupt request.

@endverbatim
  * @{
  */

/**
  * @brief  Enables DCMI DMA request and enables DCMI capture
  * @param  hdcmi     pointer to a DCMI_HandleTypeDef structure that contains
  *                   the configuration information for DCMI.
  * @param  DCMI_Mode DCMI capture mode snapshot or continuous grab.
  * @param  pData     The destination memory Buffer address (LCD Frame buffer).
  * @param  Length    The length of capture to be transferred.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_Start_DMA(DCMI_HandleTypeDef* hdcmi, uint32_t DCMI_Mode, uint32_t pData, uint32_t Length)
{
  /* Initialize the second memory address */
  uint32_t SecondMemAddress;

  /* Check function parameters */
  assert_param(IS_DCMI_CAPTURE_MODE(DCMI_Mode));

  /* Process Locked */
  __HAL_LOCK(hdcmi);

  /* Lock the DCMI peripheral state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;

  /* Enable DCMI by setting DCMIEN bit */
  __HAL_DCMI_ENABLE(hdcmi);

  /* Configure the DCMI Mode */
  hdcmi->Instance->CR &= ~(DCMI_CR_CM);
  hdcmi->Instance->CR |=  (uint32_t)(DCMI_Mode);

  /* Set the DMA memory0 conversion complete callback */
  hdcmi->DMA_Handle->XferCpltCallback = DCMI_DMAXferCplt;

  /* Set the DMA error callback */
  hdcmi->DMA_Handle->XferErrorCallback = DCMI_DMAError;

  /* Set the dma abort callback */
  hdcmi->DMA_Handle->XferAbortCallback = NULL;

  /* Reset transfer counters value */
  hdcmi->XferCount = 0;
  hdcmi->XferTransferNumber = 0;

  if(Length <= 0xFFFFU)
  {
    /* Enable the DMA Stream */
    if (HAL_DMA_Start_IT(hdcmi->DMA_Handle, (uint32_t)&hdcmi->Instance->DR, (uint32_t)pData, Length) != HAL_OK)
    {
      /* Set Error Code */
      hdcmi->ErrorCode = HAL_DCMI_ERROR_DMA;
      /* Change DCMI state */
      hdcmi->State = HAL_DCMI_STATE_READY;
      /* Release Lock */
      __HAL_UNLOCK(hdcmi);
      /* Return function status */
      return HAL_ERROR;
    }
  }
  else /* DCMI_DOUBLE_BUFFER Mode */
  {
    /* Set the DMA memory1 conversion complete callback */
    hdcmi->DMA_Handle->XferM1CpltCallback = DCMI_DMAXferCplt;

    /* Initialize transfer parameters */
    hdcmi->XferCount = 1;
    hdcmi->XferSize = Length;
    hdcmi->pBuffPtr = pData;

    /* Get the number of buffer */
    while(hdcmi->XferSize > 0xFFFFU)
    {
      hdcmi->XferSize = (hdcmi->XferSize/2U);
      hdcmi->XferCount = hdcmi->XferCount*2U;
    }

    /* Update DCMI counter  and transfer number*/
    hdcmi->XferCount = (hdcmi->XferCount - 2U);
    hdcmi->XferTransferNumber = hdcmi->XferCount;

    /* Update second memory address */
    SecondMemAddress = (uint32_t)(pData + (4U*hdcmi->XferSize));

    /* Start DMA multi buffer transfer */
    if (HAL_DMAEx_MultiBufferStart_IT(hdcmi->DMA_Handle, (uint32_t)&hdcmi->Instance->DR, (uint32_t)pData, SecondMemAddress, hdcmi->XferSize) != HAL_OK)
    {
      /* Set Error Code */
      hdcmi->ErrorCode = HAL_DCMI_ERROR_DMA;
      /* Change DCMI state */
      hdcmi->State = HAL_DCMI_STATE_READY;
      /* Release Lock */
      __HAL_UNLOCK(hdcmi);
      /* Return function status */
      return HAL_ERROR;
    }
  }

  /* Enable Capture */
  hdcmi->Instance->CR |= DCMI_CR_CAPTURE;

  /* Release Lock */
  __HAL_UNLOCK(hdcmi);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Disable DCMI DMA request and Disable DCMI capture
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_Stop(DCMI_HandleTypeDef* hdcmi)
{
  register uint32_t count = HAL_TIMEOUT_DCMI_STOP * (SystemCoreClock /8U/1000U);
  HAL_StatusTypeDef status = HAL_OK;

  /* Process locked */
  __HAL_LOCK(hdcmi);

  /* Lock the DCMI peripheral state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;

  /* Disable Capture */
  hdcmi->Instance->CR &= ~(DCMI_CR_CAPTURE);

  /* Check if the DCMI capture effectively disabled */
  do
  {
    if (count-- == 0U)
    {
      /* Update error code */
      hdcmi->ErrorCode |= HAL_DCMI_ERROR_TIMEOUT;

      status = HAL_TIMEOUT;
      break;
    }
  }
  while((hdcmi->Instance->CR & DCMI_CR_CAPTURE) != 0U);

  /* Disable the DCMI */
  __HAL_DCMI_DISABLE(hdcmi);

  /* Disable the DMA */
  (void)HAL_DMA_Abort(hdcmi->DMA_Handle);

  /* Update error code */
  hdcmi->ErrorCode |= HAL_DCMI_ERROR_NONE;

  /* Change DCMI state */
  hdcmi->State = HAL_DCMI_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  /* Return function status */
  return status;
}

/**
  * @brief  Suspend DCMI capture
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_Suspend(DCMI_HandleTypeDef* hdcmi)
{
  register uint32_t count = HAL_TIMEOUT_DCMI_STOP * (SystemCoreClock /8U/1000U);
  HAL_StatusTypeDef status = HAL_OK;

  /* Process locked */
  __HAL_LOCK(hdcmi);

  if(hdcmi->State == HAL_DCMI_STATE_BUSY)
  {
    /* Change DCMI state */
    hdcmi->State = HAL_DCMI_STATE_SUSPENDED;

    /* Disable Capture */
    hdcmi->Instance->CR &= ~(DCMI_CR_CAPTURE);

    /* Check if the DCMI capture effectively disabled */
    do
    {
      if (count-- == 0U)
      {
        /* Update error code */
        hdcmi->ErrorCode |= HAL_DCMI_ERROR_TIMEOUT;

        /* Change DCMI state */
        hdcmi->State = HAL_DCMI_STATE_READY;

        status = HAL_TIMEOUT;
        break;
      }
    }
    while((hdcmi->Instance->CR & DCMI_CR_CAPTURE) != 0U);
  }
  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  /* Return function status */
  return status;
}

/**
  * @brief  Resume DCMI capture
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_Resume(DCMI_HandleTypeDef* hdcmi)
{
  /* Process locked */
  __HAL_LOCK(hdcmi);

  if(hdcmi->State == HAL_DCMI_STATE_SUSPENDED)
  {
    /* Change DCMI state */
    hdcmi->State = HAL_DCMI_STATE_BUSY;

    /* Disable Capture */
    hdcmi->Instance->CR |= DCMI_CR_CAPTURE;
  }
  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Handles DCMI interrupt request.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for the DCMI.
  * @retval None
  */
void HAL_DCMI_IRQHandler(DCMI_HandleTypeDef *hdcmi)
{
  uint32_t isr_value = READ_REG(hdcmi->Instance->MISR);

  /* Synchronization error interrupt management *******************************/
  if((isr_value & DCMI_FLAG_ERRRI) == DCMI_FLAG_ERRRI)
  {
    /* Clear the Synchronization error flag */
    __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_ERRRI);

    /* Update error code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_SYNC;

    /* Change DCMI state */
    hdcmi->State = HAL_DCMI_STATE_ERROR;

    /* Set the synchronization error callback */
    hdcmi->DMA_Handle->XferAbortCallback = DCMI_DMAError;

    /* Abort the DMA Transfer */
    (void)HAL_DMA_Abort_IT(hdcmi->DMA_Handle);
  }
  /* Overflow interrupt management ********************************************/
  if((isr_value & DCMI_FLAG_OVRRI) == DCMI_FLAG_OVRRI)
  {
    /* Clear the Overflow flag */
    __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_OVRRI);

    /* Update error code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_OVR;

    /* Change DCMI state */
    hdcmi->State = HAL_DCMI_STATE_ERROR;

    /* Set the overflow callback */
    hdcmi->DMA_Handle->XferAbortCallback = DCMI_DMAError;

    /* Abort the DMA Transfer */
    (void)HAL_DMA_Abort_IT(hdcmi->DMA_Handle);
  }
  /* Line Interrupt management ************************************************/
  if((isr_value & DCMI_FLAG_LINERI) == DCMI_FLAG_LINERI)
  {
    /* Clear the Line interrupt flag */
    __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_LINERI);

    /* Line interrupt Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
    hdcmi->LineEventCallback(hdcmi);
#else
    HAL_DCMI_LineEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */

  }
  /* VSYNC interrupt management ***********************************************/
  if((isr_value & DCMI_FLAG_VSYNCRI) == DCMI_FLAG_VSYNCRI)
  {
    /* Clear the VSYNC flag */
    __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_VSYNCRI);

    /* VSYNC Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
    hdcmi->VsyncEventCallback(hdcmi);
#else
    HAL_DCMI_VsyncEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
  }
  /* FRAME interrupt management ***********************************************/
  if((isr_value & DCMI_FLAG_FRAMERI) == DCMI_FLAG_FRAMERI)
  {
    /* When snapshot mode, disable Vsync, Error and Overrun interrupts */
    if((hdcmi->Instance->CR & DCMI_CR_CM) == DCMI_MODE_SNAPSHOT)
    {
      /* Disable the Line, Vsync, Error and Overrun interrupts */
      __HAL_DCMI_DISABLE_IT(hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_ERR | DCMI_IT_OVR);
    }

    /* Disable the Frame interrupt */
    __HAL_DCMI_DISABLE_IT(hdcmi, DCMI_IT_FRAME);

    /* Clear the End of Frame flag */
    __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_FRAMERI);

    /* Frame Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
    hdcmi->FrameEventCallback(hdcmi);
#else
    HAL_DCMI_FrameEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */

  }
}

/**
  * @brief  Error DCMI callback.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmi);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_ErrorCallback could be implemented in the user file
   */
}

/**
  * @brief  Line Event callback.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_LineEventCallback could be implemented in the user file
   */
}

/**
  * @brief  VSYNC Event callback.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmi);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_VsyncEventCallback could be implemented in the user file
   */
}

/**
  * @brief  Frame Event callback.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval None
  */
__weak void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hdcmi);

  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_DCMI_FrameEventCallback could be implemented in the user file
   */
}

/**
  * @}
  */

/** @defgroup DCMI_Exported_Functions_Group3 Peripheral Control functions
 *  @brief    Peripheral Control functions
 *
@verbatim
 ===============================================================================
                    ##### Peripheral Control functions #####
 ===============================================================================
[..]  This section provides functions allowing to:
      (+) Configure the CROP feature.
      (+) Enable/Disable the CROP feature.

@endverbatim
  * @{
  */

/**
  * @brief  Configure the DCMI CROP coordinate.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @param  YSize DCMI Line number
  * @param  XSize DCMI Pixel per line
  * @param  X0    DCMI window X offset
  * @param  Y0    DCMI window Y offset
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_ConfigCrop(DCMI_HandleTypeDef *hdcmi, uint32_t X0, uint32_t Y0, uint32_t XSize, uint32_t YSize)
{
  /* Process Locked */
  __HAL_LOCK(hdcmi);

  /* Lock the DCMI peripheral state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;

  /* Check the parameters */
  assert_param(IS_DCMI_WINDOW_COORDINATE(X0));
  assert_param(IS_DCMI_WINDOW_HEIGHT(Y0));
  assert_param(IS_DCMI_WINDOW_COORDINATE(XSize));
  assert_param(IS_DCMI_WINDOW_COORDINATE(YSize));

  /* Configure CROP */
  hdcmi->Instance->CWSIZER = (XSize | (YSize << DCMI_CWSIZE_VLINE_Pos));
  hdcmi->Instance->CWSTRTR = (X0 | (Y0 << DCMI_CWSTRT_VST_Pos));

  /* Initialize the DCMI state*/
  hdcmi->State  = HAL_DCMI_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  return HAL_OK;
}

/**
  * @brief  Disable the Crop feature.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_DisableCrop(DCMI_HandleTypeDef *hdcmi)
{
  /* Process Locked */
  __HAL_LOCK(hdcmi);

  /* Lock the DCMI peripheral state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;

  /* Disable DCMI Crop feature */
  hdcmi->Instance->CR &= ~(uint32_t)DCMI_CR_CROP;

  /* Change the DCMI state*/
  hdcmi->State = HAL_DCMI_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  return HAL_OK;
}

/**
  * @brief  Enable the Crop feature.
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DCMI_EnableCrop(DCMI_HandleTypeDef *hdcmi)
{
  /* Process Locked */
  __HAL_LOCK(hdcmi);

  /* Lock the DCMI peripheral state */
  hdcmi->State = HAL_DCMI_STATE_BUSY;

  /* Enable DCMI Crop feature */
  hdcmi->Instance->CR |= (uint32_t)DCMI_CR_CROP;

  /* Change the DCMI state*/
  hdcmi->State = HAL_DCMI_STATE_READY;

  /* Process Unlocked */
  __HAL_UNLOCK(hdcmi);

  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup DCMI_Exported_Functions_Group4 Peripheral State functions
 *  @brief    Peripheral State functions
 *
@verbatim
 ===============================================================================
               ##### Peripheral State and Errors functions #####
 ===============================================================================
    [..]
    This subsection provides functions allowing to
      (+) Check the DCMI state.
      (+) Get the specific DCMI error flag.

@endverbatim
  * @{
  */

/**
  * @brief  Return the DCMI state
  * @param  hdcmi pointer to a DCMI_HandleTypeDef structure that contains
  *               the configuration information for DCMI.
  * @retval HAL state
  */
HAL_DCMI_StateTypeDef HAL_DCMI_GetState(DCMI_HandleTypeDef *hdcmi)
{
  return hdcmi->State;
}

/**
* @brief  Return the DCMI error code
* @param  hdcmi  pointer to a DCMI_HandleTypeDef structure that contains
  *              the configuration information for DCMI.
* @retval DCMI Error Code
*/
uint32_t HAL_DCMI_GetError(DCMI_HandleTypeDef *hdcmi)
{
  return hdcmi->ErrorCode;
}

/**
  * @}
  */
/* Private functions ---------------------------------------------------------*/
/** @defgroup DCMI_Private_Functions DCMI Private Functions
  * @{
  */
  /**
  * @brief  DMA conversion complete callback.
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void DCMI_DMAXferCplt(DMA_HandleTypeDef *hdma)
{
  uint32_t tmp;

  DCMI_HandleTypeDef* hdcmi = ( DCMI_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  if(hdcmi->XferCount != 0U)
  {
    /* Update memory 0 address location */
    tmp = ((((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->CR) & DMA_SxCR_CT);
    if(((hdcmi->XferCount % 2U) == 0U) && (tmp != 0U))
    {
      tmp = ((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->M0AR;
      (void) HAL_DMAEx_ChangeMemory(hdcmi->DMA_Handle, (tmp + (8U*hdcmi->XferSize)), MEMORY0);
      hdcmi->XferCount--;
    }
    /* Update memory 1 address location */
    else if((((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->CR & DMA_SxCR_CT) == 0U)
    {
      tmp = ((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->M1AR;
      (void) HAL_DMAEx_ChangeMemory(hdcmi->DMA_Handle, (tmp + (8U*hdcmi->XferSize)), MEMORY1);
      hdcmi->XferCount--;
    }
    else
    {
      /* Nothing to do */
    }
  }
  /* Update memory 0 address location */
  else if((((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->CR & DMA_SxCR_CT) != 0U)
  {
    ((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->M0AR = hdcmi->pBuffPtr;
  }
  /* Update memory 1 address location */
  else if((((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->CR & DMA_SxCR_CT) == 0U)
  {
    tmp = hdcmi->pBuffPtr;
    ((DMA_Stream_TypeDef *)(hdcmi->DMA_Handle->Instance))->M1AR = (tmp + (4U*hdcmi->XferSize));
    hdcmi->XferCount = hdcmi->XferTransferNumber;
  }
  else
  {
    /* Nothing to do */
  }

  /* Check if the frame is transferred */
  if(hdcmi->XferCount == hdcmi->XferTransferNumber)
  {
    /* Enable the Frame interrupt */
    __HAL_DCMI_ENABLE_IT(hdcmi, DCMI_IT_FRAME);

    /* When snapshot mode, set dcmi state to ready */
    if((hdcmi->Instance->CR & DCMI_CR_CM) == DCMI_MODE_SNAPSHOT)
    {
      hdcmi->State= HAL_DCMI_STATE_READY;
    }
  }
}

/**
  * @brief  DMA error callback
  * @param  hdma pointer to a DMA_HandleTypeDef structure that contains
  *              the configuration information for the specified DMA module.
  * @retval None
  */
static void DCMI_DMAError(DMA_HandleTypeDef *hdma)
{
  DCMI_HandleTypeDef* hdcmi = ( DCMI_HandleTypeDef* )((DMA_HandleTypeDef* )hdma)->Parent;

  if(hdcmi->DMA_Handle->ErrorCode != HAL_DMA_ERROR_FE)
  {
    /* Initialize the DCMI state*/
    hdcmi->State = HAL_DCMI_STATE_READY;

    /* Set DCMI Error Code */
    hdcmi->ErrorCode |= HAL_DCMI_ERROR_DMA;
  }

  /* DCMI error Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
  hdcmi->ErrorCallback(hdcmi);
#else
  HAL_DCMI_ErrorCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
}

/**
  * @}
  */

/**
  * @}
  */
#endif /* HAL_DCMI_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

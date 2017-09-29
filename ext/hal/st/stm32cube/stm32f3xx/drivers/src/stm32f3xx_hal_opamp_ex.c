/**
  ******************************************************************************
  * @file    stm32f3xx_hal_opamp_ex.c
  * @author  MCD Application Team
  * @brief   Extended OPAMP HAL module driver.
  *
  *          This file provides firmware functions to manage the following
  *          functionalities of the Power Controller (OPAMP) peripheral:
  *           + Extended Initialization and de-initialization  functions
  *           + Extended Peripheral Control  functions
  *
  @verbatim
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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
#include "stm32f3xx_hal.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

#ifdef HAL_OPAMP_MODULE_ENABLED

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx) || \
    defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

/** @defgroup OPAMPEx OPAMPEx
  * @brief OPAMP Extended HAL module driver.
  * @{
  */


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/

/** @defgroup OPAMPEx_Exported_Functions OPAMP Extended Exported Functions
  * @{
  */


/** @defgroup OPAMPEx_Exported_Functions_Group1 Extended Input and Output operation functions
  * @brief    Extended Self calibration functions
  *
@verbatim
 ===============================================================================
              ##### Extended IO operation functions #####
 ===============================================================================
  [..]

@endverbatim
  * @{
  */

#if defined(STM32F302xE) || \
    defined(STM32F302xC)
/*  2 OPAMPS available */
/*  2 OPAMPS can be calibrated in parallel */

/**
  * @brief  Run the self calibration of 2 OPAMPs in parallel.
  * @param  hopamp1 handle
  * @param  hopamp2 handle
  * @retval HAL status
  * @note   Updated offset trimming values (PMOS & NMOS), user trimming is enabled
  * @note   Calibration runs about 25 ms.
  */

HAL_StatusTypeDef HAL_OPAMPEx_SelfCalibrateAll(OPAMP_HandleTypeDef *hopamp1, OPAMP_HandleTypeDef *hopamp2)
{
  HAL_StatusTypeDef status = HAL_OK;

  uint32_t trimmingvaluen1 = 0U;
  uint32_t trimmingvaluep1 = 0U;
  uint32_t trimmingvaluen2 = 0U;
  uint32_t trimmingvaluep2 = 0U;

  uint32_t delta;

  if((hopamp1 == NULL) || (hopamp1->State == HAL_OPAMP_STATE_BUSYLOCKED) || \
     (hopamp2 == NULL) || (hopamp2->State == HAL_OPAMP_STATE_BUSYLOCKED))
  {
    status = HAL_ERROR;
  }

  if(status == HAL_OK)
  {
    /* Check if OPAMP in calibration mode and calibration not yet enable */
    if((hopamp1->State ==  HAL_OPAMP_STATE_READY) && (hopamp2->State ==  HAL_OPAMP_STATE_READY))
    {
      /* Check the parameter */
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp1->Instance));
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp2->Instance));

      /* Set Calibration mode */
      /* Non-inverting input connected to calibration reference voltage. */
      SET_BIT(hopamp1->Instance->CSR, OPAMP_CSR_FORCEVP);
      SET_BIT(hopamp2->Instance->CSR, OPAMP_CSR_FORCEVP);

      /*  user trimming values are used for offset calibration */
      SET_BIT(hopamp1->Instance->CSR, OPAMP_CSR_USERTRIM);
      SET_BIT(hopamp2->Instance->CSR, OPAMP_CSR_USERTRIM);

      /* Enable calibration */
      SET_BIT (hopamp1->Instance->CSR, OPAMP_CSR_CALON);
      SET_BIT (hopamp2->Instance->CSR, OPAMP_CSR_CALON);

      /* 1st calibration - N */
      /* Select 90U% VREF */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);

      /* Enable the opamps */
      SET_BIT (hopamp1->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      SET_BIT (hopamp2->Instance->CSR, OPAMP_CSR_OPAMPxEN);

      /* Init trimming counter */
      /* Medium value */
      trimmingvaluen1 = 16U;
      trimmingvaluen2 = 16U;
      delta = 8U;

      while (delta != 0U)
      {
        /* Set candidate trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);

        /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
        /* Offset trim time: during calibration, minimum time needed between */
        /* two steps to have 1 mV accuracy */
        HAL_Delay(2U);

        if (hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen1 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen1 -= delta;
        }

        if (hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen2 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen2 -= delta;
        }

        delta >>= 1U;
      }

      // Still need to check if righ calibration is current value or un step below
      // Indeed the first value that causes the OUTCAL bit to change from 1 to 0
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);

      /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
      /* Offset trim time: during calibration, minimum time needed between */
      /* two steps to have 1 mV accuracy */
      HAL_Delay(2U);

      if (hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is actually one value more */
          trimmingvaluen1++;
          /* Set right trimming */
          MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
        }

      if (hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is actually one value more */
          trimmingvaluen2++;
          /* Set right trimming */
          MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);
        }

      /* 2nd calibration - P */
      /* Select 10U% VREF */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);

      /* Init trimming counter */
      /* Medium value */
      trimmingvaluep1 = 16U;
      trimmingvaluep2 = 16U;
      delta = 8U;

      while (delta != 0U)
      {
        /* Set candidate trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);

        /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
        /* Offset trim time: during calibration, minimum time needed between */
        /* two steps to have 1 mV accuracy */
        HAL_Delay(2U);

        if (hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep1 += delta;
        }
        else
        {
          trimmingvaluep1 -= delta;
        }

        if (hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep2 += delta;
        }
        else
        {
          trimmingvaluep2 -= delta;
        }

        delta >>= 1U;
      }

      // Still need to check if righ calibration is current value or un step below
      // Indeed the first value that causes the OUTCAL bit to change from 1 to 0
      /* Set candidate trimming */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);

       /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
       /* Offset trim time: during calibration, minimum time needed between */
       /* two steps to have 1 mV accuracy */
       HAL_Delay(2U);

      if (hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is actually one value more */
          trimmingvaluep1++;
          /* Set right trimming */
          MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
        }

      if (hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL)
        {
          /* OPAMP_CSR_OUTCAL is actually one value more */
          trimmingvaluep2++;
          /* Set right trimming */
          MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);
        }

      /* Disable calibration */
      CLEAR_BIT (hopamp1->Instance->CSR, OPAMP_CSR_CALON);
      CLEAR_BIT (hopamp2->Instance->CSR, OPAMP_CSR_CALON);

      /* Disable the OPAMPs */
      CLEAR_BIT (hopamp1->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      CLEAR_BIT (hopamp2->Instance->CSR, OPAMP_CSR_OPAMPxEN);

      /* Set operating mode back */
      CLEAR_BIT(hopamp1->Instance->CSR, OPAMP_CSR_FORCEVP);
      CLEAR_BIT(hopamp2->Instance->CSR, OPAMP_CSR_FORCEVP);

      /* Self calibration is successful  */
      /* Store calibration(user timming) results in init structure. */
      /* Select user timming mode */

      /* Write calibration result N */
      hopamp1->Init.TrimmingValueN = trimmingvaluen1;
      hopamp2->Init.TrimmingValueN = trimmingvaluen2;

      /* Write calibration result P */
      hopamp1->Init.TrimmingValueP = trimmingvaluep1;
      hopamp2->Init.TrimmingValueP = trimmingvaluep2;

      /* Calibration */
      hopamp1->Init.UserTrimming = OPAMP_TRIMMING_USER;
      hopamp2->Init.UserTrimming = OPAMP_TRIMMING_USER;

      /* Select user timming mode */
      /* And updated with calibrated settings */
      hopamp1->Init.UserTrimming = OPAMP_TRIMMING_USER;
      hopamp2->Init.UserTrimming = OPAMP_TRIMMING_USER;

      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);

      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);

    }

    else
    {
      /* At least one OPAMP can not be calibrated */
      status = HAL_ERROR;
    }
  }

  return status;
}
#endif /* STM32F302xE || */
       /* STM32F302xC    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)
/*  4 OPAMPS available */
/*  4 OPAMPS can be calibrated in parallel */

/**
  * @brief  Run the self calibration of 4 OPAMPs in parallel.
  * @param  hopamp1 handle
  * @param  hopamp2 handle
  * @param  hopamp3 handle
  * @param  hopamp4 handle
  * @retval HAL status
  * @note   Updated offset trimming values (PMOS & NMOS), user trimming is enabled
  * @note   Calibration runs about 25 ms.
  */

HAL_StatusTypeDef HAL_OPAMPEx_SelfCalibrateAll(OPAMP_HandleTypeDef *hopamp1, OPAMP_HandleTypeDef *hopamp2, OPAMP_HandleTypeDef *hopamp3, OPAMP_HandleTypeDef *hopamp4)
{
  HAL_StatusTypeDef status = HAL_OK;

  uint32_t trimmingvaluen1 = 0U;
  uint32_t trimmingvaluep1 = 0U;
  uint32_t trimmingvaluen2 = 0U;
  uint32_t trimmingvaluep2 = 0U;
  uint32_t trimmingvaluen3 = 0U;
  uint32_t trimmingvaluep3 = 0U;
  uint32_t trimmingvaluen4 = 0U;
  uint32_t trimmingvaluep4 = 0U;

  uint32_t delta;

  if((hopamp1 == NULL) || (hopamp1->State == HAL_OPAMP_STATE_BUSYLOCKED) || \
     (hopamp2 == NULL) || (hopamp2->State == HAL_OPAMP_STATE_BUSYLOCKED) || \
     (hopamp3 == NULL) || (hopamp3->State == HAL_OPAMP_STATE_BUSYLOCKED) || \
     (hopamp4 == NULL) || (hopamp4->State == HAL_OPAMP_STATE_BUSYLOCKED))
  {
    status = HAL_ERROR;
  }

  if(status == HAL_OK)
  {
    /* Check if OPAMP in calibration mode and calibration not yet enable */
    if((hopamp1->State ==  HAL_OPAMP_STATE_READY) && (hopamp2->State ==  HAL_OPAMP_STATE_READY) && \
       (hopamp3->State ==  HAL_OPAMP_STATE_READY) && (hopamp4->State ==  HAL_OPAMP_STATE_READY))
    {
      /* Check the parameter */
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp1->Instance));
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp2->Instance));
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp3->Instance));
      assert_param(IS_OPAMP_ALL_INSTANCE(hopamp4->Instance));

      /* Set Calibration mode */
      /* Non-inverting input connected to calibration reference voltage. */
      SET_BIT(hopamp1->Instance->CSR, OPAMP_CSR_FORCEVP);
      SET_BIT(hopamp2->Instance->CSR, OPAMP_CSR_FORCEVP);
      SET_BIT(hopamp3->Instance->CSR, OPAMP_CSR_FORCEVP);
      SET_BIT(hopamp4->Instance->CSR, OPAMP_CSR_FORCEVP);

      /*  user trimming values are used for offset calibration */
      SET_BIT(hopamp1->Instance->CSR, OPAMP_CSR_USERTRIM);
      SET_BIT(hopamp2->Instance->CSR, OPAMP_CSR_USERTRIM);
      SET_BIT(hopamp3->Instance->CSR, OPAMP_CSR_USERTRIM);
      SET_BIT(hopamp4->Instance->CSR, OPAMP_CSR_USERTRIM);

      /* Enable calibration */
      SET_BIT (hopamp1->Instance->CSR, OPAMP_CSR_CALON);
      SET_BIT (hopamp2->Instance->CSR, OPAMP_CSR_CALON);
      SET_BIT (hopamp3->Instance->CSR, OPAMP_CSR_CALON);
      SET_BIT (hopamp4->Instance->CSR, OPAMP_CSR_CALON);

      /* 1st calibration - N */
      /* Select 90U% VREF */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_90VDDA);

      /* Enable the opamps */
      SET_BIT (hopamp1->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      SET_BIT (hopamp2->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      SET_BIT (hopamp3->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      SET_BIT (hopamp4->Instance->CSR, OPAMP_CSR_OPAMPxEN);

      /* Init trimming counter */
      /* Medium value */
      trimmingvaluen1 = 16U;
      trimmingvaluen2 = 16U;
      trimmingvaluen3 = 16U;
      trimmingvaluen4 = 16U;
      delta = 8U;

      while (delta != 0U)
      {
        /* Set candidate trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);
        MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen3<<OPAMP_INPUT_INVERTING);
        MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen4<<OPAMP_INPUT_INVERTING);

        /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
        /* Offset trim time: during calibration, minimum time needed between */
        /* two steps to have 1 mV accuracy */
        HAL_Delay(2U);

        if ((hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen1 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen1 -= delta;
        }

        if ((hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen2 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen2 -= delta;
        }

        if ((hopamp3->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen3 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen3 -= delta;
        }

        if ((hopamp4->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluen4 += delta;
        }
        else
        {
          /* OPAMP_CSR_OUTCAL is LOW try lower trimming */
          trimmingvaluen4 -= delta;
        }

        delta >>= 1U;
      }

      /* Still need to check if righ calibration is current value or un step below */
      /* Indeed the first value that causes the OUTCAL bit to change from 1 to 0U */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen3<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen4<<OPAMP_INPUT_INVERTING);

      /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
      /* Offset trim time: during calibration, minimum time needed between */
      /* two steps to have 1 mV accuracy */
      HAL_Delay(2U);

      if ((hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* OPAMP_CSR_OUTCAL is actually one value more */
        trimmingvaluen1++;
        /* Set right trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
      }

      if ((hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* OPAMP_CSR_OUTCAL is actually one value more */
        trimmingvaluen2++;
        /* Set right trimming */
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);
      }

      if ((hopamp3->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* OPAMP_CSR_OUTCAL is actually one value more */
        trimmingvaluen3++;
        /* Set right trimming */
        MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen3<<OPAMP_INPUT_INVERTING);
      }

      if ((hopamp4->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* OPAMP_CSR_OUTCAL is actually one value more */
        trimmingvaluen4++;
        /* Set right trimming */
        MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen4<<OPAMP_INPUT_INVERTING);
      }

      /* 2nd calibration - P */
      /* Select 10U% VREF */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_CALSEL, OPAMP_VREF_10VDDA);

      /* Init trimming counter */
      /* Medium value */
      trimmingvaluep1 = 16U;
      trimmingvaluep2 = 16U;
      trimmingvaluep3 = 16U;
      trimmingvaluep4 = 16U;

      delta = 8U;

      while (delta != 0U)
      {
        /* Set candidate trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);
        MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep3<<OPAMP_INPUT_NONINVERTING);
        MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep4<<OPAMP_INPUT_NONINVERTING);

        /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
        /* Offset trim time: during calibration, minimum time needed between */
        /* two steps to have 1 mV accuracy */
        HAL_Delay(2U);

        if ((hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep1 += delta;
        }
        else
        {
          trimmingvaluep1 -= delta;
        }

        if ((hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep2 += delta;
        }
        else
        {
          trimmingvaluep2 -= delta;
        }

        if ((hopamp3->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep3 += delta;
        }
        else
        {
          trimmingvaluep3 -= delta;
        }

        if ((hopamp4->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
        {
          /* OPAMP_CSR_OUTCAL is HIGH try higher trimming */
          trimmingvaluep4 += delta;
        }
        else
        {
          trimmingvaluep4 -= delta;
        }

        delta >>= 1U;
      }

      /* Still need to check if righ calibration is current value or un step below */
      /* Indeed the first value that causes the OUTCAL bit to change from 1 to 0U */
      /* Set candidate trimming */
      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep3<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep4<<OPAMP_INPUT_NONINVERTING);

      /* OFFTRIMmax delay 2 ms as per datasheet (electrical characteristics */
      /* Offset trim time: during calibration, minimum time needed between */
      /* two steps to have 1 mV accuracy */
      HAL_Delay(2U);

      if ((hopamp1->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* Trimming value is actually one value more */
        trimmingvaluep1++;
        /* Set right trimming */
        MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
      }

      if ((hopamp2->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* Trimming value is actually one value more */
        trimmingvaluep2++;
        /* Set right trimming */
        MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);
      }

      if ((hopamp3->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* Trimming value is actually one value more */
        trimmingvaluep3++;
        /* Set right trimming */
        MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep3<<OPAMP_INPUT_NONINVERTING);
      }

      if ((hopamp4->Instance->CSR & OPAMP_CSR_OUTCAL) != RESET)
      {
        /* Trimming value is actually one value more */
        trimmingvaluep4++;
        /* Set right trimming */
        MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep4<<OPAMP_INPUT_NONINVERTING);
      }

      /* Disable calibration */
      CLEAR_BIT (hopamp1->Instance->CSR, OPAMP_CSR_CALON);
      CLEAR_BIT (hopamp2->Instance->CSR, OPAMP_CSR_CALON);
      CLEAR_BIT (hopamp3->Instance->CSR, OPAMP_CSR_CALON);
      CLEAR_BIT (hopamp4->Instance->CSR, OPAMP_CSR_CALON);

      /* Disable the OPAMPs */
      CLEAR_BIT (hopamp1->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      CLEAR_BIT (hopamp2->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      CLEAR_BIT (hopamp3->Instance->CSR, OPAMP_CSR_OPAMPxEN);
      CLEAR_BIT (hopamp4->Instance->CSR, OPAMP_CSR_OPAMPxEN);

      /* Set normal operating mode back */
      CLEAR_BIT(hopamp1->Instance->CSR, OPAMP_CSR_FORCEVP);
      CLEAR_BIT(hopamp2->Instance->CSR, OPAMP_CSR_FORCEVP);
      CLEAR_BIT(hopamp3->Instance->CSR, OPAMP_CSR_FORCEVP);
      CLEAR_BIT(hopamp4->Instance->CSR, OPAMP_CSR_FORCEVP);

      /* Self calibration is successful  */
      /* Store calibration(user timming) results in init structure. */
      /* Select user timming mode */

      /* Write calibration result N */
      hopamp1->Init.TrimmingValueN = trimmingvaluen1;
      hopamp2->Init.TrimmingValueN = trimmingvaluen2;
      hopamp3->Init.TrimmingValueN = trimmingvaluen3;
      hopamp4->Init.TrimmingValueN = trimmingvaluen4;

      /* Write calibration result P */
      hopamp1->Init.TrimmingValueP = trimmingvaluep1;
      hopamp2->Init.TrimmingValueP = trimmingvaluep2;
      hopamp3->Init.TrimmingValueP = trimmingvaluep3;
      hopamp4->Init.TrimmingValueP = trimmingvaluep4;

      /* Select user timming mode */
      /* And updated with calibrated settings */
      hopamp1->Init.UserTrimming = OPAMP_TRIMMING_USER;
      hopamp2->Init.UserTrimming = OPAMP_TRIMMING_USER;
      hopamp3->Init.UserTrimming = OPAMP_TRIMMING_USER;
      hopamp4->Init.UserTrimming = OPAMP_TRIMMING_USER;

      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen1<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen2<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen3<<OPAMP_INPUT_INVERTING);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETN, trimmingvaluen4<<OPAMP_INPUT_INVERTING);

      MODIFY_REG(hopamp1->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep1<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp2->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep2<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp3->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep3<<OPAMP_INPUT_NONINVERTING);
      MODIFY_REG(hopamp4->Instance->CSR, OPAMP_CSR_TRIMOFFSETP, trimmingvaluep4<<OPAMP_INPUT_NONINVERTING);

    }

    else
    {
      /* At least one OPAMP can not be calibrated */
      status = HAL_ERROR;
    }
  }

  return status;
}
#endif /* STM32F303xE || STM32F398xx  || */
       /* STM32F303xC || STM32F358xx     */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#endif /* HAL_OPAMP_MODULE_ENABLED */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

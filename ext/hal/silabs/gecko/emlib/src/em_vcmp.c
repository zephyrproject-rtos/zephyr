/***************************************************************************//**
 * @file em_vcmp.c
 * @brief Voltage Comparator (VCMP) peripheral API
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include "em_vcmp.h"
#if defined(VCMP_COUNT) && (VCMP_COUNT > 0)

#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup VCMP
 * @brief Voltage Comparator (VCMP) Peripheral API
 * @details
 *  This module contains functions to control the VCMP peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The VCMP monitors the input voltage supply and
 *  generates interrupts on events using as little as 100 nA.
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Configure and enable Voltage Comparator
 *
 * @param[in] vcmpInit
 *   VCMP Initialization structure
 ******************************************************************************/
void VCMP_Init(const VCMP_Init_TypeDef *vcmpInit)
{
  /* Verify input */
  EFM_ASSERT((vcmpInit->inactive == 0) || (vcmpInit->inactive == 1));
  EFM_ASSERT((vcmpInit->biasProg >= 0) && (vcmpInit->biasProg < 16));

  /* Configure Half Bias setting */
  if (vcmpInit->halfBias)
  {
    VCMP->CTRL |= VCMP_CTRL_HALFBIAS;
  }
  else
  {
    VCMP->CTRL &= ~(VCMP_CTRL_HALFBIAS);
  }

  /* Configure bias prog */
  VCMP->CTRL &= ~(_VCMP_CTRL_BIASPROG_MASK);
  VCMP->CTRL |= (vcmpInit->biasProg << _VCMP_CTRL_BIASPROG_SHIFT);

  /* Configure sense for falling edge */
  if (vcmpInit->irqFalling)
  {
    VCMP->CTRL |= VCMP_CTRL_IFALL;
  }
  else
  {
    VCMP->CTRL &= ~(VCMP_CTRL_IFALL);
  }

  /* Configure sense for rising edge */
  if (vcmpInit->irqRising)
  {
    VCMP->CTRL |= VCMP_CTRL_IRISE;
  }
  else
  {
    VCMP->CTRL &= ~(VCMP_CTRL_IRISE);
  }

  /* Configure warm-up time */
  VCMP->CTRL &= ~(_VCMP_CTRL_WARMTIME_MASK);
  VCMP->CTRL |= (vcmpInit->warmup << _VCMP_CTRL_WARMTIME_SHIFT);

  /* Configure hysteresis */
  switch (vcmpInit->hyst)
  {
    case vcmpHyst20mV:
      VCMP->CTRL |= VCMP_CTRL_HYSTEN;
      break;
    case vcmpHystNone:
      VCMP->CTRL &= ~(VCMP_CTRL_HYSTEN);
      break;
    default:
      break;
  }

  /* Configure inactive output value */
  VCMP->CTRL |= (vcmpInit->inactive << _VCMP_CTRL_INACTVAL_SHIFT);

  /* Configure trigger level */
  VCMP_TriggerSet(vcmpInit->triggerLevel);

  /* Enable or disable VCMP */
  if (vcmpInit->enable)
  {
    VCMP->CTRL |= VCMP_CTRL_EN;
  }
  else
  {
    VCMP->CTRL &= ~(VCMP_CTRL_EN);
  }

  /* If Low Power Reference is enabled, wait until VCMP is ready */
  /* before enabling it, see reference manual for deatils        */
  /* Configuring Low Power Ref without enable has no effect      */
  if(vcmpInit->lowPowerRef && vcmpInit->enable)
  {
    /* Poll for VCMP ready */
    while(!VCMP_Ready());
    VCMP_LowPowerRefSet(vcmpInit->lowPowerRef);
  }

  /* Clear edge interrupt */
  VCMP_IntClear(VCMP_IF_EDGE);
}


/***************************************************************************//**
 * @brief
 *    Enable or disable Low Power Reference setting
 *
 * @param[in] enable
 *    If true, enables low power reference, if false disable low power reference
 ******************************************************************************/
void VCMP_LowPowerRefSet(bool enable)
{
  if (enable)
  {
    VCMP->INPUTSEL |= VCMP_INPUTSEL_LPREF;
  }
  else
  {
    VCMP->INPUTSEL &= ~VCMP_INPUTSEL_LPREF;
  }
}


/***************************************************************************//**
 * @brief
 *    Configure trigger level of voltage comparator
 *
 * @param[in] level
 *    Trigger value, in range 0-63
 ******************************************************************************/
void VCMP_TriggerSet(int level)
{
  /* Trigger range is 6 bits, value from 0-63 */
  EFM_ASSERT((level > 0) && (level < 64));

  /* Set trigger level */
  VCMP->INPUTSEL = (VCMP->INPUTSEL & ~(_VCMP_INPUTSEL_TRIGLEVEL_MASK))
                   | (level << _VCMP_INPUTSEL_TRIGLEVEL_SHIFT);
}


/** @} (end addtogroup VCMP) */
/** @} (end addtogroup emlib) */
#endif /* defined(VCMP_COUNT) && (VCMP_COUNT > 0) */

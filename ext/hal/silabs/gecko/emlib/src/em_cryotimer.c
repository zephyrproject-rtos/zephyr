/***************************************************************************//**
 * @file em_cryotimer.c
 * @brief Ultra Low Energy Timer/Counter (CRYOTIMER) peripheral API
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
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
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

#include "em_cryotimer.h"
#include "em_bus.h"

#if defined(CRYOTIMER_PRESENT) && (CRYOTIMER_COUNT == 1)

/***************************************************************************//**
 * @brief
 *   Initialize the CRYOTIMER.
 *
 * @details
 *   Use this function to initialize the CRYOTIMER.
 *   Select prescaler setting and select low frequency oscillator.
 *   Refer to the configuration structure @ref CRYOTIMER_Init_TypeDef for more
 *   details.
 *
 * @param[in] init
 *   Pointer to initialization structure.
 ******************************************************************************/
void CRYOTIMER_Init(const CRYOTIMER_Init_TypeDef *init)
{
  CRYOTIMER->PERIODSEL = (uint32_t)init->period & _CRYOTIMER_PERIODSEL_MASK;
  CRYOTIMER->CTRL = ((uint32_t)init->enable << _CRYOTIMER_CTRL_EN_SHIFT)
                  | ((uint32_t)init->debugRun << _CRYOTIMER_CTRL_DEBUGRUN_SHIFT)
                  | ((uint32_t)init->osc << _CRYOTIMER_CTRL_OSCSEL_SHIFT)
                  | ((uint32_t)init->presc << _CRYOTIMER_CTRL_PRESC_SHIFT);
  CRYOTIMER_EM4WakeupEnable(init->em4Wakeup);
}

#endif /* defined(CRYOTIMER_PRESENT) && (CRYOTIMER_COUNT > 0) */

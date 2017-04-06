/***************************************************************************//**
 * @file em_wdog.c
 * @brief Watchdog (WDOG) peripheral API
 *   devices.
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

#include "em_wdog.h"
#if defined(WDOG_COUNT) && (WDOG_COUNT > 0)

#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup WDOG
 * @brief Watchdog (WDOG) Peripheral API
 * @details
 *  This module contains functions to control the WDOG peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The WDOG resets the system in case of a fault
 *  condition.
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Enable/disable the watchdog timer.
 *
 * @note
 *   This function modifies the WDOG CTRL register which requires
 *   synchronization into the low frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] enable
 *   true to enable watchdog, false to disable. Watchdog cannot be disabled if
 *   watchdog has been locked.
 ******************************************************************************/
void WDOGn_Enable(WDOG_TypeDef *wdog, bool enable)
{
  /* SYNCBUSY may stall when locked. */
  if (wdog->CTRL & WDOG_CTRL_LOCK)
  {
    return;
  }

  if (!enable)
  {
    /* If the user intends to disable and the WDOG is enabled */
    if (BUS_RegBitRead(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT))
    {
      /* Wait for any pending previous write operation to have been completed in */
      /* low frequency domain */
      while (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL)
        ;

      BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT, 0);
    }
  }
  else
  {
    BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT, 1);
  }
}


/***************************************************************************//**
 * @brief
 *   Feed the watchdog.
 *
 * @details
 *   When the watchdog is activated, it must be fed (ie clearing the counter)
 *   before it reaches the defined timeout period. Otherwise, the watchdog
 *   will generate a reset.
  *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 ******************************************************************************/
void WDOGn_Feed(WDOG_TypeDef *wdog)
{
  /* The watchdog should not be fed while it is disabled */
  if (!(wdog->CTRL & WDOG_CTRL_EN))
  {
    return;
  }

  /* If a previous clearing is being synchronized to LF domain, then there */
  /* is no point in waiting for it to complete before clearing over again. */
  /* This avoids stalling the core in the typical use case where some idle loop */
  /* keeps clearing the watchdog. */
  if (wdog->SYNCBUSY & WDOG_SYNCBUSY_CMD)
  {
    return;
  }
  /* Before writing to the WDOG_CMD register we also need to make sure that
   * any previous write to WDOG_CTRL is complete. */
  while ( wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL )
    ;

  wdog->CMD = WDOG_CMD_CLEAR;
}


/***************************************************************************//**
 * @brief
 *   Initialize watchdog (assuming the watchdog configuration has not been
 *   locked).
 *
 * @note
 *   This function modifies the WDOG CTRL register which requires
 *   synchronization into the low frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 *
 * @param[in] init
 *   Structure holding watchdog configuration. A default setting
 *   #WDOG_INIT_DEFAULT is available for init.
 ******************************************************************************/
void WDOGn_Init(WDOG_TypeDef *wdog, const WDOG_Init_TypeDef *init)
{
  uint32_t setting;

  if (init->enable)
  {
    setting = WDOG_CTRL_EN;
  }
  else
  {
    setting = 0;
  }

  if (init->debugRun)
  {
    setting |= WDOG_CTRL_DEBUGRUN;
  }

  if (init->em2Run)
  {
    setting |= WDOG_CTRL_EM2RUN;
  }

  if (init->em3Run)
  {
    setting |= WDOG_CTRL_EM3RUN;
  }

  if (init->em4Block)
  {
    setting |= WDOG_CTRL_EM4BLOCK;
  }
  if (init->swoscBlock)
  {
    setting |= WDOG_CTRL_SWOSCBLOCK;
  }
  if (init->lock)
  {
    setting |= WDOG_CTRL_LOCK;
  }
#if defined( _WDOG_CTRL_WDOGRSTDIS_MASK )
  if (init->resetDisable)
  {
    setting |= WDOG_CTRL_WDOGRSTDIS;
  }
#endif
  setting |= ((uint32_t)(init->clkSel)   << _WDOG_CTRL_CLKSEL_SHIFT)
#if defined( _WDOG_CTRL_WARNSEL_MASK )
             | ((uint32_t)(init->warnSel) << _WDOG_CTRL_WARNSEL_SHIFT)
#endif
#if defined( _WDOG_CTRL_WINSEL_MASK )
             | ((uint32_t)(init->winSel) << _WDOG_CTRL_WINSEL_SHIFT)
#endif
             | ((uint32_t)(init->perSel) << _WDOG_CTRL_PERSEL_SHIFT);

  /* Wait for any pending previous write operation to have been completed in */
  /* low frequency domain */
  while (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL)
    ;

  wdog->CTRL = setting;
}


/***************************************************************************//**
 * @brief
 *   Lock the watchdog configuration.
 *
 * @details
 *   This prevents errors from overwriting the watchdog configuration, possibly
 *   disabling it. Only a reset can unlock the watchdog config, once locked.
 *
 *   If the LFRCO or LFXO clocks are used to clock the watchdog, one should
 *   consider using the option of inhibiting those clocks to be disabled,
 *   please see the WDOG_Enable() init structure.
 *
 * @note
 *   This function modifies the WDOG CTRL register which requires
 *   synchronization into the low frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   Pointer to WDOG peripheral register block.
 ******************************************************************************/
void WDOGn_Lock(WDOG_TypeDef *wdog)
{
  /* Wait for any pending previous write operation to have been completed in */
  /* low frequency domain */
  while (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL)
    ;

  /* Disable writing to the control register */
  BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_LOCK_SHIFT, 1);
}


/** @} (end addtogroup WDOG) */
/** @} (end addtogroup emlib) */
#endif /* defined(WDOG_COUNT) && (WDOG_COUNT > 0) */

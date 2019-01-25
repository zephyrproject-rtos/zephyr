/***************************************************************************//**
 * @file em_wdog.c
 * @brief Watchdog (WDOG) peripheral API
 *   devices.
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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
#include "em_core.h"

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
 *   synchronization into the low-frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   A pointer to the WDOG peripheral register block.
 *
 * @param[in] enable
 *   True to enable Watchdog, false to disable. Watchdog cannot be disabled if
 *   it's been locked.
 ******************************************************************************/
void WDOGn_Enable(WDOG_TypeDef *wdog, bool enable)
{
  /* SYNCBUSY may stall when locked. */
#if defined(_WDOG_STATUS_MASK)
  if ((wdog->STATUS & _WDOG_STATUS_LOCK_MASK) == WDOG_STATUS_LOCK_LOCKED) {
    return;
  }
#else
  if (wdog->CTRL & WDOG_CTRL_LOCK) {
    return;
  }
#endif

#if defined(_WDOG_EN_MASK)
  if (!enable) {
    while (wdog->SYNCBUSY & WDOG_SYNCBUSY_CMD) {
    }
    wdog->EN_CLR = WDOG_EN_EN;
  } else {
    wdog->EN_SET = WDOG_EN_EN;
  }
#else
  if (!enable) {
    /* If the user intends to disable and the WDOG is enabled. */
    if (BUS_RegBitRead(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT)) {
      /* Wait for any pending previous write operation to have been completed in */
      /* the low-frequency domain. */
      while ( (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL) != 0U ) {
      }

      BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT, 0);
    }
  } else {
    BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_EN_SHIFT, 1);
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Feed WDOG.
 *
 * @details
 *   When WDOG is activated, it must be fed (i.e., clearing the counter)
 *   before it reaches the defined timeout period. Otherwise, WDOG
 *   will generate a reset.
 *
 * @param[in] wdog
 *   A pointer to the WDOG peripheral register block.
 ******************************************************************************/
void WDOGn_Feed(WDOG_TypeDef *wdog)
{
#if (_SILICON_LABS_32B_SERIES < 2)

  /* WDOG should not be fed while it is disabled. */
  if (!(wdog->CTRL & WDOG_CTRL_EN)) {
    return;
  }

  /* If a previous clearing is synchronized to theLF domain, there */
  /* is no point in waiting for it to complete before clearing over again. */
  /* This avoids stalling the core in the typical use case where some idle loop */
  /* keeps clearing WDOG. */
  if (wdog->SYNCBUSY & WDOG_SYNCBUSY_CMD) {
    return;
  }
  /* Before writing to the WDOG_CMD register, make sure that
   * any previous write to the WDOG_CTRL is complete. */
  while ( (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL) != 0U ) {
  }

  wdog->CMD = WDOG_CMD_CLEAR;

#else /* Series 2 devices */

  CORE_DECLARE_IRQ_STATE;

  /* WDOG should not be fed while it is disabled. */
  if ((wdog->EN & WDOG_EN_EN) == 0U) {
    return;
  }

  /* We need an atomic section around the check for sync and the clear command
   * because sending a clear command while a previous command is being synchronized
   * will cause a BusFault. */
  CORE_ENTER_ATOMIC();
  if ((wdog->SYNCBUSY & WDOG_SYNCBUSY_CMD) == 0U) {
    wdog->CMD = WDOG_CMD_CLEAR;
  }
  CORE_EXIT_ATOMIC();

#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize WDOG (assuming the WDOG configuration has not been
 *   locked).
 *
 * @note
 *   This function modifies the WDOG CTRL register which requires
 *   synchronization into the low-frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   Pointer to the WDOG peripheral register block.
 *
 * @param[in] init
 *   The structure holding the WDOG configuration. A default setting
 *   #WDOG_INIT_DEFAULT is available for initialization.
 ******************************************************************************/
void WDOGn_Init(WDOG_TypeDef *wdog, const WDOG_Init_TypeDef *init)
{
#if defined(_WDOG_CFG_MASK)
  // Handle series-2 devices

  if (wdog->EN != 0U) {
    while (wdog->SYNCBUSY != 0U) {
      // Wait for any potential synchronization to finish
    }
    wdog->EN_CLR = WDOG_EN_EN;
  }

  wdog->CFG = (init->debugRun       ? WDOG_CFG_DEBUGRUN   : 0U)
              | (init->em2Run       ? WDOG_CFG_EM2RUN     : 0U)
              | (init->em3Run       ? WDOG_CFG_EM3RUN     : 0U)
              | (init->em4Block     ? WDOG_CFG_EM4BLOCK   : 0U)
              | (init->resetDisable ? WDOG_CFG_WDOGRSTDIS : 0U)
              | ((uint32_t)(init->warnSel) << _WDOG_CFG_WARNSEL_SHIFT)
              | ((uint32_t)(init->winSel) << _WDOG_CFG_WINSEL_SHIFT)
              | ((uint32_t)(init->perSel) << _WDOG_CFG_PERSEL_SHIFT);

  WDOGn_Enable(wdog, init->enable);

  if (init->lock) {
    WDOGn_Lock(wdog);
  }
#else
  // Handle series-0 and series-1 devices
  uint32_t setting;

  setting = (init->enable       ? WDOG_CTRL_EN         : 0U)
            | (init->debugRun   ? WDOG_CTRL_DEBUGRUN   : 0U)
            | (init->em2Run     ? WDOG_CTRL_EM2RUN     : 0U)
            | (init->em3Run     ? WDOG_CTRL_EM3RUN     : 0U)
            | (init->em4Block   ? WDOG_CTRL_EM4BLOCK   : 0U)
            | (init->swoscBlock ? WDOG_CTRL_SWOSCBLOCK : 0U)
            | (init->lock       ? WDOG_CTRL_LOCK       : 0U)
            | ((uint32_t)(init->clkSel) << _WDOG_CTRL_CLKSEL_SHIFT)
            | ((uint32_t)(init->perSel) << _WDOG_CTRL_PERSEL_SHIFT);

#if defined(_WDOG_CTRL_WDOGRSTDIS_MASK)
  setting |= (init->resetDisable ? WDOG_CTRL_WDOGRSTDIS : 0U);
#endif
#if defined(_WDOG_CTRL_WARNSEL_MASK)
  setting |= ((uint32_t)(init->warnSel) << _WDOG_CTRL_WARNSEL_SHIFT);
#endif
#if defined(_WDOG_CTRL_WINSEL_MASK)
  setting |= ((uint32_t)(init->winSel) << _WDOG_CTRL_WINSEL_SHIFT);
#endif

  /* Wait for previous synchronization to finish if the watchdog is enabled */
  if ((wdog->CTRL & WDOG_CTRL_EN) != 0U) {
    while ((wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL) != 0U) {
    }
  }

  wdog->CTRL = setting;
#endif
}

/***************************************************************************//**
 * @brief
 *   Lock the WDOG configuration.
 *
 * @details
 *   This prevents errors from overwriting the WDOG configuration, possibly
 *   disabling it. Only a reset can unlock the WDOG configuration once locked.
 *
 *   If the LFRCO or LFXO clocks are used to clock WDOG,
 *   consider using the option of inhibiting those clocks to be disabled.
 *   See the WDOG_Enable() initialization structure.
 *
 * @note
 *   This function modifies the WDOG CTRL register which requires
 *   synchronization into the low-frequency domain. If this register is modified
 *   before a previous update to the same register has completed, this function
 *   will stall until the previous synchronization has completed.
 *
 * @param[in] wdog
 *   A pointer to WDOG peripheral register block.
 ******************************************************************************/
void WDOGn_Lock(WDOG_TypeDef *wdog)
{
#if defined(_WDOG_LOCK_MASK)
  wdog->LOCK = _WDOG_LOCK_LOCKKEY_LOCK;
#else
  /* Wait for any pending previous write operation to have been completed in */
  /* the low-frequency domain. */
  while ( (wdog->SYNCBUSY & WDOG_SYNCBUSY_CTRL) != 0U ) {
  }

  /* Disable writing to the control register. */
  BUS_RegBitWrite(&wdog->CTRL, _WDOG_CTRL_LOCK_SHIFT, 1);
#endif
}

/***************************************************************************//**
 * @brief
 *   Unlock the WDOG configuration.
 *
 * @details
 *   Note that this function will have no effect on devices where a reset is
 *   the only way to unlock the watchdog.
 *
 * @param[in] wdog
 *   A pointer to WDOG peripheral register block.
 ******************************************************************************/
void WDOGn_Unlock(WDOG_TypeDef *wdog)
{
#if defined(_WDOG_LOCK_MASK)
  wdog->LOCK = _WDOG_LOCK_LOCKKEY_UNLOCK;
#else
  (void) wdog;
#endif
}

/** @} (end addtogroup WDOG) */
/** @} (end addtogroup emlib) */
#endif /* defined(WDOG_COUNT) && (WDOG_COUNT > 0) */

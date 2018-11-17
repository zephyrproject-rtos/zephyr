/***************************************************************************//**
 * @file em_letimer.c
 * @brief Low Energy Timer (LETIMER) Peripheral API
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

#include "em_letimer.h"
#if defined(LETIMER_COUNT) && (LETIMER_COUNT > 0)
#include "em_cmu.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LETIMER
 * @brief Low Energy Timer (LETIMER) Peripheral API
 * @details
 *  This module contains functions to control the LETIMER peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The LETIMER is a down-counter that can keep track
 *  of time and output configurable waveforms.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** A validation of the valid comparator register for assert statements. */
#define LETIMER_COMP_REG_VALID(reg)    (((reg) <= 1))

/** A validation of the LETIMER register block pointer reference for assert statements. */
#if (LETIMER_COUNT == 1)
#define LETIMER_REF_VALID(ref)         ((ref) == LETIMER0)
#elif (LETIMER_COUNT == 2)
#define LETIMER_REF_VALID(ref)         (((ref) == LETIMER0) ||  ((ref) == LETIMER1))
#else
#error Undefined number of analog comparators (ACMP).
#endif

/** A validation of the valid repeat counter register for assert statements. */
#define LETIMER_REP_REG_VALID(reg)     (((reg) <= 1))

/** @endcond */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

#if defined(_EFM32_GECKO_FAMILY)
/***************************************************************************//**
 * @brief
 *   Wait for an ongoing sync of register(s) to the low-frequency domain to complete.
 *
 * @note
 *   This only applies to the Gecko Family. See the reference manual
 *   chapter about Access to Low Energy Peripherals (Asynchronos Registers)
 *   for details.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] mask
 *   A bitmask corresponding to SYNCBUSY register defined bits, indicating
 *   registers that must complete any ongoing synchronization.
 ******************************************************************************/
__STATIC_INLINE void regSync(LETIMER_TypeDef *letimer, uint32_t mask)
{
#if defined(_LETIMER_FREEZE_MASK)
  /* Avoid a deadlock if modifying the same register twice when freeze mode is */
  /* activated. */
  if (letimer->FREEZE & LETIMER_FREEZE_REGFREEZE) {
    return;
  }
#endif

  /* Wait for any pending previous write operation to complete */
  /* in the low-frequency domain. This is only required for the Gecko Family of devices.  */
  while (letimer->SYNCBUSY & mask) {
  }
}
#endif

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Get the LETIMER compare register value.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] comp
 *   A compare register to get, either 0 or 1.
 *
 * @return
 *   A compare register value, 0 if invalid register selected.
 ******************************************************************************/
uint32_t LETIMER_CompareGet(LETIMER_TypeDef *letimer, unsigned int comp)
{
  uint32_t ret;

  EFM_ASSERT(LETIMER_REF_VALID(letimer) && LETIMER_COMP_REG_VALID(comp));

  /* Initialize the selected compare value. */
  switch (comp) {
    case 0:
      ret = letimer->COMP0;
      break;

    case 1:
      ret = letimer->COMP1;
      break;

    default:
      /* An unknown compare register selected. */
      ret = 0;
      break;
  }

  return(ret);
}

/***************************************************************************//**
 * @brief
 *   Set the LETIMER compare register value.
 *
 * @note
 *   The setting of a compare register requires synchronization into the
 *   low frequency domain. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed. This only applies to the Gecko Family. See
 *   comments in the LETIMER_Sync() internal function call.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] comp
 *   A compare register to set, either 0 or 1.
 *
 * @param[in] value
 *   An initialization value (<= 0x0000ffff).
 ******************************************************************************/
void LETIMER_CompareSet(LETIMER_TypeDef *letimer,
                        unsigned int comp,
                        uint32_t value)
{
  volatile uint32_t *compReg;

  EFM_ASSERT(LETIMER_REF_VALID(letimer)
             && LETIMER_COMP_REG_VALID(comp)
             && ((value & ~(_LETIMER_COMP0_COMP0_MASK
                            >> _LETIMER_COMP0_COMP0_SHIFT))
                 == 0));

  /* Initialize the selected compare value. */
  switch (comp) {
    case 0:
      compReg  = &(letimer->COMP0);
      break;

    case 1:
      compReg  = &(letimer->COMP1);
      break;

    default:
      /* An unknown compare register selected, abort. */
      return;
  }

#if defined(_EFM32_GECKO_FAMILY)
  /* LF register about to be modified requires sync; busy check. */
  regSync(letimer, comp ? LETIMER_SYNCBUSY_COMP1 : LETIMER_SYNCBUSY_COMP0);
#endif

  *compReg = value;
}

/***************************************************************************//**
 * @brief
 *   Start/stop LETIMER.
 *
 * @note
 *   The enabling/disabling of the LETIMER modifies the LETIMER CMD register
 *   which requires synchronization into the low-frequency domain. If this
 *   register is modified before a previous update to the same register has
 *   completed, this function will stall until the previous synchronization has
 *   completed. This only applies to the Gecko Family. See comments in the
 *   LETIMER_Sync() internal function call.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] enable
 *   True to enable counting, false to disable.
 ******************************************************************************/
void LETIMER_Enable(LETIMER_TypeDef *letimer, bool enable)
{
  EFM_ASSERT(LETIMER_REF_VALID(letimer));

#if defined(_EFM32_GECKO_FAMILY)
  /* LF register about to be modified requires sync; busy check. */
  regSync(letimer, LETIMER_SYNCBUSY_CMD);
#elif defined (LETIMER_SYNCBUSY_START) && defined (LETIMER_SYNCBUSY_STOP)
  uint32_t syncBusyMask = LETIMER_SYNCBUSY_STOP | LETIMER_SYNCBUSY_START;
  while (letimer->SYNCBUSY & syncBusyMask) {
  }
#endif

  if (enable) {
    letimer->CMD = LETIMER_CMD_START;
  } else {
    letimer->CMD = LETIMER_CMD_STOP;
  }
}

#if defined(_LETIMER_FREEZE_MASK)
/***************************************************************************//**
 * @brief
 *   LETIMER register synchronization freeze control.
 *
 * @details
 *   Some LETIMER registers require synchronization into the low-frequency (LF)
 *   domain. The freeze feature allows for several such registers to be
 *   modified before passing them to the LF domain simultaneously (which
 *   takes place when the freeze mode is disabled).
 *
 * @note
 *   When enabling freeze mode, this function will wait for all current
 *   ongoing LETIMER synchronization to the LF domain to complete (Normally
 *   synchronization will not be in progress.) However, for this reason, when
 *   using freeze mode, modifications of registers requiring the LF synchronization
 *   should be done within one freeze enable/disable block to avoid unecessary
 *   stalling.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] enable
 *   @li True - enable freeze, modified registers are not propagated to the
 *       LF domain
 *   @li False - disables freeze, modified registers are propagated to the LF
 *       domain
 ******************************************************************************/
void LETIMER_FreezeEnable(LETIMER_TypeDef *letimer, bool enable)
{
  if (enable) {
    /*
     * Wait for any ongoing LF synchronization to complete to
     * protect against the rare case when a user
     * - modifies a register requiring LF sync
     * - then enables freeze before LF sync completed
     * - then modifies the same register again
     * since modifying a register while it is in sync progress should be
     * avoided.
     */
    while (letimer->SYNCBUSY) {
    }

    letimer->FREEZE = LETIMER_FREEZE_REGFREEZE;
  } else {
    letimer->FREEZE = 0;
  }
}
#endif /* defined(_LETIMER_FREEZE_MASK) */

/***************************************************************************//**
 * @brief
 *   Initialize LETIMER.
 *
 * @details
 *   Note that the compare/repeat values must be set separately with
 *   LETIMER_CompareSet() and LETIMER_RepeatSet(). That should probably be done
 *   prior using this function if configuring the LETIMER to start when
 *   initialization is complete.
 *
 * @note
 *   The initialization of the LETIMER modifies the LETIMER CTRL/CMD registers
 *   which require synchronization into the low-frequency domain. If any of those
 *   registers are modified before a previous update to the same register has
 *   completed, this function will stall until the previous synchronization has
 *   completed. This only applies to the Gecko Family. See comments in the
 *   LETIMER_Sync() internal function call.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] init
 *   A pointer to the LETIMER initialization structure.
 ******************************************************************************/
void LETIMER_Init(LETIMER_TypeDef *letimer, const LETIMER_Init_TypeDef *init)
{
  uint32_t tmp = 0;

  EFM_ASSERT(LETIMER_REF_VALID(letimer));

#if defined (LETIMER_EN_EN)
  letimer->EN_SET = LETIMER_EN_EN;
#endif

  /* Stop the timer if specified to be disabled and running. */
  if (!(init->enable) && (letimer->STATUS & LETIMER_STATUS_RUNNING)) {
#if defined(_EFM32_GECKO_FAMILY)
    /* LF register about to be modified requires sync; busy check. */
    regSync(letimer, LETIMER_SYNCBUSY_CMD);
#elif defined(LETIMER_SYNCBUSY_STOP)
    while (letimer->SYNCBUSY & LETIMER_SYNCBUSY_STOP) {
    }
#endif
    letimer->CMD = LETIMER_CMD_STOP;
  }

  /* Configure the DEBUGRUN flag, which sets whether or not the counter should be
   * updated when the debugger is active. */
  if (init->debugRun) {
    tmp |= LETIMER_CTRL_DEBUGRUN;
  }

#if defined(LETIMER_CTRL_RTCC0TEN)
  if (init->rtcComp0Enable) {
    tmp |= LETIMER_CTRL_RTCC0TEN;
  }

  if (init->rtcComp1Enable) {
    tmp |= LETIMER_CTRL_RTCC1TEN;
  }
#endif

  if ((init->comp0Top) || (init->topValue != 0U)) {
#if defined (LETIMER_CTRL_COMP0TOP)
    tmp |= LETIMER_CTRL_COMP0TOP;
    letimer->COMP0 = init->topValue;
#elif defined (LETIMER_CTRL_CNTTOPEN)
    tmp |= LETIMER_CTRL_CNTTOPEN;
    letimer->TOP = init->topValue;
#endif
  }

  if (init->bufTop) {
    tmp |= LETIMER_CTRL_BUFTOP;
  }

  if (init->out0Pol) {
    tmp |= LETIMER_CTRL_OPOL0;
  }

  if (init->out1Pol) {
    tmp |= LETIMER_CTRL_OPOL1;
  }

  tmp |= init->ufoa0 << _LETIMER_CTRL_UFOA0_SHIFT;
  tmp |= init->ufoa1 << _LETIMER_CTRL_UFOA1_SHIFT;
  tmp |= init->repMode << _LETIMER_CTRL_REPMODE_SHIFT;

#if defined(_EFM32_GECKO_FAMILY)
  /* LF register about to be modified requires sync; busy check. */
  regSync(letimer, LETIMER_SYNCBUSY_CTRL);
#endif
  letimer->CTRL = tmp;

  /* Start the timer if specified to be enabled and not already running. */
  if (init->enable && !(letimer->STATUS & LETIMER_STATUS_RUNNING)) {
#if defined(_EFM32_GECKO_FAMILY)
    /* LF register about to be modified requires sync; busy check. */
    regSync(letimer, LETIMER_SYNCBUSY_CMD);
#elif defined(LETIMER_SYNCBUSY_START)
    while (letimer->SYNCBUSY & LETIMER_SYNCBUSY_START) {
    }
#endif
    letimer->CMD = LETIMER_CMD_START;
  }
}

/***************************************************************************//**
 * @brief
 *   Get the LETIMER repeat register value.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] rep
 *   Repeat register to get, either 0 or 1.
 *
 * @return
 *   Repeat register value, 0 if invalid register selected.
 ******************************************************************************/
uint32_t LETIMER_RepeatGet(LETIMER_TypeDef *letimer, unsigned int rep)
{
  uint32_t ret;

  EFM_ASSERT(LETIMER_REF_VALID(letimer) && LETIMER_REP_REG_VALID(rep));

  /* Initialize the selected compare value. */
  switch (rep) {
    case 0:
      ret = letimer->REP0;
      break;

    case 1:
      ret = letimer->REP1;
      break;

    default:
      /* An unknown compare register selected. */
      ret = 0;
      break;
  }

  return(ret);
}

/***************************************************************************//**
 * @brief
 *   Set the LETIMER repeat counter register value.
 *
 * @note
 *   The setting of a repeat counter register requires synchronization into the
 *   low-frequency domain. If the same register is modified before a previous
 *   update has completed, this function will stall until the previous
 *   synchronization has completed. This only applies to the Gecko Family. See
 *   comments in the LETIMER_Sync() internal function call.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] rep
 *   Repeat counter register to set, either 0 or 1.
 *
 * @param[in] value
 *   An initialization value (<= 0x0000ffff).
 ******************************************************************************/
void LETIMER_RepeatSet(LETIMER_TypeDef *letimer,
                       unsigned int rep,
                       uint32_t value)
{
  volatile uint32_t *repReg;
#if defined(_EFM32_GECKO_FAMILY)
  uint32_t          syncbusy;
#endif
  EFM_ASSERT(LETIMER_REF_VALID(letimer)
             && LETIMER_REP_REG_VALID(rep)
             && ((value & ~(_LETIMER_REP0_REP0_MASK
                            >> _LETIMER_REP0_REP0_SHIFT))
                 == 0));

  /* Initialize the selected compare value. */
  switch (rep) {
    case 0:
      repReg = &(letimer->REP0);
#if defined(_EFM32_GECKO_FAMILY)
      syncbusy = LETIMER_SYNCBUSY_REP0;
#endif
      break;

    case 1:
      repReg = &(letimer->REP1);
#if defined(_EFM32_GECKO_FAMILY)
      syncbusy = LETIMER_SYNCBUSY_REP1;
#endif
      break;

    default:
      /* An unknown compare register selected, abort. */
      return;
  }

#if defined(_EFM32_GECKO_FAMILY)
  /* LF register about to be modified requires sync; busy check. */
  regSync(letimer, syncbusy);
#endif

  *repReg = value;
}

/***************************************************************************//**
 * @brief
 *   Reset LETIMER to the same state that it was in after a hardware reset.
 *
 * @note
 *   The ROUTE register is NOT reset by this function to allow for
 *   a centralized setup of this feature.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 ******************************************************************************/
void LETIMER_Reset(LETIMER_TypeDef *letimer)
{
#if defined(_LETIMER_FREEZE_MASK)
  /* Freeze registers to avoid stalling for LF synchronization. */
  LETIMER_FreezeEnable(letimer, true);
#endif

  /* Make sure disabled first, before resetting other registers. */
  letimer->CMD = LETIMER_CMD_STOP | LETIMER_CMD_CLEAR
                 | LETIMER_CMD_CTO0 | LETIMER_CMD_CTO1;
  letimer->CTRL  = _LETIMER_CTRL_RESETVALUE;
  letimer->COMP0 = _LETIMER_COMP0_RESETVALUE;
  letimer->COMP1 = _LETIMER_COMP1_RESETVALUE;
  letimer->REP0  = _LETIMER_REP0_RESETVALUE;
  letimer->REP1  = _LETIMER_REP1_RESETVALUE;
  letimer->IEN   = _LETIMER_IEN_RESETVALUE;
  LETIMER_IntClear(letimer, _LETIMER_IF_MASK);

#if defined(_LETIMER_FREEZE_MASK)
  /* Unfreeze registers and pass new settings to LETIMER. */
  LETIMER_FreezeEnable(letimer, false);
#endif

#if defined(_LETIMER_SYNCBUSY_MASK)
  while (LETIMER0->SYNCBUSY & _LETIMER_SYNCBUSY_MASK) {
  }
#endif

#if defined (LETIMER_EN_EN)
  letimer->EN_CLR = LETIMER_EN_EN;
#endif
}

/***************************************************************************//**
 * @brief
 *   Wait for the LETIMER to complete all synchronization of register changes
 *   and commands.
 ******************************************************************************/
void LETIMER_SyncWait(LETIMER_TypeDef *letimer)
{
#if defined(_SILICON_LABS_32B_SERIES_2)
  while ((letimer->EN != 0U) && (letimer->SYNCBUSY != 0U)) {
    /* Wait for previous synchronization to finish */
  }
#else
  while (letimer->SYNCBUSY != 0U) {
    /* Wait for previous synchronization to finish */
  }
#endif
}

/***************************************************************************//**
 * @brief
 *   Set the LETIMER top value.
 *
 * @note
 *   The LETIMER is a down-counter, so when the counter reaches 0 then the top
 *   value will be loaded into the counter. This function can be used to set
 *   the top value.
 *
 *   If the LETIMER is not already configured to use a top value then this
 *   function will enable that functionality for the user.
 *
 * @param[in] letimer
 *   A pointer to the LETIMER peripheral register block.
 *
 * @param[in] value
 *   The top value. This can be a 16 bit value on series-0 and series-1 devices
 *   and a 24 bit value on series-2 devices.
 ******************************************************************************/
void LETIMER_TopSet(LETIMER_TypeDef *letimer, uint32_t value)
{
  LETIMER_SyncWait(letimer);

#if defined(_LETIMER_TOP_MASK)
  /* Make sure TOP value is enabled. */
  if ((letimer->CTRL & LETIMER_CTRL_CNTTOPEN) == 0U) {
    letimer->CTRL_SET = LETIMER_CTRL_CNTTOPEN;
  }
  letimer->TOP = value;
#else
  /* Make sure TOP value is enabled. */
  if ((letimer->CTRL & LETIMER_CTRL_COMP0TOP) == 0U) {
    letimer->CTRL |= LETIMER_CTRL_COMP0TOP;
  }
  LETIMER_CompareSet(letimer, 0, value);
#endif
}

/** @} (end addtogroup LETIMER) */
/** @} (end addtogroup emlib) */
#endif /* defined(LETIMER_COUNT) && (LETIMER_COUNT > 0) */

/***************************************************************************//**
 * @file em_burtc.c
 * @brief Backup Real Time Counter (BURTC) Peripheral API
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

#include "em_burtc.h"
#if defined(BURTC_PRESENT)

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup BURTC
 * @brief Backup Real Time Counter (BURTC) Peripheral API
 * @details
 *  This module contains functions to control the BURTC peripheral of Silicon
 *  Labs 32-bit MCUs. The Backup Real Time Counter allows timekeeping in all
 *  energy modes. The Backup RTC is also available when the system is in backup
 *  mode.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/***************************************************************************//**
 * @brief Convert dividend to a prescaler logarithmic value. Only works for even
 *        numbers equal to 2^n.
 * @param[in] div Unscaled dividend,
 * @return Base 2 logarithm of input, as used by fixed prescalers.
 ******************************************************************************/
__STATIC_INLINE uint32_t divToLog2(uint32_t div)
{
  uint32_t log2;

  /* Prescaler accepts an argument of 128 or less, valid values being 2^n. */
  EFM_ASSERT((div > 0) && (div <= 32768));

  /* Count leading zeroes and "reverse" result, Cortex-M3 intrinsic. */
  log2 = (31 - __CLZ(div));

  return log2;
}

/***************************************************************************//**
 * @brief
 *   Wait for an ongoing sync of register(s) to low frequency domain to complete.
 *
 * @param[in] mask
 *   A bitmask corresponding to SYNCBUSY register defined bits, indicating
 *   registers that must complete any ongoing synchronization.
 ******************************************************************************/
__STATIC_INLINE void regSync(uint32_t mask)
{
#if defined(_BURTC_FREEZE_MASK)
  /* Avoid deadlock if modifying the same register twice when freeze mode is
     activated or when a clock is not selected for the BURTC. If a clock is
     not selected, then the sync is done once the clock source is set. */
  if ((BURTC->FREEZE & BURTC_FREEZE_REGFREEZE)
      || ((BURTC->CTRL & _BURTC_CTRL_CLKSEL_MASK) == BURTC_CTRL_CLKSEL_NONE)
      || ((BURTC->CTRL & _BURTC_CTRL_RSTEN_MASK) == BURTC_CTRL_RSTEN)) {
    return;
  }
#endif

  /* Wait for any pending previous write operation to complete */
  /* in low frequency domain. This is only required for the Gecko Family. */
  while ((BURTC->SYNCBUSY & mask) != 0U)
    ;
}
/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief Initialize BURTC.
 *
 * @details
 *    Configures the BURTC peripheral.
 *
 * @note
 *   Before initialization, BURTC module must first be enabled by clearing the
 *   reset bit in the RMU, i.e.,
 * @verbatim
 *   RMU_ResetControl(rmuResetBU, rmuResetModeClear);
 * @endverbatim
 *   Compare channel 0 must be configured outside this function, before
 *   initialization if enable is set to true. The counter will always be reset.
 *
 * @param[in] burtcInit
 *   A pointer to the BURTC initialization structure.
 ******************************************************************************/
void BURTC_Init(const BURTC_Init_TypeDef *burtcInit)
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  uint32_t ctrl;
  uint32_t presc;

  /* Check initializer structure integrity. */
  EFM_ASSERT(burtcInit != (BURTC_Init_TypeDef *) 0);
  /* Clock divider must be between 1 and 128, really on the form 2^n. */
  EFM_ASSERT((burtcInit->clkDiv >= 1) && (burtcInit->clkDiv <= 128));

  /* Ignored compare bits during low power operation must be less than 7. */
  /* Note! Giant Gecko revision C errata, do NOT use LPCOMP=7. */
  EFM_ASSERT(burtcInit->lowPowerComp <= 6);
  /* You cannot enable the BURTC if mode is set to disabled. */
  EFM_ASSERT((burtcInit->enable == false)
             || ((burtcInit->enable == true)
                 && (burtcInit->mode != burtcModeDisable)));
  /* Low power mode is only available with LFRCO or LFXO as clock source. */
  EFM_ASSERT((burtcInit->clkSel != burtcClkSelULFRCO)
             || ((burtcInit->clkSel == burtcClkSelULFRCO)
                 && (burtcInit->lowPowerMode == burtcLPDisable)));

  /* Calculate a prescaler value from the clock divider input. */
  /* Note! If clock select (clkSel) is ULFRCO, a clock divisor (clkDiv) of
     value 1 will select a 2 kHz ULFRCO clock, while any other value will
     select a 1 kHz ULFRCO clock source. */
  presc = divToLog2(burtcInit->clkDiv);

  /* Make sure all registers are updated simultaneously. */
  if (burtcInit->enable) {
    BURTC_FreezeEnable(true);
  }

  /* Modification of LPMODE register requires sync with potential ongoing
   * register updates in LF domain. */
  regSync(BURTC_SYNCBUSY_LPMODE);

  /* Configure low power mode. */
  BURTC->LPMODE = (uint32_t) (burtcInit->lowPowerMode);

  /* New configuration. */
  ctrl = (BURTC_CTRL_RSTEN
          | (burtcInit->mode)
          | (burtcInit->debugRun << _BURTC_CTRL_DEBUGRUN_SHIFT)
          | (burtcInit->compare0Top << _BURTC_CTRL_COMP0TOP_SHIFT)
          | (burtcInit->lowPowerComp << _BURTC_CTRL_LPCOMP_SHIFT)
          | (presc << _BURTC_CTRL_PRESC_SHIFT)
          | (burtcInit->clkSel)
          | (burtcInit->timeStamp << _BURTC_CTRL_BUMODETSEN_SHIFT));

  /* Clear interrupts. */
  BURTC_IntClear(0xFFFFFFFF);

  /* Set the new configuration. */
  BURTC->CTRL = ctrl;

  /* Enable BURTC and counter. */
  if (burtcInit->enable) {
    /* To enable BURTC counter, disable reset. */
    BURTC_Enable(true);

    /* Clear freeze. */
    BURTC_FreezeEnable(false);
  }
#elif defined(_SILICON_LABS_32B_SERIES_2)
  uint32_t presc;

  presc = divToLog2(burtcInit->clkDiv);

  if (BURTC->EN != 0U) {
    BURTC_SyncWait();
  }
  BURTC->EN_CLR = BURTC_EN_EN;
  regSync(BURTC_SYNCBUSY_EN);
  BURTC->CFG = (presc << _BURTC_CFG_CNTPRESC_SHIFT)
               | ((burtcInit->compare0Top ? 1U : 0U) << _BURTC_CFG_COMPTOP_SHIFT)
               | ((burtcInit->debugRun ? 1U : 0U) << _BURTC_CFG_DEBUGRUN_SHIFT);
  BURTC->EM4WUEN = ((burtcInit->em4comp ? 1U : 0U) << _BURTC_EM4WUEN_COMPEM4WUEN_SHIFT)
                   | ((burtcInit->em4overflow ? 1U : 0U) << _BURTC_EM4WUEN_OFEM4WUEN_SHIFT);
  BURTC->EN_SET = BURTC_EN_EN;
  if (burtcInit->start) {
    BURTC_Start();
  }
#endif
}

#if defined(_SILICON_LABS_32B_SERIES_2)
/***************************************************************************//**
 * @brief
 *   Enable or Disable BURTC peripheral.
 *
 * @param[in] enable
 *   true to enable, false to disable.
 ******************************************************************************/
void BURTC_Enable(bool enable)
{
  regSync(BURTC_SYNCBUSY_EN);

  if ((BURTC->EN == 0U) && !enable) {
    /* Trying to disable BURTC when it's already disabled */
    return;
  }

  if (BURTC->EN != 0U) {
    /* Modifying the enable bit while synchronization is active will BusFault */
    BURTC_SyncWait();
  }

  if (enable) {
    BURTC->EN_SET = BURTC_EN_EN;
  } else {
    BURTC_Stop();
    BURTC_SyncWait(); /* Wait for the stop to synchronize */
    BURTC->EN_CLR = BURTC_EN_EN;
  }
}
#elif defined(_SILICON_LABS_32B_SERIES_0)
/***************************************************************************//**
 * @brief
 *   Enable or Disable BURTC peripheral reset and start counter
 * @param[in] enable
 *   If true; asserts reset to BURTC, halts counter, if false; deassert reset
 ******************************************************************************/
void BURTC_Enable(bool enable)
{
  /* Note! If mode is disabled, BURTC counter will not start */
  EFM_ASSERT(((enable == true)
              && ((BURTC->CTRL & _BURTC_CTRL_MODE_MASK)
                  != BURTC_CTRL_MODE_DISABLE))
             || (enable == false));
  BUS_RegBitWrite(&BURTC->CTRL, _BURTC_CTRL_RSTEN_SHIFT, (uint32_t) !enable);
}
#endif

/***************************************************************************//**
 * @brief Set BURTC compare channel.
 *
 * @param[in] comp Compare the channel index, must be 0 for current devices.
 *
 * @param[in] value New compare value.
 ******************************************************************************/
void BURTC_CompareSet(unsigned int comp, uint32_t value)
{
  (void) comp;  /* Unused parameter when EFM_ASSERT is undefined. */

  EFM_ASSERT(comp == 0U);

#if defined(_BURTC_COMP0_MASK)
  /* Modification of COMP0 register requires sync with potential ongoing
   * register updates in LF domain. */
  regSync(BURTC_SYNCBUSY_COMP0);

  /* Configure compare channel 0/. */
  BURTC->COMP0 = value;
#else
  /* Wait for last potential write to complete. */
  regSync(BURTC_SYNCBUSY_COMP);

  /* Configure compare channel 0 */
  BURTC->COMP = value;
  regSync(BURTC_SYNCBUSY_COMP);
#endif
}

/***************************************************************************//**
 * @brief Get the BURTC compare value.
 *
 * @param[in] comp Compare the channel index value, must be 0 for Giant/Leopard Gecko.
 *
 * @return The currently configured value for this compare channel.
 ******************************************************************************/
uint32_t BURTC_CompareGet(unsigned int comp)
{
  (void) comp;  /* Unused parameter when EFM_ASSERT is undefined. */

  EFM_ASSERT(comp == 0U);
#if defined(_BURTC_COMP0_MASK)
  return BURTC->COMP0;
#else
  return BURTC->COMP;
#endif
}

/***************************************************************************//**
 * @brief Reset counter
 ******************************************************************************/
void BURTC_CounterReset(void)
{
#if defined(_BURTC_CTRL_MASK)
  /* Set and clear reset bit */
  BUS_RegBitWrite(&BURTC->CTRL, _BURTC_CTRL_RSTEN_SHIFT, 1U);
  BUS_RegBitWrite(&BURTC->CTRL, _BURTC_CTRL_RSTEN_SHIFT, 0U);
#else
  BURTC_Stop();
  BURTC->CNT = 0U;
  BURTC_Start();
#endif
}

/***************************************************************************//**
 * @brief
 *   Restore BURTC to reset state.
 * @note
 *   Before accessing the BURTC, BURSTEN in RMU->CTRL must be cleared.
 *   LOCK will not be reset to default value, as this will disable access
 *   to core BURTC registers.
 ******************************************************************************/
void BURTC_Reset(void)
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  bool buResetState;

  /* Read reset state, set reset, and restore state. */
  buResetState = BUS_RegBitRead(&RMU->CTRL, _RMU_CTRL_BURSTEN_SHIFT);
  BUS_RegBitWrite(&RMU->CTRL, _RMU_CTRL_BURSTEN_SHIFT, 1);
  BUS_RegBitWrite(&RMU->CTRL, _RMU_CTRL_BURSTEN_SHIFT, buResetState);
#elif defined(_SILICON_LABS_32B_SERIES_2)
  if (BURTC->EN != 0U) {
    BURTC_SyncWait();
  }
  BURTC->EN_SET = BURTC_EN_EN;
  BURTC_Stop();
  BURTC->CNT     = 0x0;
  BURTC->PRECNT  = 0x0;
  BURTC->COMP    = 0x0;
  BURTC->EM4WUEN = _BURTC_EM4WUEN_RESETVALUE;
  BURTC->IEN     = _BURTC_IEN_RESETVALUE;
  BURTC->IF_CLR  = _BURTC_IF_MASK;
  /* Wait for all values to synchronize. BusFaults can happen if we don't
   * do this before the enable bit is cleared. */
  BURTC_SyncWait();
  BURTC->EN_CLR  = BURTC_EN_EN;
  BURTC_SyncWait();
  BURTC->CFG = _BURTC_CFG_RESETVALUE;
#endif
}

#if defined(_BURTC_CTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Get the clock frequency of the BURTC.
 *
 * @return
 *   The current frequency in Hz.
 ******************************************************************************/
uint32_t BURTC_ClockFreqGet(void)
{
  uint32_t clkSel;
  uint32_t clkDiv;
  uint32_t frequency;

  clkSel = BURTC->CTRL & _BURTC_CTRL_CLKSEL_MASK;
  clkDiv = (BURTC->CTRL & _BURTC_CTRL_PRESC_MASK) >> _BURTC_CTRL_PRESC_SHIFT;

  switch (clkSel) {
    /** Ultra-low frequency (1 kHz) clock. */
    case BURTC_CTRL_CLKSEL_ULFRCO:
      if (_BURTC_CTRL_PRESC_DIV1 == clkDiv) {
        frequency = 2000;     /* 2 kHz when clock divisor is 1. */
      } else {
        frequency = SystemULFRCOClockGet();  /* 1 kHz when divisor is different
                                                from 1. */
      }
      break;

    /** Low-frequency RC oscillator. */
    case BURTC_CTRL_CLKSEL_LFRCO:
      frequency = SystemLFRCOClockGet() / (1 << clkDiv); /* freq=32768/2^clkDiv */
      break;

    /** Low-frequency crystal oscillator. */
    case BURTC_CTRL_CLKSEL_LFXO:
      frequency = SystemLFXOClockGet() / (1 << clkDiv); /* freq=32768/2^clkDiv */
      break;

    default:
      /* No clock selected for BURTC. */
      frequency = 0;
  }
  return frequency;
}
#endif

/** @} (end addtogroup BURTC) */
/** @} (end addtogroup emlib) */

#endif /* BURTC_PRESENT */

/***************************************************************************//**
 * @file system_efr32fg1p.c
 * @brief CMSIS Cortex-M3/M4 System Layer for EFR32 devices.
 * @version 5.6.0
 ******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories, Inc. www.silabs.com</b>
 ******************************************************************************
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
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Laboratories, Inc.
 * has no obligation to support this Software. Silicon Laboratories, Inc. is
 * providing the Software "AS IS", with no express or implied warranties of any
 * kind, including, but not limited to, any implied warranties of
 * merchantability or fitness for any particular purpose or warranties against
 * infringement of any proprietary rights of a third party.
 *
 * Silicon Laboratories, Inc. will not be liable for any consequential,
 * incidental, or special damages, or any other relief, or for any claim by
 * any third party, arising from your use of this Software.
 *
 *****************************************************************************/

#include <stdint.h>
#include "em_device.h"

/*******************************************************************************
 ******************************   DEFINES   ************************************
 ******************************************************************************/

/** LFRCO frequency, tuned to below frequency during manufacturing. */
#define EFR32_LFRCO_FREQ  (32768UL)
/** ULFRCO frequency */
#define EFR32_ULFRCO_FREQ (1000UL)

/*******************************************************************************
 **************************   LOCAL VARIABLES   ********************************
 ******************************************************************************/

/* System oscillator frequencies. These frequencies are normally constant */
/* for a target, but they are made configurable in order to allow run-time */
/* handling of different boards. The crystal oscillator clocks can be set */
/* compile time to a non-default value by defining respective EFR32_nFXO_FREQ */
/* values according to board design. By defining the EFR32_nFXO_FREQ to 0, */
/* one indicates that the oscillator is not present, in order to save some */
/* SW footprint. */

#ifndef EFR32_HFRCO_MAX_FREQ
/** Maximum HFRCO frequency */
#define EFR32_HFRCO_MAX_FREQ            (38000000UL)
#endif

#ifndef EFR32_HFXO_FREQ
/** HFXO frequency */
#define EFR32_HFXO_FREQ                 (38400000UL)
#endif

#ifndef EFR32_HFRCO_STARTUP_FREQ
/** HFRCO startup frequency */
#define EFR32_HFRCO_STARTUP_FREQ        (19000000UL)
#endif

/* Do not define variable if HF crystal oscillator not present */
#if (EFR32_HFXO_FREQ > 0U)
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/** System HFXO clock. */
static uint32_t SystemHFXOClock = EFR32_HFXO_FREQ;
/** @endcond (DO_NOT_INCLUDE_WITH_DOXYGEN) */
#endif

#ifndef EFR32_LFXO_FREQ
/** LFXO frequency */
#define EFR32_LFXO_FREQ (EFR32_LFRCO_FREQ)
#endif
/* Do not define variable if LF crystal oscillator not present */
#if (EFR32_LFXO_FREQ > 0U)
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/** System LFXO clock. */
static uint32_t SystemLFXOClock = EFR32_LFXO_FREQ;
/** @endcond (DO_NOT_INCLUDE_WITH_DOXYGEN) */
#endif

/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/

/**
 * @brief
 *   System System Clock Frequency (Core Clock).
 *
 * @details
 *   Required CMSIS global variable that must be kept up-to-date.
 */
uint32_t SystemCoreClock = EFR32_HFRCO_STARTUP_FREQ;

/**
 * @brief
 *   System HFRCO frequency
 *
 * @note
 *   This is an EFR32 proprietary variable, not part of the CMSIS definition.
 *
 * @details
 *   Frequency of the system HFRCO oscillator
 */
uint32_t SystemHfrcoFreq = EFR32_HFRCO_STARTUP_FREQ;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
#if defined(__ICCARM__)    /* IAR requires the __vector_table symbol */
#define __Vectors    __vector_table
#endif
extern uint32_t __Vectors;
#endif

/***************************************************************************//**
 * @brief
 *   Get the current core clock frequency.
 *
 * @details
 *   Calculate and get the current core clock frequency based on the current
 *   configuration. Assuming that the SystemCoreClock global variable is
 *   maintained, the core clock frequency is stored in that variable as well.
 *   This function will however calculate the core clock based on actual HW
 *   configuration. It will also update the SystemCoreClock global variable.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   The current core clock frequency in Hz.
 ******************************************************************************/
uint32_t SystemCoreClockGet(void)
{
  uint32_t ret;
  uint32_t presc;

  ret   = SystemHFClockGet();
  presc = (CMU->HFCOREPRESC & _CMU_HFCOREPRESC_PRESC_MASK)
          >> _CMU_HFCOREPRESC_PRESC_SHIFT;
  ret  /= presc + 1U;

  /* Keep CMSIS system clock variable up-to-date */
  SystemCoreClock = ret;

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Get the maximum core clock frequency.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   The maximum core clock frequency in Hz.
 ******************************************************************************/
uint32_t SystemMaxCoreClockGet(void)
{
#if (EFR32_HFRCO_MAX_FREQ > EFR32_HFXO_FREQ)
  return EFR32_HFRCO_MAX_FREQ;
#else
  return EFR32_HFXO_FREQ;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get the current HFCLK frequency.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   The current HFCLK frequency in Hz.
 ******************************************************************************/
uint32_t SystemHFClockGet(void)
{
  uint32_t ret;

  switch (CMU->HFCLKSTATUS & _CMU_HFCLKSTATUS_SELECTED_MASK) {
    case CMU_HFCLKSTATUS_SELECTED_LFXO:
#if (EFR32_LFXO_FREQ > 0U)
      ret = SystemLFXOClock;
#else
      /* We should not get here, since core should not be clocked. May */
      /* be caused by a misconfiguration though. */
      ret = 0U;
#endif
      break;

    case CMU_HFCLKSTATUS_SELECTED_LFRCO:
      ret = EFR32_LFRCO_FREQ;
      break;

    case CMU_HFCLKSTATUS_SELECTED_HFXO:
#if (EFR32_HFXO_FREQ > 0U)
      ret = SystemHFXOClock;
#else
      /* We should not get here, since core should not be clocked. May */
      /* be caused by a misconfiguration though. */
      ret = 0U;
#endif
      break;

    default: /* CMU_HFCLKSTATUS_SELECTED_HFRCO */
      ret = SystemHfrcoFreq;
      break;
  }

  return ret / (1U + ((CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK)
                      >> _CMU_HFPRESC_PRESC_SHIFT));
}

/**************************************************************************//**
 * @brief
 *   Get high frequency crystal oscillator clock frequency for target system.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   HFXO frequency in Hz.
 *****************************************************************************/
uint32_t SystemHFXOClockGet(void)
{
  /* External crystal oscillator present? */
#if (EFR32_HFXO_FREQ > 0U)
  return SystemHFXOClock;
#else
  return 0U;
#endif
}

/**************************************************************************//**
 * @brief
 *   Set high frequency crystal oscillator clock frequency for target system.
 *
 * @note
 *   This function is mainly provided for being able to handle target systems
 *   with different HF crystal oscillator frequencies run-time. If used, it
 *   should probably only be used once during system startup.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @param[in] freq
 *   HFXO frequency in Hz used for target.
 *****************************************************************************/
void SystemHFXOClockSet(uint32_t freq)
{
  /* External crystal oscillator present? */
#if (EFR32_HFXO_FREQ > 0U)
  SystemHFXOClock = freq;

  /* Update core clock frequency if HFXO is used to clock core */
  if ((CMU->HFCLKSTATUS & _CMU_HFCLKSTATUS_SELECTED_MASK)
      == CMU_HFCLKSTATUS_SELECTED_HFXO) {
    /* The function will update the global variable */
    (void)SystemCoreClockGet();
  }
#else
  (void)freq; /* Unused parameter */
#endif
}

/**************************************************************************//**
 * @brief
 *   Initialize the system.
 *
 * @details
 *   Do required generic HW system init.
 *
 * @note
 *   This function is invoked during system init, before the main() routine
 *   and any data has been initialized. For this reason, it cannot do any
 *   initialization of variables etc.
 *****************************************************************************/
void SystemInit(void)
{
#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  SCB->VTOR = (uint32_t)&__Vectors;
#endif

#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
  /* Set floating point coprosessor access mode. */
  SCB->CPACR |= ((3UL << 10 * 2)                    /* set CP10 Full Access */
                 | (3UL << 11 * 2));                /* set CP11 Full Access */
#endif

#if defined(UNALIGNED_SUPPORT_DISABLE)
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif

  /****************************
   * Fix for errata DCDC_E206
   * Enable bypass switch as errata workaround. The bypass current limit will be
   * disabled again in CHIP_Init() to avoid added current consumption. */

  EMU->DCDCCLIMCTRL |= 1U << _EMU_DCDCCLIMCTRL_BYPLIMEN_SHIFT;
  EMU->DCDCCTRL = (EMU->DCDCCTRL & ~_EMU_DCDCCTRL_DCDCMODE_MASK)
                  | EMU_DCDCCTRL_DCDCMODE_BYPASS;
  *(volatile uint32_t *)(0x400E3074) &= ~(0x1UL << 0);
  *(volatile uint32_t *)(0x400E3060) &= ~(0x1UL << 28);
}

/**************************************************************************//**
 * @brief
 *   Get low frequency RC oscillator clock frequency for target system.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   LFRCO frequency in Hz.
 *****************************************************************************/
uint32_t SystemLFRCOClockGet(void)
{
  /* Currently we assume that this frequency is properly tuned during */
  /* manufacturing and is not changed after reset. If future requirements */
  /* for re-tuning by user, we can add support for that. */
  return EFR32_LFRCO_FREQ;
}

/**************************************************************************//**
 * @brief
 *   Get ultra low frequency RC oscillator clock frequency for target system.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   ULFRCO frequency in Hz.
 *****************************************************************************/
uint32_t SystemULFRCOClockGet(void)
{
  /* The ULFRCO frequency is not tuned, and can be very inaccurate */
  return EFR32_ULFRCO_FREQ;
}

/**************************************************************************//**
 * @brief
 *   Get low frequency crystal oscillator clock frequency for target system.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @return
 *   LFXO frequency in Hz.
 *****************************************************************************/
uint32_t SystemLFXOClockGet(void)
{
  /* External crystal oscillator present? */
#if (EFR32_LFXO_FREQ > 0U)
  return SystemLFXOClock;
#else
  return 0U;
#endif
}

/**************************************************************************//**
 * @brief
 *   Set low frequency crystal oscillator clock frequency for target system.
 *
 * @note
 *   This function is mainly provided for being able to handle target systems
 *   with different HF crystal oscillator frequencies run-time. If used, it
 *   should probably only be used once during system startup.
 *
 * @note
 *   This is an EFR32 proprietary function, not part of the CMSIS definition.
 *
 * @param[in] freq
 *   LFXO frequency in Hz used for target.
 *****************************************************************************/
void SystemLFXOClockSet(uint32_t freq)
{
  /* External crystal oscillator present? */
#if (EFR32_LFXO_FREQ > 0U)
  SystemLFXOClock = freq;

  /* Update core clock frequency if LFXO is used to clock core */
  if ((CMU->HFCLKSTATUS & _CMU_HFCLKSTATUS_SELECTED_MASK)
      == CMU_HFCLKSTATUS_SELECTED_LFXO) {
    /* The function will update the global variable */
    (void)SystemCoreClockGet();
  }
#else
  (void)freq; /* Unused parameter */
#endif
}

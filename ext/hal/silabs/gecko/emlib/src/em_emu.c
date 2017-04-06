/***************************************************************************//**
 * @file em_emu.c
 * @brief Energy Management Unit (EMU) Peripheral API
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

#include <limits.h>

#include "em_emu.h"
#if defined( EMU_PRESENT ) && ( EMU_COUNT > 0 )

#include "em_cmu.h"
#include "em_system.h"
#include "em_common.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup EMU
 * @brief Energy Management Unit (EMU) Peripheral API
 * @details
 *  This module contains functions to control the EMU peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The EMU handles the different low energy modes
 *  in Silicon Labs microcontrollers.
 * @{
 ******************************************************************************/

/* Consistency check, since restoring assumes similar bitpositions in */
/* CMU OSCENCMD and STATUS regs */
#if (CMU_STATUS_AUXHFRCOENS != CMU_OSCENCMD_AUXHFRCOEN)
#error Conflict in AUXHFRCOENS and AUXHFRCOEN bitpositions
#endif
#if (CMU_STATUS_HFXOENS != CMU_OSCENCMD_HFXOEN)
#error Conflict in HFXOENS and HFXOEN bitpositions
#endif
#if (CMU_STATUS_LFRCOENS != CMU_OSCENCMD_LFRCOEN)
#error Conflict in LFRCOENS and LFRCOEN bitpositions
#endif
#if (CMU_STATUS_LFXOENS != CMU_OSCENCMD_LFXOEN)
#error Conflict in LFXOENS and LFXOEN bitpositions
#endif

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if defined( _SILICON_LABS_32B_SERIES_0 )
/* Fix for errata EMU_E107 - non-WIC interrupt masks.
 * Zero Gecko and future families are not affected by errata EMU_E107 */
#if defined( _EFM32_GECKO_FAMILY )
#define ERRATA_FIX_EMU_E107_EN
#define NON_WIC_INT_MASK_0    (~(0x0dfc0323U))
#define NON_WIC_INT_MASK_1    (~(0x0U))

#elif defined( _EFM32_TINY_FAMILY )
#define ERRATA_FIX_EMU_E107_EN
#define NON_WIC_INT_MASK_0    (~(0x001be323U))
#define NON_WIC_INT_MASK_1    (~(0x0U))

#elif defined( _EFM32_GIANT_FAMILY )
#define ERRATA_FIX_EMU_E107_EN
#define NON_WIC_INT_MASK_0    (~(0xff020e63U))
#define NON_WIC_INT_MASK_1    (~(0x00000046U))

#elif defined( _EFM32_WONDER_FAMILY )
#define ERRATA_FIX_EMU_E107_EN
#define NON_WIC_INT_MASK_0    (~(0xff020e63U))
#define NON_WIC_INT_MASK_1    (~(0x00000046U))

#endif
#endif

/* Fix for errata EMU_E108 - High Current Consumption on EM4 Entry. */
#if defined(_SILICON_LABS_32B_SERIES_0) && defined( _EFM32_HAPPY_FAMILY )
#define ERRATA_FIX_EMU_E108_EN
#endif

/* Fix for errata EMU_E208 - Occasional Full Reset After Exiting EM4H */
#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
#define ERRATA_FIX_EMU_E208_EN
#endif

/* Enable FETCNT tuning errata fix */
#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
#define ERRATA_FIX_DCDC_FETCNT_SET_EN
#endif

/* Enable LN handshake errata fix */
#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
#define ERRATA_FIX_DCDC_LNHS_BLOCK_EN
typedef enum
{
  errataFixDcdcHsInit,
  errataFixDcdcHsTrimSet,
  errataFixDcdcHsBypassLn,
  errataFixDcdcHsLnWaitDone
} errataFixDcdcHs_TypeDef;
static errataFixDcdcHs_TypeDef errataFixDcdcHsState = errataFixDcdcHsInit;
#endif

/* Used to figure out if a memory address is inside or outside of a RAM block.
 * A memory address is inside a RAM block if the address is greater than the
 * RAM block address. */
#define ADDRESS_NOT_IN_BLOCK(addr, block)  ((addr) <= (block))

/* RAM Block layout for various device families. Note that some devices
 * have special layout in RAM0. */
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84)
#define RAM1_BLOCKS            2
#define RAM1_BLOCK_SIZE  0x10000  // 64 kB blocks
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_89)
#define RAM0_BLOCKS            2
#define RAM0_BLOCK_SIZE   0x4000
#define RAM1_BLOCKS            2
#define RAM1_BLOCK_SIZE   0x4000  // 16 kB blocks
#elif defined(_SILICON_LABS_32B_SERIES_0) && defined(_EFM32_GIANT_FAMILY)
#define RAM0_BLOCKS            4
#define RAM0_BLOCK_SIZE   0x8000  // 32 kB blocks
#elif defined(_SILICON_LABS_32B_SERIES_0) && defined(_EFM32_GECKO_FAMILY)
#define RAM0_BLOCKS            4
#define RAM0_BLOCK_SIZE   0x1000  //  4 kB blocks
#endif

#if defined(_SILICON_LABS_32B_SERIES_0)
/* RAM_MEM_END on Gecko devices have a value larger than the SRAM_SIZE */
#define RAM0_END    (SRAM_BASE + SRAM_SIZE - 1)
#else
#define RAM0_END    RAM_MEM_END
#endif

/** @endcond */


#if defined( _EMU_DCDCCTRL_MASK )
/* DCDCTODVDD output range min/max */
#if !defined(PWRCFG_DCDCTODVDD_VMIN)
#define PWRCFG_DCDCTODVDD_VMIN          1800
#endif
#if !defined(PWRCFG_DCDCTODVDD_VMAX)
#define PWRCFG_DCDCTODVDD_VMAX          3000
#endif
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Static user configuration */
#if defined( _EMU_DCDCCTRL_MASK )
static uint16_t dcdcMaxCurrent_mA;
static uint16_t dcdcEm01LoadCurrent_mA;
static EMU_DcdcLnReverseCurrentControl_TypeDef dcdcReverseCurrentControl;
#endif
#if defined( _EMU_CMD_EM01VSCALE0_MASK )
static EMU_EM01Init_TypeDef vScaleEM01Config = {false};
#endif
/** @endcond */


/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

#if defined( _EMU_CMD_EM01VSCALE0_MASK )
/* Convert from level to EM0 and 1 command bit */
__STATIC_INLINE uint32_t vScaleEM01Cmd(EMU_VScaleEM01_TypeDef level)
{
  return EMU_CMD_EM01VSCALE0 << (_EMU_STATUS_VSCALE_VSCALE0 - (uint32_t)level);
}
#endif

/***************************************************************************//**
 * @brief
 *   Save/restore/update oscillator, core clock and voltage scaling configuration on
 *   EM2 or EM3 entry/exit.
 *
 * @details
 *   Hardware may automatically change oscillator and voltage scaling configuration
 *   when going into or out of an energy mode. Static data in this function keeps track of
 *   such configuration bits and is used to restore state if needed.
 *
 ******************************************************************************/
typedef enum
{
  emState_Save,         /* Save EMU and CMU state */
  emState_Restore,      /* Restore and unlock     */
} emState_TypeDef;

static void emState(emState_TypeDef action)
{
  uint32_t oscEnCmd;
  uint32_t cmuLocked;
  static uint32_t cmuStatus;
  static CMU_Select_TypeDef hfClock;
#if defined( _EMU_CMD_EM01VSCALE0_MASK )
  static uint8_t vScaleStatus;
#endif


  /* Save or update state */
  if (action == emState_Save)
  {
    /* Save configuration. */
    cmuStatus = CMU->STATUS;
    hfClock = CMU_ClockSelectGet(cmuClock_HF);
#if defined( _EMU_CMD_EM01VSCALE0_MASK )
    /* Save vscale */
    EMU_VScaleWait();
    vScaleStatus   = (uint8_t)((EMU->STATUS & _EMU_STATUS_VSCALE_MASK)
                               >> _EMU_STATUS_VSCALE_SHIFT);
#endif
  }
  else if (action == emState_Restore) /* Restore state */
  {
    /* Apply saved configuration. */
#if defined( _EMU_CMD_EM01VSCALE0_MASK )
    /* Restore EM0 and 1 voltage scaling level. EMU_VScaleWait() is called later,
       just before HF clock select is set. */
    EMU->CMD = vScaleEM01Cmd((EMU_VScaleEM01_TypeDef)vScaleStatus);
#endif

    /* CMU registers may be locked */
    cmuLocked = CMU->LOCK & CMU_LOCK_LOCKKEY_LOCKED;
    CMU_Unlock();

    /* AUXHFRCO are automatically disabled (except if using debugger). */
    /* HFRCO, USHFRCO and HFXO are automatically disabled. */
    /* LFRCO/LFXO may be disabled by SW in EM3. */
    /* Restore according to status prior to entering energy mode. */
    oscEnCmd = 0;
    oscEnCmd |= ((cmuStatus & CMU_STATUS_HFRCOENS)    ? CMU_OSCENCMD_HFRCOEN : 0);
    oscEnCmd |= ((cmuStatus & CMU_STATUS_AUXHFRCOENS) ? CMU_OSCENCMD_AUXHFRCOEN : 0);
    oscEnCmd |= ((cmuStatus & CMU_STATUS_LFRCOENS)    ? CMU_OSCENCMD_LFRCOEN : 0);
    oscEnCmd |= ((cmuStatus & CMU_STATUS_HFXOENS)     ? CMU_OSCENCMD_HFXOEN : 0);
    oscEnCmd |= ((cmuStatus & CMU_STATUS_LFXOENS)     ? CMU_OSCENCMD_LFXOEN : 0);
#if defined( _CMU_STATUS_USHFRCOENS_MASK )
    oscEnCmd |= ((cmuStatus & CMU_STATUS_USHFRCOENS)  ? CMU_OSCENCMD_USHFRCOEN : 0);
#endif
    CMU->OSCENCMD = oscEnCmd;

#if defined( _EMU_STATUS_VSCALE_MASK )
    /* Wait for upscale to complete and then restore selected clock */
    EMU_VScaleWait();
#endif

    if (hfClock != cmuSelect_HFRCO)
    {
      CMU_ClockSelectSet(cmuClock_HF, hfClock);
    }

    /* If HFRCO was disabled before entering Energy Mode, turn it off again */
    /* as it is automatically enabled by wake up */
    if ( ! (cmuStatus & CMU_STATUS_HFRCOENS) )
    {
      CMU->OSCENCMD = CMU_OSCENCMD_HFRCODIS;
    }

    /* Restore CMU register locking */
    if (cmuLocked)
    {
      CMU_Lock();
    }
  }
}


#if defined( ERRATA_FIX_EMU_E107_EN )
/* Get enable conditions for errata EMU_E107 fix. */
__STATIC_INLINE bool getErrataFixEmuE107En(void)
{
  /* SYSTEM_ChipRevisionGet could have been used here, but we would like a
   * faster implementation in this case.
   */
  uint16_t majorMinorRev;

  /* CHIP MAJOR bit [3:0] */
  majorMinorRev = ((ROMTABLE->PID0 & _ROMTABLE_PID0_REVMAJOR_MASK)
                   >> _ROMTABLE_PID0_REVMAJOR_SHIFT)
                  << 8;
  /* CHIP MINOR bit [7:4] */
  majorMinorRev |= ((ROMTABLE->PID2 & _ROMTABLE_PID2_REVMINORMSB_MASK)
                    >> _ROMTABLE_PID2_REVMINORMSB_SHIFT)
                   << 4;
  /* CHIP MINOR bit [3:0] */
  majorMinorRev |= (ROMTABLE->PID3 & _ROMTABLE_PID3_REVMINORLSB_MASK)
                   >> _ROMTABLE_PID3_REVMINORLSB_SHIFT;

#if defined( _EFM32_GECKO_FAMILY )
  return (majorMinorRev <= 0x0103);
#elif defined( _EFM32_TINY_FAMILY )
  return (majorMinorRev <= 0x0102);
#elif defined( _EFM32_GIANT_FAMILY )
  return (majorMinorRev <= 0x0103) || (majorMinorRev == 0x0204);
#elif defined( _EFM32_WONDER_FAMILY )
  return (majorMinorRev == 0x0100);
#else
  /* Zero Gecko and future families are not affected by errata EMU_E107 */
  return false;
#endif
}
#endif

/* LP prepare / LN restore P/NFET count */
#define DCDC_LP_PFET_CNT        7
#define DCDC_LP_NFET_CNT        7
#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
static void currentLimitersUpdate(void);
static void dcdcFetCntSet(bool lpModeSet)
{
  uint32_t tmp;
  static uint32_t emuDcdcMiscCtrlReg;

  if (lpModeSet)
  {
    emuDcdcMiscCtrlReg = EMU->DCDCMISCCTRL;
    tmp  = EMU->DCDCMISCCTRL
           & ~(_EMU_DCDCMISCCTRL_PFETCNT_MASK | _EMU_DCDCMISCCTRL_NFETCNT_MASK);
    tmp |= (DCDC_LP_PFET_CNT << _EMU_DCDCMISCCTRL_PFETCNT_SHIFT)
            | (DCDC_LP_NFET_CNT << _EMU_DCDCMISCCTRL_NFETCNT_SHIFT);
    EMU->DCDCMISCCTRL = tmp;
    currentLimitersUpdate();
  }
  else
  {
    EMU->DCDCMISCCTRL = emuDcdcMiscCtrlReg;
    currentLimitersUpdate();
  }
}
#endif

#if defined( ERRATA_FIX_DCDC_LNHS_BLOCK_EN )
static void dcdcHsFixLnBlock(void)
{
#define EMU_DCDCSTATUS  (* (volatile uint32_t *)(EMU_BASE + 0x7C))
  if ((errataFixDcdcHsState == errataFixDcdcHsTrimSet)
      || (errataFixDcdcHsState == errataFixDcdcHsBypassLn))
  {
    /* Wait for LNRUNNING */
    if ((EMU->DCDCCTRL & _EMU_DCDCCTRL_DCDCMODE_MASK) == EMU_DCDCCTRL_DCDCMODE_LOWNOISE)
    {
      while (!(EMU_DCDCSTATUS & (0x1 << 16)));
    }
    errataFixDcdcHsState = errataFixDcdcHsLnWaitDone;
  }
}
#endif


#if defined( _EMU_CTRL_EM23VSCALE_MASK )
/* Configure EMU and CMU for EM2 and 3 voltage downscale */
static void vScaleDownEM23Setup(void)
{
  uint32_t hfSrcClockFrequency;

  EMU_VScaleEM23_TypeDef scaleEM23Voltage =
          (EMU_VScaleEM23_TypeDef)((EMU->CTRL & _EMU_CTRL_EM23VSCALE_MASK)
                                   >> _EMU_CTRL_EM23VSCALE_SHIFT);

  EMU_VScaleEM01_TypeDef currentEM01Voltage =
          (EMU_VScaleEM01_TypeDef)((EMU->STATUS & _EMU_STATUS_VSCALE_MASK)
                                   >> _EMU_STATUS_VSCALE_SHIFT);

  /* Wait until previous scaling is done. */
  EMU_VScaleWait();

  /* Inverse coding. */
  if ((uint32_t)scaleEM23Voltage > (uint32_t)currentEM01Voltage)
  {
    /* Set safe clock and wait-states. */
    if (scaleEM23Voltage == emuVScaleEM23_LowPower)
    {
      hfSrcClockFrequency = CMU_ClockDivGet(cmuClock_HF) * CMU_ClockFreqGet(cmuClock_HF);
      /* Set default low power voltage HFRCO band as HF clock. */
      if (hfSrcClockFrequency > CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX)
      {
        CMU_HFRCOBandSet(cmuHFRCOFreq_19M0Hz);
      }
      CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFRCO);
    }
    else
    {
      /* Other voltage scaling levels are not currently supported. */
      EFM_ASSERT(false);
    }
  }
  else
  {
    /* Same voltage or hardware will scale to min(EMU_CTRL_EM23VSCALE, EMU_STATUS_VSCALE)  */
  }
}
#endif
/** @endcond */


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Enter energy mode 2 (EM2).
 *
 * @details
 *   When entering EM2, the high frequency clocks are disabled, ie HFXO, HFRCO
 *   and AUXHFRCO (for AUXHFRCO, see exception note below). When re-entering
 *   EM0, HFRCO is re-enabled and the core will be clocked by the configured
 *   HFRCO band. This ensures a quick wakeup from EM2.
 *
 *   However, prior to entering EM2, the core may have been using another
 *   oscillator than HFRCO. The @p restore parameter gives the user the option
 *   to restore all HF oscillators according to state prior to entering EM2,
 *   as well as the clock used to clock the core. This restore procedure is
 *   handled by SW. However, since handled by SW, it will not be restored
 *   before completing the interrupt function(s) waking up the core!
 *
 * @note
 *   If restoring core clock to use the HFXO oscillator, which has been
 *   disabled during EM2 mode, this function will stall until the oscillator
 *   has stabilized. Stalling time can be reduced by adding interrupt
 *   support detecting stable oscillator, and an asynchronous switch to the
 *   original oscillator. See CMU documentation. Such a feature is however
 *   outside the scope of the implementation in this function.
 * @par
 *   If HFXO is re-enabled by this function, and NOT used to clock the core,
 *   this function will not wait for HFXO to stabilize. This must be considered
 *   by the application if trying to use features relying on that oscillator
 *   upon return.
 * @par
 *   If a debugger is attached, the AUXHFRCO will not be disabled if enabled
 *   upon entering EM2. It will thus remain enabled when returning to EM0
 *   regardless of the @p restore parameter.
 * @par
 *   If HFXO autostart and select is enabled by using CMU_HFXOAutostartEnable(),
 *   the starting and selecting of the core clocks will be identical to the user
 *   independently of the value of the @p restore parameter when waking up on
 *   the wakeup sources corresponding to the autostart and select setting.
 * @par
 *   If voltage scaling is supported, the restore parameter is true and the EM0 
 *   voltage scaling level is set higher than the EM2 level, then the EM0 level is 
 *   also restored.
 *
 * @param[in] restore
 *   @li true - restore oscillators, clocks and voltage scaling, see function details.
 *   @li false - do not restore oscillators and clocks, see function details.
 * @par
 *   The @p restore option should only be used if all clock control is done
 *   via the CMU API.
 ******************************************************************************/
void EMU_EnterEM2(bool restore)
{
#if defined( ERRATA_FIX_EMU_E107_EN )
  bool errataFixEmuE107En;
  uint32_t nonWicIntEn[2];
#endif

  /* Save EMU and CMU state requiring restore on EM2 exit. */
  emState(emState_Save);

#if defined( _EMU_CTRL_EM23VSCALE_MASK )
  vScaleDownEM23Setup();
#endif

  /* Enter Cortex deep sleep mode */
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

  /* Fix for errata EMU_E107 - store non-WIC interrupt enable flags.
     Disable the enabled non-WIC interrupts. */
#if defined( ERRATA_FIX_EMU_E107_EN )
  errataFixEmuE107En = getErrataFixEmuE107En();
  if (errataFixEmuE107En)
  {
    nonWicIntEn[0] = NVIC->ISER[0] & NON_WIC_INT_MASK_0;
    NVIC->ICER[0] = nonWicIntEn[0];
#if (NON_WIC_INT_MASK_1 != (~(0x0U)))
    nonWicIntEn[1] = NVIC->ISER[1] & NON_WIC_INT_MASK_1;
    NVIC->ICER[1] = nonWicIntEn[1];
#endif
  }
#endif

#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
  dcdcFetCntSet(true);
#endif
#if defined( ERRATA_FIX_DCDC_LNHS_BLOCK_EN )
  dcdcHsFixLnBlock();
#endif

  __WFI();

#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
  dcdcFetCntSet(false);
#endif

  /* Fix for errata EMU_E107 - restore state of non-WIC interrupt enable flags. */
#if defined( ERRATA_FIX_EMU_E107_EN )
  if (errataFixEmuE107En)
  {
    NVIC->ISER[0] = nonWicIntEn[0];
#if (NON_WIC_INT_MASK_1 != (~(0x0U)))
    NVIC->ISER[1] = nonWicIntEn[1];
#endif
  }
#endif

  /* Restore oscillators/clocks and voltage scaling if supported. */
  if (restore)
  {
    emState(emState_Restore);
  }
  else
  {
    /* If not restoring, and original clock was not HFRCO, we have to */
    /* update CMSIS core clock variable since HF clock has changed */
    /* to HFRCO. */
    SystemCoreClockUpdate();
  }
}


/***************************************************************************//**
 * @brief
 *   Enter energy mode 3 (EM3).
 *
 * @details
 *   When entering EM3, the high frequency clocks are disabled by HW, ie HFXO,
 *   HFRCO and AUXHFRCO (for AUXHFRCO, see exception note below). In addition,
 *   the low frequency clocks, ie LFXO and LFRCO are disabled by SW. When
 *   re-entering EM0, HFRCO is re-enabled and the core will be clocked by the
 *   configured HFRCO band. This ensures a quick wakeup from EM3.
 *
 *   However, prior to entering EM3, the core may have been using another
 *   oscillator than HFRCO. The @p restore parameter gives the user the option
 *   to restore all HF/LF oscillators according to state prior to entering EM3,
 *   as well as the clock used to clock the core. This restore procedure is
 *   handled by SW. However, since handled by SW, it will not be restored
 *   before completing the interrupt function(s) waking up the core!
 *
 * @note
 *   If restoring core clock to use an oscillator other than HFRCO, this
 *   function will stall until the oscillator has stabilized. Stalling time
 *   can be reduced by adding interrupt support detecting stable oscillator,
 *   and an asynchronous switch to the original oscillator. See CMU
 *   documentation. Such a feature is however outside the scope of the
 *   implementation in this function.
 * @par
 *   If HFXO/LFXO/LFRCO are re-enabled by this function, and NOT used to clock
 *   the core, this function will not wait for those oscillators to stabilize.
 *   This must be considered by the application if trying to use features
 *   relying on those oscillators upon return.
 * @par
 *   If a debugger is attached, the AUXHFRCO will not be disabled if enabled
 *   upon entering EM3. It will thus remain enabled when returning to EM0
 *   regardless of the @p restore parameter.
 * @par
 *   If voltage scaling is supported, the restore parameter is true and the EM0 
 *   voltage scaling level is set higher than the EM3 level, then the EM0 level is 
 *   also restored.
 *
 * @param[in] restore
 *   @li true - restore oscillators, clocks and voltage scaling, see function details.
 *   @li false - do not restore oscillators and clocks, see function details.
 * @par
 *   The @p restore option should only be used if all clock control is done
 *   via the CMU API.
 ******************************************************************************/
void EMU_EnterEM3(bool restore)
{
  uint32_t cmuLocked;

#if defined( ERRATA_FIX_EMU_E107_EN )
  bool errataFixEmuE107En;
  uint32_t nonWicIntEn[2];
#endif

  /* Save EMU and CMU state requiring restore on EM2 exit. */
  emState(emState_Save);

#if defined( _EMU_CTRL_EM23VSCALE_MASK )
  vScaleDownEM23Setup();
#endif

  /* CMU registers may be locked */
  cmuLocked = CMU->LOCK & CMU_LOCK_LOCKKEY_LOCKED;
  CMU_Unlock();

  /* Disable LF oscillators */
  CMU->OSCENCMD = CMU_OSCENCMD_LFXODIS | CMU_OSCENCMD_LFRCODIS;

  /* Restore CMU register locking */
  if (cmuLocked)
  {
    CMU_Lock();
  }

  /* Enter Cortex deep sleep mode */
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

  /* Fix for errata EMU_E107 - store non-WIC interrupt enable flags.
     Disable the enabled non-WIC interrupts. */
#if defined( ERRATA_FIX_EMU_E107_EN )
  errataFixEmuE107En = getErrataFixEmuE107En();
  if (errataFixEmuE107En)
  {
    nonWicIntEn[0] = NVIC->ISER[0] & NON_WIC_INT_MASK_0;
    NVIC->ICER[0] = nonWicIntEn[0];
#if (NON_WIC_INT_MASK_1 != (~(0x0U)))
    nonWicIntEn[1] = NVIC->ISER[1] & NON_WIC_INT_MASK_1;
    NVIC->ICER[1] = nonWicIntEn[1];
#endif
  }
#endif

#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
  dcdcFetCntSet(true);
#endif
#if defined( ERRATA_FIX_DCDC_LNHS_BLOCK_EN )
  dcdcHsFixLnBlock();
#endif

  __WFI();

#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
  dcdcFetCntSet(false);
#endif

  /* Fix for errata EMU_E107 - restore state of non-WIC interrupt enable flags. */
#if defined( ERRATA_FIX_EMU_E107_EN )
  if (errataFixEmuE107En)
  {
    NVIC->ISER[0] = nonWicIntEn[0];
#if (NON_WIC_INT_MASK_1 != (~(0x0U)))
    NVIC->ISER[1] = nonWicIntEn[1];
#endif
  }
#endif

  /* Restore oscillators/clocks and voltage scaling if supported. */
  if (restore)
  {
    emState(emState_Restore);
  }
  else
  {
    /* If not restoring, and original clock was not HFRCO, we have to */
    /* update CMSIS core clock variable since HF clock has changed */
    /* to HFRCO. */
    SystemCoreClockUpdate();
  }
}


/***************************************************************************//**
 * @brief
 *   Restore CMU HF clock select state, oscillator enable and voltage scaling 
 *   (if available) after @ref EMU_EnterEM2() or @ref EMU_EnterEM3() are called 
 *   with the restore parameter set to false. Calling this function is
 *   equivalent to calling @ref EMU_EnterEM2() or @ref EMU_EnterEM3() with the 
 *   restore parameter set to true, but it allows the application to evaluate the
 *   wakeup reason before restoring state.
 ******************************************************************************/
void EMU_Restore(void)
{
  emState(emState_Restore);
}


/***************************************************************************//**
 * @brief
 *   Enter energy mode 4 (EM4).
 *
 * @note
 *   Only a power on reset or external reset pin can wake the device from EM4.
 ******************************************************************************/
void EMU_EnterEM4(void)
{
  int i;

#if defined( _EMU_EM4CTRL_EM4ENTRY_SHIFT )
  uint32_t em4seq2 = (EMU->EM4CTRL & ~_EMU_EM4CTRL_EM4ENTRY_MASK)
                     | (2 << _EMU_EM4CTRL_EM4ENTRY_SHIFT);
  uint32_t em4seq3 = (EMU->EM4CTRL & ~_EMU_EM4CTRL_EM4ENTRY_MASK)
                     | (3 << _EMU_EM4CTRL_EM4ENTRY_SHIFT);
#else
  uint32_t em4seq2 = (EMU->CTRL & ~_EMU_CTRL_EM4CTRL_MASK)
                     | (2 << _EMU_CTRL_EM4CTRL_SHIFT);
  uint32_t em4seq3 = (EMU->CTRL & ~_EMU_CTRL_EM4CTRL_MASK)
                     | (3 << _EMU_CTRL_EM4CTRL_SHIFT);
#endif

  /* Make sure register write lock is disabled */
  EMU_Unlock();

#if defined( _EMU_EM4CTRL_MASK )
  if ((EMU->EM4CTRL & _EMU_EM4CTRL_EM4STATE_MASK) == EMU_EM4CTRL_EM4STATE_EM4S)
  {
    uint32_t dcdcMode = EMU->DCDCCTRL & _EMU_DCDCCTRL_DCDCMODE_MASK;
    if (dcdcMode == EMU_DCDCCTRL_DCDCMODE_LOWNOISE
        || dcdcMode == EMU_DCDCCTRL_DCDCMODE_LOWPOWER)
    {
      /* DCDC is not supported in EM4S so we switch DCDC to bypass mode before
       * entering EM4S */
      EMU_DCDCModeSet(emuDcdcMode_Bypass);
    }
  }
#endif

#if defined( _EMU_EM4CTRL_MASK ) && defined( ERRATA_FIX_EMU_E208_EN )
  if (EMU->EM4CTRL & EMU_EM4CTRL_EM4STATE_EM4H)
  {
    /* Fix for errata EMU_E208 - Occasional Full Reset After Exiting EM4H.
     * Full description of errata fix can be found in the errata document. */
    __disable_irq();
    *(volatile uint32_t *)(EMU_BASE + 0x190)  = 0x0000ADE8UL;
    *(volatile uint32_t *)(EMU_BASE + 0x198) |= (0x1UL << 7);
    *(volatile uint32_t *)(EMU_BASE + 0x88)  |= (0x1UL << 8);
  }
#endif

#if defined( ERRATA_FIX_EMU_E108_EN )
  /* Fix for errata EMU_E108 - High Current Consumption on EM4 Entry. */
  __disable_irq();
  *(volatile uint32_t *)0x400C80E4 = 0;
#endif

#if defined( ERRATA_FIX_DCDC_FETCNT_SET_EN )
  dcdcFetCntSet(true);
#endif
#if defined( ERRATA_FIX_DCDC_LNHS_BLOCK_EN )
  dcdcHsFixLnBlock();
#endif

  for (i = 0; i < 4; i++)
  {
#if defined( _EMU_EM4CTRL_EM4ENTRY_SHIFT )
    EMU->EM4CTRL = em4seq2;
    EMU->EM4CTRL = em4seq3;
  }
  EMU->EM4CTRL = em4seq2;
#else
    EMU->CTRL = em4seq2;
    EMU->CTRL = em4seq3;
  }
  EMU->CTRL = em4seq2;
#endif
}

#if defined( _EMU_EM4CTRL_MASK )
/***************************************************************************//**
 * @brief
 *   Enter energy mode 4 hibernate (EM4H).
 *
 * @note
 *   Retention of clocks and GPIO in EM4 can be configured using
 *   @ref EMU_EM4Init before calling this function.
 ******************************************************************************/
void EMU_EnterEM4H(void)
{
  BUS_RegBitWrite(&EMU->EM4CTRL, _EMU_EM4CTRL_EM4STATE_SHIFT, 1);
  EMU_EnterEM4();
}

/***************************************************************************//**
 * @brief
 *   Enter energy mode 4 shutoff (EM4S).
 *
 * @note
 *   Retention of clocks and GPIO in EM4 can be configured using
 *   @ref EMU_EM4Init before calling this function.
 ******************************************************************************/
void EMU_EnterEM4S(void)
{
  BUS_RegBitWrite(&EMU->EM4CTRL, _EMU_EM4CTRL_EM4STATE_SHIFT, 0);
  EMU_EnterEM4();
}
#endif

/***************************************************************************//**
 * @brief
 *   Power down memory block.
 *
 * @param[in] blocks
 *   Specifies a logical OR of bits indicating memory blocks to power down.
 *   Bit 0 selects block 1, bit 1 selects block 2, etc. Memory block 0 cannot
 *   be disabled. Please refer to the reference manual for available
 *   memory blocks for a device.
 *
 * @note
 *   Only a POR reset can power up the specified memory block(s) after powerdown.
 *
 * @deprecated
 *   This function is deprecated, use @ref EMU_RamPowerDown() instead which
 *   maps a user provided memory range into RAM blocks to power down.
 ******************************************************************************/
void EMU_MemPwrDown(uint32_t blocks)
{
#if defined( _EMU_MEMCTRL_MASK )
  EMU->MEMCTRL = blocks & _EMU_MEMCTRL_MASK;
#elif defined( _EMU_RAM0CTRL_MASK )
  EMU->RAM0CTRL = blocks & _EMU_RAM0CTRL_MASK;
#else
  (void)blocks;
#endif
}

/***************************************************************************//**
 * @brief
 *   Power down RAM memory blocks.
 *
 * @details
 *   This function will power down all the RAM blocks that are within a given
 *   range. The RAM block layout is different between device families, so this
 *   function can be used in a generic way to power down a RAM memory region
 *   which is known to be unused.
 *
 *   This function will only power down blocks which are completely enclosed
 *   by the memory range given by [start, end).
 *
 *   Here is an example of how to power down all RAM blocks except the first
 *   one. The first RAM block is special in that it cannot be powered down
 *   by the hardware. The size of this first RAM block is device specific
 *   see the reference manual to find the RAM block sizes.
 *
 * @code
 *   EMU_RamPowerDown(SRAM_BASE, SRAM_BASE + SRAM_SIZE);
 * @endcode
 *
 * @note
 *   Only a POR reset can power up the specified memory block(s) after powerdown.
 *
 * @param[in] start
 *   The start address of the RAM region to power down. This address is
 *   inclusive.
 *
 * @param[in] end
 *   The end address of the RAM region to power down. This address is
 *   exclusive. If this parameter is 0, then all RAM blocks contained in the
 *   region from start to the upper RAM address will be powered down.
 ******************************************************************************/
void EMU_RamPowerDown(uint32_t start, uint32_t end)
{
  uint32_t mask = 0;

  if (end == 0)
  {
    end = SRAM_BASE + SRAM_SIZE;
  }

  // Check to see if something in RAM0 can be powered down
  if (end > RAM0_END)
  {
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84) // EFM32xG12 and EFR32xG12
    // Block 0 is 16 kB and cannot be powered off
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20004000) << 0; // Block 1, 16 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20008000) << 1; // Block 2, 16 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x2000C000) << 2; // Block 3, 16 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20010000) << 3; // Block 4, 64 kB
#elif defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80) // EFM32xG1 and EFR32xG1
    // Block 0 is 4 kB and cannot be powered off
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20001000) << 0; // Block 1, 4 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20002000) << 1; // Block 2, 8 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20004000) << 2; // Block 3, 8 kB
    mask |= ADDRESS_NOT_IN_BLOCK(start, 0x20006000) << 3; // Block 4, 7 kB
#elif defined(RAM0_BLOCKS)
    // These platforms have equally sized RAM blocks
    for (int i = 1; i < RAM0_BLOCKS; i++)
    {
      mask |= ADDRESS_NOT_IN_BLOCK(start, RAM_MEM_BASE + (i * RAM0_BLOCK_SIZE)) << (i - 1);
    }
#endif
  }

  // Power down the selected blocks
#if defined( _EMU_MEMCTRL_MASK )
  EMU->MEMCTRL = EMU->MEMCTRL   | mask;
#elif defined( _EMU_RAM0CTRL_MASK )
  EMU->RAM0CTRL = EMU->RAM0CTRL | mask;
#else
  // These devices are unable to power down RAM blocks
  (void) mask;
  (void) start;
#endif

#if defined(RAM1_MEM_END)
  mask = 0;
  if (end > RAM1_MEM_END)
  {
    for (int i = 0; i < RAM1_BLOCKS; i++)
    {
      mask |= ADDRESS_NOT_IN_BLOCK(start, RAM1_MEM_BASE + (i * RAM1_BLOCK_SIZE)) << i;
    }
  }
  EMU->RAM1CTRL |= mask;
#endif
}

#if defined(_EMU_EM23PERNORETAINCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Set EM2 3 peripheral retention control.
 *
 * @param[in] periMask
 *  Peripheral select mask. Use | operator to select multiple peripheral, for example
 *  @ref emuPeripheralRetention_LEUART0 | @ref emuPeripheralRetention_VDAC0.
 * @param[in] enable
 *  Peripheral retention enable (true) or disable (false).
 *
 *
 * @note
 *   Only peripheral retention disable is currently supported. Peripherals are
 *   enabled by default, and can only be disabled.
 ******************************************************************************/
void EMU_PeripheralRetention(EMU_PeripheralRetention_TypeDef periMask, bool enable)
{
  EFM_ASSERT(!enable);
  EMU->EM23PERNORETAINCTRL = periMask & emuPeripheralRetention_ALL;
}
#endif


/***************************************************************************//**
 * @brief
 *   Update EMU module with CMU oscillator selection/enable status.
 *
 * @deprecated
 *   Oscillator status is saved in @ref EMU_EnterEM2() and @ref EMU_EnterEM3().
 ******************************************************************************/
void EMU_UpdateOscConfig(void)
{
  emState(emState_Save);
}


#if defined( _EMU_CMD_EM01VSCALE0_MASK )
/***************************************************************************//**
 * @brief
 *   Voltage scale in EM0 and 1 by clock frequency.
 *
 * @param[in] clockFrequency
 *   Use CMSIS HF clock if 0, or override to custom clock. Providing a
 *   custom clock frequency is required if using a non-standard HFXO
 *   frequency.
 * @param[in] wait
 *   Wait for scaling to complate.
 *
 * @note
 *   This function is primarely needed by the @ref CMU module.
 ******************************************************************************/
void EMU_VScaleEM01ByClock(uint32_t clockFrequency, bool wait)
{
  uint32_t hfSrcClockFrequency;
  uint32_t hfPresc = 1U + ((CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK)
                           >> _CMU_HFPRESC_PRESC_SHIFT);

  /* VSCALE frequency is HFSRCCLK */
  if (clockFrequency == 0)
  {
    hfSrcClockFrequency = SystemHFClockGet() * hfPresc;
  }
  else
  {
    hfSrcClockFrequency = clockFrequency;
  }

  /* Apply EM0 and 1 voltage scaling command. */
  if (vScaleEM01Config.vScaleEM01LowPowerVoltageEnable
      && (hfSrcClockFrequency < CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX))
  {
    EMU->CMD = vScaleEM01Cmd(emuVScaleEM01_LowPower);
  }
  else
  {
    EMU->CMD = vScaleEM01Cmd(emuVScaleEM01_HighPerformance);
  }

  if (wait)
  {
    EMU_VScaleWait();
  }
}
#endif


#if defined( _EMU_CMD_EM01VSCALE0_MASK )
/***************************************************************************//**
 * @brief
 *   Force voltage scaling in EM0 and 1 to a specific voltage level.
 *
 * @param[in] voltage
 *   Target VSCALE voltage level.
 * @param[in] wait
 *   Wait for scaling to complate.
 *
 * @note
 *   This function is useful for upscaling before programming Flash from @ref MSC,
 *   and downscaling after programming is done. Flash programming is only supported
 *   at @ref emuVScaleEM01_HighPerformance.
 *
 * @note
 *  This function ignores @ref vScaleEM01LowPowerVoltageEnable set from @ref
 *  EMU_EM01Init().
 ******************************************************************************/
void EMU_VScaleEM01(EMU_VScaleEM01_TypeDef voltage, bool wait)
{
  uint32_t hfSrcClockFrequency;
  uint32_t hfPresc = 1U + ((CMU->HFPRESC & _CMU_HFPRESC_PRESC_MASK)
                           >> _CMU_HFPRESC_PRESC_SHIFT);
  hfSrcClockFrequency = SystemHFClockGet() * hfPresc;

  if (voltage == emuVScaleEM01_LowPower)
  {
    EFM_ASSERT(hfSrcClockFrequency <= CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX);
  }

  EMU->CMD = vScaleEM01Cmd(voltage);
  if (wait)
  {
    EMU_VScaleWait();
  }
}
#endif


#if defined( _EMU_CMD_EM01VSCALE0_MASK )
/***************************************************************************//**
 * @brief
 *   Update EMU module with Energy Mode 0 and 1 configuration
 *
 * @param[in] em01Init
 *    Energy Mode 0 and 1 configuration structure
 ******************************************************************************/
void EMU_EM01Init(const EMU_EM01Init_TypeDef *em01Init)
{
  vScaleEM01Config.vScaleEM01LowPowerVoltageEnable =
    em01Init->vScaleEM01LowPowerVoltageEnable;
  EMU_VScaleEM01ByClock(0, true);
}
#endif


/***************************************************************************//**
 * @brief
 *   Update EMU module with Energy Mode 2 and 3 configuration
 *
 * @param[in] em23Init
 *    Energy Mode 2 and 3 configuration structure
 ******************************************************************************/
void EMU_EM23Init(const EMU_EM23Init_TypeDef *em23Init)
{
#if defined( _EMU_CTRL_EMVREG_MASK )
  EMU->CTRL = em23Init->em23VregFullEn ? (EMU->CTRL | EMU_CTRL_EMVREG)
                                         : (EMU->CTRL & ~EMU_CTRL_EMVREG);
#elif defined( _EMU_CTRL_EM23VREG_MASK )
  EMU->CTRL = em23Init->em23VregFullEn ? (EMU->CTRL | EMU_CTRL_EM23VREG)
                                         : (EMU->CTRL & ~EMU_CTRL_EM23VREG);
#else
  (void)em23Init;
#endif

#if defined( _EMU_CTRL_EM23VSCALE_MASK )
  EMU->CTRL = (EMU->CTRL & ~_EMU_CTRL_EM23VSCALE_MASK)
              | (em23Init->vScaleEM23Voltage << _EMU_CTRL_EM23VSCALE_SHIFT);
#endif
}


#if defined( _EMU_EM4CONF_MASK ) || defined( _EMU_EM4CTRL_MASK )
/***************************************************************************//**
 * @brief
 *   Update EMU module with Energy Mode 4 configuration
 *
 * @param[in] em4Init
 *    Energy Mode 4 configuration structure
 ******************************************************************************/
void EMU_EM4Init(const EMU_EM4Init_TypeDef *em4Init)
{
#if defined( _EMU_EM4CONF_MASK )
  /* Init for platforms with EMU->EM4CONF register */
  uint32_t em4conf = EMU->EM4CONF;

  /* Clear fields that will be reconfigured */
  em4conf &= ~(_EMU_EM4CONF_LOCKCONF_MASK
               | _EMU_EM4CONF_OSC_MASK
               | _EMU_EM4CONF_BURTCWU_MASK
               | _EMU_EM4CONF_VREGEN_MASK);

  /* Configure new settings */
  em4conf |= (em4Init->lockConfig << _EMU_EM4CONF_LOCKCONF_SHIFT)
             | (em4Init->osc)
             | (em4Init->buRtcWakeup << _EMU_EM4CONF_BURTCWU_SHIFT)
             | (em4Init->vreg << _EMU_EM4CONF_VREGEN_SHIFT);

  /* Apply configuration. Note that lock can be set after this stage. */
  EMU->EM4CONF = em4conf;

#elif defined( _EMU_EM4CTRL_MASK )
  /* Init for platforms with EMU->EM4CTRL register */

  uint32_t em4ctrl = EMU->EM4CTRL;

  em4ctrl &= ~(_EMU_EM4CTRL_RETAINLFXO_MASK
               | _EMU_EM4CTRL_RETAINLFRCO_MASK
               | _EMU_EM4CTRL_RETAINULFRCO_MASK
               | _EMU_EM4CTRL_EM4STATE_MASK
               | _EMU_EM4CTRL_EM4IORETMODE_MASK);

  em4ctrl |= (em4Init->retainLfxo     ? EMU_EM4CTRL_RETAINLFXO : 0)
              | (em4Init->retainLfrco  ? EMU_EM4CTRL_RETAINLFRCO : 0)
              | (em4Init->retainUlfrco ? EMU_EM4CTRL_RETAINULFRCO : 0)
              | (em4Init->em4State     ? EMU_EM4CTRL_EM4STATE_EM4H : 0)
              | (em4Init->pinRetentionMode);

  EMU->EM4CTRL = em4ctrl;
#endif

#if defined( _EMU_CTRL_EM4HVSCALE_MASK )
  EMU->CTRL = (EMU->CTRL & ~_EMU_CTRL_EM4HVSCALE_MASK)
              | (em4Init->vScaleEM4HVoltage << _EMU_CTRL_EM4HVSCALE_SHIFT);
#endif
}
#endif


#if defined( BU_PRESENT )
/***************************************************************************//**
 * @brief
 *   Configure Backup Power Domain settings
 *
 * @param[in] bupdInit
 *   Backup power domain initialization structure
 ******************************************************************************/
void EMU_BUPDInit(const EMU_BUPDInit_TypeDef *bupdInit)
{
  uint32_t reg;

  /* Set power connection configuration */
  reg = EMU->PWRCONF & ~(_EMU_PWRCONF_PWRRES_MASK
                         | _EMU_PWRCONF_VOUTSTRONG_MASK
                         | _EMU_PWRCONF_VOUTMED_MASK
                         | _EMU_PWRCONF_VOUTWEAK_MASK);

  reg |= bupdInit->resistor
         | (bupdInit->voutStrong << _EMU_PWRCONF_VOUTSTRONG_SHIFT)
         | (bupdInit->voutMed    << _EMU_PWRCONF_VOUTMED_SHIFT)
         | (bupdInit->voutWeak   << _EMU_PWRCONF_VOUTWEAK_SHIFT);

  EMU->PWRCONF = reg;

  /* Set backup domain inactive mode configuration */
  reg = EMU->BUINACT & ~(_EMU_BUINACT_PWRCON_MASK);
  reg |= (bupdInit->inactivePower);
  EMU->BUINACT = reg;

  /* Set backup domain active mode configuration */
  reg = EMU->BUACT & ~(_EMU_BUACT_PWRCON_MASK);
  reg |= (bupdInit->activePower);
  EMU->BUACT = reg;

  /* Set power control configuration */
  reg = EMU->BUCTRL & ~(_EMU_BUCTRL_PROBE_MASK
                        | _EMU_BUCTRL_BODCAL_MASK
                        | _EMU_BUCTRL_STATEN_MASK
                        | _EMU_BUCTRL_EN_MASK);

  /* Note use of ->enable to both enable BUPD, use BU_VIN pin input and
     release reset */
  reg |= bupdInit->probe
         | (bupdInit->bodCal          << _EMU_BUCTRL_BODCAL_SHIFT)
         | (bupdInit->statusPinEnable << _EMU_BUCTRL_STATEN_SHIFT)
         | (bupdInit->enable          << _EMU_BUCTRL_EN_SHIFT);

  /* Enable configuration */
  EMU->BUCTRL = reg;

  /* If enable is true, enable BU_VIN input power pin, if not disable it  */
  EMU_BUPinEnable(bupdInit->enable);

  /* If enable is true, release BU reset, if not keep reset asserted */
  BUS_RegBitWrite(&(RMU->CTRL), _RMU_CTRL_BURSTEN_SHIFT, !bupdInit->enable);
}


/***************************************************************************//**
 * @brief
 *   Configure Backup Power Domain BOD Threshold value
 * @note
 *   These values are precalibrated
 * @param[in] mode Active or Inactive mode
 * @param[in] value
 ******************************************************************************/
void EMU_BUThresholdSet(EMU_BODMode_TypeDef mode, uint32_t value)
{
  EFM_ASSERT(value<8);
  EFM_ASSERT(value<=(_EMU_BUACT_BUEXTHRES_MASK>>_EMU_BUACT_BUEXTHRES_SHIFT));

  switch(mode)
  {
    case emuBODMode_Active:
      EMU->BUACT = (EMU->BUACT & ~_EMU_BUACT_BUEXTHRES_MASK)
                   | (value<<_EMU_BUACT_BUEXTHRES_SHIFT);
      break;
    case emuBODMode_Inactive:
      EMU->BUINACT = (EMU->BUINACT & ~_EMU_BUINACT_BUENTHRES_MASK)
                     | (value<<_EMU_BUINACT_BUENTHRES_SHIFT);
      break;
  }
}


/***************************************************************************//**
 * @brief
 *  Configure Backup Power Domain BOD Threshold Range
 * @note
 *  These values are precalibrated
 * @param[in] mode Active or Inactive mode
 * @param[in] value
 ******************************************************************************/
void EMU_BUThresRangeSet(EMU_BODMode_TypeDef mode, uint32_t value)
{
  EFM_ASSERT(value < 4);
  EFM_ASSERT(value<=(_EMU_BUACT_BUEXRANGE_MASK>>_EMU_BUACT_BUEXRANGE_SHIFT));

  switch(mode)
  {
    case emuBODMode_Active:
      EMU->BUACT = (EMU->BUACT & ~_EMU_BUACT_BUEXRANGE_MASK)
                   | (value<<_EMU_BUACT_BUEXRANGE_SHIFT);
      break;
    case emuBODMode_Inactive:
      EMU->BUINACT = (EMU->BUINACT & ~_EMU_BUINACT_BUENRANGE_MASK)
                     | (value<<_EMU_BUINACT_BUENRANGE_SHIFT);
      break;
  }
}
#endif


/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if defined( _EMU_DCDCCTRL_MASK )
/* Translate fields with different names across platform generations to common names. */
#if defined( _EMU_DCDCMISCCTRL_LPCMPBIAS_MASK )
#define _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_MASK      _EMU_DCDCMISCCTRL_LPCMPBIAS_MASK
#define _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT     _EMU_DCDCMISCCTRL_LPCMPBIAS_SHIFT
#elif defined( _EMU_DCDCMISCCTRL_LPCMPBIASEM234H_MASK )
#define _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_MASK      _EMU_DCDCMISCCTRL_LPCMPBIASEM234H_MASK
#define _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT     _EMU_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT
#endif
#if defined( _EMU_DCDCLPCTRL_LPCMPHYSSEL_MASK )
#define _GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK      _EMU_DCDCLPCTRL_LPCMPHYSSEL_MASK
#define _GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_SHIFT     _EMU_DCDCLPCTRL_LPCMPHYSSEL_SHIFT
#elif defined( _EMU_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK )
#define _GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK      _EMU_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK
#define _GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_SHIFT     _EMU_DCDCLPCTRL_LPCMPHYSSELEM234H_SHIFT
#endif

/* Internal DCDC trim modes. */
typedef enum
{
  dcdcTrimMode_EM234H_LP = 0,
#if defined( _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK )
  dcdcTrimMode_EM01_LP,
#endif
  dcdcTrimMode_LN,
} dcdcTrimMode_TypeDef;

/***************************************************************************//**
 * @brief
 *   Load DCDC calibration constants from DI page. Const means calibration
 *   data that does not change depending on other configuration parameters.
 *
 * @return
 *   False if calibration registers are locked
 ******************************************************************************/
static bool dcdcConstCalibrationLoad(void)
{
#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
  uint32_t val;
  volatile uint32_t *reg;

  /* DI calib data in flash */
  volatile uint32_t* const diCal_EMU_DCDCLNFREQCTRL =  (volatile uint32_t *)(0x0FE08038);
  volatile uint32_t* const diCal_EMU_DCDCLNVCTRL =     (volatile uint32_t *)(0x0FE08040);
  volatile uint32_t* const diCal_EMU_DCDCLPCTRL =      (volatile uint32_t *)(0x0FE08048);
  volatile uint32_t* const diCal_EMU_DCDCLPVCTRL =     (volatile uint32_t *)(0x0FE08050);
  volatile uint32_t* const diCal_EMU_DCDCTRIM0 =       (volatile uint32_t *)(0x0FE08058);
  volatile uint32_t* const diCal_EMU_DCDCTRIM1 =       (volatile uint32_t *)(0x0FE08060);

  if (DEVINFO->DCDCLPVCTRL0 != UINT_MAX)
  {
    val = *(diCal_EMU_DCDCLNFREQCTRL + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCLNFREQCTRL;
    *reg = val;

    val = *(diCal_EMU_DCDCLNVCTRL + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCLNVCTRL;
    *reg = val;

    val = *(diCal_EMU_DCDCLPCTRL + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCLPCTRL;
    *reg = val;

    val = *(diCal_EMU_DCDCLPVCTRL + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCLPVCTRL;
    *reg = val;

    val = *(diCal_EMU_DCDCTRIM0 + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCTRIM0;
    *reg = val;

    val = *(diCal_EMU_DCDCTRIM1 + 1);
    reg = (volatile uint32_t *)*diCal_EMU_DCDCTRIM1;
    *reg = val;

    return true;
  }
  EFM_ASSERT(false);
  /* Return when assertions are disabled */
  return false;

#else
  return true;
#endif
}


/***************************************************************************//**
 * @brief
 *   Set recommended and validated current optimization and timing settings
 *
 ******************************************************************************/
static void dcdcValidatedConfigSet(void)
{
/* Disable LP mode hysterysis in the state machine control */
#define EMU_DCDCMISCCTRL_LPCMPHYSDIS (0x1UL << 1)
/* Comparator threshold on the high side */
#define EMU_DCDCMISCCTRL_LPCMPHYSHI  (0x1UL << 2)
#define EMU_DCDCSMCTRL  (* (volatile uint32_t *)(EMU_BASE + 0x44))

  uint32_t lnForceCcm;

#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
  uint32_t dcdcTiming;
  SYSTEM_ChipRevision_TypeDef rev;
#endif

  /* Enable duty cycling of the bias */
  EMU->DCDCLPCTRL |= EMU_DCDCLPCTRL_LPVREFDUTYEN;

  /* Set low-noise RCO for LNFORCECCM configuration
   * LNFORCECCM is default 1 for EFR32
   * LNFORCECCM is default 0 for EFM32
   */
  lnForceCcm = BUS_RegBitRead(&EMU->DCDCMISCCTRL, _EMU_DCDCMISCCTRL_LNFORCECCM_SHIFT);
  if (lnForceCcm)
  {
    /* 7MHz is recommended for LNFORCECCM = 1 */
    EMU_DCDCLnRcoBandSet(emuDcdcLnRcoBand_7MHz);
  }
  else
  {
    /* 3MHz is recommended for LNFORCECCM = 0 */
    EMU_DCDCLnRcoBandSet(emuDcdcLnRcoBand_3MHz);
  }

#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
  EMU->DCDCTIMING &= ~_EMU_DCDCTIMING_DUTYSCALE_MASK;
  EMU->DCDCMISCCTRL |= EMU_DCDCMISCCTRL_LPCMPHYSDIS
                       | EMU_DCDCMISCCTRL_LPCMPHYSHI;

  SYSTEM_ChipRevisionGet(&rev);
  if ((rev.major == 1)
      && (rev.minor < 3)
      && (errataFixDcdcHsState == errataFixDcdcHsInit))
  {
    /* LPCMPWAITDIS = 1 */
    EMU_DCDCSMCTRL |= 1;

    dcdcTiming = EMU->DCDCTIMING;
    dcdcTiming &= ~(_EMU_DCDCTIMING_LPINITWAIT_MASK
                    |_EMU_DCDCTIMING_LNWAIT_MASK
                    |_EMU_DCDCTIMING_BYPWAIT_MASK);

    dcdcTiming |= ((180 << _EMU_DCDCTIMING_LPINITWAIT_SHIFT)
                   | (12 << _EMU_DCDCTIMING_LNWAIT_SHIFT)
                   | (180 << _EMU_DCDCTIMING_BYPWAIT_SHIFT));
    EMU->DCDCTIMING = dcdcTiming;

    errataFixDcdcHsState = errataFixDcdcHsTrimSet;
  }
#endif
}


/***************************************************************************//**
 * @brief
 *   Compute current limiters:
 *     LNCLIMILIMSEL: LN current limiter threshold
 *     LPCLIMILIMSEL: LP current limiter threshold
 *     DCDCZDETCTRL:  zero detector limiter threshold
 ******************************************************************************/
static void currentLimitersUpdate(void)
{
  uint32_t lncLimSel;
  uint32_t zdetLimSel;
  uint32_t pFetCnt;
  uint16_t maxReverseCurrent_mA;

    /* 80mA as recommended peak in Application Note AN0948.
       The peak current is the average current plus 50% of the current ripple.
       Hence, a 14mA average current is recommended in LP mode. Since LP PFETCNT is also
       a constant, we get lpcLimImSel = 1. The following calculation is provided
       for documentation only. */
  const uint32_t lpcLim = (((14 + 40) + ((14 + 40) / 2))
                           / (5 * (DCDC_LP_PFET_CNT + 1)))
                          - 1;
  const uint32_t lpcLimSel = lpcLim << _EMU_DCDCMISCCTRL_LPCLIMILIMSEL_SHIFT;

  /* Get enabled PFETs */
  pFetCnt = (EMU->DCDCMISCCTRL & _EMU_DCDCMISCCTRL_PFETCNT_MASK)
             >> _EMU_DCDCMISCCTRL_PFETCNT_SHIFT;

  /* Compute LN current limiter threshold from nominal user input current and
     LN PFETCNT as described in the register description for
     EMU_DCDCMISCCTRL_LNCLIMILIMSEL. */
  lncLimSel = (((dcdcMaxCurrent_mA + 40) + ((dcdcMaxCurrent_mA + 40) / 2))
               / (5 * (pFetCnt + 1)))
              - 1;

  /* Saturate the register field value */
  lncLimSel = SL_MIN(lncLimSel,
                     _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_MASK
                      >> _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_SHIFT);

  lncLimSel <<= _EMU_DCDCMISCCTRL_LNCLIMILIMSEL_SHIFT;

  /* Check for overflow */
  EFM_ASSERT((lncLimSel & ~_EMU_DCDCMISCCTRL_LNCLIMILIMSEL_MASK) == 0x0);
  EFM_ASSERT((lpcLimSel & ~_EMU_DCDCMISCCTRL_LPCLIMILIMSEL_MASK) == 0x0);

  EMU->DCDCMISCCTRL = (EMU->DCDCMISCCTRL & ~(_EMU_DCDCMISCCTRL_LNCLIMILIMSEL_MASK
                                             | _EMU_DCDCMISCCTRL_LPCLIMILIMSEL_MASK))
                       | (lncLimSel | lpcLimSel);


  /* Compute reverse current limit threshold for the zero detector from user input
     maximum reverse current and LN PFETCNT as described in the register description
     for EMU_DCDCZDETCTRL_ZDETILIMSEL. */
  if (dcdcReverseCurrentControl >= 0)
  {
    /* If dcdcReverseCurrentControl < 0, then EMU_DCDCZDETCTRL_ZDETILIMSEL is "don't care" */
    maxReverseCurrent_mA = (uint16_t)dcdcReverseCurrentControl;

    zdetLimSel = ( ((maxReverseCurrent_mA + 40) + ((maxReverseCurrent_mA + 40) / 2))
                    / ((2 * (pFetCnt + 1)) + ((pFetCnt + 1) / 2)) );
    /* Saturate the register field value */
    zdetLimSel = SL_MIN(zdetLimSel,
                        _EMU_DCDCZDETCTRL_ZDETILIMSEL_MASK
                         >> _EMU_DCDCZDETCTRL_ZDETILIMSEL_SHIFT);

    zdetLimSel <<= _EMU_DCDCZDETCTRL_ZDETILIMSEL_SHIFT;

    /* Check for overflow */
    EFM_ASSERT((zdetLimSel & ~_EMU_DCDCZDETCTRL_ZDETILIMSEL_MASK) == 0x0);

    EMU->DCDCZDETCTRL = (EMU->DCDCZDETCTRL & ~_EMU_DCDCZDETCTRL_ZDETILIMSEL_MASK)
                         | zdetLimSel;
  }
}


/***************************************************************************//**
 * @brief
 *   Set static variables that hold the user set maximum peak current
 *   and reverse current. Update limiters.
 *
 * @param[in] maxCurrent_mA
 *   Set the maximum peak current that the DCDC can draw from the power source.
 * @param[in] reverseCurrentControl
 *   Reverse current control as defined by
 *   @ref EMU_DcdcLnReverseCurrentControl_TypeDef. Positive values have unit mA.
 ******************************************************************************/
static void userCurrentLimitsSet(uint32_t maxCurrent_mA,
                                 EMU_DcdcLnReverseCurrentControl_TypeDef reverseCurrentControl)
{
  dcdcMaxCurrent_mA = maxCurrent_mA;
  dcdcReverseCurrentControl = reverseCurrentControl;
}


/***************************************************************************//**
 * @brief
 *   Set DCDC low noise compensator control register
 *
 * @param[in] comp
 *   Low-noise mode compensator trim setpoint
 ******************************************************************************/
static void compCtrlSet(EMU_DcdcLnCompCtrl_TypeDef comp)
{
  switch (comp)
  {
    case emuDcdcLnCompCtrl_1u0F:
      EMU->DCDCLNCOMPCTRL = 0x57204077UL;
      break;

    case emuDcdcLnCompCtrl_4u7F:
      EMU->DCDCLNCOMPCTRL = 0xB7102137UL;
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Load EMU_DCDCLPCTRL_LPCMPHYSSEL depending on LP bias, LP feedback
 *   attenuation and DEVINFOREV.
 *
 * @param[in] lpAttenuation
 *   LP feedback attenuation.
 * @param[in] lpCmpBias
 *   lpCmpBias selection.
 * @param[in] trimMode
 *   DCDC trim mode.
 ******************************************************************************/
static bool lpCmpHystCalibrationLoad(bool lpAttenuation,
                                     uint8_t lpCmpBias,
                                     dcdcTrimMode_TypeDef trimMode)
{
  uint32_t lpcmpHystSel;

  /* Get calib data revision */
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
  uint8_t devinfoRev = SYSTEM_GetDevinfoRev();

  /* Load LPATT indexed calibration data */
  if (devinfoRev < 4)
#else
  /* Format change not present of newer families. */
  if (false)
#endif
  {
    lpcmpHystSel = DEVINFO->DCDCLPCMPHYSSEL0;

    if (lpAttenuation)
    {
      lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL0_LPCMPHYSSELLPATT1_MASK)
                      >> _DEVINFO_DCDCLPCMPHYSSEL0_LPCMPHYSSELLPATT1_SHIFT;
    }
    else
    {
      lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL0_LPCMPHYSSELLPATT0_MASK)
                      >> _DEVINFO_DCDCLPCMPHYSSEL0_LPCMPHYSSELLPATT0_SHIFT;
    }
  }
  else
  {
    /* devinfoRev >= 4: load LPCMPBIAS indexed calibration data */
    lpcmpHystSel = DEVINFO->DCDCLPCMPHYSSEL1;
    switch (lpCmpBias)
    {
      case 0:
        lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS0_MASK)
                        >> _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS0_SHIFT;
        break;

      case 1:
        lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS1_MASK)
                        >> _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS1_SHIFT;
        break;

      case 2:
        lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS2_MASK)
                        >> _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS2_SHIFT;
        break;

      case 3:
        lpcmpHystSel = (lpcmpHystSel & _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS3_MASK)
                        >> _DEVINFO_DCDCLPCMPHYSSEL1_LPCMPHYSSELLPCMPBIAS3_SHIFT;
        break;

      default:
        EFM_ASSERT(false);
        /* Return when assertions are disabled */
        return false;
    }
  }

  /* Set trims */
  if (trimMode == dcdcTrimMode_EM234H_LP)
  {
    /* Make sure the sel value is within the field range. */
    lpcmpHystSel <<= _GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_SHIFT;
    if (lpcmpHystSel & ~_GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK)
    {
      EFM_ASSERT(false);
      /* Return when assertions are disabled */
      return false;
    }
    EMU->DCDCLPCTRL = (EMU->DCDCLPCTRL & ~_GENERIC_DCDCLPCTRL_LPCMPHYSSELEM234H_MASK) | lpcmpHystSel;
  }

#if defined( _EMU_DCDCLPEM01CFG_LPCMPHYSSELEM01_MASK )
  if (trimMode == dcdcTrimMode_EM01_LP)
  {
    /* Make sure the sel value is within the field range. */
    lpcmpHystSel <<= _EMU_DCDCLPEM01CFG_LPCMPHYSSELEM01_SHIFT;
    if (lpcmpHystSel & ~_EMU_DCDCLPEM01CFG_LPCMPHYSSELEM01_MASK)
    {
      EFM_ASSERT(false);
      /* Return when assertions are disabled */
      return false;
    }
    EMU->DCDCLPEM01CFG = (EMU->DCDCLPEM01CFG & ~_EMU_DCDCLPEM01CFG_LPCMPHYSSELEM01_MASK) | lpcmpHystSel;
  }
#endif

  return true;
}


/***************************************************************************//**
 * @brief
 *   Load LPVREF low and high from DEVINFO.
 *
 * @param[out] vrefL
 *   LPVREF low from DEVINFO.
 * @param[out] vrefH
 *   LPVREF high from DEVINFO.
 * @param[in] lpAttenuation
 *   LP feedback attenuation.
 * @param[in] lpcmpBias
 *   lpcmpBias to lookup in DEVINFO.
 ******************************************************************************/
static void lpGetDevinfoVrefLowHigh(uint32_t *vrefL,
                                    uint32_t *vrefH,
                                    bool lpAttenuation,
                                    uint8_t lpcmpBias)
{
  uint32_t vrefLow = 0;
  uint32_t vrefHigh = 0;

  /* Find VREF high and low in DEVINFO indexed by LPCMPBIAS (lpcmpBias)
     and LPATT (lpAttenuation) */
  uint32_t switchVal = (lpcmpBias << 8) | (lpAttenuation ? 1 : 0);
  switch (switchVal)
  {
    case ((0 << 8) | 1):
      vrefLow  = DEVINFO->DCDCLPVCTRL2;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL2_3V0LPATT1LPCMPBIAS0_MASK)
                 >> _DEVINFO_DCDCLPVCTRL2_3V0LPATT1LPCMPBIAS0_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL2_1V8LPATT1LPCMPBIAS0_MASK)
                 >> _DEVINFO_DCDCLPVCTRL2_1V8LPATT1LPCMPBIAS0_SHIFT;
      break;

    case ((1 << 8) | 1):
      vrefLow  = DEVINFO->DCDCLPVCTRL2;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL2_3V0LPATT1LPCMPBIAS1_MASK)
                 >> _DEVINFO_DCDCLPVCTRL2_3V0LPATT1LPCMPBIAS1_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL2_1V8LPATT1LPCMPBIAS1_MASK)
                 >> _DEVINFO_DCDCLPVCTRL2_1V8LPATT1LPCMPBIAS1_SHIFT;
      break;

    case ((2 << 8) | 1):
      vrefLow  = DEVINFO->DCDCLPVCTRL3;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL3_3V0LPATT1LPCMPBIAS2_MASK)
                 >> _DEVINFO_DCDCLPVCTRL3_3V0LPATT1LPCMPBIAS2_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL3_1V8LPATT1LPCMPBIAS2_MASK)
                 >> _DEVINFO_DCDCLPVCTRL3_1V8LPATT1LPCMPBIAS2_SHIFT;
      break;

    case ((3 << 8) | 1):
      vrefLow  = DEVINFO->DCDCLPVCTRL3;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL3_3V0LPATT1LPCMPBIAS3_MASK)
                 >> _DEVINFO_DCDCLPVCTRL3_3V0LPATT1LPCMPBIAS3_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL3_1V8LPATT1LPCMPBIAS3_MASK)
                 >> _DEVINFO_DCDCLPVCTRL3_1V8LPATT1LPCMPBIAS3_SHIFT;
      break;

    case ((0 << 8) | 0):
      vrefLow  = DEVINFO->DCDCLPVCTRL0;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL0_1V8LPATT0LPCMPBIAS0_MASK)
                 >> _DEVINFO_DCDCLPVCTRL0_1V8LPATT0LPCMPBIAS0_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL0_1V2LPATT0LPCMPBIAS0_MASK)
                 >> _DEVINFO_DCDCLPVCTRL0_1V2LPATT0LPCMPBIAS0_SHIFT;
      break;

    case ((1 << 8) | 0):
      vrefLow  = DEVINFO->DCDCLPVCTRL0;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL0_1V8LPATT0LPCMPBIAS1_MASK)
                 >> _DEVINFO_DCDCLPVCTRL0_1V8LPATT0LPCMPBIAS1_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL0_1V2LPATT0LPCMPBIAS1_MASK)
                 >> _DEVINFO_DCDCLPVCTRL0_1V2LPATT0LPCMPBIAS1_SHIFT;
      break;

    case ((2 << 8) | 0):
      vrefLow  = DEVINFO->DCDCLPVCTRL1;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL1_1V8LPATT0LPCMPBIAS2_MASK)
                 >> _DEVINFO_DCDCLPVCTRL1_1V8LPATT0LPCMPBIAS2_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL1_1V2LPATT0LPCMPBIAS2_MASK)
                 >> _DEVINFO_DCDCLPVCTRL1_1V2LPATT0LPCMPBIAS2_SHIFT;
      break;

    case ((3 << 8) | 0):
      vrefLow  = DEVINFO->DCDCLPVCTRL1;
      vrefHigh = (vrefLow & _DEVINFO_DCDCLPVCTRL1_1V8LPATT0LPCMPBIAS3_MASK)
                 >> _DEVINFO_DCDCLPVCTRL1_1V8LPATT0LPCMPBIAS3_SHIFT;
      vrefLow  = (vrefLow & _DEVINFO_DCDCLPVCTRL1_1V2LPATT0LPCMPBIAS3_MASK)
                 >> _DEVINFO_DCDCLPVCTRL1_1V2LPATT0LPCMPBIAS3_SHIFT;
      break;

    default:
      EFM_ASSERT(false);
      break;
  }
  *vrefL = vrefLow;
  *vrefH = vrefHigh;
}

/** @endcond */

/***************************************************************************//**
 * @brief
 *   Set DCDC regulator operating mode
 *
 * @param[in] dcdcMode
 *   DCDC mode
 ******************************************************************************/
void EMU_DCDCModeSet(EMU_DcdcMode_TypeDef dcdcMode)
{
  uint32_t currentDcdcMode = (EMU->DCDCCTRL & _EMU_DCDCCTRL_DCDCMODE_MASK);

  if ((EMU_DcdcMode_TypeDef)currentDcdcMode == dcdcMode)
  {
    /* Mode already set - do nothing */
    return;
  }

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)

  while(EMU->DCDCSYNC & EMU_DCDCSYNC_DCDCCTRLBUSY);
  /* Configure bypass current limiter */
  BUS_RegBitWrite(&EMU->DCDCCLIMCTRL, _EMU_DCDCCLIMCTRL_BYPLIMEN_SHIFT, dcdcMode == emuDcdcMode_Bypass ? 0 : 1);

  /* Fix for errata DCDC_E203 */
  if (((EMU_DcdcMode_TypeDef)currentDcdcMode == emuDcdcMode_Bypass)
      && (dcdcMode == emuDcdcMode_LowNoise))
  {
    errataFixDcdcHsState = errataFixDcdcHsBypassLn;
  }

#else

  /* Fix for errata DCDC_E204 */
  if (((currentDcdcMode == EMU_DCDCCTRL_DCDCMODE_OFF) || (currentDcdcMode == EMU_DCDCCTRL_DCDCMODE_BYPASS))
       && ((dcdcMode == emuDcdcMode_LowPower) || (dcdcMode == emuDcdcMode_LowNoise)))
  {
    /* Always start in LOWNOISE mode and then switch to LOWPOWER mode once LOWNOISE startup is complete. */
    EMU_IntClear(EMU_IFC_DCDCLNRUNNING);
    while(EMU->DCDCSYNC & EMU_DCDCSYNC_DCDCCTRLBUSY);
    EMU->DCDCCTRL = (EMU->DCDCCTRL & ~_EMU_DCDCCTRL_DCDCMODE_MASK) | EMU_DCDCCTRL_DCDCMODE_LOWNOISE;
    while(!(EMU_IntGet() & EMU_IF_DCDCLNRUNNING));
  }
#endif

  /* Set user requested mode. */
  while(EMU->DCDCSYNC & EMU_DCDCSYNC_DCDCCTRLBUSY);
  EMU->DCDCCTRL = (EMU->DCDCCTRL & ~_EMU_DCDCCTRL_DCDCMODE_MASK) | dcdcMode;
}


/***************************************************************************//**
 * @brief
 *   Set DCDC LN regulator conduction mode
 *
 * @param[in] conductionMode
 *   DCDC LN conduction mode.
 * @param[in] rcoDefaultSet
 *   The default DCDC RCO band for the conductionMode will be used if true.
 *   Otherwise the current RCO configuration is used.
 ******************************************************************************/
void EMU_DCDCConductionModeSet(EMU_DcdcConductionMode_TypeDef conductionMode, bool rcoDefaultSet)
{
  EMU_DcdcMode_TypeDef currentDcdcMode
    = (EMU_DcdcMode_TypeDef)(EMU->DCDCCTRL & _EMU_DCDCCTRL_DCDCMODE_MASK);
  EMU_DcdcLnRcoBand_TypeDef rcoBand
    = (EMU_DcdcLnRcoBand_TypeDef)((EMU->DCDCLNFREQCTRL & _EMU_DCDCLNFREQCTRL_RCOBAND_MASK)
                                  >> _EMU_DCDCLNFREQCTRL_RCOBAND_SHIFT);

  /* Set bypass mode and wait for bypass mode to settle before
     EMU_DCDCMISCCTRL_LNFORCECCM is set. Restore current DCDC mode. */
  EMU_IntClear(EMU_IFC_DCDCINBYPASS);
  EMU_DCDCModeSet(emuDcdcMode_Bypass);
  while(EMU->DCDCSYNC & EMU_DCDCSYNC_DCDCCTRLBUSY);
  while(!(EMU_IntGet() & EMU_IF_DCDCINBYPASS));
  if (conductionMode == emuDcdcConductionMode_DiscontinuousLN)
  {
    EMU->DCDCMISCCTRL &= ~ EMU_DCDCMISCCTRL_LNFORCECCM;
    if (rcoDefaultSet)
    {
      EMU_DCDCLnRcoBandSet(emuDcdcLnRcoBand_3MHz);
    }
    else
    {
      /* emuDcdcConductionMode_DiscontinuousLN supports up to 4MHz LN RCO. */
      EFM_ASSERT(rcoBand <= emuDcdcLnRcoBand_4MHz);
    }
  }
  else
  {
    EMU->DCDCMISCCTRL |= EMU_DCDCMISCCTRL_LNFORCECCM;
    if (rcoDefaultSet)
    {
      EMU_DCDCLnRcoBandSet(emuDcdcLnRcoBand_7MHz);
    }
  }
  EMU_DCDCModeSet(currentDcdcMode);
  /* Update slice configuration as it depends on conduction mode and RCO band. */
  EMU_DCDCOptimizeSlice(dcdcEm01LoadCurrent_mA);
}


/***************************************************************************//**
 * @brief
 *   Configure DCDC regulator
 *
 * @note
 * If the power circuit is configured for NODCDC as described in Section
 * 11.3.4.3 of the Reference Manual, do not call this function. Instead call
 * EMU_DCDCPowerOff().
 *
 * @param[in] dcdcInit
 *   DCDC initialization structure
 *
 * @return
 *   True if initialization parameters are valid
 ******************************************************************************/
bool EMU_DCDCInit(const EMU_DCDCInit_TypeDef *dcdcInit)
{
  uint32_t lpCmpBiasSelEM234H;

#if defined(_EMU_PWRCFG_MASK)
  /* Set external power configuration. This enables writing to the other
     DCDC registers. */
  EMU->PWRCFG = EMU_PWRCFG_PWRCFG_DCDCTODVDD;

  /* EMU->PWRCFG is write-once and POR reset only. Check that
     we could set the desired power configuration. */
  if ((EMU->PWRCFG & _EMU_PWRCFG_PWRCFG_MASK) != EMU_PWRCFG_PWRCFG_DCDCTODVDD)
  {
    /* If this assert triggers unexpectedly, please power cycle the
       kit to reset the power configuration. */
    EFM_ASSERT(false);
    /* Return when assertions are disabled */
    return false;
  }
#endif

  /* Load DCDC calibration data from the DI page */
  dcdcConstCalibrationLoad();

  /* Check current parameters */
  EFM_ASSERT(dcdcInit->maxCurrent_mA <= 200);
  EFM_ASSERT(dcdcInit->em01LoadCurrent_mA <= dcdcInit->maxCurrent_mA);
  EFM_ASSERT(dcdcInit->reverseCurrentControl <= 200);

  if (dcdcInit->dcdcMode == emuDcdcMode_LowNoise)
  {
    /* DCDC low-noise supports max 200mA */
    EFM_ASSERT(dcdcInit->em01LoadCurrent_mA <= 200);
  }
#if (_SILICON_LABS_GECKO_INTERNAL_SDID != 80)
  else if (dcdcInit->dcdcMode == emuDcdcMode_LowPower)
  {
    /* Up to 10mA is supported for EM01-LP mode. */
    EFM_ASSERT(dcdcInit->em01LoadCurrent_mA <= 10);
  }
#endif

  /* EM2/3/4 current above 10mA is not supported */
  EFM_ASSERT(dcdcInit->em234LoadCurrent_uA <= 10000);

  if (dcdcInit->em234LoadCurrent_uA < 75)
  {
    lpCmpBiasSelEM234H  = 0;
  }
  else if (dcdcInit->em234LoadCurrent_uA < 500)
  {
    lpCmpBiasSelEM234H  = 1 << _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT;
  }
  else if (dcdcInit->em234LoadCurrent_uA < 2500)
  {
    lpCmpBiasSelEM234H  = 2 << _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT;
  }
  else
  {
    lpCmpBiasSelEM234H  = 3 << _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT;
  }

  /* ==== THESE NEXT STEPS ARE STRONGLY ORDER DEPENDENT ==== */

  /* Set DCDC low-power mode comparator bias selection */

  /* 1. Set DCDC low-power mode comparator bias selection and forced CCM
        => Updates DCDCMISCCTRL_LNFORCECCM */
  EMU->DCDCMISCCTRL = (EMU->DCDCMISCCTRL & ~(_GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_MASK
                                             | _EMU_DCDCMISCCTRL_LNFORCECCM_MASK))
                       | ((uint32_t)lpCmpBiasSelEM234H
                          | (dcdcInit->reverseCurrentControl >= 0 ?
                             EMU_DCDCMISCCTRL_LNFORCECCM : 0));
#if defined(_EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK)
  /* Only 10mA EM01-LP current is supported */
  EMU->DCDCLPEM01CFG = (EMU->DCDCLPEM01CFG & ~_EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK)
                       | EMU_DCDCLPEM01CFG_LPCMPBIASEM01_BIAS3;
#endif

  /* 2. Set recommended and validated current optimization settings
        <= Depends on LNFORCECCM
        => Updates DCDCLNFREQCTRL_RCOBAND */
  dcdcValidatedConfigSet();

  /* 3. Updated static currents and limits user data.
        Limiters are updated in EMU_DCDCOptimizeSlice() */
  userCurrentLimitsSet(dcdcInit->maxCurrent_mA,
                       dcdcInit->reverseCurrentControl);
  dcdcEm01LoadCurrent_mA = dcdcInit->em01LoadCurrent_mA;

  /* 4. Optimize LN slice based on given user input load current
        <= Depends on DCDCMISCCTRL_LNFORCECCM and DCDCLNFREQCTRL_RCOBAND
        <= Depends on dcdcInit->maxCurrent_mA and dcdcInit->reverseCurrentControl
        => Updates DCDCMISCCTRL_P/NFETCNT
        => Updates DCDCMISCCTRL_LNCLIMILIMSEL and DCDCMISCCTRL_LPCLIMILIMSEL
        => Updates DCDCZDETCTRL_ZDETILIMSEL */
  EMU_DCDCOptimizeSlice(dcdcInit->em01LoadCurrent_mA);

  /* ======================================================= */

  /* Set DCDC low noise mode compensator control register. */
  compCtrlSet(dcdcInit->dcdcLnCompCtrl);

  /* Set DCDC output voltage */
  if (!EMU_DCDCOutputVoltageSet(dcdcInit->mVout, true, true))
  {
    EFM_ASSERT(false);
    /* Return when assertions are disabled */
    return false;
  }

#if ( _SILICON_LABS_GECKO_INTERNAL_SDID == 80 )
  /* Select analog peripheral power supply. This must be done before
     DCDC mode is set for all EFM32xG1 and EFR32xG1 devices. */
  BUS_RegBitWrite(&EMU->PWRCTRL,
                  _EMU_PWRCTRL_ANASW_SHIFT,
                  dcdcInit->anaPeripheralPower ? 1 : 0);
#endif

#if defined(_EMU_PWRCTRL_REGPWRSEL_MASK)
  /* Select DVDD as input to the digital regulator. The switch to DVDD will take
     effect once the DCDC output is stable. */
  EMU->PWRCTRL |= EMU_PWRCTRL_REGPWRSEL_DVDD;
#endif

  /* Set EM0 DCDC operating mode. Output voltage set in
     EMU_DCDCOutputVoltageSet() above takes effect if mode
     is changed from bypass/off mode. */
  EMU_DCDCModeSet(dcdcInit->dcdcMode);

#if ( _SILICON_LABS_GECKO_INTERNAL_SDID != 80 )
  /* Select analog peripheral power supply. This must be done after
     DCDC mode is set for all devices other than EFM32xG1 and EFR32xG1. */
  BUS_RegBitWrite(&EMU->PWRCTRL,
                  _EMU_PWRCTRL_ANASW_SHIFT,
                  dcdcInit->anaPeripheralPower ? 1 : 0);
#endif

  return true;
}

/***************************************************************************//**
 * @brief
 *   Set DCDC output voltage
 *
 * @param[in] mV
 *   Target DCDC output voltage in mV
 *
 * @return
 *   True if the mV parameter is valid
 ******************************************************************************/
bool EMU_DCDCOutputVoltageSet(uint32_t mV,
                              bool setLpVoltage,
                              bool setLnVoltage)
{
#if defined( _DEVINFO_DCDCLNVCTRL0_3V0LNATT1_MASK )

#define DCDC_TRIM_MODES ((uint8_t)dcdcTrimMode_LN + 1)
  bool validOutVoltage;
  bool attenuationSet;
  uint32_t mVlow = 0;
  uint32_t mVhigh = 0;
  uint32_t mVdiff;
  uint32_t vrefVal[DCDC_TRIM_MODES] = {0};
  uint32_t vrefLow[DCDC_TRIM_MODES] = {0};
  uint32_t vrefHigh[DCDC_TRIM_MODES] = {0};
  uint8_t lpcmpBias[DCDC_TRIM_MODES] = {0};

  /* Check that the set voltage is within valid range.
     Voltages are obtained from the datasheet. */
  validOutVoltage = ((mV >= PWRCFG_DCDCTODVDD_VMIN)
                     && (mV <= PWRCFG_DCDCTODVDD_VMAX));

  if (!validOutVoltage)
  {
    EFM_ASSERT(false);
    /* Return when assertions are disabled */
    return false;
  }

  /* Set attenuation to use and low/high range. */
  attenuationSet = (mV > 1800);
  if (attenuationSet)
  {
    mVlow = 1800;
    mVhigh = 3000;
    mVdiff = mVhigh - mVlow;
  }
  else
  {
    mVlow = 1200;
    mVhigh = 1800;
    mVdiff = mVhigh - mVlow;
  }

    /* Get 2-point calib data from DEVINFO */

    /* LN mode */
    if (attenuationSet)
    {
      vrefLow[dcdcTrimMode_LN]  = DEVINFO->DCDCLNVCTRL0;
      vrefHigh[dcdcTrimMode_LN] = (vrefLow[dcdcTrimMode_LN] & _DEVINFO_DCDCLNVCTRL0_3V0LNATT1_MASK)
                                   >> _DEVINFO_DCDCLNVCTRL0_3V0LNATT1_SHIFT;
      vrefLow[dcdcTrimMode_LN]  = (vrefLow[dcdcTrimMode_LN] & _DEVINFO_DCDCLNVCTRL0_1V8LNATT1_MASK)
                                   >> _DEVINFO_DCDCLNVCTRL0_1V8LNATT1_SHIFT;
    }
    else
    {
      vrefLow[dcdcTrimMode_LN]  = DEVINFO->DCDCLNVCTRL0;
      vrefHigh[dcdcTrimMode_LN] = (vrefLow[dcdcTrimMode_LN] & _DEVINFO_DCDCLNVCTRL0_1V8LNATT0_MASK)
                                   >> _DEVINFO_DCDCLNVCTRL0_1V8LNATT0_SHIFT;
      vrefLow[dcdcTrimMode_LN]  = (vrefLow[dcdcTrimMode_LN] & _DEVINFO_DCDCLNVCTRL0_1V2LNATT0_MASK)
                                   >> _DEVINFO_DCDCLNVCTRL0_1V2LNATT0_SHIFT;
    }


    /* LP EM234H mode */
    lpcmpBias[dcdcTrimMode_EM234H_LP] = (EMU->DCDCMISCCTRL & _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_MASK)
                                         >> _GENERIC_DCDCMISCCTRL_LPCMPBIASEM234H_SHIFT;
    lpGetDevinfoVrefLowHigh(&vrefLow[dcdcTrimMode_EM234H_LP],
                            &vrefHigh[dcdcTrimMode_EM234H_LP],
                            attenuationSet,
                            lpcmpBias[dcdcTrimMode_EM234H_LP]);

#if defined( _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK )
    /* LP EM01 mode */
    lpcmpBias[dcdcTrimMode_EM01_LP] = (EMU->DCDCLPEM01CFG & _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK)
                                       >> _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_SHIFT;
    lpGetDevinfoVrefLowHigh(&vrefLow[dcdcTrimMode_EM01_LP],
                            &vrefHigh[dcdcTrimMode_EM01_LP],
                            attenuationSet,
                            lpcmpBias[dcdcTrimMode_EM01_LP]);
#endif


    /* Calculate output voltage trims */
    vrefVal[dcdcTrimMode_LN]         = ((mV - mVlow) * (vrefHigh[dcdcTrimMode_LN] - vrefLow[dcdcTrimMode_LN]))
                                       / mVdiff;
    vrefVal[dcdcTrimMode_LN]        += vrefLow[dcdcTrimMode_LN];

    vrefVal[dcdcTrimMode_EM234H_LP]  = ((mV - mVlow) * (vrefHigh[dcdcTrimMode_EM234H_LP] - vrefLow[dcdcTrimMode_EM234H_LP]))
                                       / mVdiff;
    vrefVal[dcdcTrimMode_EM234H_LP] += vrefLow[dcdcTrimMode_EM234H_LP];

#if defined( _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK )
    vrefVal[dcdcTrimMode_EM01_LP]    = ((mV - mVlow) * (vrefHigh[dcdcTrimMode_EM01_LP] - vrefLow[dcdcTrimMode_EM01_LP]))
                                       / mVdiff;
    vrefVal[dcdcTrimMode_EM01_LP]   += vrefLow[dcdcTrimMode_EM01_LP];
#endif

  /* Range checks */
  if ((vrefVal[dcdcTrimMode_LN] > vrefHigh[dcdcTrimMode_LN])
      || (vrefVal[dcdcTrimMode_LN] < vrefLow[dcdcTrimMode_LN])
#if defined( _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK )
      || (vrefVal[dcdcTrimMode_EM01_LP] > vrefHigh[dcdcTrimMode_EM01_LP])
      || (vrefVal[dcdcTrimMode_EM01_LP] < vrefLow[dcdcTrimMode_EM01_LP])
#endif
      || (vrefVal[dcdcTrimMode_EM234H_LP] > vrefHigh[dcdcTrimMode_EM234H_LP])
      || (vrefVal[dcdcTrimMode_EM234H_LP] < vrefLow[dcdcTrimMode_EM234H_LP]))
  {
    EFM_ASSERT(false);
    /* Return when assertions are disabled */
    return false;
  }

  /* Update output voltage tuning for LN and LP modes. */
  if (setLnVoltage)
  {
    EMU->DCDCLNVCTRL = (EMU->DCDCLNVCTRL & ~(_EMU_DCDCLNVCTRL_LNVREF_MASK | _EMU_DCDCLNVCTRL_LNATT_MASK))
                        | (vrefVal[dcdcTrimMode_LN] << _EMU_DCDCLNVCTRL_LNVREF_SHIFT)
                        | (attenuationSet ? EMU_DCDCLNVCTRL_LNATT : 0);
  }

  if (setLpVoltage)
  {
    /* Load LP EM234H comparator hysteresis calibration */
    if(!(lpCmpHystCalibrationLoad(attenuationSet, lpcmpBias[dcdcTrimMode_EM234H_LP], dcdcTrimMode_EM234H_LP)))
    {
      EFM_ASSERT(false);
      /* Return when assertions are disabled */
      return false;
    }

#if defined( _EMU_DCDCLPEM01CFG_LPCMPBIASEM01_MASK )
    /* Load LP EM234H comparator hysteresis calibration */
    if(!(lpCmpHystCalibrationLoad(attenuationSet, lpcmpBias[dcdcTrimMode_EM01_LP], dcdcTrimMode_EM01_LP)))
    {
      EFM_ASSERT(false);
      /* Return when assertions are disabled */
      return false;
    }

    /* LP VREF is that max of trims for EM01 and EM234H. */
    vrefVal[dcdcTrimMode_EM234H_LP] = SL_MAX(vrefVal[dcdcTrimMode_EM234H_LP], vrefVal[dcdcTrimMode_EM01_LP]);
#endif

    /* Don't exceed max available code as specified in the reference manual for EMU_DCDCLPVCTRL. */
    vrefVal[dcdcTrimMode_EM234H_LP] = SL_MIN(vrefVal[dcdcTrimMode_EM234H_LP], 0xE7U);
    EMU->DCDCLPVCTRL = (EMU->DCDCLPVCTRL & ~(_EMU_DCDCLPVCTRL_LPVREF_MASK | _EMU_DCDCLPVCTRL_LPATT_MASK))
                        | (vrefVal[dcdcTrimMode_EM234H_LP] << _EMU_DCDCLPVCTRL_LPVREF_SHIFT)
                        | (attenuationSet ? EMU_DCDCLPVCTRL_LPATT : 0);
  }
#endif
  return true;
}


/***************************************************************************//**
 * @brief
 *   Optimize DCDC slice count based on the estimated average load current
 *   in EM0
 *
 * @param[in] em0LoadCurrent_mA
 *   Estimated average EM0 load current in mA.
 ******************************************************************************/
void EMU_DCDCOptimizeSlice(uint32_t em0LoadCurrent_mA)
{
  uint32_t sliceCount = 0;
  uint32_t rcoBand = (EMU->DCDCLNFREQCTRL & _EMU_DCDCLNFREQCTRL_RCOBAND_MASK)
                      >> _EMU_DCDCLNFREQCTRL_RCOBAND_SHIFT;

  /* Set recommended slice count */
  if ((EMU->DCDCMISCCTRL & _EMU_DCDCMISCCTRL_LNFORCECCM_MASK) && (rcoBand >= emuDcdcLnRcoBand_5MHz))
  {
    if (em0LoadCurrent_mA < 20)
    {
      sliceCount = 4;
    }
    else if ((em0LoadCurrent_mA >= 20) && (em0LoadCurrent_mA < 40))
    {
      sliceCount = 8;
    }
    else
    {
      sliceCount = 16;
    }
  }
  else if ((!(EMU->DCDCMISCCTRL & _EMU_DCDCMISCCTRL_LNFORCECCM_MASK)) && (rcoBand <= emuDcdcLnRcoBand_4MHz))
  {
    if (em0LoadCurrent_mA < 10)
    {
      sliceCount = 4;
    }
    else if ((em0LoadCurrent_mA >= 10) && (em0LoadCurrent_mA < 20))
    {
      sliceCount = 8;
    }
    else
    {
      sliceCount = 16;
    }
  }
  else if ((EMU->DCDCMISCCTRL & _EMU_DCDCMISCCTRL_LNFORCECCM_MASK) && (rcoBand <= emuDcdcLnRcoBand_4MHz))
  {
    if (em0LoadCurrent_mA < 40)
    {
      sliceCount = 8;
    }
    else
    {
      sliceCount = 16;
    }
  }
  else
  {
    /* This configuration is not recommended. EMU_DCDCInit() applies a recommended
       configuration. */
    EFM_ASSERT(false);
  }

  /* The selected slices are PSLICESEL + 1 */
  sliceCount--;

  /* Apply slice count to both N and P slice */
  sliceCount = (sliceCount << _EMU_DCDCMISCCTRL_PFETCNT_SHIFT
                | sliceCount << _EMU_DCDCMISCCTRL_NFETCNT_SHIFT);
  EMU->DCDCMISCCTRL = (EMU->DCDCMISCCTRL & ~(_EMU_DCDCMISCCTRL_PFETCNT_MASK
                                             | _EMU_DCDCMISCCTRL_NFETCNT_MASK))
                      | sliceCount;

  /* Update current limiters */
  currentLimitersUpdate();
}

/***************************************************************************//**
 * @brief
 *   Set DCDC Low-noise RCO band.
 *
 * @param[in] band
 *   RCO band to set.
 ******************************************************************************/
void EMU_DCDCLnRcoBandSet(EMU_DcdcLnRcoBand_TypeDef band)
{
  uint32_t forcedCcm;
  forcedCcm = BUS_RegBitRead(&EMU->DCDCMISCCTRL, _EMU_DCDCMISCCTRL_LNFORCECCM_SHIFT);

  /* DCM mode supports up to 4MHz LN RCO. */
  EFM_ASSERT((!forcedCcm && band <= emuDcdcLnRcoBand_4MHz) || forcedCcm);

  EMU->DCDCLNFREQCTRL = (EMU->DCDCLNFREQCTRL & ~_EMU_DCDCLNFREQCTRL_RCOBAND_MASK)
                         | (band << _EMU_DCDCLNFREQCTRL_RCOBAND_SHIFT);

  /* Update slice configuration as this depends on the RCO band. */
  EMU_DCDCOptimizeSlice(dcdcEm01LoadCurrent_mA);
}

/***************************************************************************//**
 * @brief
 *   Power off the DCDC regulator.
 *
 * @details
 *   This function powers off the DCDC controller. This function should only be
 *   used if the external power circuit is wired for no DCDC. If the external power
 *   circuit is wired for DCDC usage, then use EMU_DCDCInit() and set the
 *   DCDC in bypass mode to disable DCDC.
 *
 * @return
 *   Return false if the DCDC could not be disabled.
 ******************************************************************************/
bool EMU_DCDCPowerOff(void)
{
  bool dcdcModeSet;

#if defined(_EMU_PWRCFG_MASK)
  /* Set DCDCTODVDD only to enable write access to EMU->DCDCCTRL */
  EMU->PWRCFG = EMU_PWRCFG_PWRCFG_DCDCTODVDD;
#endif

  /* Select DVDD as input to the digital regulator */
#if defined(EMU_PWRCTRL_IMMEDIATEPWRSWITCH)
  EMU->PWRCTRL |= EMU_PWRCTRL_REGPWRSEL_DVDD | EMU_PWRCTRL_IMMEDIATEPWRSWITCH;
#elif defined(EMU_PWRCTRL_REGPWRSEL_DVDD)
  EMU->PWRCTRL |= EMU_PWRCTRL_REGPWRSEL_DVDD;
#endif

  /* Set DCDC to OFF and disable LP in EM2/3/4. Verify that the required
     mode could be set. */
  while(EMU->DCDCSYNC & EMU_DCDCSYNC_DCDCCTRLBUSY);
  EMU->DCDCCTRL = EMU_DCDCCTRL_DCDCMODE_OFF;

  dcdcModeSet = (EMU->DCDCCTRL == EMU_DCDCCTRL_DCDCMODE_OFF);
  EFM_ASSERT(dcdcModeSet);

  return dcdcModeSet;
}
#endif


#if defined( EMU_STATUS_VMONRDY )
/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
__STATIC_INLINE uint32_t vmonMilliVoltToCoarseThreshold(int mV)
{
  return (mV - 1200) / 200;
}

__STATIC_INLINE uint32_t vmonMilliVoltToFineThreshold(int mV, uint32_t coarseThreshold)
{
  return (mV - 1200 - (coarseThreshold * 200)) / 20;
}
/** @endcond */

/***************************************************************************//**
 * @brief
 *   Initialize VMON channel.
 *
 * @details
 *   Initialize a VMON channel without hysteresis. If the channel supports
 *   separate rise and fall triggers, both thresholds will be set to the same
 *   value.
 *
 * @param[in] vmonInit
 *   VMON initialization struct
 ******************************************************************************/
void EMU_VmonInit(const EMU_VmonInit_TypeDef *vmonInit)
{
  uint32_t thresholdCoarse, thresholdFine;
  EFM_ASSERT((vmonInit->threshold >= 1200) && (vmonInit->threshold <= 3980));

  thresholdCoarse = vmonMilliVoltToCoarseThreshold(vmonInit->threshold);
  thresholdFine = vmonMilliVoltToFineThreshold(vmonInit->threshold, thresholdCoarse);

  switch(vmonInit->channel)
  {
  case emuVmonChannel_AVDD:
    EMU->VMONAVDDCTRL = (thresholdCoarse << _EMU_VMONAVDDCTRL_RISETHRESCOARSE_SHIFT)
                      | (thresholdFine << _EMU_VMONAVDDCTRL_RISETHRESFINE_SHIFT)
                      | (thresholdCoarse << _EMU_VMONAVDDCTRL_FALLTHRESCOARSE_SHIFT)
                      | (thresholdFine << _EMU_VMONAVDDCTRL_FALLTHRESFINE_SHIFT)
                      | (vmonInit->riseWakeup ? EMU_VMONAVDDCTRL_RISEWU : 0)
                      | (vmonInit->fallWakeup ? EMU_VMONAVDDCTRL_FALLWU : 0)
                      | (vmonInit->enable     ? EMU_VMONAVDDCTRL_EN     : 0);
    break;
  case emuVmonChannel_ALTAVDD:
    EMU->VMONALTAVDDCTRL = (thresholdCoarse << _EMU_VMONALTAVDDCTRL_THRESCOARSE_SHIFT)
                         | (thresholdFine << _EMU_VMONALTAVDDCTRL_THRESFINE_SHIFT)
                         | (vmonInit->riseWakeup ? EMU_VMONALTAVDDCTRL_RISEWU : 0)
                         | (vmonInit->fallWakeup ? EMU_VMONALTAVDDCTRL_FALLWU : 0)
                         | (vmonInit->enable     ? EMU_VMONALTAVDDCTRL_EN     : 0);
    break;
  case emuVmonChannel_DVDD:
    EMU->VMONDVDDCTRL = (thresholdCoarse << _EMU_VMONDVDDCTRL_THRESCOARSE_SHIFT)
                      | (thresholdFine << _EMU_VMONDVDDCTRL_THRESFINE_SHIFT)
                      | (vmonInit->riseWakeup ? EMU_VMONDVDDCTRL_RISEWU : 0)
                      | (vmonInit->fallWakeup ? EMU_VMONDVDDCTRL_FALLWU : 0)
                      | (vmonInit->enable     ? EMU_VMONDVDDCTRL_EN     : 0);
    break;
  case emuVmonChannel_IOVDD0:
    EMU->VMONIO0CTRL = (thresholdCoarse << _EMU_VMONIO0CTRL_THRESCOARSE_SHIFT)
                     | (thresholdFine << _EMU_VMONIO0CTRL_THRESFINE_SHIFT)
                     | (vmonInit->retDisable ? EMU_VMONIO0CTRL_RETDIS : 0)
                     | (vmonInit->riseWakeup ? EMU_VMONIO0CTRL_RISEWU : 0)
                     | (vmonInit->fallWakeup ? EMU_VMONIO0CTRL_FALLWU : 0)
                     | (vmonInit->enable     ? EMU_VMONIO0CTRL_EN     : 0);
    break;
  default:
    EFM_ASSERT(false);
    return;
  }
}

/***************************************************************************//**
 * @brief
 *   Initialize VMON channel with hysteresis (separate rise and fall triggers).
 *
 * @details
 *   Initialize a VMON channel which supports hysteresis. The AVDD channel is
 *   the only channel to support separate rise and fall triggers.
 *
 * @param[in] vmonInit
 *   VMON Hysteresis initialization struct
 ******************************************************************************/
void EMU_VmonHystInit(const EMU_VmonHystInit_TypeDef *vmonInit)
{
  uint32_t riseThresholdCoarse, riseThresholdFine, fallThresholdCoarse, fallThresholdFine;
  /* VMON supports voltages between 1200 mV and 3980 mV (inclusive) in 20 mV increments */
  EFM_ASSERT((vmonInit->riseThreshold >= 1200) && (vmonInit->riseThreshold < 4000));
  EFM_ASSERT((vmonInit->fallThreshold >= 1200) && (vmonInit->fallThreshold < 4000));
  /* Fall threshold has to be lower than rise threshold */
  EFM_ASSERT(vmonInit->fallThreshold <= vmonInit->riseThreshold);

  riseThresholdCoarse = vmonMilliVoltToCoarseThreshold(vmonInit->riseThreshold);
  riseThresholdFine = vmonMilliVoltToFineThreshold(vmonInit->riseThreshold, riseThresholdCoarse);
  fallThresholdCoarse = vmonMilliVoltToCoarseThreshold(vmonInit->fallThreshold);
  fallThresholdFine = vmonMilliVoltToFineThreshold(vmonInit->fallThreshold, fallThresholdCoarse);

  switch(vmonInit->channel)
  {
  case emuVmonChannel_AVDD:
    EMU->VMONAVDDCTRL = (riseThresholdCoarse << _EMU_VMONAVDDCTRL_RISETHRESCOARSE_SHIFT)
                      | (riseThresholdFine << _EMU_VMONAVDDCTRL_RISETHRESFINE_SHIFT)
                      | (fallThresholdCoarse << _EMU_VMONAVDDCTRL_FALLTHRESCOARSE_SHIFT)
                      | (fallThresholdFine << _EMU_VMONAVDDCTRL_FALLTHRESFINE_SHIFT)
                      | (vmonInit->riseWakeup ? EMU_VMONAVDDCTRL_RISEWU : 0)
                      | (vmonInit->fallWakeup ? EMU_VMONAVDDCTRL_FALLWU : 0)
                      | (vmonInit->enable     ? EMU_VMONAVDDCTRL_EN     : 0);
    break;
  default:
    EFM_ASSERT(false);
    return;
  }
}

/***************************************************************************//**
 * @brief
 *   Enable or disable a VMON channel
 *
 * @param[in] channel
 *   VMON channel to enable/disable
 *
 * @param[in] enable
 *   Whether to enable or disable
 ******************************************************************************/
void EMU_VmonEnable(EMU_VmonChannel_TypeDef channel, bool enable)
{
  uint32_t volatile * reg;
  uint32_t bit;

  switch(channel)
  {
  case emuVmonChannel_AVDD:
    reg = &(EMU->VMONAVDDCTRL);
    bit = _EMU_VMONAVDDCTRL_EN_SHIFT;
    break;
  case emuVmonChannel_ALTAVDD:
    reg = &(EMU->VMONALTAVDDCTRL);
    bit = _EMU_VMONALTAVDDCTRL_EN_SHIFT;
    break;
  case emuVmonChannel_DVDD:
    reg = &(EMU->VMONDVDDCTRL);
    bit = _EMU_VMONDVDDCTRL_EN_SHIFT;
    break;
  case emuVmonChannel_IOVDD0:
    reg = &(EMU->VMONIO0CTRL);
    bit = _EMU_VMONIO0CTRL_EN_SHIFT;
    break;
  default:
    EFM_ASSERT(false);
    return;
  }

  BUS_RegBitWrite(reg, bit, enable);
}

/***************************************************************************//**
 * @brief
 *   Get the status of a voltage monitor channel.
 *
 * @param[in] channel
 *   VMON channel to get status for
 *
 * @return
 *   Status of the selected VMON channel. True if channel is triggered.
 ******************************************************************************/
bool EMU_VmonChannelStatusGet(EMU_VmonChannel_TypeDef channel)
{
  uint32_t bit;
  switch(channel)
  {
  case emuVmonChannel_AVDD:
    bit = _EMU_STATUS_VMONAVDD_SHIFT;
    break;
  case emuVmonChannel_ALTAVDD:
    bit = _EMU_STATUS_VMONALTAVDD_SHIFT;
    break;
  case emuVmonChannel_DVDD:
    bit = _EMU_STATUS_VMONDVDD_SHIFT;
    break;
  case emuVmonChannel_IOVDD0:
    bit = _EMU_STATUS_VMONIO0_SHIFT;
    break;
  default:
    EFM_ASSERT(false);
    bit = 0;
  }

  return BUS_RegBitRead(&EMU->STATUS, bit);
}
#endif /* EMU_STATUS_VMONRDY */

#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
/***************************************************************************//**
 * @brief
 *   Adjust the bias refresh rate
 *
 * @details
 *   This function is only meant to be used under high-temperature operation on
 *   EFR32xG1 and EFM32xG1 devices. Adjusting the bias mode will
 *   increase the typical current consumption. See application note 1027
 *   and errata documents for further details.
 *
 * @param [in] mode
 *   The new bias refresh rate
 ******************************************************************************/
void EMU_SetBiasMode(EMU_BiasMode_TypeDef mode)
{
#define EMU_TESTLOCK         (*(volatile uint32_t *) (EMU_BASE + 0x190))
#define EMU_BIASCONF         (*(volatile uint32_t *) (EMU_BASE + 0x164))
#define EMU_BIASTESTCTRL     (*(volatile uint32_t *) (EMU_BASE + 0x19C))
#define CMU_ULFRCOCTRL       (*(volatile uint32_t *) (CMU_BASE + 0x03C))

  uint32_t freq = 0x2u;
  bool emuTestLocked = false;

  if (mode == emuBiasMode_1KHz)
  {
    freq = 0x0u;
  }

  if (EMU_TESTLOCK == 0x1u)
  {
    emuTestLocked = true;
    EMU_TESTLOCK = 0xADE8u;
  }

  if (mode == emuBiasMode_Continuous)
  {
    EMU_BIASCONF &= ~0x74u;
  }
  else
  {
    EMU_BIASCONF |= 0x74u;
  }

  EMU_BIASTESTCTRL |= 0x8u;
  CMU_ULFRCOCTRL    = (CMU_ULFRCOCTRL & ~0xC00u)
                      | ((freq & 0x3u) << 10u);
  EMU_BIASTESTCTRL &= ~0x8u;

  if (emuTestLocked)
  {
    EMU_TESTLOCK = 0u;
  }
}
#endif

/** @} (end addtogroup EMU) */
/** @} (end addtogroup emlib) */
#endif /* __EM_EMU_H */

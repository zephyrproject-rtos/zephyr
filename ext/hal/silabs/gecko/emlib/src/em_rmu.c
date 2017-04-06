/***************************************************************************//**
 * @file em_rmu.c
 * @brief Reset Management Unit (RMU) peripheral module peripheral API
 *
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

#include "em_rmu.h"
#if defined(RMU_COUNT) && (RMU_COUNT > 0)

#include "em_common.h"
#include "em_emu.h"
#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup RMU
 * @brief Reset Management Unit (RMU) Peripheral API
 * @details
 *  This module contains functions to control the RMU peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The RMU ensures correct reset operation. It is
 *  responsible for connecting the different reset sources to the reset lines of
 *  the MCU or SoC.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *****************************     DEFINES     *********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/* Reset cause zero and "don't care" bit definitions (XMASKs).
   A XMASK 1 bit marks a bit that must be zero in RMU_RSTCAUSE. A 0 in XMASK
   is a "don't care" bit in RMU_RSTCAUSE if also 0 in resetCauseMask
   in @ref RMU_ResetCauseMasks_Typedef. */

/* EFM32G */
#if (_RMU_RSTCAUSE_MASK == 0x0000007FUL)
#define RMU_RSTCAUSE_PORST_XMASK         0x00000000UL /** 0000000000000000  < Power On Reset */
#define RMU_RSTCAUSE_BODUNREGRST_XMASK   0x00000001UL /** 0000000000000001  < Brown Out Detector Unregulated Domain Reset */
#define RMU_RSTCAUSE_BODREGRST_XMASK     0x0000001BUL /** 0000000000011011  < Brown Out Detector Regulated Domain Reset */
#define RMU_RSTCAUSE_EXTRST_XMASK        0x00000003UL /** 0000000000000011  < External Pin Reset */
#define RMU_RSTCAUSE_WDOGRST_XMASK       0x00000003UL /** 0000000000000011  < Watchdog Reset */
#define RMU_RSTCAUSE_LOCKUPRST_XMASK     0x0000001FUL /** 0000000000011111  < LOCKUP Reset */
#define RMU_RSTCAUSE_SYSREQRST_XMASK     0x0000001FUL /** 0000000000011111  < System Request Reset */
#define NUM_RSTCAUSES                               7

/* EFM32TG, EFM32HG, EZR32HG, EFM32ZG */
#elif (_RMU_RSTCAUSE_MASK == 0x000007FFUL)
#define RMU_RSTCAUSE_PORST_XMASK         0x00000000UL /** 0000000000000000  < Power On Reset */
#define RMU_RSTCAUSE_BODUNREGRST_XMASK   0x00000081UL /** 0000000010000001  < Brown Out Detector Unregulated Domain Reset */
#define RMU_RSTCAUSE_BODREGRST_XMASK     0x00000091UL /** 0000000010010001  < Brown Out Detector Regulated Domain Reset */
#define RMU_RSTCAUSE_EXTRST_XMASK        0x00000001UL /** 0000000000000001  < External Pin Reset */
#define RMU_RSTCAUSE_WDOGRST_XMASK       0x00000003UL /** 0000000000000011  < Watchdog Reset */
#define RMU_RSTCAUSE_LOCKUPRST_XMASK     0x0000EFDFUL /** 1110111111011111  < LOCKUP Reset */
#define RMU_RSTCAUSE_SYSREQRST_XMASK     0x0000EF9FUL /** 1110111110011111  < System Request Reset */
#define RMU_RSTCAUSE_EM4RST_XMASK        0x00000719UL /** 0000011100011001  < EM4 Reset */
#define RMU_RSTCAUSE_EM4WURST_XMASK      0x00000619UL /** 0000011000011001  < EM4 Wake-up Reset */
#define RMU_RSTCAUSE_BODAVDD0_XMASK      0x0000041FUL /** 0000010000011111  < AVDD0 Bod Reset. */
#define RMU_RSTCAUSE_BODAVDD1_XMASK      0x0000021FUL /** 0000001000011111  < AVDD1 Bod Reset. */
#define NUM_RSTCAUSES                              11

/* EFM32GG, EFM32LG, EZR32LG, EFM32WG, EZR32WG */
#elif (_RMU_RSTCAUSE_MASK == 0x0000FFFFUL)
#define RMU_RSTCAUSE_PORST_XMASK         0x00000000UL /** 0000000000000000  < Power On Reset */
#define RMU_RSTCAUSE_BODUNREGRST_XMASK   0x00000081UL /** 0000000010000001  < Brown Out Detector Unregulated Domain Reset */
#define RMU_RSTCAUSE_BODREGRST_XMASK     0x00000091UL /** 0000000010010001  < Brown Out Detector Regulated Domain Reset */
#define RMU_RSTCAUSE_EXTRST_XMASK        0x00000001UL /** 0000000000000001  < External Pin Reset */
#define RMU_RSTCAUSE_WDOGRST_XMASK       0x00000003UL /** 0000000000000011  < Watchdog Reset */
#define RMU_RSTCAUSE_LOCKUPRST_XMASK     0x0000EFDFUL /** 1110111111011111  < LOCKUP Reset */
#define RMU_RSTCAUSE_SYSREQRST_XMASK     0x0000EF9FUL /** 1110111110011111  < System Request Reset */
#define RMU_RSTCAUSE_EM4RST_XMASK        0x00000719UL /** 0000011100011001  < EM4 Reset */
#define RMU_RSTCAUSE_EM4WURST_XMASK      0x00000619UL /** 0000011000011001  < EM4 Wake-up Reset */
#define RMU_RSTCAUSE_BODAVDD0_XMASK      0x0000041FUL /** 0000010000011111  < AVDD0 Bod Reset */
#define RMU_RSTCAUSE_BODAVDD1_XMASK      0x0000021FUL /** 0000001000011111  < AVDD1 Bod Reset */
#define RMU_RSTCAUSE_BUBODVDDDREG_XMASK  0x00000001UL /** 0000000000000001  < Backup Brown Out Detector, VDD_DREG */
#define RMU_RSTCAUSE_BUBODBUVIN_XMASK    0x00000001UL /** 0000000000000001  < Backup Brown Out Detector, BU_VIN */
#define RMU_RSTCAUSE_BUBODUNREG_XMASK    0x00000001UL /** 0000000000000001  < Backup Brown Out Detector Unregulated Domain */
#define RMU_RSTCAUSE_BUBODREG_XMASK      0x00000001UL /** 0000000000000001  < Backup Brown Out Detector Regulated Domain */
#define RMU_RSTCAUSE_BUMODERST_XMASK     0x00000001UL /** 0000000000000001  < Backup mode reset */
#define NUM_RSTCAUSES                              16

/* EFM32xG1, EFM32xG12, EFM32xG13 */
#elif ((_RMU_RSTCAUSE_MASK & 0x0FFFFFFF) == 0x00010F1DUL)
#define RMU_RSTCAUSE_PORST_XMASK         0x00000000UL /** 0000000000000000  < Power On Reset */
#define RMU_RSTCAUSE_BODAVDD_XMASK       0x00000001UL /** 0000000000000001  < AVDD BOD Reset */
#define RMU_RSTCAUSE_BODDVDD_XMASK       0x00000001UL /** 0000000000000001  < DVDD BOD Reset */
#define RMU_RSTCAUSE_BODREGRST_XMASK     0x00000001UL /** 0000000000000001  < Regulated Domain (DEC) BOD Reset */
#define RMU_RSTCAUSE_EXTRST_XMASK        0x00000001UL /** 0000000000000001  < External Pin Reset */
#define RMU_RSTCAUSE_LOCKUPRST_XMASK     0x0000001DUL /** 0000000000011101  < LOCKUP Reset */
#define RMU_RSTCAUSE_SYSREQRST_XMASK     0x0000001DUL /** 0000000000011101  < System Request Reset */
#define RMU_RSTCAUSE_WDOGRST_XMASK       0x0000001DUL /** 0000000000011101  < Watchdog Reset */
#define RMU_RSTCAUSE_EM4RST_XMASK        0x0000001DUL /** 0000000000011101  < EM4H/S Reset */
#define NUM_RSTCAUSES                               9

#else
#error "RMU_RSTCAUSE XMASKs are not defined for this family."
#endif

#if defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 )
/* Fix for errata EMU_E208 - Occasional Full Reset After Exiting EM4H */
#define ERRATA_FIX_EMU_E208_EN
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** Reset cause mask type. */
typedef struct
{
  /** Reset-cause 1 bits */
  uint32_t resetCauseMask;
  /** Reset-cause 0 and "don't care" bits */
  uint32_t resetCauseZeroXMask;
} RMU_ResetCauseMasks_Typedef;


/*******************************************************************************
 *******************************   TYPEDEFS   **********************************
 ******************************************************************************/

/** Reset cause mask table. */
static const RMU_ResetCauseMasks_Typedef  resetCauseMasks[NUM_RSTCAUSES] =
  {
    { RMU_RSTCAUSE_PORST,        RMU_RSTCAUSE_PORST_XMASK },
#if defined(RMU_RSTCAUSE_BODUNREGRST)
    { RMU_RSTCAUSE_BODUNREGRST,  RMU_RSTCAUSE_BODUNREGRST_XMASK },
#endif
#if defined(RMU_RSTCAUSE_BODREGRST)
    { RMU_RSTCAUSE_BODREGRST,    RMU_RSTCAUSE_BODREGRST_XMASK },
#endif
#if defined(RMU_RSTCAUSE_AVDDBOD)
    { RMU_RSTCAUSE_AVDDBOD,      RMU_RSTCAUSE_BODAVDD_XMASK },
#endif
#if defined(RMU_RSTCAUSE_DVDDBOD)
    { RMU_RSTCAUSE_DVDDBOD,      RMU_RSTCAUSE_BODDVDD_XMASK },
#endif
#if defined(RMU_RSTCAUSE_DECBOD)
    { RMU_RSTCAUSE_DECBOD,       RMU_RSTCAUSE_BODREGRST_XMASK },
#endif
    { RMU_RSTCAUSE_EXTRST,       RMU_RSTCAUSE_EXTRST_XMASK },
    { RMU_RSTCAUSE_WDOGRST,      RMU_RSTCAUSE_WDOGRST_XMASK },
    { RMU_RSTCAUSE_LOCKUPRST,    RMU_RSTCAUSE_LOCKUPRST_XMASK },
    { RMU_RSTCAUSE_SYSREQRST,    RMU_RSTCAUSE_SYSREQRST_XMASK },
#if defined(RMU_RSTCAUSE_EM4RST)
    { RMU_RSTCAUSE_EM4RST,       RMU_RSTCAUSE_EM4RST_XMASK },
#endif
#if defined(RMU_RSTCAUSE_EM4WURST)
    { RMU_RSTCAUSE_EM4WURST,     RMU_RSTCAUSE_EM4WURST_XMASK },
#endif
#if defined(RMU_RSTCAUSE_BODAVDD0)
    { RMU_RSTCAUSE_BODAVDD0,     RMU_RSTCAUSE_BODAVDD0_XMASK },
#endif
#if defined(RMU_RSTCAUSE_BODAVDD1)
    { RMU_RSTCAUSE_BODAVDD1,     RMU_RSTCAUSE_BODAVDD1_XMASK },
#endif
#if defined(BU_PRESENT)
    { RMU_RSTCAUSE_BUBODVDDDREG, RMU_RSTCAUSE_BUBODVDDDREG_XMASK },
    { RMU_RSTCAUSE_BUBODBUVIN,   RMU_RSTCAUSE_BUBODBUVIN_XMASK },
    { RMU_RSTCAUSE_BUBODUNREG,   RMU_RSTCAUSE_BUBODUNREG_XMASK },
    { RMU_RSTCAUSE_BUBODREG,     RMU_RSTCAUSE_BUBODREG_XMASK },
    { RMU_RSTCAUSE_BUMODERST,    RMU_RSTCAUSE_BUMODERST_XMASK },
#endif
  };


/*******************************************************************************
 ********************************     TEST     ********************************
 ******************************************************************************/
#if defined(EMLIB_REGRESSION_TEST)
/* Test variable that replaces the RSTCAUSE cause register when testing
   the RMU_ResetCauseGet function. */
extern uint32_t rstCause;
#endif

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Disable/enable reset for various peripherals and signal sources
 *
 * @param[in] reset Reset types to enable/disable
 *
 * @param[in] mode  Reset mode
 ******************************************************************************/
void RMU_ResetControl(RMU_Reset_TypeDef reset, RMU_ResetMode_TypeDef mode)
{
  /* Note that the RMU supports bit-band access, but not peripheral bit-field set/clear */
#if defined(_RMU_CTRL_PINRMODE_MASK)
  uint32_t val;
#endif
  uint32_t shift;

  shift = SL_CTZ((uint32_t)reset);
#if defined(_RMU_CTRL_PINRMODE_MASK)
  val = (uint32_t)mode << shift;
  RMU->CTRL = (RMU->CTRL & ~reset) | val;
#else
  BUS_RegBitWrite(&RMU->CTRL, (uint32_t)shift, mode ? 1 : 0);
#endif
}


/***************************************************************************//**
 * @brief
 *   Clear the reset cause register.
 *
 * @details
 *   This function clears all the reset cause bits of the RSTCAUSE register.
 *   The reset cause bits must be cleared by SW before a new reset occurs,
 *   otherwise reset causes may accumulate. See @ref RMU_ResetCauseGet().
 ******************************************************************************/
void RMU_ResetCauseClear(void)
{
  RMU->CMD = RMU_CMD_RCCLR;

#if defined(EMU_AUXCTRL_HRCCLR)
  {
    uint32_t locked;

    /* Clear some reset causes not cleared with RMU CMD register */
    /* (If EMU registers locked, they must be unlocked first) */
    locked = EMU->LOCK & EMU_LOCK_LOCKKEY_LOCKED;
    if (locked)
    {
      EMU_Unlock();
    }

    BUS_RegBitWrite(&(EMU->AUXCTRL), _EMU_AUXCTRL_HRCCLR_SHIFT, 1);
    BUS_RegBitWrite(&(EMU->AUXCTRL), _EMU_AUXCTRL_HRCCLR_SHIFT, 0);

    if (locked)
    {
      EMU_Lock();
    }
  }
#endif
}


/***************************************************************************//**
 * @brief
 *   Get the cause of the last reset.
 *
 * @details
 *   In order to be useful, the reset cause must be cleared by software before a new
 *   reset occurs, otherwise reset causes may accumulate. See @ref
 *   RMU_ResetCauseClear(). This function call will return the main cause for
 *   reset, which can be a bit mask (several causes), and clear away "noise".
 *
 * @return
 *   Reset cause mask. Please consult the reference manual for description
 *   of the reset cause mask.
 ******************************************************************************/
uint32_t RMU_ResetCauseGet(void)
{
#define LB_CLW0 	        (* ((volatile uint32_t *)(LOCKBITS_BASE) + 122))
#define LB_CLW0_PINRESETSOFT    (1 << 2)

#if !defined(EMLIB_REGRESSION_TEST)
  uint32_t rstCause = RMU->RSTCAUSE;
#endif
  uint32_t validRstCause = 0;
  uint32_t zeroXMask;
  uint32_t i;

  for (i = 0; i < NUM_RSTCAUSES; i++)
  {
    zeroXMask = resetCauseMasks[i].resetCauseZeroXMask;
#if defined( _SILICON_LABS_32B_SERIES_1 )
    /* Handle soft/hard pin reset */
    if (!(LB_CLW0 & LB_CLW0_PINRESETSOFT))
    {
      /* RSTCAUSE_EXTRST must be 0 if pin reset is configured as hard reset */
      switch (resetCauseMasks[i].resetCauseMask)
      {
        case RMU_RSTCAUSE_LOCKUPRST:
          /* Fallthrough */
        case RMU_RSTCAUSE_SYSREQRST:
          /* Fallthrough */
        case RMU_RSTCAUSE_WDOGRST:
          /* Fallthrough */
        case RMU_RSTCAUSE_EM4RST:
          zeroXMask |= RMU_RSTCAUSE_EXTRST;
          break;
      }
    }
#endif

#if defined( _EMU_EM4CTRL_MASK ) && defined( ERRATA_FIX_EMU_E208_EN )
    /* Ignore BOD flags impacted by EMU_E208 */
    if (*(volatile uint32_t *)(EMU_BASE + 0x88) & (0x1 << 8))
    {
      zeroXMask &= ~(RMU_RSTCAUSE_DECBOD
                     | RMU_RSTCAUSE_DVDDBOD
                     | RMU_RSTCAUSE_AVDDBOD);
    }
#endif

    /* Check reset cause requirements. Note that a bit is "don't care" if 0 in
       both resetCauseMask and resetCauseZeroXMask. */
    if ((rstCause & resetCauseMasks[i].resetCauseMask)
        && !(rstCause & zeroXMask))
    {
      /* Add this reset-cause to the mask of qualified reset-causes */
      validRstCause |= resetCauseMasks[i].resetCauseMask;
    }
  }
#if defined( _EMU_EM4CTRL_MASK ) && defined( ERRATA_FIX_EMU_E208_EN )
  /* Clear BOD flags impacted by EMU_E208 */
  if (validRstCause & RMU_RSTCAUSE_EM4RST)
  {
    validRstCause &= ~(RMU_RSTCAUSE_DECBOD
                      | RMU_RSTCAUSE_DVDDBOD
                      | RMU_RSTCAUSE_AVDDBOD);
  }
#endif
  return validRstCause;
}


/** @} (end addtogroup RMU) */
/** @} (end addtogroup emlib) */
#endif /* defined(RMU_COUNT) && (RMU_COUNT > 0) */

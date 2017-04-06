/***************************************************************************//**
 * @file em_chip.h
 * @brief Chip Initialization API
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

#ifndef EM_CHIP_H
#define EM_CHIP_H

#include "em_device.h"
#include "em_system.h"
#include "em_gpio.h"
#include "em_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CHIP
 * @brief Chip errata workarounds initialization API
 * @details
 *  API to initialize chip for errata workarounds.
 * @{
 ******************************************************************************/

/**************************************************************************//**
 * @brief
 *   Chip initialization routine for revision errata workarounds. This function
 *   must be called immediately in main().
 *
 * @note
 *   This function must be called immediately in main().
 *
 * This init function will configure the device to a state where it is
 * as similar as later revisions as possible, to improve software compatibility
 * with newer parts. See the device specific errata for details.
 *****************************************************************************/
__STATIC_INLINE void CHIP_Init(void)
{
#if defined(_SILICON_LABS_32B_SERIES_0) && defined(_EFM32_GECKO_FAMILY)
  uint32_t                    rev;
  SYSTEM_ChipRevision_TypeDef chipRev;
  volatile uint32_t           *reg;

  rev = *(volatile uint32_t *)(0x0FE081FC);
  /* Engineering Sample calibration setup */
  if ((rev >> 24) == 0)
  {
    reg   = (volatile uint32_t *)0x400CA00C;
    *reg &= ~(0x70UL);
    /* DREG */
    reg   = (volatile uint32_t *)0x400C6020;
    *reg &= ~(0xE0000000UL);
    *reg |= ~(7UL << 25);
  }
  if ((rev >> 24) <= 3)
  {
    /* DREG */
    reg   = (volatile uint32_t *)0x400C6020;
    *reg &= ~(0x00001F80UL);
    /* Update CMU reset values */
    reg  = (volatile uint32_t *)0x400C8040;
    *reg = 0;
    reg  = (volatile uint32_t *)0x400C8044;
    *reg = 0;
    reg  = (volatile uint32_t *)0x400C8058;
    *reg = 0;
    reg  = (volatile uint32_t *)0x400C8060;
    *reg = 0;
    reg  = (volatile uint32_t *)0x400C8078;
    *reg = 0;
  }

  SYSTEM_ChipRevisionGet(&chipRev);
  if (chipRev.major == 0x01)
  {
    /* Rev A errata handling for EM2/3. Must enable DMA clock in order for EM2/3 */
    /* to work. This will be fixed in later chip revisions, so only do for rev A. */
    if (chipRev.minor == 00)
    {
      reg   = (volatile uint32_t *)0x400C8040;
      *reg |= 0x2;
    }

    /* Rev A+B errata handling for I2C when using EM2/3. USART0 clock must be enabled */
    /* after waking up from EM2/EM3 in order for I2C to work. This will be fixed in */
    /* later chip revisions, so only do for rev A+B. */
    if (chipRev.minor <= 0x01)
    {
      reg   = (volatile uint32_t *)0x400C8044;
      *reg |= 0x1;
    }
  }
  /* Ensure correct ADC/DAC calibration value */
  rev = *(volatile uint32_t *)0x0FE081F0;
  if (rev < 0x4C8ABA00)
  {
    uint32_t cal;

    /* Enable ADC/DAC clocks */
    reg   = (volatile uint32_t *)0x400C8044UL;
    *reg |= (1 << 14 | 1 << 11);

    /* Retrive calibration values */
    cal = ((*(volatile uint32_t *)(0x0FE081B4UL) & 0x00007F00UL) >>
           8) << 24;

    cal |= ((*(volatile uint32_t *)(0x0FE081B4UL) & 0x0000007FUL) >>
            0) << 16;

    cal |= ((*(volatile uint32_t *)(0x0FE081B4UL) & 0x00007F00UL) >>
            8) << 8;

    cal |= ((*(volatile uint32_t *)(0x0FE081B4UL) & 0x0000007FUL) >>
            0) << 0;

    /* ADC0->CAL = 1.25 reference */
    reg  = (volatile uint32_t *)0x40002034UL;
    *reg = cal;

    /* DAC0->CAL = 1.25 reference */
    reg  = (volatile uint32_t *)(0x4000402CUL);
    cal  = *(volatile uint32_t *)0x0FE081C8UL;
    *reg = cal;

    /* Turn off ADC/DAC clocks */
    reg   = (volatile uint32_t *)0x400C8044UL;
    *reg &= ~(1 << 14 | 1 << 11);
  }
#endif

#if defined(_SILICON_LABS_32B_SERIES_0) && defined(_EFM32_GIANT_FAMILY)

  /****************************/
  /* Fix for errata CMU_E113. */

  uint8_t                     prodRev;
  SYSTEM_ChipRevision_TypeDef chipRev;

  prodRev = SYSTEM_GetProdRev();
  SYSTEM_ChipRevisionGet(&chipRev);

  if ((prodRev >= 16) && (chipRev.minor >= 3))
  {
    /* This fixes an issue with the LFXO on high temperatures. */
    *(volatile uint32_t*)0x400C80C0 =
                      ( *(volatile uint32_t*)0x400C80C0 & ~(1 << 6) ) | (1 << 4);
  }
#endif

#if defined(_SILICON_LABS_32B_SERIES_0) && defined(_EFM32_HAPPY_FAMILY)

  uint8_t prodRev;
  prodRev = SYSTEM_GetProdRev();

  if (prodRev <= 129)
  {
    /* This fixes a mistaken internal connection between PC0 and PC4 */
    /* This disables an internal pulldown on PC4 */
    *(volatile uint32_t*)(0x400C6018) = (1 << 26) | (5 << 0);
    /* This disables an internal LDO test signal driving PC4 */
    *(volatile uint32_t*)(0x400C80E4) &= ~(1 << 24);
  }
#endif

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)

  /****************************
  * Fixes for errata GPIO_E201 (slewrate) and
  * HFXO high temperature oscillator startup robustness fix */

  uint32_t port;
  uint32_t clkEn;
  uint8_t prodRev;
  const uint32_t setVal   = (0x5 << _GPIO_P_CTRL_SLEWRATEALT_SHIFT)
                             | (0x5 << _GPIO_P_CTRL_SLEWRATE_SHIFT);
  const uint32_t resetVal = _GPIO_P_CTRL_RESETVALUE
                            & ~(_GPIO_P_CTRL_SLEWRATE_MASK
                                | _GPIO_P_CTRL_SLEWRATEALT_MASK);

  prodRev = SYSTEM_GetProdRev();
  SYSTEM_ChipRevision_TypeDef chipRev;
  SYSTEM_ChipRevisionGet(&chipRev);

  /* This errata is fixed in hardware from PRODREV 0x8F. */
  if (prodRev < 0x8F)
  {
    /* Fixes for errata GPIO_E201 (slewrate) */

    /* Save HFBUSCLK enable state and enable GPIO clock. */
    clkEn = CMU->HFBUSCLKEN0;
    CMU->HFBUSCLKEN0 = clkEn | CMU_HFBUSCLKEN0_GPIO;

    /* Update slewrate */
    for(port = 0; port <= GPIO_PORT_MAX; port++)
    {
      GPIO->P[port].CTRL = setVal | resetVal;
    }

    /* Restore HFBUSCLK enable state. */
    CMU->HFBUSCLKEN0 = clkEn;
  }

  /* This errata is fixed in hardware from PRODREV 0x90. */
  if (prodRev < 0x90)
  {
    /* HFXO high temperature oscillator startup robustness fix */
    CMU->HFXOSTARTUPCTRL =
          (CMU->HFXOSTARTUPCTRL & ~_CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_MASK)
          | (0x20 << _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_SHIFT);
  }

  if (chipRev.major == 0x01)
  {
    /* Fix for errata EMU_E210 - Potential Power-Down When Entering EM2 */
    *(volatile uint32_t *)(EMU_BASE + 0x164) |= 0x4;
  }
#endif

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_84)

  uint8_t prodRev = SYSTEM_GetProdRev();

  /* EM2 current fixes for early samples */
  if (prodRev == 0)
  {
    *(volatile uint32_t *)(EMU_BASE + 0x190)  = 0x0000ADE8UL;
    *(volatile uint32_t *)(EMU_BASE + 0x198) |= (0x1 << 2);
    *(volatile uint32_t *)(EMU_BASE + 0x190)  = 0x0;
  }
  if (prodRev < 2)
  {
    *(volatile uint32_t *)(EMU_BASE + 0x164) |= (0x1 << 13);
  }

  /* Set optimal LFRCOCTRL VREFUPDATE and enable duty cycling of vref */
  CMU->LFRCOCTRL = (CMU->LFRCOCTRL & ~_CMU_LFRCOCTRL_VREFUPDATE_MASK)
                    | CMU_LFRCOCTRL_VREFUPDATE_64CYCLES
                    | CMU_LFRCOCTRL_ENVREF;
#endif

#if defined(_EFR_DEVICE) && (_SILICON_LABS_GECKO_INTERNAL_SDID >= 84)
  MSC->CTRL |= 0x1 << 8;
#endif


}

/** @} (end addtogroup CHIP) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* EM_CHIP_H */

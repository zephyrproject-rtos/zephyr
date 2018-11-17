/***************************************************************************//**
 * @file em_system.c
 * @brief System Peripheral API
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

#include "em_system.h"
#include "em_assert.h"
#include <stddef.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup SYSTEM
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Get a chip major/minor revision.
 *
 * @param[out] rev
 *   A location to place the chip revision information.
 ******************************************************************************/
void SYSTEM_ChipRevisionGet(SYSTEM_ChipRevision_TypeDef *rev)
{
#if defined(_SYSCFG_CHIPREV_FAMILY_MASK)
  /* On series-2 (and higher) the revision info is in the SYSCFG->CHIPREV register. */
  uint32_t chiprev = SYSCFG->CHIPREV;
  rev->family = (chiprev & _SYSCFG_CHIPREV_FAMILY_MASK) >> _SYSCFG_CHIPREV_FAMILY_SHIFT;
  rev->major  = (chiprev & _SYSCFG_CHIPREV_MAJOR_MASK)  >> _SYSCFG_CHIPREV_MAJOR_SHIFT;
  rev->minor  = (chiprev & _SYSCFG_CHIPREV_MINOR_MASK)  >> _SYSCFG_CHIPREV_MINOR_SHIFT;
#else
  uint8_t tmp;

  EFM_ASSERT(rev);

  /* CHIP FAMILY bit [5:2] */
  tmp  = (uint8_t)(((ROMTABLE->PID1 & _ROMTABLE_PID1_FAMILYMSB_MASK)
                    >> _ROMTABLE_PID1_FAMILYMSB_SHIFT) << 2);
  /* CHIP FAMILY bit [1:0] */
  tmp |=  (uint8_t)((ROMTABLE->PID0 & _ROMTABLE_PID0_FAMILYLSB_MASK)
                    >> _ROMTABLE_PID0_FAMILYLSB_SHIFT);
  rev->family = tmp;

  /* CHIP MAJOR bit [3:0] */
  rev->major = (uint8_t)((ROMTABLE->PID0 & _ROMTABLE_PID0_REVMAJOR_MASK)
                         >> _ROMTABLE_PID0_REVMAJOR_SHIFT);

  /* CHIP MINOR bit [7:4] */
  tmp  = (uint8_t)(((ROMTABLE->PID2 & _ROMTABLE_PID2_REVMINORMSB_MASK)
                    >> _ROMTABLE_PID2_REVMINORMSB_SHIFT) << 4);
  /* CHIP MINOR bit [3:0] */
  tmp |= (uint8_t)((ROMTABLE->PID3 & _ROMTABLE_PID3_REVMINORLSB_MASK)
                   >> _ROMTABLE_PID3_REVMINORLSB_SHIFT);
  rev->minor = tmp;
#endif
}

/***************************************************************************//**
 * @brief
 *    Get a factory calibration value for a given peripheral register.
 *
 * @param[in] regAddress
 *    The peripheral calibration register address to get a calibration value for. If
 *    the calibration value is found, this register is updated with the
 *    calibration value.
 *
 * @return
 *    True if a calibration value exists, false otherwise.
 ******************************************************************************/
bool SYSTEM_GetCalibrationValue(volatile uint32_t *regAddress)
{
  SYSTEM_CalAddrVal_TypeDef * p, * end;
#if defined(MSC_FLASH_CHIPCONFIG_MEM_BASE)
  p   = (SYSTEM_CalAddrVal_TypeDef *)MSC_FLASH_CHIPCONFIG_MEM_BASE;
  end = (SYSTEM_CalAddrVal_TypeDef *)MSC_FLASH_CHIPCONFIG_MEM_END;
#else
  p   = (SYSTEM_CalAddrVal_TypeDef *)(DEVINFO_BASE & 0xFFFFF000U);
  end = (SYSTEM_CalAddrVal_TypeDef *)DEVINFO_BASE;
#endif

  for (; p < end; p++) {
    if (p->address == 0) {
      /* p->address == 0 marks the end of the table */
      return false;
    }
    if (p->address == (uint32_t)regAddress) {
      *regAddress = p->calValue;
      return true;
    }
  }
  /* Nothing found for regAddress. */
  return false;
}

/** @} (end addtogroup SYSTEM) */
/** @} (end addtogroup emlib) */

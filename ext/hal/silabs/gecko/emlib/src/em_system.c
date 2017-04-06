/***************************************************************************//**
 * @file em_system.c
 * @brief System Peripheral API
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
 *   Get chip major/minor revision.
 *
 * @param[out] rev
 *   Location to place chip revision info.
 ******************************************************************************/
void SYSTEM_ChipRevisionGet(SYSTEM_ChipRevision_TypeDef *rev)
{
  uint8_t tmp;

  EFM_ASSERT(rev);

  /* CHIP FAMILY bit [5:2] */
  tmp  = (((ROMTABLE->PID1 & _ROMTABLE_PID1_FAMILYMSB_MASK) >> _ROMTABLE_PID1_FAMILYMSB_SHIFT) << 2);
  /* CHIP FAMILY bit [1:0] */
  tmp |=  ((ROMTABLE->PID0 & _ROMTABLE_PID0_FAMILYLSB_MASK) >> _ROMTABLE_PID0_FAMILYLSB_SHIFT);
  rev->family = tmp;

  /* CHIP MAJOR bit [3:0] */
  rev->major = (ROMTABLE->PID0 & _ROMTABLE_PID0_REVMAJOR_MASK) >> _ROMTABLE_PID0_REVMAJOR_SHIFT;

  /* CHIP MINOR bit [7:4] */
  tmp  = (((ROMTABLE->PID2 & _ROMTABLE_PID2_REVMINORMSB_MASK) >> _ROMTABLE_PID2_REVMINORMSB_SHIFT) << 4);
  /* CHIP MINOR bit [3:0] */
  tmp |=  ((ROMTABLE->PID3 & _ROMTABLE_PID3_REVMINORLSB_MASK) >> _ROMTABLE_PID3_REVMINORLSB_SHIFT);
  rev->minor = tmp;
}


/***************************************************************************//**
 * @brief
 *    Get factory calibration value for a given peripheral register.
 *
 * @param[in] regAddress
 *    Peripheral calibration register address to get calibration value for. If
 *    a calibration value is found then this register is updated with the
 *    calibration value.
 *
 * @return
 *    True if a calibration value exists, false otherwise.
 ******************************************************************************/
bool SYSTEM_GetCalibrationValue(volatile uint32_t *regAddress)
{
  SYSTEM_CalAddrVal_TypeDef * p, * end;

  p   = (SYSTEM_CalAddrVal_TypeDef *)(DEVINFO_BASE & 0xFFFFF000);
  end = (SYSTEM_CalAddrVal_TypeDef *)DEVINFO_BASE;

  for ( ; p < end; p++)
  {
    if (p->address == 0xFFFFFFFF)
    {
      /* Found table terminator */
      return false;
    }
    if (p->address == (uint32_t)regAddress)
    {
      *regAddress = p->calValue;
      return true;
    }
  }
  /* Nothing found for regAddress */
  return false;
}

/** @} (end addtogroup SYSTEM) */
/** @} (end addtogroup emlib) */

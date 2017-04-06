/***************************************************************************//**
 * @file em_mpu.c
 * @brief Memory Protection Unit (MPU) Peripheral API
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

#include "em_mpu.h"
#if defined(__MPU_PRESENT) && (__MPU_PRESENT == 1)
#include "em_assert.h"


/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/


/***************************************************************************//**
 * @addtogroup MPU
 * @brief Memory Protection Unit (MPU) Peripheral API
 * @details
 *  This module contains functions to enable, disable and setup the MPU.
 *  The MPU is used to control access attributes and permissions in the
 *  memory map. The settings that can be controlled are:
 *
 *  @li Executable attribute.
 *  @li Cachable, bufferable and shareable attributes.
 *  @li Cache policy.
 *  @li Access permissions: Priviliged or User state, read or write access,
 *      and combinations of all these.
 *
 *  The MPU can be activated and deactivated with functions:
 *  @verbatim
 *  MPU_Enable(..);
 *  MPU_Disable();@endverbatim
 *  The MPU can control 8 memory regions with individual access control
 *  settings. Section attributes and permissions are set with:
 *  @verbatim
 *  MPU_ConfigureRegion(..);@endverbatim
 *  It is advisable to disable the MPU when altering region settings.
 *
 *
 * @{
 ******************************************************************************/


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/


/***************************************************************************//**
 * @brief
 *   Configure an MPU region.
 *
 * @details
 *   Writes to MPU RBAR and RASR registers.
 *   Refer to Cortex-M3 Reference Manual, MPU chapter for further details.
 *   To disable a region it is only required to set init->regionNo to the
 *   desired value and init->regionEnable = false.
 *
 * @param[in] init
 *   Pointer to a structure containing MPU region init information.
 ******************************************************************************/
void MPU_ConfigureRegion(const MPU_RegionInit_TypeDef *init)
{
  EFM_ASSERT(init->regionNo < ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >>
                                MPU_TYPE_DREGION_Pos));

  MPU->RNR = init->regionNo;

  if (init->regionEnable)
  {
    EFM_ASSERT(!(init->baseAddress & ~MPU_RBAR_ADDR_Msk));
    EFM_ASSERT(init->tex <= 0x7);

    MPU->RBAR = init->baseAddress;
    MPU->RASR = ((init->disableExec ? 1 : 0)   << MPU_RASR_XN_Pos)
                | (init->accessPermission      << MPU_RASR_AP_Pos)
                | (init->tex                   << MPU_RASR_TEX_Pos)
                | ((init->shareable   ? 1 : 0) << MPU_RASR_S_Pos)
                | ((init->cacheable   ? 1 : 0) << MPU_RASR_C_Pos)
                | ((init->bufferable  ? 1 : 0) << MPU_RASR_B_Pos)
                | (init->srd                   << MPU_RASR_SRD_Pos)
                | (init->size                  << MPU_RASR_SIZE_Pos)
                | (1                           << MPU_RASR_ENABLE_Pos);
  }
  else
  {
    MPU->RBAR = 0;
    MPU->RASR = 0;
  }
}


/** @} (end addtogroup CMU) */
/** @} (end addtogroup emlib) */
#endif /* defined(__MPU_PRESENT) && (__MPU_PRESENT == 1) */

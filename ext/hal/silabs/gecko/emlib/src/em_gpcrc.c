/***************************************************************************//**
 * @file
 * @brief General Purpose Cyclic Redundancy Check (GPCRC) API.
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

#include "em_gpcrc.h"
#include "em_assert.h"

#if defined(GPCRC_PRESENT) && (GPCRC_COUNT > 0)

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup GPCRC
 * @{
 ******************************************************************************/

/*******************************************************************************
 ***************************   GLOBAL FUNCTIONS   ******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize the General Purpose Cyclic Redundancy Check (GPCRC) module.
 *
 * @details
 *   Use this function to configure the operational parameters of the GPCRC
 *   such as the polynomial to use and how the input should be preprocessed
 *   before entering the CRC calculation.
 *
 * @note
 *   This function will not copy the init value to the data register in order
 *   to prepare for a new CRC calculation. This must be done by a call
 *   to @ref GPCRC_Start before each calculation, or by using the
 *   autoInit functionality.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure the GPCRC.
 ******************************************************************************/
void GPCRC_Init(GPCRC_TypeDef * gpcrc, const GPCRC_Init_TypeDef * init)
{
  uint32_t polySelect;

  if (init->crcPoly == 0x04C11DB7)
  {
    polySelect = GPCRC_CTRL_POLYSEL_CRC32;
  }
  else
  {
    // If not using the fixed CRC-32 polynomial then we must be using 16-bit
    EFM_ASSERT((init->crcPoly & 0xFFFF0000) == 0);
    polySelect = GPCRC_CTRL_POLYSEL_16;
  }

  gpcrc->CTRL = (((uint32_t)init->autoInit << _GPCRC_CTRL_AUTOINIT_SHIFT)
                | ((uint32_t)init->reverseByteOrder << _GPCRC_CTRL_BYTEREVERSE_SHIFT)
                | ((uint32_t)init->reverseBits << _GPCRC_CTRL_BITREVERSE_SHIFT)
                | ((uint32_t)init->enableByteMode << _GPCRC_CTRL_BYTEMODE_SHIFT)
                | polySelect
                | ((uint32_t)init->enable << _GPCRC_CTRL_EN_SHIFT));

  if (polySelect == GPCRC_CTRL_POLYSEL_16)
  {
    // Set CRC polynomial value
    uint32_t revPoly = __RBIT(init->crcPoly) >> 16;
    gpcrc->POLY = revPoly & _GPCRC_POLY_POLY_MASK;
  }

  // Load CRC initialization value to GPCRC_INIT
  gpcrc->INIT = init->initValue;
}

/***************************************************************************//**
 * @brief
 *   Reset GPCRC registers to the hardware reset state.
 *
 * @note
 *   The data registers are not reset by this function.
 *
 * @param[in] gpcrc
 *   Pointer to GPCRC peripheral register block.
 ******************************************************************************/
void GPCRC_Reset(GPCRC_TypeDef * gpcrc)
{
  gpcrc->CTRL = _GPCRC_CTRL_RESETVALUE;
  gpcrc->POLY = _GPCRC_POLY_RESETVALUE;
  gpcrc->INIT = _GPCRC_INIT_RESETVALUE;
}

/** @} (end addtogroup GPCRC) */
/** @} (end addtogroup emlib) */

#endif /* defined(GPCRC_COUNT) && (GPCRC_COUNT > 0) */

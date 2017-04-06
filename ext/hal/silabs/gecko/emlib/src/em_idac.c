/***************************************************************************//**
 * @file em_idac.c
 * @brief Current Digital to Analog Converter (IDAC) peripheral API
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

#include "em_idac.h"
#if defined(IDAC_COUNT) && (IDAC_COUNT > 0)
#include "em_cmu.h"
#include "em_assert.h"
#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup IDAC
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Fix for errata IDAC_E101 - IDAC output current degradation */
#if defined(_SILICON_LABS_32B_SERIES_0)  \
    && (defined(_EFM32_ZERO_FAMILY) || defined(_EFM32_HAPPY_FAMILY))
#define ERRATA_FIX_IDAC_E101_EN
#endif
/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/


/***************************************************************************//**
 * @brief
 *   Initialize IDAC.
 *
 * @details
 *   Initializes IDAC according to the initialization structure parameter, and
 *   sets the default calibration value stored in the DEVINFO structure.
 *
 * @note
 *   This function will disable the IDAC prior to configuration.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] init
 *   Pointer to IDAC initialization structure.
 ******************************************************************************/
void IDAC_Init(IDAC_TypeDef *idac, const IDAC_Init_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(IDAC_REF_VALID(idac));

  tmp = (uint32_t)(init->prsSel);

  tmp |= init->outMode;

  if (init->enable)
  {
    tmp |= IDAC_CTRL_EN;
  }
  if (init->prsEnable)
  {
#if defined(_IDAC_CTRL_OUTENPRS_MASK)
    tmp |= IDAC_CTRL_OUTENPRS;
#else
    tmp |= IDAC_CTRL_APORTOUTENPRS;
#endif
  }
  if (init->sinkEnable)
  {
    tmp |= IDAC_CTRL_CURSINK;
  }

  idac->CTRL = tmp;
}


/***************************************************************************//**
 * @brief
 *   Enable/disable IDAC.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] enable
 *   true to enable IDAC, false to disable.
 ******************************************************************************/
void IDAC_Enable(IDAC_TypeDef *idac, bool enable)
{
  EFM_ASSERT(IDAC_REF_VALID(idac));
  BUS_RegBitWrite(&idac->CTRL, _IDAC_CTRL_EN_SHIFT, enable);
}


/***************************************************************************//**
 * @brief
 *   Reset IDAC to same state as after a HW reset.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 ******************************************************************************/
void IDAC_Reset(IDAC_TypeDef *idac)
{
  EFM_ASSERT(IDAC_REF_VALID(idac));

#if defined(ERRATA_FIX_IDAC_E101_EN)
  /* Fix for errata IDAC_E101 - IDAC output current degradation:
     Instead of disabling it we will put it in it's lowest power state (50 nA)
     to avoid degradation over time */

  /* Make sure IDAC is enabled with disabled output */
  idac->CTRL = _IDAC_CTRL_RESETVALUE | IDAC_CTRL_EN;

  /* Set lowest current (50 nA) */
  idac->CURPROG = IDAC_CURPROG_RANGESEL_RANGE0 |
                  (0x0 << _IDAC_CURPROG_STEPSEL_SHIFT);

  /* Enable duty-cycling for all energy modes */
  idac->DUTYCONFIG = IDAC_DUTYCONFIG_DUTYCYCLEEN;
#else
  idac->CTRL       = _IDAC_CTRL_RESETVALUE;
  idac->CURPROG    = _IDAC_CURPROG_RESETVALUE;
  idac->DUTYCONFIG = _IDAC_DUTYCONFIG_RESETVALUE;
#endif
#if defined ( _IDAC_CAL_MASK )
  idac->CAL        = _IDAC_CAL_RESETVALUE;
#endif
}


/***************************************************************************//**
 * @brief
 *   Enable/disable Minimal Output Transition mode.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] enable
 *   true to enable Minimal Output Transition mode, false to disable.
 ******************************************************************************/
void IDAC_MinimalOutputTransitionMode(IDAC_TypeDef *idac, bool enable)
{
  EFM_ASSERT(IDAC_REF_VALID(idac));
  BUS_RegBitWrite(&idac->CTRL, _IDAC_CTRL_MINOUTTRANS_SHIFT, enable);
}


/***************************************************************************//**
 * @brief
 *   Set the current range of the IDAC output.
 *
 * @details
 *   This function sets the current range of the IDAC output. The function
 *   also updates the IDAC calibration register (IDAC_CAL) with the default
 *   calibration value from DEVINFO (factory calibration) corresponding to the
 *   specified range.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] range
 *   Current range value.
 ******************************************************************************/
void IDAC_RangeSet(IDAC_TypeDef *idac, const IDAC_Range_TypeDef range)
{
  uint32_t tmp;
#if defined( _IDAC_CURPROG_TUNING_MASK )
  uint32_t diCal0;
  uint32_t diCal1;
#endif

  EFM_ASSERT(IDAC_REF_VALID(idac));
  EFM_ASSERT(((uint32_t)range >> _IDAC_CURPROG_RANGESEL_SHIFT)
             <= (_IDAC_CURPROG_RANGESEL_MASK >> _IDAC_CURPROG_RANGESEL_SHIFT));

#if defined ( _IDAC_CAL_MASK )

  /* Load proper calibration data depending on selected range */
  switch ((IDAC_Range_TypeDef)range)
  {
    case idacCurrentRange0:
      idac->CAL = (DEVINFO->IDAC0CAL0 & _DEVINFO_IDAC0CAL0_RANGE0_MASK)
                  >> _DEVINFO_IDAC0CAL0_RANGE0_SHIFT;
      break;
    case idacCurrentRange1:
      idac->CAL = (DEVINFO->IDAC0CAL0 & _DEVINFO_IDAC0CAL0_RANGE1_MASK)
                  >> _DEVINFO_IDAC0CAL0_RANGE1_SHIFT;
      break;
    case idacCurrentRange2:
      idac->CAL = (DEVINFO->IDAC0CAL0 & _DEVINFO_IDAC0CAL0_RANGE2_MASK)
                  >> _DEVINFO_IDAC0CAL0_RANGE2_SHIFT;
      break;
    case idacCurrentRange3:
      idac->CAL = (DEVINFO->IDAC0CAL0 & _DEVINFO_IDAC0CAL0_RANGE3_MASK)
                  >> _DEVINFO_IDAC0CAL0_RANGE3_SHIFT;
      break;
  }

  tmp  = idac->CURPROG & ~_IDAC_CURPROG_RANGESEL_MASK;
  tmp |= (uint32_t)range;

#elif defined( _IDAC_CURPROG_TUNING_MASK )

  /* Load calibration data depending on selected range and sink/source mode */
  /* TUNING (calibration) field in CURPROG register. */
  EFM_ASSERT(idac == IDAC0);
  diCal0 = DEVINFO->IDAC0CAL0;
  diCal1 = DEVINFO->IDAC0CAL1;

  tmp = idac->CURPROG & ~(_IDAC_CURPROG_TUNING_MASK
                          | _IDAC_CURPROG_RANGESEL_MASK);
  if (idac->CTRL & IDAC_CTRL_CURSINK)
  {
    switch (range)
    {
      case idacCurrentRange0:
        tmp |= ((diCal1 & _DEVINFO_IDAC0CAL1_SINKRANGE0TUNING_MASK)
                >> _DEVINFO_IDAC0CAL1_SINKRANGE0TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange1:
        tmp |= ((diCal1 & _DEVINFO_IDAC0CAL1_SINKRANGE1TUNING_MASK)
                >> _DEVINFO_IDAC0CAL1_SINKRANGE1TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange2:
        tmp |= ((diCal1 & _DEVINFO_IDAC0CAL1_SINKRANGE2TUNING_MASK)
                >> _DEVINFO_IDAC0CAL1_SINKRANGE2TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange3:
        tmp |= ((diCal1 & _DEVINFO_IDAC0CAL1_SINKRANGE3TUNING_MASK)
                >> _DEVINFO_IDAC0CAL1_SINKRANGE3TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;
    }
  }
  else
  {
    switch (range)
    {
      case idacCurrentRange0:
        tmp |= ((diCal0 & _DEVINFO_IDAC0CAL0_SOURCERANGE0TUNING_MASK)
                >> _DEVINFO_IDAC0CAL0_SOURCERANGE0TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange1:
        tmp |= ((diCal0 & _DEVINFO_IDAC0CAL0_SOURCERANGE1TUNING_MASK)
                >> _DEVINFO_IDAC0CAL0_SOURCERANGE1TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange2:
        tmp |= ((diCal0 & _DEVINFO_IDAC0CAL0_SOURCERANGE2TUNING_MASK)
                >> _DEVINFO_IDAC0CAL0_SOURCERANGE2TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;

      case idacCurrentRange3:
        tmp |= ((diCal0 & _DEVINFO_IDAC0CAL0_SOURCERANGE3TUNING_MASK)
                >> _DEVINFO_IDAC0CAL0_SOURCERANGE3TUNING_SHIFT)
               << _IDAC_CURPROG_TUNING_SHIFT;
        break;
    }
  }

  tmp |= (uint32_t)range;

#else
#warning "IDAC calibration register definition unknown."
#endif

  idac->CURPROG = tmp;
}


/***************************************************************************//**
 * @brief
 *   Set the current step of the IDAC output.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] step
 *   Step value for IDAC output. Valid range is 0-31.
 ******************************************************************************/
void IDAC_StepSet(IDAC_TypeDef *idac, const uint32_t step)
{
  uint32_t tmp;

  EFM_ASSERT(IDAC_REF_VALID(idac));
  EFM_ASSERT(step <= (_IDAC_CURPROG_STEPSEL_MASK >> _IDAC_CURPROG_STEPSEL_SHIFT));

  tmp  = idac->CURPROG & ~_IDAC_CURPROG_STEPSEL_MASK;
  tmp |= step << _IDAC_CURPROG_STEPSEL_SHIFT;

  idac->CURPROG = tmp;
}


/***************************************************************************//**
 * @brief
 *   Enable/disable the IDAC OUT pin.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] enable
 *   true to enable the IDAC OUT pin, false to disable.
 ******************************************************************************/
void IDAC_OutEnable(IDAC_TypeDef *idac, bool enable)
{
  EFM_ASSERT(IDAC_REF_VALID(idac));
#if defined(_IDAC_CTRL_OUTEN_MASK)
  BUS_RegBitWrite(&idac->CTRL, _IDAC_CTRL_OUTEN_SHIFT, enable);
#else
  BUS_RegBitWrite(&idac->CTRL, _IDAC_CTRL_APORTOUTEN_SHIFT, enable);
#endif
}


/** @} (end addtogroup IDAC) */
/** @} (end addtogroup emlib) */

#endif /* defined(IDAC_COUNT) && (IDAC_COUNT > 0) */

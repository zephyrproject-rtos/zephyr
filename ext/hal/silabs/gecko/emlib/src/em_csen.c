/***************************************************************************//**
 * @file em_csen.c
 * @brief Capacitive Sense Module (CSEN) peripheral API
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

#include "em_csen.h"
#if defined( CSEN_COUNT ) && ( CSEN_COUNT > 0 )

#include "em_assert.h"
#include "em_cmu.h"
#include <stddef.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CSEN
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of CSEN register block pointer reference for assert statements. */
#define CSEN_REF_VALID(ref)   ((ref) == CSEN)

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set the DM integrator initial value.
 *
 * @details
 *   Sets the initial value of the integrator(s) for the Delta Modulation (DM)
 *   converter. The initial value for the ramp-down integrator has no effect 
 *   if low frequency attenuation was not selected by the mode initialization  
 *   function @ref CSEN_InitMode().
 *
 * @note
 *   Confirm CSEN is idle before calling this function.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] up
 *   Initial value for the ramp-up integrator.
 *
 * @param[in] down
 *  Initial value for the ramp-down integrator. Has no effect if low frequency 
 *  attenuation is not configured.
 ******************************************************************************/
void CSEN_DMBaselineSet(CSEN_TypeDef *csen, uint32_t up, uint32_t down)
{
  EFM_ASSERT(up < 0x10000);
  EFM_ASSERT(down < 0x10000);

  csen->DMBASELINE = (up << _CSEN_DMBASELINE_BASELINEUP_SHIFT)
                     | (down << _CSEN_DMBASELINE_BASELINEDN_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Initialize CSEN.
 *
 * @details
 *   Initializes common functionality for all measurement types. In addition,
 *   measurement mode must be configured, please refer to @ref CSEN_InitMode().
 *
 * @note
 *   This function will stop any ongoing conversion and disable CSEN.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] init
 *   Pointer to CSEN initialization structure.
 ******************************************************************************/
void CSEN_Init(CSEN_TypeDef *csen, const CSEN_Init_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(CSEN_REF_VALID(csen));
  EFM_ASSERT(init->warmUpCount < 4);

  /* Initialize CTRL. This will stop any conversion in progress. */
  tmp = CSEN_CTRL_STM_DEFAULT;

  if (init->cpAccuracyHi)
  {
    tmp |= CSEN_CTRL_CPACCURACY_HI;
  }

  if (init->localSense)
  {
    tmp |= _CSEN_CTRL_LOCALSENS_MASK;
  }

  if (init->keepWarm)
  {
    tmp |= CSEN_CTRL_WARMUPMODE_KEEPCSENWARM;
  }

  csen->CTRL = tmp;

  /* Initialize TIMCTRL. */
  csen->TIMCTRL = (init->warmUpCount << _CSEN_TIMCTRL_WARMUPCNT_SHIFT)
                  | (init->pcReload << _CSEN_TIMCTRL_PCTOP_SHIFT)
                  | (init->pcPrescale << _CSEN_TIMCTRL_PCPRESC_SHIFT);

  /* PRSSEL only has one field */
  csen->PRSSEL = init->prsSel << _CSEN_PRSSEL_PRSSEL_SHIFT;

  /* Set input selections for inputs 0 to 31 */
  csen->SCANINPUTSEL0 = (init->input0To7 << _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_SHIFT)
                        | (init->input8To15 << _CSEN_SCANINPUTSEL0_INPUT8TO15SEL_SHIFT)
                        | (init->input16To23 << _CSEN_SCANINPUTSEL0_INPUT16TO23SEL_SHIFT)
                        | (init->input24To31 << _CSEN_SCANINPUTSEL0_INPUT24TO31SEL_SHIFT);

  /* Set input selections for inputs 32 to 63 */
  csen->SCANINPUTSEL1 = (init->input32To39 << _CSEN_SCANINPUTSEL1_INPUT32TO39SEL_SHIFT)
                        | (init->input40To47 << _CSEN_SCANINPUTSEL1_INPUT40TO47SEL_SHIFT)
                        | (init->input48To55 << _CSEN_SCANINPUTSEL1_INPUT48TO55SEL_SHIFT)
                        | (init->input56To63 << _CSEN_SCANINPUTSEL1_INPUT56TO63SEL_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Initialize a CSEN measurement mode.
 *
 * @details
 *   Used to configure any type of measurement mode. After the measurement 
 *   has been configured, calling @ref CSEN_Enable() will enable CSEN and 
 *   allow it to start a conversion from the selected trigger source. To 
 *   manually start a conversion use @ref CSEN_Start(). To check if a 
 *   conversion is in progress use @ref CSEN_IsBusy(), or alternatively
 *   use the interrupt flags returned by @ref CSEN_IntGet() to detect when 
 *   a conversion is completed.
 *
 * @note
 *   This function will stop any ongoing conversion and disable CSEN.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] init
 *   Pointer to CSEN measurement mode initialization structure.
 ******************************************************************************/
void CSEN_InitMode(CSEN_TypeDef *csen, const CSEN_InitMode_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(CSEN_REF_VALID(csen));
  EFM_ASSERT(init->dmIterPerCycle < 0x10);
  EFM_ASSERT(init->dmCycles < 0x10);

  /* Initialize CTRL. This will stop any conversion in progress. 
   * These composite inputs set multiple fields. They do not need 
   * to be shifted. */
  tmp = ((uint32_t)init->sampleMode
        | (uint32_t)init->convSel
        | (uint32_t)init->cmpMode);

  tmp |= (init->trigSel << _CSEN_CTRL_STM_SHIFT)
         | (init->accMode << _CSEN_CTRL_ACU_SHIFT)
         | (init->sarRes << _CSEN_CTRL_SARCR_SHIFT);

  if (init->enableDma)
  {
    tmp |= CSEN_CTRL_DMAEN_ENABLE;
  }

  if (init->sumOnly)
  {
    tmp |= CSEN_CTRL_DRSF_ENABLE;
  }

  if (init->autoGnd)
  {
    tmp |= CSEN_CTRL_AUTOGND_ENABLE;
  }

  /* Preserve the fields that were initialized by CSEN_Init(). */
  tmp |= csen->CTRL & (_CSEN_CTRL_CPACCURACY_MASK
                      | _CSEN_CTRL_LOCALSENS_MASK
                      | _CSEN_CTRL_WARMUPMODE_MASK);

  csen->CTRL = tmp;

  /* EMACTRL only has one field */
  csen->EMACTRL = init->emaSample << _CSEN_EMACTRL_EMASAMPLE_SHIFT;

  /* CMPTHR only has one field */
  csen->CMPTHR = init->cmpThr << _CSEN_CMPTHR_CMPTHR_SHIFT;

  /* SINGLECTRL only has one field */
  csen->SINGLECTRL = init->singleSel << _CSEN_SINGLECTRL_SINGLESEL_SHIFT;

  /* Set all input enables */
  csen->SCANMASK0 = init->inputMask0;
  csen->SCANMASK1 = init->inputMask1;

  /* Initialize DMCFG. */
  tmp = (init->dmRes << _CSEN_DMCFG_CRMODE_SHIFT)
        | (init->dmCycles << _CSEN_DMCFG_DMCR_SHIFT)
        | (init->dmIterPerCycle << _CSEN_DMCFG_DMR_SHIFT)
        | (init->dmDelta << _CSEN_DMCFG_DMG_SHIFT);

  if (init->dmFixedDelta)
  {
    tmp |= CSEN_DMCFG_DMGRDIS;
  }

  csen->DMCFG = tmp;

  /* Initialize ANACTRL. */
  csen->ANACTRL = (init->resetPhase << _CSEN_ANACTRL_TRSTPROG_SHIFT)
                  | (init->driveSel << _CSEN_ANACTRL_IDACIREFS_SHIFT)
                  | (init->gainSel << _CSEN_ANACTRL_IREFPROG_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Reset CSEN to same state as after a HW reset.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 ******************************************************************************/
void CSEN_Reset(CSEN_TypeDef *csen)
{
  EFM_ASSERT(CSEN_REF_VALID(csen));

  /* Resetting CTRL stops any conversion in progress. */
  csen->CTRL          = _CSEN_CTRL_RESETVALUE;
  csen->TIMCTRL       = _CSEN_TIMCTRL_RESETVALUE;
  csen->PRSSEL        = _CSEN_PRSSEL_RESETVALUE;
  csen->DATA          = _CSEN_DATA_RESETVALUE;
  csen->SCANMASK0     = _CSEN_SCANMASK0_RESETVALUE;
  csen->SCANINPUTSEL0 = _CSEN_SCANINPUTSEL0_RESETVALUE;
  csen->SCANMASK1     = _CSEN_SCANMASK1_RESETVALUE;
  csen->SCANINPUTSEL1 = _CSEN_SCANINPUTSEL1_RESETVALUE;
  csen->CMPTHR        = _CSEN_CMPTHR_RESETVALUE;
  csen->EMA           = _CSEN_EMA_RESETVALUE;
  csen->EMACTRL       = _CSEN_EMACTRL_RESETVALUE;
  csen->SINGLECTRL    = _CSEN_SINGLECTRL_RESETVALUE;
  csen->DMBASELINE    = _CSEN_DMBASELINE_RESETVALUE;
  csen->DMCFG         = _CSEN_DMCFG_RESETVALUE;
  csen->ANACTRL       = _CSEN_ANACTRL_RESETVALUE;
  csen->IEN           = _CSEN_IEN_RESETVALUE;
  csen->IFC           = _CSEN_IF_MASK;
}


/** @} (end addtogroup CSEN) */
/** @} (end addtogroup emlib) */
#endif /* defined(CSEN_COUNT) && (CSEN_COUNT > 0) */

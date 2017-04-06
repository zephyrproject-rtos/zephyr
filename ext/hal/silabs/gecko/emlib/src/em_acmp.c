/***************************************************************************//**
 * @file em_acmp.c
 * @brief Analog Comparator (ACMP) Peripheral API
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


#include "em_acmp.h"
#if defined(ACMP_COUNT) && (ACMP_COUNT > 0)

#include <stdbool.h>
#include "em_bus.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup ACMP
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */


/** Validation of ACMP register block pointer reference
 *  for assert statements. */
#if (ACMP_COUNT == 1)
#define ACMP_REF_VALID(ref)    ((ref) == ACMP0)
#elif (ACMP_COUNT == 2)
#define ACMP_REF_VALID(ref)    (((ref) == ACMP0) || ((ref) == ACMP1))
#else
#error Undefined number of analog comparators (ACMP).
#endif

/** The maximum value that can be inserted in the route location register
 *  for the specific device. */
#if defined(_ACMP_ROUTE_LOCATION_LOC3)
#define _ACMP_ROUTE_LOCATION_MAX     _ACMP_ROUTE_LOCATION_LOC3
#elif defined(_ACMP_ROUTE_LOCATION_LOC2)
#define _ACMP_ROUTE_LOCATION_MAX     _ACMP_ROUTE_LOCATION_LOC2
#elif defined(_ACMP_ROUTE_LOCATION_LOC1)
#define _ACMP_ROUTE_LOCATION_MAX     _ACMP_ROUTE_LOCATION_LOC1
#elif defined(_ACMP_ROUTELOC0_OUTLOC_LOC31)
#define _ACMP_ROUTE_LOCATION_MAX     _ACMP_ROUTELOC0_OUTLOC_LOC31
#else
#error Undefined max route locations
#endif

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Sets up the ACMP for use in capacative sense applications.
 *
 * @details
 *   This function sets up the ACMP for use in capacacitve sense applications.
 *   To use the capacative sense functionality in the ACMP you need to use
 *   the PRS output of the ACMP module to count the number of oscillations
 *   in the capacative sense circuit (possibly using a TIMER).
 *
 * @note
 *   A basic example of capacative sensing can be found in the STK BSP
 *   (capsense demo).
 *
 * @param[in] acmp
 *   Pointer to ACMP peripheral register block.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure ACMP for capacative
 *   sensing operation.
 ******************************************************************************/
void ACMP_CapsenseInit(ACMP_TypeDef *acmp, const ACMP_CapsenseInit_TypeDef *init)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  /* Make sure that vddLevel is within bounds */
#if defined(_ACMP_INPUTSEL_VDDLEVEL_MASK)
  EFM_ASSERT(init->vddLevel < 64);
#else
  EFM_ASSERT(init->vddLevelLow < 64);
  EFM_ASSERT(init->vddLevelHigh < 64);
#endif

  /* Make sure biasprog is within bounds */
  EFM_ASSERT(init->biasProg <=
      (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));

  /* Set control register. No need to set interrupt modes */
  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
#if defined(_ACMP_CTRL_HALFBIAS_MASK)
               | (init->halfBias << _ACMP_CTRL_HALFBIAS_SHIFT)
#endif
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
#if defined(_ACMP_CTRL_WARMTIME_MASK)
               | (init->warmTime << _ACMP_CTRL_WARMTIME_SHIFT)
#endif
#if defined(_ACMP_CTRL_HYSTSEL_MASK)
               | (init->hysteresisLevel << _ACMP_CTRL_HYSTSEL_SHIFT)
#endif
#if defined(_ACMP_CTRL_ACCURACY_MASK)
               | ACMP_CTRL_ACCURACY_HIGH
#endif
               ;

#if defined(_ACMP_HYSTERESIS0_MASK)
  acmp->HYSTERESIS0 = (init->vddLevelHigh      << _ACMP_HYSTERESIS0_DIVVA_SHIFT)
                      | (init->hysteresisLevel_0 << _ACMP_HYSTERESIS0_HYST_SHIFT);
  acmp->HYSTERESIS1 = (init->vddLevelLow       << _ACMP_HYSTERESIS1_DIVVA_SHIFT)
                      | (init->hysteresisLevel_1 << _ACMP_HYSTERESIS1_HYST_SHIFT);
#endif

  /* Select capacative sensing mode by selecting a resistor and enabling it */
  acmp->INPUTSEL = (init->resistor << _ACMP_INPUTSEL_CSRESSEL_SHIFT)
                   | ACMP_INPUTSEL_CSRESEN
#if defined(_ACMP_INPUTSEL_LPREF_MASK)
                   | (init->lowPowerReferenceEnabled << _ACMP_INPUTSEL_LPREF_SHIFT)
#endif
#if defined(_ACMP_INPUTSEL_VDDLEVEL_MASK)
                   | (init->vddLevel << _ACMP_INPUTSEL_VDDLEVEL_SHIFT)
#endif
#if defined(ACMP_INPUTSEL_NEGSEL_CAPSENSE)
                   | ACMP_INPUTSEL_NEGSEL_CAPSENSE
#else
                   | ACMP_INPUTSEL_VASEL_VDD
                   | ACMP_INPUTSEL_NEGSEL_VADIV
#endif
                   ;

  /* Enable ACMP if requested. */
  BUS_RegBitWrite(&(acmp->CTRL), _ACMP_CTRL_EN_SHIFT, init->enable);
}

/***************************************************************************//**
 * @brief
 *   Sets the ACMP channel used for capacative sensing.
 *
 * @note
 *   A basic example of capacative sensing can be found in the STK BSP
 *   (capsense demo).
 *
 * @param[in] acmp
 *   Pointer to ACMP peripheral register block.
 *
 * @param[in] channel
 *   The ACMP channel to use for capacative sensing (Possel).
 ******************************************************************************/
void ACMP_CapsenseChannelSet(ACMP_TypeDef *acmp, ACMP_Channel_TypeDef channel)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_ACMP_INPUTSEL_POSSEL_CH7)
  /* Make sure that only external channels are used */
  EFM_ASSERT(channel <= _ACMP_INPUTSEL_POSSEL_CH7);
#elif defined(_ACMP_INPUTSEL_POSSEL_BUS4XCH31)
  /* Make sure that only external channels are used */
  EFM_ASSERT(channel <= _ACMP_INPUTSEL_POSSEL_BUS4XCH31);
#endif

  /* Set channel as positive channel in ACMP */
  BUS_RegMaskedWrite(&acmp->INPUTSEL, _ACMP_INPUTSEL_POSSEL_MASK,
      channel << _ACMP_INPUTSEL_POSSEL_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Disables the ACMP.
 *
 * @param[in] acmp
 *   Pointer to ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Disable(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  acmp->CTRL &= ~ACMP_CTRL_EN;
}

/***************************************************************************//**
 * @brief
 *   Enables the ACMP.
 *
 * @param[in] acmp
 *   Pointer to ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Enable(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  acmp->CTRL |= ACMP_CTRL_EN;
}

#if defined(_ACMP_EXTIFCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Select and enable an external input.
 *
 * @details
 *   This is used when an external module needs to take control of the ACMP
 *   POSSEL field in order to configure the APORT input for the ACMP. Modules
 *   like LESENSE use this to change the ACMP input during a scan sequence.
 *
 * @param[in] acmp
 *   Pointer to ACMP peripheral register block.
 *
 * @param[in] aport
 *   This parameter decides which APORT(s) the ACMP will use when it's being
 *   controlled by an external module.
 ******************************************************************************/
void ACMP_ExternalInputSelect(ACMP_TypeDef *acmp, ACMP_ExternalInput_Typedef aport)
{
  acmp->EXTIFCTRL = (aport << _ACMP_EXTIFCTRL_APORTSEL_SHIFT)
                    | ACMP_EXTIFCTRL_EN;
  while (!(acmp->STATUS & ACMP_STATUS_EXTIFACT))
    ;
}
#endif

/***************************************************************************//**
 * @brief
 *   Reset ACMP to same state as after a HW reset.
 *
 * @note
 *   The ROUTE register is NOT reset by this function, in order to allow for
 *   centralized setup of this feature.
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Reset(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  acmp->CTRL        = _ACMP_CTRL_RESETVALUE;
  acmp->INPUTSEL    = _ACMP_INPUTSEL_RESETVALUE;
#if defined(_ACMP_HYSTERESIS0_HYST_MASK)
  acmp->HYSTERESIS0 = _ACMP_HYSTERESIS0_RESETVALUE;
  acmp->HYSTERESIS1 = _ACMP_HYSTERESIS1_RESETVALUE;
#endif
  acmp->IEN         = _ACMP_IEN_RESETVALUE;
  acmp->IFC         = _ACMP_IF_MASK;
}

/***************************************************************************//**
 * @brief
 *   Sets up GPIO output from the ACMP.
 *
 * @note
 *    GPIO must be enabled in the CMU before this function call, i.e.
 * @verbatim CMU_ClockEnable(cmuClock_GPIO, true); @endverbatim
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 *
 * @param location
 *   The pin location to use. See the datasheet for location to pin mappings.
 *
 * @param enable
 *   Enable or disable pin output.
 *
 * @param invert
 *   Invert output.
 ******************************************************************************/
void ACMP_GPIOSetup(ACMP_TypeDef *acmp, uint32_t location, bool enable, bool invert)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  /* Sanity checking of location */
  EFM_ASSERT(location <= _ACMP_ROUTE_LOCATION_MAX);

  /* Set GPIO inversion */
  BUS_RegMaskedWrite(&acmp->CTRL, _ACMP_CTRL_GPIOINV_MASK,
      invert << _ACMP_CTRL_GPIOINV_SHIFT);

#if defined(_ACMP_ROUTE_MASK)
  acmp->ROUTE = (location << _ACMP_ROUTE_LOCATION_SHIFT)
                | (enable << _ACMP_ROUTE_ACMPPEN_SHIFT);
#endif
#if defined(_ACMP_ROUTELOC0_MASK)
  acmp->ROUTELOC0 = location << _ACMP_ROUTELOC0_OUTLOC_SHIFT;
  acmp->ROUTEPEN  = enable ? ACMP_ROUTEPEN_OUTPEN : 0;
#endif
}

/***************************************************************************//**
 * @brief
 *   Sets which channels should be used in ACMP comparisons.
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 *
 * @param negSel
 *   Channel to use on the negative input to the ACMP.
 *
 * @param posSel
 *   Channel to use on the positive input to the ACMP.
 ******************************************************************************/
void ACMP_ChannelSet(ACMP_TypeDef *acmp, ACMP_Channel_TypeDef negSel,
                     ACMP_Channel_TypeDef posSel)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  /* Make sure that posSel and negSel channel selectors are valid. */
#if defined(_ACMP_INPUTSEL_NEGSEL_DAC0CH1)
  EFM_ASSERT(negSel <= _ACMP_INPUTSEL_NEGSEL_DAC0CH1);
#elif defined(_ACMP_INPUTSEL_NEGSEL_CAPSENSE)
  EFM_ASSERT(negSel <= _ACMP_INPUTSEL_NEGSEL_CAPSENSE);
#endif

#if defined(_ACMP_INPUTSEL_POSSEL_CH7)
  EFM_ASSERT(posSel <= _ACMP_INPUTSEL_POSSEL_CH7);
#endif

  acmp->INPUTSEL = (acmp->INPUTSEL & ~(_ACMP_INPUTSEL_POSSEL_MASK
                                       | _ACMP_INPUTSEL_NEGSEL_MASK))
                   | (negSel << _ACMP_INPUTSEL_NEGSEL_SHIFT)
                   | (posSel << _ACMP_INPUTSEL_POSSEL_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Initialize ACMP.
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure ACMP for capacative
 *   sensing operation.
 ******************************************************************************/
void ACMP_Init(ACMP_TypeDef *acmp, const ACMP_Init_TypeDef *init)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  /* Make sure biasprog is within bounds */
  EFM_ASSERT(init->biasProg <=
      (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));

  /* Make sure the ACMP is disable since we might be changing the
   * ACMP power source */
  BUS_RegBitWrite(&acmp->CTRL, _ACMP_CTRL_EN_SHIFT, 0);

  /* Set control register. No need to set interrupt modes */
  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
#if defined(_ACMP_CTRL_HALFBIAS_MASK)
               | (init->halfBias << _ACMP_CTRL_HALFBIAS_SHIFT)
#endif
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
               | (init->interruptOnFallingEdge << _ACMP_CTRL_IFALL_SHIFT)
               | (init->interruptOnRisingEdge << _ACMP_CTRL_IRISE_SHIFT)
#if defined(_ACMP_CTRL_INPUTRANGE_MASK)
               | (init->inputRange << _ACMP_CTRL_INPUTRANGE_SHIFT)
#endif
#if defined(_ACMP_CTRL_ACCURACY_MASK)
               | (init->accuracy << _ACMP_CTRL_ACCURACY_SHIFT)
#endif
#if defined(_ACMP_CTRL_PWRSEL_MASK)
               | (init->powerSource << _ACMP_CTRL_PWRSEL_SHIFT)
#endif
#if defined(_ACMP_CTRL_WARMTIME_MASK)
               | (init->warmTime << _ACMP_CTRL_WARMTIME_SHIFT)
#endif
#if defined(_ACMP_CTRL_HYSTSEL_MASK)
               | (init->hysteresisLevel << _ACMP_CTRL_HYSTSEL_SHIFT)
#endif
               | (init->inactiveValue << _ACMP_CTRL_INACTVAL_SHIFT);

  acmp->INPUTSEL = (0)
#if defined(_ACMP_INPUTSEL_VLPSEL_MASK)
               | (init->vlpInput << _ACMP_INPUTSEL_VLPSEL_SHIFT)
#endif
#if defined(_ACMP_INPUTSEL_LPREF_MASK)
               | (init->lowPowerReferenceEnabled << _ACMP_INPUTSEL_LPREF_SHIFT)
#endif
#if defined(_ACMP_INPUTSEL_VDDLEVEL_MASK)
               | (init->vddLevel << _ACMP_INPUTSEL_VDDLEVEL_SHIFT)
#endif
               ;

  /* Enable ACMP if requested. */
  BUS_RegBitWrite(&(acmp->CTRL), _ACMP_CTRL_EN_SHIFT, init->enable);
}

#if defined(_ACMP_INPUTSEL_VASEL_MASK)
/***************************************************************************//**
 * @brief
 *   Setup the VA Source.
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 *
 * @param[in] vaconfig
 *   Pointer to the structure used to configure the VA source. This structure
 *   contains the input source as well as the 2 divider values.
 ******************************************************************************/
void ACMP_VASetup(ACMP_TypeDef *acmp, const ACMP_VAConfig_TypeDef *vaconfig)
{
  EFM_ASSERT(vaconfig->div0 < 64);
  EFM_ASSERT(vaconfig->div1 < 64);

  BUS_RegMaskedWrite(&acmp->INPUTSEL, _ACMP_INPUTSEL_VASEL_MASK,
      vaconfig->input << _ACMP_INPUTSEL_VASEL_SHIFT);
  BUS_RegMaskedWrite(&acmp->HYSTERESIS0, _ACMP_HYSTERESIS0_DIVVA_MASK,
      vaconfig->div0 << _ACMP_HYSTERESIS0_DIVVA_SHIFT);
  BUS_RegMaskedWrite(&acmp->HYSTERESIS1, _ACMP_HYSTERESIS1_DIVVA_MASK,
      vaconfig->div1 << _ACMP_HYSTERESIS1_DIVVA_SHIFT);
}
#endif

#if defined(_ACMP_INPUTSEL_VBSEL_MASK)
/***************************************************************************//**
 * @brief
 *   Setup the VB Source.
 *
 * @param[in] acmp
 *   Pointer to the ACMP peripheral register block.
 *
 * @param[in] vbconfig
 *   Pointer to the structure used to configure the VB source. This structure
 *   contains the input source as well as the 2 divider values.
 ******************************************************************************/
void ACMP_VBSetup(ACMP_TypeDef *acmp, const ACMP_VBConfig_TypeDef *vbconfig)
{
  EFM_ASSERT(vbconfig->div0 < 64);
  EFM_ASSERT(vbconfig->div1 < 64);

  BUS_RegMaskedWrite(&acmp->INPUTSEL, _ACMP_INPUTSEL_VBSEL_MASK,
      vbconfig->input << _ACMP_INPUTSEL_VBSEL_SHIFT);
  BUS_RegMaskedWrite(&acmp->HYSTERESIS0, _ACMP_HYSTERESIS0_DIVVB_MASK,
      vbconfig->div0 << _ACMP_HYSTERESIS0_DIVVB_SHIFT);
  BUS_RegMaskedWrite(&acmp->HYSTERESIS1, _ACMP_HYSTERESIS1_DIVVB_MASK,
      vbconfig->div1 << _ACMP_HYSTERESIS1_DIVVB_SHIFT);
}
#endif

/** @} (end addtogroup ACMP) */
/** @} (end addtogroup emlib) */
#endif /* defined(ACMP_COUNT) && (ACMP_COUNT > 0) */

/***************************************************************************//**
 * @file em_acmp.c
 * @brief Analog Comparator (ACMP) Peripheral API
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

#include "em_acmp.h"
#if defined(ACMP_COUNT) && (ACMP_COUNT > 0)

#include <stdbool.h>
#include "em_bus.h"
#include "em_assert.h"
#include "em_gpio.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup ACMP
 * @details
 *  This module contains functions to control the ACMP peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs.
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
#elif (ACMP_COUNT == 4)
#define ACMP_REF_VALID(ref)    (((ref) == ACMP0)    \
                                || ((ref) == ACMP1) \
                                || ((ref) == ACMP2) \
                                || ((ref) == ACMP3))
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
#elif defined(_ACMP_ROUTELOC0_OUTLOC_MASK)
#define _ACMP_ROUTE_LOCATION_MAX     _ACMP_ROUTELOC0_OUTLOC_MASK
#endif

/** Map ACMP reference to index of device. */
#define ACMP_DEVICE_ID(acmp) ( \
    (acmp) == ACMP0  ? 0       \
    : (acmp) == ACMP1  ? 1     \
    : 0)

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set up ACMP for use in capacitive sense applications.
 *
 * @details
 *   This function sets up ACMP for use in capacitive sense applications.
 *   To use the capacitive sense functionality in the ACMP, use
 *   the PRS output of the ACMP module to count the number of oscillations
 *   in the capacitive sense circuit (possibly using a TIMER).
 *
 * @note
 *   A basic example of capacitive sensing can be found in the STK BSP
 *   (capsense demo).
 *
 * @cond DOXYDOC_S2_DEVICE
 * @note
 *   A call to ACMP_CapsenseInit will enable and disable the ACMP peripheral,
 *   which can cause side effects if it was previously set up.
 * @endcond
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] init
 *   A pointer to the initialization structure used to configure ACMP for capacitive
 *   sensing operation.
 ******************************************************************************/
void ACMP_CapsenseInit(ACMP_TypeDef *acmp, const ACMP_CapsenseInit_TypeDef *init)
{
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_SILICON_LABS_32B_SERIES_2)
  EFM_ASSERT(init->vrefDiv < 64);
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CFG_BIASPROG_MASK >> _ACMP_CFG_BIASPROG_SHIFT));

  ACMP_Disable(acmp);
  acmp->CFG = (init->biasProg << _ACMP_CFG_BIASPROG_SHIFT)
              | (init->hysteresisLevel << _ACMP_CFG_HYST_SHIFT);
  acmp->CTRL = _ACMP_CTRL_RESETVALUE;
  ACMP_Enable(acmp);
  acmp->INPUTCTRL = (init->resistor << _ACMP_INPUTCTRL_CSRESSEL_SHIFT)
                    | (init->vrefDiv << _ACMP_INPUTCTRL_VREFDIV_SHIFT)
                    | (ACMP_INPUTCTRL_NEGSEL_CAPSENSE);
  if (!init->enable) {
    ACMP_Disable(acmp);
  }

#elif defined(_SILICON_LABS_32B_SERIES_1)
  EFM_ASSERT(init->vddLevelLow < 64);
  EFM_ASSERT(init->vddLevelHigh < 64);
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));

  /* Set the control register. No need to set interrupt modes. */
  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
               | ACMP_CTRL_ACCURACY_HIGH;
  acmp->HYSTERESIS0 = (init->vddLevelHigh      << _ACMP_HYSTERESIS0_DIVVA_SHIFT)
                      | (init->hysteresisLevel_0 << _ACMP_HYSTERESIS0_HYST_SHIFT);
  acmp->HYSTERESIS1 = (init->vddLevelLow       << _ACMP_HYSTERESIS1_DIVVA_SHIFT)
                      | (init->hysteresisLevel_1 << _ACMP_HYSTERESIS1_HYST_SHIFT);
  /* Select capacitive sensing mode by selecting a resistor and enabling it. */
  acmp->INPUTSEL = (init->resistor << _ACMP_INPUTSEL_CSRESSEL_SHIFT)
                   | ACMP_INPUTSEL_CSRESEN
                   | ACMP_INPUTSEL_VASEL_VDD
                   | ACMP_INPUTSEL_NEGSEL_VADIV;
  BUS_RegBitWrite(&acmp->CTRL, _ACMP_CTRL_EN_SHIFT, init->enable);

#elif defined(_SILICON_LABS_32B_SERIES_0)
  EFM_ASSERT(init->vddLevel < 64);
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));

  /* Set the control register. No need to set interrupt modes. */
  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
               | (init->halfBias << _ACMP_CTRL_HALFBIAS_SHIFT)
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
               | (init->warmTime << _ACMP_CTRL_WARMTIME_SHIFT)
               | (init->hysteresisLevel << _ACMP_CTRL_HYSTSEL_SHIFT);
  /* Select capacitive sensing mode by selecting a resistor and enabling it. */
  acmp->INPUTSEL = (init->resistor << _ACMP_INPUTSEL_CSRESSEL_SHIFT)
                   | ACMP_INPUTSEL_CSRESEN
                   | (init->lowPowerReferenceEnabled << _ACMP_INPUTSEL_LPREF_SHIFT)
                   | (init->vddLevel << _ACMP_INPUTSEL_VDDLEVEL_SHIFT)
                   | ACMP_INPUTSEL_NEGSEL_CAPSENSE;
  BUS_RegBitWrite(&acmp->CTRL, _ACMP_CTRL_EN_SHIFT, init->enable);
#endif
}

/***************************************************************************//**
 * @brief
 *   Set the ACMP channel used for capacitive sensing.
 *
 * @note
 *   A basic example of capacitive sensing can be found in the STK BSP
 *   (capsense demo).
 *
 * @cond DOXYDOC_S2_DEVICE
 * @note
 *   Can only be called when the peripheral is enabled.
 * @endcond
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] channel
 *   The ACMP channel to use for capacitive sensing (Possel).
 ******************************************************************************/
void ACMP_CapsenseChannelSet(ACMP_TypeDef *acmp, ACMP_Channel_TypeDef channel)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_ACMP_INPUTSEL_POSSEL_CH7)
  /* Make sure that only external channels are used. */
  EFM_ASSERT(channel <= _ACMP_INPUTSEL_POSSEL_CH7);
#elif defined(_ACMP_INPUTSEL_POSSEL_BUS4XCH31)
  /* Make sure that only external channels are used. */
  EFM_ASSERT(channel <= _ACMP_INPUTSEL_POSSEL_BUS4XCH31);
#elif defined(_ACMP_INPUTCTRL_POSSEL_PD15)
  EFM_ASSERT(channel != _ACMP_INPUTCTRL_NEGSEL_CAPSENSE);
  EFM_ASSERT(_ACMP_INPUTCTRL_POSSEL_PA0 <= channel);
  EFM_ASSERT(channel <= _ACMP_INPUTCTRL_POSSEL_PD15);
#endif

#if defined(_ACMP_INPUTCTRL_MASK)
  /* Make sure that the ACMP is enabled before changing INPUTCTRL. */
  EFM_ASSERT(acmp->EN & ACMP_EN_EN);

  /* Set channel as positive channel in ACMP */
  BUS_RegMaskedWrite(&acmp->INPUTCTRL, _ACMP_INPUTCTRL_POSSEL_MASK,
                     channel << _ACMP_INPUTCTRL_POSSEL_SHIFT);
#else
  /* Set channel as a positive channel in ACMP. */
  BUS_RegMaskedWrite(&acmp->INPUTSEL, _ACMP_INPUTSEL_POSSEL_MASK,
                     channel << _ACMP_INPUTSEL_POSSEL_SHIFT);
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable ACMP.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Disable(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_ACMP_EN_MASK)
  acmp->EN_CLR = ACMP_EN_EN;
#else
  acmp->CTRL &= ~ACMP_CTRL_EN;
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable ACMP.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Enable(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_ACMP_EN_MASK)
  acmp->EN_SET = ACMP_EN_EN;
#else
  acmp->CTRL |= ACMP_CTRL_EN;
#endif
}

#if defined(_ACMP_EXTIFCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Select and enable external input.
 *
 * @details
 *   This is used when an external module needs to take control of the ACMP
 *   POSSEL field to configure the APORT input for the ACMP. Modules,
 *   such as LESENSE, use this to change the ACMP input during a scan sequence.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] aport
 *   This parameter decides which APORT(s) the ACMP will use when it's
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
 *   Reset ACMP to the same state that it was in after a hardware reset.
 *
 * @note
 *   The GPIO ACMP ROUTE register is NOT reset by this function to allow for
 *   centralized setup of this feature.
 *
 * @note
 *   The peripheral may be enabled and disabled during reset.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 ******************************************************************************/
void ACMP_Reset(ACMP_TypeDef *acmp)
{
  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_SILICON_LABS_32B_SERIES_2)
  acmp->IEN         = _ACMP_IEN_RESETVALUE;
  ACMP_Enable(acmp);
  acmp->INPUTCTRL   = _ACMP_INPUTCTRL_RESETVALUE;
  ACMP_Disable(acmp);
  acmp->CFG         = _ACMP_CFG_RESETVALUE;
  acmp->CTRL        = _ACMP_CTRL_RESETVALUE;
  acmp->IF_CLR      = _ACMP_IF_MASK;
#else // Series 0 and Series 1 devices
  acmp->IEN         = _ACMP_IEN_RESETVALUE;
  acmp->CTRL        = _ACMP_CTRL_RESETVALUE;
  acmp->INPUTSEL    = _ACMP_INPUTSEL_RESETVALUE;
#if defined(_ACMP_HYSTERESIS0_HYST_MASK)
  acmp->HYSTERESIS0 = _ACMP_HYSTERESIS0_RESETVALUE;
  acmp->HYSTERESIS1 = _ACMP_HYSTERESIS1_RESETVALUE;
#endif
  acmp->IFC         = _ACMP_IF_MASK;
#endif
}

#if defined(_GPIO_ACMP_ROUTEEN_MASK)
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
 * @param port
 *   The GPIO port to use.
 *
 * @param pin
 *   The GPIO pin to use.
 *
 * @param enable
 *   Enable or disable pin output.
 *
 * @param invert
 *   Invert output.
 ******************************************************************************/
void ACMP_GPIOSetup(ACMP_TypeDef *acmp, GPIO_Port_TypeDef port,
                    unsigned int pin, bool enable, bool invert)
{
  int acmpIndex = ACMP_DEVICE_ID(acmp);

  /* Make sure the module exists on the selected chip */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

  /* Make sure that the port/pin combination is valid. */
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  /* Set GPIO inversion */
  acmp->CTRL = (acmp->CTRL & _ACMP_CTRL_NOTRDYVAL_MASK)
               | (invert << _ACMP_CTRL_GPIOINV_SHIFT);

  GPIO->ACMPROUTE[acmpIndex].ACMPOUTROUTE = (port << _GPIO_ACMP_ACMPOUTROUTE_PORT_SHIFT)
                                            | (pin << _GPIO_ACMP_ACMPOUTROUTE_PIN_SHIFT);
  GPIO->ACMPROUTE[acmpIndex].ROUTEEN  = enable ? GPIO_ACMP_ROUTEEN_ACMPOUTPEN : 0;
}
#else
/***************************************************************************//**
 * @brief
 *   Set up GPIO output from ACMP.
 *
 * @note
 *    GPIO must be enabled in the CMU before this function call, i.e.,
 * @verbatim CMU_ClockEnable(cmuClock_GPIO, true); @endverbatim
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param location
 *   The pin location to use. See the data sheet for location to pin mappings.
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
#endif /* defined(_GPIO_ACMP_ROUTEEN_MASK) */

/***************************************************************************//**
 * @brief
 *   Set which channels should be used in ACMP comparisons.
 *
 * @cond DOXYDOC_S2_DEVICE
 * @note
 *   Can only be called when the peripheral is enabled.
 *
 * @note
 *   If GPIO is used for both posSel and negSel, they cannot both use even
 *   or odd pins.
 * @endcond
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param negSel
 *   A channel to use on the negative input to the ACMP.
 *
 * @param posSel
 *   A channel to use on the positive input to the ACMP.
 ******************************************************************************/
void ACMP_ChannelSet(ACMP_TypeDef *acmp, ACMP_Channel_TypeDef negSel,
                     ACMP_Channel_TypeDef posSel)
{
  /* Make sure the module exists on the selected chip. */
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

  /* Make sure that posSel and negSel channel selectors are valid. */
#if defined(_ACMP_INPUTCTRL_POSSEL_PD15)
  EFM_ASSERT(negSel <= _ACMP_INPUTCTRL_POSSEL_PD15);
  EFM_ASSERT(posSel <= _ACMP_INPUTCTRL_POSSEL_PD15);
  EFM_ASSERT(posSel != _ACMP_INPUTCTRL_NEGSEL_CAPSENSE);

  /* Make sure that posSel and negSel channel selectors don't both
   * use odd or even pins. */
  EFM_ASSERT(!((posSel >= _ACMP_INPUTCTRL_POSSEL_PA0)
               && (negSel >= _ACMP_INPUTCTRL_NEGSEL_PA0)
               && (posSel % 2 == negSel % 2)));
#endif

#if defined(_ACMP_INPUTCTRL_MASK)
  /* Make sure that the ACMP is enabled before changing INPUTCTRL. */
  EFM_ASSERT(acmp->EN & ACMP_EN_EN);

  acmp->INPUTCTRL = (acmp->INPUTCTRL & ~(_ACMP_INPUTCTRL_POSSEL_MASK
                                         | _ACMP_INPUTCTRL_NEGSEL_MASK))
                    | (negSel << _ACMP_INPUTCTRL_NEGSEL_SHIFT)
                    | (posSel << _ACMP_INPUTCTRL_POSSEL_SHIFT);
#else
  acmp->INPUTSEL = (acmp->INPUTSEL & ~(_ACMP_INPUTSEL_POSSEL_MASK
                                       | _ACMP_INPUTSEL_NEGSEL_MASK))
                   | (negSel << _ACMP_INPUTSEL_NEGSEL_SHIFT)
                   | (posSel << _ACMP_INPUTSEL_POSSEL_SHIFT);
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize ACMP.
 *
 * @cond DOXYDOC_S2_DEVICE
 * @note
 *   A call to ACMP_Init can cause side effects since it can enable/disable
 *   the peripheral.
 * @endcond
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] init
 *   A pointer to the initialization structure used to configure ACMP.
 ******************************************************************************/
void ACMP_Init(ACMP_TypeDef *acmp, const ACMP_Init_TypeDef *init)
{
  /* Make sure the module exists on the selected chip. */
  EFM_ASSERT(ACMP_REF_VALID(acmp));

#if defined(_SILICON_LABS_32B_SERIES_2)
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CFG_BIASPROG_MASK >> _ACMP_CFG_BIASPROG_SHIFT));
  /* Make sure the ACMP is disabled since ACMP power source might be changed.*/
  ACMP_Disable(acmp);

  acmp->CFG = (init->biasProg << _ACMP_CFG_BIASPROG_SHIFT)
              | (init->inputRange << _ACMP_CFG_INPUTRANGE_SHIFT)
              | (init->accuracy << _ACMP_CFG_ACCURACY_SHIFT)
              | (init->hysteresisLevel << _ACMP_CFG_HYST_SHIFT);
  acmp->CTRL = init->inactiveValue << _ACMP_CTRL_NOTRDYVAL_SHIFT;
  ACMP_Enable(acmp);
  BUS_RegMaskedWrite(&acmp->INPUTCTRL, _ACMP_INPUTCTRL_VREFDIV_MASK,
                     init->vrefDiv << _ACMP_INPUTCTRL_VREFDIV_SHIFT);

#elif defined(_SILICON_LABS_32B_SERIES_1)
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));
  /* Make sure the ACMP is disabled since ACMP power source might be changed.*/
  ACMP_Disable(acmp);

  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
               | (init->interruptOnFallingEdge << _ACMP_CTRL_IFALL_SHIFT)
               | (init->interruptOnRisingEdge << _ACMP_CTRL_IRISE_SHIFT)
               | (init->inputRange << _ACMP_CTRL_INPUTRANGE_SHIFT)
               | (init->accuracy << _ACMP_CTRL_ACCURACY_SHIFT)
               | (init->powerSource << _ACMP_CTRL_PWRSEL_SHIFT)
               | (init->inactiveValue << _ACMP_CTRL_INACTVAL_SHIFT);
  acmp->INPUTSEL = init->vlpInput << _ACMP_INPUTSEL_VLPSEL_SHIFT;
  acmp->HYSTERESIS0 = init->hysteresisLevel_0;
  acmp->HYSTERESIS1 = init->hysteresisLevel_1;

#elif defined(_SILICON_LABS_32B_SERIES_0)
  EFM_ASSERT(init->biasProg
             <= (_ACMP_CTRL_BIASPROG_MASK >> _ACMP_CTRL_BIASPROG_SHIFT));
  /* Make sure the ACMP is disabled since ACMP power source might be changed.*/
  ACMP_Disable(acmp);

  acmp->CTRL = (init->fullBias << _ACMP_CTRL_FULLBIAS_SHIFT)
               | (init->halfBias << _ACMP_CTRL_HALFBIAS_SHIFT)
               | (init->biasProg << _ACMP_CTRL_BIASPROG_SHIFT)
               | (init->interruptOnFallingEdge << _ACMP_CTRL_IFALL_SHIFT)
               | (init->interruptOnRisingEdge << _ACMP_CTRL_IRISE_SHIFT)
               | (init->warmTime << _ACMP_CTRL_WARMTIME_SHIFT)
               | (init->hysteresisLevel << _ACMP_CTRL_HYSTSEL_SHIFT)
               | (init->inactiveValue << _ACMP_CTRL_INACTVAL_SHIFT);
  acmp->INPUTSEL = (init->lowPowerReferenceEnabled << _ACMP_INPUTSEL_LPREF_SHIFT)
                   | (init->vddLevel << _ACMP_INPUTSEL_VDDLEVEL_SHIFT);

#endif

  if (init->enable) {
    ACMP_Enable(acmp);
  } else {
    ACMP_Disable(acmp);
  }
}

#if defined(_ACMP_INPUTSEL_VASEL_MASK)
/***************************************************************************//**
 * @brief
 *   Set up the VA source.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] vaconfig
 *   A pointer to the structure used to configure the VA source. This structure
 *   contains the input source and the 2 divider values.
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
 *   Set up the VB Source.
 *
 * @param[in] acmp
 *   A pointer to the ACMP peripheral register block.
 *
 * @param[in] vbconfig
 *   A pointer to the structure used to configure the VB source. This structure
 *   contains the input source and the 2 divider values.
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

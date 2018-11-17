/***************************************************************************//**
 * @file em_adc.c
 * @brief Analog to Digital Converter (ADC) Peripheral API
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

#include "em_adc.h"
#if defined(ADC_COUNT) && (ADC_COUNT > 0)

#include "em_assert.h"
#include "em_cmu.h"
#include <stddef.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup ADC
 * @brief Analog to Digital Converter (ADC) Peripheral API
 * @details
 *  This module contains functions to control the ADC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The ADC is used to convert analog signals into a
 *  digital representation.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of ADC register block pointer reference for assert statements. */
#if (ADC_COUNT == 1)
#define ADC_REF_VALID(ref)    ((ref) == ADC0)
#elif (ADC_COUNT == 2)
#define ADC_REF_VALID(ref)    (((ref) == ADC0) || ((ref) == ADC1))
#endif

/** Maximum ADC clock */
#if defined(_SILICON_LABS_32B_SERIES_0)
#define ADC_MAX_CLOCK    13000000UL
#else
#define ADC_MAX_CLOCK    16000000UL
#endif

/** Minimum ADC clock */
#define ADC_MIN_CLOCK    32000UL

/** Helper defines for selecting ADC calibration and DEVINFO register fields. */
#if defined(_DEVINFO_ADC0CAL0_1V25_GAIN_MASK)
#define DEVINFO_ADC0_GAIN1V25_MASK _DEVINFO_ADC0CAL0_1V25_GAIN_MASK
#elif defined(_DEVINFO_ADC0CAL0_GAIN1V25_MASK)
#define DEVINFO_ADC0_GAIN1V25_MASK _DEVINFO_ADC0CAL0_GAIN1V25_MASK
#endif

#if defined(_DEVINFO_ADC0CAL0_1V25_GAIN_SHIFT)
#define DEVINFO_ADC0_GAIN1V25_SHIFT _DEVINFO_ADC0CAL0_1V25_GAIN_SHIFT
#elif defined(_DEVINFO_ADC0CAL0_GAIN1V25_SHIFT)
#define DEVINFO_ADC0_GAIN1V25_SHIFT _DEVINFO_ADC0CAL0_GAIN1V25_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL0_1V25_OFFSET_MASK)
#define DEVINFO_ADC0_OFFSET1V25_MASK _DEVINFO_ADC0CAL0_1V25_OFFSET_MASK
#elif defined(_DEVINFO_ADC0CAL0_OFFSET1V25_MASK)
#define DEVINFO_ADC0_OFFSET1V25_MASK _DEVINFO_ADC0CAL0_OFFSET1V25_MASK
#endif

#if defined(_DEVINFO_ADC0CAL0_1V25_OFFSET_SHIFT)
#define DEVINFO_ADC0_OFFSET1V25_SHIFT _DEVINFO_ADC0CAL0_1V25_OFFSET_SHIFT
#elif defined(_DEVINFO_ADC0CAL0_OFFSET1V25_SHIFT)
#define DEVINFO_ADC0_OFFSET1V25_SHIFT _DEVINFO_ADC0CAL0_OFFSET1V25_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL0_2V5_GAIN_MASK)
#define DEVINFO_ADC0_GAIN2V5_MASK _DEVINFO_ADC0CAL0_2V5_GAIN_MASK
#elif defined(_DEVINFO_ADC0CAL0_GAIN2V5_MASK)
#define DEVINFO_ADC0_GAIN2V5_MASK _DEVINFO_ADC0CAL0_GAIN2V5_MASK
#endif

#if defined(_DEVINFO_ADC0CAL0_2V5_GAIN_SHIFT)
#define DEVINFO_ADC0_GAIN2V5_SHIFT _DEVINFO_ADC0CAL0_2V5_GAIN_SHIFT
#elif defined(_DEVINFO_ADC0CAL0_GAIN2V5_SHIFT)
#define DEVINFO_ADC0_GAIN2V5_SHIFT _DEVINFO_ADC0CAL0_GAIN2V5_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL0_2V5_OFFSET_MASK)
#define DEVINFO_ADC0_OFFSET2V5_MASK _DEVINFO_ADC0CAL0_2V5_OFFSET_MASK
#elif defined(_DEVINFO_ADC0CAL0_OFFSET2V5_MASK)
#define DEVINFO_ADC0_OFFSET2V5_MASK _DEVINFO_ADC0CAL0_OFFSET2V5_MASK
#endif

#if defined(_DEVINFO_ADC0CAL0_2V5_OFFSET_SHIFT)
#define DEVINFO_ADC0_OFFSET2V5_SHIFT _DEVINFO_ADC0CAL0_2V5_OFFSET_SHIFT
#elif defined(_DEVINFO_ADC0CAL0_OFFSET2V5_SHIFT)
#define DEVINFO_ADC0_OFFSET2V5_SHIFT _DEVINFO_ADC0CAL0_OFFSET2V5_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL1_VDD_GAIN_MASK)
#define DEVINFO_ADC0_GAINVDD_MASK _DEVINFO_ADC0CAL1_VDD_GAIN_MASK
#elif defined(_DEVINFO_ADC0CAL1_GAINVDD_MASK)
#define DEVINFO_ADC0_GAINVDD_MASK _DEVINFO_ADC0CAL1_GAINVDD_MASK
#endif

#if defined(_DEVINFO_ADC0CAL1_VDD_GAIN_SHIFT)
#define DEVINFO_ADC0_GAINVDD_SHIFT _DEVINFO_ADC0CAL1_VDD_GAIN_SHIFT
#elif defined(_DEVINFO_ADC0CAL1_GAINVDD_SHIFT)
#define DEVINFO_ADC0_GAINVDD_SHIFT _DEVINFO_ADC0CAL1_GAINVDD_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL1_VDD_OFFSET_MASK)
#define DEVINFO_ADC0_OFFSETVDD_MASK _DEVINFO_ADC0CAL1_VDD_OFFSET_MASK
#elif defined(_DEVINFO_ADC0CAL1_OFFSETVDD_MASK)
#define DEVINFO_ADC0_OFFSETVDD_MASK _DEVINFO_ADC0CAL1_OFFSETVDD_MASK
#endif

#if defined(_DEVINFO_ADC0CAL1_VDD_OFFSET_SHIFT)
#define DEVINFO_ADC0_OFFSETVDD_SHIFT _DEVINFO_ADC0CAL1_VDD_OFFSET_SHIFT
#elif defined(_DEVINFO_ADC0CAL1_OFFSETVDD_SHIFT)
#define DEVINFO_ADC0_OFFSETVDD_SHIFT _DEVINFO_ADC0CAL1_OFFSETVDD_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL1_5VDIFF_GAIN_MASK)
#define DEVINFO_ADC0_GAIN5VDIFF_MASK _DEVINFO_ADC0CAL1_5VDIFF_GAIN_MASK
#elif defined(_DEVINFO_ADC0CAL1_GAIN5VDIFF_MASK)
#define DEVINFO_ADC0_GAIN5VDIFF_MASK _DEVINFO_ADC0CAL1_GAIN5VDIFF_MASK
#endif

#if defined(_DEVINFO_ADC0CAL1_5VDIFF_GAIN_SHIFT)
#define DEVINFO_ADC0_GAIN5VDIFF_SHIFT _DEVINFO_ADC0CAL1_5VDIFF_GAIN_SHIFT
#elif defined(_DEVINFO_ADC0CAL1_GAIN5VDIFF_SHIFT)
#define DEVINFO_ADC0_GAIN5VDIFF_SHIFT _DEVINFO_ADC0CAL1_GAIN5VDIFF_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL1_5VDIFF_OFFSET_MASK)
#define DEVINFO_ADC0_OFFSET5VDIFF_MASK _DEVINFO_ADC0CAL1_5VDIFF_OFFSET_MASK
#elif defined(_DEVINFO_ADC0CAL1_OFFSET5VDIFF_MASK)
#define DEVINFO_ADC0_OFFSET5VDIFF_MASK _DEVINFO_ADC0CAL1_OFFSET5VDIFF_MASK
#endif

#if defined(_DEVINFO_ADC0CAL1_5VDIFF_OFFSET_SHIFT)
#define DEVINFO_ADC0_OFFSET5VDIFF_SHIFT _DEVINFO_ADC0CAL1_5VDIFF_OFFSET_SHIFT
#elif defined(_DEVINFO_ADC0CAL1_OFFSET5VDIFF_SHIFT)
#define DEVINFO_ADC0_OFFSET5VDIFF_SHIFT _DEVINFO_ADC0CAL1_OFFSET5VDIFF_SHIFT
#endif

#if defined(_DEVINFO_ADC0CAL2_2XVDDVSS_OFFSET_MASK)
#define DEVINFO_ADC0_OFFSET2XVDD_MASK _DEVINFO_ADC0CAL2_2XVDDVSS_OFFSET_MASK
#elif defined(_DEVINFO_ADC0CAL2_OFFSET2XVDD_MASK)
#define DEVINFO_ADC0_OFFSET2XVDD_MASK _DEVINFO_ADC0CAL2_OFFSET2XVDD_MASK
#endif

#if defined(_DEVINFO_ADC0CAL2_2XVDDVSS_OFFSET_SHIFT)
#define DEVINFO_ADC0_OFFSET2XVDD_SHIFT _DEVINFO_ADC0CAL2_2XVDDVSS_OFFSET_SHIFT
#elif defined(_DEVINFO_ADC0CAL2_OFFSET2XVDD_SHIFT)
#define DEVINFO_ADC0_OFFSET2XVDD_SHIFT _DEVINFO_ADC0CAL2_OFFSET2XVDD_SHIFT
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
#define FIX_ADC_TEMP_BIAS_EN
#endif

/** @endcond */

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *   Load the ADC calibration register for a selected reference and conversion mode.
 *
 * @details
 *   During production, calibration values are stored in the device
 *   information page for internal references. Notice that, for external references,
 *   calibration values must be determined explicitly. This function
 *   will not modify the calibration register for external references.
 *
 * @param[in] adc
 *   A pointer to ADC peripheral register block.
 *
 * @param[in] ref
 *   A reference to load calibrated values for. No values are loaded for
 *   external references.
 *
 * @param[in] setScanCal
 *   Select scan mode (true) or single mode (false) calibration load.
 ******************************************************************************/
static void ADC_LoadDevinfoCal(ADC_TypeDef *adc,
                               ADC_Ref_TypeDef ref,
                               bool setScanCal)
{
  uint32_t calReg;
  uint32_t newCal;
  uint32_t mask;
  uint32_t shift;
  __IM uint32_t * diCalReg;

  if (setScanCal) {
    shift = _ADC_CAL_SCANOFFSET_SHIFT;
    mask  = ~(_ADC_CAL_SCANOFFSET_MASK
#if defined(_ADC_CAL_SCANOFFSETINV_MASK)
              | _ADC_CAL_SCANOFFSETINV_MASK
#endif
              | _ADC_CAL_SCANGAIN_MASK);
  } else {
    shift = _ADC_CAL_SINGLEOFFSET_SHIFT;
    mask  = ~(_ADC_CAL_SINGLEOFFSET_MASK
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
              | _ADC_CAL_SINGLEOFFSETINV_MASK
#endif
              | _ADC_CAL_SINGLEGAIN_MASK);
  }

  calReg = adc->CAL & mask;
  newCal = 0;

  if (adc == ADC0) {
    diCalReg = &DEVINFO->ADC0CAL0;
  }
#if defined(ADC1)
  else if (adc == ADC1) {
    diCalReg = &DEVINFO->ADC1CAL0;
  }
#endif
  else {
    return;
  }

  switch (ref) {
    case adcRef1V25:
      newCal |= ((diCalReg[0] & DEVINFO_ADC0_GAIN1V25_MASK)
                 >> DEVINFO_ADC0_GAIN1V25_SHIFT)
                << _ADC_CAL_SINGLEGAIN_SHIFT;
      newCal |= ((diCalReg[0] & DEVINFO_ADC0_OFFSET1V25_MASK)
                 >> DEVINFO_ADC0_OFFSET1V25_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
      newCal |= ((diCalReg[0] & _DEVINFO_ADC0CAL0_NEGSEOFFSET1V25_MASK)
                 >> _DEVINFO_ADC0CAL0_NEGSEOFFSET1V25_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
#endif
      break;

    case adcRef2V5:
      newCal |= ((diCalReg[0] & DEVINFO_ADC0_GAIN2V5_MASK)
                 >> DEVINFO_ADC0_GAIN2V5_SHIFT)
                << _ADC_CAL_SINGLEGAIN_SHIFT;
      newCal |= ((diCalReg[0] & DEVINFO_ADC0_OFFSET2V5_MASK)
                 >> DEVINFO_ADC0_OFFSET2V5_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
      newCal |= ((diCalReg[0] & _DEVINFO_ADC0CAL0_NEGSEOFFSET2V5_MASK)
                 >> _DEVINFO_ADC0CAL0_NEGSEOFFSET2V5_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
#endif
      break;

    case adcRefVDD:
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_GAINVDD_MASK)
                 >> DEVINFO_ADC0_GAINVDD_SHIFT)
                << _ADC_CAL_SINGLEGAIN_SHIFT;
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_OFFSETVDD_MASK)
                 >> DEVINFO_ADC0_OFFSETVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
      newCal |= ((diCalReg[1] & _DEVINFO_ADC0CAL1_NEGSEOFFSETVDD_MASK)
                 >> _DEVINFO_ADC0CAL1_NEGSEOFFSETVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
#endif
      break;

    case adcRef5VDIFF:
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_GAIN5VDIFF_MASK)
                 >> DEVINFO_ADC0_GAIN5VDIFF_SHIFT)
                << _ADC_CAL_SINGLEGAIN_SHIFT;
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_OFFSET5VDIFF_MASK)
                 >> DEVINFO_ADC0_OFFSET5VDIFF_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
      newCal |= ((diCalReg[1] & _DEVINFO_ADC0CAL1_NEGSEOFFSET5VDIFF_MASK)
                 >> _DEVINFO_ADC0CAL1_NEGSEOFFSET5VDIFF_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
#endif
      break;

    case adcRef2xVDD:
      /* There is no gain calibration for this reference */
      newCal |= ((diCalReg[2] & DEVINFO_ADC0_OFFSET2XVDD_MASK)
                 >> DEVINFO_ADC0_OFFSET2XVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
#if defined(_ADC_CAL_SINGLEOFFSETINV_MASK)
      newCal |= ((diCalReg[2] & _DEVINFO_ADC0CAL2_NEGSEOFFSET2XVDD_MASK)
                 >> _DEVINFO_ADC0CAL2_NEGSEOFFSET2XVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
#endif
      break;

#if defined(_ADC_SINGLECTRLX_VREFSEL_VDDXWATT)
    case adcRefVddxAtt:
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_GAINVDD_MASK)
                 >> DEVINFO_ADC0_GAINVDD_SHIFT)
                << _ADC_CAL_SINGLEGAIN_SHIFT;
      newCal |= ((diCalReg[1] & DEVINFO_ADC0_OFFSETVDD_MASK)
                 >> DEVINFO_ADC0_OFFSETVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSET_SHIFT;
      newCal |= ((diCalReg[1] & _DEVINFO_ADC0CAL1_NEGSEOFFSETVDD_MASK)
                 >> _DEVINFO_ADC0CAL1_NEGSEOFFSETVDD_SHIFT)
                << _ADC_CAL_SINGLEOFFSETINV_SHIFT;
      break;
#endif

    /* For external references, the calibration must be determined for the
       specific application and set by the user. Calibration data is also not
       available for the internal references adcRefVBGR, adcRefVEntropy, and
       adcRefVBGRlow. */
    default:
      newCal = 0;
      break;
  }

  adc->CAL = calReg | (newCal << shift);
}

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize ADC.
 *
 * @details
 *   Initializes common parts for both single conversion and scan sequence.
 *   In addition, single and/or scan control configuration must be done. See
 *   @ref ADC_InitSingle() and @ref ADC_InitScan() respectively.
 *   For ADC architectures with the ADCn->SCANINPUTSEL register, use
 *   @ref ADC_ScanSingleEndedInputAdd() to configure single-ended scan inputs or
 *   @ref ADC_ScanDifferentialInputAdd() to configure differential scan inputs.
 *   @ref ADC_ScanInputClear() is also provided for applications that need to update
 *   the input configuration.
 *
 * @note
 *   This function will stop any ongoing conversion.
 *
 * @param[in] adc
 *   A pointer to the ADC peripheral register block.
 *
 * @param[in] init
 *   A pointer to the ADC initialization structure.
 ******************************************************************************/
void ADC_Init(ADC_TypeDef *adc, const ADC_Init_TypeDef *init)
{
  uint32_t tmp;
  uint8_t presc = init->prescale;

  EFM_ASSERT(ADC_REF_VALID(adc));

  if (presc == 0U) {
    /* Assume maximum ADC clock for prescaler 0. */
    presc = ADC_PrescaleCalc(ADC_MAX_CLOCK, 0);
  } else {
    /* Check prescaler bounds against ADC_MAX_CLOCK and ADC_MIN_CLOCK. */
#if defined(_ADC_CTRL_ADCCLKMODE_MASK)
    if ((adc->CTRL & _ADC_CTRL_ADCCLKMODE_MASK) == ADC_CTRL_ADCCLKMODE_SYNC)
#endif
    {
      EFM_ASSERT(presc >= ADC_PrescaleCalc(ADC_MAX_CLOCK, 0));
      EFM_ASSERT(presc <= ADC_PrescaleCalc(ADC_MIN_CLOCK, 0));
    }
  }

  /* Make sure conversion is not in progress. */
  adc->CMD = ADC_CMD_SINGLESTOP | ADC_CMD_SCANSTOP;

  tmp = ((uint32_t)(init->ovsRateSel) << _ADC_CTRL_OVSRSEL_SHIFT)
        | (((uint32_t)(init->timebase) << _ADC_CTRL_TIMEBASE_SHIFT)
           & _ADC_CTRL_TIMEBASE_MASK)
        | (((uint32_t)(presc) << _ADC_CTRL_PRESC_SHIFT)
           & _ADC_CTRL_PRESC_MASK)
#if defined (_ADC_CTRL_LPFMODE_MASK)
        | ((uint32_t)(init->lpfMode) << _ADC_CTRL_LPFMODE_SHIFT)
#endif
        | ((uint32_t)(init->warmUpMode) << _ADC_CTRL_WARMUPMODE_SHIFT);

  if (init->tailgate) {
    tmp |= ADC_CTRL_TAILGATE;
  }
  adc->CTRL = tmp;

#if defined(_ADC_CTRL_ADCCLKMODE_MASK)
  /* Set ADC EM2 clock configuration. */
  BUS_RegMaskedWrite(&adc->CTRL,
                     _ADC_CTRL_ADCCLKMODE_MASK | _ADC_CTRL_ASYNCCLKEN_MASK,
                     (uint32_t)init->em2ClockConfig);

#if defined(_SILICON_LABS_32B_SERIES_1)
  /* In asynch clock mode assert that the ADC clock frequency is
     less or equal to 2/3 of the HFPER clock frequency. */
  if ((adc->CTRL & _ADC_CTRL_ADCCLKMODE_MASK) == ADC_CTRL_ADCCLKMODE_ASYNC) {
    CMU_Clock_TypeDef asyncClk = cmuClock_ADC0ASYNC;
    uint32_t adcClkFreq;
    uint32_t hfperClkFreq;
#if defined(_CMU_ADCCTRL_ADC1CLKSEL_MASK)
    if ( adc == ADC1 ) {
      asyncClk = cmuClock_ADC1ASYNC;
    }
#endif
    adcClkFreq = CMU_ClockFreqGet(asyncClk);
    hfperClkFreq = CMU_ClockFreqGet(cmuClock_HFPER);
    EFM_ASSERT(hfperClkFreq >= (adcClkFreq * 3) / 2);
  }
#endif /* #if defined(_SILICON_LABS_32B_SERIES_1) */
#endif /* #if defined(_ADC_CTRL_ADCCLKMODE_MASK) */

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
  /* A debugger can trigger the SCANUF interrupt on EFM32xG1 or EFR32xG1 */
  ADC_IntClear(adc, ADC_IFC_SCANUF);
#endif
}

#if defined(_ADC_SCANINPUTSEL_MASK)
/***************************************************************************//**
 * @brief
 *   Clear ADC scan input configuration.
 *
 * @param[in] scanInit
 *   Structure to hold the scan configuration and input configuration.
 ******************************************************************************/
void ADC_ScanInputClear(ADC_InitScan_TypeDef *scanInit)
{
  /* Clear the input configuration. */

  /* Select none. */
  scanInit->scanInputConfig.scanInputSel = ADC_SCANINPUTSEL_NONE;
  scanInit->scanInputConfig.scanInputEn = 0;

  /* Default alternative negative inputs. */
  scanInit->scanInputConfig.scanNegSel = _ADC_SCANNEGSEL_RESETVALUE;
}

/***************************************************************************//**
 * @brief
 *   Initialize ADC scan single-ended input configuration.
 *
 * @details
 *   Set a configuration for ADC scan conversion with single-ended inputs. The
 *   ADC_InitScan_TypeDef structure updated from this function will be passed to
 *   ADC_InitScan().
 *
 * @param[in] inputGroup
 *   ADC scan input group. See section 25.3.4 in the reference manual for
 *   more information.
 *
 * @param[in] singleEndedSel
 *   APORT select.
 *
 * @return
 *   Scan ID of selected ADC input. See section 25.3.4 in the reference manual for
 *   more information. Note that the returned integer represents the bit position
 *   in ADCn_SCANMASK set by this function. The accumulated mask is stored in
 *   scanInit->scanInputConfig->scanInputEn.
 ******************************************************************************/
uint32_t ADC_ScanSingleEndedInputAdd(ADC_InitScan_TypeDef *scanInit,
                                     ADC_ScanInputGroup_TypeDef inputGroup,
                                     ADC_PosSel_TypeDef singleEndedSel)
{
  uint32_t currentSel;
  uint32_t newSel;
  uint32_t scanId;

  scanInit->diff = false;

  /* Check for unsupported APORTs. */
  EFM_ASSERT((singleEndedSel <= adcPosSelAPORT0YCH0)
             || (singleEndedSel >= adcPosSelAPORT0YCH15));

  /* Check for an illegal group. */
  EFM_ASSERT((unsigned)inputGroup < 4U);

  /* Decode the input group select by shifting right by 3. */
  newSel = (unsigned)singleEndedSel >> 3;

  currentSel = (scanInit->scanInputConfig.scanInputSel
                >> ((unsigned)inputGroup * 8U)) & 0xFFU;

  /* If none selected. */
  if (currentSel == ADC_SCANINPUTSEL_GROUP_NONE) {
    scanInit->scanInputConfig.scanInputSel &=
      ~(0xFFU << ((unsigned)inputGroup * 8U));
    scanInit->scanInputConfig.scanInputSel |=
      newSel << ((unsigned)inputGroup * 8U);
  } else if (currentSel == newSel) {
    /* Ok, but do nothing.  */
  } else {
    /* Invalid channel range. A range is already selected for this group. */
    EFM_ASSERT(false);
  }

  /* Update and return scan input enable mask (SCANMASK). */
  scanId = ((unsigned)inputGroup * 8U) + ((unsigned)singleEndedSel & 0x7U);
  EFM_ASSERT(scanId < 32U);
  scanInit->scanInputConfig.scanInputEn |= 0x1UL << scanId;
  return scanId;
}

/***************************************************************************//**
 * @brief
 *   Initialize the ADC scan differential input configuration.
 *
 * @details
 *   Set a configuration for the ADC scan conversion with differential inputs. The
 *   ADC_InitScan_TypeDef structure updated by this function should be passed to
 *   ADC_InitScan().
 *
 * @param[in] scanInit
 *   Structure to hold the scan and input configuration.
 *
 * @param[in] inputGroup
 *   ADC scan input group. See section 25.3.4 in the reference manual for
 *   more information.
 *
 * @param[in] posSel
 *   APORT bus pair select. The negative terminal is implicitly selected by
 *   the positive terminal.
 *
 * @param[in] negInput
 *   ADC scan alternative negative input. Set to adcScanNegInputDefault to select
 *   a default negative input (implicit from posSel).
 *
 * @return
 *   Scan ID of the selected ADC input. See section 25.3.4 in the reference manual for
 *   more information. Note that the returned integer represents the bit position
 *   in ADCn_SCANMASK set by this function. The accumulated mask is stored in the
 *   scanInit->scanInputConfig->scanInputEn.
 ******************************************************************************/
uint32_t ADC_ScanDifferentialInputAdd(ADC_InitScan_TypeDef *scanInit,
                                      ADC_ScanInputGroup_TypeDef inputGroup,
                                      ADC_PosSel_TypeDef posSel,
                                      ADC_ScanNegInput_TypeDef negInput)
{
  uint32_t negInputRegMask = 0;
  uint32_t negInputRegShift = 0;
  uint32_t negInputRegVal = 0;
  uint32_t scanId;

  /* Perform a single-ended initialization, then update for differential scan. */
  scanId = ADC_ScanSingleEndedInputAdd(scanInit, inputGroup, posSel);

  /* Reset to differential mode. */
  scanInit->diff = true;

  /* Set negative ADC input unless the default is selected. */
  if (negInput != adcScanNegInputDefault) {
    if (scanId == 0U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT0NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT0NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 0U);
    } else if (scanId == 2U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT2NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT2NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 0U);
    } else if (scanId == 4U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT4NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT4NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 0U);
    } else if (scanId == 6U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT6NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT6NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 0U);
    } else if (scanId == 9U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT9NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT9NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 1U);
    } else if (scanId == 11U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT11NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT11NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 1U);
    } else if (scanId == 13U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT13NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT13NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 1U);
    } else if (scanId == 15U) {
      negInputRegMask  = _ADC_SCANNEGSEL_INPUT15NEGSEL_MASK;
      negInputRegShift = _ADC_SCANNEGSEL_INPUT15NEGSEL_SHIFT;
      EFM_ASSERT((unsigned)inputGroup == 1U);
    } else {
      /* The positive input does not have a negative input option (negInput is posInput + 1). */
      EFM_ASSERT(false);
    }

    /* Find ADC_SCANNEGSEL_CHxNSEL value for positive input 0, 2, 4, and 6. */
    if ((unsigned)inputGroup == 0U) {
      switch (negInput) {
        case adcScanNegInput1:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT1;
          break;

        case adcScanNegInput3:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT3;
          break;

        case adcScanNegInput5:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT5;
          break;

        case adcScanNegInput7:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT0NEGSEL_INPUT7;
          break;

        default:
          /* An invalid selection. Options are input 1, 3, 5 and 7. */
          EFM_ASSERT(false);
          break;
      }
    } else { /* inputGroup == 1 */
      /* Find ADC_SCANNEGSEL_CHxNSEL value for positive input 9, 11, 13, and 15. */
      switch (negInput) {
        case adcScanNegInput8:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT8;
          break;

        case adcScanNegInput10:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT10;
          break;

        case adcScanNegInput12:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT12;
          break;

        case adcScanNegInput14:
          negInputRegVal = _ADC_SCANNEGSEL_INPUT9NEGSEL_INPUT14;
          break;

        default:
          /* Invalid selection. Options are input 8, 10, 12, and 14. */
          EFM_ASSERT(false);
          break;
      }
    }

    /* Update configuration. */
    scanInit->scanInputConfig.scanNegSel &= ~negInputRegMask;
    scanInit->scanInputConfig.scanNegSel |= negInputRegVal << negInputRegShift;
  }
  return scanId;
}
#endif

/***************************************************************************//**
 * @brief
 *   Initialize the ADC scan sequence.
 *
 * @details
 *   See ADC_Start() for starting a scan sequence.
 *
 *   When selecting an external reference, the gain and offset calibration
 *   must be set explicitly (CAL register). For other references, the
 *   calibration is updated with values defined during manufacturing.
 *   For ADC architectures with the ADCn->SCANINPUTSEL register, use
 *   @ref ADC_ScanSingleEndedInputAdd() to configure single-ended scan inputs or
 *   @ref ADC_ScanDifferentialInputAdd() to configure differential scan inputs.
 *   @ref ADC_ScanInputClear() is also provided for applications that need to update
 *   the input configuration.
 *
 * @note
 *   This function will stop any ongoing scan sequence.
 *
 * @param[in] adc
 *   A pointer to the ADC peripheral register block.
 *
 * @param[in] init
 *   A pointer to the ADC initialization structure.
 ******************************************************************************/
void ADC_InitScan(ADC_TypeDef *adc, const ADC_InitScan_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(ADC_REF_VALID(adc));

  /* Make sure scan sequence is not in progress. */
  adc->CMD = ADC_CMD_SCANSTOP;

  /* Load calibration data for a selected reference. */
  ADC_LoadDevinfoCal(adc, init->reference, true);

  tmp = 0UL
#if defined (_ADC_SCANCTRL_PRSSEL_MASK)
        | ((uint32_t)init->prsSel << _ADC_SCANCTRL_PRSSEL_SHIFT)
#endif
        | ((uint32_t)init->acqTime << _ADC_SCANCTRL_AT_SHIFT)
#if defined (_ADC_SCANCTRL_INPUTMASK_MASK)
        | init->input
#endif
        | ((uint32_t)init->resolution << _ADC_SCANCTRL_RES_SHIFT);

  if (init->prsEnable) {
    tmp |= ADC_SCANCTRL_PRSEN;
  }

  if (init->leftAdjust) {
    tmp |= ADC_SCANCTRL_ADJ_LEFT;
  }

#if defined(_ADC_SCANCTRL_INPUTMASK_MASK)
  if (init->diff)
#elif defined(_ADC_SCANINPUTSEL_MASK)
  if (init->diff)
#endif
  {
    tmp |= ADC_SCANCTRL_DIFF;
  }

  if (init->rep) {
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
    /* Scan repeat mode does not work on EFM32JG1, EFM32PG1, or EFR32xG1x devices.
     * The errata is called ADC_E211 in the errata document. */
    EFM_ASSERT(false);
#endif
    tmp |= ADC_SCANCTRL_REP;
  }

  /* Set scan reference. Check if the reference configuration is extended to SCANCTRLX. */
#if defined (_ADC_SCANCTRLX_VREFSEL_MASK)
  if (((uint32_t)init->reference & ADC_CTRLX_VREFSEL_REG) != 0UL) {
    /* Select the extension register. */
    tmp |= ADC_SCANCTRL_REF_CONF;
  } else {
    tmp |= (uint32_t)init->reference << _ADC_SCANCTRL_REF_SHIFT;
  }
#else
  tmp |= init->reference << _ADC_SCANCTRL_REF_SHIFT;
#endif

#if defined(_ADC_SCANCTRL_INPUTMASK_MASK)
  tmp |= init->input;
#endif

  adc->SCANCTRL = tmp;

  /* Update SINGLECTRLX for reference select and PRS select. */
#if defined (_ADC_SCANCTRLX_MASK)
  tmp = adc->SCANCTRLX & ~(_ADC_SCANCTRLX_VREFSEL_MASK
                           | _ADC_SCANCTRLX_PRSSEL_MASK
                           | _ADC_SCANCTRLX_FIFOOFACT_MASK);
  if (((uint32_t)init->reference & ADC_CTRLX_VREFSEL_REG) != 0UL) {
    tmp |= ((uint32_t)init->reference & ~ADC_CTRLX_VREFSEL_REG) << _ADC_SCANCTRLX_VREFSEL_SHIFT;
  }

  tmp |= (uint32_t)init->prsSel << _ADC_SCANCTRLX_PRSSEL_SHIFT;

  if (init->fifoOverwrite) {
    tmp |= ADC_SCANCTRLX_FIFOOFACT_OVERWRITE;
  }

  adc->SCANCTRLX = tmp;
#endif

#if defined(_ADC_CTRL_SCANDMAWU_MASK)
  BUS_RegBitWrite(&adc->CTRL,
                  _ADC_CTRL_SCANDMAWU_SHIFT,
                  (uint32_t)init->scanDmaEm2Wu);
#endif

  /* Write the scan input configuration. */
#if defined(_ADC_SCANINPUTSEL_MASK)
  /* Check for valid scan input configuration. Use @ref ADC_ScanInputClear(),
     @ref ADC_ScanSingleEndedInputAdd(), and @ref ADC_ScanDifferentialInputAdd() to set
     the scan input configuration.  */
  EFM_ASSERT(init->scanInputConfig.scanInputSel != ADC_SCANINPUTSEL_NONE);
  adc->SCANINPUTSEL = init->scanInputConfig.scanInputSel;
  adc->SCANMASK     = init->scanInputConfig.scanInputEn;
  adc->SCANNEGSEL   = init->scanInputConfig.scanNegSel;
#endif

  /* Assert for any APORT bus conflicts programming errors. */
#if defined(_ADC_BUSCONFLICT_MASK)
  tmp = adc->BUSREQ;
  EFM_ASSERT(!(tmp & adc->BUSCONFLICT));
  EFM_ASSERT(!(adc->STATUS & _ADC_STATUS_PROGERR_MASK));
#endif
}

/***************************************************************************//**
 * @brief
 *   Initialize the single ADC sample conversion.
 *
 * @details
 *   See ADC_Start() for starting a single conversion.
 *
 *   When selecting an external reference, the gain and offset calibration
 *   must be set explicitly (CAL register). For other references, the
 *   calibration is updated with values defined during manufacturing.
 *
 * @note
 *   This function will stop any ongoing single conversion.
 *
 * @cond DOXYDOC_P2_DEVICE
 * @note
 *   This function will set the BIASPROG_GPBIASACC bit when selecting the
 *   internal temperature sensor and clear the bit otherwise. Any
 *   application that depends on the state of the BIASPROG_GPBIASACC bit should
 *   modify it after a call to this function.
 * @endcond
 *
 * @param[in] adc
 *   A pointer to the ADC peripheral register block.
 *
 * @param[in] init
 *   A pointer to the ADC initialization structure.
 ******************************************************************************/
void ADC_InitSingle(ADC_TypeDef *adc, const ADC_InitSingle_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(ADC_REF_VALID(adc));

  /* Make sure single conversion is not in progress. */
  adc->CMD = ADC_CMD_SINGLESTOP;

  /* Load calibration data for selected reference. */
  ADC_LoadDevinfoCal(adc, init->reference, false);

  tmp = 0UL
#if defined(_ADC_SINGLECTRL_PRSSEL_MASK)
        | ((uint32_t)init->prsSel << _ADC_SINGLECTRL_PRSSEL_SHIFT)
#endif
        | ((uint32_t)init->acqTime << _ADC_SINGLECTRL_AT_SHIFT)
#if defined(_ADC_SINGLECTRL_INPUTSEL_MASK)
        | (init->input << _ADC_SINGLECTRL_INPUTSEL_SHIFT)
#endif
#if defined(_ADC_SINGLECTRL_POSSEL_MASK)
        | ((uint32_t)init->posSel << _ADC_SINGLECTRL_POSSEL_SHIFT)
#endif
#if defined(_ADC_SINGLECTRL_NEGSEL_MASK)
        | ((uint32_t)init->negSel << _ADC_SINGLECTRL_NEGSEL_SHIFT)
#endif
        | ((uint32_t)(init->resolution) << _ADC_SINGLECTRL_RES_SHIFT);

  if (init->prsEnable) {
    tmp |= ADC_SINGLECTRL_PRSEN;
  }

  if (init->leftAdjust) {
    tmp |= ADC_SINGLECTRL_ADJ_LEFT;
  }

  if (init->diff) {
    tmp |= ADC_SINGLECTRL_DIFF;
  }

  if (init->rep) {
    tmp |= ADC_SINGLECTRL_REP;
  }

#if defined(_ADC_SINGLECTRL_POSSEL_TEMP)
  /* Force at least 8 cycle acquisition time when reading the internal temperature
   * sensor with 1.25 V reference */
  if ((init->posSel == adcPosSelTEMP)
      && (init->reference == adcRef1V25)
      && (init->acqTime < adcAcqTime8)) {
    tmp = (tmp & ~_ADC_SINGLECTRL_AT_MASK)
          | ((uint32_t)adcAcqTime8 << _ADC_SINGLECTRL_AT_SHIFT);
  }
#endif

  /* Set a single reference. Check if the reference configuration is extended to SINGLECTRLX. */
#if defined (_ADC_SINGLECTRLX_MASK)
  if (((uint32_t)init->reference & ADC_CTRLX_VREFSEL_REG) != 0UL) {
    /* Select the extension register. */
    tmp |= ADC_SINGLECTRL_REF_CONF;
  } else {
    tmp |= (uint32_t)init->reference << _ADC_SINGLECTRL_REF_SHIFT;
  }
#else
  tmp |= (uint32_t)init->reference << _ADC_SINGLECTRL_REF_SHIFT;
#endif
  adc->SINGLECTRL = tmp;

  /* Update SINGLECTRLX for reference select and PRS select. */
#if defined (_ADC_SINGLECTRLX_VREFSEL_MASK)
  tmp = adc->SINGLECTRLX & ~(_ADC_SINGLECTRLX_VREFSEL_MASK
                             | _ADC_SINGLECTRLX_PRSSEL_MASK
                             | _ADC_SINGLECTRLX_FIFOOFACT_MASK);
  if (((uint32_t)init->reference & ADC_CTRLX_VREFSEL_REG) != 0UL) {
    tmp |= ((uint32_t)init->reference & ~ADC_CTRLX_VREFSEL_REG)
           << _ADC_SINGLECTRLX_VREFSEL_SHIFT;
  }

  tmp |= (uint32_t)init->prsSel << _ADC_SINGLECTRLX_PRSSEL_SHIFT;

  if (init->fifoOverwrite) {
    tmp |= ADC_SINGLECTRLX_FIFOOFACT_OVERWRITE;
  }

  adc->SINGLECTRLX = tmp;
#endif

  /* Set DMA availability in EM2. */
#if defined(_ADC_CTRL_SINGLEDMAWU_MASK)
  BUS_RegBitWrite(&adc->CTRL,
                  _ADC_CTRL_SINGLEDMAWU_SHIFT,
                  (uint32_t)init->singleDmaEm2Wu);
#endif

#if defined(_ADC_BIASPROG_GPBIASACC_MASK) && defined(FIX_ADC_TEMP_BIAS_EN)
  if (init->posSel == adcPosSelTEMP) {
    /* ADC should always use low accuracy setting when reading the internal
     * temperature sensor on Series 1 devices. Using high
     * accuracy setting can introduce a glitch. */
    BUS_RegBitWrite(&adc->BIASPROG, _ADC_BIASPROG_GPBIASACC_SHIFT, 1);
  } else {
    BUS_RegBitWrite(&adc->BIASPROG, _ADC_BIASPROG_GPBIASACC_SHIFT, 0);
  }
#endif

  /* Assert for any APORT bus conflicts programming errors. */
#if defined(_ADC_BUSCONFLICT_MASK)
  tmp = adc->BUSREQ;
  EFM_ASSERT(!(tmp & adc->BUSCONFLICT));
  EFM_ASSERT(!(adc->STATUS & _ADC_STATUS_PROGERR_MASK));
#endif
}

#if defined(_ADC_SCANDATAX_MASK)
/***************************************************************************//**
 * @brief
 *   Get a scan result and scan select ID.
 *
 * @note
 *   Only use if scan data valid. This function does not check the DV flag.
 *   The return value is meant to be used as an index for the scan select ID.
 *
 * @param[in] adc
 *   A pointer to the ADC peripheral register block.
 *
 * @param[out] scanId
 *   A scan select ID of the first data in the scan FIFO.
 *
 * @return
 *   The first scan data in the scan FIFO.
 ******************************************************************************/
uint32_t ADC_DataIdScanGet(ADC_TypeDef *adc, uint32_t *scanId)
{
  uint32_t scanData;

  /* Pop data FIFO with scan ID */
  scanData = adc->SCANDATAX;
  *scanId = (scanData & _ADC_SCANDATAX_SCANINPUTID_MASK) >> _ADC_SCANDATAX_SCANINPUTID_SHIFT;
  return (scanData & _ADC_SCANDATAX_DATA_MASK) >> _ADC_SCANDATAX_DATA_SHIFT;
}
#endif

/***************************************************************************//**
 * @brief
 *   Calculate the prescaler value used to determine the ADC clock.
 *
 * @details
 *   The ADC clock is given by: HFPERCLK / (prescale + 1).
 *
 * @note
 *   The return value is clamped to the maximum prescaler value that the hardware supports.
 *
 * @param[in] adcFreq ADC frequency wanted. The frequency will automatically
 *   be adjusted to a valid range according to the reference manual.
 *
 * @param[in] hfperFreq Frequency in Hz of reference HFPER clock. Set to 0 to
 *   use currently defined HFPER clock setting.
 *
 * @return
 *   A prescaler value to use for ADC in order to achieve a clock value
 *   <= @p adcFreq.
 ******************************************************************************/
uint8_t ADC_PrescaleCalc(uint32_t adcFreq, uint32_t hfperFreq)
{
  uint32_t ret;

  /* Make sure that the selected ADC clock is within a valid range. */
  if (adcFreq > ADC_MAX_CLOCK) {
    adcFreq = ADC_MAX_CLOCK;
  } else if (adcFreq < ADC_MIN_CLOCK) {
    adcFreq = ADC_MIN_CLOCK;
  } else {
    /* Valid frequency. */
  }

  /* Use current HFPER frequency. */
  if (hfperFreq == 0UL) {
    hfperFreq = CMU_ClockFreqGet(cmuClock_HFPER);
  }

  ret = (hfperFreq + adcFreq - 1U) / adcFreq;
  if (ret > 0U) {
    ret--;
  }

  if (ret > (_ADC_CTRL_PRESC_MASK >> _ADC_CTRL_PRESC_SHIFT)) {
    ret = _ADC_CTRL_PRESC_MASK >> _ADC_CTRL_PRESC_SHIFT;
  }

  return (uint8_t)ret;
}

/***************************************************************************//**
 * @brief
 *   Reset ADC to a state that it was in after a hardware reset.
 *
 * @note
 *   The ROUTE register is NOT reset by this function to allow
 *   a centralized setup of this feature.
 *
 * @param[in] adc
 *   A pointer to ADC peripheral register block.
 ******************************************************************************/
void ADC_Reset(ADC_TypeDef *adc)
{
  /* Stop conversions, before resetting other registers. */
  adc->CMD          = ADC_CMD_SINGLESTOP | ADC_CMD_SCANSTOP;
  adc->SINGLECTRL   = _ADC_SINGLECTRL_RESETVALUE;
#if defined(_ADC_SINGLECTRLX_MASK)
  adc->SINGLECTRLX  = _ADC_SINGLECTRLX_RESETVALUE;
#endif
  adc->SCANCTRL     = _ADC_SCANCTRL_RESETVALUE;
#if defined(_ADC_SCANCTRLX_MASK)
  adc->SCANCTRLX    = _ADC_SCANCTRLX_RESETVALUE;
#endif
  adc->CTRL         = _ADC_CTRL_RESETVALUE;
  adc->IEN          = _ADC_IEN_RESETVALUE;
  adc->IFC          = _ADC_IFC_MASK;
  adc->BIASPROG     = _ADC_BIASPROG_RESETVALUE;
#if defined(_ADC_SCANMASK_MASK)
  adc->SCANMASK     = _ADC_SCANMASK_RESETVALUE;
#endif
#if defined(_ADC_SCANINPUTSEL_MASK)
  adc->SCANINPUTSEL = _ADC_SCANINPUTSEL_RESETVALUE;
#endif
#if defined(_ADC_SCANNEGSEL_MASK)
  adc->SCANNEGSEL   = _ADC_SCANNEGSEL_RESETVALUE;
#endif

  /* Clear data FIFOs. */
#if defined(_ADC_SINGLEFIFOCLEAR_MASK)
  adc->SINGLEFIFOCLEAR |= ADC_SINGLEFIFOCLEAR_SINGLEFIFOCLEAR;
  adc->SCANFIFOCLEAR   |= ADC_SCANFIFOCLEAR_SCANFIFOCLEAR;
#endif

  /* Load calibration values for the 1V25 internal reference. */
  ADC_LoadDevinfoCal(adc, adcRef1V25, false);
  ADC_LoadDevinfoCal(adc, adcRef1V25, true);

#if defined(_ADC_SCANINPUTSEL_MASK)
  /* Do not reset route register, setting should be done independently. */
#endif
}

/***************************************************************************//**
 * @brief
 *   Calculate a timebase value to get a timebase providing at least 1 us.
 *
 * @param[in] hfperFreq Frequency in Hz of the reference HFPER clock. Set to 0 to
 *   use currently defined HFPER clock setting.
 *
 * @return
 *   A timebase value to use for ADC to achieve at least 1 us.
 ******************************************************************************/
uint8_t ADC_TimebaseCalc(uint32_t hfperFreq)
{
  if (hfperFreq == 0UL) {
    hfperFreq = CMU_ClockFreqGet(cmuClock_HFPER);

    /* Make sure that the frequency is not 0 in below calculation. */
    if (hfperFreq == 0UL) {
      hfperFreq = 1UL;
    }
  }
#if defined(_SILICON_LABS_32B_SERIES_0) \
  && (defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY))
  /* Handle errata on Giant Gecko, maximum TIMEBASE is 5 bits wide or max 0x1F */
  /* cycles. This will give a warm up time of e.g., 0.645 us, not the       */
  /* required 1 us when operating at 48 MHz. One must also increase acqTime  */
  /* to compensate for the missing clock cycles, adding up to 1 us total.*/
  /* See reference manual for details. */
  if ( hfperFreq > 32000000UL ) {
    hfperFreq = 32000000UL;
  }
#endif
  /* Determine the number of HFPERCLK cycle >= 1 us. */
  hfperFreq += 999999UL;
  hfperFreq /= 1000000UL;

  /* Return timebase value (N+1 format). */
  return (uint8_t)(hfperFreq - 1UL);
}

/** @} (end addtogroup ADC) */
/** @} (end addtogroup emlib) */
#endif /* defined(ADC_COUNT) && (ADC_COUNT > 0) */

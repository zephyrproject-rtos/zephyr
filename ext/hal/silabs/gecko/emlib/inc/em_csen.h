/***************************************************************************//**
 * @file em_csen.h
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

#ifndef EM_CSEN_H
#define EM_CSEN_H

#include "em_device.h"
#if defined( CSEN_COUNT ) && ( CSEN_COUNT > 0 )

#include <stdbool.h>
#include "em_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup CSEN
 * @brief Capacitive Sense (CSEN) Peripheral API
 *
 * @details
 *  This module provides functions for controlling the capacitive sense 
 *  peripheral of Silicon Labs 32-bit MCUs and SoCs. The CSEN includes a 
 *  capacitance-to-digital circuit that measures capacitance on selected 
 *  inputs. Measurements are performed using either a successive approximation 
 *  register (SAR) or a delta modulator (DM) analog to digital converter.
 *
 *  The CSEN can be configured to measure capacitance on a single port pin 
 *  or to automatically measure multiple port pins in succession using scan 
 *  mode. Also several port pins can be shorted together to measure the 
 *  combined capacitance.
 *
 *  The CSEN includes an accumulator which can be configured to average 
 *  multiple conversions on the selected input. Additionally, an exponential 
 *  moving average (EMA) calculator is included to provide data smoothing. 
 *  A comparator is also included and can be used to terminate a continuous 
 *  conversion when the configured threshold condition is met.
 *
 *  The following example shows how to intialize and start a single 
 *  conversion on one input:
 *
 *  @include em_csen_single.c
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Comparator Mode. Selects the operation of the digital comparator. */
typedef enum
{
  /** Comparator is disabled. */
  csenCmpModeDisabled    = 0,

  /** Comparator trips when the result is greater than the threshold. */
  csenCmpModeGreater     = CSEN_CTRL_CMPEN | CSEN_CTRL_CMPPOL_GT,

  /** Comparator trips when the result is less or equal to the threshold. */
  csenCmpModeLessOrEqual = CSEN_CTRL_CMPEN | CSEN_CTRL_CMPPOL_LTE,

  /** Comparator trips when the EMA is within the threshold window. */
  csenCmpModeEMAWindow   = CSEN_CTRL_EMACMPEN,
} CSEN_CmpMode_TypeDef;


/** Converter Select. Determines the converter operational mode. */
typedef enum
{
  /** Successive Approximation (SAR) converter. */
  csenConvSelSAR     = CSEN_CTRL_CONVSEL_SAR,

  /** Successive Approximation (SAR) converter with low freq attenuation. */
  csenConvSelSARChop = CSEN_CTRL_CONVSEL_SAR | CSEN_CTRL_CHOPEN_ENABLE,

  /** Delta Modulation (DM) converter. */
  csenConvSelDM      = CSEN_CTRL_CONVSEL_DM,

  /** Delta Modulation (DM) converter with low frequency attenuation. */
  csenConvSelDMChop  = CSEN_CTRL_CONVSEL_DM | CSEN_CTRL_CHOPEN_ENABLE,
} CSEN_ConvSel_TypeDef;


/** Sample Mode. Determines how inputs are sampled for a conversion. */
typedef enum
{
  /** Convert multiple inputs shorted together and stop. */
  csenSampleModeBonded     = CSEN_CTRL_CM_SGL | CSEN_CTRL_MCEN_ENABLE,

  /** Convert one input and stop. */
  csenSampleModeSingle     = CSEN_CTRL_CM_SGL,

  /** Convert multiple inputs one at a time and stop. */
  csenSampleModeScan       = CSEN_CTRL_CM_SCAN,

  /** Continuously convert multiple inputs shorted together. */
  csenSampleModeContBonded = CSEN_CTRL_CM_CONTSGL | CSEN_CTRL_MCEN_ENABLE,

  /** Continuously convert one input. */
  csenSampleModeContSingle = CSEN_CTRL_CM_CONTSGL,

  /** Continuously convert multiple inputs one at a time. */
  csenSampleModeContScan   = CSEN_CTRL_CM_CONTSCAN,
} CSEN_SampleMode_TypeDef;


/** Start Trigger Select. */
typedef enum
{
  csenTrigSelPRS   = _CSEN_CTRL_STM_PRS,   /**< PRS system. */
  csenTrigSelTimer = _CSEN_CTRL_STM_TIMER, /**< CSEN PC timer. */
  csenTrigSelStart = _CSEN_CTRL_STM_START, /**< Start bit. */
} CSEN_TrigSel_TypeDef;


/** Accumulator Mode Select. */
typedef enum
{
  csenAccMode1  = _CSEN_CTRL_ACU_ACC1,  /**< Accumulate 1 sample. */
  csenAccMode2  = _CSEN_CTRL_ACU_ACC2,  /**< Accumulate 2 samples. */
  csenAccMode4  = _CSEN_CTRL_ACU_ACC4,  /**< Accumulate 4 samples. */
  csenAccMode8  = _CSEN_CTRL_ACU_ACC8,  /**< Accumulate 8 samples. */
  csenAccMode16 = _CSEN_CTRL_ACU_ACC16, /**< Accumulate 16 samples. */
  csenAccMode32 = _CSEN_CTRL_ACU_ACC32, /**< Accumulate 32 samples. */
  csenAccMode64 = _CSEN_CTRL_ACU_ACC64, /**< Accumulate 64 samples. */
} CSEN_AccMode_TypeDef;


/** Successive Approximation (SAR) Conversion Resolution. */
typedef enum
{
  csenSARRes10 = _CSEN_CTRL_SARCR_CLK10, /**< 10-bit resolution. */
  csenSARRes12 = _CSEN_CTRL_SARCR_CLK12, /**< 12-bit resolution. */
  csenSARRes14 = _CSEN_CTRL_SARCR_CLK14, /**< 14-bit resolution. */
  csenSARRes16 = _CSEN_CTRL_SARCR_CLK16, /**< 16-bit resolution. */
} CSEN_SARRes_TypeDef;


/** Delta Modulator (DM) Conversion Resolution. */
typedef enum
{
  csenDMRes10 = _CSEN_DMCFG_CRMODE_DM10, /**< 10-bit resolution. */
  csenDMRes12 = _CSEN_DMCFG_CRMODE_DM12, /**< 12-bit resolution. */
  csenDMRes14 = _CSEN_DMCFG_CRMODE_DM14, /**< 14-bit resolution. */
  csenDMRes16 = _CSEN_DMCFG_CRMODE_DM16, /**< 16-bit resolution. */
} CSEN_DMRes_TypeDef;


/** Period counter clock pre-scaler. See the reference manual for source clock 
 *  information. */
typedef enum
{
  csenPCPrescaleDiv1   = _CSEN_TIMCTRL_PCPRESC_DIV1,   /**< Divide by 1. */
  csenPCPrescaleDiv2   = _CSEN_TIMCTRL_PCPRESC_DIV2,   /**< Divide by 2. */
  csenPCPrescaleDiv4   = _CSEN_TIMCTRL_PCPRESC_DIV4,   /**< Divide by 4. */
  csenPCPrescaleDiv8   = _CSEN_TIMCTRL_PCPRESC_DIV8,   /**< Divide by 8. */
  csenPCPrescaleDiv16  = _CSEN_TIMCTRL_PCPRESC_DIV16,  /**< Divide by 16. */
  csenPCPrescaleDiv32  = _CSEN_TIMCTRL_PCPRESC_DIV32,  /**< Divide by 32. */
  csenPCPrescaleDiv64  = _CSEN_TIMCTRL_PCPRESC_DIV64,  /**< Divide by 64. */
  csenPCPrescaleDiv128 = _CSEN_TIMCTRL_PCPRESC_DIV128, /**< Divide by 128. */
} CSEN_PCPrescale_TypeDef;


/** Exponential Moving Average sample weight. */
typedef enum
{
  csenEMASampleW1  = _CSEN_EMACTRL_EMASAMPLE_W1,  /**< Weight 1. */
  csenEMASampleW2  = _CSEN_EMACTRL_EMASAMPLE_W2,  /**< Weight 2. */
  csenEMASampleW4  = _CSEN_EMACTRL_EMASAMPLE_W4,  /**< Weight 4. */
  csenEMASampleW8  = _CSEN_EMACTRL_EMASAMPLE_W8,  /**< Weight 8. */
  csenEMASampleW16 = _CSEN_EMACTRL_EMASAMPLE_W16, /**< Weight 16. */
  csenEMASampleW32 = _CSEN_EMACTRL_EMASAMPLE_W32, /**< Weight 32. */
  csenEMASampleW64 = _CSEN_EMACTRL_EMASAMPLE_W64, /**< Weight 64. */
} CSEN_EMASample_TypeDef;


/** Reset Phase Timing Select (units are microseconds). */
typedef enum
{
  csenResetPhaseSel0 = 0,  /**< Reset phase time = 0.75 usec. */
  csenResetPhaseSel1 = 1,  /**< Reset phase time = 1.00 usec. */
  csenResetPhaseSel2 = 2,  /**< Reset phase time = 1.20 usec. */
  csenResetPhaseSel3 = 3,  /**< Reset phase time = 1.50 usec. */
  csenResetPhaseSel4 = 4,  /**< Reset phase time = 2.00 usec. */
  csenResetPhaseSel5 = 5,  /**< Reset phase time = 3.00 usec. */
  csenResetPhaseSel6 = 6,  /**< Reset phase time = 6.00 usec. */
  csenResetPhaseSel7 = 7,  /**< Reset phase time = 12.0 usec. */
} CSEN_ResetPhaseSel_TypeDef;


/** Drive Strength Select. Scales the output current. */
typedef enum
{
  csenDriveSelFull = 0,  /**< Drive strength = fully on. */
  csenDriveSel1 = 1,     /**< Drive strength = 1/8 full scale. */
  csenDriveSel2 = 2,     /**< Drive strength = 1/4 full scale. */
  csenDriveSel3 = 3,     /**< Drive strength = 3/8 full scale. */
  csenDriveSel4 = 4,     /**< Drive strength = 1/2 full scale. */
  csenDriveSel5 = 5,     /**< Drive strength = 5/8 full scale. */
  csenDriveSel6 = 6,     /**< Drive strength = 3/4 full scale. */
  csenDriveSel7 = 7,     /**< Drive strength = 7/8 full scale. */
} CSEN_DriveSel_TypeDef;


/** Gain Select. See reference manual for information on each setting. */
typedef enum
{
  csenGainSel1X = 0,  /**< Gain = 1x. */
  csenGainSel2X = 1,  /**< Gain = 2x. */
  csenGainSel3X = 2,  /**< Gain = 3x. */
  csenGainSel4X = 3,  /**< Gain = 4x. */
  csenGainSel5X = 4,  /**< Gain = 5x. */
  csenGainSel6X = 5,  /**< Gain = 6x. */
  csenGainSel7X = 6,  /**< Gain = 7x. */
  csenGainSel8X = 7,  /**< Gain = 8x. */
} CSEN_GainSel_TypeDef;


/** Peripheral Reflex System signal used to trigger conversion. */
typedef enum
{
  csenPRSSELCh0  = _CSEN_PRSSEL_PRSSEL_PRSCH0,  /**< PRS channel 0. */
  csenPRSSELCh1  = _CSEN_PRSSEL_PRSSEL_PRSCH1,  /**< PRS channel 1. */
  csenPRSSELCh2  = _CSEN_PRSSEL_PRSSEL_PRSCH2,  /**< PRS channel 2. */
  csenPRSSELCh3  = _CSEN_PRSSEL_PRSSEL_PRSCH3,  /**< PRS channel 3. */
  csenPRSSELCh4  = _CSEN_PRSSEL_PRSSEL_PRSCH4,  /**< PRS channel 4. */
  csenPRSSELCh5  = _CSEN_PRSSEL_PRSSEL_PRSCH5,  /**< PRS channel 5. */
  csenPRSSELCh6  = _CSEN_PRSSEL_PRSSEL_PRSCH6,  /**< PRS channel 6. */
  csenPRSSELCh7  = _CSEN_PRSSEL_PRSSEL_PRSCH7,  /**< PRS channel 7. */
  csenPRSSELCh8  = _CSEN_PRSSEL_PRSSEL_PRSCH8,  /**< PRS channel 8. */
  csenPRSSELCh9  = _CSEN_PRSSEL_PRSSEL_PRSCH9,  /**< PRS channel 9. */
  csenPRSSELCh10 = _CSEN_PRSSEL_PRSSEL_PRSCH10, /**< PRS channel 10. */
  csenPRSSELCh11 = _CSEN_PRSSEL_PRSSEL_PRSCH11, /**< PRS channel 11. */
} CSEN_PRSSel_TypeDef;


/** APORT channel to CSEN input selection. */
typedef enum
{
  csenInputSelDefault        = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_DEFAULT,
  csenInputSelAPORT1CH0TO7   = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH0TO7,
  csenInputSelAPORT1CH8TO15  = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH8TO15,
  csenInputSelAPORT1CH16TO23 = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH16TO23,
  csenInputSelAPORT1CH24TO31 = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT1CH24TO31,
  csenInputSelAPORT3CH0TO7   = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH0TO7,
  csenInputSelAPORT3CH8TO15  = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH8TO15,
  csenInputSelAPORT3CH16TO23 = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH16TO23,
  csenInputSelAPORT3CH24TO31 = _CSEN_SCANINPUTSEL0_INPUT0TO7SEL_APORT3CH24TO31,
} CSEN_InputSel_TypeDef;


/** APORT channel to CSEN single input selection. */
typedef enum
{
  csenSingleSelDefault     = _CSEN_SINGLECTRL_SINGLESEL_DEFAULT,
  csenSingleSelAPORT1XCH0  = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH0,
  csenSingleSelAPORT1YCH1  = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH1,
  csenSingleSelAPORT1XCH2  = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH2,
  csenSingleSelAPORT1YCH3  = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH3,
  csenSingleSelAPORT1XCH4  = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH4,
  csenSingleSelAPORT1YCH5  = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH5,
  csenSingleSelAPORT1XCH6  = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH6,
  csenSingleSelAPORT1YCH7  = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH7,
  csenSingleSelAPORT1XCH8  = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH8,
  csenSingleSelAPORT1YCH9  = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH9,
  csenSingleSelAPORT1XCH10 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH10,
  csenSingleSelAPORT1YCH11 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH11,
  csenSingleSelAPORT1XCH12 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH12,
  csenSingleSelAPORT1YCH13 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH13,
  csenSingleSelAPORT1XCH14 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH14,
  csenSingleSelAPORT1YCH15 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH15,
  csenSingleSelAPORT1XCH16 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH16,
  csenSingleSelAPORT1YCH17 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH17,
  csenSingleSelAPORT1XCH18 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH18,
  csenSingleSelAPORT1YCH19 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH19,
  csenSingleSelAPORT1XCH20 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH20,
  csenSingleSelAPORT1YCH21 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH21,
  csenSingleSelAPORT1XCH22 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH22,
  csenSingleSelAPORT1YCH23 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH23,
  csenSingleSelAPORT1XCH24 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH24,
  csenSingleSelAPORT1YCH25 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH25,
  csenSingleSelAPORT1XCH26 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH26,
  csenSingleSelAPORT1YCH27 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH27,
  csenSingleSelAPORT1XCH28 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH28,
  csenSingleSelAPORT1YCH29 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH29,
  csenSingleSelAPORT1XCH30 = _CSEN_SINGLECTRL_SINGLESEL_APORT1XCH30,
  csenSingleSelAPORT1YCH31 = _CSEN_SINGLECTRL_SINGLESEL_APORT1YCH31,
  csenSingleSelAPORT3XCH0  = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH0,
  csenSingleSelAPORT3YCH1  = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH1,
  csenSingleSelAPORT3XCH2  = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH2,
  csenSingleSelAPORT3YCH3  = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH3,
  csenSingleSelAPORT3XCH4  = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH4,
  csenSingleSelAPORT3YCH5  = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH5,
  csenSingleSelAPORT3XCH6  = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH6,
  csenSingleSelAPORT3YCH7  = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH7,
  csenSingleSelAPORT3XCH8  = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH8,
  csenSingleSelAPORT3YCH9  = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH9,
  csenSingleSelAPORT3XCH10 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH10,
  csenSingleSelAPORT3YCH11 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH11,
  csenSingleSelAPORT3XCH12 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH12,
  csenSingleSelAPORT3YCH13 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH13,
  csenSingleSelAPORT3XCH14 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH14,
  csenSingleSelAPORT3YCH15 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH15,
  csenSingleSelAPORT3XCH16 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH16,
  csenSingleSelAPORT3YCH17 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH17,
  csenSingleSelAPORT3XCH18 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH18,
  csenSingleSelAPORT3YCH19 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH19,
  csenSingleSelAPORT3XCH20 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH20,
  csenSingleSelAPORT3YCH21 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH21,
  csenSingleSelAPORT3XCH22 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH22,
  csenSingleSelAPORT3YCH23 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH23,
  csenSingleSelAPORT3XCH24 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH24,
  csenSingleSelAPORT3YCH25 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH25,
  csenSingleSelAPORT3XCH26 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH26,
  csenSingleSelAPORT3YCH27 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH27,
  csenSingleSelAPORT3XCH28 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH28,
  csenSingleSelAPORT3YCH29 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH29,
  csenSingleSelAPORT3XCH30 = _CSEN_SINGLECTRL_SINGLESEL_APORT3XCH30,
  csenSingleSelAPORT3YCH31 = _CSEN_SINGLECTRL_SINGLESEL_APORT3YCH31,
} CSEN_SingleSel_TypeDef;


/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** CSEN init structure, common for all measurement modes. */
typedef struct
{
  /** Requests system charge pump high accuracy mode. */
  bool                          cpAccuracyHi;

  /** Disables external kelvin connection and senses capacitor locally. */
  bool                          localSense;

  /** Keeps the converter warm allowing continuous conversions. */
  bool                          keepWarm;

  /** Converter warmup time is warmUpCount + 3 converter clock cycles. */
  uint8_t                       warmUpCount;

  /** Period counter reload value. */
  uint8_t                       pcReload;

  /** Period counter pre-scaler. */
  CSEN_PCPrescale_TypeDef       pcPrescale;

  /** Peripheral reflex system trigger selection. */
  CSEN_PRSSel_TypeDef           prsSel;

  /** CSEN input to APORT channel mapping. */
  CSEN_InputSel_TypeDef         input0To7;
  CSEN_InputSel_TypeDef         input8To15;
  CSEN_InputSel_TypeDef         input16To23;
  CSEN_InputSel_TypeDef         input24To31;
  CSEN_InputSel_TypeDef         input32To39;
  CSEN_InputSel_TypeDef         input40To47;
  CSEN_InputSel_TypeDef         input48To55;
  CSEN_InputSel_TypeDef         input56To63;
} CSEN_Init_TypeDef;

#define CSEN_INIT_DEFAULT                                               \
{                                                                       \
  false,                        /* Charge pump low accuracy mode. */    \
  false,                        /* Use external kelvin connection. */   \
  false,                        /* Disable keep warm. */                \
  0,                            /* 0+3 cycle warmup time. */            \
  0,                            /* Period counter reload. */            \
  csenPCPrescaleDiv1,           /* Period counter prescale. */          \
  csenPRSSELCh0,                /* PRS channel 0. */                    \
  csenInputSelAPORT1CH0TO7,     /* input0To7   -> aport1ch0to7 */       \
  csenInputSelAPORT1CH8TO15,    /* input8To15  -> aport1ch8to15 */      \
  csenInputSelAPORT1CH16TO23,   /* input16To23 -> aport1ch16to23 */     \
  csenInputSelAPORT1CH24TO31,   /* input24To31 -> aport1ch24to31 */     \
  csenInputSelAPORT3CH0TO7,     /* input32To39 -> aport3ch0to7 */       \
  csenInputSelAPORT3CH8TO15,    /* input40To47 -> aport3ch8to15 */      \
  csenInputSelAPORT3CH16TO23,   /* input48To55 -> aport3ch16to23 */     \
  csenInputSelAPORT3CH24TO31,   /* input56To63 -> aport3ch24to31 */     \
}


/** Measurement mode init structure. */
typedef struct
{
  /** Selects the conversion sample mode. */
  CSEN_SampleMode_TypeDef       sampleMode;

  /** Selects the conversion trigger source. */
  CSEN_TrigSel_TypeDef          trigSel;

  /** Enables DMA operation. */
  bool                          enableDma;

  /** Disables dividing the accumulated result. */
  bool                          sumOnly;

  /** Selects the number of samples to accumulate per conversion. */
  CSEN_AccMode_TypeDef          accMode;

  /** Selects the Exponential Moving Average sample weighting. */
  CSEN_EMASample_TypeDef        emaSample;

  /** Enables the comparator and selects the comparison type. */
  CSEN_CmpMode_TypeDef          cmpMode;

  /** Comparator threshold value. Meaning depends on @p cmpMode. */
  uint16_t                      cmpThr;

  /** Selects an APORT channel for a single conversion. */
  CSEN_SingleSel_TypeDef        singleSel;

  /** 
   * Mask selects inputs 0 to 31. Effect depends on @p sampleMode. If sample 
   * mode is bonded, then mask selects inputs to short together. If sample 
   * mode is scan, then mask selects which inputs will be scanned. If sample 
   * mode is single and auto-ground is on (@p autoGnd is true), mask selects 
   * which pins are grounded.
   */
  uint32_t                      inputMask0;

  /** Mask selects inputs 32 to 63. See @p inputMask0 for more information. */
  uint32_t                      inputMask1;

  /** Ground inactive inputs during a conversion. */
  bool                          autoGnd;

  /** Selects the converter type. */
  CSEN_ConvSel_TypeDef          convSel;

  /** Selects the Successive Approximation (SAR) converter resolution. */
  CSEN_SARRes_TypeDef           sarRes;

  /** Selects the Delta Modulation (DM) converter resolution. */
  CSEN_DMRes_TypeDef            dmRes;
  
  /** Sets the number of DM iterations (comparisons) per cycle. Only applies 
    * to the Delta Modulation converter. */
  uint8_t                       dmIterPerCycle;
  
  /** Sets number of DM converter cycles. Only applies to the 
    * Delta Modulation converter. */
  uint8_t                       dmCycles;

  /** Sets the DM converter initial delta value. Only applies to the 
    * Delta Modulation converter. */
  uint8_t                       dmDelta;

  /** Disable DM automatic delta size reduction per cycle. Only applies to the 
    * Delta Modulation converter. */
  bool                          dmFixedDelta;

  /** Selects the reset phase timing. Most measurements should use the default  
    * value. See reference manual for details on when to adjust. */
  CSEN_ResetPhaseSel_TypeDef    resetPhase;

  /** Selects the output drive strength.  Most measurements should use the 
    * default value. See reference manual for details on when to adjust. */
  CSEN_DriveSel_TypeDef         driveSel;

  /** Selects the converter gain. */
  CSEN_GainSel_TypeDef          gainSel;
} CSEN_InitMode_TypeDef;

#define CSEN_INITMODE_DEFAULT                                           \
{                                                                       \
  csenSampleModeSingle,         /* Sample one input and stop. */        \
  csenTrigSelStart,             /* Use start bit to trigger. */         \
  false,                        /* Disable DMA. */                      \
  false,                        /* Average the accumulated result. */   \
  csenAccMode1,                 /* Accumulate 1 sample. */              \
  csenEMASampleW1,              /* Disable the EMA. */                  \
  csenCmpModeDisabled,          /* Disable the comparator. */           \
  0,                            /* Comparator threshold not used. */    \
  csenSingleSelDefault,         /* Disconnect the single input. */      \
  0,                            /* Disable inputs 0 to 31. */           \
  0,                            /* Disable inputs 32 to 63. */          \
  false,                        /* Do not ground inactive inputs. */    \
  csenConvSelSAR,               /* Use the SAR converter. */            \
  csenSARRes10,                 /* Set SAR resolution to 10 bits. */    \
  csenDMRes10,                  /* Set DM resolution to 10 bits. */     \
  0,                            /* Set DM conv/cycle to default. */     \
  0,                            /* Set DM cycles to default. */         \
  0,                            /* Set DM initial delta to default. */  \
  false,                        /* Use DM auto delta reduction. */      \
  csenResetPhaseSel0,           /* Use shortest reset phase time. */    \
  csenDriveSelFull,             /* Use full output current. */          \
  csenGainSel8X,                /* Use highest converter gain. */       \
}


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Get last conversion result.
 *
 * @note
 *   Check conversion busy flag before calling this function. In addition, 
 *   the result width and format depend on the parameters passed to the 
 *   @ref CSEN_InitMode() function.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @return
 *   Result data from last conversion.
 ******************************************************************************/
__STATIC_INLINE uint32_t CSEN_DataGet(CSEN_TypeDef *csen)
{
  return csen->DATA;
}

/***************************************************************************//**
 * @brief
 *   Get last exponential moving average.
 *
 * @note
 *   Confirm CSEN is idle before calling this function.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @return
 *   Exponential moving average from last conversion.
 ******************************************************************************/
__STATIC_INLINE uint32_t CSEN_EMAGet(CSEN_TypeDef *csen)
{
  return (csen->EMA & _CSEN_EMA_EMA_MASK);
}

/***************************************************************************//**
 * @brief
 *   Set exponential moving average initial value.
 *
 * @note
 *   Call this function before starting a conversion.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] ema
 *   Initial value for the exponential moving average.
 ******************************************************************************/
__STATIC_INLINE void CSEN_EMASet(CSEN_TypeDef *csen, uint32_t ema)
{
  csen->EMA = ema & _CSEN_EMA_EMA_MASK;
}

/***************************************************************************//**
 * @brief
 *   Disables the CSEN.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CSEN_Disable(CSEN_TypeDef *csen)
{
  BUS_RegBitWrite(&csen->CTRL, _CSEN_CTRL_EN_SHIFT, 0);
}

/***************************************************************************//**
 * @brief
 *   Enables the CSEN.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CSEN_Enable(CSEN_TypeDef *csen)
{
  BUS_RegBitWrite(&csen->CTRL, _CSEN_CTRL_EN_SHIFT, 1);
}

void CSEN_DMBaselineSet(CSEN_TypeDef *csen, uint32_t up, uint32_t down);
void CSEN_Init(CSEN_TypeDef *csen, const CSEN_Init_TypeDef *init);
void CSEN_InitMode(CSEN_TypeDef *csen, const CSEN_InitMode_TypeDef *init);
void CSEN_Reset(CSEN_TypeDef *csen);


/***************************************************************************//**
 * @brief
 *   Clear one or more pending CSEN interrupts.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] flags
 *   Pending CSEN interrupt source to clear. Use a bitwise logic OR combination
 *   of valid interrupt flags for the CSEN module (CSEN_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CSEN_IntClear(CSEN_TypeDef *csen, uint32_t flags)
{
  csen->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more CSEN interrupts.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] flags
 *   CSEN interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the CSEN module (CSEN_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CSEN_IntDisable(CSEN_TypeDef *csen, uint32_t flags)
{
  csen->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more CSEN interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using CSEN_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] flags
 *   CSEN interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the CSEN module (CSEN_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CSEN_IntEnable(CSEN_TypeDef *csen, uint32_t flags)
{
  csen->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending CSEN interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @return
 *   CSEN interrupt sources pending. A bitwise logic OR combination of valid
 *   interrupt flags for the CSEN module (CSEN_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t CSEN_IntGet(CSEN_TypeDef *csen)
{
  return csen->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending CSEN interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled CSEN interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in CSENx_IEN_nnn
 *     register (CSENx_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the CSEN module
 *     (CSENx_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t CSEN_IntGetEnabled(CSEN_TypeDef *csen)
{
  uint32_t ien;

  /* Store CSENx->IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  ien = csen->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return csen->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending CSEN interrupts from SW.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @param[in] flags
 *   CSEN interrupt sources to set to pending. Use a bitwise logic OR combination
 *   of valid interrupt flags for the CSEN module (CSEN_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void CSEN_IntSet(CSEN_TypeDef *csen, uint32_t flags)
{
  csen->IFS = flags;
}


/***************************************************************************//**
 * @brief
 *   Return CSEN conversion busy status.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 *
 * @return
 *   True if CSEN conversion is in progress.
 ******************************************************************************/
__STATIC_INLINE bool CSEN_IsBusy(CSEN_TypeDef *csen)
{
  return (bool)(csen->STATUS & _CSEN_STATUS_CSENBUSY_MASK);
}


/***************************************************************************//**
 * @brief
 *   Start scan sequence and/or single conversion.
 *
 * @param[in] csen
 *   Pointer to CSEN peripheral register block.
 ******************************************************************************/
__STATIC_INLINE void CSEN_Start(CSEN_TypeDef *csen)
{
  csen->CMD = CSEN_CMD_START;
}


/** @} (end addtogroup CSEN) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(CSEN_COUNT) && (CSEN_COUNT > 0) */
#endif /* EM_CSEN_H */

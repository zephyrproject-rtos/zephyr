/***************************************************************************//**
 * @file em_iadc.c
 * @brief Incremental Analog to Digital Converter (IADC) Peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
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

#include "em_iadc.h"
#if defined(IADC_COUNT) && (IADC_COUNT > 0)

#include "em_assert.h"
#include "em_cmu.h"
#include "em_common.h"
#include <stddef.h>

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup IADC
 * @brief Incremental Analog to Digital Converter (IADC) Peripheral API
 * @details
 *  This module contains functions to control the IADC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The IADC is used to convert analog signals into a
 *  digital representation.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

// Validation of IADC register block pointer reference for assert statements.
#if (IADC_COUNT == 1)
#define IADC_REF_VALID(ref)    ((ref) == IADC0)
#elif (IADC_COUNT == 2)
#define IADC_REF_VALID(ref)    (((ref) == IADC0) || ((ref) == IADC1))
#endif

// Max IADC clock rates
#define IADC_CLK_MAX_FREQ                   40000000UL
#define IADC_ANA_CLK_HIGH_SPEED_MAX_FREQ    20000000UL
#define IADC_ANA_CLK_NORMAL_MAX_FREQ        10000000UL
#define IADC_ANA_CLK_HIGH_ACCURACY_MAX_FREQ  5000000UL
#define IADC_ANA_CLK_MAX_FREQ(adcMode) (                          \
    (adcMode) == iadcCfgModeNormal ? IADC_ANA_CLK_NORMAL_MAX_FREQ \
    : IADC_ANA_CLK_HIGH_ACCURACY_MAX_FREQ                         \
    )

// TODO these should be defined in device headers somewhere
#define IADC0_SCANENTRIES IADC0_ENTRIES
#define IADC0_FIFOENTRIES 0x4UL

#define IADC1_SCANENTRIES IADC1_ENTRIES
#define IADC1_FIFOENTRIES 0x4UL

#define IADC_SCANENTRIES(iadc) (        \
    (iadc) == IADC0 ? IADC0_SCANENTRIES \
    : 0UL)
#define IADC_CONFIGNUM(iadc) (        \
    (iadc) == IADC0 ? IADC0_CONFIGNUM \
    : 0UL)
#define IADC_FIFOENTRIES(iadc) (        \
    (iadc) == IADC0 ? IADC0_FIFOENTRIES \
    : 0UL)
#define IADC_CMU_CLOCK(iadc) (       \
    (iadc) == IADC0 ? cmuClock_IADC0 \
    : cmuClock_IADC0)

// Gain and offset correction
// TODO These defines must be replaced with production calibrated values
//   from the DI page once these are defined

// High speed and normal mode gain correction
// Same value is used for all oversampling rates
#define IADC_OSRHS_GAIN_CORRECTION        0x8000UL

// High speed and normal mode offset correction
static const uint32_t highSpeedOffsetCorrection[6] =
{
  0x2C000, /* 2x */
  0x36000, /* 4x */
  0x3B000, /* 8x */
  0x3D800, /* 16x */
  0x3EC00, /* 32x */
  0x3F600  /* 64x */
};

/** @endcond */

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

static void IADC_disable(IADC_TypeDef *iadc)
{
  iadc->EN_CLR = IADC_EN_EN;
}

static void IADC_enable(IADC_TypeDef *iadc)
{
  iadc->EN_SET = IADC_EN_EN;
}

static IADC_Result_t IADC_ConvertRawDataToResult(uint32_t rawData,
                                                 IADC_Alignment_t alignment)
{
  IADC_Result_t result;

  switch (alignment) {
    case iadcAlignRight12:
      // Mask out ID and replace with sign extension
      result.data = (rawData & 0x00FFFFFFUL)
                    | ((rawData & 0x00800000UL) != 0x0UL ? 0xFF000000UL : 0x0UL);
      // Mask out data and shift down
      result.id   = (uint8_t) (rawData & 0xFF000000UL) >> 24;
      break;
    case iadcAlignLeft12:
      result.data = rawData & 0xFFFFFF00UL;
      result.id   = (uint8_t) (rawData & 0x000000FFUL);
      break;
    default:
      break;
  }
  return result;
}

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize IADC.
 *
 * @details
 *   Initializes common parts for both single conversion and scan sequence.
 *   In addition, single and/or scan control configuration must be done, please
 *   refer to @ref IADC_initSingle() and @ref IADC_initScan() respectively.
 *
 * @note
 *   This function will stop any ongoing conversions.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] init
 *   Pointer to IADC initialization structure.
 *
 * @param[in] allConfigs
 *   Pointer to structure holding all configs.
 ******************************************************************************/
void IADC_init(IADC_TypeDef *iadc,
               const IADC_Init_t *init,
               const IADC_AllConfigs_t *allConfigs)
{
  uint32_t tmp;
  uint32_t config;
  uint8_t wantedPrescale;
  uint8_t srcClkPrescale;
  uint8_t adcClkPrescale;
  uint8_t timebase;
  IADC_CfgAdcMode_t adcMode;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  // Calculate min allowed SRC_CLK prescaler setting
  srcClkPrescale = IADC_calcSrcClkPrescale(iadc, IADC_CLK_MAX_FREQ, 0);

  wantedPrescale = init->srcClkPrescale;
  // Use wanted SRC_CLK prescaler setting instead if it is high enough
  if (wantedPrescale >= srcClkPrescale) {
    srcClkPrescale = wantedPrescale;
  }

  IADC_disable(iadc);

  timebase = init->timebase;
  if (timebase == 0) {
    // Calculate timebase based on CMU_IADCCLKCTRL
    timebase = IADC_calcTimebase(iadc, 0);
  }

  tmp = (((uint32_t)(init->warmup) << _IADC_CTRL_WARMUPMODE_SHIFT)
         & _IADC_CTRL_WARMUPMODE_MASK)
        | (((uint32_t)(timebase) << _IADC_CTRL_TIMEBASE_SHIFT)
           & _IADC_CTRL_TIMEBASE_MASK)
        | (((uint32_t)(srcClkPrescale) << _IADC_CTRL_HSCLKRATE_SHIFT)
           & _IADC_CTRL_HSCLKRATE_MASK);

  if (init->iadcClkSuspend0) {
    tmp |= IADC_CTRL_ADCCLKSUSPEND0;
  }
  if (init->iadcClkSuspend1) {
    tmp |= IADC_CTRL_ADCCLKSUSPEND1;
  }
  if (init->debugHalt) {
    tmp |= IADC_CTRL_DBGHALT;
  }
  iadc->CTRL = tmp;

  iadc->TIMER = ((uint32_t) (init->timerCycles) << _IADC_TIMER_TIMER_SHIFT)
                & _IADC_TIMER_TIMER_MASK;

  iadc->CMPTHR = (((uint32_t) (init->greaterThanEqualThres) << _IADC_CMPTHR_ADGT_SHIFT)
                  & _IADC_CMPTHR_ADGT_MASK)
                 | (((uint32_t) (init->lessThanEqualThres) << _IADC_CMPTHR_ADLT_SHIFT)
                    & _IADC_CMPTHR_ADLT_MASK);

  // Write configurations
  for (config = 0; config < IADC_CONFIGNUM(iadc); config++) {
    // Find min allowed ADC_CLK prescaler setting for given mode
    adcMode = allConfigs->configs[config].adcMode;
    wantedPrescale = allConfigs->configs[config].adcClkPrescale;
    adcClkPrescale = IADC_calcAdcClkPrescale(iadc,
                                             IADC_ANA_CLK_MAX_FREQ(adcMode),
                                             0,
                                             adcMode,
                                             srcClkPrescale);

    // Use wanted ADC_CLK prescaler setting instead if it is high enough
    adcClkPrescale = SL_MAX(adcClkPrescale, wantedPrescale);

    tmp = iadc->CFG[config].CFG & ~(_IADC_CFG_ADCMODE_MASK | _IADC_CFG_OSRHS_MASK
                                    | _IADC_CFG_ANALOGGAIN_MASK | _IADC_CFG_REFSEL_MASK
                                    | _IADC_CFG_TWOSCOMPL_MASK);
    iadc->CFG[config].CFG = tmp
                            | (((uint32_t)(adcMode) << _IADC_CFG_ADCMODE_SHIFT) & _IADC_CFG_ADCMODE_MASK)
                            | (((uint32_t)(allConfigs->configs[config].osrHighSpeed) << _IADC_CFG_OSRHS_SHIFT)
                               & _IADC_CFG_OSRHS_MASK)
                            | (((uint32_t)(allConfigs->configs[config].analogGain) << _IADC_CFG_ANALOGGAIN_SHIFT)
                               & _IADC_CFG_ANALOGGAIN_MASK)
                            | (((uint32_t)(allConfigs->configs[config].reference) << _IADC_CFG_REFSEL_SHIFT)
                               & _IADC_CFG_REFSEL_MASK)
                            | (((uint32_t)(allConfigs->configs[config].twosComplement) << _IADC_CFG_TWOSCOMPL_SHIFT)
                               & _IADC_CFG_TWOSCOMPL_MASK);

    // Gain and offset correction is applied according to adcMode and oversampling rate.
    switch (adcMode) {
      case iadcCfgModeNormal:
        iadc->CFG[config].SCALE = (((uint32_t) highSpeedOffsetCorrection[allConfigs->configs[config].osrHighSpeed] << _IADC_SCALE_OFFSET_SHIFT)
                                   & _IADC_SCALE_OFFSET_MASK)
                                  | (((uint32_t) (IADC_OSRHS_GAIN_CORRECTION & 0x1FFFUL) << _IADC_SCALE_GAIN13LSB_SHIFT)
                                     & _IADC_SCALE_GAIN13LSB_MASK)
                                  | (((uint32_t) ((IADC_OSRHS_GAIN_CORRECTION & 0x8000UL) >> 15) << _IADC_SCALE_GAIN3MSB_SHIFT)
                                     & _IADC_SCALE_GAIN3MSB_MASK);
        break;
      default:
        break;
    }
    iadc->CFG[config].SCHED = (((uint32_t)(adcClkPrescale) << _IADC_SCHED_PRESCALE_SHIFT)
                               & _IADC_SCHED_PRESCALE_MASK);
  }
  IADC_enable(iadc);
}

/***************************************************************************//**
 * @brief
 *   Initialize IADC scan sequence.
 *
 * @details
 *   This function will configure scan mode and set up entries in the scan
 *   table. The scan table mask can be updated by calling IADC_updateScanMask.
 *
 * @note
 *   This function will stop any ongoing conversions.
 *
 * @note If an even numbered pin is selected for the positive input, the
 *   negative input must use an odd numbered pin and vice versa.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] init
 *   Pointer to IADC initialization structure.
 *
 * @param[in] scanTable
 *   Pointer to IADC scan table structure.
 ******************************************************************************/
void IADC_initScan(IADC_TypeDef *iadc,
                   const IADC_InitScan_t *init,
                   const IADC_ScanTable_t *scanTable)
{
  uint32_t i;
  uint32_t tmp;
  EFM_ASSERT(IADC_REF_VALID(iadc));

  IADC_disable(iadc);

  iadc->SCANFIFOCFG = (((uint32_t) (init->alignment) << _IADC_SCANFIFOCFG_ALIGNMENT_SHIFT)
                       & _IADC_SCANFIFOCFG_ALIGNMENT_MASK)
                      | (init->showId ? IADC_SCANFIFOCFG_SHOWID : 0UL)
                      | (((uint32_t) (init->dataValidLevel) << _IADC_SCANFIFOCFG_DVL_SHIFT)
                         & _IADC_SCANFIFOCFG_DVL_MASK)
                      | (init->fifoDmaWakeup ? IADC_SCANFIFOCFG_DMAWUFIFOSCAN : 0UL);

  // Clear bitfields for scan conversion in IADCn->TRIGGER and set new values
  iadc->TRIGGER = (iadc->TRIGGER & ~(_IADC_TRIGGER_SCANTRIGSEL_MASK
                                     | _IADC_TRIGGER_SCANTRIGACTION_MASK))
                  | (((uint32_t) (init->triggerSelect) << _IADC_TRIGGER_SCANTRIGSEL_SHIFT)
                     & _IADC_TRIGGER_SCANTRIGSEL_MASK)
                  | (((uint32_t) (init->triggerAction) << _IADC_TRIGGER_SCANTRIGACTION_SHIFT)
                     & _IADC_TRIGGER_SCANTRIGACTION_MASK);

  // Write scan table
  for (i = 0; i < IADC_SCANENTRIES(iadc); i++) {
    iadc->SCANTABLE[i].SCAN = (((uint32_t) (scanTable->entries[i].negInput) << _IADC_SCAN_PINNEG_SHIFT)
                               & (_IADC_SCAN_PORTNEG_MASK | _IADC_SCAN_PINNEG_MASK))
                              | (((uint32_t) (scanTable->entries[i].posInput) << _IADC_SCAN_PINPOS_SHIFT)
                                 & (_IADC_SCAN_PORTPOS_MASK | _IADC_SCAN_PINPOS_MASK))
                              | (((uint32_t) (scanTable->entries[i].configId) << _IADC_SCAN_CFG_SHIFT)
                                 & _IADC_SCAN_CFG_MASK)
                              | (scanTable->entries[i].compare ? IADC_SCAN_CMP : 0UL);
  }

  IADC_enable(iadc);

  // Set scan mask
  tmp = 0;
  for (i = 0; i < IADC_SCANENTRIES(iadc); i++) {
    if (scanTable->entries[i].includeInScan) {
      tmp |= (1UL << i) << _IADC_MASKREQ_MASKREQ_SHIFT;
    }
  }
  iadc->MASKREQ = tmp;

  if (init->start) {
    IADC_command(iadc, iadcCmdStartScan);
  }
}

/***************************************************************************//**
 * @brief
 *   Initialize single IADC conversion.
 *
 * @details
 *   This function will initialize the single conversion and configure the
 *   single input selection.
 *
 * @note
 *   This function will stop any ongoing conversions.
 *
 * @note If an even numbered pin is selected for the positive input, the
 *   negative input must use an odd numbered pin and vice versa.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] init
 *   Pointer to IADC single initialization structure.
 *
 * @param[in] singleInput
 *   Pointer to IADC single input selection initialization structure.
 ******************************************************************************/
void IADC_initSingle(IADC_TypeDef *iadc,
                     const IADC_InitSingle_t *init,
                     const IADC_SingleInput_t *input)
{
  EFM_ASSERT(IADC_REF_VALID(iadc));

  IADC_disable(iadc);

  iadc->SINGLEFIFOCFG = (((uint32_t) (init->alignment) << _IADC_SINGLEFIFOCFG_ALIGNMENT_SHIFT)
                         & _IADC_SINGLEFIFOCFG_ALIGNMENT_MASK)
                        | (init->showId ? IADC_SINGLEFIFOCFG_SHOWID : 0UL)
                        | (((uint32_t) (init->dataValidLevel) << _IADC_SINGLEFIFOCFG_DVL_SHIFT)
                           & _IADC_SINGLEFIFOCFG_DVL_MASK)
                        | (init->fifoDmaWakeup ? IADC_SINGLEFIFOCFG_DMAWUFIFOSINGLE : 0UL);

  // Clear bitfields for single conversion in IADCn->TRIGGER and set new values
  iadc->TRIGGER = (iadc->TRIGGER & (_IADC_TRIGGER_SINGLETRIGSEL_MASK
                                    | _IADC_TRIGGER_SINGLETRIGACTION_MASK
                                    | _IADC_TRIGGER_SINGLETAILGATE_MASK))
                  | (((uint32_t) (init->triggerSelect) << _IADC_TRIGGER_SINGLETRIGSEL_SHIFT)
                     & _IADC_TRIGGER_SINGLETRIGSEL_MASK)
                  | (((uint32_t) (init->triggerAction) << _IADC_TRIGGER_SINGLETRIGACTION_SHIFT)
                     & _IADC_TRIGGER_SINGLETRIGACTION_MASK)
                  | (init->singleTailgate ? IADC_TRIGGER_SINGLETAILGATE : 0UL);

  IADC_updateSingleInput(iadc, input);

  IADC_enable(iadc);

  if (init->start) {
    IADC_command(iadc, iadcCmdStartSingle);
  }
}

/***************************************************************************//**
 * @brief
 *   Update IADC single input selection.
 *
 * @details
 *   This function updates the single input selection. The function can be
 *   called while single and/or scan conversions are ongoing and the new input
 *   configuration will take place on the next single conversion.
 *
 * @note If an even numbered pin is selected for the positive input, the
 *   negative input must use an odd numbered pin and vice versa.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] input
 *   Pointer to single input selection structure.
 ******************************************************************************/
void IADC_updateSingleInput(IADC_TypeDef *iadc,
                            const IADC_SingleInput_t *input)
{
  bool enabled;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  enabled = (iadc->EN & IADC_EN_EN) != 0UL;

  // IADCn->SINGLE has WSYNC type and can only be written while enabled
  IADC_enable(iadc);

  iadc->SINGLE = (((uint32_t) (input->negInput) << _IADC_SINGLE_PINNEG_SHIFT)
                  & (_IADC_SINGLE_PORTNEG_MASK | _IADC_SINGLE_PINNEG_MASK))
                 | (((uint32_t) (input->posInput) << _IADC_SINGLE_PINPOS_SHIFT)
                    & (_IADC_SINGLE_PORTPOS_MASK | _IADC_SINGLE_PINPOS_MASK))
                 | (((uint32_t) (input->configId) << _IADC_SINGLE_CFG_SHIFT)
                    & _IADC_SINGLE_CFG_MASK)
                 | (input->compare ? IADC_SINGLE_CMP : 0UL);

  // Restore enabled state
  if (!enabled) {
    IADC_disable(iadc);
  }
}

/***************************************************************************//**
 * @brief
 *   Set mask of IADC scan table entries to include in scan.
 *
 * @details
 *   Set mask of scan table entries to include in next scan. This function
 *   can be called while scan conversions are ongoing, but the new scan mask
 *   will take effect once the ongoing scan is completed.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] mask
 *   Mask of scan table entries to include in scan.
 ******************************************************************************/
void IADC_setScanMask(IADC_TypeDef *iadc, uint32_t mask)
{
  bool enabled;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  EFM_ASSERT(mask <= ((1UL << IADC_SCANENTRIES(iadc)) - 1UL));

  enabled = (iadc->EN & IADC_EN_EN) != 0UL;

  // IADC must be enabled to update scan table mask
  IADC_enable(iadc);

  iadc->MASKREQ = (mask << _IADC_MASKREQ_MASKREQ_SHIFT)
                  & _IADC_MASKREQ_MASKREQ_MASK;

  // Restore enabled state
  if (!enabled) {
    IADC_disable(iadc);
  }
}

/***************************************************************************//**
 * @brief
 *   Add/update entry in scan table.
 *
 * @details
 *   This function will update or add an entry in the scan table with a specific
 *   ID.
 *
 * @note
 *   This function will stop any ongoing conversions.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] id
 *   Id of scan table entry to add.
 *
 * @param[in] entry
 *   Pointer to scan table entry structure.
 ******************************************************************************/
void IADC_updateScanEntry(IADC_TypeDef *iadc,
                          uint8_t id,
                          IADC_ScanTableEntry_t *entry)
{
  bool enabled;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  enabled = (iadc->EN & IADC_EN_EN) != 0UL;

  // IADC must be disabled to update scan table
  IADC_disable(iadc);

  // Update entry in scan table
  iadc->SCANTABLE[id].SCAN = (((uint32_t) (entry->negInput) << _IADC_SCAN_PINNEG_SHIFT)
                              & (_IADC_SCAN_PORTNEG_MASK | _IADC_SCAN_PINNEG_MASK))
                             | (((uint32_t) (entry->posInput) << _IADC_SCAN_PINPOS_SHIFT)
                                & (_IADC_SCAN_PORTPOS_MASK | _IADC_SCAN_PINPOS_MASK))
                             | (((uint32_t) (entry->configId) << _IADC_SCAN_CFG_SHIFT)
                                & _IADC_SCAN_CFG_MASK)
                             | (entry->compare ? IADC_SCAN_CMP : 0UL);

  // IADC must be enabled to update scan table mask
  IADC_enable(iadc);

  if (entry->includeInScan) {
    iadc->MASKREQ_SET = (1UL << (id & 0x1FUL)) << _IADC_MASKREQ_MASKREQ_SHIFT;
  } else {
    iadc->MASKREQ_CLR = (1UL << (id & 0x1FUL)) << _IADC_MASKREQ_MASKREQ_SHIFT;
  }

  // Restore enabled state
  if (!enabled) {
    IADC_disable(iadc);
  }
}

/***************************************************************************//**
 * @brief
 *   Reset IADC to same state as after a HW reset.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 ******************************************************************************/
void IADC_reset(IADC_TypeDef *iadc)
{
  uint32_t i;
  EFM_ASSERT(IADC_REF_VALID(iadc));

  // Write all WSYNC registers to reset value while enabled
  IADC_enable(iadc);

  // Stop conversions and timer, before resetting other registers.
  iadc->CMD = IADC_CMD_SINGLESTOP | IADC_CMD_SCANSTOP | IADC_CMD_TIMERDIS;

  // Wait for all IADC operations to stop
  while ((iadc->STATUS & (IADC_STATUS_CONVERTING
                          | IADC_STATUS_SCANQUEUEPENDING
                          | IADC_STATUS_SINGLEQUEUEPENDING
                          | IADC_STATUS_TIMERACTIVE))
         != 0UL) {
  }

  // Reset all WSYNC registers
  iadc->MASKREQ = _IADC_MASKREQ_RESETVALUE;
  iadc->SINGLE  = _IADC_SINGLE_RESETVALUE;

  // Wait for SINGLE and MASQREQ writes to propagate to working registers
  while ((iadc->STATUS & (IADC_STATUS_MASKREQWRITEPENDING
                          | IADC_STATUS_SINGLEWRITEPENDING))
         != 0UL) {
  }

  // Pull from FIFOs until they are empty
  while ((iadc->STATUS & IADC_STATUS_SINGLEFIFODV) != 0UL) {
    (void) IADC_pullSingleFifoData(iadc);
  }

  while ((iadc->STATUS & IADC_STATUS_SCANFIFODV) != 0UL) {
    (void) IADC_pullScanFifoData(iadc);
  }

  // Read data registers to clear data valid flags
  (void) IADC_readSingleData(iadc);
  (void) IADC_readScanData(iadc);

  // Write all WSTATIC registers to reset value while disabled
  IADC_disable(iadc);

  // Reset all WSTATIC registers
  iadc->CTRL            = _IADC_CTRL_RESETVALUE;
  iadc->TIMER           = _IADC_TIMER_RESETVALUE;
  iadc->TRIGGER         = _IADC_TRIGGER_RESETVALUE;

  iadc->CMPTHR          = _IADC_CMPTHR_RESETVALUE;
  iadc->SINGLEFIFOCFG   = _IADC_SINGLEFIFOCFG_RESETVALUE;
  iadc->SCANFIFOCFG     = _IADC_SCANFIFOCFG_RESETVALUE;

  for (i = 0; i < IADC_CONFIGNUM(iadc); i++) {
    iadc->CFG[i].CFG    = _IADC_CFG_RESETVALUE;
    iadc->CFG[i].SCALE  = _IADC_SCALE_RESETVALUE;
    iadc->CFG[i].SCHED  = _IADC_SCHED_RESETVALUE;
  }

  for (i = 0; i < IADC_SCANENTRIES(iadc); i++) {
    iadc->SCANTABLE[i].SCAN = _IADC_SCAN_RESETVALUE;
  }

  // Clear interrupt flags and disable interrupts
  IADC_clearInt(iadc, _IADC_IF_MASK);
  IADC_disableInt(iadc, _IADC_IEN_MASK);
}

/***************************************************************************//**
 * @brief
 *   Calculate timebase value in order to get a timebase providing at least 1us.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] cmuClkFreq Frequency in Hz of reference CLK_CMU_ADC clock. Set to 0 to
 *   use currently defined CMU clock setting for the IADC.
 *
 * @return
 *   Timebase value to use for IADC in order to achieve at least 1 us.
 ******************************************************************************/
uint8_t IADC_calcTimebase(IADC_TypeDef *iadc, uint32_t cmuClkFreq)
{
  EFM_ASSERT(IADC_REF_VALID(iadc));

  if (cmuClkFreq == 0UL) {
    cmuClkFreq = CMU_ClockFreqGet(IADC_CMU_CLOCK(iadc));

    // Just in case, make sure we get non-zero freq for below calculation
    if (cmuClkFreq == 0UL) {
      cmuClkFreq = 1;
    }
  }
  // Determine number of ADCCLK cycle >= 1us
  cmuClkFreq += 999999UL;
  cmuClkFreq /= 1000000UL;

  // Convert to N+1 format
  cmuClkFreq -= 1UL;

  // Limit to max allowed register setting
  cmuClkFreq = SL_MIN(cmuClkFreq, (_IADC_CTRL_TIMEBASE_MASK >> _IADC_CTRL_TIMEBASE_SHIFT));

  // Return timebase value
  return (uint8_t) cmuClkFreq;
}

/***************************************************************************//**
 * @brief
 *   Calculate prescaler for CLK_SRC_ADC high speed clock
 *
 * @details
 *   The IADC high speed clock is given by: CLK_SRC_ADC / (srcClkPrescaler + 1).
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] srcClkFreq CLK_SRC_ADC frequency wanted. The frequency will
 *   automatically be adjusted to be within valid range according to reference
 *   manual.
 *
 * @param[in] cmuClkFreq Frequency in Hz of reference CLK_CMU_ADC. Set to 0
 *   to use currently defined CMU clock setting for the IADC.
 *
 * @return
 *   Divider value to use for IADC in order to achieve a high speed clock value
 *   <= @p srcClkFreq.
 ******************************************************************************/
uint8_t IADC_calcSrcClkPrescale(IADC_TypeDef *iadc,
                                uint32_t srcClkFreq,
                                uint32_t cmuClkFreq)
{
  uint32_t ret;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  // Make sure wanted CLK_SRC_ADC clock is below max allowed frequency
  srcClkFreq = SL_MIN(srcClkFreq, IADC_CLK_MAX_FREQ);

  // Use current CLK_CMU_ADC frequency?
  if (cmuClkFreq == 0UL) {
    cmuClkFreq = CMU_ClockFreqGet(IADC_CMU_CLOCK(iadc));
  }

  ret = (cmuClkFreq + srcClkFreq - 1UL) / srcClkFreq;
  if (ret != 0UL) {
    ret--;
  }

  // Limit to max allowed register setting
  if (ret > _IADC_CTRL_HSCLKRATE_DIV4) {
    ret = _IADC_CTRL_HSCLKRATE_DIV4;
  }

  return (uint8_t)ret;
}

/***************************************************************************//**
 * @brief
 *   Calculate prescaler for ADC_CLK clock.
 *
 * @details
 *   The ADC_CLK is given by: CLK_SRC_ADC / (adcClkprescale + 1).
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @param[in] adcClkFreq  ADC_CLK frequency wanted. The frequency will
 *   automatically be adjusted to be within valid range according to reference
 *   manual.
 *
 * @param[in] CmuClkFreq Frequency in Hz of CLK_CMU_ADC Set to 0 to
 *   use currently defined IADC clock setting (in CMU).
 *
 * @param[in] adcMode Mode for IADC config.
 *
 * @param[in] srcClkPrescaler Precaler setting for ADC_CLK
 *
 * @return
 *   Divider value to use for IADC in order to achieve a ADC_CLK frequency
 *   <= @p adcClkFreq.
 ******************************************************************************/
uint8_t IADC_calcAdcClkPrescale(IADC_TypeDef *iadc,
                                uint32_t adcClkFreq,
                                uint32_t cmuClkFreq,
                                IADC_CfgAdcMode_t adcMode,
                                uint8_t srcClkPrescaler)
{
  uint32_t ret;
  uint32_t resFreq;

  EFM_ASSERT(IADC_REF_VALID(iadc));

  // Make sure wanted analog clock is below max allowed frequency for the given
  // mode.
  if (adcClkFreq > IADC_ANA_CLK_MAX_FREQ(adcMode)) {
    adcClkFreq = IADC_ANA_CLK_MAX_FREQ(adcMode);
  }

  // Use current CLK_CMU_ADC frequency?
  if (cmuClkFreq == 0UL) {
    resFreq = CMU_ClockFreqGet(IADC_CMU_CLOCK(iadc));
  } else {
    resFreq = cmuClkFreq;
  }

  // Apply CLK_SRC_ADC prescaler
  resFreq /= srcClkPrescaler + 1UL;

  ret = (resFreq + adcClkFreq - 1UL) / adcClkFreq;
  if (ret != 0UL) {
    ret--;
  }

  // Limit to max allowed register setting
  ret = SL_MIN(ret, (_IADC_SCHED_PRESCALE_MASK >> _IADC_SCHED_PRESCALE_SHIFT));

  return (uint8_t)ret;
}

/***************************************************************************//**
 * @brief
 *   Pull result from single data FIFO. The result struct includes both the data
 *   and the ID (0x20) if showId was set when initializing single mode.
 *
 * @note
 *   Check data valid flag before calling this function.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @return
 *   Single conversion result struct holding data and id.
 ******************************************************************************/
IADC_Result_t IADC_pullSingleFifoResult(IADC_TypeDef *iadc)
{
  uint32_t alignment = (iadc->SINGLEFIFOCFG & _IADC_SINGLEFIFOCFG_ALIGNMENT_MASK)
                       >> _IADC_SINGLEFIFOCFG_ALIGNMENT_SHIFT;
  return IADC_ConvertRawDataToResult(iadc->SINGLEFIFODATA,
                                     (IADC_Alignment_t) alignment);
}

/***************************************************************************//**
 * @brief
 *   Read most recent single conversion result. The result struct includes both
 *   the data and the ID (0x20) if showId was set when initializing single mode.
 *   Calling this function will not affect the state of the single data FIFO.
 *
 * @note
 *   Check data valid flag before calling this function.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @return
 *   Single conversion result struct holding data and id.
 ******************************************************************************/
IADC_Result_t IADC_readSingleResult(IADC_TypeDef *iadc)
{
  uint32_t alignment = (iadc->SINGLEFIFOCFG & _IADC_SINGLEFIFOCFG_ALIGNMENT_MASK)
                       >> _IADC_SINGLEFIFOCFG_ALIGNMENT_SHIFT;
  return IADC_ConvertRawDataToResult(iadc->SINGLEDATA,
                                     (IADC_Alignment_t) alignment);
}

/***************************************************************************//**
 * @brief
 *   Pull result from scan data FIFO. The result struct includes both the data
 *   and the ID (0x20) if showId was set when initializing scan entry.
 *
 * @note
 *   Check data valid flag before calling this function.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @return
 *   Scan conversion result struct holding data and id.
 ******************************************************************************/
IADC_Result_t IADC_pullScanFifoResult(IADC_TypeDef *iadc)
{
  uint32_t alignment = (iadc->SCANFIFOCFG & _IADC_SCANFIFOCFG_ALIGNMENT_MASK)
                       >> _IADC_SCANFIFOCFG_ALIGNMENT_SHIFT;
  return IADC_ConvertRawDataToResult(iadc->SCANFIFODATA,
                                     (IADC_Alignment_t) alignment);
}

/***************************************************************************//**
 * @brief
 *   Read most recent scan conversion result. The result struct includes both
 *   the data and the ID (0x20) if showId was set when initializing scan entry.
 *   Calling this function will not affect the state of the scan data FIFO.
 *
 * @note
 *   Check data valid flag before calling this function.
 *
 * @param[in] iadc
 *   Pointer to IADC peripheral register block.
 *
 * @return
 *   Scan conversion result struct holding data and id.
 ******************************************************************************/
IADC_Result_t IADC_readScanResult(IADC_TypeDef *iadc)
{
  uint32_t alignment = (iadc->SCANFIFOCFG & _IADC_SCANFIFOCFG_ALIGNMENT_MASK)
                       >> _IADC_SCANFIFOCFG_ALIGNMENT_SHIFT;
  return IADC_ConvertRawDataToResult(iadc->SCANDATA,
                                     (IADC_Alignment_t) alignment);
}

/** @} (end addtogroup IADC) */
/** @} (end addtogroup emlib) */
#endif /* defined(IADC_COUNT) && (IADC_COUNT > 0) */

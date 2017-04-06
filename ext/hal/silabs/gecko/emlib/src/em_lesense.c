/***************************************************************************//**
 * @file em_lesense.c
 * @brief Low Energy Sensor (LESENSE) Peripheral API
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

#include "em_lesense.h"

#if defined(LESENSE_COUNT) && (LESENSE_COUNT > 0)
#include "em_assert.h"
#include "em_bus.h"
#include "em_cmu.h"

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if !defined(UINT32_MAX)
#define UINT32_MAX ((uint32_t)(0xFFFFFFFF))
#endif
/** @endcond */

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LESENSE
 * @brief Low Energy Sensor (LESENSE) Peripheral API
 * @details
 *  This module contains functions to control the LESENSE peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. LESENSE is a low energy sensor interface capable
 *  of autonomously collecting and processing data from multiple sensors even
 *  when in EM2.
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if defined(_LESENSE_ROUTE_MASK)
#define GENERIC_LESENSE_ROUTE    LESENSE->ROUTE
#else
#define GENERIC_LESENSE_ROUTE    LESENSE->ROUTEPEN
#endif

#if defined(_SILICON_LABS_32B_SERIES_0)
/* DACOUT mode only available on channel 0,1,2,3,12,13,14,15 */
#define DACOUT_SUPPORT  0xF00F
#else
/* DACOUT mode only available on channel 4,5,7,10,12,13 */
#define DACOUT_SUPPORT  0x34B0
#endif
/** @endcond */

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize the LESENSE module.
 *
 * @details
 *   This function configures the main parameters of the LESENSE interface.
 *   Please refer to the initialization parameter type definition
 *   (@ref LESENSE_Init_TypeDef) for more details.
 *
 * @note
 *   @ref LESENSE_Init() has been designed for initializing LESENSE once in an
 *   operation cycle. Be aware of the effects of reconfiguration if using this
 *   function from multiple sources in your code. This function has not been
 *   designed to be re-entrant.
 *   Requesting reset by setting @p reqReset to true is required in each reset
 *   or power-on cycle in order to configure the default values of the RAM
 *   mapped LESENSE registers.
 *   Notice that GPIO pins used by the LESENSE module must be properly
 *   configured by the user explicitly, in order for the LESENSE to work as
 *   intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] init
 *   LESENSE initialization structure.
 *
 * @param[in] reqReset
 *   Request to call @ref LESENSE_Reset() first in order to initialize all
 *   LESENSE registers with the default value.
 ******************************************************************************/
void LESENSE_Init(const LESENSE_Init_TypeDef * init, bool reqReset)
{
  /* Sanity check of initialization values */
  EFM_ASSERT((uint32_t)init->timeCtrl.startDelay < 4U);
#if defined(_LESENSE_PERCTRL_DACPRESC_MASK)
  EFM_ASSERT((uint32_t)init->perCtrl.dacPresc < 32U);
#endif

  /* Reset LESENSE registers if requested. */
  if (reqReset)
  {
    LESENSE_Reset();
  }

  /* Set sensor start delay for each channel. */
  LESENSE_StartDelaySet((uint32_t)init->timeCtrl.startDelay);
#if defined(_LESENSE_TIMCTRL_AUXSTARTUP_MASK)
  /* Configure the AUXHRFCO startup delay. */
  LESENSE->TIMCTRL = (LESENSE->TIMCTRL & (~_LESENSE_TIMCTRL_AUXSTARTUP_MASK))
                     | (init->timeCtrl.delayAuxStartup << _LESENSE_TIMCTRL_AUXSTARTUP_SHIFT);
#endif

  /* LESENSE core control configuration.
   * Set PRS source, SCANCONF register usage strategy, interrupt and
   * DMA trigger level condition, DMA wakeup condition, bias mode,
   * enable/disable to sample both ACMPs simultaneously, enable/disable to store
   * SCANRES in CNT_RES after each scan, enable/disable to always write to the
   * result buffer, even if it is full, enable/disable LESENSE running in debug
   * mode. */
  LESENSE->CTRL =
    ((uint32_t)init->coreCtrl.prsSel         << _LESENSE_CTRL_PRSSEL_SHIFT)
    | (uint32_t)init->coreCtrl.scanConfSel
    | (uint32_t)init->coreCtrl.bufTrigLevel
    | (uint32_t)init->coreCtrl.wakeupOnDMA
#if defined(_LESENSE_CTRL_ACMP0INV_MASK)
    | ((uint32_t)init->coreCtrl.invACMP0     << _LESENSE_CTRL_ACMP0INV_SHIFT)
    | ((uint32_t)init->coreCtrl.invACMP1     << _LESENSE_CTRL_ACMP1INV_SHIFT)
#endif
    | ((uint32_t)init->coreCtrl.dualSample   << _LESENSE_CTRL_DUALSAMPLE_SHIFT)
    | ((uint32_t)init->coreCtrl.storeScanRes << _LESENSE_CTRL_STRSCANRES_SHIFT)
    | ((uint32_t)init->coreCtrl.bufOverWr    << _LESENSE_CTRL_BUFOW_SHIFT)
    | ((uint32_t)init->coreCtrl.debugRun     << _LESENSE_CTRL_DEBUGRUN_SHIFT);

  /* Set scan mode in the CTRL register using the provided function, don't
   * start scanning immediately. */
  LESENSE_ScanModeSet((LESENSE_ScanMode_TypeDef)init->coreCtrl.scanStart, false);

  /* LESENSE peripheral control configuration.
   * Set DAC0 and DAC1 data source, conversion mode, output mode. Set DAC
   * prescaler and reference. Set ACMP0 and ACMP1 control mode. Set ACMP and DAC
   * duty cycle (warm up) mode. */
  LESENSE->PERCTRL =
    ((uint32_t)init->perCtrl.dacCh0Data       << _LESENSE_PERCTRL_DACCH0DATA_SHIFT)
    | ((uint32_t)init->perCtrl.dacCh1Data     << _LESENSE_PERCTRL_DACCH1DATA_SHIFT)
#if defined(_LESENSE_PERCTRL_DACCH0CONV_MASK)
    | ((uint32_t)init->perCtrl.dacCh0ConvMode << _LESENSE_PERCTRL_DACCH0CONV_SHIFT)
    | ((uint32_t)init->perCtrl.dacCh0OutMode  << _LESENSE_PERCTRL_DACCH0OUT_SHIFT)
    | ((uint32_t)init->perCtrl.dacCh1ConvMode << _LESENSE_PERCTRL_DACCH1CONV_SHIFT)
    | ((uint32_t)init->perCtrl.dacCh1OutMode  << _LESENSE_PERCTRL_DACCH1OUT_SHIFT)
    | ((uint32_t)init->perCtrl.dacPresc       << _LESENSE_PERCTRL_DACPRESC_SHIFT)
    | (uint32_t)init->perCtrl.dacRef
#endif
    | ((uint32_t)init->perCtrl.acmp0Mode      << _LESENSE_PERCTRL_ACMP0MODE_SHIFT)
    | ((uint32_t)init->perCtrl.acmp1Mode      << _LESENSE_PERCTRL_ACMP1MODE_SHIFT)
#if defined(_LESENSE_PERCTRL_ACMP0INV_MASK)
    | ((uint32_t)init->coreCtrl.invACMP0      << _LESENSE_PERCTRL_ACMP0INV_SHIFT)
    | ((uint32_t)init->coreCtrl.invACMP1      << _LESENSE_PERCTRL_ACMP1INV_SHIFT)
#endif
#if defined(_LESENSE_PERCTRL_DACCONVTRIG_MASK)
    | ((uint32_t)init->perCtrl.dacScan        << _LESENSE_PERCTRL_DACCONVTRIG_SHIFT)
#endif
    | (uint32_t)init->perCtrl.warmupMode;

  /* LESENSE decoder general control configuration.
   * Set decoder input source, select PRS input for decoder bits.
   * Enable/disable the decoder to check the present state.
   * Enable/disable decoder to channel interrupt mapping.
   * Enable/disable decoder hysteresis on PRS output.
   * Enable/disable decoder hysteresis on count events.
   * Enable/disable decoder hysteresis on interrupt requests.
   * Enable/disable count mode on LESPRS0 and LESPRS1. */
  LESENSE->DECCTRL =
    (uint32_t)init->decCtrl.decInput
    | ((uint32_t)init->decCtrl.prsChSel0 << _LESENSE_DECCTRL_PRSSEL0_SHIFT)
    | ((uint32_t)init->decCtrl.prsChSel1 << _LESENSE_DECCTRL_PRSSEL1_SHIFT)
    | ((uint32_t)init->decCtrl.prsChSel2 << _LESENSE_DECCTRL_PRSSEL2_SHIFT)
    | ((uint32_t)init->decCtrl.prsChSel3 << _LESENSE_DECCTRL_PRSSEL3_SHIFT)
    | ((uint32_t)init->decCtrl.chkState  << _LESENSE_DECCTRL_ERRCHK_SHIFT)
    | ((uint32_t)init->decCtrl.intMap    << _LESENSE_DECCTRL_INTMAP_SHIFT)
    | ((uint32_t)init->decCtrl.hystPRS0  << _LESENSE_DECCTRL_HYSTPRS0_SHIFT)
    | ((uint32_t)init->decCtrl.hystPRS1  << _LESENSE_DECCTRL_HYSTPRS1_SHIFT)
    | ((uint32_t)init->decCtrl.hystPRS2  << _LESENSE_DECCTRL_HYSTPRS2_SHIFT)
    | ((uint32_t)init->decCtrl.hystIRQ   << _LESENSE_DECCTRL_HYSTIRQ_SHIFT)
    | ((uint32_t)init->decCtrl.prsCount  << _LESENSE_DECCTRL_PRSCNT_SHIFT);

  /* Set initial LESENSE decoder state. */
  LESENSE_DecoderStateSet((uint32_t)init->decCtrl.initState);

  /* LESENSE bias control configuration. */
  LESENSE->BIASCTRL = (uint32_t)init->coreCtrl.biasMode;
}


/***************************************************************************//**
 * @brief
 *   Set scan frequency for periodic scanning.
 *
 * @details
 *   This function only applies to LESENSE if period counter is being used as
 *   a trigger for scan start.
 *   The calculation is based on the following formula:
 *   Fscan = LFACLKles / ((1+PCTOP)*2^PCPRESC)
 *
 * @note
 *   Note that the calculation does not necessarily result in the requested
 *   scan frequency due to integer division. Check the return value for the
 *   resulted scan frequency.
 *
 * @param[in] refFreq
 *   Select reference LFACLK clock frequency in Hz. If set to 0, the current
 *   clock frequency is being used as a reference.
 *
 * @param[in] scanFreq
 *   Set the desired scan frequency in Hz.
 *
 * @return
 *   Frequency in Hz calculated and set by this function. Users can use this to
 *   compare the requested and set values.
 ******************************************************************************/
uint32_t LESENSE_ScanFreqSet(uint32_t refFreq, uint32_t scanFreq)
{
  uint32_t tmp;
  uint32_t pcPresc = 0UL;  /* Period counter prescaler. */
  uint32_t clkDiv  = 1UL;  /* Clock divisor value (2^pcPresc). */
  uint32_t pcTop   = 63UL; /* Period counter top value (max. 63). */
  uint32_t calcScanFreq;   /* Variable for testing the calculation algorithm. */


  /* If refFreq is set to 0, the currently configured reference clock is
   * assumed. */
  if (!refFreq)
  {
    refFreq = CMU_ClockFreqGet(cmuClock_LESENSE);
  }

  /* Max. value of pcPresc is 128, thus using reference frequency less than
   * 33554431Hz (33.554431MHz), the frequency calculation in the while loop
   * below will not overflow. */
  EFM_ASSERT(refFreq < ((uint32_t)UINT32_MAX / 128UL));

  /* Sanity check of scan frequency value. */
  EFM_ASSERT((scanFreq > 0U) && (scanFreq <= refFreq));

  /* Calculate the minimum necessary prescaler value in order to provide the
   * biggest possible resolution for setting scan frequency.
   * Maximum number of calculation cycles is 7 (value of lesenseClkDiv_128). */
  while ((refFreq / ((uint32_t)scanFreq * clkDiv) > (pcTop + 1UL))
         && (pcPresc < lesenseClkDiv_128))
  {
    ++pcPresc;
    clkDiv = (uint32_t)1UL << pcPresc;
  }

  /* Calculate pcTop value. */
  pcTop = ((uint32_t)refFreq / ((uint32_t)scanFreq * clkDiv)) - 1UL;

  /* Clear current PCPRESC and PCTOP settings. Be aware of the effect of
   * non-atomic Read-Modify-Write on LESENSE->TIMCRTL. */
  tmp = LESENSE->TIMCTRL & (~_LESENSE_TIMCTRL_PCPRESC_MASK
                            & ~_LESENSE_TIMCTRL_PCTOP_MASK);

  /* Set new values in tmp while reserving other settings. */
  tmp |= ((uint32_t)pcPresc << _LESENSE_TIMCTRL_PCPRESC_SHIFT)
         | ((uint32_t)pcTop << _LESENSE_TIMCTRL_PCTOP_SHIFT);

  /* Set values in LESENSE_TIMCTRL register. */
  LESENSE->TIMCTRL = tmp;

  /* For testing the calculation algorithm. */
  calcScanFreq = ((uint32_t)refFreq / ((uint32_t)(1UL + pcTop) * clkDiv));

  return calcScanFreq;
}


/***************************************************************************//**
 * @brief
 *   Set scan mode of the LESENSE channels.
 *
 * @details
 *   This function configures how the scan start is being triggered. It can be
 *   used for re-configuring the scan mode while running the application but it
 *   is also used by LESENSE_Init() for initialization.
 *
 * @note
 *   Users can configure the scan mode by LESENSE_Init() function, but only with
 *   a significant overhead. This simple function serves the purpose of
 *   controlling this parameter after the channel has been configured.
 *   Please be aware the effects of the non-atomic Read-Modify-Write cycle!
 *
 * @param[in] scanMode
 *   Select where to map LESENSE alternate excitation channels.
 *   @li lesenseScanStartPeriodic - New scan is started each time the period
 *                                  counter overflows.
 *   @li lesenseScanStartOneShot - Single scan is performed when
 *                                 LESENSE_ScanStart() is called.
 *   @li lesenseScanStartPRS - New scan is triggered by pulse on PRS channel.
 *
 * @param[in] start
 *   If true, LESENSE_ScanStart() is immediately issued after configuration.
 ******************************************************************************/
void LESENSE_ScanModeSet(LESENSE_ScanMode_TypeDef scanMode,
                         bool start)
{
  uint32_t tmp; /* temporary storage of the CTRL register value */


  /* Save the CTRL register value to tmp.
   * Please be aware the effects of the non-atomic Read-Modify-Write cycle! */
  tmp = LESENSE->CTRL & ~(_LESENSE_CTRL_SCANMODE_MASK);
  /* Setting the requested scanMode to the CTRL register. Casting signed int
   * (enum) to unsigned long (uint32_t). */
  tmp |= (uint32_t)scanMode;

  /* Write the new value to the CTRL register. */
  LESENSE->CTRL = tmp;

  /* Start sensor scanning if requested. */
  if (start)
  {
    LESENSE_ScanStart();
  }
}


/***************************************************************************//**
 * @brief
 *   Set start delay of sensor interaction on each channel.
 *
 * @details
 *   This function sets start delay of sensor interaction on each channel.
 *   It can be used for adjusting the start delay while running the application
 *   but it is also used by LESENSE_Init() for initialization.
 *
 * @note
 *   Users can configure the start delay by LESENSE_Init() function, but only
 *   with a significant overhead. This simple function serves the purpose of
 *   controlling this parameter after the channel has been configured.
 *   Please be aware the effects of the non-atomic Read-Modify-Write cycle!
 *
 * @param[in] startDelay
 *   Number of LFACLK cycles to delay. Valid range: 0-3 (2 bit).
 ******************************************************************************/
void LESENSE_StartDelaySet(uint8_t startDelay)
{
  uint32_t tmp; /* temporary storage of the TIMCTRL register value */


  /* Sanity check of startDelay. */
  EFM_ASSERT(startDelay < 4U);

  /* Save the TIMCTRL register value to tmp.
   * Please be aware the effects of the non-atomic Read-Modify-Write cycle! */
  tmp = LESENSE->TIMCTRL & ~(_LESENSE_TIMCTRL_STARTDLY_MASK);
  /* Setting the requested startDelay to the TIMCTRL register. */
  tmp |= (uint32_t)startDelay << _LESENSE_TIMCTRL_STARTDLY_SHIFT;

  /* Write the new value to the TIMCTRL register. */
  LESENSE->TIMCTRL = tmp;
}


/***************************************************************************//**
 * @brief
 *   Set clock division for LESENSE timers.
 *
 * @details
 *   Use this function to configure the clock division for the LESENSE timers
 *   used for excitation timing.
 *   The division setting is global, but the clock source can be selected for
 *   each channel using LESENSE_ChannelConfig() function, please refer to the
 *   documentation of it for more details.
 *
 * @note
 *   If AUXHFRCO is used for excitation timing, LFACLK can not exceed 500kHz.
 *   LFACLK can not exceed 50kHz if the ACMP threshold level (ACMPTHRES) is not
 *   equal for all channels.
 *
 * @param[in] clk
 *   Select clock to prescale.
 *    @li lesenseClkHF - set AUXHFRCO clock divisor for HF timer.
 *    @li lesenseClkLF - set LFACLKles clock divisor for LF timer.
 *
 * @param[in] clkDiv
 *   Clock divisor value. Valid range depends on the @p clk value.
 ******************************************************************************/
void LESENSE_ClkDivSet(LESENSE_ChClk_TypeDef clk,
                       LESENSE_ClkPresc_TypeDef clkDiv)
{
  uint32_t tmp;


  /* Select clock to prescale */
  switch (clk)
  {
    case lesenseClkHF:
      /* Sanity check of clock divisor for HF clock. */
      EFM_ASSERT((uint32_t)clkDiv <= lesenseClkDiv_8);

      /* Clear current AUXPRESC settings. */
      tmp = LESENSE->TIMCTRL & ~(_LESENSE_TIMCTRL_AUXPRESC_MASK);

      /* Set new values in tmp while reserving other settings. */
      tmp |= ((uint32_t)clkDiv << _LESENSE_TIMCTRL_AUXPRESC_SHIFT);

      /* Set values in LESENSE_TIMCTRL register. */
      LESENSE->TIMCTRL = tmp;
      break;

    case lesenseClkLF:
      /* Clear current LFPRESC settings. */
      tmp = LESENSE->TIMCTRL & ~(_LESENSE_TIMCTRL_LFPRESC_MASK);

      /* Set new values in tmp while reserving other settings. */
      tmp |= ((uint32_t)clkDiv << _LESENSE_TIMCTRL_LFPRESC_SHIFT);

      /* Set values in LESENSE_TIMCTRL register. */
      LESENSE->TIMCTRL = tmp;
      break;

    default:
      EFM_ASSERT(0);
      break;
  }
}


/***************************************************************************//**
 * @brief
 *   Configure all (16) LESENSE sensor channels.
 *
 * @details
 *   This function configures all the sensor channels of LESENSE interface.
 *   Please refer to the configuration parameter type definition
 *   (LESENSE_ChAll_TypeDef) for more details.
 *
 * @note
 *   Channels can be configured individually using LESENSE_ChannelConfig()
 *   function.
 *   Notice that pins used by the LESENSE module must be properly configured
 *   by the user explicitly, in order for the LESENSE to work as intended.
 *   (When configuring pins, one should remember to consider the sequence of
 *   configuration, in order to avoid unintended pulses/glitches on output
 *   pins.)
 *
 * @param[in] confChAll
 *   Configuration structure for all (16) LESENSE sensor channels.
 ******************************************************************************/
void LESENSE_ChannelAllConfig(const LESENSE_ChAll_TypeDef * confChAll)
{
  uint32_t i;

  /* Iterate through all the 16 channels */
  for (i = 0U; i < LESENSE_NUM_CHANNELS; ++i)
  {
    /* Configure scan channels. */
    LESENSE_ChannelConfig(&confChAll->Ch[i], i);
  }
}


/***************************************************************************//**
 * @brief
 *   Configure a single LESENSE sensor channel.
 *
 * @details
 *   This function configures a single sensor channel of the LESENSE interface.
 *   Please refer to the configuration parameter type definition
 *   (LESENSE_ChDesc_TypeDef) for more details.
 *
 * @note
 *   This function has been designed to minimize the effects of sensor channel
 *   reconfiguration while LESENSE is in operation, however one shall be aware
 *   of these effects and the right timing of calling this function.
 *   Parameter @p useAltEx must be true in the channel configuration in order to
 *   use alternate excitation pins.
 *
 * @param[in] confCh
 *   Configuration structure for a single LESENSE sensor channel.
 *
 * @param[in] chIdx
 *   Channel index to configure (0-15).
 ******************************************************************************/
void LESENSE_ChannelConfig(const LESENSE_ChDesc_TypeDef * confCh,
                           uint32_t chIdx)
{
  uint32_t tmp; /* Service variable. */


  /* Sanity check of configuration parameters */
  EFM_ASSERT(chIdx < LESENSE_NUM_CHANNELS);
  EFM_ASSERT(confCh->exTime      <= (_LESENSE_CH_TIMING_EXTIME_MASK >> _LESENSE_CH_TIMING_EXTIME_SHIFT));
  EFM_ASSERT(confCh->measDelay   <= (_LESENSE_CH_TIMING_MEASUREDLY_MASK >> _LESENSE_CH_TIMING_MEASUREDLY_SHIFT));
#if defined(_SILICON_LABS_32B_SERIES_0)
  // Sample delay on other devices are 8 bits which fits perfectly in uint8_t
  EFM_ASSERT(confCh->sampleDelay <= (_LESENSE_CH_TIMING_SAMPLEDLY_MASK >> _LESENSE_CH_TIMING_SAMPLEDLY_SHIFT));
#endif

  /* Not a complete assert, as the max. value of acmpThres depends on other
   * configuration parameters, check the parameter description of acmpThres for
   * for more details! */
  EFM_ASSERT(confCh->acmpThres < 4096U);
  if (confCh->chPinExMode == lesenseChPinExDACOut)
  {
    EFM_ASSERT((0x1 << chIdx) & DACOUT_SUPPORT);
  }

#if defined(_LESENSE_IDLECONF_CH0_DACCH0)
  EFM_ASSERT(!(confCh->chPinIdleMode == lesenseChPinIdleDACCh1
               && ((chIdx != 12U)
                   && (chIdx != 13U)
                   && (chIdx != 14U)
                   && (chIdx != 15U))));
  EFM_ASSERT(!(confCh->chPinIdleMode == lesenseChPinIdleDACCh0
               && ((chIdx != 0U)
                   && (chIdx != 1U)
                   && (chIdx != 2U)
                   && (chIdx != 3U))));
#endif

  /* Configure chIdx setup in LESENSE idle phase.
   * Read-modify-write in order to support reconfiguration during LESENSE
   * operation. */
  tmp               = (LESENSE->IDLECONF & ~((uint32_t)0x3UL << (chIdx * 2UL)));
  tmp              |= ((uint32_t)confCh->chPinIdleMode << (chIdx * 2UL));
  LESENSE->IDLECONF = tmp;

  /* Channel specific timing configuration on scan channel chIdx.
   * Set excitation time, sampling delay, measurement delay. */
  LESENSE_ChannelTimingSet(chIdx,
                           confCh->exTime,
                           confCh->sampleDelay,
                           confCh->measDelay);

  /* Channel specific configuration of clocks, sample mode, excitation pin mode
   * alternate excitation usage and interrupt mode on scan channel chIdx in
   * LESENSE_CHchIdx_INTERACT. */
  LESENSE->CH[chIdx].INTERACT =
        ((uint32_t)confCh->exClk       << _LESENSE_CH_INTERACT_EXCLK_SHIFT)
        | ((uint32_t)confCh->sampleClk << _LESENSE_CH_INTERACT_SAMPLECLK_SHIFT)
        | (uint32_t)confCh->sampleMode
        | (uint32_t)confCh->intMode
        | (uint32_t)confCh->chPinExMode
        | ((uint32_t)confCh->useAltEx  << _LESENSE_CH_INTERACT_ALTEX_SHIFT);

  /* Configure channel specific counter comparison mode, optional result
   * forwarding to decoder, optional counter value storing and optional result
   * inverting on scan channel chIdx in LESENSE_CHchIdx_EVAL. */
  LESENSE->CH[chIdx].EVAL =
        (uint32_t)confCh->compMode
        | ((uint32_t)confCh->shiftRes    << _LESENSE_CH_EVAL_DECODE_SHIFT)
        | ((uint32_t)confCh->storeCntRes << _LESENSE_CH_EVAL_STRSAMPLE_SHIFT)
        | ((uint32_t)confCh->invRes      << _LESENSE_CH_EVAL_SCANRESINV_SHIFT)
#if defined(_LESENSE_CH_EVAL_MODE_MASK)
        | ((uint32_t)confCh->evalMode    << _LESENSE_CH_EVAL_MODE_SHIFT)
#endif
        ;

  /* Configure analog comparator (ACMP) threshold and decision threshold for
   * counter separately with the function provided for that. */
  LESENSE_ChannelThresSet(chIdx,
                          confCh->acmpThres,
                          confCh->cntThres);

  /* Enable/disable interrupts on channel */
  BUS_RegBitWrite(&LESENSE->IEN, chIdx, confCh->enaInt);

  /* Enable/disable CHchIdx pin. */
  BUS_RegBitWrite(&GENERIC_LESENSE_ROUTE, chIdx, confCh->enaPin);

  /* Enable/disable scan channel chIdx. */
  BUS_RegBitWrite(&LESENSE->CHEN, chIdx, confCh->enaScanCh);
}


/***************************************************************************//**
 * @brief
 *   Configure the LESENSE alternate excitation modes.
 *
 * @details
 *   This function configures the alternate excitation channels of the LESENSE
 *   interface. Please refer to the configuration parameter type definition
 *   (LESENSE_ConfAltEx_TypeDef) for more details.
 *
 * @note
 *   Parameter @p useAltEx must be true in the channel configuration structrure
 *   (LESENSE_ChDesc_TypeDef) in order to use alternate excitation pins on the
 *   channel.
 *
 * @param[in] confAltEx
 *   Configuration structure for LESENSE alternate excitation pins.
 ******************************************************************************/
void LESENSE_AltExConfig(const LESENSE_ConfAltEx_TypeDef * confAltEx)
{
  uint32_t i;
  uint32_t tmp;


  /* Configure alternate excitation mapping.
   * Atomic read-modify-write using BUS_RegBitWrite function in order to
   * support reconfiguration during LESENSE operation. */
  BUS_RegBitWrite(&LESENSE->CTRL,
                  _LESENSE_CTRL_ALTEXMAP_SHIFT,
                  confAltEx->altExMap);

  switch (confAltEx->altExMap)
  {
    case lesenseAltExMapALTEX:
      /* Iterate through the 8 possible alternate excitation pin descriptors. */
      for (i = 0U; i < 8U; ++i)
      {
        /* Enable/disable alternate excitation pin i.
         * Atomic read-modify-write using BUS_RegBitWrite function in order to
         * support reconfiguration during LESENSE operation. */
        BUS_RegBitWrite(&GENERIC_LESENSE_ROUTE,
                        (16UL + i),
                        confAltEx->AltEx[i].enablePin);

        /* Setup the idle phase state of alternate excitation pin i.
         * Read-modify-write in order to support reconfiguration during LESENSE
         * operation. */
        tmp                = (LESENSE->ALTEXCONF & ~((uint32_t)0x3UL << (i * 2UL)));
        tmp               |= ((uint32_t)confAltEx->AltEx[i].idleConf << (i * 2UL));
        LESENSE->ALTEXCONF = tmp;

        /* Enable/disable always excite on channel i */
        BUS_RegBitWrite(&LESENSE->ALTEXCONF,
                        (16UL + i),
                        confAltEx->AltEx[i].alwaysEx);
      }
      break;

#if defined(_LESENSE_CTRL_ALTEXMAP_ACMP)
    case lesenseAltExMapACMP:
#else
    case lesenseAltExMapCH:
#endif
      /* Iterate through all the 16 alternate excitation channels */
      for (i = 0U; i < 16U; ++i)
      {
        /* Enable/disable alternate ACMP excitation channel pin i. */
        /* Atomic read-modify-write using BUS_RegBitWrite function in order to
         * support reconfiguration during LESENSE operation. */
        BUS_RegBitWrite(&GENERIC_LESENSE_ROUTE,
                        i,
                        confAltEx->AltEx[i].enablePin);
      }
      break;
    default:
      /* Illegal value. */
      EFM_ASSERT(0);
      break;
  }
}


/***************************************************************************//**
 * @brief
 *   Enable/disable LESENSE scan channel and the pin assigned to it.
 *
 * @details
 *   Use this function to enable/disable a selected LESENSE scan channel and the
 *   pin assigned to.
 *
 * @note
 *   Users can enable/disable scan channels and the channel pin by
 *   LESENSE_ChannelConfig() function, but only with a significant overhead.
 *   This simple function serves the purpose of controlling these parameters
 *   after the channel has been configured.
 *
 * @param[in] chIdx
 *   Identifier of the scan channel. Valid range: 0-15.
 *
 * @param[in] enaScanCh
 *   Enable/disable the selected scan channel by setting this parameter to
 *   true/false respectively.
 *
 * @param[in] enaPin
 *   Enable/disable the pin assigned to the channel selected by @p chIdx.
 ******************************************************************************/
void LESENSE_ChannelEnable(uint8_t chIdx,
                           bool enaScanCh,
                           bool enaPin)
{
  /* Enable/disable the assigned pin of scan channel chIdx.
   * Note: BUS_RegBitWrite() function is used for setting/clearing single
   * bit peripheral register bitfields. Read the function description in
   * em_bus.h for more details. */
  BUS_RegBitWrite(&GENERIC_LESENSE_ROUTE, chIdx, enaPin);

  /* Enable/disable scan channel chIdx. */
  BUS_RegBitWrite(&LESENSE->CHEN, chIdx, enaScanCh);
}


/***************************************************************************//**
 * @brief
 *   Enable/disable LESENSE scan channel and the pin assigned to it.
 *
 * @details
 *   Use this function to enable/disable LESENSE scan channels and the pins
 *   assigned to them using a mask.
 *
 * @note
 *   Users can enable/disable scan channels and channel pins by using
 *   LESENSE_ChannelAllConfig() function, but only with a significant overhead.
 *   This simple function serves the purpose of controlling these parameters
 *   after the channel has been configured.
 *
 * @param[in] chMask
 *   Set the corresponding bit to 1 to enable, 0 to disable the selected scan
 *   channel.
 *
 * @param[in] pinMask
 *   Set the corresponding bit to 1 to enable, 0 to disable the pin on selected
 *   channel.
 ******************************************************************************/
void LESENSE_ChannelEnableMask(uint16_t chMask, uint16_t pinMask)
{
  /* Enable/disable all channels at once according to the mask. */
  LESENSE->CHEN = chMask;
  /* Enable/disable all channel pins at once according to the mask. */
  GENERIC_LESENSE_ROUTE = pinMask;
}


/***************************************************************************//**
 * @brief
 *   Set LESENSE channel timing parameters.
 *
 * @details
 *   Use this function to set timing parameters on a selected LESENSE channel.
 *
 * @note
 *   Users can configure the channel timing parameters by
 *   LESENSE_ChannelConfig() function, but only with a significant overhead.
 *   This simple function serves the purpose of controlling these parameters
 *   after the channel has been configured.
 *
 * @param[in] chIdx
 *   Identifier of the scan channel. Valid range: 0-15.
 *
 * @param[in] exTime
 *   Excitation time on chIdx. Excitation will last exTime+1 excitation clock
 *   cycles. Valid range: 0-63 (6 bits).
 *
 * @param[in] sampleDelay
 *   Sample delay on chIdx. Sampling will occur after sampleDelay+1 sample clock
 *   cycles. Valid range: 0-127 (7 bits).
 *
 * @param[in] measDelay
 *   Measure delay on chIdx. Sensor measuring is delayed for measDelay+1
 *   excitation clock cycles. Valid range: 0-127 (7 bits).
 ******************************************************************************/
void LESENSE_ChannelTimingSet(uint8_t chIdx,
                              uint8_t exTime,
                              uint8_t sampleDelay,
                              uint16_t measDelay)
{
  /* Sanity check of parameters. */
  EFM_ASSERT(exTime      <= (_LESENSE_CH_TIMING_EXTIME_MASK >> _LESENSE_CH_TIMING_EXTIME_SHIFT));
  EFM_ASSERT(measDelay   <= (_LESENSE_CH_TIMING_MEASUREDLY_MASK >> _LESENSE_CH_TIMING_MEASUREDLY_SHIFT));
#if defined(_SILICON_LABS_32B_SERIES_0)
  // Sample delay on other devices are 8 bits which fits perfectly in uint8_t
  EFM_ASSERT(sampleDelay <= (_LESENSE_CH_TIMING_SAMPLEDLY_MASK >> _LESENSE_CH_TIMING_SAMPLEDLY_SHIFT));
#endif

  /* Channel specific timing configuration on scan channel chIdx.
   * Setting excitation time, sampling delay, measurement delay. */
  LESENSE->CH[chIdx].TIMING =
              ((uint32_t)exTime        << _LESENSE_CH_TIMING_EXTIME_SHIFT)
              | ((uint32_t)sampleDelay << _LESENSE_CH_TIMING_SAMPLEDLY_SHIFT)
              | ((uint32_t)measDelay   << _LESENSE_CH_TIMING_MEASUREDLY_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Set LESENSE channel threshold parameters.
 *
 * @details
 *   Use this function to set threshold parameters on a selected LESENSE
 *   channel.
 *
 * @note
 *   Users can configure the channel threshold parameters by
 *   LESENSE_ChannelConfig() function, but only with a significant overhead.
 *   This simple function serves the purpose of controlling these parameters
 *   after the channel has been configured.
 *
 * @param[in] chIdx
 *   Identifier of the scan channel. Valid range: 0-15.
 *
 * @param[in] acmpThres
 *   ACMP threshold.
 *   @li If perCtrl.dacCh0Data or perCtrl.dacCh1Data is set to
 *   #lesenseDACIfData, acmpThres defines the 12-bit DAC data in the
 *   corresponding data register of the DAC interface (DACn_CH0DATA and
 *   DACn_CH1DATA). In this case, the valid range is: 0-4095 (12 bits).
 *
 *   @li If perCtrl.dacCh0Data or perCtrl.dacCh1Data is set to
 *   #lesenseACMPThres, acmpThres defines the 6-bit Vdd scaling factor of ACMP
 *   negative input (VDDLEVEL in ACMP_INPUTSEL register). In this case, the
 *   valid range is: 0-63 (6 bits).
 *
 * @param[in] cntThres
 *   Decision threshold for counter comparison.
 *   Valid range: 0-65535 (16 bits).
 ******************************************************************************/
void LESENSE_ChannelThresSet(uint8_t chIdx,
                             uint16_t acmpThres,
                             uint16_t cntThres)
{
  uint32_t tmp; /* temporary storage */


  /* Sanity check for acmpThres only, cntThres is 16bit value. */
  EFM_ASSERT(acmpThres < 4096U);
  /* Sanity check for LESENSE channel id. */
  EFM_ASSERT(chIdx < LESENSE_NUM_CHANNELS);

  /* Save the INTERACT register value of channel chIdx to tmp.
   * Please be aware the effects of the non-atomic Read-Modify-Write cycle! */
  tmp = LESENSE->CH[chIdx].INTERACT & ~(0xFFF);
  /* Set the ACMP threshold value to the INTERACT register of channel chIdx. */
  tmp |= (uint32_t)acmpThres;
  /* Write the new value to the INTERACT register. */
  LESENSE->CH[chIdx].INTERACT = tmp;

  /* Save the EVAL register value of channel chIdx to tmp.
   * Please be aware the effects of the non-atomic Read-Modify-Write cycle! */
  tmp = LESENSE->CH[chIdx].EVAL & ~(_LESENSE_CH_EVAL_COMPTHRES_MASK);
  /* Set the counter threshold value to the INTERACT register of channel chIdx. */
  tmp |= (uint32_t)cntThres << _LESENSE_CH_EVAL_COMPTHRES_SHIFT;
  /* Write the new value to the EVAL register. */
  LESENSE->CH[chIdx].EVAL = tmp;
}

#if defined(_LESENSE_CH_EVAL_MODE_MASK)
/***************************************************************************//**
 * @brief
 *   Configure Sliding Window evaluation mode for a specific channel
 *
 * @details
 *   This function will configure the evaluation mode, the initial
 *   sensor measurement (COMPTHRES) and the window size. For other channel
 *   related configuration see the @ref LESENSE_ChannelConfig() function.
 *
 * @warning
 *   Beware that the step size and window size configuration are global to all
 *   LESENSE channels and use the same register field in the hardware. This
 *   means that any windowSize configuration passed to this function will
 *   apply for all channels and override all other stepSize/windowSize
 *   configurations.
 *
 * @param[in] chIdx
 *   Identifier of the scan channel. Valid range: 0-15.
 *
 * @param[in] windowSize
 *   Window size to be used on all channels.
 *
 * @param[in] initValue
 *   The initial sensor value for the channel.
 ******************************************************************************/
void LESENSE_ChannelSlidingWindow(uint8_t chIdx,
                                  uint32_t windowSize,
                                  uint32_t initValue)
{
  LESENSE_CH_TypeDef * ch = &LESENSE->CH[chIdx];

  LESENSE_WindowSizeSet(windowSize);
  ch->EVAL = (ch->EVAL & ~(_LESENSE_CH_EVAL_COMPTHRES_MASK | _LESENSE_CH_EVAL_MODE_MASK))
             | (initValue << _LESENSE_CH_EVAL_COMPTHRES_SHIFT)
             | LESENSE_CH_EVAL_MODE_SLIDINGWIN;
}

/***************************************************************************//**
 * @brief
 *   Configure step detection evaluation mode for a specific channel
 *
 * @details
 *   This function will configure the evaluation mode, the initial
 *   sensor measurement (COMPTHRES) and the window size. For other channel
 *   related configuration see the @ref LESENSE_ChannelConfig() function.
 *
 * @warning
 *   Beware that the step size and window size configuration are global to all
 *   LESENSE channels and use the same register field in the hardware. This
 *   means that any stepSize configuration passed to this function will
 *   apply for all channels and override all other stepSize/windowSize
 *   configurations.
 *
 * @param[in] chIdx
 *   Identifier of the scan channel. Valid range: 0-15.
 *
 * @param[in] stepSize
 *   Step size to be used on all channels.
 *
 * @param[in] initValue
 *   The initial sensor value for the channel.
 ******************************************************************************/
void LESENSE_ChannelStepDetection(uint8_t chIdx,
                                  uint32_t stepSize,
                                  uint32_t initValue)
{
  LESENSE_CH_TypeDef * ch = &LESENSE->CH[chIdx];

  LESENSE_StepSizeSet(stepSize);
  ch->EVAL = (ch->EVAL & ~(_LESENSE_CH_EVAL_COMPTHRES_MASK | _LESENSE_CH_EVAL_MODE_MASK))
             | (initValue << _LESENSE_CH_EVAL_COMPTHRES_SHIFT)
             | LESENSE_CH_EVAL_MODE_STEPDET;
}

/***************************************************************************//**
 * @brief
 *   Set the window size for all LESENSE channels.
 *
 * @details
 *   The window size is used by all channels that are configured as
 *   @ref lesenseEvalModeSlidingWindow.
 *
 * @warning
 *   The window size configuration is using the same register field as the
 *   step detection size. So the window size configuration will have an
 *   effect on channels configured with the @ref lesenseEvalModeStepDetection
 *   evaluation mode as well.
 *
 * @param[in] windowSize
 *   The window size to use for all channels.
 ******************************************************************************/
void LESENSE_WindowSizeSet(uint32_t windowSize)
{
  LESENSE->EVALCTRL = (LESENSE->EVALCTRL & ~_LESENSE_EVALCTRL_WINSIZE_MASK)
                      | windowSize;
}

/***************************************************************************//**
 * @brief
 *   Set the step size for all LESENSE channels.
 *
 * @details
 *   The step size is configured using the same register field as use to
 *   configure window size. So calling this function will overwrite any
 *   previously configured window size as done by the
 *   @ref LESENSE_WindowSizeSet() function.
 *
 * @param[in] stepSize
 *   The step size to use for all channels.
 ******************************************************************************/
void LESENSE_StepSizeSet(uint32_t stepSize)
{
  LESENSE_WindowSizeSet(stepSize);
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure all LESENSE decoder states.
 *
 * @details
 *   This function configures all the decoder states of the LESENSE interface.
 *   Please refer to the configuration parameter type definition
 *   (LESENSE_DecStAll_TypeDef) for more details.
 *
 * @note
 *   Decoder states can be configured individually using
 *   LESENSE_DecoderStateConfig() function.
 *
 * @param[in] confDecStAll
 *   Configuration structure for all (16 or 32) LESENSE decoder states.
 ******************************************************************************/
void LESENSE_DecoderStateAllConfig(const LESENSE_DecStAll_TypeDef * confDecStAll)
{
  uint32_t i;

  /* Iterate through all the 16 or 32 decoder states. */
  for (i = 0U; i < LESENSE_NUM_DECODER_STATES; ++i)
  {
    /* Configure decoder state i. */
    LESENSE_DecoderStateConfig(&confDecStAll->St[i], i);
  }
}


/***************************************************************************//**
 * @brief
 *   Configure a single LESENSE decoder state.
 *
 * @details
 *   This function configures a single decoder state of the LESENSE interface.
 *   Please refer to the configuration parameter type definition
 *   (LESENSE_DecStDesc_TypeDef) for more details.
 *
 * @param[in] confDecSt
 *   Configuration structure for a single LESENSE decoder state.
 *
 * @param[in] decSt
 *   Decoder state index to configure (0-15) or (0-31) depending on device.
 ******************************************************************************/
void LESENSE_DecoderStateConfig(const LESENSE_DecStDesc_TypeDef * confDecSt,
                                uint32_t decSt)
{
  /* Sanity check of configuration parameters */
  EFM_ASSERT(decSt < LESENSE_NUM_DECODER_STATES);
  EFM_ASSERT((uint32_t)confDecSt->confA.compMask < 16U);
  EFM_ASSERT((uint32_t)confDecSt->confA.compVal < 16U);
  EFM_ASSERT((uint32_t)confDecSt->confA.nextState < LESENSE_NUM_DECODER_STATES);
  EFM_ASSERT((uint32_t)confDecSt->confB.compMask < 16U);
  EFM_ASSERT((uint32_t)confDecSt->confB.compVal < 16U);
  EFM_ASSERT((uint32_t)confDecSt->confB.nextState < LESENSE_NUM_DECODER_STATES);

  /* Configure state descriptor A (LESENSE_STi_TCONFA) for decoder state i.
   * Setting sensor compare value, sensor mask, next state index,
   * transition action, interrupt flag option and state descriptor chaining
   * configurations. */
  LESENSE->ST[decSt].TCONFA =
    (uint32_t)confDecSt->confA.prsAct
    | ((uint32_t)confDecSt->confA.compMask  << _LESENSE_ST_TCONFA_MASK_SHIFT)
    | ((uint32_t)confDecSt->confA.compVal   << _LESENSE_ST_TCONFA_COMP_SHIFT)
    | ((uint32_t)confDecSt->confA.nextState << _LESENSE_ST_TCONFA_NEXTSTATE_SHIFT)
    | ((uint32_t)confDecSt->confA.setInt    << _LESENSE_ST_TCONFA_SETIF_SHIFT)
    | ((uint32_t)confDecSt->chainDesc       << _LESENSE_ST_TCONFA_CHAIN_SHIFT);

  /* Configure state descriptor Bi (LESENSE_STi_TCONFB).
   * Setting sensor compare value, sensor mask, next state index, transition
   * action and interrupt flag option configurations. */
  LESENSE->ST[decSt].TCONFB =
  (uint32_t)confDecSt->confB.prsAct
    | ((uint32_t)confDecSt->confB.compMask  << _LESENSE_ST_TCONFB_MASK_SHIFT)
    | ((uint32_t)confDecSt->confB.compVal   << _LESENSE_ST_TCONFB_COMP_SHIFT)
    | ((uint32_t)confDecSt->confB.nextState << _LESENSE_ST_TCONFB_NEXTSTATE_SHIFT)
    | ((uint32_t)confDecSt->confB.setInt    << _LESENSE_ST_TCONFB_SETIF_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Set LESENSE decoder state.
 *
 * @details
 *   This function can be used for setting the initial state of the LESENSE
 *   decoder.
 *
 * @note
 *   Make sure the LESENSE decoder state is initialized by this function before
 *   enabling the decoder!
 *
 * @param[in] decSt
 *   Decoder state to set as current state. Valid range: 0-15 or 0-31
 *   depending on device.
 ******************************************************************************/
void LESENSE_DecoderStateSet(uint32_t decSt)
{
  EFM_ASSERT(decSt <= _LESENSE_DECSTATE_DECSTATE_MASK);

  LESENSE->DECSTATE = decSt & _LESENSE_DECSTATE_DECSTATE_MASK;
}


/***************************************************************************//**
 * @brief
 *   Get the current state of the LESENSE decoder.
 *
 * @return
 *   This function returns the value of LESENSE_DECSTATE register that
 *   represents the current state of the LESENSE decoder.
 ******************************************************************************/
uint32_t LESENSE_DecoderStateGet(void)
{
  return LESENSE->DECSTATE & _LESENSE_DECSTATE_DECSTATE_MASK;
}

#if defined(_LESENSE_PRSCTRL_MASK)
/***************************************************************************//**
 * @brief
 *   Enable or disable PRS output from the LESENSE decoder.
 *
 * @param[in] enable
 *   enable/disable the PRS output from the LESENSE decoder. true to enable and
 *   false to disable.
 *
 * @param[in] decMask
 *   Decoder state compare value mask
 *
 * @param[in] decVal
 *   Decoder state compare value.
 ******************************************************************************/
void LESENSE_DecoderPrsOut(bool enable, uint32_t decMask, uint32_t decVal)
{
  LESENSE->PRSCTRL = (enable << _LESENSE_PRSCTRL_DECCMPEN_SHIFT)
                     | (decMask << _LESENSE_PRSCTRL_DECCMPMASK_SHIFT)
                     | (decVal << _LESENSE_PRSCTRL_DECCMPVAL_SHIFT);
}
#endif

/***************************************************************************//**
 * @brief
 *   Start scanning of sensors.
 *
 * @note
 *   This function will wait for any pending previous write operation to the
 *   CMD register to complete before accessing the CMD register. It will also
 *   wait for the write operation to the CMD register to complete before
 *   returning. Each write operation to the CMD register may take up to 3 LF
 *   clock cycles, so the user should expect some delay. The user may implement
 *   a separate function to write multiple command bits in the CMD register
 *   in one single operation in order to optimize an application.
 ******************************************************************************/
void LESENSE_ScanStart(void)
{
  /* Wait for any pending previous write operation to the CMD register to
     complete before accessing the CMD register. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;

  /* Start scanning of sensors */
  LESENSE->CMD = LESENSE_CMD_START;

  /* Wait for the write operation to the CMD register to complete before
     returning. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;
}


/***************************************************************************//**
 * @brief
 *   Stop scanning of sensors.
 *
 * @note
 *   This function will wait for any pending previous write operation to the
 *   CMD register to complete before accessing the CMD register. It will also
 *   wait for the write operation to the CMD register to complete before
 *   returning. Each write operation to the CMD register may take up to 3 LF
 *   clock cycles, so the user should expect some delay. The user may implement
 *   a separate function to write multiple command bits in the CMD register
 *   in one single operation in order to optimize an application.
 *
 * @note
 *   If issued during a scan, the command takes effect after scan completion.
 ******************************************************************************/
void LESENSE_ScanStop(void)
{
  /* Wait for any pending previous write operation to the CMD register to
     complete before accessing the CMD register. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;

  /* Stop scanning of sensors */
  LESENSE->CMD = LESENSE_CMD_STOP;

  /* Wait for the write operation to the CMD register to complete before
     returning. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;
}


/***************************************************************************//**
 * @brief
 *   Start LESENSE decoder.
 *
 * @note
 *   This function will wait for any pending previous write operation to the
 *   CMD register to complete before accessing the CMD register. It will also
 *   wait for the write operation to the CMD register to complete before
 *   returning. Each write operation to the CMD register may take up to 3 LF
 *   clock cycles, so the user should expect some delay. The user may implement
 *   a separate function to write multiple command bits in the CMD register
 *   in one single operation in order to optimize an application.
 ******************************************************************************/
void LESENSE_DecoderStart(void)
{
  /* Wait for any pending previous write operation to the CMD register to
     complete before accessing the CMD register. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;

  /* Start decoder */
  LESENSE->CMD = LESENSE_CMD_DECODE;

  /* Wait for the write operation to the CMD register to complete before
     returning. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;
}


/***************************************************************************//**
 * @brief
 *   Clear result buffer.
 *
 * @note
 *   This function will wait for any pending previous write operation to the
 *   CMD register to complete before accessing the CMD register. It will also
 *   wait for the write operation to the CMD register to complete before
 *   returning. Each write operation to the CMD register may take up to 3 LF
 *   clock cycles, so the user should expect some delay. The user may implement
 *   a separate function to write multiple command bits in the CMD register
 *   in one single operation in order to optimize an application.
 ******************************************************************************/
void LESENSE_ResultBufferClear(void)
{
  /* Wait for any pending previous write operation to the CMD register to
     complete before accessing the CMD register. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;

  LESENSE->CMD = LESENSE_CMD_CLEARBUF;

  /* Wait for the write operation to the CMD register to complete before
     returning. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;
}


/***************************************************************************//**
 * @brief
 *   Reset the LESENSE module.
 *
 * @details
 *   Use this function to reset the LESENSE registers.
 *
 * @note
 *   Resetting LESENSE registers is required in each reset or power-on cycle in
 *   order to configure the default values of the RAM mapped LESENSE registers.
 *   LESENSE_Reset() can be called on initialization by setting the @p reqReset
 *   parameter to true in LESENSE_Init().
 ******************************************************************************/
void LESENSE_Reset(void)
{
  uint32_t i;

  /* Disable all LESENSE interrupts first */
  LESENSE->IEN = _LESENSE_IEN_RESETVALUE;

  /* Clear all pending LESENSE interrupts */
  LESENSE->IFC = _LESENSE_IFC_MASK;

  /* Stop the decoder */
  LESENSE->DECCTRL |= LESENSE_DECCTRL_DISABLE;

  /* Wait for any pending previous write operation to the CMD register to
     complete before accessing the CMD register. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;

  /* Stop sensor scan and clear result buffer */
  LESENSE->CMD = (LESENSE_CMD_STOP | LESENSE_CMD_CLEARBUF);

  /* Reset LESENSE configuration registers */
  LESENSE->CTRL      = _LESENSE_CTRL_RESETVALUE;
  LESENSE->PERCTRL   = _LESENSE_PERCTRL_RESETVALUE;
  LESENSE->DECCTRL   = _LESENSE_DECCTRL_RESETVALUE;
  LESENSE->BIASCTRL  = _LESENSE_BIASCTRL_RESETVALUE;
#if defined(_LESENSE_EVALCTRL_MASK)
  LESENSE->EVALCTRL  = _LESENSE_EVALCTRL_RESETVALUE;
  LESENSE->PRSCTRL   = _LESENSE_PRSCTRL_RESETVALUE;
#endif
  LESENSE->CHEN      = _LESENSE_CHEN_RESETVALUE;
  LESENSE->IDLECONF  = _LESENSE_IDLECONF_RESETVALUE;
  LESENSE->ALTEXCONF = _LESENSE_ALTEXCONF_RESETVALUE;

  /* Disable LESENSE to control GPIO pins */
#if defined(_LESENSE_ROUTE_MASK)
  LESENSE->ROUTE    = _LESENSE_ROUTE_RESETVALUE;
#else
  LESENSE->ROUTEPEN = _LESENSE_ROUTEPEN_RESETVALUE;
#endif

  /* Reset all channel configuration registers */
  for (i = 0U; i < LESENSE_NUM_CHANNELS; ++i)
  {
    LESENSE->CH[i].TIMING   = _LESENSE_CH_TIMING_RESETVALUE;
    LESENSE->CH[i].INTERACT = _LESENSE_CH_INTERACT_RESETVALUE;
    LESENSE->CH[i].EVAL     = _LESENSE_CH_EVAL_RESETVALUE;
  }

  /* Reset all decoder state configuration registers */
  for (i = 0U; i < LESENSE_NUM_DECODER_STATES; ++i)
  {
    LESENSE->ST[i].TCONFA = _LESENSE_ST_TCONFA_RESETVALUE;
    LESENSE->ST[i].TCONFB = _LESENSE_ST_TCONFB_RESETVALUE;
  }

  /* Wait for the write operation to the CMD register to complete before
     returning. */
  while (LESENSE_SYNCBUSY_CMD & LESENSE->SYNCBUSY)
    ;
}


/** @} (end addtogroup LESENSE) */
/** @} (end addtogroup emlib) */

#endif /* defined(LESENSE_COUNT) && (LESENSE_COUNT > 0) */

/***************************************************************************//**
 * @file em_vdac.c
 * @brief Digital to Analog Converter (VDAC) Peripheral API
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

#include "em_vdac.h"
#if defined(VDAC_COUNT) && (VDAC_COUNT > 0)
#include "em_cmu.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup VDAC
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of VDAC channel for assert statements. */
#define VDAC_CH_VALID(ch)    ((ch) <= 1)

/** Max VDAC clock */
#define VDAC_MAX_CLOCK            1000000

/** Max clock frequency of internal clock oscillator, 10 MHz + 20%. */
#define VDAC_INTERNAL_CLOCK_FREQ  12000000

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Enable/disable VDAC channel.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] ch
 *   Channel to enable/disable.
 *
 * @param[in] enable
 *   true to enable VDAC channel, false to disable.
 ******************************************************************************/
void VDAC_Enable(VDAC_TypeDef *vdac, unsigned int ch, bool enable)
{
  EFM_ASSERT(VDAC_REF_VALID(vdac));
  EFM_ASSERT(VDAC_CH_VALID(ch));

  if (ch == 0)
  {
    if (enable)
    {
      vdac->CMD = VDAC_CMD_CH0EN;
    }
    else
    {
      vdac->CMD = VDAC_CMD_CH0DIS;
      while (vdac->STATUS & VDAC_STATUS_CH0ENS);
    }
  }
  else
  {
    if (enable)
    {
      vdac->CMD = VDAC_CMD_CH1EN;
    }
    else
    {
      vdac->CMD = VDAC_CMD_CH1DIS;
      while (vdac->STATUS & VDAC_STATUS_CH1ENS);
    }
  }
}

/***************************************************************************//**
 * @brief
 *   Initialize VDAC.
 *
 * @details
 *   Initializes common parts for both channels. This function will also load
 *   calibration values from the Device Information (DI) page into the VDAC
 *   calibration register.
 *   To complete a VDAC setup, channel control configuration must also be done,
 *   please refer to VDAC_InitChannel().
 *
 * @note
 *   This function will disable both channels prior to configuration.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] init
 *   Pointer to VDAC initialization structure.
 ******************************************************************************/
void VDAC_Init(VDAC_TypeDef *vdac, const VDAC_Init_TypeDef *init)
{
  uint32_t cal, tmp = 0;
  uint32_t const volatile *calData;

  EFM_ASSERT(VDAC_REF_VALID(vdac));

  /* Make sure both channels are disabled. */
  vdac->CMD = VDAC_CMD_CH0DIS | VDAC_CMD_CH1DIS;
  while (vdac->STATUS & (VDAC_STATUS_CH0ENS | VDAC_STATUS_CH1ENS));

  /* Get OFFSETTRIM calibration value. */
  cal = ((DEVINFO->VDAC0CH1CAL & _DEVINFO_VDAC0CH1CAL_OFFSETTRIM_MASK)
         >> _DEVINFO_VDAC0CH1CAL_OFFSETTRIM_SHIFT)
        << _VDAC_CAL_OFFSETTRIM_SHIFT;

  if (init->mainCalibration)
  {
    calData = &DEVINFO->VDAC0MAINCAL;
  }
  else
  {
    calData = &DEVINFO->VDAC0ALTCAL;
  }

  /* Get correct GAINERRTRIM calibration value. */
  switch (init->reference)
  {
    case vdacRef1V25Ln:
      tmp = (*calData & _DEVINFO_VDAC0MAINCAL_GAINERRTRIM1V25LN_MASK)
            >> _DEVINFO_VDAC0MAINCAL_GAINERRTRIM1V25LN_SHIFT;
      break;

    case vdacRef2V5Ln:
      tmp = (*calData & _DEVINFO_VDAC0MAINCAL_GAINERRTRIM2V5LN_MASK)
            >> _DEVINFO_VDAC0MAINCAL_GAINERRTRIM2V5LN_SHIFT;
      break;

    case vdacRef1V25:
      tmp = (*calData & _DEVINFO_VDAC0MAINCAL_GAINERRTRIM1V25_MASK)
            >> _DEVINFO_VDAC0MAINCAL_GAINERRTRIM1V25_SHIFT;
      break;

    case vdacRef2V5:
      tmp = (*calData & _DEVINFO_VDAC0MAINCAL_GAINERRTRIM2V5_MASK)
            >> _DEVINFO_VDAC0MAINCAL_GAINERRTRIM2V5_SHIFT;
      break;

    case vdacRefAvdd:
    case vdacRefExtPin:
      tmp = (*calData & _DEVINFO_VDAC0MAINCAL_GAINERRTRIMVDDANAEXTPIN_MASK)
            >> _DEVINFO_VDAC0MAINCAL_GAINERRTRIMVDDANAEXTPIN_SHIFT;
      break;
  }

  /* Set GAINERRTRIM calibration value. */
  cal |= tmp << _VDAC_CAL_GAINERRTRIM_SHIFT;

  /* Get GAINERRTRIMCH1 calibration value. */
  switch (init->reference)
  {
    case vdacRef1V25Ln:
    case vdacRef1V25:
    case vdacRefAvdd:
    case vdacRefExtPin:
      tmp = (DEVINFO->VDAC0CH1CAL && _DEVINFO_VDAC0CH1CAL_GAINERRTRIMCH1A_MASK)
            >> _DEVINFO_VDAC0CH1CAL_GAINERRTRIMCH1A_SHIFT;
      break;

    case vdacRef2V5Ln:
    case vdacRef2V5:
      tmp = (DEVINFO->VDAC0CH1CAL && _DEVINFO_VDAC0CH1CAL_GAINERRTRIMCH1B_MASK)
            >> _DEVINFO_VDAC0CH1CAL_GAINERRTRIMCH1B_SHIFT;
      break;
  }

  /* Set GAINERRTRIM calibration value. */
  cal |= tmp << _VDAC_CAL_GAINERRTRIMCH1_SHIFT;

  tmp = ((uint32_t)init->asyncClockMode << _VDAC_CTRL_DACCLKMODE_SHIFT)
        | ((uint32_t)init->warmupKeepOn << _VDAC_CTRL_WARMUPMODE_SHIFT)
        | ((uint32_t)init->refresh      << _VDAC_CTRL_REFRESHPERIOD_SHIFT)
        | (((uint32_t)init->prescaler   << _VDAC_CTRL_PRESC_SHIFT)
           & _VDAC_CTRL_PRESC_MASK)
        | ((uint32_t)init->reference    << _VDAC_CTRL_REFSEL_SHIFT)
        | ((uint32_t)init->ch0ResetPre  << _VDAC_CTRL_CH0PRESCRST_SHIFT)
        | ((uint32_t)init->outEnablePRS << _VDAC_CTRL_OUTENPRS_SHIFT)
        | ((uint32_t)init->sineEnable   << _VDAC_CTRL_SINEMODE_SHIFT)
        | ((uint32_t)init->diff         << _VDAC_CTRL_DIFF_SHIFT);

  /* Write to VDAC registers. */
  vdac->CAL = cal;
  vdac->CTRL = tmp;
}

/***************************************************************************//**
 * @brief
 *   Initialize a VDAC channel.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] init
 *   Pointer to VDAC channel initialization structure.
 *
 * @param[in] ch
 *   Channel number to initialize.
 ******************************************************************************/
void VDAC_InitChannel(VDAC_TypeDef *vdac,
                      const VDAC_InitChannel_TypeDef *init,
                      unsigned int ch)
{
  uint32_t vdacChCtrl, vdacStatus;

  EFM_ASSERT(VDAC_REF_VALID(vdac));
  EFM_ASSERT(VDAC_CH_VALID(ch));

  /* Make sure both channels are disabled. */
  vdacStatus = vdac->STATUS;
  vdac->CMD = VDAC_CMD_CH0DIS | VDAC_CMD_CH1DIS;
  while (vdac->STATUS & (VDAC_STATUS_CH0ENS | VDAC_STATUS_CH1ENS));

  vdacChCtrl = ((uint32_t)init->prsSel          << _VDAC_CH0CTRL_PRSSEL_SHIFT)
               | ((uint32_t)init->prsAsync      << _VDAC_CH0CTRL_PRSASYNC_SHIFT)
               | ((uint32_t)init->trigMode      << _VDAC_CH0CTRL_TRIGMODE_SHIFT)
               | ((uint32_t)init->sampleOffMode << _VDAC_CH0CTRL_CONVMODE_SHIFT);

  if (ch == 0)
  {
    vdac->CH0CTRL = vdacChCtrl;
  }
  else
  {
    vdac->CH1CTRL = vdacChCtrl;
  }

  /* Check if the channel must be enabled. */
  if (init->enable)
  {
    if (ch == 0)
    {
      vdac->CMD = VDAC_CMD_CH0EN;
    }
    else
    {
      vdac->CMD = VDAC_CMD_CH1EN;
    }
  }

  /* Check if the other channel had to be turned off above
   * and needs to be turned on again. */
  if (ch == 0)
  {
    if (vdacStatus & VDAC_STATUS_CH1ENS)
    {
      vdac->CMD = VDAC_CMD_CH1EN;
    }
  }
  else
  {
    if (vdacStatus & VDAC_STATUS_CH0ENS)
    {
      vdac->CMD = VDAC_CMD_CH0EN;
    }
  }
}

/***************************************************************************//**
 * @brief
 *   Set the output signal of a VDAC channel to a given value.
 *
 * @details
 *   This function sets the output signal of a VDAC channel by writing @p value
 *   to the corresponding CHnDATA register.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] channel
 *   Channel number to set output of.
 *
 * @param[in] value
 *   Value to write to the channel output register CHnDATA.
 ******************************************************************************/
void VDAC_ChannelOutputSet(VDAC_TypeDef *vdac,
                           unsigned int channel,
                           uint32_t value)
{
  switch(channel)
  {
    case 0:
      VDAC_Channel0OutputSet(vdac, value);
      break;
    case 1:
      VDAC_Channel1OutputSet(vdac, value);
      break;
    default:
      EFM_ASSERT(0);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Calculate prescaler value used to determine VDAC clock.
 *
 * @details
 *   The VDAC clock is given by input clock divided by prescaler+1.
 *
 *     VDAC_CLK = IN_CLK / (prescale + 1)
 *
 *   Maximum VDAC clock is 1 MHz. Input clock is HFPERCLK when VDAC synchronous
 *   mode is selected, or an internal oscillator of 10 MHz +/- 20% when
 *   asynchronous mode is selected.
 *
 * @note
 *   If the requested VDAC frequency is low and the max prescaler value can not
 *   adjust the actual VDAC frequency lower than requested, the max prescaler
 *   value is returned, resulting in a higher VDAC frequency than requested.
 *
 * @param[in] vdacFreq VDAC frequency target. The frequency will automatically
 *   be adjusted to be below max allowed VDAC clock.
 *
 * @param[in] syncMode Set to true if you intend to use VDAC in synchronous
 *   mode.
 *
 * @param[in] hfperFreq Frequency in Hz of HFPERCLK oscillator. Set to 0 to
 *   use currently defined HFPERCLK clock setting. This parameter is only used
 *   when syncMode is set to true.
 *
 * @return
 *   Prescaler value to use for VDAC in order to achieve a clock value less than
 *   or equal to @p vdacFreq.
 ******************************************************************************/
uint32_t VDAC_PrescaleCalc(uint32_t vdacFreq, bool syncMode, uint32_t hfperFreq)
{
  uint32_t ret, refFreq;

  /* Make sure selected VDAC clock is below max value */
  if (vdacFreq > VDAC_MAX_CLOCK)
  {
    vdacFreq = VDAC_MAX_CLOCK;
  }

  if (!syncMode)
  {
    refFreq = VDAC_INTERNAL_CLOCK_FREQ;
  }
  else
  {
    if (hfperFreq)
    {
      refFreq = hfperFreq;
    }
    else
    {
      refFreq = CMU_ClockFreqGet(cmuClock_HFPER);
    }
  }

  /* Iterate in order to determine best prescale value. Start with lowest */
  /* prescaler value in order to get the first equal or less VDAC         */
  /* frequency value. */
  for (ret = 0; ret <= _VDAC_CTRL_PRESC_MASK >> _VDAC_CTRL_PRESC_SHIFT; ret++)
  {
    if ((refFreq / (ret + 1)) <= vdacFreq)
    {
      break;
    }
  }

  /* If ret is higher than the max prescaler value, make sure to return
     the max value. */
  if (ret > (_VDAC_CTRL_PRESC_MASK >> _VDAC_CTRL_PRESC_SHIFT))
  {
    ret = _VDAC_CTRL_PRESC_MASK >> _VDAC_CTRL_PRESC_SHIFT;
  }

  return ret;
}

/***************************************************************************//**
 * @brief
 *   Reset VDAC to same state as after a HW reset.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 ******************************************************************************/
void VDAC_Reset(VDAC_TypeDef *vdac)
{
  /* Disable channels, before resetting other registers. */
  vdac->CMD     = VDAC_CMD_CH0DIS | VDAC_CMD_CH1DIS;
  while (vdac->STATUS & (VDAC_STATUS_CH0ENS | VDAC_STATUS_CH1ENS));
  vdac->CH0CTRL = _VDAC_CH0CTRL_RESETVALUE;
  vdac->CH1CTRL = _VDAC_CH1CTRL_RESETVALUE;
  vdac->CH0DATA = _VDAC_CH0DATA_RESETVALUE;
  vdac->CH1DATA = _VDAC_CH1DATA_RESETVALUE;
  vdac->CTRL    = _VDAC_CTRL_RESETVALUE;
  vdac->IEN     = _VDAC_IEN_RESETVALUE;
  vdac->IFC     = _VDAC_IFC_MASK;
  vdac->CAL     = _VDAC_CAL_RESETVALUE;
}

/** @} (end addtogroup VDAC) */
/** @} (end addtogroup emlib) */
#endif /* defined(VDAC_COUNT) && (VDAC_COUNT > 0) */

/***************************************************************************//**
 * @file em_dac.c
 * @brief Digital to Analog Converter (DAC) Peripheral API
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

#include "em_dac.h"
#if defined(DAC_COUNT) && (DAC_COUNT > 0)
#include "em_cmu.h"
#include "em_assert.h"
#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DAC
 * @brief Digital to Analog Converter (DAC) Peripheral API
 * @details
 *  This module contains functions to control the DAC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The DAC converts digital values to analog signals
 *  at up to 500 ksps with 12-bit accuracy. The DAC is designed for low energy
 *  consumption, but can also provide very good performance.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of DAC channel for assert statements. */
#define DAC_CH_VALID(ch)    ((ch) <= 1)

/** Max DAC clock */
#define DAC_MAX_CLOCK    1000000

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Enable/disable DAC channel.
 *
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 *
 * @param[in] ch
 *   Channel to enable/disable.
 *
 * @param[in] enable
 *   true to enable DAC channel, false to disable.
 ******************************************************************************/
void DAC_Enable(DAC_TypeDef *dac, unsigned int ch, bool enable)
{
  volatile uint32_t *reg;

  EFM_ASSERT(DAC_REF_VALID(dac));
  EFM_ASSERT(DAC_CH_VALID(ch));

  if (!ch)
  {
    reg = &(dac->CH0CTRL);
  }
  else
  {
    reg = &(dac->CH1CTRL);
  }

  BUS_RegBitWrite(reg, _DAC_CH0CTRL_EN_SHIFT, enable);
}


/***************************************************************************//**
 * @brief
 *   Initialize DAC.
 *
 * @details
 *   Initializes common parts for both channels. In addition, channel control
 *   configuration must be done, please refer to DAC_InitChannel().
 *
 * @note
 *   This function will disable both channels prior to configuration.
 *
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 *
 * @param[in] init
 *   Pointer to DAC initialization structure.
 ******************************************************************************/
void DAC_Init(DAC_TypeDef *dac, const DAC_Init_TypeDef *init)
{
  uint32_t tmp;

  EFM_ASSERT(DAC_REF_VALID(dac));

  /* Make sure both channels are disabled. */
  BUS_RegBitWrite(&(dac->CH0CTRL), _DAC_CH0CTRL_EN_SHIFT, 0);
  BUS_RegBitWrite(&(dac->CH1CTRL), _DAC_CH0CTRL_EN_SHIFT, 0);

  /* Load proper calibration data depending on selected reference */
  switch (init->reference)
  {
    case dacRef2V5:
      dac->CAL = DEVINFO->DAC0CAL1;
      break;

    case dacRefVDD:
      dac->CAL = DEVINFO->DAC0CAL2;
      break;

    default: /* 1.25V */
      dac->CAL = DEVINFO->DAC0CAL0;
      break;
  }

  tmp = ((uint32_t)(init->refresh)     << _DAC_CTRL_REFRSEL_SHIFT)
        | (((uint32_t)(init->prescale) << _DAC_CTRL_PRESC_SHIFT)
           & _DAC_CTRL_PRESC_MASK)
        | ((uint32_t)(init->reference) << _DAC_CTRL_REFSEL_SHIFT)
        | ((uint32_t)(init->outMode)   << _DAC_CTRL_OUTMODE_SHIFT)
        | ((uint32_t)(init->convMode)  << _DAC_CTRL_CONVMODE_SHIFT);

  if (init->ch0ResetPre)
  {
    tmp |= DAC_CTRL_CH0PRESCRST;
  }

  if (init->outEnablePRS)
  {
    tmp |= DAC_CTRL_OUTENPRS;
  }

  if (init->sineEnable)
  {
    tmp |= DAC_CTRL_SINEMODE;
  }

  if (init->diff)
  {
    tmp |= DAC_CTRL_DIFF;
  }

  dac->CTRL = tmp;
}


/***************************************************************************//**
 * @brief
 *   Initialize DAC channel.
 *
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 *
 * @param[in] init
 *   Pointer to DAC initialization structure.
 *
 * @param[in] ch
 *   Channel number to initialize.
 ******************************************************************************/
void DAC_InitChannel(DAC_TypeDef *dac,
                     const DAC_InitChannel_TypeDef *init,
                     unsigned int ch)
{
  uint32_t tmp;

  EFM_ASSERT(DAC_REF_VALID(dac));
  EFM_ASSERT(DAC_CH_VALID(ch));

  tmp = (uint32_t)(init->prsSel) << _DAC_CH0CTRL_PRSSEL_SHIFT;

  if (init->enable)
  {
    tmp |= DAC_CH0CTRL_EN;
  }

  if (init->prsEnable)
  {
    tmp |= DAC_CH0CTRL_PRSEN;
  }

  if (init->refreshEnable)
  {
    tmp |= DAC_CH0CTRL_REFREN;
  }

  if (ch)
  {
    dac->CH1CTRL = tmp;
  }
  else
  {
    dac->CH0CTRL = tmp;
  }
}


/***************************************************************************//**
 * @brief
 *   Set the output signal of a DAC channel to a given value.
 *
 * @details
 *   This function sets the output signal of a DAC channel by writing @p value
 *   to the corresponding CHnDATA register.
 *
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 *
 * @param[in] channel
 *   Channel number to set output of.
 *
 * @param[in] value
 *   Value to write to the channel output register CHnDATA.
 ******************************************************************************/
void DAC_ChannelOutputSet( DAC_TypeDef *dac,
                           unsigned int channel,
                           uint32_t     value )
{
  switch(channel)
  {
    case 0:
      DAC_Channel0OutputSet(dac, value);
      break;
    case 1:
      DAC_Channel1OutputSet(dac, value);
      break;
    default:
      EFM_ASSERT(0);
      break;
  }
}


/***************************************************************************//**
 * @brief
 *   Calculate prescaler value used to determine DAC clock.
 *
 * @details
 *   The DAC clock is given by: HFPERCLK / (prescale ^ 2). If the requested
 *   DAC frequency is low and the max prescaler value can not adjust the
 *   actual DAC frequency lower than the requested DAC frequency, then the
 *   max prescaler value is returned, resulting in a higher DAC frequency
 *   than requested.
 *
 * @param[in] dacFreq DAC frequency wanted. The frequency will automatically
 *   be adjusted to be below max allowed DAC clock.
 *
 * @param[in] hfperFreq Frequency in Hz of reference HFPER clock. Set to 0 to
 *   use currently defined HFPER clock setting.
 *
 * @return
 *   Prescaler value to use for DAC in order to achieve a clock value
 *   <= @p dacFreq.
 ******************************************************************************/
uint8_t DAC_PrescaleCalc(uint32_t dacFreq, uint32_t hfperFreq)
{
  uint32_t ret;

  /* Make sure selected DAC clock is below max value */
  if (dacFreq > DAC_MAX_CLOCK)
  {
    dacFreq = DAC_MAX_CLOCK;
  }

  /* Use current HFPER frequency? */
  if (!hfperFreq)
  {
    hfperFreq = CMU_ClockFreqGet(cmuClock_HFPER);
  }

  /* Iterate in order to determine best prescale value. Only a few possible */
  /* values. We start with lowest prescaler value in order to get first */
  /* equal or below wanted DAC frequency value. */
  for (ret = 0; ret <= (_DAC_CTRL_PRESC_MASK >> _DAC_CTRL_PRESC_SHIFT); ret++)
  {
    if ((hfperFreq >> ret) <= dacFreq)
      break;
  }

  /* If ret is higher than the max prescaler value, make sure to return
     the max value. */
  if (ret > (_DAC_CTRL_PRESC_MASK >> _DAC_CTRL_PRESC_SHIFT))
  {
    ret = _DAC_CTRL_PRESC_MASK >> _DAC_CTRL_PRESC_SHIFT;
  }

  return (uint8_t)ret;
}


/***************************************************************************//**
 * @brief
 *   Reset DAC to same state as after a HW reset.
 *
 * @param[in] dac
 *   Pointer to ADC peripheral register block.
 ******************************************************************************/
void DAC_Reset(DAC_TypeDef *dac)
{
  /* Disable channels, before resetting other registers. */
  dac->CH0CTRL  = _DAC_CH0CTRL_RESETVALUE;
  dac->CH1CTRL  = _DAC_CH1CTRL_RESETVALUE;
  dac->CTRL     = _DAC_CTRL_RESETVALUE;
  dac->IEN      = _DAC_IEN_RESETVALUE;
  dac->IFC      = _DAC_IFC_MASK;
  dac->CAL      = DEVINFO->DAC0CAL0;
  dac->BIASPROG = _DAC_BIASPROG_RESETVALUE;
  /* Do not reset route register, setting should be done independently */
}


/** @} (end addtogroup DAC) */
/** @} (end addtogroup emlib) */
#endif /* defined(DAC_COUNT) && (DAC_COUNT > 0) */

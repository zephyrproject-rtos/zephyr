/**************************************************************************//**
 * @file em_opamp.c
 * @brief Operational Amplifier (OPAMP) peripheral API
 * @version 5.1.2
 ******************************************************************************
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

#include "em_opamp.h"
#if ((defined(_SILICON_LABS_32B_SERIES_0) && defined(OPAMP_PRESENT) && (OPAMP_COUNT == 1)) \
     || (defined(_SILICON_LABS_32B_SERIES_1) && defined(VDAC_PRESENT)  && (VDAC_COUNT > 0)))

#include "em_system.h"
#include "em_assert.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/


/***************************************************************************//**
 * @addtogroup OPAMP
 * @brief Operational Amplifier (OPAMP) peripheral API
 * @details
 *  This module contains functions to:
 *   @li OPAMP_Enable()       Configure and enable an opamp.
 *   @li OPAMP_Disable()      Disable an opamp.
 *
 * @if DOXYDOC_P1_DEVICE
 * All OPAMP functions assume that the DAC clock is running. If the DAC is not
 * used, the clock can be turned off when the opamp's are configured.
 * @elseif DOXYDOC_P2_DEVICE
 * All OPAMP functions assume that the VDAC clock is running. If the VDAC is not
 * used, the clock can be turned off when the opamp's are configured.
 * @endif
 *
 * If the available gain values dont suit the application at hand, the resistor
 * ladders can be disabled and external gain programming resistors used.
 *
 * A number of predefined opamp setup macros are available for configuration
 * of the most common opamp topologies (see figures below).
 *
 * @note
 * <em>The terms POSPAD and NEGPAD in the figures are used to indicate that these
 * pads should be connected to a suitable signal ground.</em>
 *
 * \n<b>Unity gain voltage follower.</b>\n
 * @if DOXYDOC_P1_DEVICE
 * Use predefined macros @ref OPA_INIT_UNITY_GAIN and
 * @ref OPA_INIT_UNITY_GAIN_OPA2.
 * @elseif DOXYDOC_P2_DEVICE
 * Use predefined macro @ref OPA_INIT_UNITY_GAIN.
 * @endif
 * @verbatim

                       |\
            ___________|+\
                       |  \_______
                    ___|_ /    |
                   |   | /     |
                   |   |/      |
                   |___________|
   @endverbatim
 *
 * \n<b>Non-inverting amplifier.</b>\n
 * @if DOXYDOC_P1_DEVICE
 * Use predefined macros @ref OPA_INIT_NON_INVERTING and
 * @ref OPA_INIT_NON_INVERTING_OPA2.
 * @elseif DOXYDOC_P2_DEVICE
 * Use predefined macro @ref OPA_INIT_NON_INVERTING.
 * @endif
 * @verbatim

                       |\
            ___________|+\
                       |  \_______
                    ___|_ /    |
                   |   | /     |
                   |   |/      |
                   |_____R2____|
                   |
                   R1
                   |
                 NEGPAD @endverbatim
 *
 * \n<b>Inverting amplifier.</b>\n
 * @if DOXYDOC_P1_DEVICE
 * Use predefined macros @ref OPA_INIT_INVERTING and
 * @ref OPA_INIT_INVERTING_OPA2.
 * @elseif DOXYDOC_P2_DEVICE
 * Use predefined macro @ref OPA_INIT_INVERTING.
 * @endif
 * @verbatim

                    _____R2____
                   |           |
                   |   |\      |
            ____R1_|___|_\     |
                       |  \____|___
                    ___|  /
                   |   |+/
                   |   |/
                   |
                 POSPAD @endverbatim
 *
 * \n<b>Cascaded non-inverting amplifiers.</b>\n
 * Use predefined macros @ref OPA_INIT_CASCADED_NON_INVERTING_OPA0,
 * @ref OPA_INIT_CASCADED_NON_INVERTING_OPA1 and
 * @ref OPA_INIT_CASCADED_NON_INVERTING_OPA2.
 * @verbatim

                       |\                       |\                       |\
            ___________|+\ OPA0      ___________|+\ OPA1      ___________|+\ OPA2
                       |  \_________|           |  \_________|           |  \_______
                    ___|_ /    |             ___|_ /    |             ___|_ /    |
                   |   | /     |            |   | /     |            |   | /     |
                   |   |/      |            |   |/      |            |   |/      |
                   |_____R2____|            |_____R2____|            |_____R2____|
                   |                        |                        |
                   R1                       R1                       R1
                   |                        |                        |
                 NEGPAD                   NEGPAD                   NEGPAD @endverbatim
 *
 * \n<b>Cascaded inverting amplifiers.</b>\n
 * Use predefined macros @ref OPA_INIT_CASCADED_INVERTING_OPA0,
 * @ref OPA_INIT_CASCADED_INVERTING_OPA1 and
 * @ref OPA_INIT_CASCADED_INVERTING_OPA2.
 * @verbatim

                    _____R2____              _____R2____              _____R2____
                   |           |            |           |            |           |
                   |   |\      |            |   |\      |            |   |\      |
            ____R1_|___|_\     |     ____R1_|___|_\     |     ____R1_|___|_\     |
                       |  \____|____|           |  \____|___|            |  \____|__
                    ___|  /                  ___|  /                  ___|  /
                   |   |+/ OPA0             |   |+/ OPA1             |   |+/ OPA2
                   |   |/                   |   |/                   |   |/
                   |                        |                        |
                 POSPAD                   POSPAD                   POSPAD @endverbatim
 *
 * \n<b>Differential driver with two opamp's.</b>\n
 * Use predefined macros @ref OPA_INIT_DIFF_DRIVER_OPA0 and
 * @ref OPA_INIT_DIFF_DRIVER_OPA1.
 * @verbatim

                                     __________________________
                                    |                          +
                                    |        _____R2____
                       |\           |       |           |
            ___________|+\ OPA0     |       |   |\ OPA1 |
                       |  \_________|____R1_|___|_\     |      _
                    ___|_ /         |           |  \____|______
                   |   | /          |        ___|  /
                   |   |/           |       |   |+/
                   |________________|       |   |/
                                            |
                                          POSPAD @endverbatim
 *
 * \n<b>Differential receiver with three opamp's.</b>\n
 * Use predefined macros @ref OPA_INIT_DIFF_RECEIVER_OPA0,
 * @ref OPA_INIT_DIFF_RECEIVER_OPA1 and @ref OPA_INIT_DIFF_RECEIVER_OPA2.
 * @verbatim

                       |\
             __________|+\ OPA1
            _          |  \_________
                    ___|_ /    |    |        _____R2____
                   |   | /     |    |       |           |
                   |   |/      |    |       |   |\      |
                   |___________|    |____R1_|___|_\     |
                                                |  \____|___
                       |\            ____R1_ ___|  /
            +__________|+\ OPA0     |       |   |+/ OPA2
                       |  \_________|       |   |/
                    ___|_ /    |            R2
                   |   | /     |            |
                   |   |/      |          NEGPAD OPA0
                   |___________|
   @endverbatim
 *
 * @if DOXYDOC_P2_DEVICE
 * \n<b>Instrumentation amplifier.</b>\n
 * Use predefined macros @ref OPA_INIT_INSTR_AMP_OPA0 and
 * @ref OPA_INIT_INSTR_AMP_OPA0.
 * @verbatim

                       |\
             __________|+\ OPA1
                       |  \______________
                    ___|_ /     |
                   |   | /      |
                   |   |/       R2
                   |____________|
                                |
                                R1
                                |
                                R1
                    ____________|
                   |            |
                   |            R2
                   |   |\       |
                   |___|+\ OPA0 |
                       |  \_____|________
             __________|_ /
                       | /
                       |/

   @endverbatim
 * @endif
 *
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Disable an Operational Amplifier.
 *
 * @if DOXYDOC_P1_DEVICE
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 * @elseif DOXYDOC_P2_DEVICE
 * @param[in] dac
 *   Pointer to VDAC peripheral register block.
 * @endif
 *
 *
 * @param[in] opa
 *   Selects an OPA, valid vaules are @ref OPA0, @ref OPA1 and @ref OPA2.
 ******************************************************************************/
void OPAMP_Disable(
#if defined(_SILICON_LABS_32B_SERIES_0)
  DAC_TypeDef *dac,
#elif defined(_SILICON_LABS_32B_SERIES_1)
  VDAC_TypeDef *dac,
#endif
  OPAMP_TypeDef opa)
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  EFM_ASSERT(DAC_REF_VALID(dac));
  EFM_ASSERT(DAC_OPA_VALID(opa));

  if (opa == OPA0)
  {
    dac->CH0CTRL &= ~DAC_CH0CTRL_EN;
    dac->OPACTRL &= ~DAC_OPACTRL_OPA0EN;
  }
  else if (opa == OPA1)
  {
    dac->CH1CTRL &= ~DAC_CH1CTRL_EN;
    dac->OPACTRL &= ~DAC_OPACTRL_OPA1EN;
  }
  else /* OPA2 */
  {
    dac->OPACTRL &= ~DAC_OPACTRL_OPA2EN;
  }

#elif defined(_SILICON_LABS_32B_SERIES_1)
  EFM_ASSERT(VDAC_REF_VALID(dac));
  EFM_ASSERT(VDAC_OPA_VALID(opa));

  if (opa == OPA0)
  {
    dac->CMD |= VDAC_CMD_OPA0DIS;
    while (dac->STATUS & VDAC_STATUS_OPA0ENS)
    {
    }
  }
  else if (opa == OPA1)
  {
    dac->CMD |= VDAC_CMD_OPA1DIS;
    while (dac->STATUS & VDAC_STATUS_OPA1ENS)
    {
    }
  }
  else /* OPA2 */
  {
    dac->CMD |= VDAC_CMD_OPA2DIS;
    while (dac->STATUS & VDAC_STATUS_OPA2ENS)
    {
    }
  }
#endif
}


/***************************************************************************//**
 * @brief
 *   Configure and enable an Operational Amplifier.
 *
 * @if DOXYDOC_P1_DEVICE
 * @note
 *   The value of the alternate output enable bit mask in the OPAMP_Init_TypeDef
 *   structure should consist of one or more of the
 *   DAC_OPA[opa#]MUX_OUTPEN_OUT[output#] flags
 *   (defined in \<part_name\>_dac.h) OR'ed together. @n @n
 *   For OPA0:
 *   @li DAC_OPA0MUX_OUTPEN_OUT0
 *   @li DAC_OPA0MUX_OUTPEN_OUT1
 *   @li DAC_OPA0MUX_OUTPEN_OUT2
 *   @li DAC_OPA0MUX_OUTPEN_OUT3
 *   @li DAC_OPA0MUX_OUTPEN_OUT4
 *
 *   For OPA1:
 *   @li DAC_OPA1MUX_OUTPEN_OUT0
 *   @li DAC_OPA1MUX_OUTPEN_OUT1
 *   @li DAC_OPA1MUX_OUTPEN_OUT2
 *   @li DAC_OPA1MUX_OUTPEN_OUT3
 *   @li DAC_OPA1MUX_OUTPEN_OUT4
 *
 *   For OPA2:
 *   @li DAC_OPA2MUX_OUTPEN_OUT0
 *   @li DAC_OPA2MUX_OUTPEN_OUT1
 *
 *   E.g: @n
 *   init.outPen = DAC_OPA0MUX_OUTPEN_OUT0 | DAC_OPA0MUX_OUTPEN_OUT4;
 *
 * @param[in] dac
 *   Pointer to DAC peripheral register block.
 * @elseif DOXYDOC_P2_DEVICE
 * @note
 *   The value of the alternate output enable bit mask in the OPAMP_Init_TypeDef
 *   structure should consist of one or more of the
 *   VDAC_OPA_OUT_ALTOUTPADEN_OUT[output#] flags
 *   (defined in \<part_name\>_vdac.h) OR'ed together. @n @n
 *   @li VDAC_OPA_OUT_ALTOUTPADEN_OUT0
 *   @li VDAC_OPA_OUT_ALTOUTPADEN_OUT1
 *   @li VDAC_OPA_OUT_ALTOUTPADEN_OUT2
 *   @li VDAC_OPA_OUT_ALTOUTPADEN_OUT3
 *   @li VDAC_OPA_OUT_ALTOUTPADEN_OUT4
 *
 *   E.g: @n
 *   init.outPen = VDAC_OPA_OUT_ALTOUTPADEN_OUT0 | VDAC_OPA_OUT_ALTOUTPADEN_OUT4;
 * @param[in] dac
 *   Pointer to VDAC peripheral register block.
 * @endif
 *
 * @param[in] opa
 *   Selects an OPA, valid vaules are @ref OPA0, @ref OPA1 and @ref OPA2.
 *
 * @param[in] init
 *   Pointer to a structure containing OPAMP init information.
 ******************************************************************************/
void OPAMP_Enable(
#if defined(_SILICON_LABS_32B_SERIES_0)
  DAC_TypeDef *dac,
#elif defined(_SILICON_LABS_32B_SERIES_1)
  VDAC_TypeDef *dac,
#endif
  OPAMP_TypeDef opa,
  const OPAMP_Init_TypeDef *init)
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  uint32_t offset;

  EFM_ASSERT(DAC_REF_VALID(dac));
  EFM_ASSERT(DAC_OPA_VALID(opa));
  EFM_ASSERT(init->bias <= (_DAC_BIASPROG_BIASPROG_MASK
                             >> _DAC_BIASPROG_BIASPROG_SHIFT));

  if (opa == OPA0)
  {
    EFM_ASSERT((init->outPen & ~_DAC_OPA0MUX_OUTPEN_MASK) == 0);

    dac->BIASPROG = (dac->BIASPROG
                     & ~(_DAC_BIASPROG_BIASPROG_MASK
                         | DAC_BIASPROG_HALFBIAS))
                    | (init->bias     << _DAC_BIASPROG_BIASPROG_SHIFT)
                    | (init->halfBias ?   DAC_BIASPROG_HALFBIAS : 0);

    if (init->defaultOffset)
    {
      offset = SYSTEM_GetCalibrationValue(&dac->CAL);
      dac->CAL = (dac->CAL & ~_DAC_CAL_CH0OFFSET_MASK)
                 | (offset &  _DAC_CAL_CH0OFFSET_MASK);
    }
    else
    {
      EFM_ASSERT(init->offset <= (_DAC_CAL_CH0OFFSET_MASK
                                  >> _DAC_CAL_CH0OFFSET_SHIFT));

      dac->CAL = (dac->CAL & ~_DAC_CAL_CH0OFFSET_MASK)
                 | (init->offset << _DAC_CAL_CH0OFFSET_SHIFT);
    }

    dac->OPA0MUX  = (uint32_t)init->resSel
                    | (uint32_t)init->outMode
                    | init->outPen
                    | (uint32_t)init->resInMux
                    | (uint32_t)init->negSel
                    | (uint32_t)init->posSel
                    | ( init->nextOut ? DAC_OPA0MUX_NEXTOUT : 0)
                    | ( init->npEn    ? DAC_OPA0MUX_NPEN    : 0)
                    | ( init->ppEn    ? DAC_OPA0MUX_PPEN    : 0);

    dac->CH0CTRL |= DAC_CH0CTRL_EN;
    dac->OPACTRL  = (dac->OPACTRL
                     & ~(DAC_OPACTRL_OPA0SHORT
                         | _DAC_OPACTRL_OPA0LPFDIS_MASK
                         |  DAC_OPACTRL_OPA0HCMDIS))
                    | (init->shortInputs ?  DAC_OPACTRL_OPA0SHORT : 0)
                    | (init->lpfPosPadDisable
                       ? DAC_OPACTRL_OPA0LPFDIS_PLPFDIS : 0)
                    | (init->lpfNegPadDisable
                       ? DAC_OPACTRL_OPA0LPFDIS_NLPFDIS : 0)
                    | (init->hcmDisable ? DAC_OPACTRL_OPA0HCMDIS : 0)
                    | DAC_OPACTRL_OPA0EN;
  }
  else if ( opa == OPA1 )
  {
    EFM_ASSERT((init->outPen & ~_DAC_OPA1MUX_OUTPEN_MASK) == 0);

    dac->BIASPROG = (dac->BIASPROG
                     & ~(_DAC_BIASPROG_BIASPROG_MASK
                         | DAC_BIASPROG_HALFBIAS))
                    | (init->bias   << _DAC_BIASPROG_BIASPROG_SHIFT)
                    | (init->halfBias ? DAC_BIASPROG_HALFBIAS : 0 );

    if (init->defaultOffset)
    {
      offset = SYSTEM_GetCalibrationValue(&dac->CAL);
      dac->CAL = (dac->CAL & ~_DAC_CAL_CH1OFFSET_MASK)
                 | (offset &  _DAC_CAL_CH1OFFSET_MASK);
    }
    else
    {
      EFM_ASSERT(init->offset <= (_DAC_CAL_CH1OFFSET_MASK
                                  >> _DAC_CAL_CH1OFFSET_SHIFT));

      dac->CAL = (dac->CAL & ~_DAC_CAL_CH1OFFSET_MASK)
                 | (init->offset << _DAC_CAL_CH1OFFSET_SHIFT);
    }

    dac->OPA1MUX  = (uint32_t)init->resSel
                    | (uint32_t)init->outMode
                    | init->outPen
                    | (uint32_t)init->resInMux
                    | (uint32_t)init->negSel
                    | (uint32_t)init->posSel
                    | (init->nextOut ? DAC_OPA1MUX_NEXTOUT : 0)
                    | (init->npEn    ? DAC_OPA1MUX_NPEN    : 0)
                    | (init->ppEn    ? DAC_OPA1MUX_PPEN    : 0);

    dac->CH1CTRL |= DAC_CH1CTRL_EN;
    dac->OPACTRL  = (dac->OPACTRL
                     & ~(DAC_OPACTRL_OPA1SHORT
                         | _DAC_OPACTRL_OPA1LPFDIS_MASK
                         | DAC_OPACTRL_OPA1HCMDIS))
                    | (init->shortInputs ? DAC_OPACTRL_OPA1SHORT : 0)
                    | (init->lpfPosPadDisable
                       ? DAC_OPACTRL_OPA1LPFDIS_PLPFDIS : 0)
                    | (init->lpfNegPadDisable
                       ? DAC_OPACTRL_OPA1LPFDIS_NLPFDIS : 0)
                    | (init->hcmDisable ? DAC_OPACTRL_OPA1HCMDIS : 0)
                    | DAC_OPACTRL_OPA1EN;
  }
  else /* OPA2 */
  {
    EFM_ASSERT((init->posSel == DAC_OPA2MUX_POSSEL_DISABLE)
               || (init->posSel == DAC_OPA2MUX_POSSEL_POSPAD)
               || (init->posSel == DAC_OPA2MUX_POSSEL_OPA1INP)
               || (init->posSel == DAC_OPA2MUX_POSSEL_OPATAP));

    EFM_ASSERT((init->outMode & ~DAC_OPA2MUX_OUTMODE) == 0);

    EFM_ASSERT((init->outPen & ~_DAC_OPA2MUX_OUTPEN_MASK) == 0);

    dac->BIASPROG = (dac->BIASPROG
                     & ~(_DAC_BIASPROG_OPA2BIASPROG_MASK
                         | DAC_BIASPROG_OPA2HALFBIAS))
                    | (init->bias << _DAC_BIASPROG_OPA2BIASPROG_SHIFT)
                    | (init->halfBias ? DAC_BIASPROG_OPA2HALFBIAS : 0);

    if (init->defaultOffset)
    {
      offset = SYSTEM_GetCalibrationValue(&dac->OPAOFFSET);
      dac->OPAOFFSET = (dac->OPAOFFSET & ~_DAC_OPAOFFSET_OPA2OFFSET_MASK)
                       | (offset       &  _DAC_OPAOFFSET_OPA2OFFSET_MASK);
    }
    else
    {
      EFM_ASSERT(init->offset <= (_DAC_OPAOFFSET_OPA2OFFSET_MASK
                                  >> _DAC_OPAOFFSET_OPA2OFFSET_SHIFT));
      dac->OPAOFFSET = (dac->OPAOFFSET & ~_DAC_OPAOFFSET_OPA2OFFSET_MASK)
                       | (init->offset << _DAC_OPAOFFSET_OPA2OFFSET_SHIFT);
    }

    dac->OPA2MUX  = (uint32_t)init->resSel
                    | (uint32_t)init->outMode
                    | init->outPen
                    | (uint32_t)init->resInMux
                    | (uint32_t)init->negSel
                    | (uint32_t)init->posSel
                    | ( init->nextOut ? DAC_OPA2MUX_NEXTOUT : 0 )
                    | ( init->npEn    ? DAC_OPA2MUX_NPEN    : 0 )
                    | ( init->ppEn    ? DAC_OPA2MUX_PPEN    : 0 );

    dac->OPACTRL  = (dac->OPACTRL
                     & ~(DAC_OPACTRL_OPA2SHORT
                         | _DAC_OPACTRL_OPA2LPFDIS_MASK
                         | DAC_OPACTRL_OPA2HCMDIS))
                    | (init->shortInputs ?  DAC_OPACTRL_OPA2SHORT : 0)
                    | (init->lpfPosPadDisable
                       ? DAC_OPACTRL_OPA2LPFDIS_PLPFDIS : 0)
                    | (init->lpfNegPadDisable
                       ? DAC_OPACTRL_OPA2LPFDIS_NLPFDIS : 0)
                    | (init->hcmDisable ? DAC_OPACTRL_OPA2HCMDIS : 0)
                    | DAC_OPACTRL_OPA2EN;
  }

#elif defined(_SILICON_LABS_32B_SERIES_1)
  uint32_t calData = 0;
  uint32_t warmupTime;

  EFM_ASSERT(VDAC_REF_VALID(dac));
  EFM_ASSERT(VDAC_OPA_VALID(opa));
  EFM_ASSERT(init->settleTime <= (_VDAC_OPA_TIMER_SETTLETIME_MASK
                                  >> _VDAC_OPA_TIMER_SETTLETIME_SHIFT));
  EFM_ASSERT(init->startupDly <= (_VDAC_OPA_TIMER_STARTUPDLY_MASK
                                  >> _VDAC_OPA_TIMER_STARTUPDLY_SHIFT));
  EFM_ASSERT((init->outPen & ~_VDAC_OPA_OUT_ALTOUTPADEN_MASK) == 0);
  EFM_ASSERT(!((init->gain3xEn == true)
             && ((init->negSel == opaNegSelResTap)
                 || (init->posSel == opaPosSelResTap))));
  EFM_ASSERT((init->drvStr == opaDrvStrLowerAccLowStr)
             || (init->drvStr == opaDrvStrLowAccLowStr)
             || (init->drvStr == opaDrvStrHighAccHighStr)
             || (init->drvStr == opaDrvStrHigherAccHighStr));

  /* Disable OPAMP before writing to registers. */
  OPAMP_Disable(dac, opa);

  /* Get the calibration value based on OPAMP, Drive Strength, and INCBW. */
  switch (opa)
  {
    case OPA0:
      switch (init->drvStr)
      {
        case opaDrvStrLowerAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA0CAL0 : DEVINFO->OPA0CAL4);
          break;
        case opaDrvStrLowAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA0CAL1 : DEVINFO->OPA0CAL5);
          break;
        case opaDrvStrHighAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA0CAL2 : DEVINFO->OPA0CAL6);
          break;
        case opaDrvStrHigherAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA0CAL3 : DEVINFO->OPA0CAL7);
          break;
      }
      break;

    case OPA1:
      switch (init->drvStr)
      {
        case opaDrvStrLowerAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA1CAL0 : DEVINFO->OPA1CAL4);
          break;
        case opaDrvStrLowAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA1CAL1 : DEVINFO->OPA1CAL5);
          break;
        case opaDrvStrHighAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA1CAL2 : DEVINFO->OPA1CAL6);
          break;
        case opaDrvStrHigherAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA1CAL3 : DEVINFO->OPA1CAL7);
          break;
      }
      break;

    case OPA2:
      switch (init->drvStr)
      {
        case opaDrvStrLowerAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA2CAL0 : DEVINFO->OPA2CAL4);
          break;
        case opaDrvStrLowAccLowStr:
          calData = (init->ugBwScale ? DEVINFO->OPA2CAL1 : DEVINFO->OPA2CAL5);
          break;
        case opaDrvStrHighAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA2CAL2 : DEVINFO->OPA2CAL6);
          break;
        case opaDrvStrHigherAccHighStr:
          calData = (init->ugBwScale ? DEVINFO->OPA2CAL3 : DEVINFO->OPA2CAL7);
          break;
      }
      break;
  }
  if (!init->defaultOffsetN)
  {
    EFM_ASSERT(init->offsetN <= (_VDAC_OPA_CAL_OFFSETN_MASK
                                 >> _VDAC_OPA_CAL_OFFSETN_SHIFT));
    calData = (calData & ~_VDAC_OPA_CAL_OFFSETN_MASK)
              | (init->offsetN << _VDAC_OPA_CAL_OFFSETN_SHIFT);
  }
  if (!init->defaultOffsetP)
  {
    EFM_ASSERT(init->offsetP <= (_VDAC_OPA_CAL_OFFSETP_MASK
                                 >> _VDAC_OPA_CAL_OFFSETP_SHIFT));
    calData = (calData & ~_VDAC_OPA_CAL_OFFSETP_MASK)
              | (init->offsetP << _VDAC_OPA_CAL_OFFSETP_SHIFT);
  }

  dac->OPA[opa].CAL = (calData &  _VDAC_OPA_CAL_MASK);

  dac->OPA[opa].MUX = (uint32_t)init->resSel
                      | (init->gain3xEn  ? VDAC_OPA_MUX_GAIN3X : 0)
                      | (uint32_t)init->resInMux
                      | (uint32_t)init->negSel
                      | (uint32_t)init->posSel;

  dac->OPA[opa].OUT = (uint32_t)init->outMode
                      | (uint32_t)init->outPen;

  switch (init->drvStr)
  {
    case opaDrvStrHigherAccHighStr:
      warmupTime = 6;
      break;

    case opaDrvStrHighAccHighStr:
      warmupTime = 8;
      break;

    case opaDrvStrLowAccLowStr:
      warmupTime = 85;
      break;

    case opaDrvStrLowerAccLowStr:
    default:
      warmupTime = 100;
      break;
  }

  dac->OPA[opa].TIMER = (uint32_t)(init->settleTime
                                   << _VDAC_OPA_TIMER_SETTLETIME_SHIFT)
                        | (uint32_t)(warmupTime
                                     << _VDAC_OPA_TIMER_WARMUPTIME_SHIFT)
                        | (uint32_t)(init->startupDly
                                     << _VDAC_OPA_TIMER_STARTUPDLY_SHIFT);

  dac->OPA[opa].CTRL  = (init->aportYMasterDisable
                         ? VDAC_OPA_CTRL_APORTYMASTERDIS : 0)
                        | (init->aportXMasterDisable
                           ? VDAC_OPA_CTRL_APORTXMASTERDIS : 0)
                        | (uint32_t)init->prsOutSel
                        | (uint32_t)init->prsSel
                        | (uint32_t)init->prsMode
                        | (init->prsEn ? VDAC_OPA_CTRL_PRSEN : 0)
                        | (init->halfDrvStr
                           ? VDAC_OPA_CTRL_OUTSCALE_HALF
                             : VDAC_OPA_CTRL_OUTSCALE_FULL)
                        | (init->hcmDisable ? VDAC_OPA_CTRL_HCMDIS : 0)
                        | (init->ugBwScale ? VDAC_OPA_CTRL_INCBW : 0)
                        | (uint32_t)init->drvStr;

  if (opa == OPA0)
  {
    dac->CMD |= VDAC_CMD_OPA0EN;
  }
  else if (opa == OPA1)
  {
    dac->CMD |= VDAC_CMD_OPA1EN;
  }
  else /* OPA2 */
  {
    dac->CMD |= VDAC_CMD_OPA2EN;
  }

#endif
}

/** @} (end addtogroup OPAMP) */
/** @} (end addtogroup emlib) */

#endif /* (defined(OPAMP_PRESENT) && (OPAMP_COUNT == 1)
           || defined(VDAC_PRESENT) && (VDAC_COUNT > 0) */

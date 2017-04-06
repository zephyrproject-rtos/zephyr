/***************************************************************************//**
 * @file em_vdac.h
 * @brief Digital to Analog Converter (VDAC) peripheral API
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

#ifndef EM_VDAC_H
#define EM_VDAC_H

#include "em_device.h"

#if defined(VDAC_COUNT) && (VDAC_COUNT > 0)

#include "em_assert.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup VDAC
 * @brief Digital to Analog Voltage Converter (VDAC) Peripheral API
 *
 * @details
 *  This module contains functions to control the VDAC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The VDAC converts digital values to analog
 *  signals at up to 500 ksps with 12-bit accuracy. The VDAC is designed for
 *  low energy consumption, but can also provide very good performance.
 *
 *  The following steps are necessary for basic operation:
 *
 *  Clock enable:
 *  @code
    CMU_ClockEnable(cmuClock_VDAC0, true);@endcode
 *
 *  Initialize the VDAC with default settings and modify selected fields:
 *  @code
    VDAC_Init_TypeDef vdacInit          = VDAC_INIT_DEFAULT;
    VDAC_InitChannel_TypeDef vdacChInit = VDAC_INITCHANNEL_DEFAULT;

    // Set prescaler to get 1 MHz VDAC clock frequency.
    vdacInit.prescaler = VDAC_PrescaleCalc(1000000, true, 0);
    VDAC_Init(VDAC0, &vdacInit);

    vdacChInit.enable = true;
    VDAC_InitChannel(VDAC0, &vdacChInit, 0);@endcode
 *
 *  Perform a conversion:
 *  @code
    VDAC_ChannelOutputSet(VDAC0, 0, 250);@endcode
 *
 * @note The output stage of a VDAC channel consist of an onchip operational
 *   amplifier in the OPAMP module. This opamp is highly configurable and to
 *   exploit the VDAC functionality fully, you might need to configure the opamp
 *   using the OPAMP API. By using the OPAMP API you will also load opamp
 *   calibration values. The default (reset) settings of the opamp will be
 *   sufficient for many applications.
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of VDAC register block pointer reference for assert statements.*/
#define VDAC_REF_VALID(ref)   ((ref) == VDAC0)

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Channel refresh period. */
typedef enum
{
  vdacRefresh8  = _VDAC_CTRL_REFRESHPERIOD_8CYCLES,  /**< Refresh every 8 clock cycles. */
  vdacRefresh16 = _VDAC_CTRL_REFRESHPERIOD_16CYCLES, /**< Refresh every 16 clock cycles. */
  vdacRefresh32 = _VDAC_CTRL_REFRESHPERIOD_32CYCLES, /**< Refresh every 32 clock cycles. */
  vdacRefresh64 = _VDAC_CTRL_REFRESHPERIOD_64CYCLES, /**< Refresh every 64 clock cycles. */
} VDAC_Refresh_TypeDef;

/** Reference voltage for VDAC. */
typedef enum
{
  vdacRef1V25Ln = _VDAC_CTRL_REFSEL_1V25LN, /**< Internal low noise 1.25 V bandgap reference. */
  vdacRef2V5Ln  = _VDAC_CTRL_REFSEL_2V5LN,  /**< Internal low noise 2.5 V bandgap reference. */
  vdacRef1V25   = _VDAC_CTRL_REFSEL_1V25,   /**< Internal 1.25 V bandgap reference. */
  vdacRef2V5    = _VDAC_CTRL_REFSEL_2V5,    /**< Internal 2.5 V bandgap reference. */
  vdacRefAvdd   = _VDAC_CTRL_REFSEL_VDD,    /**< AVDD reference. */
  vdacRefExtPin = _VDAC_CTRL_REFSEL_EXT,    /**< External pin reference. */
} VDAC_Ref_TypeDef;

/** Peripheral Reflex System signal used to trig VDAC channel conversion. */
typedef enum
{
  vdacPrsSelCh0 =  _VDAC_CH0CTRL_PRSSEL_PRSCH0 , /**< PRS ch 0 triggers conversion. */
  vdacPrsSelCh1 =  _VDAC_CH0CTRL_PRSSEL_PRSCH1 , /**< PRS ch 1 triggers conversion. */
  vdacPrsSelCh2 =  _VDAC_CH0CTRL_PRSSEL_PRSCH2 , /**< PRS ch 2 triggers conversion. */
  vdacPrsSelCh3 =  _VDAC_CH0CTRL_PRSSEL_PRSCH3 , /**< PRS ch 3 triggers conversion. */
  vdacPrsSelCh4 =  _VDAC_CH0CTRL_PRSSEL_PRSCH4 , /**< PRS ch 4 triggers conversion. */
  vdacPrsSelCh5 =  _VDAC_CH0CTRL_PRSSEL_PRSCH5 , /**< PRS ch 5 triggers conversion. */
  vdacPrsSelCh6 =  _VDAC_CH0CTRL_PRSSEL_PRSCH6 , /**< PRS ch 6 triggers conversion. */
  vdacPrsSelCh7 =  _VDAC_CH0CTRL_PRSSEL_PRSCH7 , /**< PRS ch 7 triggers conversion. */
  vdacPrsSelCh8 =  _VDAC_CH0CTRL_PRSSEL_PRSCH8 , /**< PRS ch 8 triggers conversion. */
  vdacPrsSelCh9 =  _VDAC_CH0CTRL_PRSSEL_PRSCH9 , /**< PRS ch 9 triggers conversion. */
  vdacPrsSelCh10 = _VDAC_CH0CTRL_PRSSEL_PRSCH10, /**< PRS ch 10 triggers conversion. */
  vdacPrsSelCh11 = _VDAC_CH0CTRL_PRSSEL_PRSCH11, /**< PRS ch 11 triggers conversion. */
} VDAC_PrsSel_TypeDef;

/** Channel conversion trigger mode. */
typedef enum
{
  vdacTrigModeSw        = _VDAC_CH0CTRL_TRIGMODE_SW,        /**< Channel is triggered by CHnDATA or COMBDATA write. */
  vdacTrigModePrs       = _VDAC_CH0CTRL_TRIGMODE_PRS,       /**< Channel is triggered by PRS input. */
  vdacTrigModeRefresh   = _VDAC_CH0CTRL_TRIGMODE_REFRESH,   /**< Channel is triggered by Refresh timer. */
  vdacTrigModeSwPrs     = _VDAC_CH0CTRL_TRIGMODE_SWPRS,     /**< Channel is triggered by CHnDATA/COMBDATA write or PRS input. */
  vdacTrigModeSwRefresh = _VDAC_CH0CTRL_TRIGMODE_SWREFRESH, /**< Channel is triggered by CHnDATA/COMBDATA write or Refresh timer. */
  vdacTrigModeLesense   = _VDAC_CH0CTRL_TRIGMODE_LESENSE,   /**< Channel is triggered by LESENSE. */
} VDAC_TrigMode_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** VDAC init structure, common for both channels. */
typedef struct
{
  /** Select between main and alternate output path calibration values. */
  bool                  mainCalibration;

  /** Selects clock from asynchronous or synchronous (with respect to
      peripheral clock) source */
  bool                  asyncClockMode;

  /** Warmup mode, keep VDAC on (in idle) - or shutdown between conversions.*/
  bool                  warmupKeepOn;

  /** Channel refresh period. */
  VDAC_Refresh_TypeDef  refresh;

  /** Prescaler for VDAC clock. Clock is source clock divided by prescaler+1. */
  uint32_t              prescaler;

  /** Reference voltage to use. */
  VDAC_Ref_TypeDef      reference;

  /** Enable/disable reset of prescaler on CH 0 start. */
  bool                 ch0ResetPre;

  /** Enable/disable output enable control by CH1 PRS signal. */
  bool                 outEnablePRS;

  /** Enable/disable sine mode. */
  bool                 sineEnable;

  /** Select if single ended or differential output mode. */
  bool                 diff;
} VDAC_Init_TypeDef;

/** Default config for VDAC init structure. */
#define VDAC_INIT_DEFAULT                                                 \
{                                                                         \
  true,                   /* Use main output path calibration values. */  \
  false,                  /* Use synchronous clock mode. */               \
  false,                  /* Turn off between sample off conversions.*/   \
  vdacRefresh8,           /* Refresh every 8th cycle. */                  \
  0,                      /* No prescaling. */                            \
  vdacRef1V25Ln,          /* 1.25V internal low noise reference. */       \
  false,                  /* Do not reset prescaler on CH 0 start. */     \
  false,                  /* VDAC output enable always on. */             \
  false,                  /* Disable sine mode. */                        \
  false                   /* Single ended mode. */                        \
}

/** VDAC channel init structure. */
typedef struct
{
  /** Enable channel. */
  bool                  enable;

  /**
   * Peripheral reflex system trigger selection. Only applicable if @p trigMode
   * is set to @p vdacTrigModePrs or @p vdacTrigModeSwPrs. */
  VDAC_PrsSel_TypeDef   prsSel;

  /** Treat the PRS signal asynchronously. */
  bool                  prsAsync;

  /** Channel conversion trigger mode. */
  VDAC_TrigMode_TypeDef trigMode;

  /** Set channel conversion mode to sample/shut-off mode. Default is
   *  continous.*/
  bool                  sampleOffMode;
} VDAC_InitChannel_TypeDef;

/** Default config for VDAC channel init structure. */
#define VDAC_INITCHANNEL_DEFAULT                                              \
{                                                                             \
  false,              /* Leave channel disabled when init done. */            \
  vdacPrsSelCh0,      /* PRS CH 0 triggers conversion. */                     \
  false,              /* Treat PRS channel as a synchronous signal. */        \
  vdacTrigModeSw,     /* Conversion trigged by CH0DATA or COMBDATA write. */  \
  false,              /* Channel conversion set to continous. */              \
}

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void VDAC_ChannelOutputSet(VDAC_TypeDef *vdac,
                           unsigned int channel,
                           uint32_t     value);
void VDAC_Enable(VDAC_TypeDef *vdac, unsigned int ch, bool enable);
void VDAC_Init(VDAC_TypeDef *vdac, const VDAC_Init_TypeDef *init);
void VDAC_InitChannel(VDAC_TypeDef *vdac,
                      const VDAC_InitChannel_TypeDef *init,
                      unsigned int ch);

/***************************************************************************//**
 * @brief
 *   Set the output signal of VDAC channel 0 to a given value.
 *
 * @details
 *   This function sets the output signal of VDAC channel 0 by writing @p value
 *   to the CH0DATA register.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] value
 *   Value to write to channel 0 output register CH0DATA.
 ******************************************************************************/
__STATIC_INLINE void VDAC_Channel0OutputSet(VDAC_TypeDef *vdac,
                                            uint32_t value)
{
  EFM_ASSERT(value<=_VDAC_CH0DATA_MASK);
  vdac->CH0DATA = value;
}

/***************************************************************************//**
 * @brief
 *   Set the output signal of VDAC channel 1 to a given value.
 *
 * @details
 *   This function sets the output signal of VDAC channel 1 by writing @p value
 *   to the CH1DATA register.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] value
 *   Value to write to channel 1 output register CH1DATA.
 ******************************************************************************/
__STATIC_INLINE void VDAC_Channel1OutputSet(VDAC_TypeDef *vdac,
                                            uint32_t value)
{
  EFM_ASSERT(value<=_VDAC_CH1DATA_MASK);
  vdac->CH1DATA = value;
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending VDAC interrupts.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] flags
 *   Pending VDAC interrupt source to clear. Use a bitwise logic OR combination
 *   of valid interrupt flags for the VDAC module (VDAC_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void VDAC_IntClear(VDAC_TypeDef *vdac, uint32_t flags)
{
  vdac->IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable one or more VDAC interrupts.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] flags
 *   VDAC interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the VDAC module (VDAC_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void VDAC_IntDisable(VDAC_TypeDef *vdac, uint32_t flags)
{
  vdac->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more VDAC interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using VDAC_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] flags
 *   VDAC interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the VDAC module (VDAC_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void VDAC_IntEnable(VDAC_TypeDef *vdac, uint32_t flags)
{
  vdac->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending VDAC interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @return
 *   VDAC interrupt sources pending. A bitwise logic OR combination of valid
 *   interrupt flags for the VDAC module (VDAC_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t VDAC_IntGet(VDAC_TypeDef *vdac)
{
  return vdac->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending VDAC interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled VDAC interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in VDACx_IEN_nnn
 *     register (VDACx_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the VDAC module
 *     (VDACx_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t VDAC_IntGetEnabled(VDAC_TypeDef *vdac)
{
  uint32_t ien = vdac->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return vdac->IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending VDAC interrupts from SW.
 *
 * @param[in] vdac
 *   Pointer to VDAC peripheral register block.
 *
 * @param[in] flags
 *   VDAC interrupt sources to set to pending. Use a bitwise logic OR
 *   combination of valid interrupt flags for the VDAC module (VDAC_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void VDAC_IntSet(VDAC_TypeDef *vdac, uint32_t flags)
{
  vdac->IFS = flags;
}

uint32_t VDAC_PrescaleCalc(uint32_t vdacFreq, bool syncMode, uint32_t hfperFreq);
void VDAC_Reset(VDAC_TypeDef *vdac);

/** @} (end addtogroup VDAC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(VDAC_COUNT) && (VDAC_COUNT > 0) */
#endif /* EM_VDAC_H */

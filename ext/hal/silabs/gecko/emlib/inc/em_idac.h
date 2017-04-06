/***************************************************************************//**
 * @file em_idac.h
 * @brief Current Digital to Analog Converter (IDAC) peripheral API
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

#ifndef EM_IDAC_H
#define EM_IDAC_H

#include "em_device.h"

#if defined(IDAC_COUNT) && (IDAC_COUNT > 0)
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup IDAC
 * @brief
 *  Current Digital-to-Analog Converter (IDAC) Peripheral API
 *
 * @details
 *  The current digital-to-analog converter (IDAC) can source or sink a configurable
 *  constant current, which can be output on, or sinked from pin or ADC. The current
 *  is configurable with several ranges of various step sizes. The IDAC can be used
 *  with PRS and it can operate down to EM3.
 *
 *  The following steps are necessary for basic operation:
 *
 *  Clock enable:
 *  @include em_idac_clock_enable.c
 *
 *  Initialize the peripheral with default settings and modify selected fields such as
 *  output select:
 *  @if DOXYDOC_P1_DEVICE
 *  @include em_idac_init_adc.c
 *  @endif
 *  @if DOXYDOC_P2_DEVICE
 *  @include em_idac_init_aport.c
 *  @endif
 *
 *  Set output:
 *  @include em_idac_basic_usage.c
 *
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of IDAC register block pointer reference for assert statements. */
#define IDAC_REF_VALID(ref)    ((ref) == IDAC0)

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Output mode. */
typedef enum
{
#if defined( _IDAC_CTRL_OUTMODE_MASK )
  idacOutputPin     = IDAC_CTRL_OUTMODE_PIN,     /**< Output to IDAC OUT pin */
  idacOutputADC     = IDAC_CTRL_OUTMODE_ADC      /**< Output to ADC */
#elif ( _IDAC_CTRL_APORTOUTSEL_MASK )
  idacOutputAPORT1XCH0 = IDAC_CTRL_APORTOUTSEL_APORT1XCH0, /**< Output to APORT 1X CH0 */
  idacOutputAPORT1YCH1 = IDAC_CTRL_APORTOUTSEL_APORT1YCH1, /**< Output to APORT 1Y CH1 */
  idacOutputAPORT1XCH2 = IDAC_CTRL_APORTOUTSEL_APORT1XCH2, /**< Output to APORT 1X CH2 */
  idacOutputAPORT1YCH3 = IDAC_CTRL_APORTOUTSEL_APORT1YCH3, /**< Output to APORT 1Y CH3 */
  idacOutputAPORT1XCH4 = IDAC_CTRL_APORTOUTSEL_APORT1XCH4, /**< Output to APORT 1X CH4 */
  idacOutputAPORT1YCH5 = IDAC_CTRL_APORTOUTSEL_APORT1YCH5, /**< Output to APORT 1Y CH5 */
  idacOutputAPORT1XCH6 = IDAC_CTRL_APORTOUTSEL_APORT1XCH6, /**< Output to APORT 1X CH6 */
  idacOutputAPORT1YCH7 = IDAC_CTRL_APORTOUTSEL_APORT1YCH7, /**< Output to APORT 1Y CH7 */
  idacOutputAPORT1XCH8 = IDAC_CTRL_APORTOUTSEL_APORT1XCH8, /**< Output to APORT 1X CH8 */
  idacOutputAPORT1YCH9 = IDAC_CTRL_APORTOUTSEL_APORT1YCH9, /**< Output to APORT 1Y CH9 */
  idacOutputAPORT1XCH10 = IDAC_CTRL_APORTOUTSEL_APORT1XCH10, /**< Output to APORT 1X CH10 */
  idacOutputAPORT1YCH11 = IDAC_CTRL_APORTOUTSEL_APORT1YCH11, /**< Output to APORT 1Y CH11 */
  idacOutputAPORT1XCH12 = IDAC_CTRL_APORTOUTSEL_APORT1XCH12, /**< Output to APORT 1X CH12 */
  idacOutputAPORT1YCH13 = IDAC_CTRL_APORTOUTSEL_APORT1YCH13, /**< Output to APORT 1Y CH13 */
  idacOutputAPORT1XCH14 = IDAC_CTRL_APORTOUTSEL_APORT1XCH14, /**< Output to APORT 1X CH14 */
  idacOutputAPORT1YCH15 = IDAC_CTRL_APORTOUTSEL_APORT1YCH15, /**< Output to APORT 1Y CH15 */
  idacOutputAPORT1XCH16 = IDAC_CTRL_APORTOUTSEL_APORT1XCH16, /**< Output to APORT 1X CH16 */
  idacOutputAPORT1YCH17 = IDAC_CTRL_APORTOUTSEL_APORT1YCH17, /**< Output to APORT 1Y CH17 */
  idacOutputAPORT1XCH18 = IDAC_CTRL_APORTOUTSEL_APORT1XCH18, /**< Output to APORT 1X CH18 */
  idacOutputAPORT1YCH19 = IDAC_CTRL_APORTOUTSEL_APORT1YCH19, /**< Output to APORT 1Y CH19 */
  idacOutputAPORT1XCH20 = IDAC_CTRL_APORTOUTSEL_APORT1XCH20, /**< Output to APORT 1X CH20 */
  idacOutputAPORT1YCH21 = IDAC_CTRL_APORTOUTSEL_APORT1YCH21, /**< Output to APORT 1Y CH21 */
  idacOutputAPORT1XCH22 = IDAC_CTRL_APORTOUTSEL_APORT1XCH22, /**< Output to APORT 1X CH22 */
  idacOutputAPORT1YCH23 = IDAC_CTRL_APORTOUTSEL_APORT1YCH23, /**< Output to APORT 1Y CH23 */
  idacOutputAPORT1XCH24 = IDAC_CTRL_APORTOUTSEL_APORT1XCH24, /**< Output to APORT 1X CH24 */
  idacOutputAPORT1YCH25 = IDAC_CTRL_APORTOUTSEL_APORT1YCH25, /**< Output to APORT 1Y CH25 */
  idacOutputAPORT1XCH26 = IDAC_CTRL_APORTOUTSEL_APORT1XCH26, /**< Output to APORT 1X CH26 */
  idacOutputAPORT1YCH27 = IDAC_CTRL_APORTOUTSEL_APORT1YCH27, /**< Output to APORT 1Y CH27 */
  idacOutputAPORT1XCH28 = IDAC_CTRL_APORTOUTSEL_APORT1XCH28, /**< Output to APORT 1X CH28 */
  idacOutputAPORT1YCH29 = IDAC_CTRL_APORTOUTSEL_APORT1YCH29, /**< Output to APORT 1Y CH29 */
  idacOutputAPORT1XCH30 = IDAC_CTRL_APORTOUTSEL_APORT1XCH30, /**< Output to APORT 1X CH30 */
  idacOutputAPORT1YCH31 = IDAC_CTRL_APORTOUTSEL_APORT1YCH31, /**< Output to APORT 1Y CH31 */
#endif
} IDAC_OutMode_TypeDef;


/** Selects which Peripheral Reflex System (PRS) signal to use when
    PRS is set to control the IDAC output. */
typedef enum
{
  idacPRSSELCh0 = IDAC_CTRL_PRSSEL_PRSCH0,      /**< PRS channel 0. */
  idacPRSSELCh1 = IDAC_CTRL_PRSSEL_PRSCH1,      /**< PRS channel 1. */
  idacPRSSELCh2 = IDAC_CTRL_PRSSEL_PRSCH2,      /**< PRS channel 2. */
  idacPRSSELCh3 = IDAC_CTRL_PRSSEL_PRSCH3,      /**< PRS channel 3. */
#if defined( IDAC_CTRL_PRSSEL_PRSCH4 )
  idacPRSSELCh4 = IDAC_CTRL_PRSSEL_PRSCH4,      /**< PRS channel 4. */
  idacPRSSELCh5 = IDAC_CTRL_PRSSEL_PRSCH5,      /**< PRS channel 5. */
#endif
#if defined( IDAC_CTRL_PRSSEL_PRSCH6 )
  idacPRSSELCh6 = IDAC_CTRL_PRSSEL_PRSCH6,      /**< PRS channel 6. */
  idacPRSSELCh7 = IDAC_CTRL_PRSSEL_PRSCH7,      /**< PRS channel 7. */
  idacPRSSELCh8 = IDAC_CTRL_PRSSEL_PRSCH8,      /**< PRS channel 8. */
  idacPRSSELCh9 = IDAC_CTRL_PRSSEL_PRSCH9,      /**< PRS channel 9. */
  idacPRSSELCh10 = IDAC_CTRL_PRSSEL_PRSCH10,    /**< PRS channel 10 */
  idacPRSSELCh11 = IDAC_CTRL_PRSSEL_PRSCH11,    /**< PRS channel 11 */
#endif
} IDAC_PRSSEL_TypeDef;


/** Selects which current range to use. */
typedef enum
{
  idacCurrentRange0 = IDAC_CURPROG_RANGESEL_RANGE0, /**< current range 0. */
  idacCurrentRange1 = IDAC_CURPROG_RANGESEL_RANGE1, /**< current range 1. */
  idacCurrentRange2 = IDAC_CURPROG_RANGESEL_RANGE2, /**< current range 2. */
  idacCurrentRange3 = IDAC_CURPROG_RANGESEL_RANGE3, /**< current range 3. */
} IDAC_Range_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** IDAC init structure, common for both channels. */
typedef struct
{
  /** Enable IDAC. */
  bool                  enable;

  /** Output mode */
  IDAC_OutMode_TypeDef  outMode;

  /**
   * Enable Peripheral reflex system (PRS) to control IDAC output. If false,
   * the IDAC output is controlled by writing to IDAC_OUTEN in IDAC_CTRL or
   * by calling IDAC_OutEnable().
   */
  bool                  prsEnable;

  /**
   * Peripheral reflex system channel selection. Only applicable if @p prsEnable
   * is enabled.
   */
  IDAC_PRSSEL_TypeDef   prsSel;

  /** Enable/disable current sink mode. */
  bool                  sinkEnable;

} IDAC_Init_TypeDef;

/** Default config for IDAC init structure. */
#if defined( _IDAC_CTRL_OUTMODE_MASK )
#define IDAC_INIT_DEFAULT                                              \
{                                                                      \
  false,          /**< Leave IDAC disabled when init done. */          \
  idacOutputPin,  /**< Output to IDAC output pin. */                   \
  false,          /**< Disable PRS triggering. */                      \
  idacPRSSELCh0,  /**< Select PRS ch0 (if PRS triggering enabled). */  \
  false           /**< Disable current sink mode. */                   \
}
#elif ( _IDAC_CTRL_APORTOUTSEL_MASK )
#define IDAC_INIT_DEFAULT                                              \
{                                                                      \
  false,          /**< Leave IDAC disabled when init done. */          \
  idacOutputAPORT1XCH0, /**< Output to APORT. */                       \
  false,          /**< Disable PRS triggering. */                      \
  idacPRSSELCh0,  /**< Select PRS ch0 (if PRS triggering enabled). */  \
  false           /**< Disable current sink mode. */                   \
}
#endif


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/


void IDAC_Init(IDAC_TypeDef *idac, const IDAC_Init_TypeDef *init);
void IDAC_Enable(IDAC_TypeDef *idac, bool enable);
void IDAC_Reset(IDAC_TypeDef *idac);
void IDAC_MinimalOutputTransitionMode(IDAC_TypeDef *idac, bool enable);
void IDAC_RangeSet(IDAC_TypeDef *idac, const IDAC_Range_TypeDef range);
void IDAC_StepSet(IDAC_TypeDef *idac, const uint32_t step);
void IDAC_OutEnable(IDAC_TypeDef *idac, bool enable);


#if defined( _IDAC_IEN_MASK )
/***************************************************************************//**
 * @brief
 *   Clear one or more pending IDAC interrupts.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] flags
 *   Pending IDAC interrupt source(s) to clear. Use one or more valid
 *   interrupt flags for the IDAC module (IDAC_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void IDAC_IntClear(IDAC_TypeDef *idac, uint32_t flags)
{
  idac->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more IDAC interrupts.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] flags
 *   IDAC interrupt source(s) to disable. Use one or more valid
 *   interrupt flags for the IDAC module (IDAC_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void IDAC_IntDisable(IDAC_TypeDef *idac, uint32_t flags)
{
  idac->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more IDAC interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using IDAC_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] flags
 *   IDAC interrupt source(s) to enable. Use one or more valid
 *   interrupt flags for the IDAC module (IDAC_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void IDAC_IntEnable(IDAC_TypeDef *idac, uint32_t flags)
{
  idac->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending IDAC interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @return
 *   IDAC interrupt source(s) pending. Returns one or more valid
 *   interrupt flags for the IDAC module (IDAC_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE uint32_t IDAC_IntGet(IDAC_TypeDef *idac)
{
  return idac->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending IDAC interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled IDAC interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in IDACx_IEN_nnn
 *     register (IDACx_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the IDAC module
 *     (IDACx_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t IDAC_IntGetEnabled(IDAC_TypeDef *idac)
{
  uint32_t ien;

  /* Store flags in temporary variable in order to define explicit order
   * of volatile accesses. */
  ien = idac->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return idac->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending IDAC interrupts from SW.
 *
 * @param[in] idac
 *   Pointer to IDAC peripheral register block.
 *
 * @param[in] flags
 *   IDAC interrupt source(s) to set to pending. Use one or more valid
 *   interrupt flags for the IDAC module (IDAC_IF_nnn) OR'ed together.
 ******************************************************************************/
__STATIC_INLINE void IDAC_IntSet(IDAC_TypeDef *idac, uint32_t flags)
{
  idac->IFS = flags;
}
#endif


/** @} (end addtogroup IDAC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(IDAC_COUNT) && (IDAC_COUNT > 0) */

#endif /* EM_IDAC_H */

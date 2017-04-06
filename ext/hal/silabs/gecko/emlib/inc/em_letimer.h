/***************************************************************************//**
 * @file em_letimer.h
 * @brief Low Energy Timer (LETIMER) peripheral API
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

#ifndef EM_LETIMER_H
#define EM_LETIMER_H

#include <stdbool.h>
#include "em_device.h"
#if defined(LETIMER_COUNT) && (LETIMER_COUNT > 0)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LETIMER
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Repeat mode. */
typedef enum
{
  /** Count until stopped by SW. */
  letimerRepeatFree     = _LETIMER_CTRL_REPMODE_FREE,
  /** Count REP0 times. */
  letimerRepeatOneshot  = _LETIMER_CTRL_REPMODE_ONESHOT,
  /**
   * Count REP0 times, if REP1 has been written to, it is loaded into
   * REP0 when REP0 is about to be decremented to 0.
   */
  letimerRepeatBuffered = _LETIMER_CTRL_REPMODE_BUFFERED,
  /**
   * Run as long as both REP0 and REP1 are not 0. Both REP0 and REP1
   * are decremented when counter underflows.
   */
  letimerRepeatDouble   = _LETIMER_CTRL_REPMODE_DOUBLE
} LETIMER_RepeatMode_TypeDef;


/** Underflow action on output. */
typedef enum
{
  /** No output action. */
  letimerUFOANone   = _LETIMER_CTRL_UFOA0_NONE,
  /** Toggle output when counter underflows. */
  letimerUFOAToggle = _LETIMER_CTRL_UFOA0_TOGGLE,
  /** Hold output one LETIMER clock cycle when counter underflows. */
  letimerUFOAPulse  = _LETIMER_CTRL_UFOA0_PULSE,
  /** Set output idle when counter underflows, and active when matching COMP1. */
  letimerUFOAPwm    = _LETIMER_CTRL_UFOA0_PWM
} LETIMER_UFOA_TypeDef;

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** LETIMER initialization structure. */
typedef struct
{
  bool                       enable;         /**< Start counting when init completed. */
  bool                       debugRun;       /**< Counter shall keep running during debug halt. */
#if defined(LETIMER_CTRL_RTCC0TEN)
  bool                       rtcComp0Enable; /**< Start counting on RTC COMP0 match. */
  bool                       rtcComp1Enable; /**< Start counting on RTC COMP1 match. */
#endif
  bool                       comp0Top;       /**< Load COMP0 register into CNT when counter underflows. */
  bool                       bufTop;         /**< Load COMP1 into COMP0 when REP0 reaches 0. */
  uint8_t                    out0Pol;        /**< Idle value for output 0. */
  uint8_t                    out1Pol;        /**< Idle value for output 1. */
  LETIMER_UFOA_TypeDef       ufoa0;          /**< Underflow output 0 action. */
  LETIMER_UFOA_TypeDef       ufoa1;          /**< Underflow output 1 action. */
  LETIMER_RepeatMode_TypeDef repMode;        /**< Repeat mode. */
} LETIMER_Init_TypeDef;

/** Default config for LETIMER init structure. */
#if defined(LETIMER_CTRL_RTCC0TEN)
#define LETIMER_INIT_DEFAULT                                                  \
{                                                                             \
  true,               /* Enable timer when init complete. */                  \
  false,              /* Stop counter during debug halt. */                   \
  false,              /* Do not start counting on RTC COMP0 match. */         \
  false,              /* Do not start counting on RTC COMP1 match. */         \
  false,              /* Do not load COMP0 into CNT on underflow. */          \
  false,              /* Do not load COMP1 into COMP0 when REP0 reaches 0. */ \
  0,                  /* Idle value 0 for output 0. */                        \
  0,                  /* Idle value 0 for output 1. */                        \
  letimerUFOANone,    /* No action on underflow on output 0. */               \
  letimerUFOANone,    /* No action on underflow on output 1. */               \
  letimerRepeatFree   /* Count until stopped by SW. */                        \
}
#else
#define LETIMER_INIT_DEFAULT                                                  \
{                                                                             \
  true,               /* Enable timer when init complete. */                  \
  false,              /* Stop counter during debug halt. */                   \
  false,              /* Do not load COMP0 into CNT on underflow. */          \
  false,              /* Do not load COMP1 into COMP0 when REP0 reaches 0. */ \
  0,                  /* Idle value 0 for output 0. */                        \
  0,                  /* Idle value 0 for output 1. */                        \
  letimerUFOANone,    /* No action on underflow on output 0. */               \
  letimerUFOANone,    /* No action on underflow on output 1. */               \
  letimerRepeatFree   /* Count until stopped by SW. */                        \
}
#endif

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

uint32_t LETIMER_CompareGet(LETIMER_TypeDef *letimer, unsigned int comp);
void LETIMER_CompareSet(LETIMER_TypeDef *letimer,
                        unsigned int comp,
                        uint32_t value);


/***************************************************************************//**
 * @brief
 *   Get LETIMER counter value.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @return
 *   Current LETIMER counter value.
 ******************************************************************************/
__STATIC_INLINE uint32_t LETIMER_CounterGet(LETIMER_TypeDef *letimer)
{
  return(letimer->CNT);
}


void LETIMER_Enable(LETIMER_TypeDef *letimer, bool enable);
#if defined(_LETIMER_FREEZE_MASK)
void LETIMER_FreezeEnable(LETIMER_TypeDef *letimer, bool enable);
#endif
void LETIMER_Init(LETIMER_TypeDef *letimer, const LETIMER_Init_TypeDef *init);


/***************************************************************************//**
 * @brief
 *   Clear one or more pending LETIMER interrupts.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @param[in] flags
 *   Pending LETIMER interrupt source to clear. Use a bitwise logic OR
 *    combination of valid interrupt flags for the LETIMER module
 *    (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LETIMER_IntClear(LETIMER_TypeDef *letimer, uint32_t flags)
{
  letimer->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more LETIMER interrupts.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @param[in] flags
 *   LETIMER interrupt sources to disable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the LETIMER module (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LETIMER_IntDisable(LETIMER_TypeDef *letimer, uint32_t flags)
{
  letimer->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more LETIMER interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using LETIMER_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @param[in] flags
 *   LETIMER interrupt sources to enable. Use a bitwise logic OR combination of
 *   valid interrupt flags for the LETIMER module (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LETIMER_IntEnable(LETIMER_TypeDef *letimer, uint32_t flags)
{
  letimer->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending LETIMER interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @return
 *   LETIMER interrupt sources pending. A bitwise logic OR combination of
 *    valid interrupt flags for the LETIMER module (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LETIMER_IntGet(LETIMER_TypeDef *letimer)
{
  return letimer->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending LETIMER interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @return
 *   Pending and enabled LETIMER interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in LETIMER_IEN_nnn
 *   register (LETIMER_IEN_nnn) and
 *   - the OR combination of valid interrupt flags of the LETIMER module
 *   (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LETIMER_IntGetEnabled(LETIMER_TypeDef *letimer)
{
  uint32_t ien;


  /* Store flags in temporary variable in order to define explicit order
   * of volatile accesses. */
  ien = letimer->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return letimer->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending LETIMER interrupts from SW.
 *
 * @param[in] letimer
 *   Pointer to LETIMER peripheral register block.
 *
 * @param[in] flags
 *   LETIMER interrupt sources to set to pending. Use a bitwise logic OR
 *   combination of valid interrupt flags for the LETIMER module (LETIMER_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void LETIMER_IntSet(LETIMER_TypeDef *letimer, uint32_t flags)
{
  letimer->IFS = flags;
}


uint32_t LETIMER_RepeatGet(LETIMER_TypeDef *letimer, unsigned int rep);
void LETIMER_RepeatSet(LETIMER_TypeDef *letimer,
                       unsigned int rep,
                       uint32_t value);
void LETIMER_Reset(LETIMER_TypeDef *letimer);


/** @} (end addtogroup LETIMER) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(LETIMER_COUNT) && (LETIMER_COUNT > 0) */
#endif /* EM_LETIMER_H */

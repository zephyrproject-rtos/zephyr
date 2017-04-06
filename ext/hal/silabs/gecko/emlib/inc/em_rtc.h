/***************************************************************************//**
 * @file em_rtc.h
 * @brief Real Time Counter (RTC) peripheral API
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

#ifndef EM_RTC_H
#define EM_RTC_H

#include "em_device.h"
#if defined(RTC_COUNT) && (RTC_COUNT > 0)

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup RTC
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** RTC initialization structure. */
typedef struct
{
  bool enable;   /**< Start counting when init completed. */
  bool debugRun; /**< Counter shall keep running during debug halt. */
  bool comp0Top; /**< Use compare register 0 as max count value. */
} RTC_Init_TypeDef;

/** Suggested default config for RTC init structure. */
#define RTC_INIT_DEFAULT                                     \
{                                                            \
  true,    /* Start counting when init done */               \
  false,   /* Disable updating during debug halt */          \
  true     /* Restart counting from 0 when reaching COMP0 */ \
}


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

uint32_t RTC_CompareGet(unsigned int comp);
void RTC_CompareSet(unsigned int comp, uint32_t value);

/***************************************************************************//**
 * @brief
 *   Get RTC counter value.
 *
 * @return
 *   Current RTC counter value.
 ******************************************************************************/
__STATIC_INLINE uint32_t RTC_CounterGet(void)
{
  return RTC->CNT;
}

#if !defined(_EFM32_GECKO_FAMILY)
/***************************************************************************//**
 * @brief
 *   Set the RTC counter value.
 *
 * @param[in] value
 *   The new RTC counter value.
 ******************************************************************************/
__STATIC_INLINE void RTC_CounterSet(uint32_t value)
{
  RTC->CNT = value;
}
#endif

void RTC_CounterReset(void);
void RTC_Enable(bool enable);
void RTC_FreezeEnable(bool enable);
void RTC_Init(const RTC_Init_TypeDef *init);

/***************************************************************************//**
 * @brief
 *   Clear one or more pending RTC interrupts.
 *
 * @param[in] flags
 *   RTC interrupt sources to clear. Use a set of interrupt flags OR-ed
 *   together to clear multiple interrupt sources for the RTC module
 *   (RTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void RTC_IntClear(uint32_t flags)
{
  RTC->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more RTC interrupts.
 *
 * @param[in] flags
 *   RTC interrupt sources to disable. Use a set of interrupt flags OR-ed
 *   together to disable multiple interrupt sources for the RTC module
 *   (RTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void RTC_IntDisable(uint32_t flags)
{
  RTC->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more RTC interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using RTC_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   RTC interrupt sources to enable. Use a set of interrupt flags OR-ed
 *   together to set multiple interrupt sources for the RTC module
 *   (RTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void RTC_IntEnable(uint32_t flags)
{
  RTC->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending RTC interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   Pending RTC interrupt sources. Returns a set of interrupt flags OR-ed
 *   together for multiple interrupt sources in the RTC module (RTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t RTC_IntGet(void)
{
  return RTC->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending RTC interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled RTC interrupt sources
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in RTC_IEN and
 *   - the pending interrupt flags RTC_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t RTC_IntGetEnabled(void)
{
  uint32_t ien;

  ien = RTC->IEN;
  return RTC->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending RTC interrupts from SW.
 *
 * @param[in] flags
 *   RTC interrupt sources to set to pending. Use a set of interrupt flags
 *   OR-ed together to set multiple interrupt sources for the RTC module
 *   (RTC_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void RTC_IntSet(uint32_t flags)
{
  RTC->IFS = flags;
}

void RTC_Reset(void);

/** @} (end addtogroup RTC) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(RTC_COUNT) && (RTC_COUNT > 0) */
#endif /* EM_RTC_H */

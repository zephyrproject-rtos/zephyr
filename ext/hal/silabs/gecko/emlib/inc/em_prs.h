/***************************************************************************//**
 * @file em_prs.h
 * @brief Peripheral Reflex System (PRS) peripheral API
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

#ifndef EM_PRS_H
#define EM_PRS_H

#include "em_device.h"
#if defined(PRS_COUNT) && (PRS_COUNT > 0)

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup PRS
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** Edge detection type. */
typedef enum
{
  prsEdgeOff  = PRS_CH_CTRL_EDSEL_OFF,      /**< Leave signal as is. */
  prsEdgePos  = PRS_CH_CTRL_EDSEL_POSEDGE,  /**< Generate pules on positive edge. */
  prsEdgeNeg  = PRS_CH_CTRL_EDSEL_NEGEDGE,  /**< Generate pules on negative edge. */
  prsEdgeBoth = PRS_CH_CTRL_EDSEL_BOTHEDGES /**< Generate pules on both edges. */
} PRS_Edge_TypeDef;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Set level control bit for one or more channels.
 *
 * @details
 *   The level value for a channel is XORed with both the pulse possible issued
 *   by PRS_PulseTrigger() and the PRS input signal selected for the channel(s).
 *
 * @param[in] level
 *   Level to use for channels indicated by @p mask. Use logical OR combination
 *   of PRS_SWLEVEL_CHnLEVEL defines for channels to set high level, otherwise 0.
 *
 * @param[in] mask
 *   Mask indicating which channels to set level for. Use logical OR combination
 *   of PRS_SWLEVEL_CHnLEVEL defines.
 ******************************************************************************/
__STATIC_INLINE void PRS_LevelSet(uint32_t level, uint32_t mask)
{
  PRS->SWLEVEL = (PRS->SWLEVEL & ~mask) | (level & mask);
}


/***************************************************************************//**
 * @brief
 *   Trigger a high pulse (one HFPERCLK) for one or more channels.
 *
 * @details
 *   Setting a bit for a channel causes the bit in the register to remain high
 *   for one HFPERCLK cycle. The pulse is XORed with both the corresponding bit
 *   in PRS SWLEVEL register and the PRS input signal selected for the
 *   channel(s).
 *
 * @param[in] channels
 *   Logical ORed combination of channels to trigger a pulse for. Use
 *   PRS_SWPULSE_CHnPULSE defines.
 ******************************************************************************/
__STATIC_INLINE void PRS_PulseTrigger(uint32_t channels)
{
  PRS->SWPULSE = channels & _PRS_SWPULSE_MASK;
}

void PRS_SourceSignalSet(unsigned int ch,
                         uint32_t source,
                         uint32_t signal,
                         PRS_Edge_TypeDef edge);

#if defined( PRS_CH_CTRL_ASYNC )
void PRS_SourceAsyncSignalSet(unsigned int ch,
                              uint32_t source,
                              uint32_t signal);
#endif

/** @} (end addtogroup PRS) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(PRS_COUNT) && (PRS_COUNT > 0) */
#endif /* EM_PRS_H */

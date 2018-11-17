/***************************************************************************//**
 * @file em_rmu.h
 * @brief Reset Management Unit (RMU) peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#ifndef EM_RMU_H
#define EM_RMU_H

#include "em_device.h"
#if (defined(RMU_COUNT) && (RMU_COUNT > 0)) || (_EMU_RSTCTRL_MASK)
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
 * @addtogroup RMU
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** RMU reset modes. */
typedef enum {
#if defined(_RMU_CTRL_PINRMODE_MASK)
  rmuResetModeDisabled = _RMU_CTRL_PINRMODE_DISABLED,
  rmuResetModeLimited  = _RMU_CTRL_PINRMODE_LIMITED,
  rmuResetModeExtended = _RMU_CTRL_PINRMODE_EXTENDED,
  rmuResetModeFull     = _RMU_CTRL_PINRMODE_FULL,
#elif defined(_EMU_RSTCTRL_MASK)
  rmuResetModeDisabled = 0,
  rmuResetModeEnabled  = 1,
#else
  rmuResetModeClear    = 0,
  rmuResetModeSet      = 1,
#endif
} RMU_ResetMode_TypeDef;

/** RMU controlled peripheral reset control and reset source control. */
typedef enum {
#if defined(RMU_CTRL_BURSTEN)
  rmuResetBU = _RMU_CTRL_BURSTEN_MASK,              /**< Reset control over Backup Power domain select. */
#endif
#if defined(RMU_CTRL_LOCKUPRDIS)
  rmuResetLockUp = _RMU_CTRL_LOCKUPRDIS_MASK,       /**< Cortex lockup reset select. */
#elif defined(_RMU_CTRL_LOCKUPRMODE_MASK)
  rmuResetLockUp = _RMU_CTRL_LOCKUPRMODE_MASK,      /**< Cortex lockup reset select. */
#endif
#if defined(_RMU_CTRL_WDOGRMODE_MASK)
  rmuResetWdog = _RMU_CTRL_WDOGRMODE_MASK,          /**< WDOG reset select. */
#endif
#if defined(_RMU_CTRL_LOCKUPRMODE_MASK)
  rmuResetCoreLockup = _RMU_CTRL_LOCKUPRMODE_MASK,  /**< Cortex lockup reset select. */
#endif
#if defined(_RMU_CTRL_SYSRMODE_MASK)
  rmuResetSys = _RMU_CTRL_SYSRMODE_MASK,            /**< SYSRESET select. */
#endif
#if defined(_RMU_CTRL_PINRMODE_MASK)
  rmuResetPin = _RMU_CTRL_PINRMODE_MASK,            /**< Pin reset select. */
#endif

#if defined(_EMU_RSTCTRL_WDOG0RMODE_MASK)
  rmuResetWdog0 = _EMU_RSTCTRL_WDOG0RMODE_MASK,        /**< WDOG0 reset select */
#endif
#if defined(_EMU_RSTCTRL_WDOG1RMODE_MASK)
  rmuResetWdog1 = _EMU_RSTCTRL_WDOG1RMODE_MASK,        /**< WDOG1 reset select */
#endif
#if defined(_EMU_RSTCTRL_SYSRMODE_MASK)
  rmuResetSys = _EMU_RSTCTRL_SYSRMODE_MASK,            /**< SYSRESET select */
#endif
#if defined(_EMU_RSTCTRL_LOCKUPRMODE_MASK)
  rmuResetCoreLockup = _EMU_RSTCTRL_LOCKUPRMODE_MASK,  /**< Cortex lockup reset select */
#endif
#if defined(_EMU_RSTCTRL_AVDDBODRMODE_MASK)
  rmuResetAVDD = _EMU_RSTCTRL_AVDDBODRMODE_MASK,       /**< AVDD monitoring select */
#endif
#if defined(_EMU_RSTCTRL_IOVDD0BODRMODE_MASK)
  rmuResetIOVDD0 = _EMU_RSTCTRL_IOVDD0BODRMODE_MASK,   /**< IOVDD0 monitoring select */
#endif
#if defined(_EMU_RSTCTRL_DECBODRMODE_MASK)
  rmuResetDecouple = _EMU_RSTCTRL_DECBODRMODE_MASK,    /**< Decouple monitoring select */
#endif
#if defined(_EMU_RSTCTRL_M0SYSRMODE_MASK)
  rmuResetM0Sys = _EMU_RSTCTRL_M0SYSRMODE_MASK,        /**< M0+ (SE) system reset select */
#endif
#if defined(_EMU_RSTCTRL_M0LOCKUPRMODE_MASK)
  rmuResetM0Lockup = _EMU_RSTCTRL_M0LOCKUPRMODE_MASK,  /**< M0+ (SE) lockup select */
#endif
} RMU_Reset_TypeDef;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

/** RMU_LockupResetDisable kept for backwards compatibility. */
#define RMU_LockupResetDisable(A) RMU_ResetControl(rmuResetLockUp, A)

void RMU_ResetControl(RMU_Reset_TypeDef reset, RMU_ResetMode_TypeDef mode);
void RMU_ResetCauseClear(void);
uint32_t RMU_ResetCauseGet(void);

#if defined(_RMU_CTRL_RESETSTATE_MASK)
/***************************************************************************//**
 * @brief
 *   Set user reset state. Reset only by a Power-on-reset and a pin reset.
 *
 * @param[in] userState User state to set
 ******************************************************************************/
__STATIC_INLINE void RMU_UserResetStateSet(uint32_t userState)
{
  EFM_ASSERT(!(userState
               & ~(_RMU_CTRL_RESETSTATE_MASK >> _RMU_CTRL_RESETSTATE_SHIFT)));
  RMU->CTRL = (RMU->CTRL & ~_RMU_CTRL_RESETSTATE_MASK)
              | (userState << _RMU_CTRL_RESETSTATE_SHIFT);
}

/***************************************************************************//**
 * @brief
 *   Get user reset state. Reset only by a Power-on-reset and a pin reset.
 *
 * @return
 *   Reset surviving user state.
 ******************************************************************************/
__STATIC_INLINE uint32_t RMU_UserResetStateGet(void)
{
  uint32_t userState = (RMU->CTRL & _RMU_CTRL_RESETSTATE_MASK)
                       >> _RMU_CTRL_RESETSTATE_SHIFT;
  return userState;
}
#endif

/** @} (end addtogroup RMU) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(RMU_COUNT) && (RMU_COUNT > 0) */
#endif /* EM_RMU_H */

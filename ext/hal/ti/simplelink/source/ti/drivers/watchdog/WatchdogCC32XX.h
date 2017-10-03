/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** ============================================================================
 *  @file       WatchdogCC32XX.h
 *
 *  @brief      Watchdog driver implementation for CC32XX
 *
 *  The Watchdog header file for CC32XX should be included in an application
 *  as follows:
 *  @code
 *  #include <ti/drivers/Watchdog.h>
 *  #include <ti/drivers/watchdog/WatchdogCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref Watchdog.h for a complete description of APIs.
 *
 *  This Watchdog driver implementation is designed to operate on a CC32XX
 *  device. Once opened, CC32XX Watchdog will count down from the reload
 *  value specified in the WatchdogCC32XX_HWAttrs. If it times out, the
 *  Watchdog interrupt flag will be set, and a user-provided callback function
 *  will be called. If the Watchdog Timer is allowed to time out again while
 *  the interrupt flag is still pending, a reset signal will be generated.
 *  To prevent a reset, Watchdog_clear() must be called to clear the interrupt
 *  flag.
 *
 *  The reload value from which the Watchdog Timer counts down may be changed
 *  during runtime using Watchdog_setReload().
 *
 *  Watchdog_close() is <b>not</b> supported by this driver implementation.
 *
 *  By default the Watchdog driver has resets turned on and this feature cannot
 *  be turned disabled.
 *
 *  To have a user-defined function run at the warning interrupt, first define
 *  a void-type function that takes a Watchdog_Handle cast to a UArg as an
 *  argument. The callback and code to start the Watchdog timer are shown below.
 *
 *  @code
 *  void watchdogCallback(UArg handle);
 *
 *  ...
 *
 *  Watchdog_Handle handle;
 *  Watchdog_Params params;
 *  uint32_t tickValue;
 *
 *  Watchdog_Params_init(&params);
 *  params.callbackFxn = watchdogCallback;
 *  handle = Watchdog_open(Watchdog_configIndex, &params);
 *  // Set timeout period to 100 ms
 *  tickValue = Watchdog_convertMsToTicks(handle, 100);
 *  Watchdog_setReload(handle, tickValue);
 *
 *  ...
 *
 *  void watchdogCallback(UArg handle)
 *  {
 *      // User-defined code here
 *      ...
 *
 *  }
 *  @endcode
 *  ============================================================================
 */

#ifndef ti_drivers_watchdog_WatchdogCC32XX__include
#define ti_drivers_watchdog_WatchdogCC32XX__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ti/drivers/Watchdog.h>

/**
 *  @addtogroup Watchdog_STATUS
 *  WatchdogCC32XX_STATUS_* macros are command codes only defined in the
 *  WatchdogCC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/watchdog/WatchdogCC32XX.h>
 *  @endcode
 *  @{
 */

/* Add WatchdogCC32XX_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup Watchdog_CMD
 *  WatchdogCC32XX_CMD_* macros are command codes only defined in the
 *  WatchdogCC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/watchdog/WatchdogCC32XX.h>
 *  @endcode
 *  @{
 */

/*!
 *  @brief Command used by Watchdog_control to determines
 *  whether the watchdog timer is enabled
 *
 *  With this command code, @b arg is a pointer to a @c bool.
 *  @b *arg contains @c true if the watchdog timer is enabled,
 *  else @c false if it is not.
 */
#define WatchdogCC32XX_CMD_IS_TIMER_ENABLE          (Watchdog_CMD_RESERVED + 0)


/*!
 *  @brief Command used by Watchdog_control
 *  to gets the current watchdog timer value
 *
 *  With this command code, @b arg is a pointer to an @a integer.
 *  @b *arg contains the current value of the watchdog timer.
 */
#define WatchdogCC32XX_CMD_GET_TIMER_VALUE          (Watchdog_CMD_RESERVED + 1)


/*!
 *  @brief Command used by Watchdog_control to determines
 *  whether the watchdog timer is locked
 *
 *  With this command code, @b arg is a pointer to a @c bool.
 *  @b *arg contains @c true if the watchdog timer is locked,
 *  else @c false if it is not.
 */
#define WatchdogCC32XX_CMD_IS_TIMER_LOCKED          (Watchdog_CMD_RESERVED + 2)


/*!
 *  @brief Command used by Watchdog_control
 *  to gets the current watchdog timer reload value
 *
 *  With this command code, @b arg is a pointer to an @a integer.
 *  @b *arg contains the current value loaded into the watchdog timer when
 *  the count reaches zero for the first time.
 */
#define WatchdogCC32XX_CMD_GET_TIMER_RELOAD_VALUE   (Watchdog_CMD_RESERVED + 3)


/** @}*/

/*!  @brief  Watchdog function table for CC32XX */
extern const Watchdog_FxnTable WatchdogCC32XX_fxnTable;

/*!
 *  @brief  Watchdog hardware attributes for CC32XX
 *
 *  intPriority is the Watchdog timer's interrupt priority, as defined by the
 *  underlying OS.  It is passed unmodified to the underlying OS's interrupt
 *  handler creation code, so you need to refer to the OS documentation
 *  for usage.  For example, for SYS/BIOS applications, refer to the
 *  ti.sysbios.family.arm.m3.Hwi documentation for SYS/BIOS usage of
 *  interrupt priorities.  If the driver uses the ti.dpl interface
 *  instead of making OS calls directly, then the HwiP port handles the
 *  interrupt priority in an OS specific way.  In the case of the SYS/BIOS
 *  port, intPriority is passed unmodified to Hwi_create().
 */
typedef struct WatchdogCC32XX_HWAttrs {
    unsigned int baseAddr;       /*!< Base adddress for Watchdog */
    unsigned int intNum;         /*!< WDT interrupt number */
    unsigned int intPriority;    /*!< WDT interrupt priority */
    uint32_t     reloadValue;    /*!< Reload value for Watchdog */
} WatchdogCC32XX_HWAttrs;

/*!
 *  @brief      Watchdog Object for CC32XX
 *
 *  Not to be accessed by the user.
 */
typedef struct WatchdogCC32XX_Object {
    bool         isOpen;              /* Flag for open/close status */
} WatchdogCC32XX_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_watchdog_WatchdogCC32XX__include */

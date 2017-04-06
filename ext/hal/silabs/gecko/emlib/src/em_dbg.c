/***************************************************************************//**
 * @file em_dbg.c
 * @brief Debug (DBG) Peripheral API
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

#include "em_dbg.h"

#if defined( CoreDebug_DHCSR_C_DEBUGEN_Msk )

#include "em_assert.h"
#include "em_cmu.h"
#include "em_gpio.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DBG
 * @brief Debug (DBG) Peripheral API
 * @details
 *  This module contains functions to control the DBG peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The Debug Interface is used to program and debug
 *  Silicon Labs devices.
 * @{
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

#if defined( GPIO_ROUTE_SWOPEN ) || defined( GPIO_ROUTEPEN_SWVPEN )
/***************************************************************************//**
 * @brief
 *   Enable Serial Wire Output (SWO) pin.
 *
 * @details
 *   The SWO pin (sometimes denoted SWV, serial wire viewer) allows for
 *   miscellaneous output to be passed from the Cortex-M3 debug trace module to
 *   an external debug probe. By default, the debug trace module and pin output
 *   may be disabled.
 *
 *   Since the SWO pin is only useful when using a debugger, a suggested use
 *   of this function during startup may be:
 * @verbatim
 * if (DBG_Connected())
 * {
 * DBG_SWOEnable(1);
 * }
 * @endverbatim
 *   By checking if debugger is attached, some setup leading to higher energy
 *   consumption when debugger is attached, can be avoided when not using
 *   a debugger.
 *
 *   Another alternative may be to set the debugger tool chain to configure
 *   the required setup (similar to the content of this function) by some
 *   sort of toolchain scripting during its attach/reset procedure. In that
 *   case, the above suggested code for enabling the SWO pin is not required
 *   in the application.
 *
 * @param[in] location
 *   Pin location used for SWO pin on the application in use.
 ******************************************************************************/
void DBG_SWOEnable(unsigned int location)
{
  int port;
  int pin;

  EFM_ASSERT(location < AFCHANLOC_MAX);

#if defined ( AF_DBG_SWO_PORT )
  port = AF_DBG_SWO_PORT(location);
  pin  = AF_DBG_SWO_PIN(location);
#elif defined (AF_DBG_SWV_PORT )
  port = AF_DBG_SWV_PORT(location);
  pin  = AF_DBG_SWV_PIN(location);
#else
#warning "AF debug port is not defined."
#endif

  /* Port/pin location not defined for device? */
  if ((pin < 0) || (port < 0))
  {
    EFM_ASSERT(0);
    return;
  }

  /* Ensure auxiliary clock going to the Cortex debug trace module is enabled */
  CMU_OscillatorEnable(cmuOsc_AUXHFRCO, true, false);

  /* Set selected pin location for SWO pin and enable it */
  GPIO_DbgLocationSet(location);
  GPIO_DbgSWOEnable(true);

  /* Configure SWO pin for output */
  GPIO_PinModeSet((GPIO_Port_TypeDef)port, pin, gpioModePushPull, 0);
}
#endif

/** @} (end addtogroup DBG) */
/** @} (end addtogroup emlib) */
#endif /* defined( CoreDebug_DHCSR_C_DEBUGEN_Msk ) */

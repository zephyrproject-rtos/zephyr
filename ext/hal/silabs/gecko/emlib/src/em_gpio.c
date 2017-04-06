/***************************************************************************//**
 * @file em_gpio.c
 * @brief General Purpose IO (GPIO) peripheral API
 *   devices.
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


#include "em_gpio.h"

#if defined(GPIO_COUNT) && (GPIO_COUNT > 0)

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup GPIO
 * @brief General Purpose Input/Output (GPIO) API
 * @details
 *  This module contains functions to control the GPIO peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The GPIO peripheral is used for pin configuration
 *  and direct pin manipulation and sensing as well as routing for peripheral
 *  pin connections.
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of pin typically usable in assert statements. */
#define GPIO_DRIVEMODE_VALID(mode)    ((mode) <= 3)
#define GPIO_STRENGHT_VALID(strenght) (!((strenght) & \
                                         ~(_GPIO_P_CTRL_DRIVESTRENGTH_MASK \
                                           | _GPIO_P_CTRL_DRIVESTRENGTHALT_MASK)))
/** @endcond */


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Sets the pin location of the debug pins (Serial Wire interface).
 *
 * @note
 *   Changing the pins used for debugging uncontrolled, may result in a lockout.
 *
 * @param[in] location
 *   The debug pin location to use (0-3).
 ******************************************************************************/
void GPIO_DbgLocationSet(unsigned int location)
{
#if defined ( _GPIO_ROUTE_SWLOCATION_MASK )
  EFM_ASSERT(location < AFCHANLOC_MAX);

  GPIO->ROUTE = (GPIO->ROUTE & ~_GPIO_ROUTE_SWLOCATION_MASK) |
                (location << _GPIO_ROUTE_SWLOCATION_SHIFT);
#else
  (void)location;
#endif
}

#if defined (_GPIO_P_CTRL_DRIVEMODE_MASK)
/***************************************************************************//**
 * @brief
 *   Sets the drive mode for a GPIO port.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] mode
 *   Drive mode to use for port.
 ******************************************************************************/
void GPIO_DriveModeSet(GPIO_Port_TypeDef port, GPIO_DriveMode_TypeDef mode)
{
  EFM_ASSERT(GPIO_PORT_VALID(port) && GPIO_DRIVEMODE_VALID(mode));

  GPIO->P[port].CTRL = (GPIO->P[port].CTRL & ~(_GPIO_P_CTRL_DRIVEMODE_MASK))
                       | (mode << _GPIO_P_CTRL_DRIVEMODE_SHIFT);
}
#endif

#if defined (_GPIO_P_CTRL_DRIVESTRENGTH_MASK)
/***************************************************************************//**
 * @brief
 *   Sets the drive strength for a GPIO port.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] strength
 *   Drive strength to use for port.
 ******************************************************************************/
void GPIO_DriveStrengthSet(GPIO_Port_TypeDef port,
                           GPIO_DriveStrength_TypeDef strength)
{
  EFM_ASSERT(GPIO_PORT_VALID(port) && GPIO_STRENGHT_VALID(strength));
  BUS_RegMaskedWrite(&GPIO->P[port].CTRL,
                     _GPIO_P_CTRL_DRIVESTRENGTH_MASK | _GPIO_P_CTRL_DRIVESTRENGTHALT_MASK,
                     strength);
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure GPIO external pin interrupt.
 *
 * @details
 *   If reconfiguring a GPIO interrupt that is already enabled, it is generally
 *   recommended to disable it first, see GPIO_Disable().
 *
 *   The actual GPIO interrupt handler must be in place before enabling the
 *   interrupt.
 *
 *   Notice that any pending interrupt for the selected interrupt is cleared
 *   by this function.
 *
 * @note
 *   On series 0 devices the pin number parameter is not used. The
 *   pin number used on these devices is hardwired to the interrupt with the
 *   same number. @n
 *   On series 1 devices, pin number can be selected freely within a group.
 *   Interrupt numbers are divided into 4 groups (intNo / 4) and valid pin
 *   number within the interrupt groups are:
 *       0: pins 0-3
 *       1: pins 4-7
 *       2: pins 8-11
 *       3: pins 12-15
 *
 * @param[in] port
 *   The port to associate with @p pin.
 *
 * @param[in] pin
 *   The pin number on the port.
 *
 * @param[in] intNo
 *   The interrupt number to trigger.
 *
 * @param[in] risingEdge
 *   Set to true if interrupts shall be enabled on rising edge, otherwise false.
 *
 * @param[in] fallingEdge
 *   Set to true if interrupts shall be enabled on falling edge, otherwise false.
 *
 * @param[in] enable
 *   Set to true if interrupt shall be enabled after configuration completed,
 *   false to leave disabled. See GPIO_IntDisable() and GPIO_IntEnable().
 ******************************************************************************/
void GPIO_ExtIntConfig(GPIO_Port_TypeDef port,
                       unsigned int pin,
                       unsigned int intNo,
                       bool risingEdge,
                       bool fallingEdge,
                       bool enable)
{
  uint32_t tmp = 0;
#if !defined(_GPIO_EXTIPINSELL_MASK)
  (void)pin;
#endif

  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));
#if defined(_GPIO_EXTIPINSELL_MASK)
  EFM_ASSERT(GPIO_INTNO_PIN_VALID(intNo, pin));
#endif

  /* There are two registers controlling the interrupt configuration:
   * The EXTIPSELL register controls pins 0-7 and EXTIPSELH controls
   * pins 8-15. */
  if (intNo < 8)
  {
    BUS_RegMaskedWrite(&GPIO->EXTIPSELL,
                       _GPIO_EXTIPSELL_EXTIPSEL0_MASK
                       << (_GPIO_EXTIPSELL_EXTIPSEL1_SHIFT * intNo),
                       port << (_GPIO_EXTIPSELL_EXTIPSEL1_SHIFT * intNo));
  }
  else
  {
    tmp = intNo - 8;
    BUS_RegMaskedWrite(&GPIO->EXTIPSELH,
                       _GPIO_EXTIPSELH_EXTIPSEL8_MASK
                       << (_GPIO_EXTIPSELH_EXTIPSEL9_SHIFT * tmp),
                       port << (_GPIO_EXTIPSELH_EXTIPSEL9_SHIFT * tmp));
  }

#if defined(_GPIO_EXTIPINSELL_MASK)
  /* There are two registers controlling the interrupt/pin number mapping:
   * The EXTIPINSELL register controls interrupt 0-7 and EXTIPINSELH controls
   * interrupt 8-15. */
  if (intNo < 8)
  {
    BUS_RegMaskedWrite(&GPIO->EXTIPINSELL,
                       _GPIO_EXTIPINSELL_EXTIPINSEL0_MASK
                       << (_GPIO_EXTIPINSELL_EXTIPINSEL1_SHIFT * intNo),
                       ((pin % 4) & _GPIO_EXTIPINSELL_EXTIPINSEL0_MASK)
                       << (_GPIO_EXTIPINSELL_EXTIPINSEL1_SHIFT * intNo));
  }
  else
  {
    BUS_RegMaskedWrite(&GPIO->EXTIPINSELH,
                       _GPIO_EXTIPINSELH_EXTIPINSEL8_MASK
                       << (_GPIO_EXTIPINSELH_EXTIPINSEL9_SHIFT * tmp),
                       ((pin % 4) & _GPIO_EXTIPINSELH_EXTIPINSEL8_MASK)
                       << (_GPIO_EXTIPSELH_EXTIPSEL9_SHIFT * tmp));
  }
#endif

  /* Enable/disable rising edge */
  BUS_RegBitWrite(&(GPIO->EXTIRISE), intNo, risingEdge);

  /* Enable/disable falling edge */
  BUS_RegBitWrite(&(GPIO->EXTIFALL), intNo, fallingEdge);

  /* Clear any pending interrupt */
  GPIO->IFC = 1 << intNo;

  /* Finally enable/disable interrupt */
  BUS_RegBitWrite(&(GPIO->IEN), intNo, enable);
}

/***************************************************************************//**
 * @brief
 *   Set the mode for a GPIO pin.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin number in the port.
 *
 * @param[in] mode
 *   The desired pin mode.
 *
 * @param[in] out
 *   Value to set for pin in DOUT register. The DOUT setting is important for
 *   even some input mode configurations, determining pull-up/down direction.
 ******************************************************************************/
void GPIO_PinModeSet(GPIO_Port_TypeDef port,
                     unsigned int pin,
                     GPIO_Mode_TypeDef mode,
                     unsigned int out)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  /* If disabling pin, do not modify DOUT in order to reduce chance for */
  /* glitch/spike (may not be sufficient precaution in all use cases) */
  if (mode != gpioModeDisabled)
  {
    if (out)
    {
      GPIO_PinOutSet(port, pin);
    }
    else
    {
      GPIO_PinOutClear(port, pin);
    }
  }

  /* There are two registers controlling the pins for each port. The MODEL
   * register controls pins 0-7 and MODEH controls pins 8-15. */
  if (pin < 8)
  {
    GPIO->P[port].MODEL = (GPIO->P[port].MODEL & ~(0xFu << (pin * 4)))
                          | (mode << (pin * 4));
  }
  else
  {
    GPIO->P[port].MODEH = (GPIO->P[port].MODEH & ~(0xFu << ((pin - 8) * 4)))
                          | (mode << ((pin - 8) * 4));
  }

  if (mode == gpioModeDisabled)
  {
    if (out)
    {
      GPIO_PinOutSet(port, pin);
    }
    else
    {
      GPIO_PinOutClear(port, pin);
    }
  }
}

/***************************************************************************//**
 * @brief
 *   Get the mode for a GPIO pin.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin number in the port.
 *
 * @return
 *   The pin mode.
 ******************************************************************************/
GPIO_Mode_TypeDef GPIO_PinModeGet(GPIO_Port_TypeDef port,
                                  unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  if (pin < 8)
  {
    return (GPIO_Mode_TypeDef) ((GPIO->P[port].MODEL >> (pin * 4)) & 0xF);
  }
  else
  {
    return (GPIO_Mode_TypeDef) ((GPIO->P[port].MODEH >> ((pin - 8) * 4)) & 0xF);
  }
}

#if defined( _GPIO_EM4WUEN_MASK )
/**************************************************************************//**
 * @brief
 *   Enable GPIO pin wake-up from EM4. When the function exits,
 *   EM4 mode can be safely entered.
 *
 * @note
 *   It is assumed that the GPIO pin modes are set correctly.
 *   Valid modes are @ref gpioModeInput and @ref gpioModeInputPull.
 *
 * @param[in] pinmask
 *   Bitmask containing the bitwise logic OR of which GPIO pin(s) to enable.
 *   Refer to Reference Manuals for pinmask to GPIO port/pin mapping.
 * @param[in] polaritymask
 *   Bitmask containing the bitwise logic OR of GPIO pin(s) wake-up polarity.
 *   Refer to Reference Manuals for pinmask to GPIO port/pin mapping.
 *****************************************************************************/
void GPIO_EM4EnablePinWakeup(uint32_t pinmask, uint32_t polaritymask)
{
  EFM_ASSERT((pinmask & ~_GPIO_EM4WUEN_MASK) == 0);

#if defined( _GPIO_EM4WUPOL_MASK )
  EFM_ASSERT((polaritymask & ~_GPIO_EM4WUPOL_MASK) == 0);
  GPIO->EM4WUPOL &= ~pinmask;               /* Set wakeup polarity */
  GPIO->EM4WUPOL |= pinmask & polaritymask;
#elif defined( _GPIO_EXTILEVEL_MASK )
  EFM_ASSERT((polaritymask & ~_GPIO_EXTILEVEL_MASK) == 0);
  GPIO->EXTILEVEL &= ~pinmask;
  GPIO->EXTILEVEL |= pinmask & polaritymask;
#endif
  GPIO->EM4WUEN  |= pinmask;                /* Enable wakeup */

  GPIO_EM4SetPinRetention(true);            /* Enable pin retention */

#if defined( _GPIO_CMD_EM4WUCLR_MASK )
  GPIO->CMD = GPIO_CMD_EM4WUCLR;            /* Clear wake-up logic */
#elif defined( _GPIO_IFC_EM4WU_MASK )
  GPIO_IntClear(pinmask);
#endif
}
#endif

/** @} (end addtogroup GPIO) */
/** @} (end addtogroup emlib) */

#endif /* defined(GPIO_COUNT) && (GPIO_COUNT > 0) */

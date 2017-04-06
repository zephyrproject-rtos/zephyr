/***************************************************************************//**
 * @file em_gpio.h
 * @brief General Purpose IO (GPIO) peripheral API
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


#ifndef EM_GPIO_H
#define EM_GPIO_H

#include "em_device.h"
#if defined(GPIO_COUNT) && (GPIO_COUNT > 0)

#include <stdbool.h>
#include "em_bus.h"
#include "em_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup GPIO
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#if defined( _EFM32_TINY_FAMILY ) || defined( _EFM32_ZERO_FAMILY )

#define _GPIO_PORT_A_PIN_COUNT 14
#define _GPIO_PORT_B_PIN_COUNT 10
#define _GPIO_PORT_C_PIN_COUNT 16
#define _GPIO_PORT_D_PIN_COUNT 9
#define _GPIO_PORT_E_PIN_COUNT 12
#define _GPIO_PORT_F_PIN_COUNT 6
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0xF77F
#define _GPIO_PORT_B_PIN_MASK 0x79F8
#define _GPIO_PORT_C_PIN_MASK 0xFFFF
#define _GPIO_PORT_D_PIN_MASK 0x01FF
#define _GPIO_PORT_E_PIN_MASK 0xFFF0
#define _GPIO_PORT_F_PIN_MASK 0x003F
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _EFM32_HAPPY_FAMILY )

#define _GPIO_PORT_A_PIN_COUNT 6
#define _GPIO_PORT_B_PIN_COUNT 5
#define _GPIO_PORT_C_PIN_COUNT 12
#define _GPIO_PORT_D_PIN_COUNT 4
#define _GPIO_PORT_E_PIN_COUNT 4
#define _GPIO_PORT_F_PIN_COUNT 6
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0x0707
#define _GPIO_PORT_B_PIN_MASK 0x6980
#define _GPIO_PORT_C_PIN_MASK 0xEF1F
#define _GPIO_PORT_D_PIN_MASK 0x00F0
#define _GPIO_PORT_E_PIN_MASK 0x3C00
#define _GPIO_PORT_F_PIN_MASK 0x003F
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _EFM32_GIANT_FAMILY ) \
      || defined( _EFM32_WONDER_FAMILY )

#define _GPIO_PORT_A_PIN_COUNT 16
#define _GPIO_PORT_B_PIN_COUNT 16
#define _GPIO_PORT_C_PIN_COUNT 16
#define _GPIO_PORT_D_PIN_COUNT 16
#define _GPIO_PORT_E_PIN_COUNT 16
#define _GPIO_PORT_F_PIN_COUNT 13
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0xFFFF
#define _GPIO_PORT_B_PIN_MASK 0xFFFF
#define _GPIO_PORT_C_PIN_MASK 0xFFFF
#define _GPIO_PORT_D_PIN_MASK 0xFFFF
#define _GPIO_PORT_E_PIN_MASK 0xFFFF
#define _GPIO_PORT_F_PIN_MASK 0x1FFF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _EFM32_GECKO_FAMILY )

#define _GPIO_PORT_A_PIN_COUNT 16
#define _GPIO_PORT_B_PIN_COUNT 16
#define _GPIO_PORT_C_PIN_COUNT 16
#define _GPIO_PORT_D_PIN_COUNT 16
#define _GPIO_PORT_E_PIN_COUNT 16
#define _GPIO_PORT_F_PIN_COUNT 10
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0xFFFF
#define _GPIO_PORT_B_PIN_MASK 0xFFFF
#define _GPIO_PORT_C_PIN_MASK 0xFFFF
#define _GPIO_PORT_D_PIN_MASK 0xFFFF
#define _GPIO_PORT_E_PIN_MASK 0xFFFF
#define _GPIO_PORT_F_PIN_MASK 0x03FF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 ) && defined( _EFR_DEVICE )

#define _GPIO_PORT_A_PIN_COUNT 6
#define _GPIO_PORT_B_PIN_COUNT 5
#define _GPIO_PORT_C_PIN_COUNT 6
#define _GPIO_PORT_D_PIN_COUNT 6
#define _GPIO_PORT_E_PIN_COUNT 0
#define _GPIO_PORT_F_PIN_COUNT 8
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0x003F
#define _GPIO_PORT_B_PIN_MASK 0xF800
#define _GPIO_PORT_C_PIN_MASK 0x0FC0
#define _GPIO_PORT_D_PIN_MASK 0xFC00
#define _GPIO_PORT_E_PIN_MASK 0x0000
#define _GPIO_PORT_F_PIN_MASK 0x00FF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _SILICON_LABS_GECKO_INTERNAL_SDID_80 ) && defined( _EFM_DEVICE )

#define _GPIO_PORT_A_PIN_COUNT 6
#define _GPIO_PORT_B_PIN_COUNT 5
#define _GPIO_PORT_C_PIN_COUNT 6
#define _GPIO_PORT_D_PIN_COUNT 7
#define _GPIO_PORT_E_PIN_COUNT 0
#define _GPIO_PORT_F_PIN_COUNT 8
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0x003F
#define _GPIO_PORT_B_PIN_MASK 0xF800
#define _GPIO_PORT_C_PIN_MASK 0x0FC0
#define _GPIO_PORT_D_PIN_MASK 0xFE00
#define _GPIO_PORT_E_PIN_MASK 0x0000
#define _GPIO_PORT_F_PIN_MASK 0x00FF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#elif defined( _SILICON_LABS_GECKO_INTERNAL_SDID_84 )

#define _GPIO_PORT_A_PIN_COUNT 10
#define _GPIO_PORT_B_PIN_COUNT 10
#define _GPIO_PORT_C_PIN_COUNT 12
#define _GPIO_PORT_D_PIN_COUNT 8
#define _GPIO_PORT_E_PIN_COUNT 0
#define _GPIO_PORT_F_PIN_COUNT 16
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 4
#define _GPIO_PORT_J_PIN_COUNT 2
#define _GPIO_PORT_K_PIN_COUNT 3

#define _GPIO_PORT_A_PIN_MASK 0x03FF
#define _GPIO_PORT_B_PIN_MASK 0xFFC0
#define _GPIO_PORT_C_PIN_MASK 0x0FFF
#define _GPIO_PORT_D_PIN_MASK 0xFF00
#define _GPIO_PORT_E_PIN_MASK 0x0000
#define _GPIO_PORT_F_PIN_MASK 0xFFFF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x000F
#define _GPIO_PORT_J_PIN_MASK 0xC000
#define _GPIO_PORT_K_PIN_MASK 0x0007

#elif defined( _SILICON_LABS_GECKO_INTERNAL_SDID_89 )

#define _GPIO_PORT_A_PIN_COUNT 6
#define _GPIO_PORT_B_PIN_COUNT 5
#define _GPIO_PORT_C_PIN_COUNT 6
#define _GPIO_PORT_D_PIN_COUNT 6
#define _GPIO_PORT_E_PIN_COUNT 0
#define _GPIO_PORT_F_PIN_COUNT 8
#define _GPIO_PORT_G_PIN_COUNT 0
#define _GPIO_PORT_H_PIN_COUNT 0
#define _GPIO_PORT_I_PIN_COUNT 0
#define _GPIO_PORT_J_PIN_COUNT 0
#define _GPIO_PORT_K_PIN_COUNT 0

#define _GPIO_PORT_A_PIN_MASK 0x003F
#define _GPIO_PORT_B_PIN_MASK 0xF800
#define _GPIO_PORT_C_PIN_MASK 0x0FC0
#define _GPIO_PORT_D_PIN_MASK 0xFC00
#define _GPIO_PORT_E_PIN_MASK 0x0000
#define _GPIO_PORT_F_PIN_MASK 0x00FF
#define _GPIO_PORT_G_PIN_MASK 0x0000
#define _GPIO_PORT_H_PIN_MASK 0x0000
#define _GPIO_PORT_I_PIN_MASK 0x0000
#define _GPIO_PORT_J_PIN_MASK 0x0000
#define _GPIO_PORT_K_PIN_MASK 0x0000

#else
#warning "Port and pin masks are not defined for this family."
#endif

#define _GPIO_PORT_SIZE(port) (                 \
        (port) == 0  ? _GPIO_PORT_A_PIN_COUNT : \
        (port) == 1  ? _GPIO_PORT_B_PIN_COUNT : \
        (port) == 2  ? _GPIO_PORT_C_PIN_COUNT : \
        (port) == 3  ? _GPIO_PORT_D_PIN_COUNT : \
        (port) == 4  ? _GPIO_PORT_E_PIN_COUNT : \
        (port) == 5  ? _GPIO_PORT_F_PIN_COUNT : \
        (port) == 6  ? _GPIO_PORT_G_PIN_COUNT : \
        (port) == 7  ? _GPIO_PORT_H_PIN_COUNT : \
        (port) == 8  ? _GPIO_PORT_I_PIN_COUNT : \
        (port) == 9  ? _GPIO_PORT_J_PIN_COUNT : \
        (port) == 10 ? _GPIO_PORT_K_PIN_COUNT : \
        0)

#define _GPIO_PORT_MASK(port) (                \
        (port) == 0  ? _GPIO_PORT_A_PIN_MASK : \
        (port) == 1  ? _GPIO_PORT_B_PIN_MASK : \
        (port) == 2  ? _GPIO_PORT_C_PIN_MASK : \
        (port) == 3  ? _GPIO_PORT_D_PIN_MASK : \
        (port) == 4  ? _GPIO_PORT_E_PIN_MASK : \
        (port) == 5  ? _GPIO_PORT_F_PIN_MASK : \
        (port) == 6  ? _GPIO_PORT_G_PIN_MASK : \
        (port) == 7  ? _GPIO_PORT_H_PIN_MASK : \
        (port) == 8  ? _GPIO_PORT_I_PIN_MASK : \
        (port) == 9  ? _GPIO_PORT_J_PIN_MASK : \
        (port) == 10 ? _GPIO_PORT_K_PIN_MASK : \
        0)

/** Validation of port and pin */
#define GPIO_PORT_VALID(port)          ( _GPIO_PORT_MASK(port) )
#define GPIO_PORT_PIN_VALID(port, pin) ((( _GPIO_PORT_MASK(port)) >> (pin)) & 0x1 )

#if defined(_GPIO_EXTIPINSELL_MASK)
/** Validation of interrupt number and pin */
#define GPIO_INTNO_PIN_VALID(intNo, pin)              \
  ((intNo & ~_GPIO_EXTIPINSELL_EXTIPINSEL0_MASK)      \
   == (pin & ~_GPIO_EXTIPINSELL_EXTIPINSEL0_MASK))
#endif

/** Highest GPIO pin number */
#define GPIO_PIN_MAX  15

/** Highest GPIO port number */
#if ( _GPIO_PORT_K_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  10
#elif ( _GPIO_PORT_J_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  9
#elif ( _GPIO_PORT_I_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  8
#elif ( _GPIO_PORT_H_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  7
#elif ( _GPIO_PORT_G_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  6
#elif ( _GPIO_PORT_F_PIN_COUNT > 0 )
#define GPIO_PORT_MAX  5
#else
#error "Max GPIO port number is undefined for this part."
#endif

/** Highest EXT GPIO interrupt number */
#define GPIO_EXTINTNO_MAX   15

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** GPIO ports ids. */
typedef enum
{
#if ( _GPIO_PORT_A_PIN_COUNT > 0 )
  gpioPortA = 0,
#endif
#if ( _GPIO_PORT_B_PIN_COUNT > 0 )
  gpioPortB = 1,
#endif
#if ( _GPIO_PORT_C_PIN_COUNT > 0 )
  gpioPortC = 2,
#endif
#if ( _GPIO_PORT_D_PIN_COUNT > 0 )
  gpioPortD = 3,
#endif
#if ( _GPIO_PORT_E_PIN_COUNT > 0 )
  gpioPortE = 4,
#endif
#if ( _GPIO_PORT_F_PIN_COUNT > 0 )
  gpioPortF = 5,
#endif
#if ( _GPIO_PORT_G_PIN_COUNT > 0 )
  gpioPortG = 6,
#endif
#if ( _GPIO_PORT_H_PIN_COUNT > 0 )
  gpioPortH = 7,
#endif
#if ( _GPIO_PORT_I_PIN_COUNT > 0 )
  gpioPortI = 8,
#endif
#if ( _GPIO_PORT_J_PIN_COUNT > 0 )
  gpioPortJ = 9,
#endif
#if ( _GPIO_PORT_K_PIN_COUNT > 0 )
  gpioPortK = 10,
#endif
} GPIO_Port_TypeDef;

#if defined( _GPIO_P_CTRL_DRIVEMODE_MASK )
/** GPIO drive mode. */
typedef enum
{
  /** Default 6mA */
  gpioDriveModeStandard = GPIO_P_CTRL_DRIVEMODE_STANDARD,
  /** 0.5 mA */
  gpioDriveModeLowest   = GPIO_P_CTRL_DRIVEMODE_LOWEST,
  /** 20 mA */
  gpioDriveModeHigh     = GPIO_P_CTRL_DRIVEMODE_HIGH,
  /** 2 mA */
  gpioDriveModeLow      = GPIO_P_CTRL_DRIVEMODE_LOW
} GPIO_DriveMode_TypeDef;
#endif

#if defined( _GPIO_P_CTRL_DRIVESTRENGTH_MASK ) && defined( _GPIO_P_CTRL_DRIVESTRENGTHALT_MASK )
/** GPIO drive strength. */
typedef enum
{
  /** GPIO weak 1mA and alternate function weak 1mA */
  gpioDriveStrengthWeakAlternateWeak     = GPIO_P_CTRL_DRIVESTRENGTH_WEAK | GPIO_P_CTRL_DRIVESTRENGTHALT_WEAK,

  /** GPIO weak 1mA and alternate function strong 10mA */
  gpioDriveStrengthWeakAlternateStrong   = GPIO_P_CTRL_DRIVESTRENGTH_WEAK | GPIO_P_CTRL_DRIVESTRENGTHALT_STRONG,

    /** GPIO strong 10mA and alternate function weak 1mA */
  gpioDriveStrengthStrongAlternateWeak   = GPIO_P_CTRL_DRIVESTRENGTH_STRONG | GPIO_P_CTRL_DRIVESTRENGTHALT_WEAK,

  /** GPIO strong 10mA and alternate function strong 10mA */
  gpioDriveStrengthStrongAlternateStrong = GPIO_P_CTRL_DRIVESTRENGTH_STRONG | GPIO_P_CTRL_DRIVESTRENGTHALT_STRONG,
} GPIO_DriveStrength_TypeDef;

/* Deprecated enums */
#define gpioDriveStrengthStrong   gpioDriveStrengthStrongAlternateStrong
#define gpioDriveStrengthWeak     gpioDriveStrengthWeakAlternateWeak
#endif

/** Pin mode. For more details on each mode, please refer to the
 * reference manual. */
typedef enum
{
  /** Input disabled. Pullup if DOUT is set. */
  gpioModeDisabled                  = _GPIO_P_MODEL_MODE0_DISABLED,
  /** Input enabled. Filter if DOUT is set */
  gpioModeInput                     = _GPIO_P_MODEL_MODE0_INPUT,
  /** Input enabled. DOUT determines pull direction */
  gpioModeInputPull                 = _GPIO_P_MODEL_MODE0_INPUTPULL,
  /** Input enabled with filter. DOUT determines pull direction */
  gpioModeInputPullFilter           = _GPIO_P_MODEL_MODE0_INPUTPULLFILTER,
  /** Push-pull output */
  gpioModePushPull                  = _GPIO_P_MODEL_MODE0_PUSHPULL,
#if defined( _GPIO_P_MODEL_MODE0_PUSHPULLDRIVE )
  /** Push-pull output with drive-strength set by DRIVEMODE */
  gpioModePushPullDrive             = _GPIO_P_MODEL_MODE0_PUSHPULLDRIVE,
#endif
#if defined( _GPIO_P_MODEL_MODE0_PUSHPULLALT )
  /** Push-pull using alternate control */
  gpioModePushPullAlternate       = _GPIO_P_MODEL_MODE0_PUSHPULLALT,
#endif
  /** Wired-or output */
  gpioModeWiredOr                       = _GPIO_P_MODEL_MODE0_WIREDOR,
  /** Wired-or output with pull-down */
  gpioModeWiredOrPullDown               = _GPIO_P_MODEL_MODE0_WIREDORPULLDOWN,
  /** Open-drain output */
  gpioModeWiredAnd                      = _GPIO_P_MODEL_MODE0_WIREDAND,
  /** Open-drain output with filter */
  gpioModeWiredAndFilter                = _GPIO_P_MODEL_MODE0_WIREDANDFILTER,
  /** Open-drain output with pullup */
  gpioModeWiredAndPullUp                = _GPIO_P_MODEL_MODE0_WIREDANDPULLUP,
  /** Open-drain output with filter and pullup */
  gpioModeWiredAndPullUpFilter          = _GPIO_P_MODEL_MODE0_WIREDANDPULLUPFILTER,
#if defined( _GPIO_P_MODEL_MODE0_WIREDANDDRIVE )
  /** Open-drain output with drive-strength set by DRIVEMODE */
  gpioModeWiredAndDrive                 = _GPIO_P_MODEL_MODE0_WIREDANDDRIVE,
  /** Open-drain output with filter and drive-strength set by DRIVEMODE */
  gpioModeWiredAndDriveFilter           = _GPIO_P_MODEL_MODE0_WIREDANDDRIVEFILTER,
  /** Open-drain output with pullup and drive-strength set by DRIVEMODE */
  gpioModeWiredAndDrivePullUp           = _GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUP,
  /** Open-drain output with filter, pullup and drive-strength set by DRIVEMODE */
  gpioModeWiredAndDrivePullUpFilter     = _GPIO_P_MODEL_MODE0_WIREDANDDRIVEPULLUPFILTER
#endif
#if defined( _GPIO_P_MODEL_MODE0_WIREDANDALT )
  /** Open-drain output using alternate control */
  gpioModeWiredAndAlternate             = _GPIO_P_MODEL_MODE0_WIREDANDALT,
  /** Open-drain output using alternate control with filter */
  gpioModeWiredAndAlternateFilter       = _GPIO_P_MODEL_MODE0_WIREDANDALTFILTER,
  /** Open-drain output using alternate control with pullup */
  gpioModeWiredAndAlternatePullUp       = _GPIO_P_MODEL_MODE0_WIREDANDALTPULLUP,
  /** Open-drain output uisng alternate control with filter and pullup */
  gpioModeWiredAndAlternatePullUpFilter = _GPIO_P_MODEL_MODE0_WIREDANDALTPULLUPFILTER,
#endif
} GPIO_Mode_TypeDef;

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void GPIO_DbgLocationSet(unsigned int location);

/***************************************************************************//**
 * @brief
 *   Enable/disable serial wire clock pin.
 *
 * @note
 *   Disabling SWDClk will disable the debug interface, which may result in
 *   a lockout if done early in startup (before debugger is able to halt core).
 *
 * @param[in] enable
 *   @li false - disable serial wire clock.
 *   @li true - enable serial wire clock (default after reset).
 ******************************************************************************/
__STATIC_INLINE void GPIO_DbgSWDClkEnable(bool enable)
{
#if defined( _GPIO_ROUTE_SWCLKPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTE), _GPIO_ROUTE_SWCLKPEN_SHIFT, enable);
#elif defined( _GPIO_ROUTEPEN_SWCLKTCKPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTEPEN), _GPIO_ROUTEPEN_SWCLKTCKPEN_SHIFT, enable);
#else
#warning "ROUTE enable for SWCLK pin is not defined."
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable/disable serial wire data I/O pin.
 *
 * @note
 *   Disabling SWDClk will disable the debug interface, which may result in
 *   a lockout if done early in startup (before debugger is able to halt core).
 *
 * @param[in] enable
 *   @li false - disable serial wire data pin.
 *   @li true - enable serial wire data pin (default after reset).
 ******************************************************************************/
__STATIC_INLINE void GPIO_DbgSWDIOEnable(bool enable)
{
#if defined( _GPIO_ROUTE_SWDIOPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTE), _GPIO_ROUTE_SWDIOPEN_SHIFT, enable);
#elif defined( _GPIO_ROUTEPEN_SWDIOTMSPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTEPEN), _GPIO_ROUTEPEN_SWDIOTMSPEN_SHIFT, enable);
#else
#warning "ROUTE enable for SWDIO pin is not defined."
#endif
}

#if defined( _GPIO_ROUTE_SWOPEN_MASK ) || defined( _GPIO_ROUTEPEN_SWVPEN_MASK )
/***************************************************************************//**
 * @brief
 *   Enable/Disable serial wire output pin.
 *
 * @note
 *   Enabling this pin is not sufficient to fully enable serial wire output
 *   which is also dependent on issues outside the GPIO module. Please refer to
 *   DBG_SWOEnable().
 *
 * @param[in] enable
 *   @li false - disable serial wire viewer pin (default after reset).
 *   @li true - enable serial wire viewer pin.
 ******************************************************************************/
__STATIC_INLINE void GPIO_DbgSWOEnable(bool enable)
{
#if defined( _GPIO_ROUTE_SWOPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTE), _GPIO_ROUTE_SWOPEN_SHIFT, enable);
#elif defined( _GPIO_ROUTEPEN_SWVPEN_MASK )
  BUS_RegBitWrite(&(GPIO->ROUTEPEN), _GPIO_ROUTEPEN_SWVPEN_SHIFT, enable);
#else
#warning "ROUTE enable for SWO/SWV pin is not defined."
#endif
}
#endif

#if defined (_GPIO_P_CTRL_DRIVEMODE_MASK)
void GPIO_DriveModeSet(GPIO_Port_TypeDef port, GPIO_DriveMode_TypeDef mode);
#endif

#if defined( _GPIO_P_CTRL_DRIVESTRENGTH_MASK )
void GPIO_DriveStrengthSet(GPIO_Port_TypeDef port, GPIO_DriveStrength_TypeDef strength);
#endif

# if defined( _GPIO_EM4WUEN_MASK )
/**************************************************************************//**
 * @brief
 *   Disable GPIO pin wake-up from EM4.
 *
 * @param[in] pinmask
 *   Bitmask containing the bitwise logic OR of which GPIO pin(s) to disable.
 *   Refer to Reference Manuals for pinmask to GPIO port/pin mapping.
 *****************************************************************************/
__STATIC_INLINE void GPIO_EM4DisablePinWakeup(uint32_t pinmask)
{
  EFM_ASSERT((pinmask & ~_GPIO_EM4WUEN_MASK) == 0);

  GPIO->EM4WUEN &= ~pinmask;
}
#endif

# if defined( _GPIO_EM4WUEN_MASK )
void GPIO_EM4EnablePinWakeup(uint32_t pinmask, uint32_t polaritymask);
#endif

#if defined( _GPIO_EM4WUCAUSE_MASK ) || defined( _GPIO_IF_EM4WU_MASK )
/**************************************************************************//**
 * @brief
 *   Check which GPIO pin(s) that caused a wake-up from EM4.
 *
 * @return
 *   Bitmask containing the bitwise logic OR of which GPIO pin(s) caused the
 *   wake-up. Refer to Reference Manuals for pinmask to GPIO port/pin mapping.
 *****************************************************************************/
__STATIC_INLINE uint32_t GPIO_EM4GetPinWakeupCause(void)
{
#if defined( _GPIO_EM4WUCAUSE_MASK )
  return GPIO->EM4WUCAUSE & _GPIO_EM4WUCAUSE_MASK;
#else
  return GPIO->IF & _GPIO_IF_EM4WU_MASK;
#endif
}
#endif

#if defined( GPIO_CTRL_EM4RET ) || defined( _EMU_EM4CTRL_EM4IORETMODE_MASK )
/**************************************************************************//**
 * @brief
 *   Enable GPIO pin retention of output enable, output value, pull enable and
 *   pull direction in EM4.
 *
 * @note
 *   For platform 2 parts, EMU_EM4Init() and EMU_UnlatchPinRetention() offers
 *   more pin retention features. This function implements the EM4EXIT retention
 *   mode on platform 2.
 *
 * @param[in] enable
 *   @li true - enable EM4 pin retention.
 *   @li false - disable EM4 pin retention.
 *****************************************************************************/
__STATIC_INLINE void GPIO_EM4SetPinRetention(bool enable)
{
  if (enable)
  {
#if defined( GPIO_CTRL_EM4RET )
    GPIO->CTRL |= GPIO_CTRL_EM4RET;
#else
    EMU->EM4CTRL = (EMU->EM4CTRL & ~_EMU_EM4CTRL_EM4IORETMODE_MASK)
                   | EMU_EM4CTRL_EM4IORETMODE_EM4EXIT;
#endif
  }
  else
  {
#if defined( GPIO_CTRL_EM4RET )
    GPIO->CTRL &= ~GPIO_CTRL_EM4RET;
#else
    EMU->EM4CTRL = (EMU->EM4CTRL & ~_EMU_EM4CTRL_EM4IORETMODE_MASK)
                   | EMU_EM4CTRL_EM4IORETMODE_DISABLE;
#endif
  }
}
#endif

void GPIO_ExtIntConfig(GPIO_Port_TypeDef port,
                       unsigned int pin,
                       unsigned int intNo,
                       bool risingEdge,
                       bool fallingEdge,
                       bool enable);

/***************************************************************************//**
 * @brief
 *   Enable/disable input sensing.
 *
 * @details
 *   Disabling input sensing if not used, can save some energy consumption.
 *
 * @param[in] val
 *   Bitwise logic OR of one or more of:
 *   @li GPIO_INSENSE_INT - interrupt input sensing.
 *   @li GPIO_INSENSE_PRS - peripheral reflex system input sensing.
 *
 * @param[in] mask
 *   Mask containing bitwise logic OR of bits similar as for @p val used to
 *   indicate which input sense options to disable/enable.
 ******************************************************************************/
__STATIC_INLINE void GPIO_InputSenseSet(uint32_t val, uint32_t mask)
{
  GPIO->INSENSE = (GPIO->INSENSE & ~mask) | (val & mask);
}

/***************************************************************************//**
 * @brief
 *   Clear one or more pending GPIO interrupts.
 *
 * @param[in] flags
 *   Bitwise logic OR of GPIO interrupt sources to clear.
 ******************************************************************************/
__STATIC_INLINE void GPIO_IntClear(uint32_t flags)
{
  GPIO->IFC = flags;
}

/***************************************************************************//**
 * @brief
 *   Disable one or more GPIO interrupts.
 *
 * @param[in] flags
 *   GPIO interrupt sources to disable.
 ******************************************************************************/
__STATIC_INLINE void GPIO_IntDisable(uint32_t flags)
{
  GPIO->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Enable one or more GPIO interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using GPIO_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   GPIO interrupt sources to enable.
 ******************************************************************************/
__STATIC_INLINE void GPIO_IntEnable(uint32_t flags)
{
  GPIO->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *   Get pending GPIO interrupts.
 *
 * @return
 *   GPIO interrupt sources pending.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPIO_IntGet(void)
{
  return GPIO->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending GPIO interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled GPIO interrupt sources.
 *   The return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in GPIO_IEN register
 *     and
 *   - the OR combination of valid interrupt flags in GPIO_IF register.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPIO_IntGetEnabled(void)
{
  uint32_t tmp;

  /* Store GPIO->IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  tmp = GPIO->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return GPIO->IF & tmp;
}

/**************************************************************************//**
 * @brief
 *   Set one or more pending GPIO interrupts from SW.
 *
 * @param[in] flags
 *   GPIO interrupt sources to set to pending.
 *****************************************************************************/
__STATIC_INLINE void GPIO_IntSet(uint32_t flags)
{
  GPIO->IFS = flags;
}

/***************************************************************************//**
 * @brief
 *   Locks the GPIO configuration.
 ******************************************************************************/
__STATIC_INLINE void GPIO_Lock(void)
{
  GPIO->LOCK = GPIO_LOCK_LOCKKEY_LOCK;
}

/***************************************************************************//**
 * @brief
 *   Read the pad value for a single pin in a GPIO port.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin number to read.
 *
 * @return
 *   The pin value, 0 or 1.
 ******************************************************************************/
__STATIC_INLINE unsigned int GPIO_PinInGet(GPIO_Port_TypeDef port,
                                           unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));
  return BUS_RegBitRead(&GPIO->P[port].DIN, pin);
}

GPIO_Mode_TypeDef GPIO_PinModeGet(GPIO_Port_TypeDef port,
                                  unsigned int pin);

void GPIO_PinModeSet(GPIO_Port_TypeDef port,
                     unsigned int pin,
                     GPIO_Mode_TypeDef mode,
                     unsigned int out);

/***************************************************************************//**
 * @brief
 *   Set a single pin in GPIO data out port register to 0.
 *
 * @note
 *   In order for the setting to take effect on the output pad, the pin must
 *   have been configured properly. If not, it will take effect whenever the
 *   pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin to set.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PinOutClear(GPIO_Port_TypeDef port, unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));
#if defined( _GPIO_P_DOUTCLR_MASK )
  GPIO->P[port].DOUTCLR = 1 << pin;
#else
  BUS_RegBitWrite(&GPIO->P[port].DOUT, pin, 0);
#endif
}

/***************************************************************************//**
 * @brief
 *   Get current setting for a pin in a GPIO port data out register.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin to get setting for.
 *
 * @return
 *   The DOUT setting for the requested pin, 0 or 1.
 ******************************************************************************/
__STATIC_INLINE unsigned int GPIO_PinOutGet(GPIO_Port_TypeDef port,
                                            unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));
  return BUS_RegBitRead(&GPIO->P[port].DOUT, pin);
}

/***************************************************************************//**
 * @brief
 *   Set a single pin in GPIO data out register to 1.
 *
 * @note
 *   In order for the setting to take effect on the output pad, the pin must
 *   have been configured properly. If not, it will take effect whenever the
 *   pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin to set.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PinOutSet(GPIO_Port_TypeDef port, unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));
#if defined( _GPIO_P_DOUTSET_MASK )
  GPIO->P[port].DOUTSET = 1 << pin;
#else
  BUS_RegBitWrite(&GPIO->P[port].DOUT, pin, 1);
#endif
}

/***************************************************************************//**
 * @brief
 *   Toggle a single pin in GPIO port data out register.
 *
 * @note
 *   In order for the setting to take effect on the output pad, the pin must
 *   have been configured properly. If not, it will take effect whenever the
 *   pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pin
 *   The pin to toggle.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PinOutToggle(GPIO_Port_TypeDef port, unsigned int pin)
{
  EFM_ASSERT(GPIO_PORT_PIN_VALID(port, pin));

  GPIO->P[port].DOUTTGL = 1 << pin;
}

/***************************************************************************//**
 * @brief
 *   Read the pad values for GPIO port.
 *
 * @param[in] port
 *   The GPIO port to access.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPIO_PortInGet(GPIO_Port_TypeDef port)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));

  return GPIO->P[port].DIN;
}

/***************************************************************************//**
 * @brief
 *   Set bits in DOUT register for a port to 0.
 *
 * @note
 *   In order for the setting to take effect on the output pad, the pin must
 *   have been configured properly. If not, it will take effect whenever the
 *   pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pins
 *   Bit mask for bits to clear in DOUT register.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PortOutClear(GPIO_Port_TypeDef port, uint32_t pins)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));
#if defined( _GPIO_P_DOUTCLR_MASK )
  GPIO->P[port].DOUTCLR = pins;
#else
  BUS_RegMaskedClear(&GPIO->P[port].DOUT, pins);
#endif
}

/***************************************************************************//**
 * @brief
 *   Get current setting for a GPIO port data out register.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @return
 *   The data out setting for the requested port.
 ******************************************************************************/
__STATIC_INLINE uint32_t GPIO_PortOutGet(GPIO_Port_TypeDef port)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));

  return GPIO->P[port].DOUT;
}

/***************************************************************************//**
 * @brief
 *   Set bits GPIO data out register to 1.
 *
 * @note
 *   In order for the setting to take effect on the respective output pads, the
 *   pins must have been configured properly. If not, it will take effect
 *   whenever the pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pins
 *   Bit mask for bits to set to 1 in DOUT register.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PortOutSet(GPIO_Port_TypeDef port, uint32_t pins)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));
#if defined( _GPIO_P_DOUTSET_MASK )
  GPIO->P[port].DOUTSET = pins;
#else
  BUS_RegMaskedSet(&GPIO->P[port].DOUT, pins);
#endif
}

/***************************************************************************//**
 * @brief
 *   Set GPIO port data out register.
 *
 * @note
 *   In order for the setting to take effect on the respective output pads, the
 *   pins must have been configured properly. If not, it will take effect
 *   whenever the pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] val
 *   Value to write to port data out register.
 *
 * @param[in] mask
 *   Mask indicating which bits to modify.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PortOutSetVal(GPIO_Port_TypeDef port,
                                        uint32_t val,
                                        uint32_t mask)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));

  GPIO->P[port].DOUT = (GPIO->P[port].DOUT & ~mask) | (val & mask);
}

/***************************************************************************//**
 * @brief
 *   Toggle pins in GPIO port data out register.
 *
 * @note
 *   In order for the setting to take effect on the output pad, the pin must
 *   have been configured properly. If not, it will take effect whenever the
 *   pin has been properly configured.
 *
 * @param[in] port
 *   The GPIO port to access.
 *
 * @param[in] pins
 *   Bitmask with pins to toggle.
 ******************************************************************************/
__STATIC_INLINE void GPIO_PortOutToggle(GPIO_Port_TypeDef port, uint32_t pins)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));

  GPIO->P[port].DOUTTGL = pins;
}

#if defined(_GPIO_P_CTRL_SLEWRATE_MASK)
/***************************************************************************//**
 * @brief
 *   Set slewrate for pins on a GPIO port.
 *
 * @param[in] port
 *   The GPIO port to configure.
 *
 * @param[in] slewrate
 *   The slewrate to configure for pins on this GPIO port.
 *
 * @param[in] slewrateAlt
 *   The slewrate to configure for pins using alternate modes on this GPIO port.
 ******************************************************************************/
__STATIC_INLINE void GPIO_SlewrateSet(GPIO_Port_TypeDef port,
                                      uint32_t slewrate,
                                      uint32_t slewrateAlt)
{
  EFM_ASSERT(GPIO_PORT_VALID(port));
  EFM_ASSERT(slewrate <= (_GPIO_P_CTRL_SLEWRATE_MASK
                          >> _GPIO_P_CTRL_SLEWRATE_SHIFT));
  EFM_ASSERT(slewrateAlt <= (_GPIO_P_CTRL_SLEWRATEALT_MASK
                             >> _GPIO_P_CTRL_SLEWRATEALT_SHIFT));

  GPIO->P[port].CTRL = (GPIO->P[port].CTRL
                        & ~(_GPIO_P_CTRL_SLEWRATE_MASK
                            | _GPIO_P_CTRL_SLEWRATEALT_MASK))
                       | (slewrate << _GPIO_P_CTRL_SLEWRATE_SHIFT)
                       | (slewrateAlt << _GPIO_P_CTRL_SLEWRATEALT_SHIFT);
}
#endif

/***************************************************************************//**
 * @brief
 *   Unlocks the GPIO configuration.
 ******************************************************************************/
__STATIC_INLINE void GPIO_Unlock(void)
{
  GPIO->LOCK = GPIO_LOCK_LOCKKEY_UNLOCK;
}

/*******************************************************************************
 ***********************   DEPRECATED PROTOTYPES   *****************************
 ***********************     (will be removed)     *****************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Configure GPIO interrupt.
 *
 * @details
 *   If reconfiguring a GPIO interrupt that is already enabled, it is generally
 *   recommended to disable it first, see GPIO_Disable().
 *
 *   The actual GPIO interrupt handler must be in place before enabling the
 *   interrupt.
 *
 *   Notice that any pending interrupt for the selected pin is cleared by this
 *   function.
 *
 * @deprecated
 *   Deprecated function. New code should use @ref GPIO_ExtIntConfig().
 *
 * @note
 *   A certain pin number can only be associated with one port. Ie, if GPIO
 *   interrupt 1 is assigned to port A/pin 1, then it is not possible to use
 *   pin 1 from any other ports for interrupts. Please refer to the reference
 *   manual. On devices which implement GPIO_EXTIPINSEL registers a more
 *   flexible approach is possible, refer to @ref GPIO_ExtIntConfig().
 *
 * @param[in] port
 *   The port to associate with @p pin.
 *
 * @param[in] pin
 *   The pin number on the port ( == GPIO EXTI interrupt number).
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
__STATIC_INLINE void GPIO_IntConfig(GPIO_Port_TypeDef port,
                                    unsigned int pin,
                                    bool risingEdge,
                                    bool fallingEdge,
                                    bool enable)
{
  GPIO_ExtIntConfig(port, pin, pin, risingEdge, fallingEdge, enable);
}

/** @} (end addtogroup GPIO) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(GPIO_COUNT) && (GPIO_COUNT > 0) */
#endif /* EM_GPIO_H */

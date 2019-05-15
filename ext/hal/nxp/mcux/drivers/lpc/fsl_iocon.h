/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_IOCON_H_
#define _FSL_IOCON_H_

#include "fsl_common.h"

/*!
 * @addtogroup lpc_iocon
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpc_iocon"
#endif

/*! @name Driver version */
/*@{*/
/*! @brief IOCON driver version 2.0.0. */
#define FSL_IOCON_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/**
 * @brief Array of IOCON pin definitions passed to IOCON_SetPinMuxing() must be in this format
 */
typedef struct _iocon_group
{
    uint32_t port : 8;      /* Pin port */
    uint32_t pin : 8;       /* Pin number */
    uint32_t ionumber : 8;  /* IO number */
    uint32_t modefunc : 16; /* Function and mode */
} iocon_group_t;

/**
 * @brief IOCON function and mode selection definitions
 * @note See the User Manual for specific modes and functions supported by the various pins.
 */
#if defined(FSL_FEATURE_IOCON_FUNC_FIELD_WIDTH) && (FSL_FEATURE_IOCON_FUNC_FIELD_WIDTH == 4)
#define IOCON_FUNC0 0x0  /*!< Selects pin function 0 */
#define IOCON_FUNC1 0x1  /*!< Selects pin function 1 */
#define IOCON_FUNC2 0x2  /*!< Selects pin function 2 */
#define IOCON_FUNC3 0x3  /*!< Selects pin function 3 */
#define IOCON_FUNC4 0x4  /*!< Selects pin function 4 */
#define IOCON_FUNC5 0x5  /*!< Selects pin function 5 */
#define IOCON_FUNC6 0x6  /*!< Selects pin function 6 */
#define IOCON_FUNC7 0x7  /*!< Selects pin function 7 */
#define IOCON_FUNC8 0x8  /*!< Selects pin function 8 */
#define IOCON_FUNC9 0x9  /*!< Selects pin function 9 */
#define IOCON_FUNC10 0xA /*!< Selects pin function 10 */
#define IOCON_FUNC11 0xB /*!< Selects pin function 11 */
#define IOCON_FUNC12 0xC /*!< Selects pin function 12 */
#define IOCON_FUNC13 0xD /*!< Selects pin function 13 */
#define IOCON_FUNC14 0xE /*!< Selects pin function 14 */
#define IOCON_FUNC15 0xF /*!< Selects pin function 15 */
#if defined(IOCON_PIO_MODE_SHIFT)
#define IOCON_MODE_INACT (0x0 << IOCON_PIO_MODE_SHIFT)    /*!< No addition pin function */
#define IOCON_MODE_PULLDOWN (0x1 << IOCON_PIO_MODE_SHIFT) /*!< Selects pull-down function */
#define IOCON_MODE_PULLUP (0x2 << IOCON_PIO_MODE_SHIFT)   /*!< Selects pull-up function */
#define IOCON_MODE_REPEATER (0x3 << IOCON_PIO_MODE_SHIFT) /*!< Selects pin repeater function */
#endif

#if defined(IOCON_PIO_I2CSLEW_SHIFT)
#define IOCON_GPIO_MODE (0x1 << IOCON_PIO_I2CSLEW_SHIFT) /*!< GPIO Mode */
#define IOCON_I2C_SLEW (0x0 << IOCON_PIO_I2CSLEW_SHIFT)  /*!< I2C Slew Rate Control */
#endif

#if defined(IOCON_PIO_EGP_SHIFT)
#define IOCON_GPIO_MODE (0x1 << IOCON_PIO_EGP_SHIFT) /*!< GPIO Mode */
#define IOCON_I2C_SLEW (0x0 << IOCON_PIO_EGP_SHIFT)  /*!< I2C Slew Rate Control */
#endif

#if defined(IOCON_PIO_SLEW_SHIFT)
#define IOCON_SLEW_STANDARD (0x0 << IOCON_PIO_SLEW_SHIFT) /*!< Driver Slew Rate Control */
#define IOCON_SLEW_FAST (0x1 << IOCON_PIO_SLEW_SHIFT)     /*!< Driver Slew Rate Control */
#endif

#if defined(IOCON_PIO_INVERT_SHIFT)
#define IOCON_INV_EN (0x1 << IOCON_PIO_INVERT_SHIFT) /*!< Enables invert function on input */
#endif

#if defined(IOCON_PIO_DIGIMODE_SHIFT)
#define IOCON_ANALOG_EN (0x0 << IOCON_PIO_DIGIMODE_SHIFT) /*!< Enables analog function by setting 0 to bit 7 */
#define IOCON_DIGITAL_EN \
    (0x1 << IOCON_PIO_DIGIMODE_SHIFT) /*!< Enables digital function by setting 1 to bit 7(default) */
#endif

#if defined(IOCON_PIO_FILTEROFF_SHIFT)
#define IOCON_INPFILT_OFF (0x1 << IOCON_PIO_FILTEROFF_SHIFT) /*!< Input filter Off for GPIO pins */
#define IOCON_INPFILT_ON (0x0 << IOCON_PIO_FILTEROFF_SHIFT)  /*!< Input filter On for GPIO pins */
#endif

#if defined(IOCON_PIO_I2CDRIVE_SHIFT)
#define IOCON_I2C_LOWDRIVER (0x0 << IOCON_PIO_I2CDRIVE_SHIFT)  /*!< Low drive, Output drive sink is 4 mA */
#define IOCON_I2C_HIGHDRIVER (0x1 << IOCON_PIO_I2CDRIVE_SHIFT) /*!< High drive, Output drive sink is 20 mA */
#endif

#if defined(IOCON_PIO_OD_SHIFT)
#define IOCON_OPENDRAIN_EN (0x1 << IOCON_PIO_OD_SHIFT) /*!< Enables open-drain function */
#endif

#if defined(IOCON_PIO_I2CFILTER_SHIFT)
#define IOCON_I2CFILTER_OFF (0x1 << IOCON_PIO_I2CFILTER_SHIFT) /*!<  I2C 50 ns glitch filter enabled */
#define IOCON_I2CFILTER_ON (0x0 << IOCON_PIO_I2CFILTER_SHIFT)  /*!<  I2C 50 ns glitch filter not enabled,  */
#endif

#if defined(IOCON_PIO_ASW_SHIFT)
#define IOCON_AWS_EN (0x1 << IOCON_PIO_ASW_SHIFT) /*!< Enables analog switch function */
#endif

#if defined(IOCON_PIO_SSEL_SHIFT)
#define IOCON_SSEL_3V3 (0x0 << IOCON_PIO_SSEL_SHIFT) /*!< 3V3 signaling in I2C mode */
#define IOCON_SSEL_1V8 (0x1 << IOCON_PIO_SSEL_SHIFT) /*!< 1V8 signaling in I2C mode */
#endif

#if defined(IOCON_PIO_ECS_SHIFT)
#define IOCON_ECS_OFF (0x0 << IOCON_PIO_ECS_SHIFT) /*!< IO is an open drain cell */
#define IOCON_ECS_ON (0x1 << IOCON_PIO_ECS_SHIFT)  /*!< Pull-up resistor is connected */
#endif

#if defined(IOCON_PIO_S_MODE_SHIFT)
#define IOCON_S_MODE_0CLK (0x0 << IOCON_PIO_S_MODE_SHIFT) /*!< Bypass input filter */
#define IOCON_S_MODE_1CLK                                                                              \
    (0x1 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 1 filter clock are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE_2CLK                                                                               \
    (0x2 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 2 filter clock2 are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE_3CLK                                                                               \
    (0x3 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 3 filter clock2 are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE(clks) ((clks) << IOCON_PIO_S_MODE_SHIFT) /*!< Select clocks for digital input filter mode */
#endif

#if defined(IOCON_PIO_CLK_DIV_SHIFT)
#define IOCON_CLKDIV(div) \
    ((div)                \
     << IOCON_PIO_CLK_DIV_SHIFT) /*!< Select peripheral clock divider for input filter sampling clock, 2^n, n=0-6 */
#endif

#else
#define IOCON_FUNC0 0x0 /*!< Selects pin function 0 */
#define IOCON_FUNC1 0x1 /*!< Selects pin function 1 */
#define IOCON_FUNC2 0x2 /*!< Selects pin function 2 */
#define IOCON_FUNC3 0x3 /*!< Selects pin function 3 */
#define IOCON_FUNC4 0x4 /*!< Selects pin function 4 */
#define IOCON_FUNC5 0x5 /*!< Selects pin function 5 */
#define IOCON_FUNC6 0x6 /*!< Selects pin function 6 */
#define IOCON_FUNC7 0x7 /*!< Selects pin function 7 */

#if defined(IOCON_PIO_MODE_SHIFT)
#define IOCON_MODE_INACT (0x0 << IOCON_PIO_MODE_SHIFT)    /*!< No addition pin function */
#define IOCON_MODE_PULLDOWN (0x1 << IOCON_PIO_MODE_SHIFT) /*!< Selects pull-down function */
#define IOCON_MODE_PULLUP (0x2 << IOCON_PIO_MODE_SHIFT)   /*!< Selects pull-up function */
#define IOCON_MODE_REPEATER (0x3 << IOCON_PIO_MODE_SHIFT) /*!< Selects pin repeater function */
#endif

#if defined(IOCON_PIO_I2CSLEW_SHIFT)
#define IOCON_GPIO_MODE (0x1 << IOCON_PIO_I2CSLEW_SHIFT) /*!< GPIO Mode */
#define IOCON_I2C_SLEW (0x0 << IOCON_PIO_I2CSLEW_SHIFT)  /*!< I2C Slew Rate Control */
#endif

#if defined(IOCON_PIO_EGP_SHIFT)
#define IOCON_GPIO_MODE (0x1 << IOCON_PIO_EGP_SHIFT) /*!< GPIO Mode */
#define IOCON_I2C_SLEW (0x0 << IOCON_PIO_EGP_SHIFT)  /*!< I2C Slew Rate Control */
#endif

#if defined(IOCON_PIO_INVERT_SHIFT)
#define IOCON_INV_EN (0x1 << IOCON_PIO_INVERT_SHIFT) /*!< Enables invert function on input */
#endif

#if defined(IOCON_PIO_DIGIMODE_SHIFT)
#define IOCON_ANALOG_EN (0x0 << IOCON_PIO_DIGIMODE_SHIFT) /*!< Enables analog function by setting 0 to bit 7 */
#define IOCON_DIGITAL_EN \
    (0x1 << IOCON_PIO_DIGIMODE_SHIFT) /*!< Enables digital function by setting 1 to bit 7(default) */
#endif

#if defined(IOCON_PIO_FILTEROFF_SHIFT)
#define IOCON_INPFILT_OFF (0x1 << IOCON_PIO_FILTEROFF_SHIFT) /*!< Input filter Off for GPIO pins */
#define IOCON_INPFILT_ON (0x0 << IOCON_PIO_FILTEROFF_SHIFT)  /*!< Input filter On for GPIO pins */
#endif

#if defined(IOCON_PIO_I2CDRIVE_SHIFT)
#define IOCON_I2C_LOWDRIVER (0x0 << IOCON_PIO_I2CDRIVE_SHIFT)  /*!< Low drive, Output drive sink is 4 mA */
#define IOCON_I2C_HIGHDRIVER (0x1 << IOCON_PIO_I2CDRIVE_SHIFT) /*!< High drive, Output drive sink is 20 mA */
#endif

#if defined(IOCON_PIO_OD_SHIFT)
#define IOCON_OPENDRAIN_EN (0x1 << IOCON_PIO_OD_SHIFT) /*!< Enables open-drain function */
#endif

#if defined(IOCON_PIO_I2CFILTER_SHIFT)
#define IOCON_I2CFILTER_OFF (0x1 << IOCON_PIO_I2CFILTER_SHIFT) /*!<  I2C 50 ns glitch filter enabled */
#define IOCON_I2CFILTER_ON (0x0 << IOCON_PIO_I2CFILTER_SHIFT)  /*!<  I2C 50 ns glitch filter not enabled */
#endif

#if defined(IOCON_PIO_S_MODE_SHIFT)
#define IOCON_S_MODE_0CLK (0x0 << IOCON_PIO_S_MODE_SHIFT) /*!< Bypass input filter */
#define IOCON_S_MODE_1CLK                                                                              \
    (0x1 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 1 filter clock are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE_2CLK                                                                               \
    (0x2 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 2 filter clock2 are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE_3CLK                                                                               \
    (0x3 << IOCON_PIO_S_MODE_SHIFT) /*!< Input pulses shorter than 3 filter clock2 are rejected \ \ \ \ \
                                           */
#define IOCON_S_MODE(clks) ((clks) << IOCON_PIO_S_MODE_SHIFT) /*!< Select clocks for digital input filter mode */
#endif

#if defined(IOCON_PIO_CLK_DIV_SHIFT)
#define IOCON_CLKDIV(div) \
    ((div)                \
     << IOCON_PIO_CLK_DIV_SHIFT) /*!< Select peripheral clock divider for input filter sampling clock, 2^n, n=0-6 */
#endif

#endif
#if defined(__cplusplus)
extern "C" {
#endif

#if (defined(FSL_FEATURE_IOCON_ONE_DIMENSION) && (FSL_FEATURE_IOCON_ONE_DIMENSION == 1))
/**
 * @brief   Sets I/O Control pin mux
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   ionumber    : GPIO number to mux
 * @param   modefunc    : OR'ed values of type IOCON_*
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_PinMuxSet(IOCON_Type *base, uint8_t ionumber, uint32_t modefunc)
{
    base->PIO[ionumber] = modefunc;
}
#else
/**
 * @brief   Sets I/O Control pin mux
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   port        : GPIO port to mux
 * @param   pin         : GPIO pin to mux
 * @param   modefunc    : OR'ed values of type IOCON_*
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_PinMuxSet(IOCON_Type *base, uint8_t port, uint8_t pin, uint32_t modefunc)
{
    base->PIO[port][pin] = modefunc;
}
#endif

/**
 * @brief   Set all I/O Control pin muxing
 * @param   base        : The base of IOCON peripheral on the chip
 * @param   pinArray    : Pointer to array of pin mux selections
 * @param   arrayLength : Number of entries in pinArray
 * @return  Nothing
 */
__STATIC_INLINE void IOCON_SetPinMuxing(IOCON_Type *base, const iocon_group_t *pinArray, uint32_t arrayLength)
{
    uint32_t i;

    for (i = 0; i < arrayLength; i++)
    {
#if (defined(FSL_FEATURE_IOCON_ONE_DIMENSION) && (FSL_FEATURE_IOCON_ONE_DIMENSION == 1))
        IOCON_PinMuxSet(base, pinArray[i].ionumber, pinArray[i].modefunc);
#else
        IOCON_PinMuxSet(base, pinArray[i].port, pinArray[i].pin, pinArray[i].modefunc);
#endif /* FSL_FEATURE_IOCON_ONE_DIMENSION */
    }
}

/* @} */

#if defined(__cplusplus)
}
#endif

#endif /* _FSL_IOCON_H_ */

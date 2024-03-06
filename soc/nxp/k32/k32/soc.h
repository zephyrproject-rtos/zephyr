/*
 * Copyright (c) 2023 Sendrato
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_K32_SOC_H_
#define ZEPHYR_SOC_ARM_NXP_K32_SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#endif /* !_ASMLANGUAGE */

/* ICON definitions are copied from `board/pin_mux.h` from several examples in
 * the MCUXpresso IDE for the QN9090DK6 development board.
 *
 * NOTE: This list of definitions might not be complete.
 */

/*!@brief Enables digital function */
#define IOCON_PIO_DIGITAL_EN 0x80u

/*!@brief IO is an open drain cell */
#define IOCON_PIO_ECS_DI 0x00u

/*!@brief I2C mode */
#define IOCON_PIO_EGP_I2C 0x00u

/*!@brief High speed IO for GPIO mode, IIC not */
#define IOCON_PIO_EHS_DI 0x00u

/*!<@brief IIC mode:Noise pulses below approximately 50ns are filtered out. GPIO mode:a 3ns filter
 */
#define IOCON_PIO_FSEL_DI 0x00u

/*!@brief Selects pin function 0 */
#define IOCON_PIO_FUNC0 0x00u
/*!@brief Selects pin function 1 */
#define IOCON_PIO_FUNC1 0x01u
/*!@brief Selects pin function 2 */
#define IOCON_PIO_FUNC2 0x02u
/*!@brief Selects pin function 5 */
#define IOCON_PIO_FUNC5 0x05u
/*!<@brief Selects pin function 6 */
#define IOCON_PIO_FUNC6 0x06u
/*!<@brief Selects pin function 7 */
#define IOCON_PIO_FUNC7 0x07u

/*!@brief Input filter disabled */
#define IOCON_PIO_INPFILT_OFF 0x0100u
/*!<@brief Input filter enabled */
#define IOCON_PIO_INPFILT_ON  0x00u

/*!@brief Input function is not inverted */
#define IOCON_PIO_INV_DI 0x00u

/*!@brief IO_CLAMP disabled */
#define IOCON_PIO_IO_CLAMP_DI 0x00u

/*!@brief Selects pull-up function */
#define IOCON_PIO_MODE_PULLUP   0x00u
/*!@brief Selects pull-down function */
#define IOCON_PIO_MODE_PULLDOWN 0x18u

/*!@brief Open drain is disabled */
#define IOCON_PIO_OPENDRAIN_DI 0x00u
/*!@brief Open drain is enabled */
#define IOCON_PIO_OPENDRAIN_EN 0x0400u

/*!@brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SLEW0_STANDARD 0x00u
/*!@brief Standard mode, output slew rate control is disabled */
#define IOCON_PIO_SLEW1_STANDARD 0x00u

#define IOCON_PIO_SLEW0_FAST 0x20u   /*!<@brief Fast mode, slew rate control is enabled */
#define IOCON_PIO_SLEW1_FAST 0x0200u /*!<@brief Fast mode, slew rate control is enabled */

/*!@brief SSEL is disabled */
#define IOCON_PIO_SSEL_DI 0x00u

/*!<@brief Enables digital function */
#define IOCON_PIO_DIGITAL_EN 0x80u

#endif /* ZEPHYR_SOC_ARM_NXP_K32_SOC_H_ */

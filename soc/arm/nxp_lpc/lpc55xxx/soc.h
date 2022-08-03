/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the nxp_lpc55s69 platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'nxp_lpc55s69' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/util.h>
#include <fsl_common.h>


#endif /* !_ASMLANGUAGE */

#define IOCON_PIO_DIGITAL_EN 0x0100u  /*!<@brief Enables digital function */
#define IOCON_PIO_FUNC0 0x00u         /*!<@brief Selects pin function 0 */
#define IOCON_PIO_FUNC1 0x01u         /*!<@brief Selects pin function 1 */
#define IOCON_PIO_FUNC2 0x02u         /*!<@brief Selects pin function 2 */
#define IOCON_PIO_FUNC4 0x04u         /*!<@brief Selects pin function 4 */
#define IOCON_PIO_FUNC5 0x05u         /*!<@brief Selects pin function 5 */
#define IOCON_PIO_FUNC6 0x06u         /*!<@brief Selects pin function 6 */
#define IOCON_PIO_FUNC7 0x07u         /*!<@brief Selects pin function 7 */
#define IOCON_PIO_FUNC9 0x09u         /*!<@brief Selects pin function 9 */
#define IOCON_PIO_FUNC10 0x0Au        /*!<@brief Selects pin function 10 */
#define IOCON_PIO_FUNC11 0x0Bu        /*!<@brief Selects pin function 11 */

#define IOCON_PIO_INV_DI 0x00u        /*!<@brief Input function not inverted */
#define IOCON_PIO_MODE_INACT 0x00u    /*!<@brief No addition pin function */
#define IOCON_PIO_OPENDRAIN_DI 0x00u  /*!<@brief Open drain is disabled */
#define IOCON_PIO_SLEW_STANDARD 0x00u /*!<@brief Standard slew rate mode */
#define IOCON_PIO_SLEW_FAST 0x40u     /*!<@brief Fast slew rate mode */
#define IOCON_PIO_MODE_PULLDOWN 0x10u /*!<@brief Selects pull-down function */
#define IOCON_PIO_MODE_PULLUP 0x20u   /*!<@brief Selects pull-up function */
#define IOCON_PIO_ASW_EN 0x0400u      /*!<@brief Analog switch is closed (enabled) */
#define IOCON_PIO_INPFILT_OFF 0x1000u /*!<@brief Input filter disabled */

#endif /* _SOC__H_ */

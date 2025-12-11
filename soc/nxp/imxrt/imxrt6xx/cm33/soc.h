/*
 * Copyright 2020, 2025 NXP
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
#include <fsl_power.h>
#include <soc_common.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#endif /* !_ASMLANGUAGE */

/*!<@brief Analog mux is disabled */
#define IOPCTL_PIO_ANAMUX_DI 0x00u
/*!<@brief Analog mux is enabled */
#define IOPCTL_PIO_ANAMUX_EN 0x0200u
/*!<@brief Normal drive */
#define IOPCTL_PIO_FULLDRIVE_DI 0x00u
/*!<@brief Full drive */
#define IOPCTL_PIO_FULLDRIVE_EN 0x0100u
/*!<@brief Selects pin function 0 */
#define IOPCTL_PIO_FUNC0 0x00u
/*!<@brief Selects pin function 1 */
#define IOPCTL_PIO_FUNC1 0x01u
/*!<@brief Selects pin function 2 */
#define IOPCTL_PIO_FUNC2 0x02u
/*!<@brief Selects pin function 3 */
#define IOPCTL_PIO_FUNC3 0x03u
/*!<@brief Selects pin function 4 */
#define IOPCTL_PIO_FUNC4 0x04u
/*!<@brief Selects pin function 5 */
#define IOPCTL_PIO_FUNC5 0x05u
/*!<@brief Selects pin function 6 */
#define IOPCTL_PIO_FUNC6 0x06u
/*!<@brief Selects pin function 7 */
#define IOPCTL_PIO_FUNC7 0x07u
/*!<@brief Selects pin function 8 */
#define IOPCTL_PIO_FUNC8 0x08u
/*!<@brief Disable input buffer function */
#define IOPCTL_PIO_INBUF_DI 0x00u
/*!<@brief Enables input buffer function */
#define IOPCTL_PIO_INBUF_EN 0x40u
/*!<@brief Input function is not inverted */
#define IOPCTL_PIO_INV_DI 0x00u
/*!<@brief Input function is inverted */
#define IOPCTL_PIO_INV_EN 0x0800u
/*!<@brief Pseudo Output Drain is disabled */
#define IOPCTL_PIO_PSEDRAIN_DI 0x00u
/*!<@brief Pseudo Output Drain is enabled */
#define IOPCTL_PIO_PSEDRAIN_EN 0x0400u
/*!<@brief Enable pull-down function */
#define IOPCTL_PIO_PULLDOWN_EN 0x00u
/*!<@brief Enable pull-up function */
#define IOPCTL_PIO_PULLUP_EN 0x20u
/*!<@brief Disable pull-up / pull-down function */
#define IOPCTL_PIO_PUPD_DI 0x00u
/*!<@brief Enable pull-up / pull-down function */
#define IOPCTL_PIO_PUPD_EN 0x10u
/*!<@brief Normal mode */
#define IOPCTL_PIO_SLEW_RATE_NORMAL 0x00u
/*!<@brief Slow mode */
#define IOPCTL_PIO_SLEW_RATE_SLOW 0x80u

/* Workaround to handle macro variation in the SDK */
#ifndef INPUTMUX_PINTSEL_COUNT
#define INPUTMUX_PINTSEL_COUNT INPUTMUX_PINT_SEL_COUNT
#endif

/* Handle variation to implement Wakeup Interrupt */
#undef NXP_ENABLE_WAKEUP_SIGNAL
#undef NXP_DISABLE_WAKEUP_SIGNAL
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) EnableDeepSleepIRQ(irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) DisableDeepSleepIRQ(irqn)

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_IMX_USDHC &&					\
	(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc0)) ||	\
	 DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usdhc1)))

void imxrt_usdhc_pinmux(uint16_t nusdhc,
	bool init, uint32_t speed, uint32_t strength);
void imxrt_usdhc_dat3_pull(bool pullup);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */

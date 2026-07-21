/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control header file for Microchip SoC devices.
 *
 * This header selects the SoC-family-specific Microchip clock definitions based on the
 * active Kconfig selection.
 * @ingroup clock_control_mchp
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_
#define INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_

/**
 * @defgroup clock_control_mchp Microchip
 * @ingroup clock_control_interface_ext
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>

#if CONFIG_CLOCK_CONTROL_MCHP_SAM_D5X_E5X
#include <zephyr/drivers/clock_control/mchp_clock_sam_d5x_e5x.h>
#endif /* CLOCK_CONTROL_MCHP_SAM_D5X_E5X */

#if CONFIG_CLOCK_CONTROL_MCHP_PIC32CM_JH
#include <zephyr/drivers/clock_control/mchp_clock_pic32cm_jh.h>
#endif /* CONFIG_CLOCK_CONTROL_MCHP_PIC32CM_JH */

#if CONFIG_CLOCK_CONTROL_MCHP_PIC32CM_PL
#include <zephyr/drivers/clock_control/mchp_clock_pic32cm_pl.h>
#endif /* CONFIG_CLOCK_CONTROL_MCHP_PIC32CM_PL */

#if CONFIG_CLOCK_CONTROL_MCHP_PIC32CZ_CA
#include <zephyr/drivers/clock_control/mchp_clock_pic32cz_ca.h>
#endif /* CONFIG_CLOCK_CONTROL_MCHP_PIC32CZ_CA */

/** @} */

#endif /* INCLUDE_ZEPHYR_DRIVERS_CLOCK_CONTROL_MCHP_CLOCK_CONTROL_H_ */

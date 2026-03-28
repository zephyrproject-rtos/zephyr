/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_H__
#define NRFX_CONFIG_H__

/* Define nrfx API version used in Zephyr. */
#define NRFX_CONFIG_API_VER_MAJOR 4
#define NRFX_CONFIG_API_VER_MINOR 1
#define NRFX_CONFIG_API_VER_MICRO 0

/* Macros used in zephyr-specific config files. */
#include "nrfx_zephyr_utils.h"

/* Define nrfx configuration based on Zephyrs KConfigs. */
#include "nrfx_kconfig.h"

/* Define resources reserved outside nrfx scope. */
#ifdef CONFIG_NRFX_RESERVED_RESOURCES_HEADER
#include CONFIG_NRFX_RESERVED_RESOURCES_HEADER
#endif

/* Include babble-sim configuration. */
#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "nrfx_config_bsim.h"
#endif

/* Use defaults for undefined symbols. */
#include "nrfx_templates_config.h"
#endif // NRFX_CONFIG_H__

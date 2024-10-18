/*
 * Copyright (c) 2022 - 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_COMMON_H__
#define NRFX_CONFIG_COMMON_H__

#ifndef NRFX_CONFIG_H__
#error "This file should not be included directly. Include nrfx_config.h instead."
#endif

/** @brief Symbol specifying major version of the nrfx API to be used. */
#ifndef NRFX_CONFIG_API_VER_MAJOR
#define NRFX_CONFIG_API_VER_MAJOR 3
#endif

/** @brief Symbol specifying minor version of the nrfx API to be used. */
#ifndef NRFX_CONFIG_API_VER_MINOR
#define NRFX_CONFIG_API_VER_MINOR 7
#endif

/** @brief Symbol specifying micro version of the nrfx API to be used. */
#ifndef NRFX_CONFIG_API_VER_MICRO
#define NRFX_CONFIG_API_VER_MICRO 0
#endif

#endif /* NRFX_CONFIG_COMMON_H__ */

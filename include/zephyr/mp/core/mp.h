/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Umbrella header.
 *
 * Applications should include this header for the basic core APIs.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_H_

/**
 * @defgroup mp MultimediaPipe
 * @ingroup os_services
 * @brief MultimediaPipe (MP) subsystem.
 * @since 4.5
 */

/**
 * @defgroup mp_core Core
 * @ingroup mp
 * @brief Core MultimediaPipe APIs.
 */

/**
 * @defgroup mp_plugins Plugins
 * @ingroup mp
 * @brief MultimediaPipe plugins.
 */

#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>
#if CONFIG_MP_RPC
#include <zephyr/mp/core/mp_transform_client.h>
#endif

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_H_ */

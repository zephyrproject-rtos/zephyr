/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Media Pipeline (MP) umbrella header.
 *
 * Applications should include this header for the core MP APIs.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_H_

/**
 * @brief Media Pipeline (MP) subsystem.
 * @defgroup mp Media Pipeline
 * @since 4.2
 * @ingroup os_services
 * @{
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

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_H_ */

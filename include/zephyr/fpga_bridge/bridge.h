/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BRIDGE_H_
#define ZEPHYR_INCLUDE_BRIDGE_H_

#include <zephyr/kernel.h>

/**
 * @file
 *
 * @brief API for bridge enable/disable
 */
#define BRIDGES_MASK	0xffffffff

/**
 * @brief Do the bridge reset based on action enable/disable
 *
 * @param[in] action Enable/Disable bridges
 * @param[in] mask Bridge mask
 *
 * @return 0 on success, negative errno code on fail
 */
int32_t do_bridge_reset(uint32_t action, uint32_t mask);

#endif

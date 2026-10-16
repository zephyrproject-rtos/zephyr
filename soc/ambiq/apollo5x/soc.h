/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AMBIQ_APOLLO_5X_SOC_H_
#define ZEPHYR_SOC_AMBIQ_APOLLO_5X_SOC_H_

#include <am_mcu_apollo.h>

bool buf_in_nocache(uintptr_t buf, size_t len_bytes);

#endif /* ZEPHYR_SOC_AMBIQ_APOLLO_5X_SOC_H_ */

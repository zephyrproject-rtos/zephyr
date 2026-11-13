/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Deprecated location of the boot mode interface, use <zephyr/bootmode.h>.
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_BOOTMODE_H_
#define ZEPHYR_INCLUDE_RETENTION_BOOTMODE_H_

#include <zephyr/toolchain.h>
#include <zephyr/bootmode.h>

__WARN("This header is deprecated, use `zephyr/bootmode.h` instead");

#endif /* ZEPHYR_INCLUDE_RETENTION_BOOTMODE_H_ */

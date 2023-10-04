/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_TYPES_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_TYPES_H_

/**
 * @brief ZSAI internal Interface
 * @defgroup zsai_internal_interface ZSAI internal Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 */

struct zsai_ioctl_range {
	uint32_t offset;
	uint32_t size;
};

typedef struct zsai_ioctl_range zsai_range;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_TYPES_H_ */

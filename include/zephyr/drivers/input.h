/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INPUT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INPUT_H_

/**
 * @brief Input Driver Interface
 * @defgroup input_driver_interface Input Driver Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Input device driver API. */
__subsystem struct input_driver_api {
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INPUT_H_ */

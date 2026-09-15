/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for controlling generic emulated sensor test device
 * @ingroup generic_emul
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>

/**
 * @defgroup generic_emul Generic Sensor Emulator
 * @ingroup sensor_interface_ext_zephyr
 * @brief Generic sensor emulator for testing
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset all channels back to unconfigured
 *
 * @param target Generic sensor emulator to reset
 * @param reset_rc Reset the return codes
 */
void generic_emul_reset(const struct emul *target, bool reset_rc);

/**
 * @brief Configure return value for generic sensor device
 *
 * @param target Generic sensor emulator to configure
 * @param resume_rc Return code for @a PM_DEVICE_ACTION_RESUME
 * @param suspend_rc Return code for @a PM_DEVICE_ACTION_SUSPEND
 * @param fetch_rc Return code for @a sensor_sample_fetch
 */
void generic_emul_func_rc(const struct emul *target, int resume_rc, int suspend_rc, int fetch_rc);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_GENERIC_EMUL_H_ */

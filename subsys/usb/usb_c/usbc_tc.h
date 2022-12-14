/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_TC_H_
#define ZEPHYR_SUBSYS_USBC_TC_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/smf.h>

#include "usbc_stack.h"

/**
 * @brief This function must only be called in the subsystem init function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void tc_subsys_init(const struct device *dev);

/**
 * @brief Run the TC Layer state machine. This is called from the subsystems
 *	  port stack thread.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dpm_request Device Policy Manager request
 */
void tc_run(const struct device *dev, int32_t dpm_request);

/**
 * @brief Checks if the TC Layer is in an Attached state
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval true if TC Layer is in an Attached state, else false
 */
bool tc_is_in_attached_state(const struct device *dev);

#endif /* ZEPHYR_SUBSYS_USBC_TC_H_ */

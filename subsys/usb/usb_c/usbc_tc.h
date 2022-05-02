/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_H_
#define ZEPHYR_SUBSYS_USBC_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/smf.h>
#include "usbc_timer.h"

/**
 * @brief TC Layer State Machine Object
 */
struct tc_sm_t {
	/** TC layer state machine context */
	struct smf_ctx ctx;
	/** Port device */
	const struct device *dev;
	/** TC layer flags */
	atomic_t flags;
	/** VBUS measurement device */
	const struct device *vbus_dev;
	/** Port polarity */
	enum tc_cc_polarity cc_polarity;
	/** The cc state */
	enum tc_cc_states cc_state;
	/** Voltage on CC pin */
	enum tc_cc_voltage_state cc_voltage;
	/** Current CC1 value */
	enum tc_cc_voltage_state cc1;
	/** Current CC2 value */
	enum tc_cc_voltage_state cc2;

	/* Timers */

	/** tCCDebounce timer */
	struct usbc_timer_t tc_t_cc_debounce;
	/** tRpValueChange timer */
	struct usbc_timer_t tc_t_rp_value_change;
	/** tErrorRecovery timer */
	struct usbc_timer_t tc_t_error_recovery;
};

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

/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USBC_TC_COMMON_INTERNAL_H_
#define ZEPHYR_SUBSYS_USBC_TC_COMMON_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/smf.h>

#include "usbc_timer.h"
#include "usbc_stack.h"
#include "usbc_tc.h"

/**
 * @brief Type-C States
 */
enum tc_state_t {
	/** Super state that opens the CC lines */
	TC_CC_OPEN_SUPER_STATE,
	/** Super state that applies Rd to the CC lines */
	TC_CC_RD_SUPER_STATE,
	/** Disabled State */
	TC_DISABLED_STATE,
	/** Error Recovery State */
	TC_ERROR_RECOVERY_STATE,
	/** Unattached Sink State */
	TC_UNATTACHED_SNK_STATE,
	/** Attach Wait Sink State */
	TC_ATTACH_WAIT_SNK_STATE,
	/** Attached Sink State */
	TC_ATTACHED_SNK_STATE,
};

/**
 * @brief Type-C Layer flags
 */
enum tc_flags {
	/**
	 * Flag to track Rp resistor change when the sink attached
	 * sub-state runs
	 */
	TC_FLAGS_RP_SUBSTATE_CHANGE = 0,
};

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
 * @brief Sets a Type-C State
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param state next Type-C State to enter
 */
void tc_set_state(const struct device *dev, const enum tc_state_t state);

/**
 * @brief Get current Type-C State
 *
 * @param dev Pointer to the device structure for the driver instance
 * @return current Type-C state
 */
enum tc_state_t tc_get_state(const struct device *dev);

/**
 * @brief Enable Power Delivery
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param enable set true to enable Power Deliver
 */
void tc_pd_enable(const struct device *dev, const bool enable);

#endif /* ZEPHYR_SUBSYS_USBC_TC_COMMON_INTERNAL_H_ */

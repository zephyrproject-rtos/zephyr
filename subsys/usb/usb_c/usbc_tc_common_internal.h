/*
 * Copyright (c) 2023 The Chromium OS Authors
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

enum tc_flags {
	/**
	 * Flag to track Rp resistor change when the sink attached
	 * sub-state runs
	 */
	TC_FLAGS_RP_SUBSTATE_CHANGE = 0,
	/** Tracks if VCONN is ON or OFF */
	TC_FLAGS_VCONN_ON,
};

/**
 * @brief Type-C States
 */
enum tc_state_t {
	/** Super state that opens the CC lines */
	TC_CC_OPEN_SUPER_STATE,
#ifdef CONFIG_USBC_CSM_SINK_ONLY
	/** Super state that applies Rd to the CC lines */
	TC_CC_RD_SUPER_STATE,
	/** Unattached Sink State */
	TC_UNATTACHED_SNK_STATE,
	/** Attach Wait Sink State */
	TC_ATTACH_WAIT_SNK_STATE,
	/** Attached Sink State */
	TC_ATTACHED_SNK_STATE,
#else
	/** Super state that applies Rp to the CC lines */
	TC_CC_RP_SUPER_STATE,
	/** Unattached Source State */
	TC_UNATTACHED_SRC_STATE,
	/** Unattached Wait Source State */
	TC_UNATTACHED_WAIT_SRC_STATE,
	/** Attach Wait Source State */
	TC_ATTACH_WAIT_SRC_STATE,
	/** Attached Source State */
	TC_ATTACHED_SRC_STATE,
#endif
	/** Disabled State */
	TC_DISABLED_STATE,
	/** Error Recovery State */
	TC_ERROR_RECOVERY_STATE,

	/** Number of TC States */
	TC_STATE_COUNT
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
#ifdef CONFIG_USBC_CSM_SOURCE_ONLY
	/** tVconnOff timer */
	struct usbc_timer_t tc_t_vconn_off;
#endif
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

/**
 * @brief This function must only be called in the subsystem init function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
void tc_subsys_init(const struct device *dev);

/**
 * @brief Run the TC Layer state machine. This is called from the subsystems
 *        port stack thread.
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

/**
 * @brief Sets the Collision Avoidance Rp value
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param rp Collision Avoidance Rp to set
 */
void tc_select_src_collision_rp(const struct device *dev, enum tc_rp_value rp);

#endif /* ZEPHYR_SUBSYS_USBC_TC_COMMON_INTERNAL_H_ */

/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup pse_interface
 * @brief Main header file for Power Sourcing Equipment (PSE) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PSE_H_
#define ZEPHYR_INCLUDE_DRIVERS_PSE_H_

/**
 * @brief Power Sourcing Equipment (PSE) Interface
 * @defgroup pse_interface Power Sourcing Equipment (PSE)
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PSE power sourcing protocol family.
 *
 * A bitmask: a controller's supported types can be tested with `&`.
 */
enum pse_type {
	/** IEEE 802.3 Clause 104 Power over Data Lines (single balanced pair). */
	PSE_TYPE_PODL = BIT(0),
	/** IEEE 802.3 Clause 33 / Clause 145 multi-pair Power over Ethernet. */
	PSE_TYPE_POE = BIT(1),
};

/**
 * @brief Common PSE controller config.
 *
 * This structure **must** be placed first in the driver's config structure.
 */
struct pse_common_config {
	/** Which protocol family this controller implements. */
	enum pse_type type;
};

/**
 * @brief Read which PSE protocol family a controller implements.
 *
 * Determines whether a pse_pi_get_pw_status() result should be
 * interpreted via @ref pse_podl_pw_status or @ref pse_poe_pw_status.
 *
 * @param dev PSE controller device instance.
 *
 * @return The controller's supported type(s); see @ref pse_type.
 */
static inline enum pse_type pse_get_type(const struct device *dev)
{
	const struct pse_common_config *config = (const struct pse_common_config *)dev->config;

	return config->type;
}

/**
 * @brief Power detection status of a @ref PSE_TYPE_PODL PI.
 *
 * IEEE 802.3-2018 30.15.1.1.3 aPoDLPSEPowerDetectionStatus.
 */
enum pse_podl_pw_status {
	/** Power detection status is not yet known. */
	PSE_PODL_PW_STATUS_UNKNOWN = 1,
	/** `mr_pse_enable` is false. */
	PSE_PODL_PW_STATUS_DISABLED,
	/** `pi_detecting` or `pi_classifying` is true. */
	PSE_PODL_PW_STATUS_SEARCHING,
	/** `pi_powered` is true. */
	PSE_PODL_PW_STATUS_DELIVERING,
	/** `pi_sleeping` is true. */
	PSE_PODL_PW_STATUS_SLEEP,
	/** `pi_prebiased && !pi_sleeping` is true. */
	PSE_PODL_PW_STATUS_IDLE,
	/** `overload_held` is true. */
	PSE_PODL_PW_STATUS_ERROR,
};

/**
 * @brief Power detection status of a @ref PSE_TYPE_POE PI.
 *
 * IEEE 802.3-2022 30.9.1.1.5 aPSEPowerDetectionStatus. Type 3 and Type 4
 * PSEs (Clause 145) never report @ref PSE_POE_PW_STATUS_TEST or
 * @ref PSE_POE_PW_STATUS_FAULT — those two only occur on Type 1/2
 * (Clause 33) controllers.
 */
enum pse_poe_pw_status {
	/** Power detection status is not yet known. */
	PSE_POE_PW_STATUS_UNKNOWN = 1,
	/** PSE state diagram is in the state DISABLED. */
	PSE_POE_PW_STATUS_DISABLED,
	/** PSE state diagram is in a state other than those listed. */
	PSE_POE_PW_STATUS_SEARCHING,
	/** PSE state diagram is in the state POWER_ON. */
	PSE_POE_PW_STATUS_DELIVERING,
	/** PSE state diagram is in the state TEST_MODE. */
	PSE_POE_PW_STATUS_TEST,
	/** PSE state diagram is in the state TEST_ERROR. */
	PSE_POE_PW_STATUS_FAULT,
	/** The state diagram variable error_condition is true. */
	PSE_POE_PW_STATUS_OTHERFAULT,
};

/**
 * @brief Extended-state group of a @ref PSE_TYPE_POE PI.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables.
 */
enum pse_ext_state {
	/** Group of @ref pse_ext_state_error_condition substates. */
	PSE_EXT_STATE_ERROR_CONDITION = 1,
	/** Group of @ref pse_ext_state_mr_mps_valid substates. */
	PSE_EXT_STATE_MR_MPS_VALID,
	/** Group of @ref pse_ext_state_mr_pse_enable substates. */
	PSE_EXT_STATE_MR_PSE_ENABLE,
	/** Group of @ref pse_ext_state_option_detect_ted substates. */
	PSE_EXT_STATE_OPTION_DETECT_TED,
	/** Group of @ref pse_ext_state_option_vport_lim substates. */
	PSE_EXT_STATE_OPTION_VPORT_LIM,
	/** Group of @ref pse_ext_state_ovld_detected substates. */
	PSE_EXT_STATE_OVLD_DETECTED,
	/** PD power type reported over the data link layer; has no substate enum. */
	PSE_EXT_STATE_PD_DLL_POWER_TYPE,
	/** Group of @ref pse_ext_state_power_not_available substates. */
	PSE_EXT_STATE_POWER_NOT_AVAILABLE,
	/** Group of @ref pse_ext_state_short_detected substates. */
	PSE_EXT_STATE_SHORT_DETECTED,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_ERROR_CONDITION.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (error_condition): implementation-
 * specific fault conditions, distinct from the ones tracked by the main
 * PSE state diagram, that require the PSE not to source power.
 */
enum pse_ext_state_error_condition {
	/** The addressed port does not exist. */
	PSE_EXT_STATE_ERROR_CONDITION_NON_EXISTING_PORT = 1,
	/** The port is not defined/configured. */
	PSE_EXT_STATE_ERROR_CONDITION_UNDEFINED_PORT,
	/** An internal hardware fault was detected. */
	PSE_EXT_STATE_ERROR_CONDITION_INTERNAL_HW_FAULT,
	/** Communication error occurred after a force-on command. */
	PSE_EXT_STATE_ERROR_CONDITION_COMM_ERROR_AFTER_FORCE_ON,
	/** Port status could not be determined. */
	PSE_EXT_STATE_ERROR_CONDITION_UNKNOWN_PORT_STATUS,
	/** Power was turned off following a host crash. */
	PSE_EXT_STATE_ERROR_CONDITION_HOST_CRASH_TURN_OFF,
	/** Power was force-shut-down following a host crash. */
	PSE_EXT_STATE_ERROR_CONDITION_HOST_CRASH_FORCE_SHUTDOWN,
	/** A configuration change disabled the port. */
	PSE_EXT_STATE_ERROR_CONDITION_CONFIG_CHANGE,
	/** An over-temperature condition was detected. */
	PSE_EXT_STATE_ERROR_CONDITION_DETECTED_OVER_TEMP,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_MR_MPS_VALID.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (mr_mps_valid): presence or absence of
 * a valid Maintain Power Signature (33.2.9.1).
 */
enum pse_ext_state_mr_mps_valid {
	/** An underload condition was detected on the port. */
	PSE_EXT_STATE_MR_MPS_VALID_DETECTED_UNDERLOAD = 1,
	/** The port connection is open (no valid PD). */
	PSE_EXT_STATE_MR_MPS_VALID_CONNECTION_OPEN,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_MR_PSE_ENABLE.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (mr_pse_enable).
 */
enum pse_ext_state_mr_pse_enable {
	/** The hardware disable pin is active, holding the port off. */
	PSE_EXT_STATE_MR_PSE_ENABLE_DISABLE_PIN_ACTIVE = 1,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_OPTION_DETECT_TED.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (option_detect_ted): whether detection
 * can be performed during the ted_timer interval.
 */
enum pse_ext_state_option_detect_ted {
	/** Detection is currently in process. */
	PSE_EXT_STATE_OPTION_DETECT_TED_DET_IN_PROCESS = 1,
	/** The connection check failed. */
	PSE_EXT_STATE_OPTION_DETECT_TED_CONNECTION_CHECK_ERROR,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_OPTION_VPORT_LIM.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (option_vport_lim): whether VPSE is out
 * of the operating range during normal operation.
 */
enum pse_ext_state_option_vport_lim {
	/** Port voltage is above the operating range. */
	PSE_EXT_STATE_OPTION_VPORT_LIM_HIGH_VOLTAGE = 1,
	/** Port voltage is below the operating range. */
	PSE_EXT_STATE_OPTION_VPORT_LIM_LOW_VOLTAGE,
	/** Externally injected voltage was detected on the port. */
	PSE_EXT_STATE_OPTION_VPORT_LIM_VOLTAGE_INJECTION,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_OVLD_DETECTED.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (ovld_detected): output current has
 * been in an overload condition (33.2.7.6) for at least TCUT of a
 * one-second sliding window.
 */
enum pse_ext_state_ovld_detected {
	/** An output overload condition was detected. */
	PSE_EXT_STATE_OVLD_DETECTED_OVERLOAD = 1,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_POWER_NOT_AVAILABLE.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (power_not_available): asserted when
 * the PSE can no longer source sufficient power for the attached PD's
 * classified power requirement (33.2.6).
 */
enum pse_ext_state_power_not_available {
	/** The controller's overall power budget is exceeded. */
	PSE_EXT_STATE_POWER_NOT_AVAILABLE_BUDGET_EXCEEDED = 1,
	/** The port power limit exceeds the remaining controller budget. */
	PSE_EXT_STATE_POWER_NOT_AVAILABLE_PORT_PW_LIMIT_EXCEEDS_CONTROLLER_BUDGET,
	/** The PD's requested power exceeds the port limit. */
	PSE_EXT_STATE_POWER_NOT_AVAILABLE_PD_REQUEST_EXCEEDS_PORT_LIMIT,
	/** A hardware power limit prevents sourcing the requested power. */
	PSE_EXT_STATE_POWER_NOT_AVAILABLE_HW_PW_LIMIT,
};

/**
 * @brief Substate values for @ref PSE_EXT_STATE_SHORT_DETECTED.
 *
 * IEEE 802.3-2022 33.2.4.4 Variables (short_detected): output current has
 * been in a short circuit condition for TLIM within a sliding window
 * (33.2.7.7).
 */
enum pse_ext_state_short_detected {
	/** An output short-circuit condition was detected. */
	PSE_EXT_STATE_SHORT_DETECTED_SHORT_CONDITION = 1,
};

/**
 * @brief Extended state of a @ref PSE_TYPE_POE PI.
 *
 * @p substate is a tagged union: which member is valid is determined by
 * @p group. @ref PSE_EXT_STATE_PD_DLL_POWER_TYPE has no dedicated substate
 * enum defined here yet; use @p substate.raw for it.
 */
struct pse_ext_state_info {
	/** Which extended-state group this state belongs to. */
	enum pse_ext_state group;
	/** Group-specific substate value; which member is valid is given by @p group. */
	union {
		/** Valid when @p group is @ref PSE_EXT_STATE_ERROR_CONDITION. */
		enum pse_ext_state_error_condition error_condition;
		/** Valid when @p group is @ref PSE_EXT_STATE_MR_MPS_VALID. */
		enum pse_ext_state_mr_mps_valid mr_mps_valid;
		/** Valid when @p group is @ref PSE_EXT_STATE_MR_PSE_ENABLE. */
		enum pse_ext_state_mr_pse_enable mr_pse_enable;
		/** Valid when @p group is @ref PSE_EXT_STATE_OPTION_DETECT_TED. */
		enum pse_ext_state_option_detect_ted option_detect_ted;
		/** Valid when @p group is @ref PSE_EXT_STATE_OPTION_VPORT_LIM. */
		enum pse_ext_state_option_vport_lim option_vport_lim;
		/** Valid when @p group is @ref PSE_EXT_STATE_OVLD_DETECTED. */
		enum pse_ext_state_ovld_detected ovld_detected;
		/** Valid when @p group is @ref PSE_EXT_STATE_POWER_NOT_AVAILABLE. */
		enum pse_ext_state_power_not_available power_not_available;
		/** Valid when @p group is @ref PSE_EXT_STATE_SHORT_DETECTED. */
		enum pse_ext_state_short_detected short_detected;
		/** Raw value, for @ref PSE_EXT_STATE_PD_DLL_POWER_TYPE or any
		 *  group without a dedicated substate enum above.
		 */
		uint32_t raw;
	} substate;
};

/**
 * @brief One supported power-limit configuration range of a PI.
 *
 * Fetched one at a time by index via pse_pi_get_pw_limit_range(), after
 * pse_pi_get_pw_limit_range_count().
 */
struct pse_pw_limit_range {
	/** Minimum configurable power limit, in microwatts. */
	uint32_t min_uw;
	/** Maximum configurable power limit, in microwatts. */
	uint32_t max_uw;
};

/**
 * @brief Enable power sourcing on a PI.
 *
 * See pse_pi_enable() for argument description.
 */
typedef int (*pse_pi_enable_t)(const struct device *dev, uint8_t pi_idx);

/**
 * @brief Disable power sourcing on a PI.
 *
 * See pse_pi_disable() for argument description.
 */
typedef int (*pse_pi_disable_t)(const struct device *dev, uint8_t pi_idx);

/**
 * @brief Read the power detection status of a PI.
 *
 * See pse_pi_get_pw_status() for argument description.
 */
typedef int (*pse_pi_get_pw_status_t)(const struct device *dev, uint8_t pi_idx, uint8_t *status);

/**
 * @brief Read the extended state of a @ref PSE_TYPE_POE PI.
 *
 * See pse_pi_get_ext_state() for argument description.
 */
typedef int (*pse_pi_get_ext_state_t)(const struct device *dev, uint8_t pi_idx,
				      struct pse_ext_state_info *ext_state);

/**
 * @brief Read the negotiated power class of the PD attached to a PI.
 *
 * See pse_pi_get_pw_class() for argument description.
 */
typedef int (*pse_pi_get_pw_class_t)(const struct device *dev, uint8_t pi_idx, uint8_t *pd_class);

/**
 * @brief Read the power currently being delivered by a PI.
 *
 * See pse_pi_get_actual_pw() for argument description.
 */
typedef int (*pse_pi_get_actual_pw_t)(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw);

/**
 * @brief Read the output voltage of a PI.
 *
 * See pse_pi_get_voltage() for argument description.
 */
typedef int (*pse_pi_get_voltage_t)(const struct device *dev, uint8_t pi_idx, int32_t *voltage_uv);

/**
 * @brief Read the configured power limit of a PI.
 *
 * See pse_pi_get_pw_limit() for argument description.
 */
typedef int (*pse_pi_get_pw_limit_t)(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw);

/**
 * @brief Set the power limit of a PI.
 *
 * See pse_pi_set_pw_limit() for argument description.
 */
typedef int (*pse_pi_set_pw_limit_t)(const struct device *dev, uint8_t pi_idx, uint32_t power_uw);

/**
 * @brief Read the number of supported power-limit configuration ranges of a PI.
 *
 * See pse_pi_get_pw_limit_range_count() for argument description.
 */
typedef int (*pse_pi_get_pw_limit_range_count_t)(const struct device *dev, uint8_t pi_idx,
						 uint8_t *count);

/**
 * @brief Read one supported power-limit configuration range of a PI.
 *
 * See pse_pi_get_pw_limit_range() for argument description.
 */
typedef int (*pse_pi_get_pw_limit_range_t)(const struct device *dev, uint8_t pi_idx, uint8_t idx,
					   struct pse_pw_limit_range *range);

/**
 * @brief Read the power-budget priority of a PI.
 *
 * See pse_pi_get_prio() for argument description.
 */
typedef int (*pse_pi_get_prio_t)(const struct device *dev, uint8_t pi_idx, uint8_t *prio);

/**
 * @brief Set the power-budget priority of a PI.
 *
 * See pse_pi_set_prio() for argument description.
 */
typedef int (*pse_pi_set_prio_t)(const struct device *dev, uint8_t pi_idx, uint8_t prio);

/**
 * @brief Read the power requested by a PD before its PI is enabled.
 *
 * See pse_pi_get_pw_req() for argument description.
 */
typedef int (*pse_pi_get_pw_req_t)(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw);

/**
 * @driver_ops{PSE}
 */
__subsystem struct pse_driver_api {
	/** @driver_ops_mandatory @copybrief pse_pi_enable */
	pse_pi_enable_t pi_enable;
	/** @driver_ops_mandatory @copybrief pse_pi_disable */
	pse_pi_disable_t pi_disable;
	/** @driver_ops_mandatory @copybrief pse_pi_get_pw_status */
	pse_pi_get_pw_status_t pi_get_pw_status;
	/** @driver_ops_optional @copybrief pse_pi_get_ext_state */
	pse_pi_get_ext_state_t pi_get_ext_state;
	/** @driver_ops_optional @copybrief pse_pi_get_pw_class */
	pse_pi_get_pw_class_t pi_get_pw_class;
	/** @driver_ops_optional @copybrief pse_pi_get_actual_pw */
	pse_pi_get_actual_pw_t pi_get_actual_pw;
	/** @driver_ops_optional @copybrief pse_pi_get_voltage */
	pse_pi_get_voltage_t pi_get_voltage;
	/** @driver_ops_optional @copybrief pse_pi_get_pw_limit */
	pse_pi_get_pw_limit_t pi_get_pw_limit;
	/** @driver_ops_optional @copybrief pse_pi_set_pw_limit */
	pse_pi_set_pw_limit_t pi_set_pw_limit;
	/** @driver_ops_optional @copybrief pse_pi_get_pw_limit_range_count */
	pse_pi_get_pw_limit_range_count_t pi_get_pw_limit_range_count;
	/** @driver_ops_optional @copybrief pse_pi_get_pw_limit_range */
	pse_pi_get_pw_limit_range_t pi_get_pw_limit_range;
	/** @driver_ops_optional @copybrief pse_pi_get_prio */
	pse_pi_get_prio_t pi_get_prio;
	/** @driver_ops_optional @copybrief pse_pi_set_prio */
	pse_pi_set_prio_t pi_set_prio;
	/** @driver_ops_optional @copybrief pse_pi_get_pw_req */
	pse_pi_get_pw_req_t pi_get_pw_req;
};

/**
 * @brief Enable power sourcing on a PI.
 *
 * Runs detection and, if the controller supports it, classification, then
 * applies full operating voltage to the PI once a valid PD is confirmed
 * present.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p pi_idx does not correspond to a PI on this controller.
 */
static inline int pse_pi_enable(const struct device *dev, uint8_t pi_idx)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	return api->pi_enable(dev, pi_idx);
}

/**
 * @brief Disable power sourcing on a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p pi_idx does not correspond to a PI on this controller.
 */
static inline int pse_pi_disable(const struct device *dev, uint8_t pi_idx)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	return api->pi_disable(dev, pi_idx);
}

/**
 * @brief Read the power detection status of a PI.
 *
 * The returned code is interpreted via @ref pse_podl_pw_status or
 * @ref pse_poe_pw_status depending on which @ref pse_type the controller
 * implements; the two value sets are not interchangeable.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] status Where the power detection status code will be stored.
 * Must not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int pse_pi_get_pw_status(const struct device *dev, uint8_t pi_idx, uint8_t *status)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	return api->pi_get_pw_status(dev, pi_idx, status);
}

/**
 * @brief Read the extended state of a @ref PSE_TYPE_POE PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] ext_state Where the extended state will be stored. Must not be
 * `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_ext_state(const struct device *dev, uint8_t pi_idx,
				       struct pse_ext_state_info *ext_state)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_ext_state == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_ext_state(dev, pi_idx, ext_state);
}

/**
 * @brief Read the negotiated power class of the PD attached to a PI.
 *
 * The returned value is an opaque class number as negotiated by whatever
 * classification mechanism the underlying protocol defines (Physical Layer
 * or Data Link Layer classification for @ref PSE_TYPE_POE, or SCCP for
 * @ref PSE_TYPE_PODL); this API does not interpret it further.
 *
 * @note On controllers where classification is driven by the host over a
 * bit-banged or otherwise slow serial link (for example SCCP on
 * @ref PSE_TYPE_PODL hardware), this call may block for several tens of
 * milliseconds. This is not universal: controllers with autonomous or
 * chip-firmware-driven classification return immediately.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] pd_class Where the negotiated class will be stored. Must not be
 * `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_pw_class(const struct device *dev, uint8_t pi_idx, uint8_t *pd_class)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_pw_class == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_pw_class(dev, pi_idx, pd_class);
}

/**
 * @brief Read the power currently being delivered by a PI.
 *
 * IEEE 802.3-2022 30.9.1.1.23 aPSEActualPower.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] power_uw Where the delivered power will be stored, in
 * microwatts. Must not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_actual_pw(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_actual_pw == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_actual_pw(dev, pi_idx, power_uw);
}

/**
 * @brief Read the output voltage of a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] voltage_uv Where the measured output voltage will be stored,
 * in microvolts. Must not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_voltage(const struct device *dev, uint8_t pi_idx, int32_t *voltage_uv)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_voltage == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_voltage(dev, pi_idx, voltage_uv);
}

/**
 * @brief Read the configured power limit of a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] power_uw Where the configured power limit will be stored, in
 * microwatts. Must not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_pw_limit(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_pw_limit == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_pw_limit(dev, pi_idx, power_uw);
}

/**
 * @brief Set the power limit of a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param power_uw Requested power limit, in microwatts.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p power_uw is outside the range the controller can apply.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_set_pw_limit(const struct device *dev, uint8_t pi_idx, uint32_t power_uw)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_set_pw_limit == NULL) {
		return -ENOSYS;
	}

	return api->pi_set_pw_limit(dev, pi_idx, power_uw);
}

/**
 * @brief Read the number of supported power-limit configuration ranges of a PI.
 *
 * Each range gets an index, starting from zero; use
 * pse_pi_get_pw_limit_range() to read one.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] count Where the number of supported ranges will be stored. Must
 * not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_pw_limit_range_count(const struct device *dev, uint8_t pi_idx,
						  uint8_t *count)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_pw_limit_range_count == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_pw_limit_range_count(dev, pi_idx, count);
}

/**
 * @brief Read one supported power-limit configuration range of a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param idx Range index; see pse_pi_get_pw_limit_range_count().
 * @param[out] range Where the range at @p idx will be stored. Must not be
 * `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p idx does not correspond to a supported range.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_pw_limit_range(const struct device *dev, uint8_t pi_idx, uint8_t idx,
					    struct pse_pw_limit_range *range)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_pw_limit_range == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_pw_limit_range(dev, pi_idx, idx, range);
}

/**
 * @brief Read the power-budget priority of a PI.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] prio Where the priority level will be stored. Lower numeric
 * values indicate higher priority; range is controller-specific. Must not be
 * `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_prio(const struct device *dev, uint8_t pi_idx, uint8_t *prio)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_prio == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_prio(dev, pi_idx, prio);
}

/**
 * @brief Set the power-budget priority of a PI.
 *
 * On controllers that manage a shared power budget across multiple PIs,
 * this hints which PIs should be reduced or shut down first when the
 * budget is exceeded. Lower numeric values indicate higher priority.
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param prio Requested priority level; range is controller-specific.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p prio is outside the range the controller supports.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_set_prio(const struct device *dev, uint8_t pi_idx, uint8_t prio)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_set_prio == NULL) {
		return -ENOSYS;
	}

	return api->pi_set_prio(dev, pi_idx, prio);
}

/**
 * @brief Read the power requested by a PD before its PI is enabled.
 *
 * Only meaningful once a PD has been detected but before the PI has been
 * enabled; relevant to budget-management logic reacting to a detection
 * interrupt before deciding whether to call pse_pi_enable().
 *
 * @param dev PSE controller device instance.
 * @param pi_idx Index of the PI, as given by its devicetree `reg`.
 * @param[out] power_uw Where the requested power will be stored, in
 * microwatts. Must not be `NULL`.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -ENOSYS Function is not implemented.
 */
static inline int pse_pi_get_pw_req(const struct device *dev, uint8_t pi_idx, uint32_t *power_uw)
{
	const struct pse_driver_api *api = DEVICE_API_GET(pse, dev);

	if (api->pi_get_pw_req == NULL) {
		return -ENOSYS;
	}

	return api->pi_get_pw_req(dev, pi_idx, power_uw);
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PSE_H_ */

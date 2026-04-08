/*
 * Copyright 2022 The Chromium OS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USBC Type-C Port Controller device APIs
 *
 * This file contains the USB Type-C Port Controller device APIs.
 * All Type-C Port Controller device drivers should implement the
 * APIs described in this file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TCPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TCPC_H_

/**
 * @brief USB Type-C Port Controller API
 * @defgroup usb_type_c_port_controller_api USB Type-C Port Controller API
 * @since 3.1
 * @version 0.1.1
 * @ingroup usb_type_c
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <errno.h>

#include "usbc_tc.h"
#include "usbc_pd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TCPC Alert bits
 */
enum tcpc_alert {
	/** CC status changed */
	TCPC_ALERT_CC_STATUS,
	/** Power status changed */
	TCPC_ALERT_POWER_STATUS,
	/** Receive Buffer register changed */
	TCPC_ALERT_MSG_STATUS,
	/** Received Hard Reset message */
	TCPC_ALERT_HARD_RESET_RECEIVED,
	/** SOP* message transmission not successful */
	TCPC_ALERT_TRANSMIT_MSG_FAILED,
	/**
	 * Reset or SOP* message transmission not sent
	 * due to an incoming receive message
	 */
	TCPC_ALERT_TRANSMIT_MSG_DISCARDED,
	/** Reset or SOP* message transmission successful */
	TCPC_ALERT_TRANSMIT_MSG_SUCCESS,
	/** A high-voltage alarm has occurred */
	TCPC_ALERT_VBUS_ALARM_HI,
	/** A low-voltage alarm has occurred */
	TCPC_ALERT_VBUS_ALARM_LO,
	/** A fault has occurred. Read the FAULT_STATUS register */
	TCPC_ALERT_FAULT_STATUS,
	/** TCPC RX buffer has overflowed */
	TCPC_ALERT_RX_BUFFER_OVERFLOW,
	/** The TCPC in Attached.SNK state has detected a sink disconnect */
	TCPC_ALERT_VBUS_SNK_DISCONNECT,
	/** Receive buffer register changed */
	TCPC_ALERT_BEGINNING_MSG_STATUS,
	/** Extended status changed */
	TCPC_ALERT_EXTENDED_STATUS,
	/**
	 * An extended interrupt event has occurred. Read the alert_extended
	 * register
	 */
	TCPC_ALERT_EXTENDED,
	/** A vendor defined alert has been detected */
	TCPC_ALERT_VENDOR_DEFINED
};

/**
 * @brief TCPC Status register
 */
enum tcpc_status_reg {
	/** The Alert register */
	TCPC_ALERT_STATUS,
	/** The CC Status register */
	TCPC_CC_STATUS,
	/** The Power Status register */
	TCPC_POWER_STATUS,
	/** The Fault Status register */
	TCPC_FAULT_STATUS,
	/** The Extended Status register */
	TCPC_EXTENDED_STATUS,
	/** The Extended Alert Status register */
	TCPC_EXTENDED_ALERT_STATUS,
	/** The Vendor Defined Status register */
	TCPC_VENDOR_DEFINED_STATUS
};

/**
 * @brief TCPC Chip Information
 */
struct tcpc_chip_info {
	/** Vendor Id */
	uint16_t vendor_id;
	/** Product Id */
	uint16_t product_id;
	/** Device Id */
	uint16_t device_id;
	/** Firmware version number */
	uint64_t fw_version_number;

	union {
		/** Minimum Required firmware version string */
		uint8_t min_req_fw_version_string[8];
		/** Minimum Required firmware version number */
		uint64_t min_req_fw_version_number;
	};
};

/**
 * @brief Callback type for VCONN control
 *
 * @param tcpc_dev  Runtime tcpc device structure
 * @param usbc_dev  Runtime usbc device structure
 * @param pol       CC polarity indicating which CC line carries VCONN
 * @param enable    VCONN is enabled when true, disabled when false
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
typedef int (*tcpc_vconn_control_cb_t)(const struct device *tcpc_dev, const struct device *usbc_dev,
				       enum tc_cc_polarity pol, bool enable);

/**
 * @brief Callback type for discharging VCONN
 *
 * @param tcpc_dev  Runtime tcpc device structure
 * @param usbc_dev  Runtime usbc device structure
 * @param pol       CC polarity indicating which CC line carries VCONN
 * @param enable    VCONN discharge is enabled when true, disabled when false
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
typedef int (*tcpc_vconn_discharge_cb_t)(const struct device *tcpc_dev,
					 const struct device *usbc_dev, enum tc_cc_polarity pol,
					 bool enable);

/**
 * @brief Callback type for handling TCPC alert events
 *
 * @param dev    Runtime device structure
 * @param data   User data passed when registering the callback via tcpc_set_alert_handler_cb
 * @param alert  The alert type that was triggered
 */
typedef void (*tcpc_alert_handler_cb_t)(const struct device *dev, void *data,
					enum tcpc_alert alert);

/**
 * @def_driverbackendgroup{USB Type-C Port Controller,usb_type_c_port_controller_api}
 * @ingroup usb_type_c_port_controller_api
 * @{
 */

/**
 * @brief Callback API to initialize the TCPC.
 *
 * See tcpc_init() for argument description.
 */
typedef int (*tcpc_api_init_t)(const struct device *dev);

/**
 * @brief Callback API to read CC line status.
 *
 * See tcpc_get_cc() for argument description.
 */
typedef int (*tcpc_api_get_cc_t)(const struct device *dev, enum tc_cc_voltage_state *cc1,
				 enum tc_cc_voltage_state *cc2);

/**
 * @brief Callback API to set source Rp value.
 *
 * See tcpc_select_rp_value() for argument description.
 */
typedef int (*tcpc_api_select_rp_value_t)(const struct device *dev, enum tc_rp_value rp);

/**
 * @brief Callback API to get source Rp value.
 *
 * See tcpc_get_rp_value() for argument description.
 */
typedef int (*tcpc_api_get_rp_value_t)(const struct device *dev, enum tc_rp_value *rp);

/**
 * @brief Callback API to set CC pull and role.
 *
 * See tcpc_set_cc() for argument description.
 */
typedef int (*tcpc_api_set_cc_t)(const struct device *dev, enum tc_cc_pull pull);

/**
 * @brief Callback API to register the VCONN discharge application callback.
 *
 * See tcpc_set_vconn_discharge_cb() for argument description.
 */
typedef void (*tcpc_api_set_vconn_discharge_cb_t)(const struct device *dev,
						  tcpc_vconn_discharge_cb_t cb,
						  const struct device *usbc_dev);

/**
 * @brief Callback API to register the VCONN control application callback.
 *
 * See tcpc_set_vconn_cb() for argument description.
 */
typedef void (*tcpc_api_set_vconn_cb_t)(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb,
					const struct device *usbc_dev);

/**
 * @brief Callback API to enable or disable VCONN discharge.
 *
 * See tcpc_vconn_discharge() for argument description.
 */
typedef int (*tcpc_api_vconn_discharge_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to enable or disable VCONN.
 *
 * See tcpc_set_vconn() for argument description.
 */
typedef int (*tcpc_api_set_vconn_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to set power and data roles for PD message header.
 *
 * See tcpc_set_roles() for argument description.
 */
typedef int (*tcpc_api_set_roles_t)(const struct device *dev, enum tc_power_role power_role,
				    enum tc_data_role data_role);

/**
 * @brief Callback API to get a pending RX message or query RX status.
 *
 * See tcpc_get_rx_pending_msg() for argument description.
 */
typedef int (*tcpc_api_get_rx_pending_msg_t)(const struct device *dev, struct pd_msg *msg);

/**
 * @brief Callback API to enable or disable SOP* RX.
 *
 * See tcpc_set_rx_enable() for argument description.
 */
typedef int (*tcpc_api_set_rx_enable_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to set CC polarity.
 *
 * See tcpc_set_cc_polarity() for argument description.
 */
typedef int (*tcpc_api_set_cc_polarity_t)(const struct device *dev, enum tc_cc_polarity polarity);

/**
 * @brief Callback API to transmit a PD message.
 *
 * See tcpc_transmit_data() for argument description.
 */
typedef int (*tcpc_api_transmit_data_t)(const struct device *dev, struct pd_msg *msg);

/**
 * @brief Callback API to dump standard TCPC registers.
 *
 * See tcpc_dump_std_reg() for argument description.
 */
typedef int (*tcpc_api_dump_std_reg_t)(const struct device *dev);

/**
 * @brief Callback API to get a status register.
 *
 * See tcpc_get_status_register() for argument description.
 */
typedef int (*tcpc_api_get_status_register_t)(const struct device *dev, enum tcpc_status_reg reg,
					      uint32_t *status);

/**
 * @brief Callback API to clear bits in a status register.
 *
 * See tcpc_clear_status_register() for argument description.
 */
typedef int (*tcpc_api_clear_status_register_t)(const struct device *dev, enum tcpc_status_reg reg,
						uint32_t mask);

/**
 * @brief Callback API to mask or unmask status register bits.
 *
 * See tcpc_mask_status_register() for argument description.
 */
typedef int (*tcpc_api_mask_status_register_t)(const struct device *dev, enum tcpc_status_reg reg,
					       uint32_t mask);

/**
 * @brief Callback API to enable or disable debug accessory control.
 *
 * See tcpc_set_debug_accessory() for argument description.
 */
typedef int (*tcpc_api_set_debug_accessory_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to detach from a debug connection.
 *
 * See tcpc_set_debug_detach() for argument description.
 */
typedef int (*tcpc_api_set_debug_detach_t)(const struct device *dev);

/**
 * @brief Callback API to enable or disable DRP toggle.
 *
 * See tcpc_set_drp_toggle() for argument description.
 */
typedef int (*tcpc_api_set_drp_toggle_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to query VBUS sink control state.
 *
 * See tcpc_get_snk_ctrl() for argument description.
 */
typedef int (*tcpc_api_get_snk_ctrl_t)(const struct device *dev);

/**
 * @brief Callback API to enable or disable VBUS sinking.
 *
 * See tcpc_set_snk_ctrl() for argument description.
 */
typedef int (*tcpc_api_set_snk_ctrl_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to query VBUS source control state.
 *
 * See tcpc_get_src_ctrl() for argument description.
 */
typedef int (*tcpc_api_get_src_ctrl_t)(const struct device *dev);

/**
 * @brief Callback API to enable or disable VBUS sourcing.
 *
 * See tcpc_set_src_ctrl() for argument description.
 */
typedef int (*tcpc_api_set_src_ctrl_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to read TCPC chip information.
 *
 * See tcpc_get_chip_info() for argument description.
 */
typedef int (*tcpc_api_get_chip_info_t)(const struct device *dev, struct tcpc_chip_info *chip_info);

/**
 * @brief Callback API to enter or exit low power mode.
 *
 * See tcpc_set_low_power_mode() for argument description.
 */
typedef int (*tcpc_api_set_low_power_mode_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to enable or disable SOP' / SOP'' reception.
 *
 * See tcpc_sop_prime_enable() for argument description.
 */
typedef int (*tcpc_api_sop_prime_enable_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to enable or disable BIST test mode.
 *
 * See tcpc_set_bist_test_mode() for argument description.
 */
typedef int (*tcpc_api_set_bist_test_mode_t)(const struct device *dev, bool enable);

/**
 * @brief Callback API to register the alert application callback.
 *
 * See tcpc_set_alert_handler_cb() for argument description.
 */
typedef int (*tcpc_api_set_alert_handler_cb_t)(const struct device *dev,
					       tcpc_alert_handler_cb_t handler, void *data);

/**
 * @driver_ops{USB Type-C Port Controller}
 */
__subsystem struct tcpc_driver_api {
	/** @driver_ops_mandatory @copybrief tcpc_init */
	tcpc_api_init_t init;
	/** @driver_ops_optional @copybrief tcpc_get_cc */
	tcpc_api_get_cc_t get_cc;
	/** @driver_ops_optional @copybrief tcpc_select_rp_value */
	tcpc_api_select_rp_value_t select_rp_value;
	/** @driver_ops_optional @copybrief tcpc_get_rp_value */
	tcpc_api_get_rp_value_t get_rp_value;
	/** @driver_ops_mandatory @copybrief tcpc_set_cc */
	tcpc_api_set_cc_t set_cc;
	/** @driver_ops_mandatory @copybrief tcpc_set_vconn_discharge_cb */
	tcpc_api_set_vconn_discharge_cb_t set_vconn_discharge_cb;
	/** @driver_ops_mandatory @copybrief tcpc_set_vconn_cb */
	tcpc_api_set_vconn_cb_t set_vconn_cb;
	/** @driver_ops_optional @copybrief tcpc_vconn_discharge */
	tcpc_api_vconn_discharge_t vconn_discharge;
	/** @driver_ops_optional @copybrief tcpc_set_vconn */
	tcpc_api_set_vconn_t set_vconn;
	/** @driver_ops_optional @copybrief tcpc_set_roles */
	tcpc_api_set_roles_t set_roles;
	/** @driver_ops_mandatory @copybrief tcpc_get_rx_pending_msg */
	tcpc_api_get_rx_pending_msg_t get_rx_pending_msg;
	/** @driver_ops_optional @copybrief tcpc_set_rx_enable */
	tcpc_api_set_rx_enable_t set_rx_enable;
	/** @driver_ops_mandatory @copybrief tcpc_set_cc_polarity */
	tcpc_api_set_cc_polarity_t set_cc_polarity;
	/** @driver_ops_optional @copybrief tcpc_transmit_data */
	tcpc_api_transmit_data_t transmit_data;
	/** @driver_ops_optional @copybrief tcpc_dump_std_reg */
	tcpc_api_dump_std_reg_t dump_std_reg;
	/** @driver_ops_optional @copybrief tcpc_get_status_register */
	tcpc_api_get_status_register_t get_status_register;
	/** @driver_ops_optional @copybrief tcpc_clear_status_register */
	tcpc_api_clear_status_register_t clear_status_register;
	/** @driver_ops_optional @copybrief tcpc_mask_status_register */
	tcpc_api_mask_status_register_t mask_status_register;
	/** @driver_ops_optional @copybrief tcpc_set_debug_accessory */
	tcpc_api_set_debug_accessory_t set_debug_accessory;
	/** @driver_ops_optional @copybrief tcpc_set_debug_detach */
	tcpc_api_set_debug_detach_t set_debug_detach;
	/** @driver_ops_optional @copybrief tcpc_set_drp_toggle */
	tcpc_api_set_drp_toggle_t set_drp_toggle;
	/** @driver_ops_optional @copybrief tcpc_get_snk_ctrl */
	tcpc_api_get_snk_ctrl_t get_snk_ctrl;
	/** @driver_ops_optional @copybrief tcpc_set_snk_ctrl */
	tcpc_api_set_snk_ctrl_t set_snk_ctrl;
	/** @driver_ops_optional @copybrief tcpc_get_src_ctrl */
	tcpc_api_get_src_ctrl_t get_src_ctrl;
	/** @driver_ops_optional @copybrief tcpc_set_src_ctrl */
	tcpc_api_set_src_ctrl_t set_src_ctrl;
	/** @driver_ops_optional @copybrief tcpc_get_chip_info */
	tcpc_api_get_chip_info_t get_chip_info;
	/** @driver_ops_optional @copybrief tcpc_set_low_power_mode */
	tcpc_api_set_low_power_mode_t set_low_power_mode;
	/** @driver_ops_optional @copybrief tcpc_sop_prime_enable */
	tcpc_api_sop_prime_enable_t sop_prime_enable;
	/** @driver_ops_optional @copybrief tcpc_set_bist_test_mode */
	tcpc_api_set_bist_test_mode_t set_bist_test_mode;
	/** @driver_ops_mandatory @copybrief tcpc_set_alert_handler_cb */
	tcpc_api_set_alert_handler_cb_t set_alert_handler_cb;
};

/** @} */

/**
 * @brief Returns whether the sink has detected a Rp resistor on the other side
 */
static inline int tcpc_is_cc_rp(enum tc_cc_voltage_state cc)
{
	return (cc == TC_CC_VOLT_RP_DEF) || (cc == TC_CC_VOLT_RP_1A5) || (cc == TC_CC_VOLT_RP_3A0);
}

/**
 * @brief Returns true if both CC lines are completely open
 */
static inline int tcpc_is_cc_open(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	return (cc1 < TC_CC_VOLT_RD) && (cc2 < TC_CC_VOLT_RD);
}

/**
 * @brief Returns true if we detect the port partner is a snk debug accessory
 */
static inline int tcpc_is_cc_snk_dbg_acc(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	return cc1 == TC_CC_VOLT_RD && cc2 == TC_CC_VOLT_RD;
}

/**
 * @brief Returns true if we detect the port partner is a src debug accessory
 */
static inline int tcpc_is_cc_src_dbg_acc(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	return tcpc_is_cc_rp(cc1) && tcpc_is_cc_rp(cc2);
}

/**
 * @brief Returns true if the port partner is an audio accessory
 */
static inline int tcpc_is_cc_audio_acc(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	return cc1 == TC_CC_VOLT_RA && cc2 == TC_CC_VOLT_RA;
}

/**
 * @brief Returns true if the port partner is presenting at least one Rd
 */
static inline int tcpc_is_cc_at_least_one_rd(enum tc_cc_voltage_state cc1,
					     enum tc_cc_voltage_state cc2)
{
	return cc1 == TC_CC_VOLT_RD || cc2 == TC_CC_VOLT_RD;
}

/**
 * @brief Returns true if the port partner is presenting Rd on only one CC line
 */
static inline int tcpc_is_cc_only_one_rd(enum tc_cc_voltage_state cc1, enum tc_cc_voltage_state cc2)
{
	return tcpc_is_cc_at_least_one_rd(cc1, cc2) && cc1 != cc2;
}

/**
 * @brief Initializes the TCPC
 *
 * @param dev Runtime device structure
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -EAGAIN if initialization should be postponed
 */
static inline int tcpc_init(const struct device *dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->init != NULL, "Callback pointer should not be NULL");

	return api->init(dev);
}

/**
 * @brief Reads the status of the CC lines
 *
 * @param dev  Runtime device structure
 * @param cc1  A pointer where the CC1 status is written
 * @param cc2  A pointer where the CC2 status is written
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_get_cc(const struct device *dev, enum tc_cc_voltage_state *cc1,
			      enum tc_cc_voltage_state *cc2)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_cc == NULL) {
		return -ENOSYS;
	}

	return api->get_cc(dev, cc1, cc2);
}

/**
 * @brief Sets the value of CC pull up resistor used when operating as a Source
 *
 * @param dev   Runtime device structure
 * @param rp    Value of the Pull-Up Resistor.
 *
 * @retval 0 on success
 * @retval -ENOSYS if not implemented
 * @retval -EIO on failure
 */
static inline int tcpc_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->select_rp_value == NULL) {
		return -ENOSYS;
	}

	return api->select_rp_value(dev, rp);
}

/**
 * @brief Gets the value of the CC pull up resistor used when operating as a Source
 *
 * @param dev   Runtime device structure
 * @param rp    pointer where the value of the Pull-Up Resistor is stored
 *
 * @retval 0 on success
 * @retval -ENOSYS if not implemented
 * @retval -EIO on failure
 */
static inline int tcpc_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_rp_value == NULL) {
		return -ENOSYS;
	}

	return api->get_rp_value(dev, rp);
}

/**
 * @brief Sets the CC pull resistor and sets the role as either Source or Sink
 *
 * @param dev   Runtime device structure
 * @param pull  The pull resistor to set
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static inline int tcpc_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_cc != NULL, "Callback pointer should not be NULL");

	return api->set_cc(dev, pull);
}

/**
 * @brief Sets a callback that can enable or disable VCONN if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the VCONN control capabilities of the TCPC
 *
 * The callback is called in the tcpc_set_vconn function if vconn_cb isn't NULL
 *
 * @param dev       Runtime device structure
 * @param vconn_cb  pointer to the callback function that controls vconn
 * @param usbc_dev  USB-C connector device
 */
static inline void tcpc_set_vconn_cb(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb,
				     const struct device *usbc_dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_vconn_cb != NULL, "Callback pointer should not be NULL");

	api->set_vconn_cb(dev, vconn_cb, usbc_dev);
}

/**
 * @brief Sets a callback that can enable or discharge VCONN if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the VCONN control capabilities of the TCPC
 *
 * The callback is called in the tcpc_vconn_discharge function if cb isn't NULL
 *
 * @param dev       Runtime device structure
 * @param cb        pointer to the callback function that discharges vconn
 * @param usbc_dev  USB-C connector device
 */
static inline void tcpc_set_vconn_discharge_cb(const struct device *dev,
					       tcpc_vconn_discharge_cb_t cb,
					       const struct device *usbc_dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_vconn_discharge_cb != NULL, "Callback pointer should not be NULL");

	api->set_vconn_discharge_cb(dev, cb, usbc_dev);
}

/**
 * @brief Discharges VCONN
 *
 * This function uses the TCPC to discharge VCONN if possible or calls the
 * callback function set by tcpc_set_vconn_cb
 *
 * @param dev     Runtime device structure
 * @param enable  VCONN discharge is enabled when true, it's disabled
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_vconn_discharge(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->vconn_discharge == NULL) {
		return -ENOSYS;
	}

	return api->vconn_discharge(dev, enable);
}

/**
 * @brief Enables or disables VCONN
 *
 * This function uses the TCPC to measure VCONN if possible or calls the
 * callback function set by tcpc_set_vconn_cb
 *
 * @param dev     Runtime device structure
 * @param enable  VCONN is enabled when true, it's disabled
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_vconn(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_vconn == NULL) {
		return -ENOSYS;
	}

	return api->set_vconn(dev, enable);
}

/**
 * @brief Sets the Power and Data Role of the PD message header
 *
 * This function only needs to be called once per data / power role change
 *
 * @param dev         Runtime device structure
 * @param power_role  current power role
 * @param data_role   current data role
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_roles(const struct device *dev, enum tc_power_role power_role,
				 enum tc_data_role data_role)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_roles == NULL) {
		return -ENOSYS;
	}

	return api->set_roles(dev, power_role, data_role);
}

/**
 * @brief Retrieves the Power Delivery message from the TCPC.
 * If buf is NULL, then only the status is returned, where 0 means there is a message pending and
 * -ENODATA means there is no pending message.
 *
 * @param dev Runtime device structure
 * @param buf pointer where the pd_buf pointer is written, NULL if only checking the status
 *
 * @retval Greater or equal to 0 is the number of bytes received if buf parameter is provided
 * @retval 0 if there is a message pending and buf parameter is NULL
 * @retval -EIO on failure
 * @retval -ENODATA if no message is pending
 */
static inline int tcpc_get_rx_pending_msg(const struct device *dev, struct pd_msg *buf)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->get_rx_pending_msg != NULL, "Callback pointer should not be NULL");

	return api->get_rx_pending_msg(dev, buf);
}

/**
 * @brief Enables the reception of SOP* message types
 *
 * @param dev     Runtime device structure
 * @param enable  Enable Power Delivery when true, else it's
 *		  disabled
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_rx_enable(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_rx_enable == NULL) {
		return -ENOSYS;
	}

	return api->set_rx_enable(dev, enable);
}

/**
 * @brief Sets the polarity of the CC lines
 *
 * @param dev       Runtime device structure
 * @param polarity  Polarity of the cc line
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static inline int tcpc_set_cc_polarity(const struct device *dev, enum tc_cc_polarity polarity)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_cc_polarity != NULL, "Callback pointer should not be NULL");

	return api->set_cc_polarity(dev, polarity);
}

/**
 * @brief Transmits a Power Delivery message
 *
 * @param dev     Runtime device structure
 * @param msg     Power Delivery message to transmit
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_transmit_data(const struct device *dev, struct pd_msg *msg)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->transmit_data == NULL) {
		return -ENOSYS;
	}

	return api->transmit_data(dev, msg);
}

/**
 * @brief Dump a set of TCPC registers
 *
 * @param dev Runtime device structure
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_dump_std_reg(const struct device *dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->dump_std_reg == NULL) {
		return -ENOSYS;
	}

	return api->dump_std_reg(dev);
}

/**
 * @brief Sets the alert function that's called when an interrupt is triggered
 *	  due to an alert bit
 *
 * Calling this function enables the particular alert bit
 *
 * @param dev      Runtime device structure
 * @param handler  The callback function called when the bit is set
 * @param data     user data passed to the callback
 *
 * @retval 0 on success
 * @retval -EINVAL on failure
 */
static inline int tcpc_set_alert_handler_cb(const struct device *dev,
					    tcpc_alert_handler_cb_t handler, void *data)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_alert_handler_cb != NULL, "Callback pointer should not be NULL");

	return api->set_alert_handler_cb(dev, handler, data);
}

/**
 * @brief Gets a status register
 *
 * @param dev     Runtime device structure
 * @param reg     The status register to read
 * @param status  Pointer where the status is stored
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_get_status_register(const struct device *dev, enum tcpc_status_reg reg,
					   uint32_t *status)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_status_register == NULL) {
		return -ENOSYS;
	}

	return api->get_status_register(dev, reg, status);
}

/**
 * @brief Clears a TCPC status register
 *
 * @param dev   Runtime device structure
 * @param reg   The status register to read
 * @param mask  A bit mask of the status register to clear.
 *		A status bit is cleared when it's set to 1.
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_clear_status_register(const struct device *dev, enum tcpc_status_reg reg,
					     uint32_t mask)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->clear_status_register == NULL) {
		return -ENOSYS;
	}

	return api->clear_status_register(dev, reg, mask);
}

/**
 * @brief Sets the mask of a TCPC status register
 *
 * @param dev   Runtime device structure
 * @param reg   The status register to read
 * @param mask  A bit mask of the status register to mask.
 *		The status bit is masked if it's 0, else it's unmasked.
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_mask_status_register(const struct device *dev, enum tcpc_status_reg reg,
					    uint32_t mask)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->mask_status_register == NULL) {
		return -ENOSYS;
	}

	return api->mask_status_register(dev, reg, mask);
}

/**
 * @brief Manual control of TCPC DebugAccessory control
 *
 * @param dev     Runtime device structure
 * @param enable  Enable Debug Accessory when true, else it's disabled
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_debug_accessory(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_debug_accessory == NULL) {
		return -ENOSYS;
	}

	return api->set_debug_accessory(dev, enable);
}

/**
 * @brief Detach from a debug connection
 *
 * @param dev Runtime device structure
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_debug_detach(const struct device *dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_debug_detach == NULL) {
		return -ENOSYS;
	}

	return api->set_debug_detach(dev);
}

/**
 * @brief Enable TCPC auto dual role toggle
 *
 * @param dev     Runtime device structure
 * @param enable  Auto dual role toggle is active when true, else it's disabled
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_drp_toggle(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_drp_toggle == NULL) {
		return -ENOSYS;
	}

	return api->set_drp_toggle(dev, enable);
}

/**
 * @brief Queries the current sinking state of the TCPC
 *
 * @param dev Runtime device structure
 *
 * @retval true if sinking power
 * @retval false if not sinking power
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_get_snk_ctrl(const struct device *dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_snk_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_snk_ctrl(dev);
}

/**
 * @brief Set the VBUS sinking state of the TCPC
 *
 * @param dev Runtime device structure
 * @param enable True if sinking should be enabled, false if disabled
 * @retval 0 on success
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_snk_ctrl(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_snk_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_snk_ctrl(dev, enable);
}

/**
 * @brief Queries the current sourcing state of the TCPC
 *
 * @param dev Runtime device structure
 *
 * @retval true if sourcing power
 * @retval false if not sourcing power
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_get_src_ctrl(const struct device *dev)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_src_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_src_ctrl(dev);
}

/**
 * @brief Set the VBUS sourcing state of the TCPC
 *
 * @param dev Runtime device structure
 * @param enable True if sourcing should be enabled, false if disabled
 * @retval 0 on success
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_src_ctrl(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_src_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_src_ctrl(dev, enable);
}

/**
 * @brief Controls the BIST Mode of the TCPC. It disables RX alerts while the
 *	  mode is active.
 *
 * @param dev Runtime device structure
 * @param enable The TCPC enters BIST TEST Mode when true
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_bist_test_mode(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_bist_test_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_bist_test_mode(dev, enable);
}

/**
 * @brief Gets the TCPC firmware version
 *
 * @param dev        Runtime device structure
 * @param chip_info  Pointer to TCPC chip info where the version is stored
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_get_chip_info(const struct device *dev, struct tcpc_chip_info *chip_info)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->get_chip_info == NULL) {
		return -ENOSYS;
	}

	return api->get_chip_info(dev, chip_info);
}

/**
 * @brief Instructs the TCPC to enter or exit low power mode
 *
 * @param dev     Runtime device structure
 * @param enable  The TCPC enters low power mode when true, else it exits it
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_set_low_power_mode(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->set_low_power_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_low_power_mode(dev, enable);
}

/**
 * @brief Enables the reception of SOP Prime and optionally SOP Double Prime messages
 *
 * @param dev     Runtime device structure
 * @param enable  Can receive SOP Prime messages and SOP Double Prime messages when true,
 *		  else it can not
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOSYS if not implemented
 */
static inline int tcpc_sop_prime_enable(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api = (const struct tcpc_driver_api *)dev->api;

	if (api->sop_prime_enable == NULL) {
		return -ENOSYS;
	}

	return api->sop_prime_enable(dev, enable);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_TCPC_H_ */

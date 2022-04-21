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
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>

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
	uint64_t fw_verion_number;

	union {
		/** Minimum Required firmware version string */
		uint8_t min_req_fw_version_string[8];
		/** Minimum Required firmware version number */
		uint64_t min_req_fw_version_number;
	};
};

typedef	int (*tcpc_vbus_cb_t)(const struct device *dev, int *vbus_meas);
typedef	int (*tcpc_discharge_vbus_cb_t)(const struct device *dev, bool enable);
typedef	int (*tcpc_vconn_control_cb_t)(const struct device *dev, bool enable);
typedef void (*tcpc_alert_handler_cb_t)(const struct device *dev, void *data,
		enum tcpc_alert alert);

__subsystem struct tcpc_driver_api {
	int (*init)(const struct device *dev);
	int (*get_cc)(const struct device *dev, enum tc_cc_voltage_state *cc1,
			enum tc_cc_voltage_state *cc2);
	void (*set_vbus_measure_cb)(const struct device *dev, tcpc_vbus_cb_t vbus_cb);
	void (*set_discharge_vbus_cb)(const struct device *dev,
			tcpc_discharge_vbus_cb_t discharge_vbus_cb);
	bool (*check_vbus_level)(const struct device *dev, enum tc_vbus_level level);
	int (*get_vbus)(const struct device *dev, int *vbus_meas);
	int (*select_rp_value)(const struct device *dev, enum tc_rp_value rp);
	int (*get_rp_value)(const struct device *dev, enum tc_rp_value *rp);
	int (*set_cc)(const struct device *dev, enum tc_cc_pull pull);
	void (*set_vconn_cb)(const struct device *dev, tcpc_vconn_control_cb_t vconn_cb);
	int (*set_vconn)(const struct device *dev, bool enable);
	int (*set_roles)(const struct device *dev, enum tc_power_role power_role,
			enum tc_data_role data_role);
	int (*receive_data)(const struct device *dev, struct pd_msg *msg);
	bool (*is_rx_pending_msg)(const struct device *dev, enum pd_packet_type *type);
	int (*set_rx_enable)(const struct device *dev, bool enable);
	int (*set_cc_polarity)(const struct device *dev, enum tc_cc_polarity polarity);
	int (*transmit_data)(const struct device *dev, struct pd_msg *msg);
	int (*dump_std_reg)(const struct device *dev);
	void (*alert_handler_cb)(const struct device *dev, void *data, enum tcpc_alert alert);
	int (*get_status_register)(const struct device *dev, enum tcpc_status_reg reg,
			int32_t *status);
	int (*clear_status_register)(const struct device *dev, enum tcpc_status_reg reg,
			uint32_t mask);
	int (*mask_status_register)(const struct device *dev, enum tcpc_status_reg reg,
			uint32_t mask);
	int (*set_discharge_vbus)(const struct device *dev, bool enable);
	int (*enable_auto_discharge_disconnect)(const struct device *dev, bool enable);
	int (*set_debug_accessory)(const struct device *dev, bool enable);
	int (*set_debug_detach)(const struct device *dev);
	int (*set_drp_toggle)(const struct device *dev, bool enable);
	bool (*get_snk_ctrl)(const struct device *dev);
	bool (*get_src_ctrl)(const struct device *dev);
	int (*get_chip_info)(const struct device *dev, struct tcpc_chip_info *chip_info);
	int (*set_low_power_mode)(const struct device *dev, bool enable);
	int (*sop_prime_enable)(const struct device *dev, bool enable);
	int (*set_bist_test_mode)(const struct device *dev, bool enable);
	int (*set_alert_handler_cb)(const struct device *dev, tcpc_alert_handler_cb_t handler,
			void *data);
};

/**
 * @brief Returns whether the sink has detected a Rp resistor on the other side
 */
static inline int tcpc_is_cc_rp(enum tc_cc_voltage_state cc)
{
	return (cc == TC_CC_VOLT_RP_DEF) || (cc == TC_CC_VOLT_RP_1A5) ||
	       (cc == TC_CC_VOLT_RP_3A0);
}

/**
 * @brief Returns true if both CC lines are completely open
 */
static inline int tcpc_is_cc_open(enum tc_cc_voltage_state cc1,
				  enum tc_cc_voltage_state cc2)
{
	return cc1 == TC_CC_VOLT_OPEN && cc2 == TC_CC_VOLT_OPEN;
}

/**
 * @brief Returns true if we detect the port partner is a snk debug accessory
 */
static inline int tcpc_is_cc_snk_dbg_acc(enum tc_cc_voltage_state cc1,
					 enum tc_cc_voltage_state cc2)
{
	return cc1 == TC_CC_VOLT_RD && cc2 == TC_CC_VOLT_RD;
}

/**
 * @brief Returns true if we detect the port partner is a src debug accessory
 */
static inline int tcpc_is_cc_src_dbg_acc(enum tc_cc_voltage_state cc1,
					 enum tc_cc_voltage_state cc2)
{
	return tcpc_is_cc_rp(cc1) && tcpc_is_cc_rp(cc2);
}

/**
 * @brief Returns true if the port partner is an audio accessory
 */
static inline int tcpc_is_cc_audio_acc(enum tc_cc_voltage_state cc1,
				       enum tc_cc_voltage_state cc2)
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
static inline int tcpc_is_cc_only_one_rd(enum tc_cc_voltage_state cc1,
					 enum tc_cc_voltage_state cc2)
{
	return tcpc_is_cc_at_least_one_rd(cc1, cc2) && cc1 != cc2;
}

/**
 * @brief Initializes the TCPC
 *
 * @param dev Runtime device structure
 *
 * @return 0 on success
 * @return -EIO on failure
 */
static inline int tcpc_init(const struct device *dev)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->init != NULL,
		 "Callback pointer should not be NULL");

	return api->init(dev);
}

/**
 * @brief Reads the status of the CC lines
 *
 * @param dev  Runtime device structure
 * @param cc1  A pointer where the CC1 status is written
 * @param cc2  A pointer where the CC2 status is written
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_get_cc(const struct device *dev,
			      enum tc_cc_voltage_state *cc1,
			      enum tc_cc_voltage_state *cc2)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->get_cc == NULL) {
		return -ENOSYS;
	}

	return api->get_cc(dev, cc1, cc2);
}

/**
 * @brief Sets a callback that can measure the value of VBUS if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the VBUS measurement and detection capabilities of the TCPC.
 *
 * The callback is called in tcpc_check_vbus_level and tcpc_get_vbus
 * functions if vbus_cb isn't NULL.
 *
 * @param dev      Runtime device structure
 * @param vbus_cb  pointer to callback function that returns a voltage
 *                 measurement
 */
static inline void tcpc_set_vbus_measure_cb(const struct device *dev,
					    tcpc_vbus_cb_t vbus_cb)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_vbus_measure_cb != NULL,
		 "Callback pointer should not be NULL");

	api->set_vbus_measure_cb(dev, vbus_cb);
}

/**
 * @brief Sets a callback that can discharge VBUS if the TCPC is
 *	  unable to or the system is configured in a way that does not use
 *	  the discharge VBUS capabilities of the TCPC.
 *
 * The callback is called in tcpc_set_discharge_vbus functions if
 * discharge_vbus_cb isn't NULL.
 *
 * @param dev                Runtime device structure
 * @param discharge_vbus_cb  pointer to callback function that discharges VBUS
 */
static inline void tcpc_set_discharge_vbus_cb(const struct device *dev,
					      tcpc_discharge_vbus_cb_t discharge_vbus_cb)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_discharge_vbus_cb != NULL,
		 "Callback pointer should not be NULL");

	api->set_discharge_vbus_cb(dev, discharge_vbus_cb);
}

/**
 * @brief Checks if VBUS is at a particular level
 *
 * @param dev    Runtime device structure
 * @param level  The level voltage to check against
 *
 * @return true if VBUS is at the level voltage
 * @return false if VBUS is not at that level voltage
 */
static inline bool tcpc_check_vbus_level(const struct device *dev,
					 enum tc_vbus_level level)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->check_vbus_level != NULL,
		 "Callback pointer should not be NULL");

	return api->check_vbus_level(dev, level);
}

/**
 * @brief Reads and returns VBUS measured in mV
 *
 * This function uses the TCPC to measure VBUS if possible or calls the
 * callback function set by tcpc_set_vbus_measure_callback. In the event that
 * the TCPC can measure VBUS and the VBUS callback measuring function is
 * set, this function uses the callback function.
 *
 * @param dev        Runtime device structure
 * @param vbus_meas  pointer where the measured VBUS voltage is stored
 *
 * @return 0 on success
 * @return -EIO on failure
 */
static inline int tcpc_get_vbus(const struct device *dev, int *vbus_meas)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->get_vbus != NULL,
		 "Callback pointer should not be NULL");

	return api->get_vbus(dev, vbus_meas);
}

/**
 * @brief Sets the value of CC pull up resistor used when operating as a Source
 *
 * @param dev   Runtime device structure
 * @param rp    Value of the Pull-Up Resistor.
 *
 * @return 0 on success
 * @return -ENOSYS
 * @return -EIO on failure
 */
static inline int tcpc_select_rp_value(const struct device *dev, enum tc_rp_value rp)
{
	const struct tcpc_driver_api *api =
	(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -ENOSYS
 * @return -EIO on failure
 */
static inline int tcpc_get_rp_value(const struct device *dev, enum tc_rp_value *rp)
{
	const struct tcpc_driver_api *api =
	(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 */
static inline int tcpc_set_cc(const struct device *dev, enum tc_cc_pull pull)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_cc != NULL,
		 "Callback pointer should not be NULL");

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
 */
static inline void tcpc_set_vconn_cb(const struct device *dev,
				     tcpc_vconn_control_cb_t vconn_cb)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_vconn_cb != NULL,
		 "Callback pointer should not be NULL");

	return api->set_vconn_cb(dev, vconn_cb);
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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_vconn(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_roles(const struct device *dev,
				 enum tc_power_role power_role,
				 enum tc_data_role data_role)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->set_roles == NULL) {
		return -ENOSYS;
	}

	return api->set_roles(dev, power_role, data_role);
}

/**
 * @brief Tests if a received Power Delivery message is pending
 *
 * @param dev  Runtime device structure
 * @param type  pointer to where message type is written. Can be NULL
 *
 * @return true if message is pending, else false
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline bool tcpc_is_rx_pending_msg(const struct device *dev,
					  enum pd_packet_type *type)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->is_rx_pending_msg == NULL) {
		return -ENOSYS;
	}

	return api->is_rx_pending_msg(dev, type);
}

/**
 * @brief Retrieves the Power Delivery message from the TCPC
 *
 * @param dev  Runtime device structure
 * @param buf  pointer where the pd_buf pointer is written
 *
 * @return Greater or equal to 0 is the number of bytes received
 * @return -EIO on failure
 * @return -EFAULT on buf being NULL
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_receive_data(const struct device *dev,
				    struct pd_msg *buf)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->receive_data == NULL) {
		return -ENOSYS;
	}

	return api->receive_data(dev, buf);
}

/**
 * @brief Enables the reception of SOP* message types
 *
 * @param dev     Runtime device structure
 * @param enable  Enable Power Delivery when true, else it's
 *		  disabled
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_rx_enable(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 */
static inline int tcpc_set_cc_polarity(const struct device *dev,
				       enum tc_cc_polarity polarity)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_cc_polarity != NULL,
		 "Callback pointer should not be NULL");

	return api->set_cc_polarity(dev, polarity);
}

/**
 * @brief Transmits a Power Delivery message
 *
 * @param dev     Runtime device structure
 * @param msg     Power Delivery message to transmit
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_transmit_data(const struct device *dev,
				     struct pd_msg *msg)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_dump_std_reg(const struct device *dev)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EINVAL on failure
 */
static inline int tcpc_set_alert_handler_cb(const struct device *dev,
					    tcpc_alert_handler_cb_t handler,
					    void *data)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	__ASSERT(api->set_alert_handler_cb != NULL,
		 "Callback pointer should not be NULL");

	return api->set_alert_handler_cb(dev, handler, data);
}

/**
 * @brief Gets a status register
 *
 * @param dev     Runtime device structure
 * @param reg     The status register to read
 * @param status  Pointer where the status is stored
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_get_status_register(const struct device *dev,
					   enum tcpc_status_reg reg,
					   int32_t *status)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_clear_status_register(const struct device *dev,
					     enum tcpc_status_reg reg,
					     uint32_t mask)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_mask_status_register(const struct device *dev,
					    enum tcpc_status_reg reg,
					    uint32_t mask)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->mask_status_register == NULL) {
		return -ENOSYS;
	}

	return api->mask_status_register(dev, reg, mask);
}

/**
 * @brief Enables discharge TypeC VBUS on Source / Sink disconnect
 *	  and power role swap
 *
 * @param dev     Runtime device structure
 * @param enable  The TypeC VBUS is discharged on disconnect or power
 *                role swap when true
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_discharge_vbus(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->set_discharge_vbus == NULL) {
		return -ENOSYS;
	}

	return api->set_discharge_vbus(dev, enable);
}

/**
 * @brief TCPC automatically discharge TypeC VBUS on Source / Sink disconnect
 *	  an power role swap
 *
 * @param dev     Runtime device structure
 * @param enable  The TCPC automatically discharges VBUS on disconnect or
 *                power role swap
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_enable_auto_discharge_disconnect(
	const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->enable_auto_discharge_disconnect == NULL) {
		return -ENOSYS;
	}

	return api->enable_auto_discharge_disconnect(dev, enable);
}

/**
 * @brief Manual control of TCPC DebugAccessory control
 *
 * @param dev     Runtime device structure
 * @param enable  Enable Debug Accessory when true, else it's disabled
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_debug_accessory(const struct device *dev,
					   bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_debug_detach(const struct device *dev)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_drp_toggle(const struct device *dev, bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return true if sinking power
 * @return false if not sinking power
 * @return -ENOSYS if not implemented
 */
static inline bool tcpc_get_snk_ctrl(const struct device *dev)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->get_snk_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_snk_ctrl(dev);
}

/**
 * @brief Queries the current sourcing state of the TCPC
 *
 * @param dev Runtime device structure
 *
 * @return true if sourcing power
 * @return false if not sourcing power
 * @return -ENOSYS if not implemented
 */
static inline bool tcpc_get_src_ctrl(const struct device *dev)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->get_src_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_src_ctrl(dev);
}

/**
 * @brief Controls the BIST Mode of the TCPC. It disables RX alerts while the
 *	  mode is active.
 *
 * @param dev Runtime device structure
 * @param enable The TCPC enters BIST TEST Mode when true
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_bist_test_mode(const struct device *dev,
					  bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_get_chip_info(const struct device *dev,
				     struct tcpc_chip_info *chip_info)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_set_low_power_mode(const struct device *dev,
					  bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

	if (api->set_low_power_mode == NULL) {
		return -ENOSYS;
	}

	return api->set_low_power_mode(dev, enable);
}

/**
 * @brief Enables the reception of SOP Prime messages
 *
 * @param dev     Runtime device structure
 * @param enable  Can receive SOP Prime messages when true, else it can not
 *
 * @return 0 on success
 * @return -EIO on failure
 * @return -ENOSYS if not implemented
 */
static inline int tcpc_sop_prime_enable(const struct device *dev,
					bool enable)
{
	const struct tcpc_driver_api *api =
		(const struct tcpc_driver_api *)dev->api;

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

/*
 * Copyright 2022 The Chromium OS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB-C Device APIs
 *
 * This file contains the USB-C Device APIs.
 */

#ifndef ZEPHYR_INCLUDE_USBC_H_
#define ZEPHYR_INCLUDE_USBC_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB-C Device APIs
 * @defgroup _usbc_device_api USB-C Device API
 * @since 3.3
 * @version 0.1.0
 * @{
 */

/**
 * @brief This Request Data Object (RDO) value can be returned from the
 *	  policy_cb_get_rdo if 5V@100mA with the following
 *	  options are sufficient for the Sink to operate.
 *
 *	  The RDO is configured as follows:
 *		Maximum operating current 100mA
 *		Operating current 100mA
 *		Unchunked Extended Messages Not Supported
 *		No USB Suspend
 *		Not USB Communications Capable
 *		No capability mismatch
 *		Don't giveback
 *		Object position 1 (5V PDO)
 */
#define FIXED_5V_100MA_RDO 0x1100280a

/**
 * @brief Device Policy Manager requests
 */
enum usbc_policy_request_t {
	/** No request */
	REQUEST_NOP,
	/** Request Type-C layer to transition to Disabled State */
	REQUEST_TC_DISABLED,
	/** Request Type-C layer to transition to Error Recovery State */
	REQUEST_TC_ERROR_RECOVERY,
	/** End of Type-C requests */
	REQUEST_TC_END,

	/** Request Policy Engine layer to perform a Data Role Swap */
	REQUEST_PE_DR_SWAP,
	/** Request Policy Engine layer to send a hard reset */
	REQUEST_PE_HARD_RESET_SEND,
	/** Request Policy Engine layer to send a soft reset */
	REQUEST_PE_SOFT_RESET_SEND,
	/**
	 * Request Policy Engine layer to get Source Capabilities from
	 * port partner
	 */
	REQUEST_PE_GET_SRC_CAPS,
	/**
	 * Request Policy Engine to get Sink Capabilities from
	 * port partner
	 */
	REQUEST_GET_SNK_CAPS,
	/**
	 * Request Policy Engine to request the port partner to source
	 * minimum power
	 */
	REQUEST_PE_GOTO_MIN,
};

/**
 * @brief Device Policy Manager notifications
 */
enum usbc_policy_notify_t {
	/** Power Delivery Accept message was received */
	MSG_ACCEPT_RECEIVED,
	/** Power Delivery Reject message was received */
	MSG_REJECTED_RECEIVED,
	/** Power Delivery discarded the message being transmited */
	MSG_DISCARDED,
	/** Power Delivery Not Supported message was received */
	MSG_NOT_SUPPORTED_RECEIVED,
	/** Data Role has been set to Upstream Facing Port (UFP) */
	DATA_ROLE_IS_UFP,
	/** Data Role has been set to Downstream Facing Port (DFP) */
	DATA_ROLE_IS_DFP,
	/** A PD Explicit Contract is in place */
	PD_CONNECTED,
	/** No PD Explicit Contract is in place */
	NOT_PD_CONNECTED,
	/** Transition the Power Supply */
	TRANSITION_PS,
	/** Port partner is not responsive */
	PORT_PARTNER_NOT_RESPONSIVE,
	/** Protocol Error occurred */
	PROTOCOL_ERROR,
	/** Transition the Sink to default */
	SNK_TRANSITION_TO_DEFAULT,
	/** Hard Reset Received */
	HARD_RESET_RECEIVED,
	/** Sink SubPower state at 0V */
	POWER_CHANGE_0A0,
	/** Sink SubPower state a 5V / 500mA */
	POWER_CHANGE_DEF,
	/** Sink SubPower state a 5V / 1.5A */
	POWER_CHANGE_1A5,
	/** Sink SubPower state a 5V / 3A */
	POWER_CHANGE_3A0,
	/** Sender Response Timeout */
	SENDER_RESPONSE_TIMEOUT,
	/** Source Capabilities Received */
	SOURCE_CAPABILITIES_RECEIVED,
};

/**
 * @brief Device Policy Manager checks
 */
enum usbc_policy_check_t {
	/** Check if Power Role Swap is allowed */
	CHECK_POWER_ROLE_SWAP,
	/** Check if Data Role Swap to DFP is allowed */
	CHECK_DATA_ROLE_SWAP_TO_DFP,
	/** Check if Data Role Swap to UFP is allowed */
	CHECK_DATA_ROLE_SWAP_TO_UFP,
	/** Check if Sink is at default level */
	CHECK_SNK_AT_DEFAULT_LEVEL,
	/** Check if should control VCONN */
	CHECK_VCONN_CONTROL,
	/** Check if Source Power Supply is at default level */
	CHECK_SRC_PS_AT_DEFAULT_LEVEL,
};

/**
 * @brief Device Policy Manager Wait message notifications
 */
enum usbc_policy_wait_t {
	/** The port partner is unable to meet the sink request at this time */
	WAIT_SINK_REQUEST,
	/** The port partner is unable to do a Power Role Swap at this time */
	WAIT_POWER_ROLE_SWAP,
	/** The port partner is unable to do a Data Role Swap at this time */
	WAIT_DATA_ROLE_SWAP,
	/** The port partner is unable to do a VCONN Swap at this time */
	WAIT_VCONN_SWAP,
};

/**
 * @brief Device Policy Manager's response to a Sink Request
 */
enum usbc_snk_req_reply_t {
	/** The sink port partner's request can be met */
	SNK_REQUEST_VALID,
	/** The sink port partner's request can not be met */
	SNK_REQUEST_REJECT,
	/** The sink port partner's request can be met at a later time */
	SNK_REQUEST_WAIT,
};

/** \addtogroup sink_callbacks
 *  @{
 */
/**
 * @brief Callback type used to get the Sink Capabilities
 *
 * @param dev USB-C Connector Instance
 * @param pdos pointer where pdos are stored
 * @param num_pdos pointer where number of pdos is stored
 * @return 0 on success
 */
typedef int (*policy_cb_get_snk_cap_t)(const struct device *dev, uint32_t **pdos, int *num_pdos);
/**
 * @brief Callback type used to report the received Port Partner's
 *	  Source Capabilities
 *
 * @param dev USB-C Connector Instance
 * @param pdos pointer to the partner's source pdos
 * @param num_pdos number of source pdos
 */
typedef void (*policy_cb_set_src_cap_t)(const struct device *dev, const uint32_t *pdos,
					const int num_pdos);

/**
 * @brief Callback type used to check a policy
 *
 * @param dev USB-C Connector Instance
 * @param policy_check policy to check
 * @return true if policy is currently allowed by the device policy manager
 */
typedef bool (*policy_cb_check_t)(const struct device *dev,
				  const enum usbc_policy_check_t policy_check);

/**
 * @brief Callback type used to notify Device Policy Manager of WAIT
 *	  message reception
 *
 * @param dev USB-C Connector Instance
 * @param wait_notify wait notification
 * @return return true if the PE should wait and resend the message
 */
typedef bool (*policy_cb_wait_notify_t)(const struct device *dev,
					const enum usbc_policy_wait_t wait_notify);

/**
 * @brief Callback type used to notify Device Policy Manager of a
 *	  policy change
 *
 * @param dev USB-C Connector Instance
 * @param policy_notify policy notification
 */
typedef void (*policy_cb_notify_t)(const struct device *dev,
				   const enum usbc_policy_notify_t policy_notify);

/**
 * @brief Callback type used to get the Request Data Object (RDO)
 *
 * @param dev USB-C Connector Instance
 * @return RDO
 */
typedef uint32_t (*policy_cb_get_rdo_t)(const struct device *dev);

/**
 * @brief Callback type used to check if the sink power supply is at
 *        the default level
 *
 * @param dev USB-C Connector Instance
 * @return true if power supply is at default level
 */
typedef bool (*policy_cb_is_snk_at_default_t)(const struct device *dev);

/** @}*/
/** \addtogroup source_callbacks
 *  @{
 */
/**
 * @brief Callback type used to get the Source Capabilities
 *	  from the Device Policy Manager
 *
 * @param dev USB-C Connector Instance
 * @param pdos pointer to source capability pdos
 * @param num_pdos pointer to number of source capability pdos
 * @return 0 on success
 */
typedef int (*policy_cb_get_src_caps_t)(const struct device *dev, const uint32_t **pdos,
					uint32_t *num_pdos);

/**
 * @brief Callback type used to check if Sink request is valid
 *
 * @param dev USB-C Connector Instance
 * @param request_msg request message to check
 * @return sink request reply
 */
typedef enum usbc_snk_req_reply_t (*policy_cb_check_sink_request_t)(const struct device *dev,
								    const uint32_t request_msg);

/**
 * @brief Callback type used to check if Source Power Supply is ready
 *
 * @param dev USB-C Connector Instance
 * @return true if power supply is ready, else false
 */
typedef bool (*policy_cb_is_ps_ready_t)(const struct device *dev);

/**
 * @brief Callback type used to check if present Contract is still valid
 *
 * @param dev USB-C Connector Instance
 * @param present_contract present contract
 * @return true if present contract is still valid
 */
typedef bool (*policy_cb_present_contract_is_valid_t)(const struct device *dev,
						   const uint32_t present_contract);

/**
 * @brief Callback type used to request that a different set of Source Caps
 *	  be sent to the Sink
 *
 * @param dev USB-C Connector Instance
 * @return true if a different set of Cource Caps is available
 */
typedef bool (*policy_cb_change_src_caps_t)(const struct device *dev);

/**
 * @brief Callback type used to report the Capabilities received from
 *	  a Sink Port Partner
 *
 * @param dev USB-C Connector Instance
 * @param pdos pointer to sink cap pdos
 * @param num_pdos number of sink cap pdos
 */
typedef void (*policy_cb_set_port_partner_snk_cap_t)(const struct device *dev, const uint32_t *pdos,
						     const int num_pdos);
/**
 * @brief Callback type used to get the Rp value that should be placed on
 *	  the CC lines
 *
 * @param dev USB-C Connector Instance
 * @param rp rp value
 * @return 0 on success
 */
typedef int (*policy_cb_get_src_rp_t)(const struct device *dev, enum tc_rp_value *rp);
/**
 * @brief Callback type used to enable VBUS
 *
 * @param dev USB-C Connector Instance
 * @param en true if VBUS should be enabled, else false to disable it
 * @return 0 on success
 */
typedef int (*policy_cb_src_en_t)(const struct device *dev, bool en);
/** @}*/

/**
 * @brief Start the USB-C Subsystem
 *
 * @param dev Runtime device structure
 *
 * @retval 0 on success
 */
int usbc_start(const struct device *dev);

/**
 * @brief Suspend the USB-C Subsystem
 *
 * @param dev Runtime device structure
 *
 * @retval 0 on success
 */
int usbc_suspend(const struct device *dev);

/**
 * @brief Make a request of the USB-C Subsystem
 *
 * @param dev Runtime device structure
 * @param req request
 *
 * @retval 0 on success
 */
int usbc_request(const struct device *dev, const enum usbc_policy_request_t req);

/**
 * @internal
 * @brief Bypass the next USB-C stack sleep and execute one more iteration of the state machines.
 * Used internally to decrease the response time.
 *
 * @param dev Runtime device structure
 */
void usbc_bypass_next_sleep(const struct device *dev);

/**
 * @brief Set pointer to Device Policy Manager (DPM) data
 *
 * @param dev Runtime device structure
 * @param dpm_data pointer to dpm data
 */
void usbc_set_dpm_data(const struct device *dev, void *dpm_data);

/**
 * @brief Get pointer to Device Policy Manager (DPM) data
 *
 * @param dev Runtime device structure
 *
 * @retval pointer to dpm data that was set with usbc_set_dpm_data
 * @retval NULL if dpm data was not set
 */
void *usbc_get_dpm_data(const struct device *dev);

/**
 * @brief Set the callback used to set VCONN control
 *
 * @param dev Runtime device structure
 * @param cb VCONN control callback
 */
void usbc_set_vconn_control_cb(const struct device *dev, const tcpc_vconn_control_cb_t cb);

/**
 * @brief Set the callback used to discharge VCONN
 *
 * @param dev Runtime device structure
 * @param cb VCONN discharge callback
 */
void usbc_set_vconn_discharge_cb(const struct device *dev, const tcpc_vconn_discharge_cb_t cb);

/**
 * @brief Set the callback used to check a policy
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_check(const struct device *dev, const policy_cb_check_t cb);

/**
 * @brief Set the callback used to notify Device Policy Manager of a
 *	  policy change
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_notify(const struct device *dev, const policy_cb_notify_t cb);

/**
 * @brief Set the callback used to notify Device Policy Manager of WAIT
 *	  message reception
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_wait_notify(const struct device *dev, const policy_cb_wait_notify_t cb);

/**
 * @brief Set the callback used to get the Sink Capabilities
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_get_snk_cap(const struct device *dev, const policy_cb_get_snk_cap_t cb);

/**
 * @brief Set the callback used to store the received Port Partner's
 *	  Source Capabilities
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_set_src_cap(const struct device *dev, const policy_cb_set_src_cap_t cb);

/**
 * @brief Set the callback used to get the Request Data Object (RDO)
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_get_rdo(const struct device *dev, const policy_cb_get_rdo_t cb);

/**
 * @brief Set the callback used to check if the sink power supply is at
 *	  the default level
 *
 * @param dev Runtime device structure
 * @param cb callback
 */
void usbc_set_policy_cb_is_snk_at_default(const struct device *dev,
					  const policy_cb_is_snk_at_default_t cb);

/**
 * @brief Set the callback used to get the Rp value that should be placed on
 *	  the CC lines
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_get_src_rp(const struct device *dev, const policy_cb_get_src_rp_t cb);

/**
 * @brief Set the callback used to enable VBUS
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_src_en(const struct device *dev, const policy_cb_src_en_t cb);

/**
 * @brief Set the callback used to get the Source Capabilities
 *	  from the Device Policy Manager
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_get_src_caps(const struct device *dev, const policy_cb_get_src_caps_t cb);

/**
 * @brief Set the callback used to check if Sink request is valid
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_check_sink_request(const struct device *dev,
					   const policy_cb_check_sink_request_t cb);

/**
 * @brief Set the callback used to check if Source Power Supply is ready
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_is_ps_ready(const struct device *dev,
					  const policy_cb_is_ps_ready_t cb);

/**
 * @brief Set the callback to check if present Contract is still valid
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_present_contract_is_valid(const struct device *dev,
					       const policy_cb_present_contract_is_valid_t cb);

/**
 * @brief Set the callback used to request that a different set of Source Caps
 *	  be sent to the Sink
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_change_src_caps(const struct device *dev,
					const policy_cb_change_src_caps_t cb);

/**
 * @brief Set the callback used to store the Capabilities received from a Sink Port Partner
 *
 * @param dev USB-C Connector Instance
 * @param cb callback
 */
void usbc_set_policy_cb_set_port_partner_snk_cap(const struct device *dev,
						 const policy_cb_set_port_partner_snk_cap_t cb);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USBC_H_ */

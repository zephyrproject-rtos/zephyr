/** @file
 *  @brief Bluetooth subsystem classic core APIs.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_

/**
 * @brief Bluetooth APIs
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic Access Profile (GAP)
 * @defgroup bt_gap Generic Access Profile (GAP)
 * @since 1.0
 * @version 1.0.0
 * @ingroup bluetooth
 * @{
 */

/**
 * @private
 * @brief BR/EDR discovery private structure
 */
struct bt_br_discovery_priv {
	/** Clock offset */
	uint16_t clock_offset;
	/** Page scan repetition mode */
	uint8_t pscan_rep_mode;
	/** Resolving remote name*/
	uint8_t resolve_state;
};

/** Maximum size of Extended Inquiry Response */
#define BT_BR_EIR_SIZE_MAX 240

/** @brief BR/EDR discovery result structure */
struct bt_br_discovery_result {
	/** Private data */
	struct bt_br_discovery_priv _priv;

	/** Remote device address */
	bt_addr_t addr;

	/** RSSI from inquiry */
	int8_t rssi;

	/** Class of Device */
	uint8_t cod[3];

	/** Extended Inquiry Response */
	uint8_t eir[BT_BR_EIR_SIZE_MAX];
};

/** BR/EDR discovery parameters */
struct bt_br_discovery_param {
	/** Maximum length of the discovery in units of 1.28 seconds.
	 *  Valid range is 0x01 - 0x30.
	 */
	uint8_t length;

	/** True if limited discovery procedure is to be used. */
	bool limited;
};

/**
 * @brief Start BR/EDR discovery
 *
 * Start BR/EDR discovery (inquiry) and provide results through the specified
 * callback. The discovery results will be notified through callbacks
 * registered by @ref bt_br_discovery_cb_register.
 * If more inquiry results were received during session than
 * fits in provided result storage, only ones with highest RSSI will be
 * reported.
 *
 * @param param Discovery parameters.
 * @param results Storage for discovery results.
 * @param count Number of results in storage. Valid range: 1-255.
 *
 * @return Zero on success or error code otherwise, positive in case
 * of protocol error or negative (POSIX) in case of stack internal error
 */
int bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  struct bt_br_discovery_result *results, size_t count);

/**
 * @brief Stop BR/EDR discovery.
 *
 * Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 * results won't be reported
 *
 * @return Zero on success or error code otherwise, positive in case of
 *         protocol error or negative (POSIX) in case of stack internal error.
 */
int bt_br_discovery_stop(void);

struct bt_br_discovery_cb {

	/**
	 * @brief An inquiry response received callback.
	 *
	 * @param result Storage used for discovery results
	 */
	void (*recv)(const struct bt_br_discovery_result *result);

	/** @brief The inquiry has stopped after discovery timeout.
	 *
	 * @param results Storage used for discovery results
	 * @param count Number of valid discovery results.
	 */
	void (*timeout)(const struct bt_br_discovery_result *results,
				  size_t count);

	sys_snode_t node;
};

/**
 * @brief Register discovery packet callbacks.
 *
 * Adds the callback structure to the list of callback structures that monitors
 * inquiry activity.
 *
 * This callback will be called for all inquiry activity, regardless of what
 * API was used to start the discovery.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_register(struct bt_br_discovery_cb *cb);

/**
 * @brief Unregister discovery packet callbacks.
 *
 * Remove the callback structure from the list of discovery callbacks.
 *
 * @param cb Callback struct. Must point to memory that remains valid.
 */
void bt_br_discovery_cb_unregister(struct bt_br_discovery_cb *cb);

struct bt_br_oob {
	/** BR/EDR address. */
	bt_addr_t addr;
};

/**
 * @brief Get BR/EDR local Out Of Band information
 *
 * This function allows to get local controller information that are useful
 * for Out Of Band pairing or connection creation process.
 *
 * @param oob Out Of Band information
 */
int bt_br_oob_get_local(struct bt_br_oob *oob);

/**
 * @brief Enable/disable set controller in discoverable state.
 *
 * Allows make local controller to listen on INQUIRY SCAN channel and responds
 * to devices making general inquiry. To enable this state it's mandatory
 * to first be in connectable state.
 *
 * If the device enters limited discoverable mode, the controller will leave from discoverable
 * mode after the duration of @kconfig{BT_LIMITED_DISCOVERABLE_DURATION} seconds in the limited
 * discoverable mode.
 *
 * @param enable Value allowing/disallowing controller to become discoverable.
 * @param limited Value allowing/disallowing controller to enter limited discoverable mode.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_discoverable(bool enable, bool limited);

/**
 * @brief BR/EDR connection request response codes.
 *
 * Response codes for incoming BR/EDR connection requests, used by the
 * connection request callback to accept or reject connections.
 */
enum bt_br_conn_req_rsp {
	/** Accept connection as central role */
	BT_BR_CONN_REQ_ACCEPT_CENTRAL      = BT_HCI_ROLE_CENTRAL,
	/** Accept connection as peripheral role */
	BT_BR_CONN_REQ_ACCEPT_PERIPHERAL   = BT_HCI_ROLE_PERIPHERAL,
	/** Reject connection due to insufficient resources */
	BT_BR_CONN_REQ_REJECT_NO_RESOURCES = BT_HCI_ERR_INSUFFICIENT_RESOURCES,
	/** Reject connection due to insufficient security */
	BT_BR_CONN_REQ_REJECT_SECURITY     = BT_HCI_ERR_INSUFFICIENT_SECURITY,
	/** Reject connection due to unacceptable device address */
	BT_BR_CONN_REQ_REJECT_ADDR         = BT_HCI_ERR_BD_ADDR_UNACCEPTABLE,
};

/**
 * @brief Callback type for incoming BR/EDR connection requests.
 *
 * This callback is invoked when a remote Bluetooth Classic device attempts to
 * establish a connection to the local controller. The application must return
 * a response code to accept or reject the connection, and specify the desired
 * role if accepting.
 *
 * @param addr Remote device address attempting to connect.
 * @param cod Class of Device of the remote device.
 *
 * @return @ref bt_br_conn_req_rsp Response code indicating whether to accept or reject
 *         the connection, and the desired role if accepting.
 */
typedef enum bt_br_conn_req_rsp (*bt_br_conn_req_func_t)(const bt_addr_t *addr, uint32_t cod);

/**
 * @brief Enable/disable set controller in connectable state.
 *
 * Allows make local controller to be connectable. It means the controller
 * start listen to devices requests on PAGE SCAN channel. If disabled also
 * resets discoverability if was set.
 *
 * @param enable Value allowing/disallowing controller to be connectable.
 * @param func   Callback function to handle incoming connection requests. If the @p enable is
 *               false, this parameter is ignored.
 *               If @p func is NULL, the conn_req will be accepted internally. The default role
 *               is peripheral. The role switch request can be performed if the
 *               @kconfig{BT_ACCEPT_CONN_AS_CENTRAL} is enabled.
 *               If @p func is provided, the conn_req will be passed to the callback function
 *               for the application to decide whether to accept or reject the connection, and
 *               the desired role for the connection.
 *
 * @return Negative if fail set to requested state or requested state has been
 *         already set. Zero if done successfully.
 */
int bt_br_set_connectable(bool enable, bt_br_conn_req_func_t func);

/** Minimum BR/EDR scan interval (0x0012, 11.25 ms, 0.625 ms units). */
#define BT_BR_SCAN_INTERVAL_MIN 0x0012
/** Maximum BR/EDR scan interval (0x1000, 2.560 s, 0.625 ms units). */
#define BT_BR_SCAN_INTERVAL_MAX 0x1000

/** Minimum BR/EDR scan window (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_SCAN_WINDOW_MIN 0x0011
/** Maximum BR/EDR scan window (0x1000, 2.560 s, 0.625 ms units). */
#define BT_BR_SCAN_WINDOW_MAX 0x1000

/**
 * @name Defined BR/EDR Page Scan Intervals and Windows
 * @{
 */

/** Default page scan interval (R0) (0x0800, 1.280 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_INTERVAL_R0 0x0800
/** Default page scan window (R0) (0x0800, 1.280 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_WINDOW_R0   0x0800

/** Fast page scan interval (R1) (0x00a0, 100.0 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_FAST_INTERVAL_R1 0x00a0
/** Fast page scan window (R1) (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_FAST_WINDOW_R1   0x0011

/** Medium page scan interval (R1) (0x0800, 1.280 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_MEDIUM_INTERVAL_R1 0x0800
/** Medium page scan window (R1) (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_MEDIUM_WINDOW_R1   0x0011

/** Slow page scan interval (R1) (0x0800, 1.280 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_SLOW_INTERVAL_R1 0x0800
/** Slow page scan window (R1) (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_SLOW_WINDOW_R1   0x0011

/** Fast page scan interval (R2) (0x1000, 2.560 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_FAST_INTERVAL_R2 0x1000
/** Fast page scan window (R2) (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_FAST_WINDOW_R2   0x0011

/** Slow page scan interval (R2) (0x1000, 2.560 s, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_SLOW_INTERVAL_R2 0x1000
/** Slow page scan window (R2) (0x0011, 10.625 ms, 0.625 ms units). */
#define BT_BR_PAGE_SCAN_SLOW_WINDOW_R2   0x0011

/**
 * @}
 */

/** Page scan type. */
enum bt_br_scan_type {
	/** Standard scan (default) */
	BT_BR_SCAN_TYPE_STANDARD = 0,

	/** Interlaced scan (1.2 devices only) */
	BT_BR_SCAN_TYPE_INTERLACED = 1,
};

/** Page scan parameters. */
struct bt_br_page_scan_param {
	/** Page scan interval in 0.625 ms units
	 *  Range: 0x0012 to 0x1000.
	 *
	 *  According to the Bluetooth Core Specification, only even values are
	 *  used by the controller; if an odd value is provided, the controller
	 *  may ignore the LSB or round the value. Applications should use even
	 *  values to be compliant.
	 */
	uint16_t interval;

	/** Page scan window in 0.625 ms units
	 *  Range: 0x0011 to 0x1000.
	 */
	uint16_t window;

	/** Page scan type. */
	enum bt_br_scan_type type;
};

/**
 * @brief Initialize BR Scan parameters
 *
 * @param _interval  Scan interval
 * @param _window    Scan window
 * @param _type      Scan type
 */

#define BT_BR_SCAN_INIT(_interval, _window, _type) \
{ \
	.interval = (_interval), \
	.window = (_window), \
	.type = (_type) \
}

/**
 * @brief Helper to declare BR/EDR page scan parameters inline
 *
 * @param _interval page scan interval, N * 0.625 milliseconds
 * @param _window   page scan window, N * 0.625 milliseconds
 * @param _type     @ref BT_BR_SCAN_TYPE_STANDARD or @ref BT_BR_SCAN_TYPE_INTERLACED
 */

#define BT_BR_PAGE_SCAN_PARAM(_interval, _window, _type) \
	((const struct bt_br_page_scan_param[]) { \
		BT_BR_SCAN_INIT(_interval, _window, _type) \
	})

/**
 * @brief Default page scan parameters for R0
 *
 * Page scan interval and window are set to 1.280 seconds (0x0800 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_R0 BT_BR_PAGE_SCAN_PARAM(BT_BR_PAGE_SCAN_INTERVAL_R0, \
						       BT_BR_PAGE_SCAN_WINDOW_R0, \
						       BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Fast page scan parameters for R1
 *
 * Page scan interval is set to 100 ms (0x00A0 in 0.625 ms units), and the
 * page scan window is set to 10.625ms (0x0011 in 0.625 ms units).
 * The scan type is set to interlaced.
 */
#define BT_BR_PAGE_SCAN_PARAM_FAST_R1 \
	BT_BR_PAGE_SCAN_PARAM(BT_BR_PAGE_SCAN_FAST_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_FAST_WINDOW_R1, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Medium page scan parameters for R1
 *
 * Page scan interval is set to 1.280 seconds (0x0800 in 0.625 ms units), and the
 * page scan window is set to 10.625ms (0x0011 in 0.625 ms units).
 * The scan type is set to interlaced.
 */
#define BT_BR_PAGE_SCAN_PARAM_MEDIUM_R1 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_MEDIUM_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_MEDIUM_WINDOW_R1, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Slow page scan parameters for R1
 *
 * Page scan interval is set to 1.280 seconds (0x0800 in 0.625 ms units), and the
 * page scan window is set to 10.625ms (0x0011 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_SLOW_R1 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_SLOW_INTERVAL_R1, \
	BT_BR_PAGE_SCAN_SLOW_WINDOW_R1, \
	BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Fast page scan parameters for R2
 *
 * Page scan interval is set to 2.560 seconds (0x1000 in 0.625 ms units), and the
 * page scan window is set to 10.625ms (0x0011 in 0.625 ms units).
 * The scan type is set to interlaced.
 */
#define BT_BR_PAGE_SCAN_PARAM_FAST_R2 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_FAST_INTERVAL_R2, \
	BT_BR_PAGE_SCAN_FAST_WINDOW_R2, \
	BT_BR_SCAN_TYPE_INTERLACED)

/**
 * @brief Slow page scan parameters for R2
 *
 * Page scan interval is set to 2.560 seconds (0x1000 in 0.625 ms units), and the
 * page scan window is set to 10.625ms (0x0011 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_PAGE_SCAN_PARAM_SLOW_R2 \
	BT_BR_PAGE_SCAN_PARAM( \
	BT_BR_PAGE_SCAN_SLOW_INTERVAL_R2, \
	BT_BR_PAGE_SCAN_SLOW_WINDOW_R2, \
	BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Update BR/EDR page scan parameters.
 *
 * This function updates the page scan parameters of the local BR/EDR controller.
 * Page scan parameters determine how the controller listens for incoming
 * connection requests from remote devices.
 *
 * The function validates the provided parameters, including the interval,
 * window, and scan type, and sends the appropriate HCI commands to update
 * the controller's page scan activity and scan type.
 *
 * The user can set custom page scan parameters using the helper macro
 * BT_BR_PAGE_SCAN_PARAM() to define their own values.
 * Alternatively, the user can use predefined standard parameters as defined
 * in the Bluetooth specification:
 * - @ref BT_BR_PAGE_SCAN_PARAM_R0 Default page scan parameters.
 * - @ref BT_BR_PAGE_SCAN_PARAM_FAST_R1 Fast page scan parameters for R1.
 * - @ref BT_BR_PAGE_SCAN_PARAM_MEDIUM_R1 Medium page scan parameters for R1.
 * - @ref BT_BR_PAGE_SCAN_PARAM_SLOW_R1 Slow page scan parameters for R1.
 * - @ref BT_BR_PAGE_SCAN_PARAM_FAST_R2 Fast page scan parameters for R2.
 * - @ref BT_BR_PAGE_SCAN_PARAM_SLOW_R2 Slow page scan parameters for R2.
 *
 * These predefined parameters are designed to meet common use cases and
 * ensure compliance with the Bluetooth specification.
 *
 * @param param Page scan parameters, including:
 *              - interval: Time between consecutive page scans (in 0.625 ms units).
 *                          Must be in the range [0x0012, 0x1000].
 *              - window: Duration of a single page scan (in 0.625 ms units).
 *                        Must be in the range [0x0011, 0x1000].
 *              - type: Page scan type (e.g., standard or interlaced).
 *
 * @return 0 on success.
 * @return -EINVAL if the provided parameters are invalid.
 * @return -EAGAIN if the device is not ready.
 * @return -ENOBUFS if memory allocation for HCI commands fails.
 * @return Other negative error codes for internal failures.
 */
int bt_br_page_scan_update_param(const struct bt_br_page_scan_param *param);

/**
 * @brief BR/EDR inquiry scan parameters
 * @note These parameters are used to configure the inquiry scan behavior of the
 *       local BR/EDR controller.
 */
struct bt_br_inquiry_scan_param {
	/** Inquiry scan interval in 0.625 ms units
	 *  Range: 0x0012 to 0x1000.
	 *
	 *  According to the Bluetooth Core Specification, only even values are
	 *  used by the controller; if an odd value is provided, the controller
	 *  may ignore the LSB or round the value. Applications should use even
	 *  values to be compliant.
	 */
	uint16_t interval;

	/** Inquiry scan window in 0.625 ms units
	 *  Range: 0x0011 to 0x1000.
	 */
	uint16_t window;

	/** Inquiry scan type. */
	enum bt_br_scan_type type;
};

/**
 * @name Defined BR Inquiry Scan Intervals and Windows
 * @{
 */

/** Default inquiry scan interval (0x1000, 2.560 s, 0.625 ms units). */
#define BT_BR_INQUIRY_SCAN_INTERVAL_DEFAULT 0x1000
/** Default inquiry scan window (0x0012, 11.25 ms, 0.625 ms units). */
#define BT_BR_INQUIRY_SCAN_WINDOW_DEFAULT   0x0012

/**
 * @}
 */

/**
 * @brief Helper to declare BR/EDR inquiry scan parameters inline
 *
 * @param _interval Inquiry scan interval, N * 0.625 milliseconds
 * @param _window   Inquiry scan window, N * 0.625 milliseconds
 * @param _type     @ref BT_BR_SCAN_TYPE_STANDARD or @ref BT_BR_SCAN_TYPE_INTERLACED
 */
#define BT_BR_INQUIRY_SCAN_PARAM(_interval, _window, _type)                                        \
	((const struct bt_br_inquiry_scan_param[]){BT_BR_SCAN_INIT(_interval, _window, _type)})

/**
 * @brief Default inquiry scan parameters
 *
 * Inquiry scan interval is set to 2.560 seconds (0x1000 in 0.625 ms units), and the
 * inquiry scan window is set to 11.25 milliseconds (0x0012 in 0.625 ms units).
 * The scan type is set to standard.
 */
#define BT_BR_INQUIRY_SCAN_PARAM_DEFAULT                                                           \
	BT_BR_INQUIRY_SCAN_PARAM(BT_BR_INQUIRY_SCAN_INTERVAL_DEFAULT,                              \
				 BT_BR_INQUIRY_SCAN_WINDOW_DEFAULT, BT_BR_SCAN_TYPE_STANDARD)

/**
 * @brief Update BR/EDR inquiry scan parameters.
 *
 * This function updates the inquiry scan parameters of the local BR/EDR controller.
 * Inquiry scan parameters determine how the controller handles inquiry requests.
 *
 * The function validates the provided parameters, including the interval,
 * window, and scan type, and sends the appropriate HCI commands to update
 * the controller's inquiry scan activity and scan type.
 *
 * The user can set custom inquiry scan parameters using the helper macro
 * @ref BT_BR_INQUIRY_SCAN_PARAM to define their own values.
 * Alternatively, the user can use predefined standard parameters as defined
 * in the Bluetooth specification:
 * - @ref BT_BR_INQUIRY_SCAN_PARAM_DEFAULT Default inquiry scan parameters.
 * These predefined parameters are designed to meet common use cases and
 * ensure compliance with the Bluetooth specification.
 *
 * @param param Inquiry scan parameters, including:
 *              - interval: Time between consecutive inquiry scans (in 0.625 ms units).
 *                          Must be in the range [0x0012, 0x1000].
 *              - window: Duration of a single inquiry scan (in 0.625 ms units).
 *                        Must be in the range [0x0011, 0x1000].
 *              - type: Inquiry scan type (e.g., standard or interlaced).
 *
 * @return 0 on success.
 * @return -EINVAL if the provided parameters are invalid.
 * @return -EAGAIN if the device is not ready.
 * @return -ENOBUFS if memory allocation for HCI commands fails.
 * @return Other negative error codes for internal failures.
 */
int bt_br_inquiry_scan_update_param(const struct bt_br_inquiry_scan_param *param);

/**
 * @brief Set the Class of Device of the local BR/EDR Controller.
 *
 * This function writes the Class of Device (COD) value to the BR/EDR
 * controller. The COD is used by remote devices during the discovery
 * process to identify the type of device and the services it provides.
 *
 * @note The limited discoverable mode bit (@ref BT_COD_MAJOR_SVC_CLASS_LIMITED_DISCOVER)
 *       cannot be set directly through this function. Use bt_br_set_discoverable()
 *       with the limited parameter to enable limited discoverable mode, which will
 *       automatically set the corresponding COD bit.
 *
 * @param cod Class of Device value to set. This should be a combination of
 *            Major Service Class bits (BT_COD_MAJOR_SVC_CLASS_*), Major Device
 *            Class bits (BT_COD_MAJOR_DEVICE_CLASS_*), and Minor Device Class
 *            bits (BT_COD_MINOR_DEVICE_CLASS_*).
 *
 * @retval 0 on success
 * @retval -EAGAIN if the Bluetooth device is not ready
 * @retval -EINVAL if the provided COD value attempts to set the limited
 *         discoverable bit directly, which is not permitted
 * @retval Other negative error codes from underlying HCI command failures
 *
 * @see bt_br_set_discoverable()
 * @see bt_br_get_class_of_device()
 */
int bt_br_set_class_of_device(uint32_t cod);

/**
 * @brief Get the Class of Device of the local BR/EDR Controller.
 *
 * @param cod Pointer to where the current Class of Device value will be stored.
 *
 * @retval 0 on success
 * @retval -EAGAIN if the Bluetooth device is not ready
 * @retval -EINVAL if @p cod is NULL
 * @retval Other negative error codes on failure
 */
int bt_br_get_class_of_device(uint32_t *cod);

/** @brief Check if a Bluetooth classic device address is bonded.
 *
 *  @param addr Bluetooth classic device address.
 *
 *  @return true if @p addr is bonded
 */
bool bt_br_bond_exists(const bt_addr_t *addr);

/**
 * @brief Clear classic pairing information .
 *
 * @param addr  Remote address, NULL or BT_ADDR_ANY to clear all remote devices.
 *
 * @return 0 on success or negative error value on failure.
 */
int bt_br_unpair(const bt_addr_t *addr);

/** Information about a bond with a remote device. */
struct bt_br_bond_info {
	/** Address of the remote device. */
	bt_addr_t addr;
};

/**
 * @brief Iterate through all existing bonds of Classic.
 *
 * @param func       Function to call for each bond.
 * @param user_data  Data to pass to the callback function.
 */
void bt_br_foreach_bond(void (*func)(const struct bt_br_bond_info *info, void *user_data),
			void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CLASSIC_H_ */

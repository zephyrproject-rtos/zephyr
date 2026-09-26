/** @file
 *  @brief Bluetooth HCI driver API.
 *
 *  Copyright (c) 2024 Johan Hedberg
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup bt_hci_api
 * @brief Main header file for Bluetooth HCI driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_

/**
 * @brief Interfaces for Bluetooth Host Controller Interface (HCI).
 * @defgroup bt_hci_api Bluetooth HCI
 *
 * @since 3.7
 * @version 0.3.0
 *
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Parameters of the setup() driver API op.
 *
 *  @deprecated Together with the op itself, see bt_hci_setup(). A driver takes
 *              the public address from bt_hci_get_public_addr() instead.
 */
struct bt_hci_setup_params {
	/** The public identity address to give to the controller. This field is used when the
	 *  driver selects @kconfig{CONFIG_BT_HCI_SET_PUBLIC_ADDR} to indicate that it supports
	 *  setting the controller's public address. It is @ref BT_ADDR_ANY when the application
	 *  has not created a public identity.
	 */
	bt_addr_t public_addr;
};

enum {
	/* The host should never send HCI_Reset */
	BT_HCI_QUIRK_NO_RESET = BIT(0),
	/* The controller does not auto-initiate a DLE procedure when the
	 * initial connection data length parameters are not equal to the
	 * default data length parameters. Therefore the host should initiate
	 * the DLE procedure after connection establishment.
	 *
	 * That requirement is stated in Core Spec v5.4 Vol 6 Part B. 4.5.10
	 * Data PDU length management:
	 *
	 * > For a new connection:
	 * > - ... If either value is not 27, then the Controller should
	 * >   initiate the Data Length Update procedure at the earliest
	 * >   practical opportunity.
	 */
	BT_HCI_QUIRK_NO_AUTO_DLE = BIT(1),
	/** The controller advertises the controller to host flow control
	 * commands as supported but does not accept them. The host treats
	 * the feature as not supported.
	 */
	BT_HCI_QUIRK_NO_FLOW_CONTROL = BIT(2),
};

#define BT_DT_HCI_QUIRK_OR(node_id, prop, idx) \
	UTIL_CAT(BT_HCI_QUIRK_, DT_STRING_UPPER_TOKEN_BY_IDX(node_id, prop, idx))
#define BT_DT_HCI_QUIRKS_GET(node_id) COND_CODE_1(DT_NODE_HAS_PROP(node_id, bt_hci_quirks), \
						  (DT_FOREACH_PROP_ELEM_SEP(node_id, \
									    bt_hci_quirks, \
									    BT_DT_HCI_QUIRK_OR, \
									    (|))), \
						  (0))
#define BT_DT_HCI_QUIRKS_INST_GET(inst) BT_DT_HCI_QUIRKS_GET(DT_DRV_INST(inst))

#define BT_DT_HCI_NAME_GET(node_id) DT_PROP_OR(node_id, bt_hci_name, "HCI")
#define BT_DT_HCI_NAME_INST_GET(inst) BT_DT_HCI_NAME_GET(DT_DRV_INST(inst))

/* Fallback default when there's no property, same as "virtual" */
#define BT_PRIV_HCI_BUS_DEFAULT (0)
#define BT_DT_HCI_BUS_GET(node_id) DT_ENUM_IDX_OR(node_id, bt_hci_bus, BT_PRIV_HCI_BUS_DEFAULT)

#define BT_DT_HCI_BUS_INST_GET(inst) BT_DT_HCI_BUS_GET(DT_DRV_INST(inst))

/**
 * @brief Common Bluetooth HCI driver configuration.
 *
 * This structure is common to all Bluetooth HCI drivers and must be the first member
 * in the object pointed to by the config field in the device
 * structure.
 */
struct bt_hci_driver_config {
	/** Quirks for this HCI device instance */
	uint32_t quirks;
};

/**
 * @brief Static initializer for @p bt_hci_driver_config struct
 *
 * @param node_id Devicetree node identifier
 */
#define BT_DT_HCI_DRIVER_CONFIG_GET(node_id)                                                    \
	{                                                                                       \
		.quirks = (uint32_t)BT_DT_HCI_QUIRKS_GET(node_id),                                \
	}

/**
 * @brief Static initializer for @p bt_hci_driver_config struct from
 * @c DT_DRV_COMPAT instance.
 *
 * @param inst @c DT_DRV_COMPAT instance number
 * @see BT_DT_HCI_DRIVER_CONFIG_GET()
 */

#define BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst)                                                  \
	BT_DT_HCI_DRIVER_CONFIG_GET(DT_DRV_INST(inst))

/**
 * @def_driverbackendgroup{Bluetooth HCI,bt_hci_api}
 * @{
 */

/**
 * @brief Deliver HCI data from the controller to the host
 *
 * Registered with bt_hci_open(). The HCI driver invokes this callback from thread context.
 * It may do so already before bt_hci_open() returns, and does not do so any more once
 * bt_hci_close() has returned successfully.
 */
typedef int (*bt_hci_recv_t)(const struct device *dev, struct net_buf *buf);

/**
 * @brief Common Bluetooth HCI driver data.
 *
 * This structure is common to all Bluetooth HCI drivers and must be the first member
 * in the driver's data struct.
 */
struct bt_hci_driver_data {
	/** Callback for the driver to deliver data received from the controller to the host. */
	bt_hci_recv_t recv;
#if defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR) || defined(__DOXYGEN__)
	/**
	 * @brief Public identity address to configure in the controller.
	 *
	 * @ref BT_ADDR_ANY when there is none, which is what zero-initialized
	 * driver data starts out with, and not @ref BT_ADDR_NONE (see
	 * bt_hci_get_public_addr()).
	 *
	 * Set with bt_hci_set_public_addr(), read with bt_hci_get_public_addr().
	 *
	 * @kconfig_dep{CONFIG_BT_HCI_SET_PUBLIC_ADDR}
	 */
	bt_addr_t public_addr;
#endif /* CONFIG_BT_HCI_SET_PUBLIC_ADDR */
};

/**
 * @brief Callback API to open the HCI transport.
 * See bt_hci_open() for argument description
 */
typedef int (*bt_hci_api_open_t)(const struct device *dev);

/**
 * @brief Callback API to close the HCI transport.
 * See bt_hci_close() for argument description
 */
typedef int (*bt_hci_api_close_t)(const struct device *dev);

/**
 * @brief Callback API to send an HCI buffer to the controller.
 * See bt_hci_send() for argument description
 */
typedef int (*bt_hci_api_send_t)(const struct device *dev, struct net_buf *buf);

/**
 * @brief Callback API for HCI vendor-specific setup.
 * See bt_hci_setup() for argument description
 *
 * @deprecated See bt_hci_setup().
 */
typedef int (*bt_hci_api_setup_t)(const struct device *dev,
				  const struct bt_hci_setup_params *param);

/**
 * @driver_ops{Bluetooth HCI}
 *
 * The operations serve any user of this API, of which the Bluetooth Host is only one:
 * a controller-only application or a test opens, closes and sends in the same way. With
 * the exception of setup(), they therefore do not use the Host's HCI command APIs
 * (bt_hci_cmd_alloc(), bt_hci_cmd_send(), bt_hci_cmd_send_sync()). The allocators of
 * @ref bt_buf "the buffer API", bt_hci_recv() and bt_hci_recv_err() are what a driver
 * delivers its data with, and not meant by that. A driver that has HCI commands of its
 * own to exchange with its controller does so over its own transport, for which the
 * @ref bt_hci_pkt and the @ref bt_hci_lockstep exist, and only inside open() and
 * close(), while the transport is not in the hands of its user.
 *
 * bt_hci_open(), bt_hci_close() and bt_hci_send() are not safe to call concurrently
 * for the same device: none of them may be called while another one, or another call
 * of the same one, is in progress. A transport has one user, the one that opened it,
 * which is the Bluetooth Host or, in a build without one, the application. That user
 * makes its calls one at a time and within the limits that the three functions
 * document, and a driver need not defend against anything else.
 */
__subsystem struct bt_hci_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief bt_hci_open
	 */
	bt_hci_api_open_t open;
	/**
	 * @driver_ops_optional @copybrief bt_hci_close
	 */
	bt_hci_api_close_t close;
	/**
	 * @driver_ops_mandatory @copybrief bt_hci_send
	 */
	bt_hci_api_send_t send;
#if defined(CONFIG_BT_HCI_SETUP) || defined(__DOXYGEN__)
	/**
	 * @driver_ops_optional @copybrief bt_hci_setup
	 * @kconfig_dep{CONFIG_BT_HCI_SETUP}
	 *
	 * @deprecated See bt_hci_setup().
	 */
	bt_hci_api_setup_t setup;
#endif /* CONFIG_BT_HCI_SETUP */
};
/**
 * @}
 */

/**
 * @brief Deliver an HCI packet from the driver.
 *
 * This function is called by the HCI driver to deliver data received from the
 * controller to the host. The buffer contains the raw HCI packet, including the
 * packet type prefix encoded in the H:4 format.
 *
 * If the function returns 0 (success) the reference to @c buf was moved to the
 * higher layer (e.g. host stack). On error, the caller (HCI driver) still owns
 * the reference and is responsible for eventually calling @ref net_buf_unref
 * on it.
 *
 * @param dev  HCI device
 * @param buf  Buffer containing data received from the controller.
 *
 * @return 0 on success or negative POSIX error number on failure.
 * @retval -ENOTCONN The HCI transport is not open.
 */
static inline int bt_hci_recv_err(const struct device *dev, struct net_buf *buf)
{
	struct bt_hci_driver_data *data = dev->data;

	if (data->recv == NULL) {
		return -ENOTCONN;
	}

	return data->recv(dev, buf);
}

/**
 * @brief Deliver an HCI packet from the driver.
 *
 * This function is the same as @ref bt_hci_recv_err except that it will internally handle
 * error situations and always consume the buffer reference.
 *
 * @param dev  HCI device
 * @param buf  Buffer containing data received from the controller.
 */
static inline void bt_hci_recv(const struct device *dev, struct net_buf *buf)
{
	int err;

	err = bt_hci_recv_err(dev, buf);
	if (err != 0) {
		net_buf_unref(buf);
	}
}

/**
 * @brief Open the HCI transport.
 *
 * Opens the HCI transport for operation. This function must not
 * return until the transport is ready for operation, meaning it
 * is safe to start calling the send() handler. It may block, and is
 * called from thread context.
 *
 * When this function fails, the transport is closed: the driver has stopped
 * what the call had started, no longer calls the recv callback, and
 * bt_hci_close() is not called for it. The exception is -EALREADY, which this
 * function returns without calling the driver when the transport is open
 * already, and which leaves it open.
 *
 * A transport that has been closed with bt_hci_close() can be opened again.
 * The function must not be called while another call to it, or to
 * bt_hci_close(), is in progress.
 *
 * @param dev  HCI device
 * @param recv This is callback through which the HCI driver provides the
 *             host with data from the controller. The callback is expected
 *             to be called from thread context, and it may be called already
 *             before bt_hci_open() returns.
 *
 * @return 0 on success or negative POSIX error number on failure.
 * @retval -EALREADY The HCI transport is already open.
 */
static inline int bt_hci_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_hci_driver_data *data = dev->data;
	int err;

	if (data->recv != NULL) {
		return -EALREADY;
	}

	data->recv = recv;

	err = DEVICE_API_GET(bt_hci, dev)->open(dev);
	if (err != 0) {
		data->recv = NULL;
	}

	return err;
}

/**
 * @brief Check whether the HCI transport can be closed.
 *
 * Closing the transport is an optional driver operation. This tells its user
 * up front whether bt_hci_close() can work, for example before it does
 * something that only makes sense if the transport is closed afterwards.
 *
 * @param dev HCI device
 *
 * @retval true  The driver supports closing the transport.
 * @retval false The driver does not support closing the transport.
 */
static inline bool bt_hci_can_close(const struct device *dev)
{
	return DEVICE_API_GET(bt_hci, dev)->close != NULL;
}

/**
 * @brief Close the HCI transport.
 *
 * Closes the HCI transport. When it succeeds, this function must not return
 * until the transport is closed: the driver then no longer calls the recv
 * callback, and the transport can be opened again with bt_hci_open(). When
 * the function fails, the transport is still open and can be used as before.
 *
 * The function must not be called while a bt_hci_send() call is in progress,
 * nor while another call to it, or to bt_hci_open(), is in progress, and
 * bt_hci_send() must not be called while it runs. It must not be called from
 * the recv callback, so that a driver is free to stop the thread it delivers
 * its data from. It may block, and is called from thread context.
 *
 * The transport must be open, see bt_hci_send().
 *
 * @param dev HCI device
 *
 * @return 0 on success or negative POSIX error number on failure.
 * @retval -ENOSYS The driver does not support closing the transport.
 */
static inline int bt_hci_close(const struct device *dev)
{
	struct bt_hci_driver_data *data = dev->data;
	int err = 0;

	if (!bt_hci_can_close(dev)) {
		return -ENOSYS;
	}

	err = DEVICE_API_GET(bt_hci, dev)->close(dev);
	if (err == 0) {
		data->recv = NULL;
	}

	return err;
}

/**
 * @brief Send HCI buffer to controller.
 *
 * Send an HCI packet to the controller. The packet type is encoded as H:4,
 * i.e. the UART transport encoding, as a prefix to the actual payload. This means
 * that HCI drivers that use H:4 as their native encoding don't need to do any
 * special handling of the packet type.
 *
 * If the function returns 0 (success) the reference to @c buf was moved to the
 * HCI driver. On error, the caller still owns the reference and is responsible
 * for eventually calling @ref net_buf_unref on it.
 *
 * The transport must be open: the function may only be called after
 * bt_hci_open() has returned successfully, and neither while a bt_hci_close()
 * call is in progress nor after one has succeeded. There is no query for
 * that: the one user of the transport knows it from its own calls. Calling
 * the function for a transport that is not open is an error of the caller
 * with undefined results, which a driver is not required to detect. Nor may
 * the function be called while another call to it is in progress.
 *
 * @note This function must only be called from a cooperative thread.
 *
 * @param dev HCI device
 * @param buf Buffer containing data to be sent to the controller.
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_send(const struct device *dev, struct net_buf *buf)
{
	return DEVICE_API_GET(bt_hci, dev)->send(dev, buf);
}

#if defined(CONFIG_BT_HCI_SETUP) || defined(__DOXYGEN__)
/**
 * @brief HCI vendor-specific setup
 *
 * Executes vendor-specific commands sequence to initialize
 * BT Controller before BT Host executes Reset sequence. This is normally
 * called directly after bt_hci_open().
 *
 * @note @kconfig{CONFIG_BT_HCI_SETUP} must be selected for this
 * field to be available.
 *
 * @deprecated A driver performs its vendor-specific initialization inside
 *             open(), over its own transport, with the helpers of hci_pkt.h
 *             and hci_lockstep.h, and takes the public address from
 *             bt_hci_get_public_addr(). Unlike setup(), that works in every
 *             build type, including one without a Bluetooth Host. The op, this
 *             function, @ref bt_hci_setup_params and
 *             @kconfig{CONFIG_BT_HCI_SETUP} are removed two releases after
 *             this one; see the 4.5 migration guide.
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_setup(const struct device *dev, struct bt_hci_setup_params *params)
{
	const struct bt_hci_driver_api *api = DEVICE_API_GET(bt_hci, dev);

	if (api->setup == NULL) {
		return -ENOSYS;
	}

	return api->setup(dev, params);
}
#endif

/**
 * @brief Set the public identity address for the controller.
 *
 * Stores the public address the driver should configure in the controller.
 *
 * The Bluetooth Host calls this before bt_hci_open() when the application has
 * created a public identity with bt_id_create(). A controller-only application
 * can likewise call it before opening the transport.
 *
 * The driver reads the address with bt_hci_get_public_addr() and applies it
 * while opening the transport, or in its setup() implementation.
 *
 * @kconfig_dep{CONFIG_BT_HCI_SET_PUBLIC_ADDR}
 *
 * @param dev  HCI device
 * @param addr Public address, or @ref BT_ADDR_ANY to clear a previously set one.
 *             @ref BT_ADDR_NONE does not clear it, see bt_hci_get_public_addr().
 */
void bt_hci_set_public_addr(const struct device *dev, const bt_addr_t *addr);

/**
 * @brief Get the public identity address the driver is to configure.
 *
 * Returns the address stored with bt_hci_set_public_addr(), for the driver to
 * write into the controller while opening the transport. It does not query
 * the controller.
 *
 * "No address" is @ref BT_ADDR_ANY and not @ref BT_ADDR_NONE, even though the
 * name of the latter suggests it. @ref BT_ADDR_ANY is the address part of
 * @ref BT_ADDR_LE_ANY, with which the Host marks an identity that has no
 * address, and it is what the setup() op receives in
 * @ref bt_hci_setup_params.public_addr when there is no public identity, so a
 * driver has one check whichever way it gets the address. It is also the
 * all-zero address, so driver data that has never been written already reads
 * as "no address". This function cannot fail, so the driver compares the
 * address it returns with @ref BT_ADDR_ANY, using bt_addr_eq(), before it
 * uses it.
 *
 * @kconfig_dep{CONFIG_BT_HCI_SET_PUBLIC_ADDR}
 *
 * @param dev HCI device
 *
 * @return The address to configure, never NULL, valid until the next
 *         bt_hci_set_public_addr() call for the device. It compares equal to
 *         @ref BT_ADDR_ANY when no public address has been set, or it has been
 *         cleared.
 */
const bt_addr_t *bt_hci_get_public_addr(const struct device *dev);

/**
 * @}
 */

/* The following functions are not strictly part of the HCI driver API, in that
 * they do not take as input a struct device which implements the HCI driver API.
 */

/**
 * @brief Setup the HCI transport, which usually means to reset the
 * Bluetooth IC.
 *
 * @note A weak version of this function is included in the H4 driver, so
 *       defining it is optional per board.
 *
 * @param dev The device structure for the bus connecting to the IC
 *
 * @return 0 on success, negative error value on failure
 */
int bt_hci_transport_setup(const struct device *dev);

/**
 * @brief Teardown the HCI transport.
 *
 * @note A weak version of this function is included in the IPC driver, so
 *		defining it is optional. NRF5340 includes support to put network core
 *		in reset state.
 *
 * @param dev The device structure for the bus connecting to the IC
 *
 * @return 0 on success, negative error value on failure
 */
int bt_hci_transport_teardown(const struct device *dev);

/** Allocate an HCI event buffer.
 *
 * This function allocates a new buffer for an HCI event. It is given the
 * event code and the total length of the parameters. Upon successful return
 * the buffer is ready to have the parameters encoded into it.
 *
 * @param evt        HCI event OpCode.
 * @param len        Length of event parameters.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_evt_create(uint8_t evt, uint8_t len);

/** Allocate an HCI Command Complete event buffer.
 *
 * This function allocates a new buffer for HCI Command Complete event.
 * It is given the OpCode (encoded e.g. using the BT_OP macro) and the total
 * length of the parameters. Upon successful return the buffer is ready to have
 * the parameters encoded into it.
 *
 * @param op         HCI command OpCode.
 * @param plen       Length of command parameters.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_cmd_complete_create(uint16_t op, uint8_t plen);

/** Allocate an HCI Command Status event buffer.
 *
 * This function allocates a new buffer for HCI Command Status event.
 * It is given the OpCode (encoded e.g. using the BT_OP macro) and the status
 * code. Upon successful return the buffer is ready to have the parameters
 * encoded into it.
 *
 * @param op         HCI command OpCode.
 * @param status     Status code.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_cmd_status_create(uint16_t op, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_ */

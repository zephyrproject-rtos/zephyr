/** @file
 *  @brief Bluetooth data buffer API
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_BUF_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_BUF_H_

/**
 * @brief Data buffers
 * @defgroup bt_buf Data buffers
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Possible types of buffers passed around the Bluetooth stack */
enum bt_buf_type {
	/** HCI command */
	BT_BUF_CMD,
	/** HCI event */
	BT_BUF_EVT,
	/** Outgoing ACL data */
	BT_BUF_ACL_OUT,
	/** Incoming ACL data */
	BT_BUF_ACL_IN,
	/** Outgoing ISO data */
	BT_BUF_ISO_OUT,
	/** Incoming ISO data */
	BT_BUF_ISO_IN,
	/** H:4 data */
	BT_BUF_H4,
};

/** @brief This is a base type for bt_buf user data. */
struct bt_buf_data {
	uint8_t type;
};

#if defined(CONFIG_BT_HCI_RAW)
#define BT_BUF_RESERVE MAX(CONFIG_BT_HCI_RESERVE, CONFIG_BT_HCI_RAW_RESERVE)
#else
#define BT_BUF_RESERVE CONFIG_BT_HCI_RESERVE
#endif

/** Helper to include reserved HCI data in buffer calculations */
#define BT_BUF_SIZE(size) (BT_BUF_RESERVE + (size))

/** Helper to calculate needed buffer size for HCI ACL packets */
#define BT_BUF_ACL_SIZE(size) BT_BUF_SIZE(BT_HCI_ACL_HDR_SIZE + (size))

/** Helper to calculate needed buffer size for HCI Event packets. */
#define BT_BUF_EVT_SIZE(size) BT_BUF_SIZE(BT_HCI_EVT_HDR_SIZE + (size))

/** Helper to calculate needed buffer size for HCI Command packets. */
#define BT_BUF_CMD_SIZE(size) BT_BUF_SIZE(BT_HCI_CMD_HDR_SIZE + (size))

/** Helper to calculate needed buffer size for HCI ISO packets. */
#define BT_BUF_ISO_SIZE(size) BT_BUF_SIZE(BT_HCI_ISO_HDR_SIZE + \
					  BT_HCI_ISO_TS_DATA_HDR_SIZE + \
					  (size))

/** Data size needed for HCI ACL RX buffers */
#define BT_BUF_ACL_RX_SIZE BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_RX_SIZE)

/** Data size needed for HCI Event RX buffers */
#define BT_BUF_EVT_RX_SIZE BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_RX_SIZE)

#if defined(CONFIG_BT_ISO)
#define BT_BUF_ISO_RX_SIZE BT_BUF_ISO_SIZE(CONFIG_BT_ISO_RX_MTU)
#define BT_BUF_ISO_RX_COUNT CONFIG_BT_ISO_RX_BUF_COUNT
#else
#define BT_BUF_ISO_RX_SIZE 0
#define BT_BUF_ISO_RX_COUNT 0
#endif /* CONFIG_BT_ISO */

/** Data size needed for HCI ACL, HCI ISO or Event RX buffers */
#define BT_BUF_RX_SIZE (MAX(MAX(BT_BUF_ACL_RX_SIZE, BT_BUF_EVT_RX_SIZE), \
			    BT_BUF_ISO_RX_SIZE))

/** Buffer count needed for HCI ACL, HCI ISO or Event RX buffers */
#define BT_BUF_RX_COUNT (MAX(MAX(CONFIG_BT_BUF_EVT_RX_COUNT, \
				 CONFIG_BT_BUF_ACL_RX_COUNT), \
			     BT_BUF_ISO_RX_COUNT))

/** Data size needed for HCI Command buffers. */
#define BT_BUF_CMD_TX_SIZE BT_BUF_CMD_SIZE(CONFIG_BT_BUF_CMD_TX_SIZE)

/** Allocate a buffer for incoming data
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_recv_prio().
 *
 *  @param type    Type of buffer. Only BT_BUF_EVT, BT_BUF_ACL_IN and BT_BUF_ISO_IN
 *                 are allowed.
 *  @param timeout Non-negative waiting period to obtain a buffer or one of the
 *                 special values K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout);

/** Allocate a buffer for outgoing data
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_send().
 *
 *  @param type    Type of buffer. Only BT_BUF_CMD, BT_BUF_ACL_OUT or
 *                 BT_BUF_H4, when operating on H:4 mode, are allowed.
 *  @param timeout Non-negative waiting period to obtain a buffer or one of the
 *                 special values K_NO_WAIT and K_FOREVER.
 *  @param data    Initial data to append to buffer.
 *  @param size    Initial data size.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_tx(enum bt_buf_type type, k_timeout_t timeout,
			      const void *data, size_t size);

/** Allocate a buffer for an HCI Command Complete/Status Event
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_recv_prio().
 *
 *  @param timeout Non-negative waiting period to obtain a buffer or one of the
 *                 special values K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_cmd_complete(k_timeout_t timeout);

/** Allocate a buffer for an HCI Event
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called before bt_recv_prio() or bt_recv().
 *
 *  @param evt          HCI event code
 *  @param discardable  Whether the driver considers the event discardable.
 *  @param timeout      Non-negative waiting period to obtain a buffer or one of
 *                      the special values K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable, k_timeout_t timeout);

/** Set the buffer type
 *
 *  @param buf   Bluetooth buffer
 *  @param type  The BT_* type to set the buffer to
 */
static inline void bt_buf_set_type(struct net_buf *buf, enum bt_buf_type type)
{
	((struct bt_buf_data *)net_buf_user_data(buf))->type = type;
}

/** Get the buffer type
 *
 *  @param buf   Bluetooth buffer
 *
 *  @return The BT_* type to of the buffer
 */
static inline enum bt_buf_type bt_buf_get_type(struct net_buf *buf)
{
	return (enum bt_buf_type)((struct bt_buf_data *)net_buf_user_data(buf))
		->type;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BUF_H_ */

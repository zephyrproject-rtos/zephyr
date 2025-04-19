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

#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Possible types of buffers passed around the Bluetooth stack in a form of bitmask. */
enum __packed bt_buf_type {
	/** HCI command */
	BT_BUF_CMD = BT_HCI_H4_CMD,
	/** HCI event */
	BT_BUF_EVT = BT_HCI_H4_EVT,
	/** Outgoing ACL data */
	BT_BUF_ACL_OUT = BT_HCI_H4_ACL,
	/** Incoming ACL data */
	BT_BUF_ACL_IN = BT_HCI_H4_ACL,
	/** Outgoing ISO data */
	BT_BUF_ISO_OUT = BT_HCI_H4_ISO,
	/** Incoming ISO data */
	BT_BUF_ISO_IN = BT_HCI_H4_ISO,
};

/* Headroom reserved in buffers, primarily for HCI transport encoding purposes */
#define BT_BUF_RESERVE 1

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
					  BT_HCI_ISO_SDU_TS_HDR_SIZE + \
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

/* see Core Spec v6.0 vol.4 part E 7.4.5 */
#define BT_BUF_ACL_RX_COUNT_MAX 65535

#if defined(CONFIG_BT_CONN) && defined(CONFIG_BT_HCI_HOST)
 /* The host needs more ACL buffers than maximum ACL links. This is because of
  * the way we re-assemble ACL packets into L2CAP PDUs.
  *
  * We keep around the first buffer (that comes from the driver) to do
  * re-assembly into, and if all links are re-assembling, there will be no buffer
  * available for the HCI driver to allocate from.
  *
  * TODO: When CONFIG_BT_BUF_ACL_RX_COUNT is removed,
  *       remove the MAX and only keep the 1.
  */
#define BT_BUF_ACL_RX_COUNT_EXTRA CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA
#define BT_BUF_ACL_RX_COUNT       (MAX(CONFIG_BT_BUF_ACL_RX_COUNT, 1) + BT_BUF_ACL_RX_COUNT_EXTRA)
#else
#define BT_BUF_ACL_RX_COUNT_EXTRA 0
#define BT_BUF_ACL_RX_COUNT       0
#endif /* CONFIG_BT_CONN && CONFIG_BT_HCI_HOST */

#if defined(CONFIG_BT_BUF_ACL_RX_COUNT) && CONFIG_BT_BUF_ACL_RX_COUNT > 0
#warning "CONFIG_BT_BUF_ACL_RX_COUNT is deprecated, see Zephyr 4.1 migration guide"
#endif /* CONFIG_BT_BUF_ACL_RX_COUNT && CONFIG_BT_BUF_ACL_RX_COUNT > 0 */

BUILD_ASSERT(BT_BUF_ACL_RX_COUNT <= BT_BUF_ACL_RX_COUNT_MAX,
	     "Maximum number of ACL RX buffer is 65535, reduce CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA");

/** Data size needed for HCI ACL, HCI ISO or Event RX buffers */
#define BT_BUF_RX_SIZE (MAX(MAX(BT_BUF_ACL_RX_SIZE, BT_BUF_EVT_RX_SIZE), \
			    BT_BUF_ISO_RX_SIZE))

/* Controller can generate up to CONFIG_BT_BUF_ACL_TX_COUNT number of unique HCI Number of Completed
 * Packets events.
 */
BUILD_ASSERT(CONFIG_BT_BUF_EVT_RX_COUNT > CONFIG_BT_BUF_ACL_TX_COUNT,
	     "Increase Event RX buffer count to be greater than ACL TX buffer count");

/** Buffer count needed for HCI ACL or HCI ISO plus Event RX buffers */
#define BT_BUF_RX_COUNT (CONFIG_BT_BUF_EVT_RX_COUNT + \
			 MAX(BT_BUF_ACL_RX_COUNT, BT_BUF_ISO_RX_COUNT))

/** Data size needed for HCI Command buffers. */
#define BT_BUF_CMD_TX_SIZE BT_BUF_CMD_SIZE(CONFIG_BT_BUF_CMD_TX_SIZE)

/** Allocate a buffer for incoming data
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called.
 *
 *  @param type    Type of buffer. Only BT_BUF_EVT, BT_BUF_ACL_IN and BT_BUF_ISO_IN
 *                 are allowed.
 *  @param timeout Non-negative waiting period to obtain a buffer or one of the
 *                 special values K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout);

/** A callback to notify about freed buffer in the incoming data pool.
 *
 * This callback is called when a buffer of a given type is freed and can be requested through the
 * @ref bt_buf_get_rx function. However, this callback is called from the context of the buffer
 * freeing operation and must not attempt to allocate a new buffer from the same pool.
 *
 * @warning When this callback is called, the scheduler is locked and the callee must not perform
 * any action that makes the current thread unready. This callback must only be used for very
 * short non-blocking operation (e.g. submitting a work item).
 *
 * @param type_mask A bit mask of buffer types (BIT(type)) that have been freed.
 */
typedef void (*bt_buf_rx_freed_cb_t)(enum bt_buf_type type_mask);

/** Set the callback to notify about freed buffer in the incoming data pool.
 *
 * @param cb Callback to notify about freed buffer in the incoming data pool. If NULL, the callback
 *           is disabled.
 */
void bt_buf_rx_freed_cb_set(bt_buf_rx_freed_cb_t cb);

/** Allocate a buffer for outgoing data
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called.
 *
 *  @param type    Type of buffer. BT_BUF_CMD or BT_BUF_ACL_OUT.
 *  @param timeout Non-negative waiting period to obtain a buffer or one of the
 *                 special values K_NO_WAIT and K_FOREVER.
 *  @param data    Initial data to append to buffer.
 *  @param size    Initial data size.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_tx(enum bt_buf_type type, k_timeout_t timeout,
			      const void *data, size_t size);

/** Allocate a buffer for an HCI Event
 *
 *  This will set the buffer type so bt_buf_set_type() does not need to
 *  be explicitly called.
 *
 *  @param evt          HCI event code
 *  @param discardable  Whether the driver considers the event discardable.
 *  @param timeout      Non-negative waiting period to obtain a buffer or one of
 *                      the special values K_NO_WAIT and K_FOREVER.
 *  @return A new buffer.
 */
struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable, k_timeout_t timeout);

/** Set the buffer type. The type is encoded as an H:4 byte prefix as part of
 *  the payload itself.
 *
 *  @param buf   Bluetooth buffer
 *  @param type  The BT_* type to set the buffer to
 */
static inline void bt_buf_set_type(struct net_buf *buf, enum bt_buf_type type)
{
	__ASSERT_NO_MSG(net_buf_headroom(buf) >= 1);
	net_buf_push_u8(buf, type);
}

/** Get the buffer type. This pulls the H:4 byte prefix from the payload, which means
 *  that the call can be done only once per buffer.
 *
 *  @param buf   Bluetooth buffer
 *
 *  @return The BT_* type to of the buffer
 */
static inline enum bt_buf_type bt_buf_get_type(struct net_buf *buf)
{
	return net_buf_pull_u8(buf);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BUF_H_ */

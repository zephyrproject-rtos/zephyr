/* opp_internal.h - Internal definitions for Object Push Profile handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_OPP_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_OPP_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/classic/opp.h>

/* Internal flag bit index: a response PDU is being assembled / sent. */
#define BT_OPP_FLAG_RSP_ONGOING_BIT 0
/* Convenience mask for the flag above. */
#define BT_OPP_FLAG_RSP_ONGOING     BIT(BT_OPP_FLAG_RSP_ONGOING_BIT)

/**
 * @brief OPP session state enumeration.
 *
 * Tracks the OBEX session state independently of the transport state.
 */
enum __packed bt_opp_state {
	/** OBEX session is not established. */
	BT_OPP_STATE_DISCONNECTED = 0,
	/** OBEX CONNECT has been sent / received; awaiting response. */
	BT_OPP_STATE_CONNECTING   = 1,
	/** OBEX session is established and ready for operations. */
	BT_OPP_STATE_CONNECTED    = 2,
	/** OBEX DISCONNECT has been sent / received; awaiting response. */
	BT_OPP_STATE_DISCONNECTING = 3,
};

/**
 * @brief OPP transport layer state enumeration.
 *
 * Tracks the underlying RFCOMM or L2CAP transport connection state.
 */
enum __packed bt_opp_transport_state {
	/** Transport is not connected. */
	BT_OPP_TRANSPORT_STATE_DISCONNECTED  = 0,
	/** Transport connection is being established. */
	BT_OPP_TRANSPORT_STATE_CONNECTING    = 1,
	/** Transport is connected. */
	BT_OPP_TRANSPORT_STATE_CONNECTED     = 2,
	/** Transport is being disconnected. */
	BT_OPP_TRANSPORT_STATE_DISCONNECTING = 3,
};

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_CLASSIC_OPP_INTERNAL_H_ */

/* opp_internal.h - Internal definitions for Object Push Profile handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/classic/opp.h>

/**
 * @brief OPP OBEX connection state enumeration.
 *
 * Tracks the OBEX connection state independently of the transport state.
 */
enum __packed bt_opp_state {
	/** OBEX connection is not established. */
	BT_OPP_STATE_DISCONNECTED  = 0,
	/** OBEX CONNECT has been sent / received; awaiting response. */
	BT_OPP_STATE_CONNECTING    = 1,
	/** OBEX connection is established and ready for operations. */
	BT_OPP_STATE_CONNECTED     = 2,
	/** OBEX DISCONNECT has been sent / received; awaiting response. */
	BT_OPP_STATE_DISCONNECTING = 3,
};

/**
 * @brief OPP transport layer state enumeration.
 *
 * Tracks the underlying RFCOMM transport connection state.
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

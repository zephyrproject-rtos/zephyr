/* opp_internal.h - Internal definitions for Object Push Profile handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/classic/opp.h>

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

/**
 * @brief OPP push operation flags.
 *
 * A push operation spans one or more PUT requests. The client tracks only
 * BT_OPP_PUSH_ONGOING, to tell the first PUT from a continuation. The server
 * tracks the Name and Body headers it has received: IrOBEX 1.2 section 3.3.3.1
 * lets the headers describing the object arrive in any request before the end
 * of the body.
 */
enum {
	/** A push operation is in progress; the next PUT continues it. */
	BT_OPP_PUSH_ONGOING,
	/** Server only: the Name header of the object has been received. */
	BT_OPP_PUSH_NAME,
	/** Server only: a Body or End-of-Body header has been received. */
	BT_OPP_PUSH_BODY,
};

/**
 * @brief OPP pull operation flags.
 *
 * A pull operation spans one or more GET requests: the first carries the
 * headers that address the Default Get Object, a continuation carries none.
 * Both the client and the server track BT_OPP_PULL_ONGOING, to tell the first
 * GET from a continuation.
 */
enum {
	/** A pull operation is in progress; the next GET continues it. */
	BT_OPP_PULL_ONGOING,
};

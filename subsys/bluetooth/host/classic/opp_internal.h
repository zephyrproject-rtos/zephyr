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
	/** OBEX DISCONNECT is in progress. */
	BT_OPP_STATE_DISCONNECTING = 3,
	/** Server is sending a response that rejects an OBEX CONNECT request. */
	BT_OPP_STATE_RESPONDING    = 4,
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

/**
 * @brief OPP push operation flags.
 *
 * A push operation spans one or more PUT requests.
 *
 * The client only uses BT_OPP_PUSH_ONGOING, to tell the first PUT of a push
 * from a continuation: it requires the Name and Length headers in the first
 * PUT and tracks nothing else.
 *
 * The server tracks what the requests it receives have carried so far. Per
 * IrOBEX 1.2 section 3.3.3.1 the headers that describe the object only have to
 * precede the object body, so the server accepts them spread over the requests
 * of the operation. It requires the Name header before the object body and
 * refuses a push that ends without any object body; it does not require the
 * Length header.
 */
enum {
	/** A push operation is in progress; the next PUT continues it. */
	BT_OPP_PUSH_ONGOING,
	/** Server only: the Name header of the object has been received. */
	BT_OPP_PUSH_NAME,
	/** Server only: a Body or End-of-Body header has been received. */
	BT_OPP_PUSH_BODY,
};

/** @brief No push operation is in progress; the next PUT starts one. */
#define BT_OPP_PUSH_IDLE 0

/**
 * @brief OPP pull operation flags.
 *
 * A pull operation spans one or more GET requests: the first one carries the
 * headers that address the Default Get Object, a continuation request carries
 * no headers at all. Only the client tracks it, to tell the two apart when it
 * checks a request against OPP spec section 5.6.
 */
enum {
	/** A pull operation is in progress; the next GET continues it. */
	BT_OPP_PULL_ONGOING,
};

/** @brief No pull operation is in progress; the next GET starts one. */
#define BT_OPP_PULL_IDLE 0

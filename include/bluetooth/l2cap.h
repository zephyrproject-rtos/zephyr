/** @file
 *  @brief Bluetooth L2CAP handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_L2CAP_H
#define __BT_L2CAP_H

/**
 * @brief L2CAP
 * @defgroup bt_l2cap L2CAP
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <atomic.h>
#include <misc/nano_work.h>
#include <bluetooth/buf.h>
#include <bluetooth/conn.h>

struct bt_l2cap_chan;

/** @typedef bt_l2cap_chan_destroy_t
 *  @brief Channel destroy callback
 *
 *  @param chan Channel object.
 */
typedef void (*bt_l2cap_chan_destroy_t)(struct bt_l2cap_chan *chan);

/** @brief Life-span states of L2CAP CoC channel. Used only by internal APIs
 *  dealing with setting channel to proper state depending on operational
 *  context.
 */
typedef enum bt_l2cap_chan_state {
	/** Channel disconnected */
	BT_L2CAP_DISCONNECTED,
	/** Channel in connecting state */
	BT_L2CAP_CONNECT,
	/** Channel in config state, BR/EDR specific */
	BT_L2CAP_CONFIG,
	/** Channel ready for upper layer traffic on it */
	BT_L2CAP_CONNECTED,
	/** Channel in disconnecting state */
	BT_L2CAP_DISCONNECT,
} __packed bt_l2cap_chan_state_t;

/** @brief L2CAP Channel structure. */
struct bt_l2cap_chan {
	/** Channel connection reference */
	struct bt_conn			*conn;
	/** Channel operations reference */
	struct bt_l2cap_chan_ops	*ops;
	struct bt_l2cap_chan		*_next;
	bt_l2cap_chan_destroy_t		destroy;
	/* Response Timeout eXpired (RTX) timer */
	struct nano_delayed_work	rtx_work;
#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
	bt_l2cap_chan_state_t		state;
	/** Helps match request context during CoC */
	uint8_t				ident;
	bt_security_t			required_sec_level;
#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
};

/** @brief LE L2CAP Endpoint structure. */
struct bt_l2cap_le_endpoint {
	/** Endpoint CID */
	uint16_t			cid;
	/** Endpoint Maximum Transmission Unit */
	uint16_t			mtu;
	/** Endpoint Maximum PDU payload Size */
	uint16_t			mps;
	/** Endpoint credits */
	struct nano_sem			credits;
};

/** @brief LE L2CAP Channel structure. */
struct bt_l2cap_le_chan {
	/** Common L2CAP channel reference object */
	struct bt_l2cap_chan		chan;
	/** Channel Receiving Endpoint */
	struct bt_l2cap_le_endpoint	rx;
	/** Channel Transmission Endpoint */
	struct bt_l2cap_le_endpoint	tx;
	/** Segment SDU packet from upper layer */
	struct net_buf			*_sdu;
	uint16_t			_sdu_len;
};

/** @def BT_L2CAP_LE_CHAN(_ch)
 *  @brief Helper macro getting container object of type bt_l2cap_le_chan
 *  address having the same container chan member address as object in question.
 *
 *  @param _ch Address of object of bt_l2cap_chan type
 *
 *  @return Address of in memory bt_l2cap_le_chan object type containing
 *  the address of in question object.
 */
#define BT_L2CAP_LE_CHAN(_ch) CONTAINER_OF(_ch, struct bt_l2cap_le_chan, chan)

/** @brief BREDR L2CAP Endpoint structure. */
struct bt_l2cap_br_endpoint {
	/** Endpoint CID */
	uint16_t			cid;
	/** Endpoint Maximum Transmission Unit */
	uint16_t			mtu;
};

/** @brief BREDR L2CAP Channel structure. */
struct bt_l2cap_br_chan {
	/** Common L2CAP channel reference object */
	struct bt_l2cap_chan		chan;
	/** Channel Receiving Endpoint */
	struct bt_l2cap_br_endpoint	rx;
	/** Channel Transmission Endpoint */
	struct bt_l2cap_br_endpoint	tx;
	/* For internal use only */
	atomic_t			flags[1];
	/** Remote PSM to be connected */
	uint16_t			psm;
};

/** @brief L2CAP Channel operations structure. */
struct bt_l2cap_chan_ops {
	/** Channel connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param chan The channel that has been connected
	 */
	void (*connected)(struct bt_l2cap_chan *chan);

	/** Channel disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  channel is disconnected, including when a connection gets
	 *  rejected.
	 *
	 *  @param chan The channel that has been Disconnected
	 */
	void (*disconnected)(struct bt_l2cap_chan *chan);

	/** Channel encrypt_change callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  security level changed.
	 *
	 *  @param chan The channel which has encryption status changed.
	 */
	void (*encrypt_change)(struct bt_l2cap_chan *chan);

	/** Channel alloc_buf callback
	 *
	 *  If this callback is provided the channel will use it to allocate
	 *  buffers to store incoming data.
	 *
	 *  @param chan The channel requesting a buffer.
	 *
	 *  @return Allocated buffer.
	 */
	struct net_buf *(*alloc_buf)(struct bt_l2cap_chan *chan);

	/** Channel recv callback
	 *
	 *  @param chan The channel receiving data.
	 *  @param buf Buffer containing incoming data.
	 */
	void (*recv)(struct bt_l2cap_chan *chan, struct net_buf *buf);
};

/** @def BT_L2CAP_CHAN_SEND_RESERVE
 *  @brief Headroom needed for outgoing buffers
 */
#define BT_L2CAP_CHAN_SEND_RESERVE (CONFIG_BLUETOOTH_HCI_SEND_RESERVE + 4 + 4 \
				    + 2)

/** @brief L2CAP Server structure. */
struct bt_l2cap_server {
	/** Server PSM */
	uint16_t		psm;

	/** Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param chan Pointer to received the allocated channel
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_l2cap_chan **chan);

	struct bt_l2cap_server	*_next;
};

/** @brief Register L2CAP server.
 *
 *  Register L2CAP server for a PSM, each new connection is authorized using
 *  the accept() callback which in case of success shall allocate the channel
 *  structure to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_server_register(struct bt_l2cap_server *server);

/** @brief Register L2CAP server on BR/EDR oriented connection.
 *
 *  Register L2CAP server for a PSM, each new connection is authorized using
 *  the accept() callback which in case of success shall allocate the channel
 *  structure to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_br_server_register(struct bt_l2cap_server *server);

/** @brief Connect L2CAP channel
 *
 *  Connect L2CAP channel by PSM, once the connection is completed channel
 *  connected() callback will be called. If the connection is rejected
 *  disconnected() callback is called instead.
 *  Channel object passed (over an address of it) as second parameter shouldn't
 *  be instantiated in application as standalone. Instead of, application should
 *  create transport dedicated L2CAP objects, i.e. type of bt_l2cap_le_chan for
 *  LE and/or type of bt_l2cap_br_chan for BR/EDR. Then pass to this API
 *  the location (address) of bt_l2cap_chan type object which is a member
 *  of both transport dedicated objects.
 *
 *  @param conn Connection object.
 *  @param chan Channel object.
 *  @param psm Channel PSM to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_chan_connect(struct bt_conn *conn, struct bt_l2cap_chan *chan,
			  uint16_t psm);

/** @brief Disconnect L2CAP channel
 *
 *  Disconnect L2CAP channel, if the connection is pending it will be
 *  canceled and as a result the channel disconnected() callback is called.
 *  Regarding to input parameter, to get details see reference description
 *  to bt_l2cap_chan_connect() API above.
 *
 *  @param chan Channel object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_chan_disconnect(struct bt_l2cap_chan *chan);

/** @brief Send data to L2CAP channel
 *
 *  Send data from buffer to the channel. This procedure may block waiting for
 *  credits to send data therefore it shall be used from a fiber to be able to
 *  receive credits when necessary.
 *  Regarding to first input parameter, to get details see reference description
 *  to bt_l2cap_chan_connect() API above.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_L2CAP_H */

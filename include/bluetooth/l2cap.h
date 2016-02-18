/** @file
 *  @brief Bluetooth L2CAP handling
 */

/*
 * Copyright (c) 2015 Intel Corporation
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

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BLUETOOTH_CENTRAL) || defined(CONFIG_BLUETOOTH_PERIPHERAL)
#include <net/buf.h>
#include <bluetooth/conn.h>

/** @brief L2CAP Endpoint structure. */
struct bt_l2cap_endpoint {
	/** Endpoint CID */
	uint16_t		cid;
	/** Endpoint Maximum PDU payload Size */
	uint16_t		mps;
	/** Endpoint Maximum Transmission Unit */
	uint16_t		mtu;
	/** Endpoint credits */
	struct nano_sem		credits;
};

/** @brief L2CAP Channel structure. */
struct bt_l2cap_chan {
	/** Channel connection reference */
	struct bt_conn		*conn;

	/** Channel operations reference */
	struct bt_l2cap_chan_ops *ops;

	/** Channel Transmission Endpoint */
	struct bt_l2cap_endpoint tx;
	/** Channel Receiving Endpoint */
	struct bt_l2cap_endpoint rx;

	struct net_buf		*_sdu;
	uint16_t		_sdu_len;

	uint8_t			_ident;

	struct bt_l2cap_chan	*_next;
};

/** @brief L2CAP Channel operations structure. */
struct bt_l2cap_chan_ops {
	/** Channel connected callback
	 *
	 *  @param chan The channel that has been connected
	 */
	void			(*connected)(struct bt_l2cap_chan *chan);

	/** Channel disconnected callback
	 *
	 *  @param chan The channel that has been Disconnected
	 */
	void			(*disconnected)(struct bt_l2cap_chan *chan);

	/** Channel encrypt_change callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  security level changed.
	 *
	 *  @param chan The channel which has encryption status changed.
	 */
	void			(*encrypt_change)(struct bt_l2cap_chan *chan);

	/** Channel alloc_buf callback
	 *
	 *  If this callback is provided the channel will use it to allocate
	 *  buffers to store incoming data.
	 *
	 *  @param chan The channel requesting a buffer.
	 *
	 *  @return Allocated buffer.
	 */
	struct net_buf		*(*alloc_buf)(struct bt_l2cap_chan *chan);

	/** Channel recv callback
	 *
	 *  @param chan The channel receiving data.
	 *  @param buf Buffer containing incoming data.
	 */
	void			(*recv)(struct bt_l2cap_chan *chan,
					struct net_buf *buf);
};

#if defined(CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL)
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
 *  Register L2CAP server for a PSM.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_server_register(struct bt_l2cap_server *server);

/** @brief Connect L2CAP channel
 *
 *  Connect L2CAP channel by PSM.
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
 *  canceled.
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
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_l2cap_chan_send(struct bt_l2cap_chan *chan, struct net_buf *buf);

#endif /* CONFIG_BLUETOOTH_L2CAP_DYNAMIC_CHANNEL */
#endif /* CONFIG_BLUETOOTH_CENTRAL || CONFIG_BLUETOOTH_PERIPHERAL */

#ifdef __cplusplus
}
#endif

#endif /* __BT_L2CAP_H */

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

#if defined(CONFIG_BLUETOOTH_CENTRAL) || defined(CONFIG_BLUETOOTH_PERIPHERAL)
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
	uint16_t		credits;
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

	struct bt_l2cap_chan	*_next;
};

/** @brief L2CAP Channel operations structure. */
struct bt_l2cap_chan_ops {
	/** Channel connected callback */
	void			(*connected)(struct bt_l2cap_chan *chan);
	/** Channel disconnected callback */
	void			(*disconnected)(struct bt_l2cap_chan *chan);
	/** Channel encrypt_change callback */
	void			(*encrypt_change)(struct bt_l2cap_chan *chan);

	/** Channel recv callback */
	void			(*recv)(struct bt_l2cap_chan *chan,
					struct bt_buf *buf);
};

/** @brief L2CAP Server structure. */
struct bt_l2cap_server {
	/** Server PSM */
	uint16_t		psm;

	/** Server accept callack */
	int (*accept)(struct bt_conn *conn, struct bt_l2cap_chan **chan);

	struct bt_l2cap_server	*_next;
};

/** @brief Register attribute database.
 *
 *  Register L2CAP server for a PSM.
 *
 *  @param server Server structure.
 *
 * @return 0 in case of success or negative value in case of error.
 */
int bt_l2cap_server_register(struct bt_l2cap_server *server);

#endif /* CONFIG_BLUETOOTH_CENTRAL || CONFIG_BLUETOOTH_PERIPHERAL */
#endif /* __BT_L2CAP_H */

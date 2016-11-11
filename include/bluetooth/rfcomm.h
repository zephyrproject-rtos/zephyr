/** @file
 *  @brief Bluetooth RFCOMM handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_RFCOMM_H
#define __BT_RFCOMM_H

/**
 * @brief RFCOMM
 * @defgroup bt_rfcomm RFCOMM
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/log.h>
#include <bluetooth/buf.h>
#include <bluetooth/conn.h>

/* RFCOMM channels (1-31): pre-allocated for profiles to avoid conflicts */
enum {
	BT_RFCOMM_CHAN_HFP_HF = 1,
	BT_RFCOMM_CHAN_HFP_AG,
	BT_RFCOMM_CHAN_HSP_AG,
	BT_RFCOMM_CHAN_HSP_HS,
	BT_RFCOMM_CHAN_SPP,
};

struct bt_rfcomm_dlc;

/** @brief RFCOMM DLC operations structure. */
struct bt_rfcomm_dlc_ops {
	/** DLC connected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  connection completes.
	 *
	 *  @param dlc The dlc that has been connected
	 */
	void (*connected)(struct bt_rfcomm_dlc *dlc);

	/** DLC disconnected callback
	 *
	 *  If this callback is provided it will be called whenever the
	 *  dlc is disconnected, including when a connection gets
	 *  rejected or cancelled (both incoming and outgoing)
	 *
	 *  @param dlc The dlc that has been Disconnected
	 */
	void (*disconnected)(struct bt_rfcomm_dlc *dlc);

	/** DLC recv callback
	 *
	 *  @param dlc The dlc receiving data.
	 *  @param buf Buffer containing incoming data.
	 */
	void (*recv)(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);
};

/** @brief Role of RFCOMM session and dlc. Used only by internal APIs
 */
typedef enum bt_rfcomm_role {
	BT_RFCOMM_ROLE_ACCEPTOR,
	BT_RFCOMM_ROLE_INITIATOR
} __packed bt_rfcomm_role_t;

/** @brief RFCOMM DLC structure. */
struct bt_rfcomm_dlc {
	/* Queue for outgoing data */
	struct k_fifo              tx_queue;

	/** TX credits */
	struct k_sem               tx_credits;

	struct bt_rfcomm_session  *session;
	struct bt_rfcomm_dlc_ops  *ops;
	struct bt_rfcomm_dlc      *_next;

	bt_security_t              required_sec_level;
	bt_rfcomm_role_t           role;

	uint16_t                   mtu;
	uint8_t                    dlci;
	uint8_t                    state;
	uint8_t                    rx_credit;

	/* Stack for TX fiber */
	BT_STACK(stack, 128);
};

struct bt_rfcomm_server {
	/** Server Channel */
	uint8_t channel;

	/** Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param dlc Pointer to received the allocated dlc
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_rfcomm_dlc **dlc);

	struct bt_rfcomm_server	*_next;
};

/** @brief Register RFCOMM server
 *
 *  Register RFCOMM server for a channel, each new connection is authorized
 *  using the accept() callback which in case of success shall allocate the dlc
 *  structure to be used by the new connection.
 *
 *  @param server Server structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_server_register(struct bt_rfcomm_server *server);

/** @brief Connect RFCOMM channel
 *
 *  Connect RFCOMM dlc by channel, once the connection is completed dlc
 *  connected() callback will be called. If the connection is rejected
 *  disconnected() callback is called instead.
 *
 *  @param conn Connection object.
 *  @param dlc Dlc object.
 *  @param channel Server channel to connect to.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_connect(struct bt_conn *conn, struct bt_rfcomm_dlc *dlc,
			  uint8_t channel);

/** @brief Send data to RFCOMM
 *
 *  Send data from buffer to the dlc. Length should be less than or equal to
 *  mtu.
 *
 *  @param dlc Dlc object.
 *  @param buf Data buffer.
 *
 *  @return Bytes sent in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_send(struct bt_rfcomm_dlc *dlc, struct net_buf *buf);

/** @brief Get the buffer from fifo after reserving head room for RFCOMM, L2CAP
 *  and ACL headers.
 *
 *  @param fifo Which FIFO to take the buffer from.
 *
 *  @return New buffer.
 */
struct net_buf *bt_rfcomm_create_pdu(struct k_fifo *fifo);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_RFCOMM_H */

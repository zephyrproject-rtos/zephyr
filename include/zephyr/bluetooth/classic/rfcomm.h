/** @file
 *  @brief Bluetooth RFCOMM handling
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_

/**
 * @brief RFCOMM
 * @defgroup bt_rfcomm RFCOMM
 * @ingroup bluetooth
 * @{
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#ifdef __cplusplus
extern "C" {
#endif

/** RFCOMM Maximum Header Size. The length could be 2 bytes, it depends on information length. */
#define BT_RFCOMM_HDR_MAX_SIZE 4
/** RFCOMM FCS Size */
#define BT_RFCOMM_FCS_SIZE     1

/** @brief Helper to calculate needed buffer size for RFCOMM PDUs.
 *         Useful for creating buffer pools.
 *
 *  @param mtu Needed RFCOMM PDU MTU.
 *
 *  @return Needed buffer size to match the requested RFCOMM PDU MTU.
 */
#define BT_RFCOMM_BUF_SIZE(mtu)                                                                    \
	BT_L2CAP_BUF_SIZE(BT_RFCOMM_HDR_MAX_SIZE + BT_RFCOMM_FCS_SIZE + (mtu))

/* RFCOMM channels (1-30): pre-allocated for profiles to avoid conflicts */
enum {
	BT_RFCOMM_CHAN_HFP_HF = 1,
	BT_RFCOMM_CHAN_HFP_AG,
	BT_RFCOMM_CHAN_HSP_AG,
	BT_RFCOMM_CHAN_HSP_HS,
	BT_RFCOMM_CHAN_SPP,
	BT_RFCOMM_CHAN_DYNAMIC_START,
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

	/** DLC sent callback
	 *
	 *  @param dlc The dlc which has sent data.
	 *  @param err Sent result.
	 */
	void (*sent)(struct bt_rfcomm_dlc *dlc, int err);
};

/** @brief Role of RFCOMM session and dlc. Used only by internal APIs
 */
typedef enum bt_rfcomm_role {
	BT_RFCOMM_ROLE_ACCEPTOR,
	BT_RFCOMM_ROLE_INITIATOR
} __packed bt_rfcomm_role_t;

/** @brief RFCOMM DLC structure. */
struct bt_rfcomm_dlc {
	/* Response Timeout eXpired (RTX) timer */
	struct k_work_delayable    rtx_work;

	/* Queue for outgoing data */
	struct k_fifo              tx_queue;

	/* TX credits, Reuse as a binary sem for MSC FC if CFC is not enabled */
	struct k_sem               tx_credits;

	/* Worker for RFCOMM TX */
	struct k_work              tx_work;

	struct bt_rfcomm_session  *session;
	struct bt_rfcomm_dlc_ops  *ops;
	struct bt_rfcomm_dlc      *_next;

	bt_security_t              required_sec_level;
	bt_rfcomm_role_t           role;

	uint16_t                   mtu;
	uint8_t                    dlci;
	uint8_t                    state;
	uint8_t                    rx_credit;
};

struct bt_rfcomm_server {
	/** Server Channel
	 *
	 *  Possible values:
	 *  0           A dynamic value will be auto-allocated when bt_rfcomm_server_register() is
	 *              called.
	 *
	 *  0x01 - 0x1e Dynamically allocated. May be pre-set by the application before server
	 *              registration (not recommended however), or auto-allocated by the stack
	 *              if the 0 is passed.
	 */
	uint8_t channel;

	/** Server accept callback
	 *
	 *  This callback is called whenever a new incoming connection requires
	 *  authorization.
	 *
	 *  @param conn The connection that is requesting authorization
	 *  @param server Pointer to the server structure this callback relates to
	 *  @param dlc Pointer to received the allocated dlc
	 *
	 *  @return 0 in case of success or negative value in case of error.
	 */
	int (*accept)(struct bt_conn *conn, struct bt_rfcomm_server *server,
		      struct bt_rfcomm_dlc **dlc);

	struct bt_rfcomm_server	*_next;
};

/** @brief RFCOMM RPN baud rate values */
enum {
	BT_RFCOMM_RPN_BAUD_RATE_2400 = 0x0,
	BT_RFCOMM_RPN_BAUD_RATE_4800 = 0x1,
	BT_RFCOMM_RPN_BAUD_RATE_7200 = 0x2,
	BT_RFCOMM_RPN_BAUD_RATE_9600 = 0x3,
	BT_RFCOMM_RPN_BAUD_RATE_19200 = 0x4,
	BT_RFCOMM_RPN_BAUD_RATE_38400 = 0x5,
	BT_RFCOMM_RPN_BAUD_RATE_57600 = 0x6,
	BT_RFCOMM_RPN_BAUD_RATE_115200 = 0x7,
	BT_RFCOMM_RPN_BAUD_RATE_230400 = 0x8
};

/** @brief RFCOMM RPN data bit values */
enum {
	BT_RFCOMM_RPN_DATA_BITS_5 = 0x0,
	BT_RFCOMM_RPN_DATA_BITS_6 = 0x1,
	BT_RFCOMM_RPN_DATA_BITS_7 = 0x2,
	BT_RFCOMM_RPN_DATA_BITS_8 = 0x3
};

/** @brief RFCOMM RPN stop bit values */
enum {
	BT_RFCOMM_RPN_STOP_BITS_1 = 0,
	BT_RFCOMM_RPN_STOP_BITS_1_5 = 1
};

/** @brief RFCOMM RPN parity bit values */
enum {
	BT_RFCOMM_RPN_PARITY_NONE = 0x0,
	BT_RFCOMM_RPN_PARITY_ODD = 0x1,
	BT_RFCOMM_RPN_PARITY_EVEN = 0x3,
	BT_RFCOMM_RPN_PARITY_MARK = 0x5,
	BT_RFCOMM_RPN_PARITY_SPACE = 0x7
};

/** @brief Combine data bits, stop bits and parity into a single line settings byte
 *
 *  @param data Data bits value (0-3)
 *  @param stop Stop bits value (0-1)
 *  @param parity Parity value (0-7)
 *
 *  @return Combined line settings byte
 */
#define BT_RFCOMM_SET_LINE_SETTINGS(data, stop, parity) ((data & 0x3) | \
							 ((stop & 0x1) << 2) | \
							 ((parity & 0x7) << 3))

#define BT_RFCOMM_RPN_FLOW_NONE         0x00
#define BT_RFCOMM_RPN_XON_CHAR          0x11
#define BT_RFCOMM_RPN_XOFF_CHAR         0x13

/* Set 1 to all the param mask except reserved */
#define BT_RFCOMM_RPN_PARAM_MASK_ALL    0x3f7f

/** @brief RFCOMM Remote Port Negotiation (RPN) structure */
struct bt_rfcomm_rpn {
	uint8_t  dlci;
	uint8_t  baud_rate;
	uint8_t  line_settings;
	uint8_t  flow_control;
	uint8_t  xon_char;
	uint8_t  xoff_char;
	uint16_t param_mask;
} __packed;

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

/** @brief Disconnect RFCOMM dlc
 *
 *  Disconnect RFCOMM dlc, if the connection is pending it will be
 *  canceled and as a result the dlc disconnected() callback is called.
 *
 *  @param dlc Dlc object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_rfcomm_dlc_disconnect(struct bt_rfcomm_dlc *dlc);

/** @brief Allocate the buffer from pool after reserving head room for RFCOMM,
 *  L2CAP and ACL headers.
 *
 *  @param pool Which pool to take the buffer from.
 *
 *  @return New buffer.
 */
struct net_buf *bt_rfcomm_create_pdu(struct net_buf_pool *pool);

/**
 * @brief Send Remote Port Negotiation command
 *
 * @param dlc Pointer to the RFCOMM DLC
 * @param rpn Pointer to the RPN parameters to send
 *
 * @return 0 on success, negative error code on failure
 */
int bt_rfcomm_send_rpn_cmd(struct bt_rfcomm_dlc *dlc, struct bt_rfcomm_rpn *rpn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_RFCOMM_H_ */

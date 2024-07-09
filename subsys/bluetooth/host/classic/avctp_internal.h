/** @file
 *  @brief Audio Video Control Transport Protocol internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_L2CAP_PSM_AVCTP 0x0017

struct bt_avctp;

struct bt_avctp_ops_cb {
	void (*connected)(struct bt_avctp *session);
	void (*disconnected)(struct bt_avctp *session);
};

struct bt_avctp {
	struct bt_l2cap_br_chan br_chan;
	const struct bt_avctp_ops_cb *ops;
};

struct bt_avctp_event_cb {
	int (*accept)(struct bt_conn *conn, struct bt_avctp **session);
};

/* Initialize AVCTP layer*/
int bt_avctp_init(void);

/* Application register with AVCTP layer */
int bt_avctp_register(const struct bt_avctp_event_cb *cb);

/* AVCTP connect */
int bt_avctp_connect(struct bt_conn *conn, struct bt_avctp *session);

/* AVCTP disconnect */
int bt_avctp_disconnect(struct bt_avctp *session);

/** @file
 *  @brief Internal APIs for Bluetooth L2CAP BR/EDR handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/iterable_sections.h>
#include "l2cap_br_interface.h"

#define BT_L2CAP_CID_BR_SIG             0x0001
#define BT_L2CAP_CID_BR_SMP             0x0007
#define BT_L2CAP_PSM_RFCOMM             0x0003

struct bt_l2cap_hdr {
	uint16_t len;
	uint16_t cid;
} __packed;

struct bt_l2cap_sig_hdr {
	uint8_t  code;
	uint8_t  ident;
	uint16_t len;
} __packed;

#define BT_L2CAP_REJ_NOT_UNDERSTOOD     0x0000
#define BT_L2CAP_REJ_MTU_EXCEEDED       0x0001
#define BT_L2CAP_REJ_INVALID_CID        0x0002

#define BT_L2CAP_CMD_REJECT             0x01
struct bt_l2cap_cmd_reject {
	uint16_t reason;
	uint8_t  data[0];
} __packed;

struct bt_l2cap_cmd_reject_cid_data {
	uint16_t scid;
	uint16_t dcid;
} __packed;

#define BT_L2CAP_CONN_REQ               0x02
struct bt_l2cap_conn_req {
	uint16_t psm;
	uint16_t scid;
} __packed;

/* command statuses in response */
#define BT_L2CAP_CS_NO_INFO             0x0000
#define BT_L2CAP_CS_AUTHEN_PEND         0x0001

/* valid results in conn response on BR/EDR */
#define BT_L2CAP_BR_SUCCESS             0x0000
#define BT_L2CAP_BR_PENDING             0x0001
#define BT_L2CAP_BR_ERR_PSM_NOT_SUPP    0x0002
#define BT_L2CAP_BR_ERR_SEC_BLOCK       0x0003
#define BT_L2CAP_BR_ERR_NO_RESOURCES    0x0004
#define BT_L2CAP_BR_ERR_INVALID_SCID    0x0006
#define BT_L2CAP_BR_ERR_SCID_IN_USE     0x0007

#define BT_L2CAP_CONN_RSP               0x03
struct bt_l2cap_conn_rsp {
	uint16_t dcid;
	uint16_t scid;
	uint16_t result;
	uint16_t status;
} __packed;

#define BT_L2CAP_CONF_SUCCESS           0x0000
#define BT_L2CAP_CONF_UNACCEPT          0x0001
#define BT_L2CAP_CONF_REJECT            0x0002

#define BT_L2CAP_CONF_REQ               0x04
struct bt_l2cap_conf_req {
	uint16_t dcid;
	uint16_t flags;
	uint8_t  data[0];
} __packed;

#define BT_L2CAP_CONF_RSP               0x05
struct bt_l2cap_conf_rsp {
	uint16_t scid;
	uint16_t flags;
	uint16_t result;
	uint8_t  data[0];
} __packed;

/* Option type used by MTU config request data */
#define BT_L2CAP_CONF_OPT_MTU           0x01
/* Options bits selecting most significant bit (hint) in type field */
#define BT_L2CAP_CONF_HINT              0x80
#define BT_L2CAP_CONF_MASK              0x7f

struct bt_l2cap_conf_opt {
	uint8_t type;
	uint8_t len;
	uint8_t data[0];
} __packed;

#define BT_L2CAP_DISCONN_REQ            0x06
struct bt_l2cap_disconn_req {
	uint16_t dcid;
	uint16_t scid;
} __packed;

#define BT_L2CAP_DISCONN_RSP            0x07
struct bt_l2cap_disconn_rsp {
	uint16_t dcid;
	uint16_t scid;
} __packed;

#define BT_L2CAP_INFO_FEAT_MASK         0x0002
#define BT_L2CAP_INFO_FIXED_CHAN        0x0003

#define BT_L2CAP_INFO_REQ               0x0a
struct bt_l2cap_info_req {
	uint16_t type;
} __packed;

/* info result */
#define BT_L2CAP_INFO_SUCCESS           0x0000
#define BT_L2CAP_INFO_NOTSUPP           0x0001

#define BT_L2CAP_INFO_RSP               0x0b
struct bt_l2cap_info_rsp {
	uint16_t type;
	uint16_t result;
	uint8_t  data[0];
} __packed;

/* Need a name different than bt_l2cap_fixed_chan for a different section */
struct bt_l2cap_br_fixed_chan {
	uint16_t		cid;
	int (*accept)(struct bt_conn *conn, struct bt_l2cap_chan **chan);
};

#define BT_L2CAP_BR_CHANNEL_DEFINE(_name, _cid, _accept)		\
	const STRUCT_SECTION_ITERABLE(bt_l2cap_br_fixed_chan, _name) = { \
				.cid = _cid,			\
				.accept = _accept,		\
			}

#define BR_CHAN(_ch) CONTAINER_OF(_ch, struct bt_l2cap_br_chan, chan)

/* Add channel to the connection */
void bt_l2cap_chan_add(struct bt_conn *conn, struct bt_l2cap_chan *chan,
		       bt_l2cap_chan_destroy_t destroy);

/* Remove channel from the connection */
void bt_l2cap_chan_remove(struct bt_conn *conn, struct bt_l2cap_chan *chan);

/* Delete channel */
void bt_l2cap_br_chan_del(struct bt_l2cap_chan *chan);

const char *bt_l2cap_chan_state_str(bt_l2cap_chan_state_t state);

#if defined(CONFIG_BT_L2CAP_LOG_LEVEL_DBG)
void bt_l2cap_br_chan_set_state_debug(struct bt_l2cap_chan *chan,
				   bt_l2cap_chan_state_t state,
				   const char *func, int line);
#define bt_l2cap_br_chan_set_state(_chan, _state) \
	bt_l2cap_br_chan_set_state_debug(_chan, _state, __func__, __LINE__)
#else
void bt_l2cap_br_chan_set_state(struct bt_l2cap_chan *chan,
			     bt_l2cap_chan_state_t state);
#endif /* CONFIG_BT_L2CAP_LOG_LEVEL_DBG */

/* Prepare an L2CAP PDU to be sent over a connection */
struct net_buf *bt_l2cap_create_pdu_timeout(struct net_buf_pool *pool,
					    size_t reserve,
					    k_timeout_t timeout);

#define bt_l2cap_create_pdu(_pool, _reserve) \
	bt_l2cap_create_pdu_timeout(_pool, _reserve, K_FOREVER)

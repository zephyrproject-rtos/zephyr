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
#define BT_L2CAP_CID_CLS                0x0002
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

struct bt_l2cap_cls_hdr {
	uint16_t psm;
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
#define BT_L2CAP_CONF_UNKNOWN_OPT       0x0003
#define BT_L2CAP_CONF_PENDING           0x0004
#define BT_L2CAP_CONF_FLOW_SPEC_REJECT  0x0005

#define BT_L2CAP_CONF_FLAGS_C           BIT(0)
#define BT_L2CAP_CONF_FLAGS_MASK        BT_L2CAP_CONF_FLAGS_C

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
#define BT_L2CAP_CONF_OPT_FLUSH_TIMEOUT 0x02
#define BT_L2CAP_CONF_OPT_QOS           0x03
#define BT_L2CAP_CONF_OPT_RET_FC        0x04
#define BT_L2CAP_CONF_OPT_FCS           0x05
#define BT_L2CAP_CONF_OPT_EXT_FLOW_SPEC 0x06
#define BT_L2CAP_CONF_OPT_EXT_WIN_SIZE  0x07

/* Options bits selecting most significant bit (hint) in type field */
#define BT_L2CAP_CONF_HINT              0x80
#define BT_L2CAP_CONF_MASK              0x7f

struct bt_l2cap_conf_opt {
	uint8_t type;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct bt_l2cap_conf_opt_mtu {
	uint16_t mtu;
} __packed;

struct bt_l2cap_conf_opt_flush_timeout {
	uint16_t timeout;
} __packed;

#define BT_L2CAP_QOS_TYPE_NO_TRAFFIC    0x00
#define BT_L2CAP_QOS_TYPE_BEST_EFFORT   0x01
#define BT_L2CAP_QOS_TYPE_GUARANTEED    0x02
struct bt_l2cap_conf_opt_qos {
	uint8_t flags;
	uint8_t service_type;
	uint32_t token_rate;
	uint32_t token_bucket_size;
	uint32_t peak_bandwidth;
	uint32_t latency;
	uint32_t delay_variation;
} __packed;

struct bt_l2cap_conf_opt_ret_fc {
	uint8_t mode;
	uint8_t tx_windows_size;
	uint8_t max_transmit;
	uint16_t retransmission_timeout;
	uint16_t monitor_timeout;
	uint16_t mps;
} __packed;

#define BT_L2CAP_FCS_TYPE_NO         0x00
#define BT_L2CAP_FCS_TYPE_16BIT      0x01
struct bt_l2cap_conf_opt_fcs {
	uint8_t type;
} __packed;

struct bt_l2cap_conf_opt_ext_flow_spec {
	uint8_t identifier;
	uint8_t service_type;
	uint16_t sdu;
	uint32_t sdu_inter_arrival_time;
	uint32_t access_latency;
	uint32_t flush_timeout;
} __packed;

struct bt_l2cap_conf_opt_ext_win_size {
	uint16_t max_windows_size;
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

#define BT_L2CAP_ECHO_REQ               0x08
struct bt_l2cap_echo_req {
	uint16_t data[0];
} __packed;

#define BT_L2CAP_ECHO_RSP               0x09
struct bt_l2cap_echo_rsp {
	uint16_t data[0];
} __packed;

#define BT_L2CAP_INFO_CLS_MTU_MASK      0x0001
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

/* I Frame Standard Control Field Format definition */
#define BT_L2CAP_I_FRAME_STD_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_I_FRAME_STD_CONTROL_GET_TX_SEQ(control)  (((control) >> 0x01) & 0x3f)
#define BT_L2CAP_I_FRAME_STD_CONTROL_GET_R(control)       (((control) >> 0x07) & 0x01)
#define BT_L2CAP_I_FRAME_STD_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x08) & 0x3f)
#define BT_L2CAP_I_FRAME_STD_CONTROL_GET_SAR(control)     (((control) >> 0x0e) & 0x03)

#define BT_L2CAP_I_FRAME_STD_CONTROL_SET(tx_seq, r, req_seq, sar)                              \
	((((tx_seq) & 0x3f) << 0x01) | (((r) & 0x01) << 0x07) | (((req_seq) & 0x3f) << 0x08) |     \
	 (((sar) & 0x03) << 0x0e))

/* I Frame Enhanced Control Field Format definition */
#define BT_L2CAP_I_FRAME_ENH_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_I_FRAME_ENH_CONTROL_GET_TX_SEQ(control)  (((control) >> 0x01) & 0x3f)
#define BT_L2CAP_I_FRAME_ENH_CONTROL_GET_F(control)       (((control) >> 0x07) & 0x01)
#define BT_L2CAP_I_FRAME_ENH_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x08) & 0x3f)
#define BT_L2CAP_I_FRAME_ENH_CONTROL_GET_SAR(control)     (((control) >> 0x0e) & 0x03)

#define BT_L2CAP_I_FRAME_ENH_CONTROL_SET(tx_seq, f, req_seq, sar)                              \
	((((tx_seq) & 0x3f) << 0x01) | (((f) & 0x01) << 0x07) | (((req_seq) & 0x3f) << 0x08) |     \
	 (((sar) & 0x03) << 0x0e))

/* I Frame Extended Control Field Format definition */
#define BT_L2CAP_I_FRAME_EXT_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_I_FRAME_EXT_CONTROL_GET_F(control)       (((control) >> 0x01) & 0x01)
#define BT_L2CAP_I_FRAME_EXT_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x02) & 0x3fff)
#define BT_L2CAP_I_FRAME_EXT_CONTROL_GET_SAR(control)     (((control) >> 0x10) & 0x03)
#define BT_L2CAP_I_FRAME_EXT_CONTROL_GET_TX_SEQ(control)  (((control) >> 0x12) & 0x3fff)

#define BT_L2CAP_I_FRAME_EXT_CONTROL_SET(f, tx_seq, sar, req_seq)                              \
	((((f) & 0x01) << 0x01) | (((req_seq) & 0x3fff) << 0x02) | (((sar) & 0x03) << 0x10) |      \
	 (((tx_seq) & 0x3fff) << 0x12))

/* S Frame Standard Control Field Format definition */
#define BT_L2CAP_S_FRAME_STD_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_S_FRAME_STD_CONTROL_GET_S(control)       (((control) >> 0x02) & 0x03)
#define BT_L2CAP_S_FRAME_STD_CONTROL_GET_R(control)       (((control) >> 0x07) & 0x01)
#define BT_L2CAP_S_FRAME_STD_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x08) & 0x3f)

#define BT_L2CAP_S_FRAME_STD_CONTROL_SET(s, r, req_seq)                                        \
	(((1) & 0x01) | (((s) & 0x03) << 0x02) | (((r) & 0x01) << 0x07) |                          \
	 (((req_seq) & 0x3f) << 0x08))

/* S Frame Enhanced Control Field Format definition */
#define BT_L2CAP_S_FRAME_ENH_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_S_FRAME_ENH_CONTROL_GET_S(control)       (((control) >> 0x02) & 0x03)
#define BT_L2CAP_S_FRAME_ENH_CONTROL_GET_P(control)       (((control) >> 0x04) & 0x01)
#define BT_L2CAP_S_FRAME_ENH_CONTROL_GET_F(control)       (((control) >> 0x07) & 0x01)
#define BT_L2CAP_S_FRAME_ENH_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x08) & 0x3f)

#define BT_L2CAP_S_FRAME_ENH_CONTROL_SET(s, p, f, req_seq)                                     \
	(((1) & 0x01) | (((s) & 0x03) << 0x02) | (((p) & 0x01) << 0x04) | (((f) & 0x01) << 0x07) | \
	 (((req_seq) & 0x3f) << 0x08))

/* S Frame Extended Control Field Format definition */
#define BT_L2CAP_S_FRAME_EXT_CONTROL_GET_TYPE(control)    ((control) & 0x01)
#define BT_L2CAP_S_FRAME_EXT_CONTROL_GET_F(control)       (((control) >> 0x01) & 0x01)
#define BT_L2CAP_S_FRAME_EXT_CONTROL_GET_REQ_SEQ(control) (((control) >> 0x02) & 0x3fff)
#define BT_L2CAP_S_FRAME_EXT_CONTROL_GET_S(control)       (((control) >> 0x10) & 0x03)
#define BT_L2CAP_S_FRAME_EXT_CONTROL_GET_P(control)       (((control) >> 0x12) & 0x01)

#define BT_L2CAP_S_FRAME_EXT_CONTROL_SET(f, req_seq, s, p)                                     \
	(((1) & 0x01) | (((f) & 0x01) << 0x01) | (((req_seq) & 0x3fff) << 0x02) |                  \
	 (((s) & 0x03) << 0x10) | (((p) & 0x01) << 0x12))

#define BT_L2CAP_CONTROL_TYPE_I 0x00
#define BT_L2CAP_CONTROL_TYPE_S 0x01

#define BT_L2CAP_CONTROL_SEQ_MAX     0x40
#define BT_L2CAP_EXT_CONTROL_SEQ_MAX 0x4000

#define BT_L2CAP_CONTROL_SAR_UNSEG 0x00
#define BT_L2CAP_CONTROL_SAR_START 0x01
#define BT_L2CAP_CONTROL_SAR_END   0x02
#define BT_L2CAP_CONTROL_SAR_CONTI 0x03

#define BT_L2CAP_CONTROL_S_RR   0x00
#define BT_L2CAP_CONTROL_S_REJ  0x01
#define BT_L2CAP_CONTROL_S_RNR  0x02
#define BT_L2CAP_CONTROL_S_SREJ 0x03


#define BT_L2CAP_RT_FC_SDU_LEN_SIZE 2

#define BT_L2CAP_STD_CONTROL_SIZE 2
#define BT_L2CAP_ENH_CONTROL_SIZE 2
#define BT_L2CAP_EXT_CONTROL_SIZE 4

#define BT_L2CAP_FCS_SIZE 2

#if defined(CONFIG_BT_L2CAP_RET) || defined(CONFIG_BT_L2CAP_FC) || \
	defined(CONFIG_BT_L2CAP_ENH_RET) || defined(CONFIG_BT_L2CAP_STREAM)
/**
 *
 *  @brief Helper to calculate L2CAP SDU header size.
 *         Useful for creating buffer pools.
 *
 *  @param mtu Required BT_L2CAP_*_SDU.
 *
 *  @return Header size of the L2CAP channel.
 */
static inline size_t bt_l2cap_br_get_ret_fc_hdr_size(struct bt_l2cap_br_chan *chan)
{
	if (chan->tx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (chan->tx.extended_control) {
			return BT_L2CAP_EXT_CONTROL_SIZE + BT_L2CAP_RT_FC_SDU_LEN_SIZE;
		} else {
			return BT_L2CAP_STD_CONTROL_SIZE + BT_L2CAP_RT_FC_SDU_LEN_SIZE;
		}
	}

	return 0;
}

static inline size_t bt_l2cap_br_get_ret_fc_tail_size(struct bt_l2cap_br_chan *chan)
{
	if (chan->tx.mode != BT_L2CAP_BR_LINK_MODE_BASIC) {
		if (chan->tx.fcs == BT_L2CAP_BR_FCS_16BIT) {
			return BT_L2CAP_FCS_SIZE;
		}
	}

	return 0;
}

/**
 *
 *  @brief Helper to calculate L2CAP SDU header size.
 *         Useful for creating buffer pools.
 *
 *  @param chan the BR channel object point to `struct bt_l2cap_br_chan`.
 *
 *  @return Header size of the L2CAP channel.
 */
#define BT_L2CAP_RT_FC_SDU_HDR_SIZE(chan) bt_l2cap_br_get_ret_fc_hdr_size(chan)

/**
 *
 *  @brief Helper to calculate L2CAP SDU tail size.
 *         Useful for creating buffer pools.
 *
 *  @param chan the BR channel object point to `struct bt_l2cap_br_chan`.
 *
 *  @return Header size of the L2CAP channel.
 */
#define BT_L2CAP_RT_FC_SDU_TAIL_SIZE(chan) bt_l2cap_br_get_ret_fc_tail_size(chan)

/**
 *
 *  @brief Helper to calculate needed buffer size for L2CAP SDUs.
 *         Useful for creating buffer pools.
 *
 *  @param chan the BR channel object point to `struct bt_l2cap_br_chan`.
 *  @param mtu Required BT_L2CAP_*_SDU.
 *
 *  @return Needed buffer size to match the requested L2CAP SDU MTU.
 */
#define BT_L2CAP_RT_FC_SDU_BUF_SIZE(chan, mtu)                              \
	(BT_L2CAP_BUF_SIZE(BT_L2CAP_RT_FC_SDU_HDR_SIZE((chan)) + (mtu) +        \
			   BT_L2CAP_RT_FC_SDU_TAIL_SIZE((chan))))

/**
 *
 *  @brief Helper to calculate needed buffer size for L2CAP SDUs.
 *         Useful for creating buffer pools.
 *
 *  @param mtu Required BT_L2CAP_*_SDU.
 *
 *  @return Needed buffer size to match the requested L2CAP SDU MTU.
 */
#define BT_L2CAP_RT_FC_MAX_SDU_BUF_SIZE(mtu)                                              \
	BT_L2CAP_BUF_SIZE((mtu) + BT_L2CAP_EXT_CONTROL_SIZE + BT_L2CAP_RT_FC_SDU_LEN_SIZE +   \
			  BT_L2CAP_FCS_SIZE)

/**
 *  @brief Headroom needed for outgoing L2CAP PDUs if channel in one of
 *  following mode, including retransmission, flow control, enhance
 *  retransmission, and streaming.
 *
 *  @param chan the BR channel object point to `struct bt_l2cap_br_chan`.
 */
#define BT_L2CAP_RET_FC_SDU_CHAN_SEND_RESERVE(chan) (BT_L2CAP_RT_FC_SDU_HDR_SIZE(chan))
#endif

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

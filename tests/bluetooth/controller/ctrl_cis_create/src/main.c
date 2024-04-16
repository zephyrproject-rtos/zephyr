/*
 * Copyright (c) 2022 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/dbuf.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn_iso.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_iso_internal.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;

static struct ll_conn_iso_group cig_mock = { 0 };
static struct ll_conn_iso_stream cis_mock = { .established = 1, .group = &cig_mock };

/* struct ll_conn_iso_stream *ll_conn_iso_stream_get(uint16_t handle); */
FAKE_VALUE_FUNC(struct ll_conn_iso_stream *, ll_conn_iso_stream_get, uint16_t);

static void cis_create_setup(void *data)
{
	test_setup(&conn);

	RESET_FAKE(ll_conn_iso_stream_get);
}

static bool is_instant_reached(struct ll_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

#define MAX_xDU 160
static struct pdu_data_llctrl_cis_req remote_cis_req = {
	.cig_id           =   0x01,
	.cis_id           =   0x02,
	.c_phy            =   0x01,
	.p_phy            =   0x01,
	.c_max_sdu_packed =   { MAX_xDU, 0 },
	.p_max_sdu        =   { MAX_xDU, 0 },
	.c_max_pdu        =   MAX_xDU,
	.p_max_pdu        =   MAX_xDU,
	.nse              =   2,
	.p_bn             =   1,
	.c_bn             =   1,
	.c_ft             =   1,
	.p_ft             =   1,
	.iso_interval     =   6,
	.conn_event_count =   12,
	.c_sdu_interval   =   { 0, 0, 0},
	.p_sdu_interval   =   { 0, 0, 0},
	.sub_interval     =   { 0, 0, 0},
	.cis_offset_min   =   { 0, 0, 0},
	.cis_offset_max   =   { 0, 0, 0}
};

static struct pdu_data_llctrl_cis_ind remote_cis_ind = {
	.aa = { 0, 0, 0, 0},
	.cig_sync_delay = { 0, 0, 0},
	.cis_offset = { 0, 0, 0},
	.cis_sync_delay = { 0, 0, 0},
	.conn_event_count = 12
};

static struct pdu_data_llctrl_cis_req local_cis_req = {
	.cig_id           =   0x00,
	.cis_id           =   0x02,
	.c_phy            =   0x01,
	.p_phy            =   0x01,
	.c_max_sdu_packed =   { MAX_xDU, 0 },
	.p_max_sdu        =   { MAX_xDU, 0 },
	.c_max_pdu        =   MAX_xDU,
	.p_max_pdu        =   MAX_xDU,
	.nse              =   2,
	.p_bn             =   1,
	.c_bn             =   1,
	.c_ft             =   1,
	.p_ft             =   1,
	.iso_interval     =   6,
	.conn_event_count =   0,
	.c_sdu_interval   =   { 0, 0, 0},
	.p_sdu_interval   =   { 0, 0, 0},
	.sub_interval     =   { 0, 0, 0},
	.cis_offset_min   =   { 0, 0, 0},
	.cis_offset_max   =   { 0, 0, 0}
};

static struct pdu_data_llctrl_cis_ind local_cis_ind = {
	.aa = { 0, 0, 0, 0},
	.cig_sync_delay = { 0, 0, 0},
	.cis_offset = { 0, 0, 0},
	.cis_sync_delay = { 0, 0, 0},
	.conn_event_count = 13
};

#define ERROR_CODE 0x17
/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    | LE CIS Request            |                           |
 *    | Accept                    |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_CIS_RSP                |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |   LL_CIS_IND              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE CIS ESTABLISHED   |                           |
 *    |<--------------------------|                           |
 */
ZTEST(cis_create, test_cc_create_periph_rem_host_accept)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_cis_rsp local_cis_rsp = {
		.cis_offset_max = { 0, 0, 0},
		.cis_offset_min = { 0, 0, 0},
		.conn_event_count = 12
	};
	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = 0x00
	};

	/* Prepare mocked call to ll_conn_iso_stream_get() */
	ll_conn_iso_stream_get_fake.return_val = &cis_mock;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Accept request */
	ull_cp_cc_accept(&conn, 0U);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CIS_RSP, &conn, &tx, &local_cis_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_CIS_IND, &conn, &remote_cis_ind);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, remote_cis_ind.conn_event_count)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Emulate CIS becoming established */
	ull_cp_cc_established(&conn, 0);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	/* Done */
	event_done(&conn);

	/* NODE_CIS_ESTABLISHED carry extra information in header rx footer param field */
	zassert_equal_ptr(ntf->hdr.rx_ftr.param, &cis_mock);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral Host rejects.
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    | LE CIS Request            |                           |
 *    | Decline                   |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
ZTEST(cis_create, test_cc_create_periph_rem_host_reject)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = ERROR_CODE,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Decline request */
	ull_cp_cc_reject(&conn, ERROR_CODE);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, ask peripheral host peripheral Host fails to reply
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_S  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |      LE CIS Request       |                           |
 *    |<--------------------------|                           |
 *    |                           |                           |
 *
 *
 *			Let time pass ......
 *
 *
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND (to)    |
 *    |                           |-------------------------->|
 *    |                           |                           |
 */
ZTEST(cis_create, test_cc_create_periph_rem_host_accept_to)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct node_rx_conn_iso_req cis_req = {
		.cig_id = 0x01,
		.cis_handle = 0x00,
		.cis_id = 0x02
	};
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = BT_HCI_ERR_CONN_ACCEPT_TIMEOUT,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};
	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = BT_HCI_ERR_CONN_ACCEPT_TIMEOUT
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_REQUEST, &ntf, &cis_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Emulate that time passes real fast re. timeout */
	conn.connect_accept_to = 0;

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should now have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS w. invalid PHY param, peripheral Host rejects.
 *
 * +-----+          +-------+                       +-----+
 * | UT  |          | LL_S  |                       | LT  |
 * +-----+          +-------+                       +-----+
 *    |                 |                               |
 *    |                 |   LL_CIS_REQ  (w. invald PHY) |
 *    |                 |<------------------------------|
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 |                               |
 *    |                 | LL_REJECT_EXT_IND             |
 *    |                 |------------------------------>|
 *    |                 |                               |
 */
ZTEST(cis_create, test_cc_create_periph_rem_invalid_phy)
{
	static struct pdu_data_llctrl_cis_req remote_cis_req_invalid_phy = {
		.cig_id           =   0x01,
		.cis_id           =   0x02,
		.c_phy            =   0x03,
		.p_phy            =   0x01,
		.c_max_sdu_packed =   { MAX_xDU, 0 },
		.p_max_sdu        =   { MAX_xDU, 0 },
		.c_max_pdu        =   MAX_xDU,
		.p_max_pdu        =   MAX_xDU,
		.nse              =   2,
		.p_bn             =   1,
		.c_bn             =   1,
		.c_ft             =   1,
		.p_ft             =   1,
		.iso_interval     =   6,
		.conn_event_count =   12,
		.c_sdu_interval   =   { 0, 0, 0},
		.p_sdu_interval   =   { 0, 0, 0},
		.sub_interval     =   { 0, 0, 0},
		.cis_offset_min   =   { 0, 0, 0},
		.cis_offset_max   =   { 0, 0, 0}
	};
	struct node_tx *tx;
	struct pdu_data_llctrl_reject_ext_ind local_reject = {
		.error_code = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_REQ, &conn, &remote_cis_req_invalid_phy);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &local_reject);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Host requests CIS, LL replies with 'remote feature unsupported'
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE CIS Create             |                           |
 *    |-------------------------->|                           |
 *    |                           |                           |
 *    |                           | (FEAT unsupported)        |
 *    |                           |                           |
 *    |                           |                           |
 *    |    LE CIS ESTABLISHED     |                           |
 *    |    (rem feat unsupported) |                           |
 *    |<--------------------------|                           |
 */
ZTEST(cis_create, test_cc_create_central_rem_unsupported)
{
	struct ll_conn_iso_stream *cis;
	struct node_rx_pdu *ntf;
	uint8_t err;

	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};

	/* Prepare mocked call to ll_conn_iso_stream_get() */
	ll_conn_iso_stream_get_fake.return_val = &cis_mock;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.valid = 1;

	cis = ll_conn_iso_stream_get(LL_CIS_HANDLE_BASE);
	cis->lll.acl_handle = conn.lll.handle;

	err = ull_cp_cis_create(&conn, cis);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* There should be excactly one host notification
	 * with status BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	 */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral accepts
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE CIS Create             |                           |
 *    |-------------------------->|                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           | LL_CIS_RSP                |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |   LL_CIS_IND              |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE CIS ESTABLISHED   |                           |
 *    |<--------------------------|                           |
 */
ZTEST(cis_create, test_cc_create_central_rem_accept)
{
	struct pdu_data_llctrl_cis_rsp remote_cis_rsp = {
		.cis_offset_max = { 0, 0, 0},
		.cis_offset_min = { 0, 0, 0},
		.conn_event_count = 13
	};
	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = BT_HCI_ERR_SUCCESS
	};
	struct ll_conn_iso_stream *cis;
	struct node_rx_pdu *ntf;
	struct node_tx *tx;
	uint8_t err;

	/* Prepare mocked call to ll_conn_iso_stream_get() */
	ll_conn_iso_stream_get_fake.return_val = &cis_mock;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.valid = 1;
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_CIS_PERIPHERAL);

	/* Setup default CIS/CIG parameters */
	cis = ll_conn_iso_stream_get(LL_CIS_HANDLE_BASE);
	cis->lll.acl_handle = conn.lll.handle;
	cis->group->cig_id = local_cis_req.cig_id;
	cis->cis_id = local_cis_req.cis_id;
	cis->lll.tx.phy = local_cis_req.c_phy;
	cis->lll.rx.phy = local_cis_req.p_phy;
	cis->group->c_sdu_interval = 0;
	cis->group->p_sdu_interval = 0;
	cis->lll.tx.max_pdu = MAX_xDU;
	cis->lll.rx.max_pdu = MAX_xDU;
	cis->c_max_sdu = MAX_xDU;
	cis->p_max_sdu = MAX_xDU;
	cis->group->iso_interval = local_cis_req.iso_interval;
	cis->framed = 0;
	cis->lll.nse = local_cis_req.nse;
	cis->lll.sub_interval = 0;
	cis->lll.tx.bn = local_cis_req.c_bn;
	cis->lll.rx.bn = local_cis_req.p_bn;
	cis->lll.tx.ft = local_cis_req.c_ft;
	cis->lll.rx.ft = local_cis_req.p_ft;

	err = ull_cp_cis_create(&conn, cis);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CIS_REQ, &conn, &tx, &local_cis_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_CIS_RSP, &conn, &remote_cis_rsp);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CIS_IND, &conn, &tx, &local_cis_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* */
	while (!is_instant_reached(&conn, remote_cis_rsp.conn_event_count)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Prepare */
	event_prepare(&conn);

	/* Done */
	event_done(&conn);

	/* Emulate CIS becoming established */
	ull_cp_cc_established(&conn, 0);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/*
 * Central-initiated CIS Create procedure.
 * Central requests CIS, peripheral rejects with 'unsupported remote feature'
 *
 * +-----+                    +-------+                    +-----+
 * | UT  |                    | LL_C  |                    | LT  |
 * +-----+                    +-------+                    +-----+
 *    |                           |                           |
 *    | LE CIS Create             |                           |
 *    |-------------------------->|                           |
 *    |                           |   LL_CIS_REQ              |
 *    |                           |-------------------------->|
 *    |                           |                           |
 *    |                           | LL_REJECT_EXT_IND         |
 *    |                           | (unsupported remote feat) |
 *    |                           |<--------------------------|
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 *    |                           |                           |
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    |                           |                           |
 *    |      LE CIS ESTABLISHED   |                           |
 *    |<--------------------------|                           |
 */
ZTEST(cis_create, test_cc_create_central_rem_reject)
{
	struct node_rx_conn_iso_estab cis_estab = {
		.cis_handle = 0x00,
		.status = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};
	struct pdu_data_llctrl_reject_ext_ind remote_reject = {
		.error_code = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE,
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_CIS_REQ
	};
	struct ll_conn_iso_stream *cis;
	struct node_rx_pdu *ntf;
	struct node_tx *tx;
	uint8_t err;

	/* Prepare mocked call to ll_conn_iso_stream_get() */
	ll_conn_iso_stream_get_fake.return_val = &cis_mock;

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	conn.llcp.fex.valid = 1;
	conn.llcp.fex.features_peer |= BIT64(BT_LE_FEAT_BIT_CIS_PERIPHERAL);

	/* Setup default CIS/CIG parameters */
	cis = ll_conn_iso_stream_get(LL_CIS_HANDLE_BASE);
	cis->lll.acl_handle = conn.lll.handle;
	cis->group->cig_id = local_cis_req.cig_id;
	cis->cis_id = local_cis_req.cis_id;
	cis->lll.tx.phy = local_cis_req.c_phy;
	cis->lll.rx.phy = local_cis_req.p_phy;
	cis->group->c_sdu_interval = 0;
	cis->group->p_sdu_interval = 0;
	cis->lll.tx.max_pdu = MAX_xDU;
	cis->lll.rx.max_pdu = MAX_xDU;
	cis->c_max_sdu = MAX_xDU;
	cis->p_max_sdu = MAX_xDU;
	cis->group->iso_interval = local_cis_req.iso_interval;
	cis->framed = 0;
	cis->lll.nse = local_cis_req.nse;
	cis->lll.sub_interval = 0;
	cis->lll.tx.bn = local_cis_req.c_bn;
	cis->lll.rx.bn = local_cis_req.p_bn;
	cis->lll.tx.ft = local_cis_req.c_ft;
	cis->lll.rx.ft = local_cis_req.p_ft;

	err = ull_cp_cis_create(&conn, cis);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CIS_REQ, &conn, &tx, &local_cis_req);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &remote_reject);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* There should be excactly one host notification */
	ut_rx_node(NODE_CIS_ESTABLISHED, &ntf, &cis_estab);
	ut_rx_q_is_empty();

	zassert_equal(conn.llcp.fex.features_peer & BIT64(BT_LE_FEAT_BIT_CIS_PERIPHERAL), 0);

	/* Done */
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}


ZTEST_SUITE(cis_create, NULL, NULL, cis_create_setup, NULL, NULL);

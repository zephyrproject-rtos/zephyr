/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

#define ULL_LLCP_UNITTEST

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
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

static struct ll_conn conn;

static void chmu_setup(void *data)
{
	test_setup(&conn);
}

static bool is_instant_reached(struct ll_conn *conn, uint16_t instant)
{
	return ((event_counter(conn) - instant) & 0xFFFF) <= 0x7FFF;
}

ZTEST(chmu, test_channel_map_update_central_loc)
{
	uint8_t chm[5] = { 0x00, 0x04, 0x05, 0x06, 0x00 };
	uint8_t initial_chm[5];
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data *pdu;
	uint16_t instant;
	struct pdu_data_llctrl_chan_map_ind chmu_ind = {
		.instant = 6,
		.chm = { 0x00, 0x04, 0x05, 0x06, 0x00 },
	};

	/* Store initial channel map */
	memcpy(initial_chm, conn.lll.data_chan_map, sizeof(conn.lll.data_chan_map));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_chan_map_update(&conn, chm);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CHAN_MAP_UPDATE_IND, &conn, &tx, &chmu_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Save Instant */
	pdu = (struct pdu_data *)tx->pdu;
	instant = sys_le16_to_cpu(pdu->llctrl.chan_map_ind.instant);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* spin conn events */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();

		/* check if still using initial channel map */
		zassert_mem_equal(conn.lll.data_chan_map, initial_chm,
				  sizeof(conn.lll.data_chan_map),
				  "Channel map invalid");
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* at this point new channel map shall be in use */
	zassert_mem_equal(conn.lll.data_chan_map, chm, sizeof(conn.lll.data_chan_map),
			  "Channel map invalid");

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(chmu, test_channel_map_update_central_invalid)
{
	uint8_t chm[5] = { 0x00, 0x04, 0x05, 0x06, 0x00 };
	uint8_t err;
	struct node_tx *tx;
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND
	};
	struct pdu_data_llctrl_chan_map_ind chmu_ind = {
		.instant = 6,
		.chm = { 0x00, 0x04, 0x05, 0x06, 0x00 },
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_chan_map_update(&conn, chm);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_CHAN_MAP_UPDATE_IND, &conn, &tx, &chmu_ind);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should NOT be a host notification */
	ut_rx_q_is_empty();


	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Inject invalid 'RSP' */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(chmu, test_channel_map_update_periph_rem)
{
	uint8_t chm[5] = { 0x00, 0x04, 0x05, 0x06, 0x00 };
	uint8_t initial_chm[5];
	struct pdu_data_llctrl_chan_map_ind chmu_ind = {
		.instant = 6,
		.chm = { 0x00, 0x04, 0x05, 0x06, 0x00 },
	};
	uint16_t instant = 6;

	/* Store initial channel map */
	memcpy(initial_chm, conn.lll.data_chan_map, sizeof(conn.lll.data_chan_map));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* RX */
	lt_tx(LL_CHAN_MAP_UPDATE_IND, &conn, &chmu_ind);

	/* Done */
	event_done(&conn);

	/* spin conn events */
	while (!is_instant_reached(&conn, instant)) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();

		/* check if using old channel map */
		zassert_mem_equal(conn.lll.data_chan_map, initial_chm,
				  sizeof(conn.lll.data_chan_map),
				  "Channel map invalid");
	}

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* There should be no host notification */
	ut_rx_q_is_empty();

	/* at this point new channel map shall be in use */
	zassert_mem_equal(conn.lll.data_chan_map, chm, sizeof(conn.lll.data_chan_map),
			  "Channel map invalid");

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(chmu, test_channel_map_update_periph_invalid)
{
	struct pdu_data_llctrl_chan_map_ind chmu_ind = {
		.instant = 6,
		.chm = { 0x00, 0x04, 0x05, 0x06, 0x00 },
	};
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_UNUSED
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* RX */
	lt_tx(LL_CHAN_MAP_UPDATE_IND, &conn, &chmu_ind);

	/* Done */
	event_done(&conn);

	/* There should not be a host notifications */
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);
	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Inject invalid 'RSP' */
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);

	/* Done */
	event_done(&conn);

	/* Tx Queue should NOT have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Termination 'triggered' */
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(chmu, test_channel_map_update_periph_loc)
{
	uint8_t err;
	uint8_t chm[5] = { 0x00, 0x06, 0x06, 0x06, 0x00 };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_chan_map_update(&conn, chm);
	zassert_equal(err, BT_HCI_ERR_CMD_DISALLOWED);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST_SUITE(chmu, NULL, NULL, chmu_setup, NULL, NULL);

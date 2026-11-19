/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>

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

#define IFS_DEFAULT_US 150U
#define IFS_LOW_LAT_US CONFIG_BT_CTLR_EVENT_IFS_LOW_LAT_US
#define FSU_ACL_TYPES  (T_IFS_ACL_CP | T_IFS_ACL_PC)

static struct ll_conn conn;

static void fsu_setup(void *data)
{
	test_setup(&conn);

	conn.lll.phy_tx = PHY_1M;
	conn.lll.phy_rx = PHY_1M;
	conn.lll.tifs_tx_us = IFS_DEFAULT_US;
	conn.lll.tifs_rx_us = IFS_DEFAULT_US;
	conn.lll.tifs_hcto_us = IFS_DEFAULT_US;
	ull_fsu_init(&conn, IFS_DEFAULT_US);
}

static void fsu_tifs_check(uint16_t tx_us, uint16_t rx_us, uint16_t hcto_us)
{
	zassert_equal(conn.lll.tifs_tx_us, tx_us, "tifs_tx_us %u", conn.lll.tifs_tx_us);
	zassert_equal(conn.lll.tifs_rx_us, rx_us, "tifs_rx_us %u", conn.lll.tifs_rx_us);
	zassert_equal(conn.lll.tifs_hcto_us, hcto_us, "tifs_hcto_us %u", conn.lll.tifs_hcto_us);
}

/* +-----+                     +-------+            +-----+
 * | UT  |                     | LL_A  |            | LT  |
 * +-----+                     +-------+            +-----+
 *    |                            |                   |
 *    | Start                      |                   |
 *    | Frame Space Update Proc.   |                   |
 *    |--------------------------->|                   |
 *    |                            |                   |
 *    |                            | LL_FRAME_SPACE_REQ|
 *    |                            |------------------>|
 *    |                            |                   |
 *    |                            | LL_FRAME_SPACE_RSP|
 *    |                            |<------------------|
 *    |                            |                   |
 *    | Frame Space Update Complete|                   |
 *    |<---------------------------|                   |
 *    |                            |                   |
 */
ZTEST(fsu_central, test_fsu_central_loc)
{
	struct pdu_data_llctrl_fsu_req local_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_rsp remote_fsu_rsp = {
		.fsu = 60U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct node_rx_pdu *ntf;
	struct node_tx *tx;
	uint8_t err;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_fsu(&conn, local_fsu_req.fsu_min, local_fsu_req.fsu_max, local_fsu_req.phys,
			 local_fsu_req.spacing_type);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);

	lt_rx(LL_FRAME_SPACE_REQ, &conn, &tx, &local_fsu_req);
	lt_rx_q_is_empty(&conn);

	/* Receive window covers the current and any frame space the peer can select */
	fsu_tifs_check(IFS_DEFAULT_US, 60U, 200U);

	lt_tx(LL_FRAME_SPACE_RSP, &conn, &remote_fsu_rsp);

	event_done(&conn);

	ull_cp_release_tx(&conn, tx);

	ut_rx_pdu(LL_FRAME_SPACE_RSP, &ntf, &remote_fsu_rsp);
	ut_rx_q_is_empty();

	fsu_tifs_check(60U, 60U, 60U);

	/* Frame spaces of PHYs not in the request are not changed */
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_cp_us, IFS_DEFAULT_US);
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_pc_us, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* Peer responds with a subset of the requested PHYs and spacing types */
ZTEST(fsu_central, test_fsu_central_loc_subset)
{
	struct pdu_data_llctrl_fsu_req local_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M | PHY_2M,
		.spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_rsp remote_fsu_rsp = {
		.fsu = 100U, .phys = PHY_2M | PHY_CODED, .spacing_type = T_IFS_ACL_PC | T_MCES,
	};
	struct pdu_data_llctrl_fsu_rsp fsu_ntf = {
		.fsu = 100U, .phys = PHY_2M, .spacing_type = T_IFS_ACL_PC,
	};
	struct node_rx_pdu *ntf;
	struct node_tx *tx;
	uint8_t err;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_fsu(&conn, local_fsu_req.fsu_min, local_fsu_req.fsu_max, local_fsu_req.phys,
			 local_fsu_req.spacing_type);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_REQ, &conn, &tx, &local_fsu_req);
	lt_rx_q_is_empty(&conn);
	lt_tx(LL_FRAME_SPACE_RSP, &conn, &remote_fsu_rsp);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	ut_rx_pdu(LL_FRAME_SPACE_RSP, &ntf, &fsu_ntf);
	ut_rx_q_is_empty();

	/* Connection is on 1M PHY, the update applies to 2M PHY only */
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);
	zassert_equal(conn.lll.fsu.phy[0].ifs_acl_pc_us, IFS_DEFAULT_US);
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_pc_us, 100U);
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_cp_us, IFS_DEFAULT_US);

	/* Central transmits with T_IFS_ACL_PC once on 2M PHY */
	conn.lll.phy_tx = PHY_2M;
	conn.lll.phy_rx = PHY_2M;
	ull_fsu_tifs_set(&conn);
	fsu_tifs_check(100U, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(fsu_central, test_fsu_central_loc_unknown_rsp)
{
	struct pdu_data_llctrl_fsu_req local_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_unknown_rsp unknown_rsp = {
		.type = PDU_DATA_LLCTRL_TYPE_FRAME_SPACE_REQ,
	};
	struct node_tx *tx;
	uint8_t err;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_fsu(&conn, local_fsu_req.fsu_min, local_fsu_req.fsu_max, local_fsu_req.phys,
			 local_fsu_req.spacing_type);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_REQ, &conn, &tx, &local_fsu_req);
	lt_rx_q_is_empty(&conn);
	lt_tx(LL_UNKNOWN_RSP, &conn, &unknown_rsp);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	ut_rx_q_is_empty();
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(fsu_central, test_fsu_central_loc_reject_ext_ind)
{
	struct pdu_data_llctrl_fsu_req local_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_FRAME_SPACE_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL,
	};
	struct node_tx *tx;
	uint8_t err;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_fsu(&conn, local_fsu_req.fsu_min, local_fsu_req.fsu_max, local_fsu_req.phys,
			 local_fsu_req.spacing_type);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_REQ, &conn, &tx, &local_fsu_req);
	lt_rx_q_is_empty(&conn);
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	zassert_equal(conn.llcp_terminate.reason_final, 0U, "Terminate reason %d",
		      conn.llcp_terminate.reason_final);
	ut_rx_q_is_empty();
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* Selected frame space outside the requested range terminates the connection */
ZTEST(fsu_central, test_fsu_central_loc_invalid_rsp)
{
	struct pdu_data_llctrl_fsu_req local_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_rsp remote_fsu_rsp = {
		.fsu = 300U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct node_tx *tx;
	uint8_t err;

	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	err = ull_cp_fsu(&conn, local_fsu_req.fsu_min, local_fsu_req.fsu_max, local_fsu_req.phys,
			 local_fsu_req.spacing_type);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_REQ, &conn, &tx, &local_fsu_req);
	lt_rx_q_is_empty(&conn);
	lt_tx(LL_FRAME_SPACE_RSP, &conn, &remote_fsu_rsp);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_LMP_PDU_NOT_ALLOWED,
		      "Terminate reason %d", conn.llcp_terminate.reason_final);
	ut_rx_q_is_empty();
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(fsu_central, test_fsu_central_loc_invalid_param)
{
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	zassert_equal(ull_cp_fsu(&conn, 200U, 60U, PHY_1M, FSU_ACL_TYPES),
		      BT_HCI_ERR_INVALID_PARAM);
	zassert_equal(ull_cp_fsu(&conn, 60U, 10001U, PHY_1M, FSU_ACL_TYPES),
		      BT_HCI_ERR_INVALID_PARAM);
	zassert_equal(ull_cp_fsu(&conn, 60U, 200U, 0U, FSU_ACL_TYPES), BT_HCI_ERR_INVALID_PARAM);
	zassert_equal(ull_cp_fsu(&conn, 60U, 200U, PHY_1M, 0U), BT_HCI_ERR_INVALID_PARAM);
	zassert_equal(ull_cp_fsu(&conn, 60U, 200U, PHY_CODED, FSU_ACL_TYPES),
		      BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
	zassert_equal(ull_cp_fsu(&conn, 60U, 200U, PHY_1M, T_MCES),
		      BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
	zassert_equal(ull_cp_fsu(&conn, 0U, IFS_LOW_LAT_US - 1U, PHY_1M, FSU_ACL_TYPES),
		      BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);

	event_prepare(&conn);
	lt_rx_q_is_empty(&conn);
	event_done(&conn);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+ +-------+                 +-----+
 * | UT  | | LL_A  |                 | LT  |
 * +-----+ +-------+                 +-----+
 *    |        |                        |
 *    |        |     LL_FRAME_SPACE_REQ |
 *    |        |<-----------------------|
 *    |        |                        |
 *    |        | LL_FRAME_SPACE_RSP     |
 *    |        |----------------------->|
 *    |        |                        |
 *    | Frame Space Update Complete     |
 *    |<-------|                        |
 *    |        |                        |
 */
ZTEST(fsu_periph, test_fsu_periph_rem)
{
	struct pdu_data_llctrl_fsu_req remote_fsu_req = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M | PHY_2M,
		.spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_rsp local_fsu_rsp = {
		.fsu = 60U, .phys = PHY_1M | PHY_2M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_req fsu_ntf = {
		.fsu_min = 60U, .fsu_max = 60U, .phys = PHY_1M | PHY_2M,
		.spacing_type = FSU_ACL_TYPES,
	};
	struct node_rx_pdu *ntf;
	struct node_tx *tx;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);
	lt_tx(LL_FRAME_SPACE_REQ, &conn, &remote_fsu_req);
	event_done(&conn);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_RSP, &conn, &tx, &local_fsu_rsp);
	lt_rx_q_is_empty(&conn);

	/* Transmit with the new frame space, receive with both until the RSP is acknowledged */
	fsu_tifs_check(60U, 60U, IFS_DEFAULT_US);

	event_tx_ack(&conn, tx);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	ut_rx_pdu(LL_FRAME_SPACE_REQ, &ntf, &fsu_ntf);
	ut_rx_q_is_empty();

	fsu_tifs_check(60U, 60U, 60U);
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_cp_us, 60U);
	zassert_equal(conn.lll.fsu.phy[1].ifs_acl_pc_us, 60U);
	zassert_equal(conn.lll.fsu.phy[2].ifs_acl_cp_us, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* No notification when the frame spaces do not change */
ZTEST(fsu_periph, test_fsu_periph_rem_no_change)
{
	struct pdu_data_llctrl_fsu_req remote_fsu_req = {
		.fsu_min = IFS_DEFAULT_US, .fsu_max = 200U, .phys = PHY_1M,
		.spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_rsp local_fsu_rsp = {
		.fsu = IFS_DEFAULT_US, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct node_tx *tx;

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	event_prepare(&conn);
	lt_tx(LL_FRAME_SPACE_REQ, &conn, &remote_fsu_req);
	event_done(&conn);

	event_prepare(&conn);
	lt_rx(LL_FRAME_SPACE_RSP, &conn, &tx, &local_fsu_rsp);
	lt_rx_q_is_empty(&conn);
	event_tx_ack(&conn, tx);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	ut_rx_q_is_empty();
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

static void fsu_periph_rem_reject(struct pdu_data_llctrl_fsu_req *remote_fsu_req,
				  uint8_t error_code)
{
	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_FRAME_SPACE_REQ,
		.error_code = error_code,
	};
	struct node_tx *tx;

	event_prepare(&conn);
	lt_tx(LL_FRAME_SPACE_REQ, &conn, remote_fsu_req);
	event_done(&conn);

	event_prepare(&conn);
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);
	event_tx_ack(&conn, tx);
	event_done(&conn);
	ull_cp_release_tx(&conn, tx);

	ut_rx_q_is_empty();
	fsu_tifs_check(IFS_DEFAULT_US, IFS_DEFAULT_US, IFS_DEFAULT_US);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
		      "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(fsu_periph, test_fsu_periph_rem_reject)
{
	struct pdu_data_llctrl_fsu_req min_above_max = {
		.fsu_min = 200U, .fsu_max = 60U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_req max_out_of_range = {
		.fsu_min = 60U, .fsu_max = 10001U, .phys = PHY_1M, .spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_req coded_phy = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M | PHY_CODED,
		.spacing_type = FSU_ACL_TYPES,
	};
	struct pdu_data_llctrl_fsu_req mces = {
		.fsu_min = 60U, .fsu_max = 200U, .phys = PHY_1M,
		.spacing_type = T_IFS_ACL_CP | T_MCES,
	};
	struct pdu_data_llctrl_fsu_req below_radio_min = {
		.fsu_min = 0U, .fsu_max = IFS_LOW_LAT_US - 1U, .phys = PHY_1M,
		.spacing_type = FSU_ACL_TYPES,
	};

	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	fsu_periph_rem_reject(&min_above_max, BT_HCI_ERR_INVALID_LL_PARAM);
	fsu_periph_rem_reject(&max_out_of_range, BT_HCI_ERR_INVALID_LL_PARAM);
	fsu_periph_rem_reject(&coded_phy, BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
	fsu_periph_rem_reject(&mces, BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
	fsu_periph_rem_reject(&below_radio_min, BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL);
}

ZTEST_SUITE(fsu_central, NULL, NULL, fsu_setup, NULL, NULL);
ZTEST_SUITE(fsu_periph, NULL, NULL, fsu_setup, NULL, NULL);

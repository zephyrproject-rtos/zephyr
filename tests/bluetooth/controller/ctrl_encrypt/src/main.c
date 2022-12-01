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
#include "ll_feat.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"

#include "ull_tx_queue.h"

#include "isoal.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_conn_internal.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

/* Tx/Rx pause flag */
#define RESUMED 0U
#define PAUSED 1U

/* Tx/Rx encryption flag */
#define UNENCRYPTED 0U
#define ENCRYPTED 1U

/* Check Rx Pause and Encryption state */
#define CHECK_RX_PE_STATE(_conn, _pause, _enc)                                       \
	do {                                                                             \
		zassert_equal(_conn.pause_rx_data, _pause, "Rx Data pause state is wrong.");\
		zassert_equal(_conn.lll.enc_rx, _enc, "Rx Encryption state is wrong.");     \
	} while (0)

/* Check Tx Pause and Encryption state */
#define CHECK_TX_PE_STATE(_conn, _pause, _enc)                                         \
	do {                                                                               \
		zassert_equal(_conn.tx_q.pause_data, _pause, "Tx Data pause state is wrong.");\
		zassert_equal(_conn.lll.enc_tx, _enc, "Tx Encryption state is wrong.");       \
	} while (0)

/* CCM direction flag */
#define CCM_DIR_M_TO_S 1U
#define CCM_DIR_S_TO_M 0U

/* Check Rx CCM state */
#define CHECK_RX_CCM_STATE(_conn, _sk_be, _iv, _cnt, _dir)                            \
	do {                                                                              \
		zassert_mem_equal(_conn.lll.ccm_rx.key, _sk_be, sizeof(_sk_be),              \
				  "CCM Rx SK not equal to expected SK");                      \
		zassert_mem_equal(_conn.lll.ccm_rx.iv, _iv, sizeof(_iv),                     \
				  "CCM Rx IV not equal to (IVm | IVs)");                      \
		zassert_equal(_conn.lll.ccm_rx.counter, _cnt, "CCM Rx Counter is wrong");    \
		zassert_equal(_conn.lll.ccm_rx.direction, _dir, "CCM Rx Direction is wrong");\
	} while (0)

/* Check Tx CCM state */
#define CHECK_TX_CCM_STATE(_conn, _sk_be, _iv, _cnt, _dir)                            \
	do {                                                                              \
		zassert_mem_equal(_conn.lll.ccm_tx.key, _sk_be, sizeof(_sk_be),              \
				  "CCM Tx SK not equal to expected SK");                      \
		zassert_mem_equal(_conn.lll.ccm_tx.iv, _iv, sizeof(_iv),                     \
				  "CCM Tx IV not equal to (IVm | IVs)");                      \
		zassert_equal(_conn.lll.ccm_tx.counter, _cnt, "CCM Tx Counter is wrong");    \
		zassert_equal(_conn.lll.ccm_tx.direction, _dir, "CCM Tx Direction is wrong");\
	} while (0)

static struct ll_conn conn;

static void enc_setup(void *data)
{
	test_setup(&conn);

	/* Fake that a Feature exchange proceudre has been executed */
	conn.llcp.fex.valid = 1U;
	conn.llcp.fex.features_used |= LL_FEAT_BIT_EXT_REJ_IND;
}

void ecb_encrypt(uint8_t const *const key_le, uint8_t const *const clear_text_le,
		 uint8_t *const cipher_text_le, uint8_t *const cipher_text_be)
{
	ztest_check_expected_data(key_le, 16);
	ztest_check_expected_data(clear_text_le, 16);
	if (cipher_text_le) {
		ztest_copy_return_data(cipher_text_le, 16);
	}

	if (cipher_text_be) {
		ztest_copy_return_data(cipher_text_be, 16);
	}
}

int lll_csrand_get(void *buf, size_t len)
{
	ztest_check_expected_value(len);
	ztest_copy_return_data(buf, len);
	return ztest_get_return_value();
}

/* BLUETOOTH CORE SPECIFICATION Version 5.2 | Vol 6, Part C
 * 1 ENCRYPTION SAMPLE DATA
 */
#define RAND 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90
#define EDIV 0x24, 0x74
#define LTK                                                                                  \
	0x4C, 0x68, 0x38, 0x41, 0x39, 0xF5, 0x74, 0xD8, 0x36, 0xBC, 0xF3, 0x4E, 0x9D, 0xFB, 0x01,\
		0xBF
#define SKDM 0xAC, 0xBD, 0xCE, 0xDF, 0xE0, 0xF1, 0x02, 0x13
#define SKDS 0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79
#define IVM 0xBA, 0xDC, 0xAB, 0x24
#define IVS 0xDE, 0xAF, 0xBA, 0xBE

#define SK_BE                                                                                \
	0x66, 0xC6, 0xC2, 0x27, 0x8E, 0x3B, 0x8E, 0x05, 0x3E, 0x7E, 0xA3, 0x26, 0x52, 0x1B, 0xAD,\
		0x99
/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_REQ |
 *    |                            |<--------------------|
 *    |          ----------------\ |                     |
 *    |          | Tx Encryption |-|                     |
 *    |          | Rx Decryption | |                     |
 *    |          |---------------| |                     |
 *    |                            |                     |
 *    |                            | LL_START_ENC_RSP    |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Tx paused & enc. */

	/* CCM Tx/Rx SK should match SK */
	/* CCM Tx/Rx IV should match the IV */
	/* CCM Tx/Rx Counter should be zero */
	/* CCM Rx Direction should be S->M */
	/* CCM Tx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Tx paused & enc. */

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |         -----------------\ |                     |
 *    |         | Reserver all   |-|                     |
 *    |         | Tx/Ntf buffers | |                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_REQ |
 *    |                            |<--------------------|
 *    |          ----------------\ |                     |
 *    |          | Tx Encryption |-|                     |
 *    |          | Rx Decryption | |                     |
 *    |          |---------------| |                     |
 *    |                            |                     |
 *    |                            | LL_START_ENC_RSP    |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |    LL_START_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_limited_memory)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct proc_ctx *ctx = NULL;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Allocate dummy procedure used to steal all buffers */
	ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);

	/* Steal all tx buffers */
	while (llcp_tx_alloc_peek(&conn, ctx)) {
		tx = llcp_tx_alloc(&conn, ctx);
		zassert_not_null(tx, NULL);
	}

	/* Dummy remove, as above loop might queue up ctx */
	llcp_tx_alloc_unpeek(ctx);

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Tx Queue should have no LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have no LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Tx paused & enc. */

	/* CCM Tx/Rx SK should match SK */
	/* CCM Tx/Rx IV should match the IV */
	/* CCM Tx/Rx Counter should be zero */
	/* CCM Tx Direction should be M->S */
	/* CCM Rx Direction should be S->M */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U);

	/* Release dummy procedure */
	llcp_proc_ctx_release(ctx);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |   LL_REJECT_EXT_IND |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_reject_ext)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	struct pdu_data_llctrl_reject_ind reject_ind = { .error_code =
								 BT_HCI_ERR_UNSUPP_REMOTE_FEATURE };

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_UNSUPP_REMOTE_FEATURE
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |   LL_REJECT_IND     |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_reject)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	struct pdu_data_llctrl_reject_ind reject_ind = { .error_code =
								 BT_HCI_ERR_UNSUPP_REMOTE_FEATURE };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |   LL_REJECT_EXT_IND |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_no_ltk)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	struct pdu_data_llctrl_reject_ind reject_ind = { .error_code =
								 BT_HCI_ERR_PIN_OR_KEY_MISSING };

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_REJECT_EXT_IND, &conn, &reject_ext_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |   LL_REJECT_IND     |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_no_ltk_2)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	struct pdu_data_llctrl_reject_ind reject_ind = { .error_code =
								 BT_HCI_ERR_PIN_OR_KEY_MISSING };

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_REJECT_IND, &conn, &reject_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_REJECT_IND, &ntf, &reject_ind);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                     +-------+              +-----+
 * | UT  |                     | LL_A  |              | LT  |
 * +-----+                     +-------+              +-----+
 *    |                            |                     |
 *    | Initiate                   |                     |
 *    | Encryption Start Proc.     |                     |
 *    |--------------------------->|                     |
 *    |         -----------------\ |                     |
 *    |         | Empty Tx queue |-|                     |
 *    |         |----------------| |                     |
 *    |                            |                     |
 *    |                            | LL_ENC_REQ          |
 *    |                            |-------------------->|
 *    |                            |                     |
 *    |                            |          LL_ENC_RSP |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |                            |      LL_VERSION_IND |
 *    |                            |<--------------------|
 *    |                            |                     |
 *    |     Encryption Start Proc. |                     |
 *    |                   Complete |                     |
 *    |<---------------------------|                     |
 *    |                            |                     |
 */
ZTEST(encryption_start, test_encryption_start_central_loc_mic)
{
	uint8_t err;
	struct node_tx *tx;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/**/
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL,
		      "Expected termination due to MIC failure");

	/*
	 * For a 40s procedure response timeout with a connection interval of
	 * 7.5ms, a total of 5333.33 connection events are needed, verify that
	 * the state doesn't change for that many invocations.
	 */
	for (int n = 5334; n > 0; n--) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* Check state */
		CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
		CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Note that for this test the context is not released */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt() - 1,
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_REQ    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Rx Decryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 *    |                       |    LL_START_ENC_RSP |
 *    |                       |<--------------------|
 *    |                       |                     |
 *    |     Encryption Change |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_RSP    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Tx Encryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 */
ZTEST(encryption_start, test_encryption_start_periph_rem)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	struct pdu_data_llctrl_enc_rsp exp_enc_rsp = {
		.skds = { SKDS },
		.ivs = { IVS },
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, &enc_req);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, &exp_enc_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn, ltk);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* CCM Rx SK should match SK */
	/* CCM Rx IV should match the IV */
	/* CCM Rx Counter should be zero */
	/* CCM Rx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Prepare */
	event_prepare(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* There should be a host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* CCM Tx SK should match SK */
	/* CCM Tx IV should match the IV */
	/* CCM Tx Counter should be zero */
	/* CCM Tx Direction should be S->M */
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |    -----------------\ |                     |
 *    |    | Reserver all   |-|                     |
 *    |    | Tx/Ntf buffers | |                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_REQ    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Rx Decryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 *    |                       |    LL_START_ENC_RSP |
 *    |                       |<--------------------|
 *    |                       |                     |
 *    |     Encryption Change |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    |                       | LL_START_ENC_RSP    |
 *    |                       |-------------------->|
 *    |     ----------------\ |                     |
 *    |     | Tx Encryption |-|                     |
 *    |     |---------------| |                     |
 *    |                       |                     |
 */
ZTEST(encryption_start, test_encryption_start_periph_rem_limited_memory)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;
	struct proc_ctx *ctx = NULL;

	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	struct pdu_data_llctrl_enc_rsp exp_enc_rsp = {
		.skds = { SKDS },
		.ivs = { IVS },
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Allocate dummy procedure used to steal all buffers */
	ctx = llcp_create_local_procedure(PROC_VERSION_EXCHANGE);

	/* Steal all tx buffers */
	while (llcp_tx_alloc_peek(&conn, ctx)) {
		tx = llcp_tx_alloc(&conn, ctx);
		zassert_not_null(tx, NULL);
	}

	/* Dummy remove, as above loop might queue up ctx */
	llcp_tx_alloc_unpeek(ctx);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, &enc_req);

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, &exp_enc_rsp);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release ntf */
	release_ntf(ntf);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn, ltk);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should not have one LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* CCM Rx SK should match SK */
	/* CCM Rx IV should match the IV */
	/* CCM Rx Counter should be zero */
	/* CCM Rx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should not have a LL Control PDU */
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, ENCRYPTED); /* Rx paused & enc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release tx */
	ull_cp_release_tx(&conn, tx);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* CCM Tx SK should match SK */
	/* CCM Tx IV should match the IV */
	/* CCM Tx Counter should be zero */
	/* CCM Tx Direction should be S->M */
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);

	/* Release dummy procedure */
	llcp_proc_ctx_release(ctx);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |           LTK Request |                     |
 *    |<----------------------|                     |
 *    |                       |                     |
 *    | LTK Request Reply     |                     |
 *    |---------------------->|                     |
 *    |                       |                     |
 *    |                       | LL_REJECT_EXT_IND   |
 *    |                       |-------------------->|
 */
ZTEST(encryption_start, test_encryption_start_periph_rem_no_ltk)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Prepare LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	struct pdu_data_llctrl_enc_rsp exp_enc_rsp = {
		.skds = { SKDS },
		.ivs = { IVS },
	};

	struct pdu_data_llctrl_reject_ext_ind reject_ext_ind = {
		.reject_opcode = PDU_DATA_LLCTRL_TYPE_ENC_REQ,
		.error_code = BT_HCI_ERR_PIN_OR_KEY_MISSING
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, &enc_req);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, &exp_enc_rsp);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_neq_reply(&conn);

	/* Check state */
	/* TODO(thoh): THIS IS WRONG! */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_REJECT_EXT_IND, &conn, &tx, &reject_ext_ind);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* All contexts should be released until now. This is a side-effect of a call to
	 * ull_cp_tx_ntf that internall calls rr_check_done and lr_check_done.
	 */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

/* +-----+                +-------+              +-----+
 * | UT  |                | LL_A  |              | LT  |
 * +-----+                +-------+              +-----+
 *    |                       |                     |
 *    |                       |          LL_ENC_REQ |
 *    |                       |<--------------------|
 *    |    -----------------\ |                     |
 *    |    | Empty Tx queue |-|                     |
 *    |    |----------------| |                     |
 *    |                       |                     |
 *    |                       | LL_ENC_RSP          |
 *    |                       |-------------------->|
 *    |                       |                     |
 *    |                       |      LL_VERSION_IND |
 *    |                       |<--------------------|
 *    |                       |                     |
 */
ZTEST(encryption_start, test_encryption_start_periph_rem_mic)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	/* Prepare LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	struct pdu_data_llctrl_enc_rsp exp_enc_rsp = {
		.skds = { SKDS },
		.ivs = { IVS },
	};

	struct pdu_data_llctrl_version_ind remote_version_ind = {
		.version_number = 0x55,
		.company_id = 0xABCD,
		.sub_version_number = 0x1234,
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, &enc_req);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Tx unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, &exp_enc_rsp);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_VERSION_IND, &conn, &remote_version_ind);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/**/
	zassert_equal(conn.llcp_terminate.reason_final, BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL,
		      "Expected termination due to MIC failure");

	/*
	 * For a 40s procedure response timeout with a connection interval of
	 * 7.5ms, a total of 5333.33 connection events are needed, verify that
	 * the state doesn't change for that many invocations.
	 */
	for (int n = 5334; n > 0; n--) {
		/* Prepare */
		event_prepare(&conn);

		/* Tx Queue should NOT have a LL Control PDU */
		lt_rx_q_is_empty(&conn);

		/* Done */
		event_done(&conn);

		/* Check state */
		CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
		CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

		/* There should NOT be a host notification */
		ut_rx_q_is_empty();
	}

	/* Note that for this test the context is not released */
	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt() - 1,
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}


ZTEST(encryption_pause, test_encryption_pause_central_loc)
{
	uint8_t err;
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t rand[] = { RAND };
	const uint8_t ediv[] = { EDIV };
	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare expected LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req exp_enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	/* Prepare LL_ENC_RSP */
	struct pdu_data_llctrl_enc_rsp enc_rsp = { .skds = { SKDS }, .ivs = { IVS } };

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm) + sizeof(exp_enc_req.ivm));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_CENTRAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Fake that encryption is already active */
	conn.lll.enc_rx = 1U;
	conn.lll.enc_tx = 1U;

	/**** ENCRYPTED ****/

	/* Initiate an Encryption Pause Procedure */
	err = ull_cp_encryption_pause(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PAUSE_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_PAUSE_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PAUSE_ENC_RSP, &conn, &tx, NULL);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.lll.enc_rx, 0U);

	/**** UNENCRYPTED ****/

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_ENC_RSP, &conn, &enc_rsp);

	/* Rx */
	lt_tx(LL_START_ENC_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* CCM Tx/Rx SK should match SK */
	/* CCM Tx/Rx IV should match the IV */
	/* CCM Tx/Rx Counter should be zero */
	/* CCM Rx Direction should be S->M */
	/* CCM Tx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_ENC_REFRESH, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST(encryption_pause, test_encryption_pause_periph_rem)
{
	struct node_tx *tx;
	struct node_rx_pdu *ntf;

	const uint8_t ltk[] = { LTK };
	const uint8_t skd[] = { SKDM, SKDS };
	const uint8_t sk_be[] = { SK_BE };
	const uint8_t iv[] = { IVM, IVS };

	/* Prepare LL_ENC_REQ */
	struct pdu_data_llctrl_enc_req enc_req = {
		.rand = { RAND },
		.ediv = { EDIV },
		.skdm = { SKDM },
		.ivm = { IVM },
	};

	struct pdu_data_llctrl_enc_rsp exp_enc_rsp = {
		.skds = { SKDS },
		.ivs = { IVS },
	};

	/* Prepare mocked call to lll_csrand_get */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds) + sizeof(exp_enc_rsp.ivs));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Fake that encryption is already active */
	conn.lll.enc_rx = 1U;
	conn.lll.enc_tx = 1U;

	/**** ENCRYPTED ****/

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_PAUSE_ENC_REQ, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PAUSE_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.lll.enc_rx, 0U);

	/* Rx */
	lt_tx(LL_PAUSE_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U);

	/**** UNENCRYPTED ****/

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_ENC_REQ, &conn, &enc_req);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_RSP, &conn, &tx, &exp_enc_rsp);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	release_ntf(ntf);

	/* LTK request reply */
	ull_cp_ltk_req_reply(&conn, ltk);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* CCM Rx SK should match SK */
	/* CCM Rx IV should match the IV */
	/* CCM Rx Counter should be zero */
	/* CCM Rx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U);

	/* Prepare */
	event_prepare(&conn);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* There should be a host notification */
	ut_rx_node(NODE_ENC_REFRESH, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_START_ENC_RSP, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(&conn, tx);

	/* CCM Tx SK should match SK */
	/* CCM Tx IV should match the IV */
	/* CCM Tx Counter should be zero */
	/* CCM Tx Direction should be S->M */
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U);

	zassert_equal(llcp_ctx_buffers_free(), test_ctx_buffers_cnt(),
				  "Free CTX buffers %d", llcp_ctx_buffers_free());
}

ZTEST_SUITE(encryption_start, NULL, NULL, enc_setup, NULL, NULL);
ZTEST_SUITE(encryption_pause, NULL, NULL, enc_setup, NULL, NULL);

/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

#define ULL_LLCP_UNITTEST

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_df_types.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"

#include "ull_internal.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
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
#define CHECK_RX_PE_STATE(_conn, _pause, _enc)                                                     \
	do {                                                                                       \
		zassert_equal(_conn.pause_rx_data, _pause, "Rx Data pause state is wrong.");       \
		zassert_equal(_conn.lll.enc_rx, _enc, "Rx Encryption state is wrong.");            \
	} while (0)

/* Check Tx Pause and Encryption state */
#define CHECK_TX_PE_STATE(_conn, _pause, _enc)                                                     \
	do {                                                                                       \
		zassert_equal(_conn.tx_q.pause_data, _pause, "Tx Data pause state is wrong.");     \
		zassert_equal(_conn.lll.enc_tx, _enc, "Tx Encryption state is wrong.");            \
	} while (0)

/* CCM direction flag */
#define CCM_DIR_M_TO_S 1U
#define CCM_DIR_S_TO_M 0U

/* Check Rx CCM state */
#define CHECK_RX_CCM_STATE(_conn, _sk_be, _iv, _cnt, _dir)                                         \
	do {                                                                                       \
		zassert_mem_equal(_conn.lll.ccm_rx.key, _sk_be, sizeof(_sk_be),                    \
				  "CCM Rx SK not equal to expected SK");                           \
		zassert_mem_equal(_conn.lll.ccm_rx.iv, _iv, sizeof(_iv),                           \
				  "CCM Rx IV not equal to (IVm | IVs)");                           \
		zassert_equal(_conn.lll.ccm_rx.counter, _cnt, "CCM Rx Counter is wrong");          \
		zassert_equal(_conn.lll.ccm_rx.direction, _dir, "CCM Rx Direction is wrong");      \
	} while (0)

/* Check Tx CCM state */
#define CHECK_TX_CCM_STATE(_conn, _sk_be, _iv, _cnt, _dir)                                         \
	do {                                                                                       \
		zassert_mem_equal(_conn.lll.ccm_tx.key, _sk_be, sizeof(_sk_be),                    \
				  "CCM Tx SK not equal to expected SK");                           \
		zassert_mem_equal(_conn.lll.ccm_tx.iv, _iv, sizeof(_iv),                           \
				  "CCM Tx IV not equal to (IVm | IVs)");                           \
		zassert_equal(_conn.lll.ccm_tx.counter, _cnt, "CCM Tx Counter is wrong");          \
		zassert_equal(_conn.lll.ccm_tx.direction, _dir, "CCM Tx Direction is wrong");      \
	} while (0)

struct ll_conn conn;

static void setup(void)
{
	test_setup(&conn);
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
#define LTK                                                                                        \
	0x4C, 0x68, 0x38, 0x41, 0x39, 0xF5, 0x74, 0xD8, 0x36, 0xBC, 0xF3, 0x4E, 0x9D, 0xFB, 0x01,  \
		0xBF
#define SKDM 0xAC, 0xBD, 0xCE, 0xDF, 0xE0, 0xF1, 0x02, 0x13
#define SKDS 0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68, 0x79
#define IVM 0xBA, 0xDC, 0xAB, 0x24
#define IVS 0xDE, 0xAF, 0xBA, 0xBE

#define SK_BE                                                                                      \
	0x66, 0xC6, 0xC2, 0x27, 0x8E, 0x3B, 0x8E, 0x05, 0x3E, 0x7E, 0xA3, 0x26, 0x52, 0x1B, 0xAD,  \
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
void test_encryption_start_mas_loc(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm));
	/* Second call for IVm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.ivm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.ivm));

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
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(tx);

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
	ull_cp_release_tx(tx);

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
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_mas_loc_limited_memory(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm));
	/* Second call for IVm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.ivm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.ivm));

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

	/* Steal all tx buffers */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx = llcp_tx_alloc();
		zassert_not_null(tx, NULL);
	}

	/* Steal all ntf buffers */
	while (ll_pdu_rx_alloc_peek(1)) {
		ntf = ll_pdu_rx_alloc();
		/* Make sure we use a correct type or the release won't work */
		ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	}

	/* Initiate an Encryption Start Procedure */
	err = ull_cp_encryption_start(&conn, rand, ediv, ltk);
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

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
	ull_cp_release_tx(tx);

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
	ull_cp_release_tx(tx);

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
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* There should be no host notifications */
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* Done */
	event_done(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Rx enc. */
	CHECK_TX_PE_STATE(conn, RESUMED, ENCRYPTED); /* Tx enc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_mas_loc_no_ltk(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm));
	/* Second call for IVm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.ivm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.ivm));

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
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(tx);

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
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_mas_loc_no_ltk_2(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm));
	/* Second call for IVm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.ivm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.ivm));

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
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, RESUMED, UNENCRYPTED); /* Rx unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* Release Tx */
	ull_cp_release_tx(tx);

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
	ull_cp_release_ntf(ntf);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_sla_rem(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds));
	/* Second call for IVs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.ivs);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.ivs));

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
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

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
	ull_cp_release_tx(tx);

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
	ull_cp_release_tx(tx);

	/* CCM Tx SK should match SK */
	/* CCM Tx IV should match the IV */
	/* CCM Tx Counter should be zero */
	/* CCM Tx Direction should be S->M */
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_sla_rem_limited_memory(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds));
	/* Second call for IVs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.ivs);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.ivs));

	/* Prepare mocked call to ecb_encrypt */
	ztest_expect_data(ecb_encrypt, key_le, ltk);
	ztest_expect_data(ecb_encrypt, clear_text_le, skd);
	ztest_return_data(ecb_encrypt, cipher_text_be, sk_be);

	/* Role */
	test_set_role(&conn, BT_HCI_ROLE_PERIPHERAL);

	/* Connect */
	ull_cp_state_set(&conn, ULL_CP_CONNECTED);

	/* Steal all tx buffers */
	for (int i = 0U; i < TX_CTRL_BUF_NUM; i++) {
		tx = llcp_tx_alloc();
		zassert_not_null(tx, NULL);
	}

	/* Steal all ntf buffers */
	while (ll_pdu_rx_alloc_peek(1)) {
		ntf = ll_pdu_rx_alloc();
		/* Make sure we use a correct type or the release won't work */
		ntf->hdr.type = NODE_RX_TYPE_DC_PDU;
	}

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
	ull_cp_release_tx(tx);

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

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* Check state */
	CHECK_RX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Rx paused & unenc. */
	CHECK_TX_PE_STATE(conn, PAUSED, UNENCRYPTED); /* Tx paused & unenc. */

	/* There should be one host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

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
	ull_cp_release_tx(tx);

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

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Release ntf */
	ull_cp_release_ntf(ntf);

	/* Prepare */
	event_prepare(&conn);

	/* There should be one host notification */
	ut_rx_pdu(LL_START_ENC_RSP, &ntf, NULL);
	ut_rx_q_is_empty();

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
	ull_cp_release_tx(tx);

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

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
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
void test_encryption_start_sla_rem_no_ltk(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds));
	/* Second call for IVs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.ivs);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.ivs));

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
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

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
	ull_cp_release_tx(tx);

	/* There should not be a host notification */
	ut_rx_q_is_empty();

	/* Note that for this test the context is not released */
	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM - 1, "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_encryption_pause_mas_loc(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.skdm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.skdm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.skdm));
	/* Second call for IVm */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_req.ivm));
	ztest_return_data(lll_csrand_get, buf, exp_enc_req.ivm);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_req.ivm));

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
	zassert_equal(err, BT_HCI_ERR_SUCCESS, NULL);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PAUSE_ENC_REQ, &conn, &tx, NULL);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_PAUSE_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Prepare */
	event_prepare(&conn);

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_PAUSE_ENC_RSP, &conn, &tx, NULL);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U, NULL);

	/* Rx Decryption should be disabled */
	zassert_equal(conn.lll.enc_rx, 0U, NULL);

	/**** UNENCRYPTED ****/

	/* Tx Queue should have one LL Control PDU */
	lt_rx(LL_ENC_REQ, &conn, &tx, &exp_enc_req);
	lt_rx_q_is_empty(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

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
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Rx */
	lt_tx(LL_START_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* There should be one host notification */
	ut_rx_node(NODE_ENC_REFRESH, &ntf, NULL);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_encryption_pause_sla_rem(void)
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

	/* Prepare mocked call(s) to lll_csrand_get */
	/* First call for SKDs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.skds));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.skds);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.skds));
	/* Second call for IVs */
	ztest_returns_value(lll_csrand_get, sizeof(exp_enc_rsp.ivs));
	ztest_return_data(lll_csrand_get, buf, exp_enc_rsp.ivs);
	ztest_expect_value(lll_csrand_get, len, sizeof(exp_enc_rsp.ivs));

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
	zassert_equal(conn.lll.enc_rx, 0U, NULL);

	/* Rx */
	lt_tx(LL_PAUSE_ENC_RSP, &conn, NULL);

	/* Done */
	event_done(&conn);

	/* Release Tx */
	ull_cp_release_tx(tx);

	/* Tx Encryption should be disabled */
	zassert_equal(conn.lll.enc_tx, 0U, NULL);

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
	ull_cp_release_tx(tx);

	/* There should be a host notification */
	ut_rx_pdu(LL_ENC_REQ, &ntf, &enc_req);
	ut_rx_q_is_empty();

	/* Release Ntf */
	ull_cp_release_ntf(ntf);

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
	ull_cp_release_tx(tx);

	/* CCM Rx SK should match SK */
	/* CCM Rx IV should match the IV */
	/* CCM Rx Counter should be zero */
	/* CCM Rx Direction should be M->S */
	CHECK_RX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_M_TO_S);

	/* Rx Decryption should be enabled */
	zassert_equal(conn.lll.enc_rx, 1U, NULL);

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
	ull_cp_release_tx(tx);

	/* CCM Tx SK should match SK */
	/* CCM Tx IV should match the IV */
	/* CCM Tx Counter should be zero */
	/* CCM Tx Direction should be S->M */
	CHECK_TX_CCM_STATE(conn, sk_be, iv, 0U, CCM_DIR_S_TO_M);

	/* Tx Encryption should be enabled */
	zassert_equal(conn.lll.enc_tx, 1U, NULL);

	zassert_equal(ctx_buffers_free(), PROC_CTX_BUF_NUM, "Free CTX buffers %d",
		      ctx_buffers_free());
}

void test_main(void)
{
	ztest_test_suite(
		encryption_start,
		ztest_unit_test_setup_teardown(test_encryption_start_mas_loc, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_mas_loc_limited_memory, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_mas_loc_no_ltk, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_mas_loc_no_ltk_2, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_sla_rem, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_sla_rem_limited_memory, setup,
					       unit_test_noop),
		ztest_unit_test_setup_teardown(test_encryption_start_sla_rem_no_ltk, setup,
					       unit_test_noop));

	ztest_test_suite(encryption_pause,
			 ztest_unit_test_setup_teardown(test_encryption_pause_mas_loc, setup,
							unit_test_noop),
			 ztest_unit_test_setup_teardown(test_encryption_pause_sla_rem, setup,
							unit_test_noop));

	ztest_run_test_suite(encryption_start);
	ztest_run_test_suite(encryption_pause);
}

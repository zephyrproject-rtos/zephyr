/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include "kconfig.h"

/* Kconfig Cheats */

#include <bluetooth/hci.h>
#include <sys/byteorder.h>
#include <sys/slist.h>
#include <sys/util.h>
#include "hal/ccm.h"

#include "util/mem.h"
#include "util/memq.h"

#include "pdu.h"
#include "ll.h"
#include "ll_settings.h"

#include "lll.h"
#include "lll_conn.h"

#include "ull_tx_queue.h"
#include "ull_conn_types.h"
#include "ull_llcp.h"
#include "ull_llcp_internal.h"

#include "helper_pdu.h"
#include "helper_util.h"

struct ll_conn conn;

/*
 * Note API and internal test are not yet split out here
 */

void test_api_init(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);

	ll_conn_init(&conn);

	zassert_true(lr_is_disconnected(&conn), NULL);
	zassert_true(rr_is_disconnected(&conn), NULL);
}

extern void test_int_mem_proc_ctx(void);
extern void test_int_mem_tx(void);
extern void test_int_mem_ntf(void);
extern void test_int_create_proc(void);
extern void test_int_local_pending_requests(void);
extern void test_int_remote_pending_requests(void);



void test_api_connect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ll_conn_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(lr_is_idle(&conn), NULL);
	zassert_true(rr_is_idle(&conn), NULL);
}

void test_api_disconnect(void)
{
	ull_cp_init();
	ull_tx_q_init(&conn.tx_q);
	ll_conn_init(&conn);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(lr_is_disconnected(&conn), NULL);
	zassert_true(rr_is_disconnected(&conn), NULL);

	ull_cp_state_set(&conn, ULL_CP_CONNECTED);
	zassert_true(lr_is_idle(&conn), NULL);
	zassert_true(rr_is_idle(&conn), NULL);

	ull_cp_state_set(&conn, ULL_CP_DISCONNECTED);
	zassert_true(lr_is_disconnected(&conn), NULL);
	zassert_true(rr_is_disconnected(&conn), NULL);
}

void test_main(void)
{
	ztest_test_suite(internal,
			 ztest_unit_test(test_int_mem_proc_ctx),
			 ztest_unit_test(test_int_mem_tx),
			 ztest_unit_test(test_int_mem_ntf),
			 ztest_unit_test(test_int_create_proc),
			 ztest_unit_test(test_int_local_pending_requests),
			 ztest_unit_test(test_int_remote_pending_requests)
			);

	ztest_test_suite(public,
			 ztest_unit_test(test_api_init),
			 ztest_unit_test(test_api_connect),
			 ztest_unit_test(test_api_disconnect)
			);

	ztest_run_test_suite(internal);
	ztest_run_test_suite(public);
}

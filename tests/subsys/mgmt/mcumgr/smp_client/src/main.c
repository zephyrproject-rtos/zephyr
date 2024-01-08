/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <mgmt/mcumgr/transport/smp_internal.h>
#include "smp_transport_stub.h"

static struct net_buf *buf[5];

static uint32_t testing_user_data;
static void *response_ptr;
static struct net_buf *res_buf;
static struct smp_client_object smp_client;

int smp_client_res_cb(struct net_buf *nb, void *user_data)
{
	res_buf = nb;
	response_ptr = user_data;
	return 0;
}

ZTEST(smp_client, test_buf_alloc)
{
	struct smp_client_object smp_client;

	/* Allocate all 4 buffer's and verify that 5th fail */
	for (int i = 0; i < 5; i++) {
		buf[i] = smp_client_buf_allocation(&smp_client, MGMT_GROUP_ID_IMAGE, 1,
						   MGMT_OP_WRITE, SMP_MCUMGR_VERSION_1);
		if (i == 4) {
			zassert_is_null(buf[i], "Buffer was not Null");
		} else {
			zassert_not_null(buf[i], "Buffer was Null");
			zassert_equal(sizeof(struct smp_hdr), buf[i]->len,
				      "Expected to receive %d response %d",
				      sizeof(struct smp_hdr), buf[i]->len);
		}
	}

	for (int i = 0; i < 4; i++) {
		smp_client_buf_free(buf[i]);
		buf[i] = NULL;
	}
}

ZTEST(smp_client, test_msg_send_timeout)
{
	struct net_buf *nb;

	int rc;

	nb = smp_client_buf_allocation(&smp_client, MGMT_GROUP_ID_IMAGE, 1, MGMT_OP_WRITE,
				       SMP_MCUMGR_VERSION_1);
	zassert_not_null(nb, "Buffer was Null");
	rc = smp_client_send_cmd(&smp_client, nb, smp_client_res_cb, &testing_user_data, 2);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	k_sleep(K_SECONDS(3));
	zassert_is_null(res_buf, "NULL pointer was not returned");
	zassert_equal_ptr(response_ptr, &testing_user_data, "User data not returned correctly");
}

ZTEST(smp_client, test_msg_response_handler)
{
	struct smp_hdr dst_hdr;
	int rc;


	response_ptr = NULL;
	res_buf = NULL;

	buf[0] = smp_client_buf_allocation(&smp_client, MGMT_GROUP_ID_IMAGE, 1, MGMT_OP_WRITE,
					   SMP_MCUMGR_VERSION_1);
	zassert_not_null(buf[0], "Buffer was Null");
	rc = smp_client_send_cmd(&smp_client, buf[0], smp_client_res_cb, &testing_user_data, 8);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	buf[1] = smp_client_buf_allocation(&smp_client, MGMT_GROUP_ID_IMAGE, 1, MGMT_OP_WRITE,
					   SMP_MCUMGR_VERSION_1);
	zassert_not_null(buf[0], "Buffer was Null");
	/* Read Pushed packet Header */
	smp_transport_read_hdr(buf[0], &dst_hdr);
	smp_client_single_response(buf[1], &dst_hdr);
	zassert_is_null(res_buf, "NULL pointer was not returned");
	zassert_is_null(response_ptr, "NULL pointer was not returned");
	/* Set Correct OP */
	dst_hdr.nh_op = MGMT_OP_WRITE_RSP;
	smp_client_single_response(buf[1], &dst_hdr);
	zassert_equal_ptr(res_buf, buf[1], "Response Buf not correct");
	zassert_equal_ptr(response_ptr, &testing_user_data, "User data not returned correctly");
	response_ptr = NULL;
	res_buf = NULL;
	smp_client_single_response(buf[1], &dst_hdr);
	zassert_is_null(res_buf, "NULL pointer was not returned");
	zassert_is_null(response_ptr, "NULL pointer was not returned");
}

static void *setup_custom_os(void)
{
	/* Registre tarnsport and init client */
	stub_smp_client_transport_register();
	smp_client_object_init(&smp_client, SMP_SERIAL_TRANSPORT);
	return NULL;
}

static void cleanup_test(void *p)
{
	for (int i = 0; i < 5; i++) {
		if (buf[i]) {
			smp_client_buf_free(buf[i]);
			buf[i] = NULL;
		}
	}
}

/* Main test set */
ZTEST_SUITE(smp_client, NULL, setup_custom_os, NULL, cleanup_test, NULL);

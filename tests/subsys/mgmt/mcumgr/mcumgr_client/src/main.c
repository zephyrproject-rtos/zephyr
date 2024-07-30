/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <string.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt_client.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt_client.h>
/* Include stubbed test helpers */
#include "smp_stub.h"
#include "img_gr_stub.h"
#include "os_gr_stub.h"

/* IMG group data */
static uint8_t image_hash[32];
static struct mcumgr_image_data image_info[2];
static uint8_t image_dummy[1024];

static const char os_echo_test[] = "TestString";
static struct smp_client_object smp_client;
static struct img_mgmt_client img_client;
static struct os_mgmt_client os_client;

ZTEST(mcumgr_client, test_img_upload)
{
	int rc;
	struct mcumgr_image_upload response;

	smp_stub_set_rx_data_verify(NULL);
	img_upload_stub_init();

	img_mgmt_client_init(&img_client, &smp_client, 2, image_info);

	rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, image_hash);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	/* Start upload  and test Timeout */
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	zassert_equal(MGMT_ERR_ETIMEOUT, response.status, "Expected to receive %d response %d\n",
		      MGMT_ERR_ETIMEOUT, response.status);
	rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, image_hash);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, image_hash);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	/* Start upload  and test Timeout */
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	zassert_equal(MGMT_ERR_ETIMEOUT, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, response.status);

	smp_client_send_status_stub(MGMT_ERR_EOK);

	/* Allocate response buf */
	img_upload_response(0, MGMT_ERR_EINVAL);
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_EINVAL, rc, "Expected to receive %d response %d", MGMT_ERR_EINVAL,
		      rc);
	zassert_equal(MGMT_ERR_EINVAL, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_EINVAL, response.status);
	rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, image_hash);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d\n", MGMT_ERR_EOK, rc);
	img_upload_response(1024, MGMT_ERR_EOK);

	smp_stub_set_rx_data_verify(img_upload_init_verify);
	img_upload_stub_init();
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(MGMT_ERR_EOK, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_EOK, response.status);
	zassert_equal(1024, response.image_upload_offset,
		      "Expected to receive offset %d response %d", 1024,
		      response.image_upload_offset);
	/* Send last frame from image */
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(MGMT_ERR_EOK, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_EOK, response.status);
	zassert_equal(TEST_IMAGE_SIZE, response.image_upload_offset,
		      "Expected to receive offset %d response %d", TEST_IMAGE_SIZE,
		      response.image_upload_offset);

	/* Test without hash */
	rc = img_mgmt_client_upload_init(&img_client, TEST_IMAGE_SIZE, TEST_IMAGE_NUM, NULL);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	img_upload_stub_init();
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(MGMT_ERR_EOK, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_EOK, response.status);
	zassert_equal(1024, response.image_upload_offset,
		      "Expected to receive offset %d response %d", 1024,
		      response.image_upload_offset);
	/* Send last frame from image */
	rc = img_mgmt_client_upload(&img_client, image_dummy, 1024, &response);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(MGMT_ERR_EOK, response.status, "Expected to receive %d response %d",
		      MGMT_ERR_EOK, response.status);
	zassert_equal(TEST_IMAGE_SIZE, response.image_upload_offset,
		      "Expected to receive offset %d response %d", TEST_IMAGE_SIZE,
		      response.image_upload_offset);
}

ZTEST(mcumgr_client, test_img_erase)
{
	int rc;

	smp_client_send_status_stub(MGMT_ERR_EOK);

	/* Test timeout */
	rc = img_mgmt_client_erase(&img_client, TEST_SLOT_NUMBER);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);

	/* Test erase fail */
	img_erase_response(MGMT_ERR_EINVAL);
	rc = img_mgmt_client_erase(&img_client, TEST_SLOT_NUMBER);
	zassert_equal(MGMT_ERR_EINVAL, rc, "Expected to receive %d response %d", MGMT_ERR_EINVAL,
		      rc);
	/* Tesk ok */
	img_erase_response(MGMT_ERR_EOK);
	rc = img_mgmt_client_erase(&img_client, TEST_SLOT_NUMBER);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
}

ZTEST(mcumgr_client, test_image_state_read)
{
	int rc;
	struct mcumgr_image_state res_buf;

	smp_client_send_status_stub(MGMT_ERR_EOK);
	/* Test timeout */
	rc = img_mgmt_client_state_read(&img_client, &res_buf);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	/* Testing read successfully 1 image info and print that */
	img_read_response(1);
	rc = img_mgmt_client_state_read(&img_client, &res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(1, res_buf.image_list_length, "Expected to receive %d response %d", 1,
		      res_buf.image_list_length);
	img_read_response(2);
	rc = img_mgmt_client_state_read(&img_client, &res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
}

ZTEST(mcumgr_client, test_image_state_set)
{
	int rc;
	char hash[32];
	struct mcumgr_image_state res_buf;

	smp_client_response_buf_clean();
	smp_stub_set_rx_data_verify(NULL);
	smp_client_send_status_stub(MGMT_ERR_EOK);
	/* Test timeout */
	rc = img_mgmt_client_state_write(&img_client, NULL, false, &res_buf);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);

	printf("Timeout OK\r\n");
	/* Read secondary image hash for testing */
	img_read_response(2);
	rc = img_mgmt_client_state_read(&img_client, &res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	zassert_equal(false, image_info[1].flags.pending, "Expected to receive %d response %d",
		      false, image_info[1].flags.pending);
	/* Copy hash for set pending flag */
	memcpy(hash, image_info[1].hash, 32);
	printf("Read OK\r\n");

	/* Read secondary image hash for testing */
	smp_stub_set_rx_data_verify(img_state_write_verify);
	rc = img_mgmt_client_state_write(&img_client, hash, false, &res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	zassert_equal(true, image_info[1].flags.pending, "Expected to receive %d response %d",
		      true, image_info[1].flags.pending);
	/* Test to set confirmed bit  */
	image_info[0].flags.confirmed = false;
	smp_stub_set_rx_data_verify(img_state_write_verify);
	rc = img_mgmt_client_state_write(&img_client, NULL, true, &res_buf);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
	zassert_equal(2, res_buf.image_list_length, "Expected to receive %d response %d", 2,
		      res_buf.image_list_length);
	zassert_equal(true, image_info[0].flags.confirmed, "Expected to receive %d response %d",
		      true, image_info[0].flags.confirmed);
}

ZTEST(mcumgr_client, test_os_reset)
{
	int rc;

	smp_client_response_buf_clean();
	smp_stub_set_rx_data_verify(NULL);

	smp_client_send_status_stub(MGMT_ERR_EOK);
	/* Test timeout */
	rc = os_mgmt_client_reset(&os_client);
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	/* Testing reset successfully handling */
	os_reset_response();
	rc = os_mgmt_client_reset(&os_client);
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
}

ZTEST(mcumgr_client, test_os_echo)
{
	int rc;

	smp_client_response_buf_clean();
	smp_stub_set_rx_data_verify(NULL);
	smp_client_send_status_stub(MGMT_ERR_EOK);
	/* Test timeout */
	rc = os_mgmt_client_echo(&os_client, os_echo_test, sizeof(os_echo_test));
	zassert_equal(MGMT_ERR_ETIMEOUT, rc, "Expected to receive %d response %d",
		      MGMT_ERR_ETIMEOUT, rc);
	/* Test successfully operation */
	smp_stub_set_rx_data_verify(os_echo_verify);
	rc = os_mgmt_client_echo(&os_client, os_echo_test, sizeof(os_echo_test));
	zassert_equal(MGMT_ERR_EOK, rc, "Expected to receive %d response %d", MGMT_ERR_EOK, rc);
}

static void *setup_custom_os(void)
{
	stub_smp_client_transport_register();
	smp_client_object_init(&smp_client, SMP_SERIAL_TRANSPORT);
	os_mgmt_client_init(&os_client, &smp_client);
	img_mgmt_client_init(&img_client, &smp_client, 2, image_info);

	img_gr_stub_data_init(image_hash);
	os_stub_init(os_echo_test);
	return NULL;
}

static void cleanup_test(void *p)
{
	smp_client_response_buf_clean();
	smp_stub_set_rx_data_verify(NULL);
}

/* Main test set */
ZTEST_SUITE(mcumgr_client, NULL, setup_custom_os, NULL, cleanup_test, NULL);

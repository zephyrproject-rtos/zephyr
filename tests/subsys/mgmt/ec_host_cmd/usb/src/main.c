/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/ztest.h>

#define EP_OUT_ADDR             0x01
#define EP_IN_BULK_ADDR         0x81
#define EP_IN_INT_ADDR          0x82
#define CMD_TEST_ID             0x0020
#define EC_HOST_REQUEST_VERSION 3

struct test_req_payload {
	uint8_t val;
} __packed;

struct test_resp_payload {
	uint8_t val_doubled;
} __packed;

static struct usbd_class_data *test_c_data;
static struct net_buf *mock_ep_out_buf;
static struct net_buf *mock_ep_in_buf;
static struct net_buf *mock_ep_int_buf;
static int mock_ep_out_enqueue_count;
static struct k_sem handler_done_sem;

static struct net_buf **get_ep_buf_slot(uint8_t ep)
{
	switch (ep) {
	case EP_OUT_ADDR:
		return &mock_ep_out_buf;
	case EP_IN_BULK_ADDR:
		return &mock_ep_in_buf;
	case EP_IN_INT_ADDR:
		return &mock_ep_int_buf;
	default:
		return NULL;
	}
}

int usbd_ep_enqueue(const struct usbd_class_data *const c_data, struct net_buf *const buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct net_buf **slot = get_ep_buf_slot(bi->ep);

	ARG_UNUSED(c_data);
	if (slot != NULL) {
		*slot = buf;
	}
	if (bi->ep == EP_OUT_ADDR) {
		mock_ep_out_enqueue_count++;
	} else if (bi->ep == EP_IN_INT_ADDR) {
		/* EP_IN_INT_ADDR is queued last; both response buffers are ready. */
		k_sem_give(&handler_done_sem);
	}
	return 0;
}

int usbd_ep_dequeue(struct usbd_context *uds_ctx, const uint8_t ep)
{
	struct net_buf **slot = get_ep_buf_slot(ep);

	ARG_UNUSED(uds_ctx);
	if (slot != NULL && *slot != NULL) {
		struct net_buf *buf = *slot;

		*slot = NULL;
		test_c_data->api->request(test_c_data, buf, -ECONNABORTED);
	}
	return 0;
}

int usbd_ep_buf_free(struct usbd_context *uds_ctx, struct net_buf *buf)
{
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct net_buf **slot = get_ep_buf_slot(bi->ep);

	ARG_UNUSED(uds_ctx);
	if (slot != NULL && *slot == buf) {
		*slot = NULL;
	}
	net_buf_unref(buf);
	return 0;
}

bool usbd_is_suspended(struct usbd_context *uds_ctx)
{
	ARG_UNUSED(uds_ctx);
	return false;
}

int usbd_wakeup_request(struct usbd_context *uds_ctx)
{
	ARG_UNUSED(uds_ctx);
	return 0;
}

static uint8_t calc_checksum(const uint8_t *buf, size_t len)
{
	uint8_t sum = 0;

	for (size_t i = 0; i < len; i++) {
		sum += buf[i];
	}
	return (uint8_t)(-sum);
}

static enum ec_host_cmd_status test_cmd_handler(struct ec_host_cmd_handler_args *args)
{
	const struct test_req_payload *req = args->input_buf;
	struct test_resp_payload *resp = args->output_buf;

	resp->val_doubled = (uint8_t)(req->val * 2U);
	args->output_buf_size = sizeof(*resp);
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(CMD_TEST_ID, test_cmd_handler, BIT(0), struct test_req_payload,
		    struct test_resp_payload);

static void send_usb_host_cmd_request(uint8_t prtcl_ver, uint16_t cmd_id,
				      const void *payload, uint16_t payload_len)
{
	struct ec_host_cmd_request_header hdr = {
		.prtcl_ver = prtcl_ver,
		.cmd_id = cmd_id,
		.data_len = payload_len,
	};
	struct net_buf *out_buf = mock_ep_out_buf;
	size_t total_len = sizeof(hdr) + payload_len;

	zassert_not_null(out_buf, "EP OUT must be armed before sending a request");
	memcpy(out_buf->data, &hdr, sizeof(hdr));
	if (payload_len > 0) {
		memcpy(out_buf->data + sizeof(hdr), payload, payload_len);
	}
	((struct ec_host_cmd_request_header *)out_buf->data)->checksum =
		calc_checksum(out_buf->data, total_len);
	out_buf->len = total_len;
	zassert_ok(test_c_data->api->request(test_c_data, out_buf, 0));
}

static void complete_usb_in_transfers(enum ec_host_cmd_status expected_result,
				      const void *expected_payload,
				      size_t expected_payload_len)
{
	const struct ec_host_cmd_response_header *resp_hdr;

	zassert_ok(k_sem_take(&handler_done_sem, K_SECONDS(1)));
	zassert_not_null(mock_ep_in_buf, "Expected response on Bulk IN endpoint");
	zassert_not_null(mock_ep_int_buf, "Expected interrupt notification on INT IN endpoint");

	resp_hdr = (const struct ec_host_cmd_response_header *)mock_ep_in_buf->data;
	zassert_equal(resp_hdr->result, expected_result);
	zassert_equal(resp_hdr->data_len, expected_payload_len);
	if (expected_payload_len > 0) {
		zassert_mem_equal(mock_ep_in_buf->data + sizeof(*resp_hdr),
				  expected_payload, expected_payload_len);
	}

	zassert_ok(test_c_data->api->request(test_c_data, mock_ep_int_buf, 0));
	zassert_ok(test_c_data->api->request(test_c_data, mock_ep_in_buf, 0));
}

static void *usb_test_setup(void)
{
	struct ec_host_cmd_backend *backend = ec_host_cmd_backend_get_usb();

	test_c_data = *(struct usbd_class_data **)backend->ctx;
	zassert_not_null(test_c_data);
	k_sem_init(&handler_done_sem, 0, 1);
	zassert_ok(ec_host_cmd_init(backend));
	return NULL;
}

static void usb_test_before(void *fixture)
{
	ARG_UNUSED(fixture);
	k_sem_reset(&handler_done_sem);
	usbd_ep_dequeue(NULL, EP_OUT_ADDR);
	usbd_ep_dequeue(NULL, EP_IN_BULK_ADDR);
	usbd_ep_dequeue(NULL, EP_IN_INT_ADDR);
	test_c_data->api->disable(test_c_data);
	mock_ep_out_enqueue_count = 0;
	test_c_data->api->enable(test_c_data);
	zassert_equal(mock_ep_out_enqueue_count, 1);
	zassert_not_null(mock_ep_out_buf);
}

ZTEST(ec_host_cmd_usb, test_usb_valid_request_succeeds)
{
	const struct test_req_payload req = { .val = 0x15 };
	const struct test_resp_payload expected_resp = { .val_doubled = req.val * 2U };

	send_usb_host_cmd_request(EC_HOST_REQUEST_VERSION, CMD_TEST_ID, &req, sizeof(req));
	complete_usb_in_transfers(EC_HOST_CMD_SUCCESS, &expected_resp, sizeof(expected_resp));
	zassert_equal(mock_ep_out_enqueue_count, 2);
	zassert_not_null(mock_ep_out_buf);
}

ZTEST(ec_host_cmd_usb, test_usb_short_header_triggers_reset)
{
	const size_t short_header_len = sizeof(struct ec_host_cmd_request_header) - 1;
	struct net_buf *out_buf = mock_ep_out_buf;

	zassert_not_null(out_buf, "EP OUT must be armed");
	memset(out_buf->data, 0, short_header_len);
	out_buf->len = short_header_len;
	zassert_ok(test_c_data->api->request(test_c_data, out_buf, 0));

	/* Wait for reset_work to execute and re-arm the OUT endpoint */
	zassert_true(WAIT_FOR(mock_ep_out_enqueue_count == 2, 1000 * USEC_PER_MSEC, k_msleep(1)),
		     "Timeout waiting for reset_work to re-arm EP OUT");

	zassert_is_null(mock_ep_in_buf);
	zassert_not_null(mock_ep_out_buf);
}

ZTEST(ec_host_cmd_usb, test_usb_exact_header_fail_truncated)
{
	send_usb_host_cmd_request(EC_HOST_REQUEST_VERSION, CMD_TEST_ID, NULL, 0);
	complete_usb_in_transfers(EC_HOST_CMD_REQUEST_TRUNCATED, NULL, 0);
	zassert_equal(mock_ep_out_enqueue_count, 2);
	zassert_not_null(mock_ep_out_buf);
}

static void verify_invalid_protocol_version(uint8_t version)
{
	send_usb_host_cmd_request(version, CMD_TEST_ID, NULL, 0);
	complete_usb_in_transfers(EC_HOST_CMD_INVALID_HEADER, NULL, 0);
	zassert_equal(mock_ep_out_enqueue_count, 2);
	zassert_not_null(mock_ep_out_buf);
}

ZTEST(ec_host_cmd_usb, test_usb_invalid_prtcl_ver_v2_fail_invalid_header)
{
	verify_invalid_protocol_version(2);
}

ZTEST(ec_host_cmd_usb, test_usb_invalid_prtcl_ver_v4_fail_invalid_header)
{
	verify_invalid_protocol_version(4);
}

ZTEST(ec_host_cmd_usb, test_usb_invalid_prtcl_ver_max_fail_invalid_header)
{
	verify_invalid_protocol_version(UINT8_MAX);
}

ZTEST(ec_host_cmd_usb, test_usb_out_transfer_while_disabled_ignored)
{
	test_c_data->api->disable(test_c_data);
	mock_ep_out_buf->len = sizeof(struct ec_host_cmd_request_header);
	zassert_ok(test_c_data->api->request(test_c_data, mock_ep_out_buf, 0));

	zassert_is_null(mock_ep_in_buf);
	zassert_is_null(mock_ep_out_buf);
	zassert_equal(mock_ep_out_enqueue_count, 1);
}

ZTEST_SUITE(ec_host_cmd_usb, NULL, usb_test_setup, usb_test_before, NULL, NULL);

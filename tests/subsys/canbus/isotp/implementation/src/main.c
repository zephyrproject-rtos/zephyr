/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/isotp.h>
#include <ztest.h>
#include <strings.h>
#include "random_data.h"
#include <zephyr/net/buf.h>

#define NUMBER_OF_REPETITIONS 5
#define DATA_SIZE_SF          7

/*
 * @addtogroup t_can
 * @{
 * @defgroup t_can_isotp test_can_isotp
 * @brief TestPurpose: struggle the implementation and see if it breaks apart.
 * @details
 * - Test Steps
 *   -#
 * - Expected Results
 *   -#
 * @}
 */

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

const struct isotp_fc_opts fc_opts = {
	.bs = 8,
	.stmin = 0
};
const struct isotp_fc_opts fc_opts_single = {
	.bs = 0,
	.stmin = 1
};
const struct isotp_msg_id rx_addr = {
	.std_id = 0x10,
	.id_type = CAN_STANDARD_IDENTIFIER,
	.use_ext_addr = 0
};
const struct isotp_msg_id tx_addr = {
	.std_id = 0x11,
	.id_type = CAN_STANDARD_IDENTIFIER,
	.use_ext_addr = 0
};

struct isotp_recv_ctx recv_ctx;
struct isotp_send_ctx send_ctx;
uint8_t data_buf[128];

void send_complette_cb(int error_nr, void *arg)
{
	zassert_equal(error_nr, ISOTP_N_OK, "Sending failed (%d)", error_nr);
}

static void send_sf(const struct device *can_dev)
{
	int ret;

	ret = isotp_send(&send_ctx, can_dev, random_data, DATA_SIZE_SF,
			 &rx_addr, &tx_addr, send_complette_cb, NULL);
	zassert_equal(ret, 0, "Send returned %d", ret);
}

static void get_sf_net(struct isotp_recv_ctx *recv_ctx)
{
	struct net_buf *buf;
	int remaining_len, ret;

	remaining_len = isotp_recv_net(recv_ctx, &buf, K_MSEC(1000));
	zassert_true(remaining_len >= 0, "recv returned %d", remaining_len);
	zassert_equal(remaining_len, 0, "SF should fit in one frame");
	zassert_equal(buf->len, DATA_SIZE_SF, "Data length (%d) should be %d.",
		      buf->len, DATA_SIZE_SF);

	ret = memcmp(random_data, buf->data, buf->len);
	zassert_equal(ret, 0, "received data differ");
	memset(buf->data, 0, buf->len);
	net_buf_unref(buf);
}

static void get_sf(struct isotp_recv_ctx *recv_ctx)
{
	int ret;
	uint8_t *data_buf_ptr = data_buf;

	memset(data_buf, 0, sizeof(data_buf));
	ret = isotp_recv(recv_ctx, data_buf_ptr++, 1, K_MSEC(1000));
	zassert_equal(ret, 1, "recv returned %d", ret);
	ret = isotp_recv(recv_ctx, data_buf_ptr++, sizeof(data_buf) - 1,
			  K_MSEC(1000));
	zassert_equal(ret, DATA_SIZE_SF - 1, "recv returned %d", ret);

	ret = memcmp(random_data, data_buf, DATA_SIZE_SF);
	zassert_equal(ret, 0, "received data differ");
}

void print_hex(const uint8_t *ptr, size_t len)
{
	while (len--) {
		printk("%02x", *ptr++);
	}
}

static void send_test_data(const struct device *can_dev, const uint8_t *data,
			   size_t len)
{
	int ret;

	ret = isotp_send(&send_ctx, can_dev, data, len, &rx_addr, &tx_addr,
			  send_complette_cb, NULL);
	zassert_equal(ret, 0, "Send returned %d", ret);
}

static const uint8_t *check_frag(struct net_buf *frag, const uint8_t *data)
{
	int ret;

	ret = memcmp(data, frag->data, frag->len);
	if (ret) {
		printk("expected bytes:\n");
		print_hex(data, frag->len);
		printk("\nreceived (%d bytes):\n", frag->len);
		print_hex(frag->data, frag->len);
		printk("\n");
	}
	zassert_equal(ret, 0, "Received data differ");
	return data + frag->len;
}

static void receive_test_data_net(struct isotp_recv_ctx *recv_ctx,
				 const uint8_t *data, size_t len, int32_t delay)
{
	int remaining_len;
	size_t received_len = 0;
	const uint8_t *data_ptr = data;
	struct net_buf *buf;

	do {
		remaining_len = isotp_recv_net(recv_ctx, &buf, K_MSEC(1000));
		zassert_true(remaining_len >= 0, "recv error: %d",
			     remaining_len);
		received_len += buf->len;
		zassert_equal(received_len + remaining_len, len,
			      "Length mismatch");

		data_ptr = check_frag(buf, data_ptr);

		if (delay) {
			k_msleep(delay);
		}
		memset(buf->data, 0, buf->len);
		net_buf_unref(buf);
	} while (remaining_len);

	remaining_len = isotp_recv_net(recv_ctx, &buf, K_MSEC(50));
	zassert_equal(remaining_len, ISOTP_RECV_TIMEOUT,
		      "Expected timeout but got %d", remaining_len);
}

static void check_data(const uint8_t *recv_data, const uint8_t *send_data, size_t len)
{
	int ret;

	ret = memcmp(send_data, recv_data, len);
	if (ret) {
		printk("expected bytes:\n");
		print_hex(send_data, len);
		printk("\nreceived (%d bytes):\n", len);
		print_hex(recv_data, len);
		printk("\n");
	}
	zassert_equal(ret, 0, "Received data differ");
}

static void receive_test_data(struct isotp_recv_ctx *recv_ctx,
			      const uint8_t *data, size_t len, int32_t delay)
{
	size_t remaining_len = len;
	int ret;
	const uint8_t *data_ptr = data;

	do {
		memset(data_buf, 0, sizeof(data_buf));
		ret = isotp_recv(recv_ctx, data_buf, sizeof(data_buf),
				 K_MSEC(1000));
		zassert_true(ret >= 0, "recv error: %d", ret);

		zassert_true(remaining_len >= ret, "More data then expected");
		check_data(data_buf, data_ptr, ret);
		data_ptr += ret;
		remaining_len -= ret;

		if (delay) {
			k_msleep(delay);
		}
	} while (remaining_len);

	ret = isotp_recv(recv_ctx, data_buf, sizeof(data_buf), K_MSEC(50));
	zassert_equal(ret, ISOTP_RECV_TIMEOUT,
		      "Expected timeout but got %d", ret);
}

static void test_send_receive_net_sf(void)
{
	int ret, i;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Bind returned %d", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_sf(can_dev);
		get_sf_net(&recv_ctx);
	}

	isotp_unbind(&recv_ctx);
}

static void test_send_receive_sf(void)
{
	int ret, i;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Bind returned %d", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_sf(can_dev);
		get_sf(&recv_ctx);
	}

	isotp_unbind(&recv_ctx);
}

static void test_send_receive_net_blocks(void)
{
	int ret, i;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_test_data(can_dev, random_data, sizeof(random_data));
		receive_test_data_net(&recv_ctx, random_data, sizeof(random_data), 0);
	}

	isotp_unbind(&recv_ctx);
}

static void test_send_receive_blocks(void)
{
	const size_t data_size = sizeof(data_buf) * 2 + 10;
	int ret, i;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_test_data(can_dev, random_data, data_size);
		receive_test_data(&recv_ctx, random_data, data_size, 0);
	}

	isotp_unbind(&recv_ctx);
}

static void test_send_receive_net_single_blocks(void)
{
	const size_t send_len = CONFIG_ISOTP_RX_BUF_COUNT *
				CONFIG_ISOTP_RX_BUF_SIZE + 6;
	int ret, i;
	size_t buf_len;
	struct net_buf *buf, *frag;
	const uint8_t *data_ptr;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_test_data(can_dev, random_data, send_len);
		data_ptr = random_data;

		ret = isotp_recv_net(&recv_ctx, &buf, K_MSEC(1000));
		zassert_equal(ret, 0, "recv returned %d", ret);
		buf_len = net_buf_frags_len(buf);
		zassert_equal(buf_len, send_len, "Data length differ");
		frag = buf;

		do {
			data_ptr = check_frag(frag, data_ptr);
			memset(frag->data, 0, frag->len);
		} while ((frag = frag->frags));

		net_buf_unref(buf);
	}

	isotp_unbind(&recv_ctx);
}

static void test_send_receive_single_block(void)
{
	const size_t send_len = CONFIG_ISOTP_RX_BUF_COUNT *
				CONFIG_ISOTP_RX_BUF_SIZE + 6;
	int ret, i;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
			 &fc_opts_single, K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		send_test_data(can_dev, random_data, send_len);

		memset(data_buf, 0, sizeof(data_buf));
		ret = isotp_recv(&recv_ctx, data_buf, sizeof(data_buf),
				 K_MSEC(1000));
		zassert_equal(ret, send_len,
			      "data should be received at once (ret: %d)", ret);
		ret = memcmp(random_data, data_buf, send_len);
		zassert_equal(ret, 0, "Data differ");
	}

	isotp_unbind(&recv_ctx);
}

static void test_bind_unbind(void)
{
	int ret, i;

	for (i = 0; i < 100; i++) {
		ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
				 &fc_opts, K_NO_WAIT);
		zassert_equal(ret, 0, "Binding failed (%d)", ret);
		isotp_unbind(&recv_ctx);
	}

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
				 &fc_opts, K_NO_WAIT);
		zassert_equal(ret, 0, "Binding failed (%d)", ret);
		send_sf(can_dev);
		k_sleep(K_MSEC(100));
		get_sf_net(&recv_ctx);
		isotp_unbind(&recv_ctx);
	}

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
				 &fc_opts, K_NO_WAIT);
		zassert_equal(ret, 0, "Binding failed (%d)", ret);
		send_sf(can_dev);
		k_sleep(K_MSEC(100));
		get_sf(&recv_ctx);
		isotp_unbind(&recv_ctx);
	}

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
				 &fc_opts, K_NO_WAIT);
		zassert_equal(ret, 0, "Binding failed (%d)", ret);
		send_test_data(can_dev, random_data, 60);
		k_sleep(K_MSEC(100));
		receive_test_data_net(&recv_ctx, random_data, 60, 0);
		isotp_unbind(&recv_ctx);
	}

	for (i = 0; i < NUMBER_OF_REPETITIONS; i++) {
		ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr,
				 &fc_opts, K_NO_WAIT);
		zassert_equal(ret, 0, "Binding failed (%d)", ret);
		send_test_data(can_dev, random_data, 60);
		k_sleep(K_MSEC(100));
		receive_test_data(&recv_ctx, random_data, 60, 0);
		isotp_unbind(&recv_ctx);
	}
}

static void test_buffer_allocation(void)
{
	int ret;
	size_t send_data_length = CONFIG_ISOTP_RX_BUF_COUNT *
				  CONFIG_ISOTP_RX_BUF_SIZE * 3 + 6;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	send_test_data(can_dev, random_data, send_data_length);
	k_msleep(100);
	receive_test_data_net(&recv_ctx, random_data, send_data_length, 200);
	isotp_unbind(&recv_ctx);
}

static void test_buffer_allocation_wait(void)
{
	int ret;
	size_t send_data_length = CONFIG_ISOTP_RX_BUF_COUNT *
				  CONFIG_ISOTP_RX_BUF_SIZE * 2 + 6;

	ret = isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts,
			 K_NO_WAIT);
	zassert_equal(ret, 0, "Binding failed (%d)", ret);

	send_test_data(can_dev, random_data, send_data_length);
	k_sleep(K_MSEC(100));
	receive_test_data_net(&recv_ctx, random_data, send_data_length, 2000);
	isotp_unbind(&recv_ctx);
}

void test_main(void)
{
	int ret;

	zassert_true(sizeof(random_data) >= sizeof(data_buf) * 2 + 10,
		     "Test data size to small");

	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	ret = can_set_mode(can_dev, CAN_MODE_LOOPBACK);
	zassert_equal(ret, 0, "Configuring loopback mode failed (%d)", ret);

	ztest_test_suite(isotp,
			 ztest_unit_test(test_bind_unbind),
			 ztest_unit_test(test_send_receive_net_sf),
			 ztest_unit_test(test_send_receive_net_blocks),
			 ztest_unit_test(test_send_receive_net_single_blocks),
			 ztest_unit_test(test_send_receive_sf),
			 ztest_unit_test(test_send_receive_blocks),
			 ztest_unit_test(test_send_receive_single_block),
			 ztest_unit_test(test_buffer_allocation),
			 ztest_unit_test(test_buffer_allocation_wait)
			 );
	ztest_run_test_suite(isotp);
}

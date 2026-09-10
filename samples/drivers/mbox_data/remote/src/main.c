/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>

/* A "word" is 4 bytes; the tail integrity check needs room for the counter
 * word plus a separate check word.
 */
#define MBOX_DATA_CHECK_MIN_SIZE (2 * sizeof(uint32_t))

#if defined(CONFIG_MULTITHREADING)
static K_SEM_DEFINE(g_mbox_data_rx_sem, 0, 1);
#else
static volatile bool mbox_received_data_flag;
#endif

static uint8_t g_mbox_received_data[CONFIG_MBOX_DATA_MAX_MSG_SIZE];
static mbox_channel_id_t g_mbox_received_channel;
static size_t g_mbox_received_size;
static bool g_mbox_receive_error;

static void callback(const struct device *dev, mbox_channel_id_t channel_id, void *user_data,
		     struct mbox_msg *data)
{
	g_mbox_receive_error = false;

	if (!data || !data->data) {
		printk("ERROR: Callback received NULL data pointer\n");
		g_mbox_receive_error = true;
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&g_mbox_data_rx_sem);
#else
		mbox_received_data_flag = true;
#endif
		return;
	}

	if (data->size > CONFIG_MBOX_DATA_MAX_MSG_SIZE) {
		printk("ERROR: Received message size %zu exceeds maximum %d\n", data->size,
		       CONFIG_MBOX_DATA_MAX_MSG_SIZE);
		g_mbox_receive_error = true;
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&g_mbox_data_rx_sem);
#else
		mbox_received_data_flag = true;
#endif
		return;
	}

	memcpy(g_mbox_received_data, data->data, data->size);
	g_mbox_received_channel = channel_id;
	g_mbox_received_size = data->size;

#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&g_mbox_data_rx_sem);
#else
	mbox_received_data_flag = true;
#endif
}

int main(void)
{
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	uint8_t message_buf[CONFIG_MBOX_DATA_MAX_MSG_SIZE] = {0};
	struct mbox_msg msg = {0};
	uint32_t message = 0;

	printk("mbox_data Server demo started\n");

	if (!device_is_ready(tx_channel.dev)) {
		printk("ERROR: TX mbox device not ready\n");
		return 0;
	}

	if (!device_is_ready(rx_channel.dev)) {
		printk("ERROR: RX mbox device not ready\n");
		return 0;
	}

	const int max_transfer_size_bytes = mbox_mtu_get_dt(&tx_channel);
	/* Sample supports transfer sizes from 1 to CONFIG_MBOX_DATA_MAX_MSG_SIZE bytes */
	if (max_transfer_size_bytes <= 0 ||
	    max_transfer_size_bytes > CONFIG_MBOX_DATA_MAX_MSG_SIZE) {
		printk("mbox_mtu_get() error: MTU=%d, max supported=%d\n", max_transfer_size_bytes,
		       CONFIG_MBOX_DATA_MAX_MSG_SIZE);
		return 0;
	}

	printk("Using MTU: %d bytes\n", max_transfer_size_bytes);

	if (mbox_register_callback_dt(&rx_channel, callback, NULL)) {
		printk("mbox_register_callback() error\n");
		return 0;
	}

	if (mbox_set_enabled_dt(&rx_channel, 1)) {
		printk("mbox_set_enable() error\n");
		return 0;
	}

	while (message < 99) {
		g_mbox_receive_error = false;
#if defined(CONFIG_MULTITHREADING)
		k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);
#else
		while (!mbox_received_data_flag) {
		}

		mbox_received_data_flag = false;
#endif

		if (g_mbox_receive_error) {
			printk("ERROR: Failed to receive valid message\n");
			goto cleanup;
		}

		/* Extract counter from the head of the received data */
		memcpy(&message, g_mbox_received_data, MIN(sizeof(message), g_mbox_received_size));

		/* Verify the tail integrity witness when present. The upper-bound
		 * check keeps the tail access provably within the buffer.
		 */
		if (g_mbox_received_size >= MBOX_DATA_CHECK_MIN_SIZE &&
		    g_mbox_received_size <= sizeof(g_mbox_received_data)) {
			uint32_t check;
			size_t offset = g_mbox_received_size - sizeof(check);

			memcpy(&check, g_mbox_received_data + offset, sizeof(check));
			if (check != (uint32_t)~message) {
				printk("ERROR: integrity check failed: tail=0x%08x "
				       "expected=0x%08x\n",
				       check, (uint32_t)~message);
				goto cleanup;
			}
		}

		printk("Server receive (on channel %d) value: %d\n", g_mbox_received_channel,
		       message);

		message++;

		/* Prepare message - counter at the head of the buffer */
		memset(message_buf, 0, max_transfer_size_bytes);
		memcpy(message_buf, &message, MIN(sizeof(message), max_transfer_size_bytes));

		/* Exercise the full MTU: place ~counter at the tail as an
		 * integrity witness when the buffer is large enough.
		 */
		if (max_transfer_size_bytes >= MBOX_DATA_CHECK_MIN_SIZE) {
			uint32_t check = ~message;
			size_t offset = max_transfer_size_bytes - sizeof(check);

			memcpy(message_buf + offset, &check, sizeof(check));
		}

		msg.data = message_buf;
		msg.size = max_transfer_size_bytes;

		printk("Server send (on channel %d) value: %d\n", tx_channel.channel_id, message);
		if (mbox_send_dt(&tx_channel, &msg) < 0) {
			printk("mbox_send() error\n");
			goto cleanup;
		}
	}

	printk("mbox_data Server demo ended.\n");

cleanup:
	mbox_set_enabled_dt(&rx_channel, 0);
	return 0;
}

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

/* Maximum supported message size - depends on platform */
#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_mbox_versal_ipi_mailbox)
#define MBOX_DATA_MAX_MSG_SIZE 32
#else
#define MBOX_DATA_MAX_MSG_SIZE 4
#endif

static K_SEM_DEFINE(g_mbox_data_rx_sem, 0, 1);

static uint8_t g_mbox_received_data[MBOX_DATA_MAX_MSG_SIZE];
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
		k_sem_give(&g_mbox_data_rx_sem);
		return;
	}

	if (data->size > MBOX_DATA_MAX_MSG_SIZE) {
		printk("ERROR: Received message size %zu exceeds maximum %d\n", data->size,
		       MBOX_DATA_MAX_MSG_SIZE);
		g_mbox_receive_error = true;
		k_sem_give(&g_mbox_data_rx_sem);
		return;
	}

	memcpy(g_mbox_received_data, data->data, data->size);
	g_mbox_received_channel = channel_id;
	g_mbox_received_size = data->size;

	k_sem_give(&g_mbox_data_rx_sem);
}

int main(void)
{
	const struct mbox_dt_spec tx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), tx);
	const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
	uint8_t message_buf[MBOX_DATA_MAX_MSG_SIZE] = {0};
	struct mbox_msg msg = {0};
	uint32_t message = 0;

	printk("mbox_data Client demo started\n");

	if (!device_is_ready(tx_channel.dev)) {
		printk("ERROR: TX mbox device not ready\n");
		return 0;
	}

	if (!device_is_ready(rx_channel.dev)) {
		printk("ERROR: RX mbox device not ready\n");
		return 0;
	}

	const int max_transfer_size_bytes = mbox_mtu_get_dt(&tx_channel);
	/* Sample supports transfer sizes from 1 to MBOX_DATA_MAX_MSG_SIZE bytes */
	if (max_transfer_size_bytes <= 0 || max_transfer_size_bytes > MBOX_DATA_MAX_MSG_SIZE) {
		printk("mbox_mtu_get() error: MTU=%d, max supported=%d\n", max_transfer_size_bytes,
		       MBOX_DATA_MAX_MSG_SIZE);
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

	while (message < 100) {
		g_mbox_receive_error = false;
		/* Prepare message - use only the needed bytes for the counter */
		memset(message_buf, 0, max_transfer_size_bytes);
		memcpy(message_buf, &message, MIN(sizeof(message), max_transfer_size_bytes));
		msg.data = message_buf;
		msg.size = max_transfer_size_bytes;

		printk("Client send (on channel %d) value: %d\n", tx_channel.channel_id, message);
		if (mbox_send_dt(&tx_channel, &msg) < 0) {
			printk("mbox_send() error\n");
			goto cleanup;
		}

		k_sem_take(&g_mbox_data_rx_sem, K_FOREVER);

		if (g_mbox_receive_error) {
			printk("ERROR: Failed to receive valid response\n");
			goto cleanup;
		}

		/* Extract counter from received data */
		memcpy(&message, g_mbox_received_data, MIN(sizeof(message), g_mbox_received_size));

		printk("Client received (on channel %d) value: %d\n", g_mbox_received_channel,
		       message);
		message++;
	}

	printk("mbox_data Client demo ended\n");

cleanup:
	mbox_set_enabled_dt(&rx_channel, 0);
	return 0;
}

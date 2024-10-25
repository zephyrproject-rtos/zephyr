/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ipc/ipc_dispatcher.h>
#include <string.h>

#define IPC_TEST_CYCLES                       1000
#define IPC_RESPONSE_TEST_TIMEOUT_MS          10

struct app_data {
	uint8_t * const tx_data;
	size_t tx_len;
	struct k_sem rx_sem;
	volatile uint64_t tick;
	volatile bool data_ok;
	size_t test_cnt;
	size_t test_cnt_fail;
	uint64_t tick_acc;
	size_t block_len_ptr;
};

static void echo_received(const void *data, size_t len, void *param)
{
	struct app_data *ctx = (struct app_data *)param;

	ctx->tick = sys_clock_cycle_get_64() - ctx->tick;

	if (len == ctx->tx_len) {
		if (!memcmp(data, ctx->tx_data, len)) {
			ctx->data_ok = true;
		}
	}
	k_sem_give(&ctx->rx_sem);
}

int main(void)
{
	static uint8_t tx_buf[CONFIG_PBUF_RX_READ_BUF_SIZE] = {
		0xff, 0xff, 0xff, 0xff,
	}; /* Fill ID */

	static const size_t block_len[] = {
		4,
		CONFIG_PBUF_RX_READ_BUF_SIZE / 16 < 4 ?
			4 : CONFIG_PBUF_RX_READ_BUF_SIZE / 16,
		CONFIG_PBUF_RX_READ_BUF_SIZE / 8  < 4 ?
			4 : CONFIG_PBUF_RX_READ_BUF_SIZE / 8,
		CONFIG_PBUF_RX_READ_BUF_SIZE / 4  < 4 ?
			4 : CONFIG_PBUF_RX_READ_BUF_SIZE / 4,
		CONFIG_PBUF_RX_READ_BUF_SIZE / 2  < 4 ?
			4 : CONFIG_PBUF_RX_READ_BUF_SIZE / 2,
		CONFIG_PBUF_RX_READ_BUF_SIZE      < 4 ?
			4 : CONFIG_PBUF_RX_READ_BUF_SIZE,
	};

	struct app_data data = {
		.tx_data          = tx_buf,
		.block_len_ptr    = 0,
	};

	(void)k_sem_init(&data.rx_sem, 0, 1);
	ipc_dispatcher_add(0xffffffff, echo_received, &data); /* Same ID as we sent */

	for (data.test_cnt = 0;; data.test_cnt++) {

		if (!(data.test_cnt % IPC_TEST_CYCLES)) {
			if (data.test_cnt) {
				data.tick_acc /= IPC_TEST_CYCLES - data.test_cnt_fail;
				printk("************************\n");
				printk("time    : %llu nS\n",
					(data.tick_acc * 1000000000) /
					CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
				printk("bitrate : %llu BpS\n",
					((uint64_t)data.tx_len *
					CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / data.tick_acc);
				printk("block   : %u B\n", data.tx_len);
				printk("tests   : %u\n", data.test_cnt);
				printk("failed  : %u\n", data.test_cnt_fail);
				printk("************************\n");

				data.block_len_ptr++;
				if (data.block_len_ptr >= ARRAY_SIZE(block_len)) {
					k_msleep(1000);
					data.block_len_ptr = 0;
				}
			}
			data.test_cnt_fail = 0;
			data.tick_acc = 0;
			data.tx_len = block_len[data.block_len_ptr];

		}
		for (size_t j = 4; j < data.tx_len; j++) {
			data.tx_data[j] = data.test_cnt + j;
		}
		k_sem_reset(&data.rx_sem);
		data.data_ok = false;
		data.tick  = sys_clock_cycle_get_64();
		ipc_dispatcher_send(data.tx_data, data.tx_len);
		if (k_sem_take(&data.rx_sem, K_MSEC(IPC_RESPONSE_TEST_TIMEOUT_MS))) {
			data.test_cnt_fail++;
		} else if (!data.data_ok) {
			data.test_cnt_fail++;
		} else {
			data.tick_acc += data.tick;
		}
	}

	return 0;
}

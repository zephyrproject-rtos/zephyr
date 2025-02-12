/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/canbus/isotp.h>

const struct isotp_fc_opts fc_opts_8_0 = {.bs = 8, .stmin = 0};
const struct isotp_fc_opts fc_opts_0_5 = {.bs = 0, .stmin = 5};

const struct isotp_msg_id rx_addr_8_0 = {
	.std_id = 0x80,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id tx_addr_8_0 = {
	.std_id = 0x180,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.dl = 64,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id rx_addr_0_5 = {
	.std_id = 0x01,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id tx_addr_0_5 = {
	.std_id = 0x101,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.dl = 64,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};

const struct device *can_dev;
struct isotp_recv_ctx recv_ctx_8_0;
struct isotp_recv_ctx recv_ctx_0_5;

K_THREAD_STACK_DEFINE(rx_8_0_thread_stack, CONFIG_SAMPLE_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(rx_0_5_thread_stack, CONFIG_SAMPLE_RX_THREAD_STACK_SIZE);
struct k_thread rx_8_0_thread_data;
struct k_thread rx_0_5_thread_data;

const char tx_data_large[] =
"========================================\n"
"|   ____  ___  ____       ____  ____   |\n"
"|  |_  _|/ __||    | ___ |_  _||  _ \\  |\n"
"|   _||_ \\__ \\| || | ___   ||  | ___/  |\n"
"|  |____||___/|____|       ||  |_|     |\n"
"========================================\n";

const char tx_data_small[] = "This is the sample test for the short payload\n";

void rx_8_0_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int ret, rem_len, received_len;
	struct net_buf *buf;

	ret = isotp_bind(&recv_ctx_8_0, can_dev,
			 &tx_addr_8_0, &rx_addr_8_0,
			 &fc_opts_8_0, K_FOREVER);
	if (ret != ISOTP_N_OK) {
		printk("Failed to bind to rx ID %d [%d]\n",
		       rx_addr_8_0.std_id, ret);
		return;
	}

	while (1) {
		received_len = 0;
		do {
			rem_len = isotp_recv_net(&recv_ctx_8_0, &buf,
						 K_MSEC(2000));
			if (rem_len < 0) {
				printk("Receiving error [%d]\n", rem_len);
				break;
			}

			while (buf != NULL) {
				received_len += buf->len;
				printk("%.*s", buf->len, buf->data);
				buf = net_buf_frag_del(NULL, buf);
			}
		} while (rem_len);
		printk("Got %d bytes in total\n", received_len);
	}
}

void rx_0_5_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int ret, received_len;
	static uint8_t rx_buffer[32];

	ret = isotp_bind(&recv_ctx_0_5, can_dev,
			 &tx_addr_0_5, &rx_addr_0_5,
			 &fc_opts_0_5, K_FOREVER);
	if (ret != ISOTP_N_OK) {
		printk("Failed to bind to rx ID %d [%d]\n",
		       rx_addr_0_5.std_id, ret);
		return;
	}

	while (1) {
		received_len = isotp_recv(&recv_ctx_0_5, rx_buffer,
					  sizeof(rx_buffer)-1U, K_MSEC(2000));
		if (received_len < 0) {
			printk("Receiving error [%d]\n", received_len);
			continue;
		}

		rx_buffer[received_len] = '\0';
		printk("%s", rx_buffer);
	}
}

void send_complette_cb(int error_nr, void *arg)
{
	ARG_UNUSED(arg);
	printk("TX complete cb [%d]\n", error_nr);
}

/**
 * @brief Main application entry point.
 *
 */
int main(void)
{
	k_tid_t tid;
	static struct isotp_send_ctx send_ctx_8_0;
	static struct isotp_send_ctx send_ctx_0_5;
	int ret = 0;

	can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	if (!device_is_ready(can_dev)) {
		printk("CAN: Device driver not ready.\n");
		return 0;
	}

	can_mode_t mode = (IS_ENABLED(CONFIG_SAMPLE_LOOPBACK_MODE) ? CAN_MODE_LOOPBACK : 0) |
			  (IS_ENABLED(CONFIG_SAMPLE_CAN_FD_MODE) ? CAN_MODE_FD : 0);
	ret = can_set_mode(can_dev, mode);
	if (ret != 0) {
		printk("CAN: Failed to set mode [%d]", ret);
		return 0;
	}

	ret = can_start(can_dev);
	if (ret != 0) {
		printk("CAN: Failed to start device [%d]\n", ret);
		return 0;
	}

	tid = k_thread_create(&rx_8_0_thread_data, rx_8_0_thread_stack,
			      K_THREAD_STACK_SIZEOF(rx_8_0_thread_stack),
			      rx_8_0_thread, NULL, NULL, NULL,
			      CONFIG_SAMPLE_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!tid) {
		printk("ERROR spawning rx thread\n");
		return 0;
	}
	k_thread_name_set(tid, "rx_8_0");

	tid = k_thread_create(&rx_0_5_thread_data, rx_0_5_thread_stack,
			      K_THREAD_STACK_SIZEOF(rx_0_5_thread_stack),
			      rx_0_5_thread, NULL, NULL, NULL,
			      CONFIG_SAMPLE_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!tid) {
		printk("ERROR spawning rx thread\n");
		return 0;
	}
	k_thread_name_set(tid, "rx_0_5");

	printk("Start sending data\n");

	while (1) {
		k_msleep(1000);
		ret = isotp_send(&send_ctx_0_5, can_dev,
				 tx_data_small, sizeof(tx_data_small),
				 &tx_addr_0_5, &rx_addr_0_5,
				 send_complette_cb, NULL);
		if (ret != ISOTP_N_OK) {
			printk("Error while sending data to ID %d [%d]\n",
			       tx_addr_0_5.std_id, ret);
		}

		ret = isotp_send(&send_ctx_8_0, can_dev,
				 tx_data_large, sizeof(tx_data_large),
				 &tx_addr_8_0, &rx_addr_8_0, NULL, NULL);
		if (ret != ISOTP_N_OK) {
			printk("Error while sending data to ID %d [%d]\n",
			       tx_addr_8_0.std_id, ret);
		}
	}
}

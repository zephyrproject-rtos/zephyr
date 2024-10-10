/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <lr11xx_radio.h>
#include <lr11xx_system.h>
#include <lr11xx_system_types.h>
#include <lr11xx_regmem.h>

#include "apps_common.h"

LOG_MODULE_REGISTER(main);

#if DT_HAS_CHOSEN(lora)
#define LORA_TRANSCEIVER DT_CHOSEN(lora)
#else
#define LORA_TRANSCEIVER DT_NODELABEL(lora0)
#endif

const struct device *context = DEVICE_DT_GET(LORA_TRANSCEIVER);
const struct device *spi_bus = DEVICE_DT_GET(DT_BUS(LORA_TRANSCEIVER));

static uint8_t buffer_tx[PAYLOAD_LENGTH];

#define PING_PONG_PREFIX_SIZE 7
static const uint8_t prefix_size = MIN(PING_PONG_PREFIX_SIZE, PAYLOAD_LENGTH);

static const uint8_t ping_msg[PING_PONG_PREFIX_SIZE] = "PINGREQ";
static const uint8_t pong_msg[PING_PONG_PREFIX_SIZE] = "PINGREP";

#define DELAY_BEFORE_TX_MS 5
#define PING_TIMEOUT_MS 1500
#define IDLE_PING_TIMEOUT_MS 0xFFFFFF

#define STACKSIZE 1024

/**
 * @brief LR11xx interrupt mask used by the application
 */
#define IRQ_MASK                                                                                   \
	(LR11XX_SYSTEM_IRQ_TX_DONE | LR11XX_SYSTEM_IRQ_RX_DONE | LR11XX_SYSTEM_IRQ_TIMEOUT |       \
	 LR11XX_SYSTEM_IRQ_HEADER_ERROR | LR11XX_SYSTEM_IRQ_CRC_ERROR |                            \
	 LR11XX_SYSTEM_IRQ_FSK_LEN_ERROR)

K_THREAD_STACK_DEFINE(lora_event_handler_loop_stack_area, STACKSIZE);
struct k_thread lora_event_handler_loop_thread_data;
k_tid_t lora_event_handler_loop_thread;

void lora_event_handler_loop(void *p1, void *p2, void *p3)
{
	while (1) {
		apps_common_lr11xx_irq_process(context, IRQ_MASK);
		k_sleep(K_MSEC(1));
	}
}

int initialize_lora(void)
{
	apps_common_lr11xx_system_init(context);

	apps_common_lr11xx_fetch_and_print_version(context);

	apps_common_lr11xx_radio_init(context);

	int ret = 0;

	LOG_INF("Set dio irq mask");
	ret = lr11xx_system_set_dio_irq_params(context, IRQ_MASK, 0);
	if (ret) {
		LOG_ERR("Failed to set dio irq params.");
		return ret;
	}

	LOG_INF("Clear irq status");
	ret = lr11xx_system_clear_irq_status(context, LR11XX_SYSTEM_IRQ_ALL_MASK);
	if (ret) {
		LOG_ERR("Failed to set dio irq params.");
		return ret;
	}

	apps_common_lr11xx_enable_irq(context);

	lora_event_handler_loop_thread = k_thread_create(
		&lora_event_handler_loop_thread_data,
		lora_event_handler_loop_stack_area,
		K_THREAD_STACK_SIZEOF(lora_event_handler_loop_stack_area),
		lora_event_handler_loop, NULL, NULL, NULL,
		7, 0, K_NO_WAIT
	);

	return ret;
}

int set_rx_mode(const unsigned int timeout)
{
	LOG_INF("Set RX mode.");
	int ret = lr11xx_radio_set_rx(context, timeout);

	if (ret) {
		LOG_ERR("Failed to set RX mode.");
	}
	return ret;
}

int main(void)
{
	if (!device_is_ready(spi_bus)) {
		LOG_ERR("SPI bus is not ready yet!");
	}
	if (!device_is_ready(context)) {
		LOG_ERR("LR11xx is not ready yet!");
	}

	initialize_lora();

	set_rx_mode(IDLE_PING_TIMEOUT_MS);

	return 0;
}

static bool tx_done;
void on_tx_done(void)
{
	LOG_INF("on TX mode.");
	tx_done = true;
}

static int lorademo_ping_send(bool request_not_reply, int counter)
{
	int ret;

	/* Setup TX buffer */
	memcpy(buffer_tx, request_not_reply ? ping_msg : pong_msg, prefix_size);
	for (int i = prefix_size; i < PAYLOAD_LENGTH; i++) {
		buffer_tx[i] = counter;
	}

	ret = lr11xx_regmem_write_buffer8(context, buffer_tx, PAYLOAD_LENGTH);
	if (ret) {
		LOG_ERR("Failed to write buffer.");
		return 1;
	}

	tx_done = false;

	ret = lr11xx_radio_set_tx(context, 0);
	if (ret) {
		LOG_ERR("Failed to set TX mode.");
	}

	WAIT_FOR(tx_done, PING_TIMEOUT_MS*1000,
		apps_common_lr11xx_irq_process(context, IRQ_MASK); k_sleep(K_MSEC(2)));

	if (!tx_done) {
		LOG_ERR("Timeout when trying to send ping msg!\n");
		return 1;
	}
	return ret;
}

static bool rx_replies_done;
static int rx_counter;

void on_rx_done(void)
{
	uint8_t buffer_rx[PAYLOAD_LENGTH];
	uint8_t size;

	apps_common_lr11xx_receive(context, buffer_rx, sizeof(buffer_rx), &size);

	if (!memcmp(buffer_rx, ping_msg, prefix_size)) {
		/* This is a request message, sending a reply */
		int counter = buffer_rx[prefix_size];
		/* Wait a bit because tx->rx takes time? */
		k_msleep(DELAY_BEFORE_TX_MS);
		/* printk("ping request %d received\n", counter); */
		lorademo_ping_send(false, counter);

		/* go back to rx mode */
		set_rx_mode(IDLE_PING_TIMEOUT_MS);
	}
	if (!memcmp(buffer_rx, pong_msg, prefix_size)) {
		/* This is a reply message */
		rx_counter = buffer_rx[prefix_size];
		rx_replies_done = true;
	}
}

void on_rx_timeout(void)
{
	set_rx_mode(IDLE_PING_TIMEOUT_MS);
}

static int tx_counter;

static int cmd_lorademo_ping(const struct shell *sh, size_t argc, char **argv)
{
	shell_fprintf(sh, SHELL_NORMAL, "Sending ping %d...\n", tx_counter);
	int64_t sent_time = k_uptime_get();

	if (lorademo_ping_send(true, tx_counter)) {
		shell_fprintf(sh, SHELL_NORMAL, "Could not send ping request!\n");
		return 1;
	}
	tx_counter++;

	set_rx_mode(PING_TIMEOUT_MS);
	rx_replies_done = false;
	WAIT_FOR(rx_replies_done, PING_TIMEOUT_MS*1000, k_sleep(K_USEC(1000)));
	if (!rx_replies_done) {
		shell_fprintf(sh, SHELL_NORMAL, "Timeout!\n");
		return 1;
	}
	int64_t recv_time = k_uptime_get();
	int64_t ping_time = recv_time - sent_time;

	shell_fprintf(sh, SHELL_NORMAL, "Ping %d received in %lldms\n", rx_counter, ping_time);
	/* we will get back automatically to rx mode with idle timeout */
	set_rx_mode(IDLE_PING_TIMEOUT_MS);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lorademo,
	SHELL_CMD_ARG(ping, NULL, "Ping LoRa friend device", cmd_lorademo_ping, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lorademo, &lorademo, "LoRa demo commands", NULL);

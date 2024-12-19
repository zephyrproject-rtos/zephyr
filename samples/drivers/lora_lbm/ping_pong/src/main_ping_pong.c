/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/random/random.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

/* #include "apps_common.h" */
/* #include "sx126x_radio.h" */
#include "sx126x.h"
#include "sx126x_hal.h"
#include "sx126x_regs.h"
/* #include "sx126x_board.h" */
#include "main_ping_pong.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/**
 * @brief Size of ping-pong message prefix
 *
 * Expressed in bytes
 */
#define PING_PONG_PREFIX_SIZE 6

/**
 * @brief Duration of the wait before packet transmission to assure
 / reception status is ready on the other side
 *
 * Expressed in milliseconds
 */
#define DELAY_BEFORE_TX_MS 20

/**
 * @brief Duration of the wait between each ping-pong activity,
 * can be used to adjust ping-pong speed
 *
 * Expressed in milliseconds
 */
#define DELAY_PING_PONG_PACE_MS 200

/**
 * @brief LR11xx interrupt mask used by the application
 */
#define IRQ_MASK                                                                                   \
	(SX126X_IRQ_TX_DONE | SX126X_IRQ_RX_DONE | SX126X_IRQ_TIMEOUT | SX126X_IRQ_HEADER_ERROR |  \
	 SX126X_IRQ_CRC_ERROR)

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

#if DT_HAS_CHOSEN(lora)
#define LORA_TRANSCEIVER DT_CHOSEN(lora)
#else
#define LORA_TRANSCEIVER DT_NODELABEL(lora0)
#endif

const struct device *context = DEVICE_DT_GET(LORA_TRANSCEIVER);
const struct device *spi_bus = DEVICE_DT_GET(DT_BUS(LORA_TRANSCEIVER));

static uint8_t buffer_tx[PAYLOAD_LENGTH];
static bool is_master = true;

static const uint8_t ping_msg[PING_PONG_PREFIX_SIZE] = "M-PING";
static const uint8_t pong_msg[PING_PONG_PREFIX_SIZE] = "S-PONG";

static const uint8_t prefix_size = MIN(PING_PONG_PREFIX_SIZE, PAYLOAD_LENGTH);

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief Handle reception failure for ping-pong example
 */
static void ping_pong_reception_failure_handling(void);

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

/**
 * @brief Main application entry point.
 */
int main(void)
{
	LOG_INF("===== SX126x TX CW example =====");

	apps_common_lr11xx_system_init(context);

	apps_common_lr11xx_fetch_and_print_version(context);

	apps_common_lr11xx_radio_init(context);

	int ret = 0;

	LOG_INF("Set dio irq mask");
	ret = sx126x_set_dio_irq_params(context, IRQ_MASK, 0);
	if (ret) {
		LOG_ERR("Failed to set dio irq params.");
	}

	LOG_INF("Clear irq status");
	ret = sx126x_clear_irq_status(context, SX126X_IRQ_ALL);
	if (ret) {
		LOG_ERR("Failed to set dio irq params.");
	}

	apps_common_lr11xx_enable_irq(context);

	/* Setup TX buffer */
	memcpy(buffer_tx, ping_msg, prefix_size);
	for (int i = prefix_size; i < PAYLOAD_LENGTH; i++) {
		buffer_tx[i] = i;
	}

	/* Write buffer */
	LOG_INF("Write TX buffer");
	ret = lr11xx_regmem_write_buffer8(context, buffer_tx, PAYLOAD_LENGTH);
	if (ret) {
		LOG_ERR("Failed to write buffer.");
	}

	/* Set in TX mode */
	LOG_INF("Set TX mode");
	ret = lr11xx_radio_set_tx(context, 0);
	if (ret) {
		LOG_ERR("Failed to set TX mode.");
	}

	while (1) {
		apps_common_lr11xx_irq_process(context, IRQ_MASK);
		k_sleep(K_MSEC(1));
	}
	return 0;
}

void on_tx_done(void)
{
	k_sleep(K_MSEC(DELAY_PING_PONG_PACE_MS));

	/* Random delay to avoid unwanted synchronization */
	int ret = lr11xx_radio_set_rx(context, RX_TIMEOUT_VALUE + sys_rand32_get() % 500);

	if (ret) {
		LOG_ERR("Failed to set RX mode.");
	}
}

void on_rx_done(void)
{
	uint8_t buffer_rx[PAYLOAD_LENGTH];
	uint8_t size;

	apps_common_lr11xx_receive(context, buffer_rx, sizeof(buffer_rx), &size);

	if (is_master == true) {
		if (memcmp(buffer_rx, ping_msg, prefix_size) == 0) {
			is_master = false;
			memcpy(buffer_tx, pong_msg, prefix_size);
		} else if (memcmp(buffer_rx, pong_msg, prefix_size) != 0) {
			LOG_ERR("Unexpected message");
		}
	} else {
		if (memcmp(buffer_rx, ping_msg, prefix_size) != 0) {
			LOG_ERR("Unexpected message");

			is_master = true;
			memcpy(buffer_tx, ping_msg, prefix_size);
		}
	}

	k_sleep(K_MSEC(DELAY_PING_PONG_PACE_MS));

	k_sleep(K_MSEC(DELAY_BEFORE_TX_MS));

	int ret = lr11xx_regmem_write_buffer8(context, buffer_tx, PAYLOAD_LENGTH);

	if (ret) {
		LOG_ERR("Failed to write buffer.");
	}

	ret = lr11xx_radio_set_tx(context, 0);
	if (ret) {
		LOG_ERR("Failed to set TX mode.");
	}
}

void on_rx_timeout(void)
{
	ping_pong_reception_failure_handling();
}

void on_rx_crc_error(void)
{
	ping_pong_reception_failure_handling();
}

void on_fsk_len_error(void)
{
	ping_pong_reception_failure_handling();
}

static void ping_pong_reception_failure_handling(void)
{
	is_master = true;
	memcpy(buffer_tx, ping_msg, prefix_size);

	int ret = lr11xx_regmem_write_buffer8(context, buffer_tx, PAYLOAD_LENGTH);

	if (ret) {
		LOG_ERR("Failed to write buffer.");
	}

	ret = lr11xx_radio_set_tx(context, 0);
	if (ret) {
		LOG_ERR("Failed to set TX mode.");
	}
}

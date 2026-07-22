/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Perry Naseck, MIT Media Lab <pnaseck@media.mit.edu>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Shared logic for DMX512 UART backends.
 *
 * Contains the RX state machine, break timing validation, byte processing, UART configuration,
 * and GPIO management.
 */

#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dmx, CONFIG_DMX_LOG_LEVEL);

#include "dmx_uart_common.h"

static void rx_timeout_handler(struct k_timer *timer)
{
	struct dmx_uart_data *data = CONTAINER_OF(timer, struct dmx_uart_data, rx_timeout);

	if (data->rx_state == DMX_UART_RX_IDLE) {
		return;
	}

	/* Abandon any in-progress frame. */
	data->rx.buf = NULL;
	data->rx.data = NULL;
	data->rx_state = DMX_UART_RX_IDLE;
	LOG_DBG("signal lost (timeout), resetting to IDLE");
}

void dmx_uart_handle_break(struct dmx_uart_data *data)
{
	uint32_t now = k_uptime_get_32();
	uint32_t elapsed = now - data->last_break_uptime;

	switch (data->rx_state) {
	case DMX_UART_RX_IDLE:
		/*
		 * Cold start (or resync after signal loss): there is no valid break-to-break timing
		 * reference yet, so this break could be a spurious edge (e.g. a cable being plugged
		 * in) rather than a real frame boundary. Only seed the timing reference and move to
		 * SYNC; do not begin capturing. The next break validates the first interval, and the
		 * break after that validates and commits the first frame.
		 *
		 * This two-break lock from IDLE ensures no data reaches the application until two
		 * consecutive valid break intervals have been observed, so a single spurious break
		 * cannot deliver a corrupt frame. Once locked, subsequent frames commit on a single
		 * validated break (DMX_UART_RX_DATA case), so this conservatism costs extra frames
		 * only at initial sync.
		 */
		LOG_DBG("sync: first break, seeding timing (discarding frame)");
		data->last_break_uptime = now;
		data->rx_state = DMX_UART_RX_SYNC;
		k_timer_start(&data->rx_timeout, K_MSEC(DMX_MAX_BREAK_TO_BREAK_MS), K_NO_WAIT);
		return;

	case DMX_UART_RX_SYNC:
		if (elapsed < DMX_MIN_BREAK_TO_BREAK_MS) {
			LOG_DBG("sync: break too fast (%u ms), restarting", elapsed);
			data->rx_state = DMX_UART_RX_IDLE;
			return;
		}
		if (elapsed > DMX_MAX_BREAK_TO_BREAK_MS) {
			LOG_DBG("sync: break too slow (%u ms), restarting", elapsed);
			data->rx_state = DMX_UART_RX_IDLE;
			return;
		}
		/*
		 * One valid interval observed. Begin capturing the next frame; its own break-to-break
		 * timing is validated at the following break (DMX_UART_RX_DATA case) before it commits.
		 * This is the second validation of the two-break cold-start lock, and also the
		 * single-break resync path after an early-commit (which returns to SYNC).
		 */
		LOG_DBG("sync: locked after %u ms", elapsed);
		data->last_break_uptime = now;
		k_timer_start(&data->rx_timeout, K_MSEC(DMX_MAX_BREAK_TO_BREAK_MS), K_NO_WAIT);
		dmx_rx_begin_frame(&data->common, &data->rx);
		data->rx_state = DMX_UART_RX_BREAK;
		return;

	case DMX_UART_RX_BREAK:
		/* Additional break byte from the same break condition. On nRF UARTE, each break
		 * byte arrives as a separate ISR invocation. Discard. Signal loss is handled by the
		 * rx_timeout timer.
		 */
		return;

	case DMX_UART_RX_DATA:
		/*
		 * If break-to-break timing is valid per ANSI E1.11 Table 7, this is the legitimate
		 * start of the next frame. Commit whatever slots we received (short frames are
		 * allowed per clause 8.7) and begin a new frame.
		 *
		 * If timing is invalid, this is likely a corrupted byte mid-frame (not a real
		 * break). Per clause 9.1, discard the improperly framed slot and all following
		 * slots.
		 */
		if (elapsed >= DMX_MIN_BREAK_TO_BREAK_MS && elapsed <= DMX_MAX_BREAK_TO_BREAK_MS) {
			LOG_DBG("break-commit in DATA (%u ms, %u slots)",
				elapsed, data->rx.data_idx);
			dmx_rx_end_frame(&data->common, &data->rx);
			data->last_break_uptime = now;
			k_timer_start(&data->rx_timeout, K_MSEC(DMX_MAX_BREAK_TO_BREAK_MS),
				K_NO_WAIT);
			dmx_rx_begin_frame(&data->common, &data->rx);
			data->rx_state = DMX_UART_RX_BREAK;
			return;
		}
		/* Invalid timing: discard and re-sync. */
		data->rx.buf = NULL;
		data->rx.data = NULL;
		data->rx_state = DMX_UART_RX_IDLE;
		k_spinlock_key_t key = k_spin_lock(&data->common.stats_lock);

		data->common.stats.rx_frame_errors++;
		k_spin_unlock(&data->common.stats_lock, key);
		LOG_DBG("frame error during DATA (%u ms since break), discarding and re-syncing",
			elapsed);
		return;

	default:
		CODE_UNREACHABLE;
	}
}

bool dmx_uart_is_receiving(const struct device *dev)
{
	const struct dmx_uart_data *data = dev->data;

	return data->common.enabled &&
	       data->common.mode == DMX_MODE_INPUT &&
	       data->rx_state != DMX_UART_RX_IDLE;
}

void dmx_uart_process_byte(struct dmx_uart_data *data, uint8_t byte)
{
	switch (data->rx_state) {
	case DMX_UART_RX_IDLE:
	case DMX_UART_RX_SYNC:
		/* Discard bytes until next BREAK. */
		break;

	case DMX_UART_RX_BREAK:
		LOG_DBG("BREAK: sc=0x%02x buf=%p", byte, (void *)data->rx.buf);
		if (dmx_rx_byte(&data->rx, byte)) {
			dmx_rx_end_frame(&data->common, &data->rx);
			data->rx_state = DMX_UART_RX_SYNC;
		} else {
			data->rx_state = DMX_UART_RX_DATA;
		}
		break;

	case DMX_UART_RX_DATA:
		if (dmx_rx_byte(&data->rx, byte)) {
			LOG_DBG("commit: idx=%u", data->rx.data_idx);
			dmx_rx_end_frame(&data->common, &data->rx);
			data->rx_state = DMX_UART_RX_SYNC;
		}
		break;

	default:
		CODE_UNREACHABLE;
	}
}

int dmx_uart_configure(const struct device *uart_dev)
{
	struct uart_config uart_cfg = {
		.baudrate = DMX_BAUD_RATE,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_2,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	int ret = uart_configure(uart_dev, &uart_cfg);

	if (ret != 0) {
		LOG_ERR("uart_configure failed: %d", ret);
	}
	return ret;
}

int dmx_uart_enable_common(const struct device *dev)
{
	struct dmx_uart_data *data = dev->data;
	const struct dmx_uart_config *cfg = dev->config;
	int ret;

	if (data->common.enabled) {
		return -EALREADY;
	}
	if (!data->common.mode_set) {
		return -EINVAL;
	}

	ret = dmx_uart_configure(cfg->uart);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_GPIO)
	/* Assert receiver-enable (active low). */
	if (cfg->re.port != NULL) {
		gpio_pin_set_dt(&cfg->re, 1);
	}

	/* De-assert driver-enable. */
	if (cfg->de.port != NULL) {
		gpio_pin_set_dt(&cfg->de, 0);
	}
#endif

	data->rx_state = DMX_UART_RX_IDLE;
	memset(&data->rx, 0, sizeof(data->rx));
	k_timer_init(&data->rx_timeout, rx_timeout_handler, NULL);

	/* Clear any stale error flags. */
	uart_err_check(cfg->uart);

	return 0;
}

void dmx_uart_disable_common(const struct device *dev)
{
	struct dmx_uart_data *data = dev->data;
	const struct dmx_uart_config *cfg = dev->config;

	k_timer_stop(&data->rx_timeout);

#if defined(CONFIG_GPIO)
	if (cfg->re.port != NULL) {
		gpio_pin_set_dt(&cfg->re, 0);
	}
#endif

	(void)cfg;
	data->common.enabled = false;
	data->rx_state = DMX_UART_RX_IDLE;
}

int dmx_uart_set_mode(const struct device *dev, enum dmx_mode mode)
{
	struct dmx_uart_data *data = dev->data;

	if (data->common.enabled) {
		return -EBUSY;
	}

	if (mode == DMX_MODE_OUTPUT) {
		return -ENOTSUP;
	}

	data->common.mode = mode;
	data->common.mode_set = true;
	return 0;
}

int dmx_uart_init_gpio(const struct dmx_uart_config *cfg)
{
#if defined(CONFIG_GPIO)
	if (cfg->de.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->de)) {
			return -ENODEV;
		}
		gpio_pin_configure_dt(&cfg->de, GPIO_OUTPUT_INACTIVE);
	}
	if (cfg->re.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->re)) {
			return -ENODEV;
		}
		gpio_pin_configure_dt(&cfg->re, GPIO_OUTPUT_INACTIVE);
	}
#else
	(void)cfg;
#endif
	return 0;
}

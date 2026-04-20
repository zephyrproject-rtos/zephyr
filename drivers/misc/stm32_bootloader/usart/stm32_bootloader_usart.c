/*
 * Copyright (c) 2026 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 system bootloader over USART (AN3155).
 *
 * Implements the standalone stm32_bootloader API over UART. All protocol
 * logic lives here; the driver is independent of flash_driver_api. A
 * companion flash adapter (see @c st,stm32-bootloader-flash) can expose
 * target memory regions as Zephyr flash devices by delegating to the
 * stm32_bootloader API.
 *
 * Layout of this file:
 *   1. Per-instance config / data structs.
 *   2. Low-level UART helpers (byte-level IO, framing helpers).
 *   3. AN3155 command building blocks (send_cmd, send_addr, wait_ack).
 *   4. Command-level helpers (fetch_info, read_mem, write_mem, erase_*).
 *   5. Public API method implementations.
 *   6. DEVICE_DT_INST_DEFINE boilerplate.
 */

#define DT_DRV_COMPAT st_stm32_bootloader

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/misc/stm32_bootloader/stm32_bootloader.h>
#include "../stm32_bootloader_gpio.h"
#include "stm32_bootloader_usart.h"

LOG_MODULE_REGISTER(stm32_sys_bl_uart, CONFIG_STM32_BOOTLOADER_UART_LOG_LEVEL);

/* 1. ---- Per-instance structs ---- */

struct stm32_bl_uart_config {
	struct stm32_bootloader_gpio_cfg gpio;
	const struct device *uart;
	unsigned int baudrate;
	unsigned int timeout_ms;
	unsigned int erase_timeout_ms;
	bool parity_none;
};

struct stm32_bl_uart_data {
	struct k_mutex lock;
	/** Backup of the host UART config, restored on exit. */
	struct uart_config uart_cfg_bkp;
	/** Target capabilities, fetched on enter. */
	struct stm32_bootloader_info info;
	bool uart_cfg_saved;
	bool connected;
};

/* 2. ---- Low-level UART helpers ---- */

static void tx(const struct device *uart, uint8_t byte)
{
	uart_poll_out(uart, byte);
}

static int rx(const struct device *uart, uint8_t *byte, unsigned int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (uart_poll_in(uart, byte) != 0) {
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10); /* ~1/9 of a character time at 115200 */
	}
	return 0;
}

static void drain_rx(const struct device *uart)
{
	uint8_t discard;

	while (uart_poll_in(uart, &discard) == 0) {
	}
}

static uint8_t xor_bytes(const uint8_t *buf, size_t len)
{
	uint8_t x = 0;

	for (size_t i = 0; i < len; i++) {
		x ^= buf[i];
	}
	return x;
}

/* 3. ---- AN3155 command building blocks ---- */

static int wait_ack(const struct device *dev, unsigned int timeout_ms)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	uint8_t b;
	int ret = rx(cfg->uart, &b, timeout_ms);

	if (ret < 0) {
		return ret;
	}
	if (b == STM32_BL_UART_ACK) {
		return 0;
	}
	if (b == STM32_BL_UART_NACK) {
		LOG_WRN("NACK received");
		return -EPROTO;
	}
	LOG_WRN("Unexpected byte 0x%02x in place of ACK", b);
	return -EPROTO;
}

/** Send cmd + ~cmd, wait for ACK. */
static int send_cmd(const struct device *dev, uint8_t cmd)
{
	const struct stm32_bl_uart_config *cfg = dev->config;

	drain_rx(cfg->uart);
	tx(cfg->uart, cmd);
	tx(cfg->uart, (uint8_t)~cmd);
	return wait_ack(dev, cfg->timeout_ms);
}

/** Send a 32-bit address big-endian, with XOR checksum. Waits for ACK. */
static int send_addr(const struct device *dev, uint32_t addr)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	uint8_t buf[4];

	sys_put_be32(addr, buf);
	for (int i = 0; i < 4; i++) {
		tx(cfg->uart, buf[i]);
	}
	tx(cfg->uart, xor_bytes(buf, 4));
	return wait_ack(dev, cfg->timeout_ms);
}

/* 4. ---- Command-level helpers ---- */

static int fetch_info(const struct device *dev, struct stm32_bootloader_info *info)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	uint8_t b;
	int ret;

	memset(info, 0, sizeof(*info));

	/* Get (0x00): N, version, N supported commands, ACK */
	ret = send_cmd(dev, STM32_BL_UART_CMD_GET);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &info->num_cmds, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &info->bl_version, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	for (uint8_t i = 0; i < info->num_cmds; i++) {
		/* Store the first ARRAY_SIZE(cmds) entries; sink any
		 * overflow into a scratch byte so the RX stream stays
		 * in sync with the protocol.
		 */
		uint8_t *dst = (i < ARRAY_SIZE(info->cmds)) ? &info->cmds[i] : &b;

		ret = rx(cfg->uart, dst, cfg->timeout_ms);
		if (ret < 0) {
			return ret;
		}
	}
	ret = wait_ack(dev, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	for (uint8_t i = 0; i < MIN(info->num_cmds, ARRAY_SIZE(info->cmds)); i++) {
		if (info->cmds[i] == STM32_BL_UART_CMD_EE) {
			info->has_extended_erase = true;
			break;
		}
	}

	/* Get Version (0x01): version, option1, option2, ACK */
	ret = send_cmd(dev, STM32_BL_UART_CMD_GV);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &info->bl_version, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &info->option1, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &info->option2, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	ret = wait_ack(dev, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}

	/* Get ID (0x02): N (=1 for 2 bytes), PID hi, PID lo, ACK */
	ret = send_cmd(dev, STM32_BL_UART_CMD_GID);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &b, cfg->timeout_ms); /* byte count minus one */
	if (ret < 0) {
		return ret;
	}
	uint8_t pid_hi, pid_lo;

	ret = rx(cfg->uart, &pid_hi, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	ret = rx(cfg->uart, &pid_lo, cfg->timeout_ms);
	if (ret < 0) {
		return ret;
	}
	info->pid = ((uint16_t)pid_hi << 8) | pid_lo;
	return wait_ack(dev, cfg->timeout_ms);
}

static int read_chunked(const struct device *dev, uint32_t addr, uint8_t *buf, size_t len)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	size_t done = 0;

	while (done < len) {
		size_t chunk = MIN(len - done, STM32_BOOTLOADER_MAX_DATA_SIZE);
		int ret = send_cmd(dev, STM32_BL_UART_CMD_RM);

		if (ret < 0) {
			return ret;
		}
		ret = send_addr(dev, addr + done);
		if (ret < 0) {
			return ret;
		}

		uint8_t n = (uint8_t)(chunk - 1);

		tx(cfg->uart, n);
		tx(cfg->uart, (uint8_t)~n);
		ret = wait_ack(dev, cfg->timeout_ms);
		if (ret < 0) {
			return ret;
		}

		for (size_t i = 0; i < chunk; i++) {
			ret = rx(cfg->uart, &buf[done + i], cfg->timeout_ms);
			if (ret < 0) {
				return ret;
			}
		}
		done += chunk;
	}
	return 0;
}

static int write_chunked(const struct device *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	size_t done = 0;

	while (done < len) {
		size_t chunk = MIN(len - done, STM32_BOOTLOADER_MAX_DATA_SIZE);
		uint8_t n = (uint8_t)(chunk - 1);
		uint8_t x = n;
		int ret = send_cmd(dev, STM32_BL_UART_CMD_WM);

		if (ret < 0) {
			return ret;
		}
		ret = send_addr(dev, addr + done);
		if (ret < 0) {
			return ret;
		}

		tx(cfg->uart, n);
		for (size_t i = 0; i < chunk; i++) {
			tx(cfg->uart, buf[done + i]);
			x ^= buf[done + i];
		}
		tx(cfg->uart, x);

		ret = wait_ack(dev, cfg->timeout_ms);
		if (ret < 0) {
			return ret;
		}
		done += chunk;
	}
	return 0;
}

static int do_mass_erase(const struct device *dev)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	const struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (data->info.has_extended_erase) {
		ret = send_cmd(dev, STM32_BL_UART_CMD_EE);
		if (ret < 0) {
			return ret;
		}
		tx(cfg->uart, 0xFF);
		tx(cfg->uart, 0xFF);
		tx(cfg->uart, 0x00); /* XOR(0xFF, 0xFF) */
		return wait_ack(dev, cfg->erase_timeout_ms);
	}

	ret = send_cmd(dev, STM32_BL_UART_CMD_ER);
	if (ret < 0) {
		return ret;
	}
	tx(cfg->uart, STM32_BL_UART_STD_MASS_ERASE);
	tx(cfg->uart, (uint8_t)~STM32_BL_UART_STD_MASS_ERASE);
	return wait_ack(dev, cfg->erase_timeout_ms);
}

static int do_erase_pages(const struct device *dev, const uint16_t *pages, size_t count)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	const struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (count == 0) {
		return 0;
	}

	if (data->info.has_extended_erase) {
		/* Values 0xFFFx are reserved for special erases. */
		if (count > 0xFFF0U) {
			return -EINVAL;
		}

		ret = send_cmd(dev, STM32_BL_UART_CMD_EE);
		if (ret < 0) {
			return ret;
		}

		uint16_t n_minus_1 = (uint16_t)(count - 1);
		uint8_t x = 0;
		uint8_t hi = (uint8_t)(n_minus_1 >> 8);
		uint8_t lo = (uint8_t)(n_minus_1 & 0xFF);

		tx(cfg->uart, hi);
		tx(cfg->uart, lo);
		x ^= hi ^ lo;

		for (size_t i = 0; i < count; i++) {
			hi = (uint8_t)(pages[i] >> 8);
			lo = (uint8_t)(pages[i] & 0xFF);
			tx(cfg->uart, hi);
			tx(cfg->uart, lo);
			x ^= hi ^ lo;
		}
		tx(cfg->uart, x);
		return wait_ack(dev, cfg->erase_timeout_ms);
	}

	/* Standard Erase addresses pages 0..255 via single bytes. */
	if (count > 255U) {
		return -EINVAL;
	}
	for (size_t i = 0; i < count; i++) {
		if (pages[i] > 0xFFU) {
			return -EINVAL;
		}
	}

	ret = send_cmd(dev, STM32_BL_UART_CMD_ER);
	if (ret < 0) {
		return ret;
	}

	uint8_t n_minus_1 = (uint8_t)(count - 1);
	uint8_t x = n_minus_1;

	tx(cfg->uart, n_minus_1);
	for (size_t i = 0; i < count; i++) {
		uint8_t page = (uint8_t)pages[i];

		tx(cfg->uart, page);
		x ^= page;
	}
	tx(cfg->uart, x);
	return wait_ack(dev, cfg->erase_timeout_ms);
}

/* 5. ---- Public API methods ---- */

static int api_enter(const struct device *dev)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->uart_cfg_saved) {
		ret = uart_config_get(cfg->uart, &data->uart_cfg_bkp);
		if (ret < 0) {
			LOG_ERR("uart_config_get failed: %d", ret);
			goto out;
		}
		data->uart_cfg_saved = true;
	}

	struct uart_config bl_cfg = {
		.baudrate = cfg->baudrate,
		.parity = cfg->parity_none ? UART_CFG_PARITY_NONE : UART_CFG_PARITY_EVEN,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
		.data_bits = UART_CFG_DATA_BITS_8,
	};

	ret = uart_configure(cfg->uart, &bl_cfg);
	if (ret < 0) {
		LOG_ERR("uart_configure failed: %d", ret);
		goto out;
	}

	ret = stm32_bootloader_gpio_enter(&cfg->gpio);
	if (ret < 0) {
		goto out;
	}

	drain_rx(cfg->uart);
	tx(cfg->uart, STM32_BL_UART_SYNC);
	ret = wait_ack(dev, cfg->timeout_ms);
	if (ret < 0) {
		LOG_ERR("Bootloader sync failed: %d", ret);
		goto out;
	}

	ret = fetch_info(dev, &data->info);
	if (ret < 0) {
		LOG_ERR("fetch_info failed: %d", ret);
		goto out;
	}

	data->connected = true;
	LOG_INF("Connected: BL v%d.%d, PID 0x%04x, %s erase", data->info.bl_version >> 4,
		data->info.bl_version & 0x0F, data->info.pid,
		data->info.has_extended_erase ? "extended" : "standard");

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_exit(const struct device *dev, bool reset_target)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	struct stm32_bl_uart_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->uart_cfg_saved) {
		int r = uart_configure(cfg->uart, &data->uart_cfg_bkp);

		if (r < 0) {
			LOG_ERR("Failed to restore UART config: %d", r);
			ret = r;
		}
	}
	(void)stm32_bootloader_gpio_exit(&cfg->gpio, reset_target);

	data->connected = false;
	LOG_INF("Disconnected");

	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_go(const struct device *dev, uint32_t addr)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	ret = send_cmd(dev, STM32_BL_UART_CMD_GO);
	if (ret < 0) {
		goto out;
	}
	ret = send_addr(dev, addr);
	if (ret < 0) {
		goto out;
	}

	(void)stm32_bootloader_gpio_exit(&cfg->gpio, false);
	data->connected = false;
	LOG_INF("Jumped to 0x%08x", addr);

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_get_info(const struct device *dev, struct stm32_bootloader_info *info)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (info == NULL) {
		return -EINVAL;
	}

	/* Read-only path: bounded lock wait so a long-running operation
	 * (e.g. mass_erase) cannot indefinitely block status callers.
	 */
	if (k_mutex_lock(&data->lock, K_MSEC(cfg->timeout_ms)) != 0) {
		return -EBUSY;
	}
	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	memcpy(info, &data->info, sizeof(*info));
	ret = 0;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_read_memory(const struct device *dev, uint32_t addr, void *buf, size_t len)
{
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}
	if (buf == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	ret = read_chunked(dev, addr, buf, len);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_write_memory(const struct device *dev, uint32_t addr, const void *buf, size_t len)
{
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}
	if (buf == NULL) {
		return -EINVAL;
	}
	/* AN3155: address and length must be 4-byte aligned. */
	if ((addr & 0x3U) != 0 || (len & 0x3U) != 0) {
		LOG_ERR("write_memory addr/len not 4-byte aligned");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	ret = write_chunked(dev, addr, buf, len);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_erase_pages(const struct device *dev, const uint16_t *pages, size_t num_pages)
{
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (num_pages == 0) {
		return 0;
	}
	if (pages == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	ret = do_erase_pages(dev, pages, num_pages);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int api_mass_erase(const struct device *dev)
{
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->connected) {
		ret = -ENOTCONN;
		goto out;
	}
	ret = do_mass_erase(dev);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_API(stm32_bootloader, stm32_bl_uart_api) = {
	.enter = api_enter,
	.exit = api_exit,
	.go = api_go,
	.get_info = api_get_info,
	.read_memory = api_read_memory,
	.write_memory = api_write_memory,
	.erase_pages = api_erase_pages,
	.mass_erase = api_mass_erase,
};

/* 6. ---- DT instantiation ---- */

static int stm32_bl_uart_init(const struct device *dev)
{
	const struct stm32_bl_uart_config *cfg = dev->config;
	struct stm32_bl_uart_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->uart)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	ret = stm32_bootloader_gpio_init(&cfg->gpio);
	if (ret < 0) {
		return ret;
	}

	k_mutex_init(&data->lock);
	data->connected = false;
	data->uart_cfg_saved = false;

	LOG_INF("STM32 bootloader/UART driver initialised");
	return 0;
}

#define GPIO_OR_NULL(inst, name) GPIO_DT_SPEC_INST_GET_OR(inst, name, {0})

#define STM32_BL_UART_DEFINE(inst)                                                                 \
	static struct stm32_bl_uart_data stm32_bl_uart_data_##inst;                                \
                                                                                                   \
	static const struct stm32_bl_uart_config stm32_bl_uart_cfg_##inst = {                      \
		.gpio =                                                                            \
			{                                                                          \
				.boot0 = GPIO_OR_NULL(inst, boot0_gpios),                          \
				.boot1 = GPIO_OR_NULL(inst, boot1_gpios),                          \
				.nrst = GPIO_OR_NULL(inst, nrst_gpios),                            \
				.reset_pulse_ms = DT_INST_PROP(inst, reset_pulse_ms),              \
				.boot_delay_ms = DT_INST_PROP(inst, boot_delay_ms),                \
			},                                                                         \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.baudrate = DT_INST_PROP(inst, baudrate),                                          \
		.timeout_ms = DT_INST_PROP(inst, timeout_ms),                                      \
		.erase_timeout_ms = DT_INST_PROP(inst, erase_timeout_ms),                          \
		.parity_none = DT_INST_PROP(inst, parity_none),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, stm32_bl_uart_init, NULL, &stm32_bl_uart_data_##inst,          \
			      &stm32_bl_uart_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_STM32_BOOTLOADER_UART_INIT_PRIORITY, &stm32_bl_uart_api);

/* Instantiate for UART-hosted nodes only. Future I2C/CAN transports live
 * in their own source files and DT_INST_FOREACH_STATUS_OKAY invocations.
 */
#define STM32_BL_DEFINE(inst) COND_CODE_1(DT_INST_ON_BUS(inst, uart),  \
									(STM32_BL_UART_DEFINE(inst)), ()	\
								)

DT_INST_FOREACH_STATUS_OKAY(STM32_BL_DEFINE)

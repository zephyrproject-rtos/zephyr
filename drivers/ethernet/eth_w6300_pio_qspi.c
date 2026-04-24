/*
 * Copyright (c) 2026 WIZnet Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private PIO QSPI transport for W6300 on Raspberry Pi Pico-class boards.
 *
 * The W6300 transfer is not representable as a generic Zephyr SPI transfer:
 * the instruction byte is sent on IO0, then the address/data phase uses
 * bidirectional IO0-IO3. Keep this transaction engine private to the W6300
 * Ethernet driver instead of registering it as a Zephyr SPI controller.
 */

#define DT_DRV_COMPAT wiznet_w6300

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(eth_w6300, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>

#include "eth_w6300_priv.h"

#define QSPI_DATA_LINES 4
#define QSPI_PIO_CYCLES 2

/* MOD bits for Quad SPI mode in the W6300 instruction byte. */
#define W6300_SPI_MOD_QUAD 0x80

struct w6300_qspi_segment {
	const uint8_t *buf;
	size_t len;
};

/*
 * Mixed-mode PIO program: serial instruction + quad write + quad read
 *
 *  0: pull block          side 0   ; get instruction byte from FIFO
 *  1: set pindirs, 1      side 0   ; IO0=output only (serial mode)
 *  2: out pins, 1         side 0   ; instruction bit 7
 *  3: nop                 side 1   ; clock
 *  4: out pins, 1         side 0   ; bit 6
 *  5: nop                 side 1
 *  6: out pins, 1         side 0   ; bit 5
 *  7: nop                 side 1
 *  8: out pins, 1         side 0   ; bit 4
 *  9: nop                 side 1
 * 10: out pins, 1         side 0   ; bit 3
 * 11: nop                 side 1
 * 12: out pins, 1         side 0   ; bit 2
 * 13: nop                 side 1
 * 14: out pins, 1         side 0   ; bit 1
 * 15: nop                 side 1
 * 16: out pins, 1         side 0   ; bit 0
 * 17: nop                 side 1
 * 18: set pindirs, 0xF    side 0   ; IO0-3=output (quad mode)
 * 19: pull block          side 0   ; get x (quad write nibbles - 1)
 * 20: out x, 32           side 0   ; load x register
 * 21: pull block          side 0   ; get y (read bytes - 1)
 * 22: out y, 32           side 0   ; load y register
 * 23: out pins, 4         side 0   ; quad write nibble (autopull)
 * 24: jmp x--, 23         side 1   ; clock + write loop
 * 25: set pins, 0         side 0   ; clear output pins
 * 26: set pindirs, 0      side 0   ; IO0-3=input (read mode)
 * 27: set x, 0            side 0   ; x=0 -> 2 iterations per byte
 * 28: in pins, 4          side 1   ; read nibble on SCK high phase
 * 29: jmp x--, 28         side 0   ; clock low + nibble loop
 * 30: in pins, 4          side 1   ; last nibble on SCK high phase
 * 31: jmp y--, 27         side 0   ; next byte
 */

#define QSPI_MIX_WRAP_TARGET 0
#define QSPI_MIX_WRAP        31

RPI_PICO_PIO_DEFINE_PROGRAM(qspi_mix, QSPI_MIX_WRAP_TARGET, QSPI_MIX_WRAP,
			    0x80A0, /*  0: pull block          side 0 */
			    0xE081, /*  1: set pindirs, 1      side 0 */
			    0x6001, /*  2: out pins, 1         side 0 */
			    0xB042, /*  3: nop                 side 1 */
			    0x6001, /*  4: out pins, 1         side 0 */
			    0xB042, /*  5: nop                 side 1 */
			    0x6001, /*  6: out pins, 1         side 0 */
			    0xB042, /*  7: nop                 side 1 */
			    0x6001, /*  8: out pins, 1         side 0 */
			    0xB042, /*  9: nop                 side 1 */
			    0x6001, /* 10: out pins, 1         side 0 */
			    0xB042, /* 11: nop                 side 1 */
			    0x6001, /* 12: out pins, 1         side 0 */
			    0xB042, /* 13: nop                 side 1 */
			    0x6001, /* 14: out pins, 1         side 0 */
			    0xB042, /* 15: nop                 side 1 */
			    0x6001, /* 16: out pins, 1         side 0 */
			    0xB042, /* 17: nop                 side 1 */
			    0xE08F, /* 18: set pindirs, 0xF    side 0 */
			    0x80A0, /* 19: pull block          side 0 */
			    0x6020, /* 20: out x, 32           side 0 */
			    0x80A0, /* 21: pull block          side 0 */
			    0x6040, /* 22: out y, 32           side 0 */
			    0x6004, /* 23: out pins, 4         side 0 */
			    0x1057, /* 24: jmp x--, 23         side 1 */
			    0xE000, /* 25: set pins, 0         side 0 */
			    0xE080, /* 26: set pindirs, 0      side 0 */
			    0xE020, /* 27: set x, 0            side 0 */
			    0x5004, /* 28: in pins, 4          side 1 */
			    0x005C, /* 29: jmp x--, 28         side 0 */
			    0x5004, /* 30: in pins, 4          side 1 */
			    0x009B, /* 31: jmp y--, 27         side 0 */
);

static float pio_clkdiv(uint32_t clk_hz, int cycles, uint32_t qspi_hz)
{
	return (float)clk_hz / (float)(cycles * qspi_hz);
}

static inline void sm_put8(PIO pio, uint sm, uint8_t v)
{
	io_rw_8 *fifo = (io_rw_8 *)&pio->txf[sm];

	*fifo = v;
}

static inline uint8_t sm_get8(PIO pio, uint sm)
{
	return (uint8_t)(pio->rxf[sm] & 0xFF);
}

static int config_gpio(const struct gpio_dt_spec *gpio, int mode)
{
	if (!device_is_ready(gpio->port)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(gpio, mode);
}

static bool qspi_try_drain_rx_byte(PIO pio, uint sm, uint8_t *rx_ptr, size_t total_rx,
				   size_t *rx_done)
{
	if (rx_ptr == NULL || *rx_done >= total_rx || pio_sm_is_rx_fifo_empty(pio, sm)) {
		return false;
	}

	rx_ptr[(*rx_done)++] = sm_get8(pio, sm);
	return true;
}

static void qspi_wait_for_tx_space(PIO pio, uint sm)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		k_yield();
	}
}

static void qspi_wait_for_tx_room(PIO pio, uint sm, uint8_t *rx_ptr, size_t total_rx,
				  size_t *rx_done)
{
	while (pio_sm_is_tx_fifo_full(pio, sm)) {
		if (qspi_try_drain_rx_byte(pio, sm, rx_ptr, total_rx, rx_done)) {
			continue;
		}

		k_yield();
	}
}

static void qspi_push_control_words(PIO pio, uint sm, uint8_t inst_byte,
				    uint32_t quad_write_nibbles, size_t total_rx)
{
	qspi_wait_for_tx_space(pio, sm);
	sm_put8(pio, sm, inst_byte);

	qspi_wait_for_tx_space(pio, sm);
	pio_sm_put(pio, sm, (quad_write_nibbles > 0) ? (quad_write_nibbles - 1) : 0);

	qspi_wait_for_tx_space(pio, sm);
	pio_sm_put(pio, sm, (total_rx > 0) ? (total_rx - 1) : 0);
}

static void qspi_push_write_data(PIO pio, uint sm, const struct w6300_qspi_segment *segments,
				 size_t segment_count, uint8_t *rx_ptr, size_t total_rx,
				 size_t *rx_done)
{
	for (size_t i = 0; i < segment_count; i++) {
		const uint8_t *bp = segments[i].buf;

		for (size_t j = 0; j < segments[i].len; j++) {
			qspi_wait_for_tx_room(pio, sm, rx_ptr, total_rx, rx_done);
			sm_put8(pio, sm, bp[j]);
		}
	}
}

static void qspi_drain_rx_data(PIO pio, uint sm, uint8_t *rx_ptr, size_t total_rx, size_t *rx_done)
{
	while (*rx_done < total_rx) {
		if (pio_sm_is_rx_fifo_empty(pio, sm)) {
			k_yield();
			continue;
		}

		rx_ptr[(*rx_done)++] = sm_get8(pio, sm);
	}
}

static void qspi_drain_dummy_rx(PIO pio, uint sm)
{
	for (int i = 0; i < 8; i++) {
		if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
			break;
		}

		arch_nop();
	}

	while (pio_sm_is_rx_fifo_empty(pio, sm)) {
		k_yield();
	}

	(void)sm_get8(pio, sm);
}

static size_t qspi_segment_len(const struct w6300_qspi_segment *segments, size_t segment_count)
{
	size_t total = 0;

	for (size_t i = 0; i < segment_count; i++) {
		total += segments[i].len;
	}

	return total;
}

static int w6300_pio_qspi_transfer(const struct device *dev, uint8_t inst_byte,
				   const struct w6300_qspi_segment *tx_segments,
				   size_t tx_segment_count, uint8_t *rx_buf, size_t rx_len)
{
	const struct w6300_config *cfg = dev->config;
	struct w6300_runtime *ctx = dev->data;
	struct w6300_pio_qspi_data *data = &ctx->qspi;
	PIO pio;
	uint sm;
	size_t rx_done = 0;
	size_t quad_write_bytes;
	uint32_t quad_write_nibbles;
	int rc;

	if (!data->configured) {
		return -EIO;
	}

	quad_write_bytes = qspi_segment_len(tx_segments, tx_segment_count);
	if (quad_write_bytes > (UINT32_MAX / 2)) {
		return -EINVAL;
	}

	quad_write_nibbles = quad_write_bytes * 2;

	k_mutex_lock(&ctx->bus_lock, K_FOREVER);

	pio = (PIO)data->pio;
	sm = data->pio_sm;

	pio_sm_set_enabled(pio, sm, false);
	pio_sm_set_wrap(pio, sm, data->pio_offset + qspi_mix_wrap_target,
			data->pio_offset + qspi_mix_wrap);
	pio_sm_clear_fifos(pio, sm);

	pio_sm_set_pindirs_with_mask(pio, sm, data->io_pin_mask, data->io_pin_mask);

	pio_sm_restart(pio, sm);
	pio_sm_clkdiv_restart(pio, sm);
	pio_sm_exec(pio, sm, pio_encode_jmp(data->pio_offset));

	rc = gpio_pin_set_dt(&cfg->qspi.cs_gpio, 1);
	if (rc < 0) {
		goto out_unlock;
	}

	pio_sm_set_enabled(pio, sm, true);

	qspi_push_control_words(pio, sm, inst_byte, quad_write_nibbles, rx_len);
	qspi_push_write_data(pio, sm, tx_segments, tx_segment_count, rx_buf, rx_len, &rx_done);

	if (rx_buf != NULL && rx_len > 0) {
		qspi_drain_rx_data(pio, sm, rx_buf, rx_len, &rx_done);
	} else {
		qspi_drain_dummy_rx(pio, sm);
	}

	pio_sm_set_enabled(pio, sm, false);

	while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
		(void)sm_get8(pio, sm);
	}

	pio_sm_set_consecutive_pindirs(pio, sm, data->io_base_pin, QSPI_DATA_LINES, false);
	pio_sm_exec(pio, sm, pio_encode_mov(pio_pins, pio_null));

	rc = gpio_pin_set_dt(&cfg->qspi.cs_gpio, 0);

out_unlock:
	k_mutex_unlock(&ctx->bus_lock);
	return rc;
}

int w6300_pio_qspi_init(const struct device *dev)
{
	const struct w6300_config *config = dev->config;
	const struct w6300_pio_qspi_config *cfg = &config->qspi;
	struct w6300_runtime *ctx = dev->data;
	struct w6300_pio_qspi_data *data = &ctx->qspi;
	uint32_t clock_freq;
	float clock_div;
	pio_sm_config sm_config;
	const struct gpio_dt_spec *clk;
	int rc;

	if (data->configured) {
		return 0;
	}

	if (!device_is_ready(cfg->piodev) || !device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	if (cfg->frequency == 0) {
		return -EINVAL;
	}

	rc = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	rc = config_gpio(&cfg->clk_gpio, GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io0_gpio, GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io1_gpio, GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io2_gpio, GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io3_gpio, GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->cs_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		return rc;
	}

	if ((cfg->io1_gpio.pin != cfg->io0_gpio.pin + 1) ||
	    (cfg->io2_gpio.pin != cfg->io0_gpio.pin + 2) ||
	    (cfg->io3_gpio.pin != cfg->io0_gpio.pin + 3)) {
		return -EINVAL;
	}

	rc = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (rc < 0) {
		return rc;
	}

	rc = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &clock_freq);
	if (rc < 0) {
		return rc;
	}

	clk = &cfg->clk_gpio;
	data->pio = pio_rpi_pico_get_pio(cfg->piodev);
	rc = pio_rpi_pico_allocate_sm(cfg->piodev, &data->pio_sm);
	if (rc < 0) {
		return rc;
	}

	data->io_base_pin = cfg->io0_gpio.pin;
	data->io_pin_mask = BIT(cfg->io0_gpio.pin) | BIT(cfg->io1_gpio.pin) |
			    BIT(cfg->io2_gpio.pin) | BIT(cfg->io3_gpio.pin);

	clock_div = pio_clkdiv(clock_freq, QSPI_PIO_CYCLES, cfg->frequency);

	if (!pio_can_add_program((PIO)data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix))) {
		LOG_ERR("No PIO space for W6300 QSPI transport");
		return -ENOMEM;
	}

	data->pio_offset = pio_add_program((PIO)data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix));

	sm_config = pio_get_default_sm_config();
	sm_config_set_clkdiv(&sm_config, clock_div);
	sm_config_set_out_pins(&sm_config, cfg->io0_gpio.pin, QSPI_DATA_LINES);
	sm_config_set_in_pins(&sm_config, cfg->io0_gpio.pin);
	sm_config_set_set_pins(&sm_config, cfg->io0_gpio.pin, QSPI_DATA_LINES);
	sm_config_set_out_shift(&sm_config, false, true, 8);
	sm_config_set_in_shift(&sm_config, false, true, 8);
	sm_config_set_sideset_pins(&sm_config, clk->pin);
	sm_config_set_sideset(&sm_config, 1, false, false);
	sm_config_set_wrap(&sm_config, data->pio_offset + qspi_mix_wrap_target,
			   data->pio_offset + qspi_mix_wrap);

	hw_set_bits(&((PIO)data->pio)->input_sync_bypass, data->io_pin_mask);

	pio_sm_set_pindirs_with_mask((PIO)data->pio, data->pio_sm,
				     BIT(clk->pin) | data->io_pin_mask,
				     BIT(clk->pin) | data->io_pin_mask);
	pio_sm_set_pins_with_mask((PIO)data->pio, data->pio_sm, 0,
				  BIT(clk->pin) | data->io_pin_mask);

	pio_gpio_init((PIO)data->pio, cfg->io0_gpio.pin);
	pio_gpio_init((PIO)data->pio, cfg->io1_gpio.pin);
	pio_gpio_init((PIO)data->pio, cfg->io2_gpio.pin);
	pio_gpio_init((PIO)data->pio, cfg->io3_gpio.pin);
	pio_gpio_init((PIO)data->pio, clk->pin);

	for (uint p = cfg->io0_gpio.pin; p <= cfg->io3_gpio.pin; p++) {
		gpio_set_input_hysteresis_enabled(p, true);
	}
	gpio_set_pulls(clk->pin, false, false);

	pio_sm_init((PIO)data->pio, data->pio_sm, data->pio_offset, &sm_config);
	pio_sm_exec((PIO)data->pio, data->pio_sm, pio_encode_set(pio_pins, 1));

	data->configured = true;
	return 0;
}

int w6300_pio_qspi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	uint8_t cmd[4] = {
		W6300_SPI_MOD_QUAD | W6300_SPI_READ | (addr & 0x1F), (addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF, 0x00,
	};
	const struct w6300_qspi_segment segments[] = {
		{
			.buf = &cmd[1],
			.len = 3,
		},
	};

	if (len == 0) {
		return 0;
	}

	return w6300_pio_qspi_transfer(dev, cmd[0], segments, ARRAY_SIZE(segments), data, len);
}

int w6300_pio_qspi_write(const struct device *dev, uint32_t addr, const uint8_t *data, size_t len)
{
	uint8_t cmd_tail[3] = {
		(addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF,
		0x00,
	};
	const struct w6300_qspi_segment segments[] = {
		{
			.buf = cmd_tail,
			.len = sizeof(cmd_tail),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	uint8_t inst_byte = W6300_SPI_MOD_QUAD | W6300_SPI_WRITE | (addr & 0x1F);

	if (len == 0) {
		return 0;
	}

	return w6300_pio_qspi_transfer(dev, inst_byte, segments, ARRAY_SIZE(segments), NULL, 0);
}

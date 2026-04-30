/*
 * Copyright (c) 2026 WIZnet Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *
 * QSPI PIO driver for W6300 — mixed-mode serial+quad.
 *
 * Single PIO program (32 instructions) handles:
 *   Phase 1: Serial 1-bit instruction (8 SCLK, IO0 only)
 *   Phase 2: Quad 4-bit write (x nibbles, IO0-IO3)
 *   Phase 3: Quad 4-bit read (y bytes, IO0-IO3)
 *
 * FIFO sequence: [instruction_byte] [x_count] [y_count] [write_data...]
 * Host pushes instruction byte, then x/y counts, then write data.
 * PIO handles serial→quad transition and write→read transition internally.
 */

#define DT_DRV_COMPAT raspberrypi_pico_spi_pio_qspi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qspi_pico_pio);

#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include "spi_context.h"

#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <hardware/pio.h>
#include "hardware/clocks.h"

#define QSPI_DATA_LINES 4
#define QSPI_PIO_CYCLES 2

struct qspi_pico_pio_config {
	const struct device *piodev;
	const struct pinctrl_dev_config *pin_cfg;
	struct gpio_dt_spec clk_gpio;
	struct gpio_dt_spec io0_gpio;
	struct gpio_dt_spec io1_gpio;
	struct gpio_dt_spec io2_gpio;
	struct gpio_dt_spec io3_gpio;
	const struct device *clk_dev;
	clock_control_subsys_t clk_id;
};

struct qspi_pico_pio_data {
	struct spi_context spi_ctx;
	PIO pio;
	size_t pio_sm;
	uint32_t pio_offset;
	uint32_t io_base_pin;
	uint32_t io_pin_mask;
};

/* ----------------------------------------------------------------
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
 * 27: set x, 0            side 0   ; x=0 → 2 iterations per byte
 * 28: in pins, 4          side 1   ; read nibble on SCK high phase
 * 29: jmp x--, 28         side 0   ; clock low + nibble loop
 * 30: in pins, 4          side 1   ; last nibble on SCK high phase
 * 31: jmp y--, 27         side 0   ; next byte
 * ----------------------------------------------------------------
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

/* --- Helpers --- */

static float pio_clkdiv(uint32_t clk_hz, int cycles, uint32_t spi_hz)
{
	return (float)clk_hz / (float)(cycles * spi_hz);
}

static inline void sm_put8(PIO pio, uint sm, uint8_t v)
{
	io_rw_8 *f = (io_rw_8 *)&pio->txf[sm];
	*f = v;
}

static inline uint8_t sm_get8(PIO pio, uint sm)
{
	return (uint8_t)(pio->rxf[sm] & 0xFF);
}

/* --- Configure --- */

static int qspi_configure(const struct qspi_pico_pio_config *cfg, struct qspi_pico_pio_data *data,
			  const struct spi_config *spi_cfg)
{
	int rc;
	uint32_t clock_freq;
	float clock_div;
	const struct gpio_dt_spec *clk;
	pio_sm_config sm_config;

	rc = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (rc < 0) {
		return rc;
	}

	rc = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &clock_freq);
	if (rc < 0) {
		return rc;
	}

	if (spi_context_configured(&data->spi_ctx, spi_cfg)) {
		return 0;
	}

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		return -ENOTSUP;
	}
	if (spi_cfg->operation & SPI_TRANSFER_LSB) {
		return -ENOTSUP;
	}
	if (SPI_MODE_GET(spi_cfg->operation) & (SPI_MODE_CPOL | SPI_MODE_CPHA)) {
		return -ENOTSUP;
	}

	if (!(spi_cfg->operation & SPI_HALF_DUPLEX)) {
		LOG_WRN("QSPI is half-duplex; forcing SPI_HALF_DUPLEX mode");
	}

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		gpio_set_outover(data->spi_ctx.config->cs.gpio.pin, GPIO_OVERRIDE_INVERT);
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

	clock_div = pio_clkdiv(clock_freq, QSPI_PIO_CYCLES, spi_cfg->frequency);

	if (!pio_can_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix))) {
		LOG_ERR("No PIO space");
		return -ENOMEM;
	}
	data->pio_offset = pio_add_program(data->pio, RPI_PICO_PIO_GET_PROGRAM(qspi_mix));

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

	hw_set_bits(&data->pio->input_sync_bypass, data->io_pin_mask);

	pio_sm_set_pindirs_with_mask(data->pio, data->pio_sm, BIT(clk->pin) | data->io_pin_mask,
				     BIT(clk->pin) | data->io_pin_mask);
	pio_sm_set_pins_with_mask(data->pio, data->pio_sm, 0, BIT(clk->pin) | data->io_pin_mask);

	pio_gpio_init(data->pio, cfg->io0_gpio.pin);
	pio_gpio_init(data->pio, cfg->io1_gpio.pin);
	pio_gpio_init(data->pio, cfg->io2_gpio.pin);
	pio_gpio_init(data->pio, cfg->io3_gpio.pin);
	pio_gpio_init(data->pio, clk->pin);

	for (uint p = cfg->io0_gpio.pin; p <= cfg->io3_gpio.pin; p++) {
		gpio_set_input_hysteresis_enabled(p, true);
	}
	gpio_set_pulls(clk->pin, false, false);

	pio_sm_init(data->pio, data->pio_sm, data->pio_offset, &sm_config);
	pio_sm_exec(data->pio, data->pio_sm, pio_encode_set(pio_pins, 1));

	data->spi_ctx.config = spi_cfg;
	return 0;
}

static size_t qspi_buf_set_len(const struct spi_buf_set *bufs)
{
	size_t total = 0;

	if (bufs == NULL) {
		return 0;
	}

	for (size_t i = 0; i < bufs->count; i++) {
		total += bufs->buffers[i].len;
	}

	return total;
}

static uint8_t *qspi_get_rx_ptr(const struct spi_buf_set *rx_bufs)
{
	if (rx_bufs == NULL || rx_bufs->count == 0 || rx_bufs->buffers[0].buf == NULL) {
		return NULL;
	}

	return rx_bufs->buffers[0].buf;
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

static void qspi_push_write_data(PIO pio, uint sm, const struct spi_buf_set *tx_bufs,
				 uint8_t *rx_ptr, size_t total_rx, size_t *rx_done)
{
	bool first_buf = true;

	if (tx_bufs == NULL) {
		return;
	}

	for (size_t i = 0; i < tx_bufs->count; i++) {
		const uint8_t *bp = tx_bufs->buffers[i].buf;
		size_t blen = tx_bufs->buffers[i].len;
		size_t start = first_buf ? 1 : 0;

		first_buf = false;

		for (size_t j = start; j < blen; j++) {
			qspi_wait_for_tx_room(pio, sm, rx_ptr, total_rx, rx_done);
			sm_put8(pio, sm, bp[j]);
		}
	}
}

static void qspi_drain_rx_data(PIO pio, uint sm, uint8_t *rx_ptr, size_t total_rx, size_t *rx_done)
{
	while (*rx_done < total_rx) {
		if (pio_sm_is_rx_fifo_empty(pio, sm)) {
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

/* --- Transceive --- */

static int qspi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
			   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const struct qspi_pico_pio_config *cfg = dev->config;
	struct qspi_pico_pio_data *data = dev->data;
	struct spi_context *ctx = &data->spi_ctx;
	PIO pio;
	uint sm;
	int rc;

	/*
	 * W6300 transaction layout from eth_w6300.c:
	 *
	 * Read:  TX buf[0] = opcode(1B) + addr_hi(1B) + addr_lo(1B) + dummy(1B) = 4B
	 *        RX buf[0] = data (N bytes)
	 *
	 * Write: TX buf[0] = opcode(1B) + addr_hi(1B) + addr_lo(1B) + dummy(1B) = 4B
	 *        TX buf[1] = data (N bytes)
	 *        RX = NULL
	 *
	 * The first byte of TX buf[0] is the instruction byte (sent 1-bit serial).
	 * Remaining TX bytes are sent quad (4-bit).
	 * RX bytes are received quad (4-bit).
	 */

	size_t total_tx = qspi_buf_set_len(tx_bufs);
	size_t total_rx = qspi_buf_set_len(rx_bufs);
	size_t rx_done = 0;

	if (total_tx < 1) {
		return -EINVAL; /* need at least instruction byte */
	}

	/* First byte of first TX buf = instruction byte (serial) */
	uint8_t inst_byte = ((const uint8_t *)tx_bufs->buffers[0].buf)[0];

	/* Remaining TX = quad write data (addr + dummy + data) */
	/* quad_write_bytes = total_tx - 1 (instruction byte) */
	size_t quad_write_bytes = total_tx - 1;
	uint32_t quad_write_nibbles = quad_write_bytes * 2;
	uint8_t *rx_ptr = qspi_get_rx_ptr(rx_bufs);

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);

	rc = qspi_configure(cfg, data, spi_cfg);
	if (rc < 0) {
		spi_context_release(ctx, rc);
		return rc;
	}

	pio = data->pio;
	sm = data->pio_sm;

	/* --- Setup PIO SM --- */
	pio_sm_set_enabled(pio, sm, false);
	pio_sm_set_wrap(pio, sm, data->pio_offset + qspi_mix_wrap_target,
			data->pio_offset + qspi_mix_wrap);
	pio_sm_clear_fifos(pio, sm);

	/* IO pins as output (serial phase sets pindirs itself) */
	pio_sm_set_pindirs_with_mask(pio, sm, data->io_pin_mask, data->io_pin_mask);

	pio_sm_restart(pio, sm);
	pio_sm_clkdiv_restart(pio, sm);
	pio_sm_exec(pio, sm, pio_encode_jmp(data->pio_offset));

	/* Assert CS */
	spi_context_cs_control(ctx, true);

	pio_sm_set_enabled(pio, sm, true);

	/*
	 * FIFO sequence:
	 *   1. instruction byte (PIO pulls, shifts out 1-bit serial)
	 *   2. x = quad_write_nibbles - 1 (PIO pulls, loads x)
	 *   3. y = read_bytes - 1 (PIO pulls, loads y)
	 *   4. quad write data bytes (PIO autopulls for out pins, 4)
	 */

	qspi_push_control_words(pio, sm, inst_byte, quad_write_nibbles, total_rx);
	qspi_push_write_data(pio, sm, tx_bufs, rx_ptr, total_rx, &rx_done);

	/* Drain RX */
	if (rx_ptr && total_rx > 0) {
		qspi_drain_rx_data(pio, sm, rx_ptr, total_rx, &rx_done);
	} else {
		/*
		 * Write-only: PIO always executes the read phase.
		 * y was pushed as 0, so the read loop runs once (y+1 = 1 byte).
		 * Drain that dummy byte to keep the RX FIFO clean.
		 */
		qspi_drain_dummy_rx(pio, sm);
	}

	pio_sm_set_enabled(pio, sm, false);

	/* Flush any stale RX FIFO entries */
	while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
		(void)sm_get8(pio, sm);
	}

	pio_sm_set_consecutive_pindirs(pio, sm, data->io_base_pin, 4, false);
	pio_sm_exec(pio, sm, pio_encode_mov(pio_pins, pio_null));

	spi_context_cs_control(ctx, false);
	spi_context_release(ctx, 0);
	return 0;
}

static int qspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct qspi_pico_pio_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->spi_ctx);
	return 0;
}

static DEVICE_API(spi, qspi_api) = {
	.transceive = qspi_transceive,
	.release = qspi_release,
};

/* --- Init --- */

static int config_gpio(const struct gpio_dt_spec *gpio, const char *tag, int mode)
{
	ARG_UNUSED(tag);
	if (!device_is_ready(gpio->port)) {
		return -ENODEV;
	}
	return gpio_pin_configure_dt(gpio, mode);
}

static int qspi_init(const struct device *dev)
{
	const struct qspi_pico_pio_config *cfg = dev->config;
	struct qspi_pico_pio_data *data = dev->data;
	int rc;

	rc = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (rc) {
		return rc;
	}

	rc = config_gpio(&cfg->clk_gpio, "clk", GPIO_OUTPUT_ACTIVE);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io0_gpio, "io0", GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io1_gpio, "io1", GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io2_gpio, "io2", GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}
	rc = config_gpio(&cfg->io3_gpio, "io3", GPIO_OUTPUT);
	if (rc < 0) {
		return rc;
	}

	if ((cfg->io1_gpio.pin != cfg->io0_gpio.pin + 1) ||
	    (cfg->io2_gpio.pin != cfg->io0_gpio.pin + 2) ||
	    (cfg->io3_gpio.pin != cfg->io0_gpio.pin + 3)) {
		return -EINVAL;
	}

	rc = spi_context_cs_configure_all(&data->spi_ctx);
	if (rc < 0) {
		return rc;
	}

	spi_context_unlock_unconditionally(&data->spi_ctx);
	return 0;
}

/* --- Instantiation --- */

#define QSPI_INIT(n)                                                                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static struct qspi_pico_pio_config qspi_cfg_##n = {                                        \
		.piodev = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.clk_gpio = GPIO_DT_SPEC_INST_GET(n, clk_gpios),                                   \
		.io0_gpio = GPIO_DT_SPEC_INST_GET(n, io0_gpios),                                   \
		.io1_gpio = GPIO_DT_SPEC_INST_GET(n, io1_gpios),                                   \
		.io2_gpio = GPIO_DT_SPEC_INST_GET(n, io2_gpios),                                   \
		.io3_gpio = GPIO_DT_SPEC_INST_GET(n, io3_gpios),                                   \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                  \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(n, clocks, 0, clk_id),        \
	};                                                                                         \
	static struct qspi_pico_pio_data qspi_data_##n = {                                         \
		SPI_CONTEXT_INIT_LOCK(qspi_data_##n, spi_ctx),                                     \
		SPI_CONTEXT_INIT_SYNC(qspi_data_##n, spi_ctx),                                     \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), spi_ctx)};                         \
	SPI_DEVICE_DT_INST_DEFINE(n, qspi_init, NULL, &qspi_data_##n, &qspi_cfg_##n, POST_KERNEL,  \
				  CONFIG_SPI_INIT_PRIORITY, &qspi_api);

DT_INST_FOREACH_STATUS_OKAY(QSPI_INIT)

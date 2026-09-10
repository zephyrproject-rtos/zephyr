/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_emul.c
 * @brief Recording SPI emulator for the CS47L63
 *
 * A frame that is not the driver's write or read shape fails with -EIO. EINT registers
 * are write-1-to-clear; every other register reads back what was written.
 */

#include "cs47l63_emul.h"

#include <errno.h>
#include <string.h>

#include <zephyr/audio/codec.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/zassert.h>

#include "cs47l63.h"

#define DT_DRV_COMPAT cirrus_cs47l63

ZASSERT_MODULE(DEFAULT);

#define CS47L63_EMUL_READ_FLAG 0x80000000U

#define CS47L63_EMUL_WORD_BYTES  4U
#define CS47L63_EMUL_READ_TX_LEN (2U * CS47L63_EMUL_WORD_BYTES)
#define CS47L63_EMUL_WRITE_LEN   (3U * CS47L63_EMUL_WORD_BYTES)

#define CS47L63_EMUL_MAX_REGS 64U

#define CS47L63_EMUL_MAX_XFERS 512U

#define CS47L63_EMUL_REVID 0x00000000U

#define CS47L63_EMUL_OTPID 0x00000008U

struct cs47l63_emul_reg {
	uint32_t addr;
	uint32_t val;
};

struct cs47l63_emul_data {
	struct cs47l63_emul_reg regs[CS47L63_EMUL_MAX_REGS];
	uint32_t num_regs;
	struct cs47l63_emul_xfer log[CS47L63_EMUL_MAX_XFERS];
	uint32_t num_xfers;
	uint32_t fail_addr;
	bool fail_armed;
	uint32_t fail_write_addr;
	bool fail_write_armed;
	uint32_t fail_hits;
	int write_ret;
};

static bool is_write_one_to_clear(uint32_t addr)
{
	return (addr == CS47L63_IRQ1_EINT_1) || (addr == CS47L63_IRQ1_EINT_2) ||
	       (addr == CS47L63_IRQ1_EINT_6);
}

static struct cs47l63_emul_reg *find_reg(struct cs47l63_emul_data *data, uint32_t addr)
{
	for (uint32_t i = 0U; i < data->num_regs; i++) {
		if (data->regs[i].addr == addr) {
			return &data->regs[i];
		}
	}

	return NULL;
}

static void store_reg(struct cs47l63_emul_data *data, uint32_t addr, uint32_t val)
{
	struct cs47l63_emul_reg *reg = find_reg(data, addr);

	if (reg != NULL) {
		reg->val = val;
		return;
	}

	ZASSERT(data->num_regs < CS47L63_EMUL_MAX_REGS, "CS47L63 emulator register file full");
	data->regs[data->num_regs].addr = addr;
	data->regs[data->num_regs].val = val;
	data->num_regs++;
}

static uint32_t load_reg(struct cs47l63_emul_data *data, uint32_t addr)
{
	const struct cs47l63_emul_reg *reg = find_reg(data, addr);

	return (reg != NULL) ? reg->val : 0U;
}

static void record(struct cs47l63_emul_data *data, uint32_t addr, uint32_t val, bool write)
{
	ZASSERT(data->num_xfers < CS47L63_EMUL_MAX_XFERS, "CS47L63 emulator log full");
	data->log[data->num_xfers].addr = addr;
	data->log[data->num_xfers].val = val;
	data->log[data->num_xfers].write = write;
	data->num_xfers++;
}

void cs47l63_emul_reset(const struct emul *target)
{
	struct cs47l63_emul_data *data = target->data;

	(void)memset(data, 0, sizeof(*data));

	store_reg(data, CS47L63_DEVID, CS47L63_DEVID_CS47L63);
	store_reg(data, CS47L63_REVID, CS47L63_EMUL_REVID);
	store_reg(data, CS47L63_OTPID, CS47L63_EMUL_OTPID);
	store_reg(data, CS47L63_IRQ1_EINT_2, CS47L63_BOOT_DONE_EINT1);
}

void cs47l63_emul_set_reg(const struct emul *target, uint32_t addr, uint32_t val)
{
	store_reg(target->data, addr, val);
}

uint32_t cs47l63_emul_get_reg(const struct emul *target, uint32_t addr)
{
	return load_reg(target->data, addr);
}

uint32_t cs47l63_emul_xfer_count(const struct emul *target)
{
	const struct cs47l63_emul_data *data = target->data;

	return data->num_xfers;
}

bool cs47l63_emul_xfer_get(const struct emul *target, uint32_t idx, struct cs47l63_emul_xfer *out)
{
	const struct cs47l63_emul_data *data = target->data;

	if ((idx >= data->num_xfers) || (out == NULL)) {
		return false;
	}

	*out = data->log[idx];

	return true;
}

static int nth_index(const struct emul *target, uint32_t addr, uint32_t nth, bool write)
{
	const struct cs47l63_emul_data *data = target->data;
	uint32_t seen = 0U;

	for (uint32_t i = 0U; i < data->num_xfers; i++) {
		if ((data->log[i].addr != addr) || (data->log[i].write != write)) {
			continue;
		}
		if (seen == nth) {
			return (int)i;
		}
		seen++;
	}

	return -1;
}

static uint32_t count_of(const struct emul *target, uint32_t addr, bool write)
{
	const struct cs47l63_emul_data *data = target->data;
	uint32_t count = 0U;

	for (uint32_t i = 0U; i < data->num_xfers; i++) {
		if ((data->log[i].addr == addr) && (data->log[i].write == write)) {
			count++;
		}
	}

	return count;
}

uint32_t cs47l63_emul_write_count(const struct emul *target, uint32_t addr)
{
	return count_of(target, addr, true);
}

uint32_t cs47l63_emul_read_count(const struct emul *target, uint32_t addr)
{
	return count_of(target, addr, false);
}

int cs47l63_emul_write_index(const struct emul *target, uint32_t addr, uint32_t nth)
{
	return nth_index(target, addr, nth, true);
}

int cs47l63_emul_read_index(const struct emul *target, uint32_t addr, uint32_t nth)
{
	return nth_index(target, addr, nth, false);
}

bool cs47l63_emul_nth_write(const struct emul *target, uint32_t addr, uint32_t nth, uint32_t *val)
{
	const struct cs47l63_emul_data *data = target->data;
	int idx = nth_index(target, addr, nth, true);

	if ((idx < 0) || (val == NULL)) {
		return false;
	}

	*val = data->log[idx].val;

	return true;
}

void cs47l63_emul_fail_at(const struct emul *target, uint32_t addr)
{
	struct cs47l63_emul_data *data = target->data;

	data->fail_addr = addr;
	data->fail_armed = true;
}

void cs47l63_emul_fail_write_at(const struct emul *target, uint32_t addr)
{
	struct cs47l63_emul_data *data = target->data;

	data->fail_write_addr = addr;
	data->fail_write_armed = true;
}

void cs47l63_emul_write_returns(const struct emul *target, int ret)
{
	struct cs47l63_emul_data *data = target->data;

	data->write_ret = ret;
}

uint32_t cs47l63_emul_fail_hits(const struct emul *target)
{
	const struct cs47l63_emul_data *data = target->data;

	return data->fail_hits;
}

struct audio_codec_cfg cs47l63_test_cfg(audio_route_t route)
{
	return (struct audio_codec_cfg){
		.mclk_freq = CS47L63_EMUL_MCLK_HZ,
		.dai_type = AUDIO_DAI_TYPE_I2S,
		.dai_route = route,
		.dai_cfg.i2s = {
			.word_size = CS47L63_EMUL_WORD_BITS,
			.channels = CS47L63_EMUL_CHANNELS,
			.format = I2S_FMT_DATA_FORMAT_I2S,
			.options = I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET,
			.frame_clk_freq = CS47L63_EMUL_RATE_HZ,
		},
	};
}

int cs47l63_test_configure(const struct device *dev, audio_route_t route)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(route);

	return audio_codec_configure(dev, &cfg);
}

static int handle_read(struct cs47l63_emul_data *data, const struct spi_buf_set *tx_bufs,
		       const struct spi_buf_set *rx_bufs)
{
	uint32_t addr;
	uint32_t val;

	if ((tx_bufs == NULL) || (tx_bufs->count != 1U) ||
	    (tx_bufs->buffers[0].len != CS47L63_EMUL_READ_TX_LEN) ||
	    (tx_bufs->buffers[0].buf == NULL)) {
		return -EIO;
	}
	if ((rx_bufs->count != 2U) || (rx_bufs->buffers[1].buf == NULL) ||
	    (rx_bufs->buffers[1].len != CS47L63_EMUL_WORD_BYTES)) {
		return -EIO;
	}

	addr = sys_get_be32(tx_bufs->buffers[0].buf);
	if ((addr & CS47L63_EMUL_READ_FLAG) == 0U) {
		return -EIO;
	}
	addr &= ~CS47L63_EMUL_READ_FLAG;

	if (sys_get_be32((const uint8_t *)tx_bufs->buffers[0].buf + CS47L63_EMUL_WORD_BYTES) !=
	    0U) {
		return -EIO;
	}

	if (data->fail_armed && (data->fail_addr == addr)) {
		data->fail_hits++;
		return -EIO;
	}

	val = load_reg(data, addr);
	sys_put_be32(val, rx_bufs->buffers[1].buf);
	record(data, addr, val, false);

	return 0;
}

static int handle_write(struct cs47l63_emul_data *data, const struct spi_buf_set *tx_bufs)
{
	const uint8_t *tx;
	uint32_t addr;
	uint32_t val;

	if ((tx_bufs == NULL) || (tx_bufs->count != 1U) ||
	    (tx_bufs->buffers[0].len != CS47L63_EMUL_WRITE_LEN) ||
	    (tx_bufs->buffers[0].buf == NULL)) {
		return -EIO;
	}

	tx = tx_bufs->buffers[0].buf;
	addr = sys_get_be32(tx);
	if ((addr & CS47L63_EMUL_READ_FLAG) != 0U) {
		return -EIO;
	}
	if (sys_get_be32(tx + CS47L63_EMUL_WORD_BYTES) != 0U) {
		return -EIO;
	}
	val = sys_get_be32(tx + (2U * CS47L63_EMUL_WORD_BYTES));

	if ((data->fail_armed && (data->fail_addr == addr)) ||
	    (data->fail_write_armed && (data->fail_write_addr == addr))) {
		data->fail_hits++;
		return -EIO;
	}

	record(data, addr, val, true);

	if (is_write_one_to_clear(addr)) {
		store_reg(data, addr, load_reg(data, addr) & ~val);
	} else {
		store_reg(data, addr, val);
	}

	return data->write_ret;
}

static int cs47l63_emul_io(const struct emul *target, const struct spi_config *config,
			   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	struct cs47l63_emul_data *data = target->data;

	ARG_UNUSED(config);

	if (rx_bufs != NULL) {
		return handle_read(data, tx_bufs, rx_bufs);
	}

	return handle_write(data, tx_bufs);
}

static int cs47l63_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	cs47l63_emul_reset(target);

	return 0;
}

static const struct spi_emul_api cs47l63_emul_api = {
	.io = cs47l63_emul_io,
};

#define CS47L63_EMUL_DEFINE(n)                                                                     \
	static struct cs47l63_emul_data cs47l63_emul_data_##n;                                     \
	EMUL_DT_INST_DEFINE(n, cs47l63_emul_init, &cs47l63_emul_data_##n, NULL, &cs47l63_emul_api, \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(CS47L63_EMUL_DEFINE)

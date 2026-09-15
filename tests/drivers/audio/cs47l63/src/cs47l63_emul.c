/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_emul.c
 * @brief Recording SPI emulator for the CS47L63
 *
 * The wire format is the one cs47l63_bus.c documents: three 32-bit words, most
 * significant byte first. A write clocks out address, a zero padding word and
 * data in one buffer. A read clocks out address with the top bit set plus the
 * padding word, and takes the data word back in a second receive buffer. A
 * frame that does not match either shape is refused with -EIO rather than
 * guessed at, so a driver that changes its framing fails here instead of
 * quietly passing.
 *
 * The interrupt edge registers are modelled write-1-to-clear, as the part has
 * them. Nothing else about a register is interpreted: a value written is the
 * value read back.
 */

#include "cs47l63_emul.h"

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "cs47l63_regs.h"

#define DT_DRV_COMPAT cirrus_cs47l63

/** Marks the address word of a read transaction. */
#define CS47L63_EMUL_READ_FLAG 0x80000000U

#define CS47L63_EMUL_WORD_BYTES  4
#define CS47L63_EMUL_READ_TX_LEN (2 * CS47L63_EMUL_WORD_BYTES)
#define CS47L63_EMUL_WRITE_LEN   (3 * CS47L63_EMUL_WORD_BYTES)

/** Distinct registers one test case can touch. */
#define CS47L63_EMUL_MAX_REGS 64

/** Transactions one test case can record. */
#define CS47L63_EMUL_MAX_XFERS 512

/** Device ID a healthy part answers with: any value that is neither reading a
 *  dead bus gives - all zeros - nor a floating one - all ones.
 */
#define CS47L63_EMUL_DEVID 0x00006363U

/** Silicon revision A0, metal revision 0. */
#define CS47L63_EMUL_REVID 0x00000000U

/** The OTP variant whose trim block cs47l63_boot.c knows how to write. */
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
};

/** @brief The interrupt edge registers, whose bits are write-1-to-clear. */
static bool is_write_one_to_clear(uint32_t addr)
{
	return addr == CS47L63_IRQ1_EINT_1 || addr == CS47L63_IRQ1_EINT_2 ||
	       addr == CS47L63_IRQ1_EINT_6;
}

static struct cs47l63_emul_reg *find_reg(struct cs47l63_emul_data *data, uint32_t addr)
{
	for (uint32_t i = 0; i < data->num_regs; i++) {
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

	__ASSERT(data->num_regs < CS47L63_EMUL_MAX_REGS, "CS47L63 emulator register file full");
	data->regs[data->num_regs].addr = addr;
	data->regs[data->num_regs].val = val;
	data->num_regs++;
}

static uint32_t load_reg(struct cs47l63_emul_data *data, uint32_t addr)
{
	const struct cs47l63_emul_reg *reg = find_reg(data, addr);

	return (reg != NULL) ? reg->val : 0;
}

static void record(struct cs47l63_emul_data *data, uint32_t addr, uint32_t val, bool write)
{
	__ASSERT(data->num_xfers < CS47L63_EMUL_MAX_XFERS, "CS47L63 emulator log full");
	data->log[data->num_xfers].addr = addr;
	data->log[data->num_xfers].val = val;
	data->log[data->num_xfers].write = write;
	data->num_xfers++;
}

void cs47l63_emul_reset(const struct emul *target)
{
	struct cs47l63_emul_data *data = target->data;

	memset(data, 0, sizeof(*data));

	store_reg(data, CS47L63_DEVID, CS47L63_EMUL_DEVID);
	store_reg(data, CS47L63_REVID, CS47L63_EMUL_REVID);
	store_reg(data, CS47L63_OTPID, CS47L63_EMUL_OTPID);
	store_reg(data, CS47L63_IRQ1_EINT_2, CS47L63_BOOT_DONE_EINT1);

	data->num_xfers = 0;
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

	if (idx >= data->num_xfers || out == NULL) {
		return false;
	}

	*out = data->log[idx];

	return true;
}

static int nth_index(const struct emul *target, uint32_t addr, uint32_t nth, bool write)
{
	const struct cs47l63_emul_data *data = target->data;
	uint32_t seen = 0;

	for (uint32_t i = 0; i < data->num_xfers; i++) {
		if (data->log[i].addr != addr || data->log[i].write != write) {
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
	uint32_t count = 0;

	for (uint32_t i = 0; i < data->num_xfers; i++) {
		if (data->log[i].addr == addr && data->log[i].write == write) {
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

	if (idx < 0 || val == NULL) {
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

/** @brief Serve a read: address plus padding out, one data word back. */
static int handle_read(struct cs47l63_emul_data *data, const struct spi_buf_set *tx_bufs,
		       const struct spi_buf_set *rx_bufs)
{
	uint32_t addr;
	uint32_t val;

	if (tx_bufs == NULL || tx_bufs->count != 1 ||
	    tx_bufs->buffers[0].len != CS47L63_EMUL_READ_TX_LEN ||
	    tx_bufs->buffers[0].buf == NULL) {
		return -EIO;
	}
	if (rx_bufs->count != 2 || rx_bufs->buffers[1].buf == NULL ||
	    rx_bufs->buffers[1].len != CS47L63_EMUL_WORD_BYTES) {
		return -EIO;
	}

	addr = sys_get_be32(tx_bufs->buffers[0].buf);
	if ((addr & CS47L63_EMUL_READ_FLAG) == 0) {
		return -EIO;
	}
	addr &= ~CS47L63_EMUL_READ_FLAG;

	/* The padding word the part uses to turn the bus around. A driver that
	 * omits it would land its data word here.
	 */
	if (sys_get_be32((const uint8_t *)tx_bufs->buffers[0].buf + CS47L63_EMUL_WORD_BYTES) != 0) {
		return -EIO;
	}

	if (data->fail_armed && data->fail_addr == addr) {
		return -EIO;
	}

	val = load_reg(data, addr);
	sys_put_be32(val, rx_bufs->buffers[1].buf);
	record(data, addr, val, false);

	return 0;
}

/** @brief Accept a write: address, padding and data in one buffer. */
static int handle_write(struct cs47l63_emul_data *data, const struct spi_buf_set *tx_bufs)
{
	const uint8_t *tx;
	uint32_t addr;
	uint32_t val;

	if (tx_bufs == NULL || tx_bufs->count != 1 ||
	    tx_bufs->buffers[0].len != CS47L63_EMUL_WRITE_LEN || tx_bufs->buffers[0].buf == NULL) {
		return -EIO;
	}

	tx = tx_bufs->buffers[0].buf;
	addr = sys_get_be32(tx);
	if ((addr & CS47L63_EMUL_READ_FLAG) != 0) {
		return -EIO;
	}
	if (sys_get_be32(tx + CS47L63_EMUL_WORD_BYTES) != 0) {
		return -EIO;
	}
	val = sys_get_be32(tx + 2 * CS47L63_EMUL_WORD_BYTES);

	if (data->fail_armed && data->fail_addr == addr) {
		return -EIO;
	}

	record(data, addr, val, true);

	if (is_write_one_to_clear(addr)) {
		store_reg(data, addr, load_reg(data, addr) & ~val);
	} else {
		store_reg(data, addr, val);
	}

	return 0;
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

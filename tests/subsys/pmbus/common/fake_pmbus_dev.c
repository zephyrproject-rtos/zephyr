/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/pmbus/pmbus_cmd.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "fake_pmbus_dev.h"

#define TO_FAKE(cfg) CONTAINER_OF(cfg, struct fake_pmbus_dev, cfg)

void fake_pmbus_dev_reset(struct fake_pmbus_dev *fake)
{
	struct i2c_target_config cfg = fake->cfg;
	uint8_t pages = fake->pages;
	uint8_t vout_mode = fake->vout_mode;

	memset(fake, 0, sizeof(*fake));
	fake->cfg = cfg;
	fake->pages = pages;
	fake->vout_mode = vout_mode;
	for (uint8_t i = 0U; i < pages; i++) {
		fake->regs[i][PMBUS_VOUT_MODE] = vout_mode;
	}
}

static uint8_t addr_crc(struct fake_pmbus_dev *fake, uint8_t crc, bool read)
{
	uint8_t byte = (fake->cfg.address << 1) | (read ? 1U : 0U);

	return crc8_ccitt(crc, &byte, 1);
}

/* The read length is not on the bus, so it comes from the command as in a real device */
static bool byte_cmd(uint8_t cmd)
{
	switch (cmd) {
	case PMBUS_PAGE:
	case PMBUS_OPERATION:
	case PMBUS_CAPABILITY:
	case PMBUS_VOUT_MODE:
	case PMBUS_STATUS_BYTE:
		return true;
	default:
		return cmd >= PMBUS_STATUS_VOUT && cmd <= PMBUS_STATUS_FANS_3_4;
	}
}

static void fake_pmbus_dev_write(struct fake_pmbus_dev *fake, const uint8_t *wr, size_t len)
{
	uint8_t cmd = wr[0];

	if (cmd == PMBUS_PAGE && len == 2U) {
		fake->page = wr[1];
		fake->page_writes++;
		return;
	}

	if (cmd == PMBUS_CLEAR_FAULTS && len == 1U) {
		fake->cml = 0U;
		fake->clear_faults++;
		fake->clear_faults_page = fake->page;
		return;
	}

	if (len != 2U && len != 3U) {
		return;
	}

	for (uint8_t i = 0U; i < fake->pages; i++) {
		if (fake->page == PMBUS_PAGE_ALL || fake->page == i) {
			fake->regs[i][cmd] = (len == 2U) ? wr[1] : sys_get_le16(&wr[1]);
		}
	}
}

static uint16_t fake_pmbus_dev_read(struct fake_pmbus_dev *fake, uint8_t cmd)
{
	uint8_t page = (fake->page < fake->pages) ? fake->page : 0U;

	if (atomic_test_bit(fake->cml_only, cmd)) {
		fake->cml |= PMBUS_STATUS_CML_INVALID_CMD;
		return 0xFFFF;
	}

	switch (cmd) {
	case PMBUS_PAGE:
		return fake->page;
	case PMBUS_STATUS_BYTE:
		return fake->regs[page][cmd] | ((fake->cml != 0U) ? PMBUS_STATUS_BYTE_CML : 0U);
	case PMBUS_STATUS_CML:
		return fake->cml;
	default:
		return fake->regs[page][cmd];
	}
}

static int fake_pmbus_dev_write_requested(struct i2c_target_config *cfg)
{
	struct fake_pmbus_dev *fake = TO_FAKE(cfg);

	if (fake->absent) {
		return -EIO;
	}

	fake->len = 0U;
	fake->reading = false;

	return 0;
}

static int fake_pmbus_dev_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	struct fake_pmbus_dev *fake = TO_FAKE(cfg);

	if (fake->len == 0U && atomic_test_bit(fake->nack, val)) {
		fake->cml |= PMBUS_STATUS_CML_INVALID_CMD;
		return -EIO;
	}

	if (fake->len == sizeof(fake->buf)) {
		return -EIO;
	}

	fake->buf[fake->len++] = val;

	return 0;
}

static int fake_pmbus_dev_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	struct fake_pmbus_dev *fake = TO_FAKE(cfg);

	if (fake->rd_pos < fake->rd_len) {
		*val = fake->rd[fake->rd_pos];
		fake->crc = crc8_ccitt(fake->crc, val, 1);
	} else if (fake->rd_pos == fake->rd_len && fake->pec) {
		*val = fake->crc;
	} else {
		*val = 0xFF;
	}

	fake->rd_pos++;

	return 0;
}

static int fake_pmbus_dev_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	struct fake_pmbus_dev *fake = TO_FAKE(cfg);
	uint8_t cmd = fake->buf[0];

	if (fake->absent || fake->len == 0U) {
		return -EIO;
	}

	fake->reading = true;
	sys_put_le16(fake_pmbus_dev_read(fake, cmd), fake->rd);
	fake->rd_len = byte_cmd(cmd) ? 1U : 2U;
	fake->rd_pos = 0U;
	fake->crc = crc8_ccitt(addr_crc(fake, 0, false), fake->buf, fake->len);
	fake->crc = addr_crc(fake, fake->crc, true);

	return fake_pmbus_dev_read_processed(cfg, val);
}

static int fake_pmbus_dev_stop(struct i2c_target_config *cfg)
{
	struct fake_pmbus_dev *fake = TO_FAKE(cfg);
	size_t len = fake->len;

	if (fake->reading || len == 0U) {
		return 0;
	}

	if (fake->pec) {
		len--;
		if (crc8_ccitt(addr_crc(fake, 0, false), fake->buf, len) != fake->buf[len]) {
			fake->cml |= PMBUS_STATUS_CML_PEC_FAILED;
			return 0;
		}
	}

	fake_pmbus_dev_write(fake, fake->buf, len);

	return 0;
}

static const struct i2c_target_callbacks fake_pmbus_dev_callbacks = {
	.write_requested = fake_pmbus_dev_write_requested,
	.write_received = fake_pmbus_dev_write_received,
	.read_requested = fake_pmbus_dev_read_requested,
	.read_processed = fake_pmbus_dev_read_processed,
	.stop = fake_pmbus_dev_stop,
};

int fake_pmbus_dev_register(struct fake_pmbus_dev *fake, const struct device *bus, uint16_t addr,
			    uint8_t pages, uint8_t vout_mode)
{
	if (pages > FAKE_PMBUS_DEV_PAGES) {
		return -EINVAL;
	}

	fake->cfg.address = addr;
	fake->cfg.callbacks = &fake_pmbus_dev_callbacks;
	fake->pages = pages;
	fake->vout_mode = vout_mode;
	fake_pmbus_dev_reset(fake);

	return i2c_target_register(bus, &fake->cfg);
}

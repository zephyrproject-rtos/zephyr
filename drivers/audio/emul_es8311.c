/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everest_es8311

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

#include "emul_es8311.h"

LOG_MODULE_REGISTER(emul_es8311, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define ES8311_EMUL_WLOG_LEN 64

#define ES8311_REG_RESET    0x00U
#define ES8311_RESET_BITS   0x1FU /* the digital/CMG/master/ADC/DAC resets */
#define ES8311_REG_CHIP_ID1 0xFDU
#define ES8311_REG_CHIP_ID2 0xFEU
#define ES8311_CHIP_ID1     0x83U
#define ES8311_CHIP_ID2     0x11U

struct es8311_emul_data {
	uint8_t regs[256];
	/* Fault injection: when > 0, the next N transfers return -EIO. */
	int fail_remaining;
	/* Zero-based transfer index; negative disables this one-shot fault. */
	int fail_at;
	/* Fail this transfer and all subsequent transfers; negative disables the fault. */
	int fail_from;
	/* Apply writes to this register before returning -EIO; negative disables the fault. */
	int fail_write_landed;
	/* Reject reads while allowing writes. */
	bool fail_reads;
	/* Reject writes to this register; negative disables the fault. */
	int fail_write_reg;
	/* Record parsed writes, including failures selected by register. */
	uint8_t wlog[ES8311_EMUL_WLOG_LEN];
	uint8_t wval[ES8311_EMUL_WLOG_LEN];
	int wcount;
	/* Pause the calling thread before a selected write while retaining its driver locks. */
	struct k_sem reached;
	struct k_sem release;
	uint8_t pause_reg;
	bool pause_armed;
};

static int es8311_emul_init(const struct emul *target, const struct device *parent);

void emul_es8311_fail_reads(const struct emul *target, bool fail)
{
	struct es8311_emul_data *data = target->data;

	data->fail_reads = fail;
}

void emul_es8311_fail_write_to(const struct emul *target, int reg)
{
	struct es8311_emul_data *data = target->data;

	data->fail_write_reg = reg;
}

/* The value written at write-log index `idx`, or -1 if there is no such write. */
int emul_es8311_wval_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	if (idx < 0 || idx >= data->wcount || idx >= ES8311_EMUL_WLOG_LEN) {
		return -1;
	}

	return (int)data->wval[idx];
}

void emul_es8311_set_fail(const struct emul *target, int n)
{
	struct es8311_emul_data *data = target->data;

	data->fail_remaining = n;
}

void emul_es8311_fail_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	data->fail_at = idx;
}

void emul_es8311_fail_from(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	data->fail_from = idx;
}

void emul_es8311_fail_write_landed(const struct emul *target, int reg)
{
	struct es8311_emul_data *data = target->data;

	data->fail_write_landed = reg;
}

void emul_es8311_reset_log(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	data->wcount = 0;
}

int emul_es8311_write_count(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	return data->wcount;
}

int emul_es8311_write_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	if (idx < 0 || idx >= data->wcount || idx >= ES8311_EMUL_WLOG_LEN) {
		return -1;
	}
	return data->wlog[idx];
}

void emul_es8311_pause_before(const struct emul *target, uint8_t reg)
{
	struct es8311_emul_data *data = target->data;

	k_sem_init(&data->reached, 0, 1);
	k_sem_init(&data->release, 0, 1);
	data->pause_reg = reg;
	data->pause_armed = true;
}

/* Block until a caller has actually parked on the armed register. */
int emul_es8311_wait_paused(const struct emul *target, k_timeout_t timeout)
{
	struct es8311_emul_data *data = target->data;

	return k_sem_take(&data->reached, timeout);
}

/* Let the parked caller finish its write, and disarm the hook. */
void emul_es8311_release(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	data->pause_armed = false;
	k_sem_give(&data->release);
}

void emul_es8311_set_chip_id(const struct emul *target, uint8_t id1, uint8_t id2)
{
	struct es8311_emul_data *data = target->data;

	data->regs[ES8311_REG_CHIP_ID1] = id1;
	data->regs[ES8311_REG_CHIP_ID2] = id2;
}

static int es8311_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct es8311_emul_data *data = target->data;

	ARG_UNUSED(addr);
	__ASSERT_NO_MSG(msgs && num_msgs);

	if (data->fail_remaining > 0) {
		data->fail_remaining--;
		return -EIO;
	}

	if (data->fail_at >= 0) {
		if (data->fail_at == 0) {
			data->fail_at = -1;
			return -EIO;
		}
		data->fail_at--;
	}

	if (data->fail_from >= 0) {
		if (data->fail_from == 0) {
			return -EIO; /* Keep the fault armed for subsequent transfers. */
		}
		data->fail_from--;
	}

	if (num_msgs == 1) {
		/* Write transaction: buf = [reg, value]; only len 2 is valid. */
		struct i2c_msg *m = &msgs[0];

		if ((m->flags & I2C_MSG_READ) || m->len != 2) {
			return -EIO;
		}

		/* Pause in the caller's context so tests can observe its lock ownership. */
		if (data->pause_armed && m->buf[0] == data->pause_reg) {
			k_sem_give(&data->reached);
			(void)k_sem_take(&data->release, K_FOREVER);
		}

		if (data->wcount < ES8311_EMUL_WLOG_LEN) {
			data->wlog[data->wcount] = m->buf[0];
			data->wval[data->wcount] = m->buf[1];
		}
		data->wcount++;

		/* Log the attempted write even when the register-specific fault rejects it. */
		if (data->fail_write_reg >= 0 && m->buf[0] == (uint8_t)data->fail_write_reg) {
			LOG_DBG("W reg=0x%02x FAILED (injected)", m->buf[0]);
			return -EIO;
		}

		/* RST_DIG and INI_REG side effects are not modeled. */
		data->regs[m->buf[0]] = m->buf[1];
		LOG_DBG("W reg=0x%02x val=0x%02x", m->buf[0], m->buf[1]);

		/* The write took effect, but its completion reports failure. */
		if (data->fail_write_landed >= 0 && m->buf[0] == (uint8_t)data->fail_write_landed) {
			LOG_DBG("W reg=0x%02x LANDED then FAILED (injected)", m->buf[0]);
			return -EIO;
		}
		return 0;
	}

	if (num_msgs == 2) {
		/* Write reg address, then read N bytes. */
		struct i2c_msg *w = &msgs[0];
		struct i2c_msg *r = &msgs[1];
		uint8_t reg;

		if ((w->flags & I2C_MSG_READ) || w->len != 1 || !(r->flags & I2C_MSG_READ)) {
			return -EIO;
		}
		if (data->fail_reads) {
			return -EIO;
		}

		reg = w->buf[0];
		for (uint32_t i = 0; i < r->len; i++) {
			r->buf[i] = data->regs[(uint8_t)(reg + i)];
		}
		return 0;
	}

	return -EIO;
}

static const struct i2c_emul_api es8311_emul_api = {
	.transfer = es8311_emul_transfer,
};

/* Reset emulator state only; driver properties are reset through the codec API. */
void emul_es8311_reset(const struct emul *target)
{
	(void)es8311_emul_init(target, NULL);
}

static int es8311_emul_init(const struct emul *target, const struct device *parent)
{
	struct es8311_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->regs, 0, sizeof(data->regs));
	data->fail_remaining = 0;
	/* Transfer zero and register zero are valid fault targets. */
	data->fail_at = -1;
	data->fail_from = -1;
	data->fail_write_reg = -1;
	data->fail_write_landed = -1;
	data->fail_reads = false;
	data->pause_armed = false;
	data->wcount = 0;
	memset(data->wval, 0, sizeof(data->wval));

	/* Chip identity registers. */
	data->regs[ES8311_REG_CHIP_ID1] = ES8311_CHIP_ID1;
	data->regs[ES8311_REG_CHIP_ID2] = ES8311_CHIP_ID2;

	return 0;
}

#define ES8311_EMUL(n)                                                                             \
	static struct es8311_emul_data es8311_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, es8311_emul_init, &es8311_emul_data_##n, NULL, &es8311_emul_api,    \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(ES8311_EMUL)

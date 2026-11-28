/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tlv320aic3104_emul.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

struct tlv320aic3104_emul_cfg {
	bool is_aic3106;
};

static uint8_t s_page;
static uint8_t s_regs[2][128];
static bool s_written[2][128];
static unsigned int s_write_seq[2][128];
static unsigned int s_writes;
static unsigned int s_reserved_writes;

static uint8_t s_pending_reg;
static bool s_pending_reg_valid;

static uint8_t s_fail_page;
static uint8_t s_fail_addr;
static bool s_fail_armed;

void tlv320aic3104_emul_set_val(uint8_t page, uint8_t addr, uint8_t val)
{
	s_regs[page & 1][addr & 0x7F] = val;
}

uint8_t tlv320aic3104_emul_last_val(uint8_t page, uint8_t addr)
{
	return s_regs[page & 1][addr & 0x7F];
}

void tlv320aic3104_emul_fail_write_at(uint8_t page, uint8_t addr)
{
	s_fail_page = page & 1;
	s_fail_addr = addr & 0x7F;
	s_fail_armed = true;
}

unsigned int tlv320aic3104_emul_write_seq(uint8_t page, uint8_t addr)
{
	return s_write_seq[page & 1][addr & 0x7F];
}

bool tlv320aic3104_emul_was_written(uint8_t page, uint8_t addr)
{
	return s_written[page & 1][addr & 0x7F];
}

unsigned int tlv320aic3104_emul_reserved_writes(void)
{
	return s_reserved_writes;
}

static bool aic3104_page0_reserved(uint8_t reg)
{
	switch (reg) {
	case 20:
	case 23:
	case 39:
	case 45:
	case 48:
	case 52:
	case 55:
	case 59:
	case 62:
	case 66:
	case 69:
	case 83:
	case 87:
	case 90:
		return true;
	default:
		return (reg >= 73 && reg <= 80) || (reg >= 98 && reg <= 100) || reg >= 110;
	}
}

void tlv320aic3104_emul_reset(void)
{
	memset(s_regs, 0, sizeof(s_regs));
	memset(s_written, 0, sizeof(s_written));
	memset(s_write_seq, 0, sizeof(s_write_seq));
	s_writes = 0;
	s_reserved_writes = 0;
	s_page = 0;
	s_pending_reg = 0;
	s_pending_reg_valid = false;
	s_fail_armed = false;
}

static int tlv320aic3104_emul_transfer(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	const struct tlv320aic3104_emul_cfg *cfg = target->cfg;

	ARG_UNUSED(addr);

	for (int i = 0; i < num_msgs; i++) {
		struct i2c_msg *m = &msgs[i];

		if (m->flags & I2C_MSG_READ) {

			if (m->len >= 1) {
				m->buf[0] = s_regs[s_page][s_pending_reg & 0x7F];
			}
			s_pending_reg_valid = false;
			continue;
		}

		if (m->len == 0) {
			continue;
		}

		uint8_t reg = m->buf[0];

		if (m->len == 1) {

			s_pending_reg = reg;
			s_pending_reg_valid = true;

			continue;
		}

		s_pending_reg_valid = false;
		uint8_t val = m->buf[1];

		if (reg == 0x00) {
			s_page = val & 1;
			continue;
		}
		if (s_fail_armed && s_page == s_fail_page && (reg & 0x7F) == s_fail_addr) {
			return -EIO;
		}
		if (!cfg->is_aic3106 && s_page == 0 && aic3104_page0_reserved(reg & 0x7F)) {
			s_reserved_writes++;
		}
		s_regs[s_page][reg & 0x7F] = val;
		s_written[s_page][reg & 0x7F] = true;
		s_write_seq[s_page][reg & 0x7F] = ++s_writes;
	}
	return 0;
}

static const struct i2c_emul_api tlv320aic3104_emul_api = {.transfer = tlv320aic3104_emul_transfer};

static int tlv320aic3104_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);
	return 0;
}

#define TLV320AIC3104_EMUL(n, part, aic3106)                                                       \
	static const struct tlv320aic3104_emul_cfg tlv320aic3104_emul_cfg_##part##_##n = {         \
		.is_aic3106 = aic3106,                                                             \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, tlv320aic3104_emul_init, NULL,                                      \
			    &tlv320aic3104_emul_cfg_##part##_##n, &tlv320aic3104_emul_api, NULL);

#define DT_DRV_COMPAT ti_tlv320aic3104
DT_INST_FOREACH_STATUS_OKAY_VARGS(TLV320AIC3104_EMUL, aic3104, false)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT ti_tlv320aic3106
DT_INST_FOREACH_STATUS_OKAY_VARGS(TLV320AIC3104_EMUL, aic3106, true)
#undef DT_DRV_COMPAT

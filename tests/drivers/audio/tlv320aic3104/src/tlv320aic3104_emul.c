/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
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

#define DT_DRV_COMPAT ti_tlv320aic3104

static uint8_t s_page;
static uint8_t s_regs[2][128];

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

void tlv320aic3104_emul_reset(void)
{
	memset(s_regs, 0, sizeof(s_regs));
	s_page = 0;
	s_pending_reg = 0;
	s_pending_reg_valid = false;
	s_fail_armed = false;
}

static int tlv320aic3104_emul_transfer(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	ARG_UNUSED(target);
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
		s_regs[s_page][reg & 0x7F] = val;
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

#define TLV320AIC3104_EMUL(n)                                                                     \
	EMUL_DT_INST_DEFINE(n, tlv320aic3104_emul_init, NULL, NULL, &tlv320aic3104_emul_api, NULL);
DT_INST_FOREACH_STATUS_OKAY(TLV320AIC3104_EMUL)

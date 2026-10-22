/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/math_extras.h>
#include "dlink_ldo_reg.h"

#define DT_DRV_COMPAT realtek_rts5817_ocotp

LOG_MODULE_REGISTER(rts_otp, CONFIG_OTP_LOG_LEVEL);

#define R_OTP_CFG_STATUS 0x6A0
#define OTP_CFG_BUSY     BIT(0)

#define OTP_POWER_ON_TIMEOUT_US 200

#define RTS_OTP_LDO_VDD 0x3

#define OTP_WORD_SIZE sizeof(uint32_t)

union otp_buffer {
	uint32_t word;
	uint8_t bytes[4];
};

struct rts_otp_config {
	mem_addr_t otp_base;
	const struct device *syscon_ldo;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct reset_dt_spec reset_spec;
	uint32_t size;
};

static bool rts_otp_range_is_valid(const struct rts_otp_config *config, off_t offset, size_t len)
{
	size_t end;

	if (size_add_overflow(offset, len, &end) || end > config->size) {
		LOG_ERR("Out of range");
		return false;
	}

	return true;
}

static int rts_otp_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct rts_otp_config *config = dev->config;
	union otp_buffer buffer_temp;
	uint8_t *rd_data = data;
	off_t offset_align = offset;
	off_t remainder = 0;
	size_t copied_num = 0;

	if (!rts_otp_range_is_valid(config, offset, len)) {
		return -EINVAL;
	}

	remainder = offset & (OTP_WORD_SIZE - 1);
	if (remainder != 0) {
		/* Handle the start unaligned to 4 bytes */
		offset_align = offset - remainder;
		buffer_temp.word = sys_read32(config->otp_base + offset_align);
		if ((OTP_WORD_SIZE - remainder) > len) {
			memcpy(rd_data, &buffer_temp.bytes[remainder], len);
			return 0;
		}
		memcpy(rd_data, &buffer_temp.bytes[remainder], OTP_WORD_SIZE - remainder);
		copied_num += OTP_WORD_SIZE - remainder;
		offset_align += OTP_WORD_SIZE;
	}

	while (copied_num < len) {
		if ((len - copied_num) >= OTP_WORD_SIZE) {
			buffer_temp.word = sys_read32(config->otp_base + offset_align);
			memcpy(rd_data + copied_num, buffer_temp.bytes, OTP_WORD_SIZE);
			offset_align += OTP_WORD_SIZE;
			copied_num += OTP_WORD_SIZE;
		} else {
			/* Handle the end unaligned to 4 bytes */
			buffer_temp.word = sys_read32(config->otp_base + offset_align);
			memcpy(rd_data + copied_num, buffer_temp.bytes, len - copied_num);
			copied_num = len;
		}
	}

	return 0;
}

#if defined(CONFIG_OTP_PROGRAM)
/*
 * OTP can only be programmed from 0 to 1, programming 0 to OTP has no effect.
 * For OTP region that have already been programmed, the OTP module has an internal protection
 * mechanism that any program operations will be ignored.
 */
static int rts_otp_program(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct rts_otp_config *config = dev->config;
	union otp_buffer buffer_temp;
	const uint8_t *wr_data = data;
	off_t offset_align = offset;
	off_t remainder = 0;
	size_t copied_num = 0;

	if (!rts_otp_range_is_valid(config, offset, len)) {
		return -EINVAL;
	}

	remainder = offset & (OTP_WORD_SIZE - 1);
	if (remainder != 0) {
		/* Handle the start unaligned to 4 bytes */
		memset(buffer_temp.bytes, 0, OTP_WORD_SIZE);
		offset_align = offset - remainder;
		if ((OTP_WORD_SIZE - remainder) > len) {
			memcpy(&buffer_temp.bytes[remainder], wr_data, len);
			sys_write32(buffer_temp.word, config->otp_base + offset_align);
			return 0;
		}
		memcpy(&buffer_temp.bytes[remainder], wr_data, OTP_WORD_SIZE - remainder);
		sys_write32(buffer_temp.word, config->otp_base + offset_align);
		copied_num += OTP_WORD_SIZE - remainder;
		offset_align += OTP_WORD_SIZE;
	}

	while (copied_num < len) {
		if ((len - copied_num) >= OTP_WORD_SIZE) {
			memcpy(buffer_temp.bytes, wr_data + copied_num, OTP_WORD_SIZE);
			sys_write32(buffer_temp.word, config->otp_base + offset_align);
			offset_align += OTP_WORD_SIZE;
			copied_num += OTP_WORD_SIZE;
		} else {
			/* Handle the end unaligned to 4 bytes */
			memset(buffer_temp.bytes, 0, OTP_WORD_SIZE);
			memcpy(buffer_temp.bytes, wr_data + copied_num, len - copied_num);
			sys_write32(buffer_temp.word, config->otp_base + offset_align);
			copied_num = len;
		}
	}

	return 0;
}
#endif /* CONFIG_OTP_PROGRAM */

static DEVICE_API(otp, rts_otp_api) = {
	.read = rts_otp_read,
#if defined(CONFIG_OTP_PROGRAM)
	.program = rts_otp_program,
#endif
};

static int rts_otp_init(const struct device *dev)
{
	const struct rts_otp_config *cfg = dev->config;
	int ret;

	ret = reset_line_assert(cfg->reset_spec.dev, cfg->reset_spec.id);
	if (ret < 0) {
		return ret;
	}
	k_busy_wait(2);

	ret = syscon_update_bits(cfg->syscon_ldo, R_LDO_TOP_PD, OTP_PD_RES_MASK, 0);
	if (ret < 0) {
		return ret;
	}
	ret = syscon_update_bits(cfg->syscon_ldo, R_LDO_TOP_VO, REG_TUNE_VO_LED_MASK,
				 RTS_OTP_LDO_VDD << REG_TUNE_VO_LED_OFFSET);
	if (ret < 0) {
		return ret;
	}
	ret = syscon_update_bits(cfg->syscon_ldo, R_LDO_TOP_POW, POW_PWD_PUFF_MASK | POW_LED_MASK,
				 POW_PWD_PUFF_MASK | POW_LED_MASK);
	if (ret < 0) {
		return ret;
	}
	k_busy_wait(100);

	ret = syscon_update_bits(cfg->syscon_ldo, R_LDO_TOP_POW, PUFF_ISO_MASK, PUFF_ISO_MASK);
	if (ret < 0) {
		return ret;
	}
	k_busy_wait(10);

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0) {
		return ret;
	}
	ret = reset_line_deassert(cfg->reset_spec.dev, cfg->reset_spec.id);
	if (ret < 0) {
		return ret;
	}

	if (!WAIT_FOR((sys_read32(cfg->otp_base + R_OTP_CFG_STATUS) & OTP_CFG_BUSY) == 0,
		      OTP_POWER_ON_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

static const struct rts_otp_config otp_config = {
	.otp_base = DT_INST_REG_ADDR(0),
	.syscon_ldo = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon_ldo)),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(0, clocks, clkid),
	.reset_spec = RESET_DT_SPEC_INST_GET(0),
	.size = DT_INST_PROP(0, otp_size),
};

DEVICE_DT_INST_DEFINE(0, rts_otp_init, NULL, NULL, &otp_config, POST_KERNEL,
		      CONFIG_OTP_INIT_PRIORITY, &rts_otp_api);

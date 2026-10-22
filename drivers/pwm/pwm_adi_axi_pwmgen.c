/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Analog Devices AXI PWM Generator core.
 * Based on the no-OS reference driver by Analog Devices.
 */

#define DT_DRV_COMPAT adi_axi_pwmgen

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_axi_pwmgen, CONFIG_PWM_LOG_LEVEL);

#define AXI_PWMGEN_REG_VERSION    0x00
#define AXI_PWMGEN_REG_ID         0x04
#define AXI_PWMGEN_REG_SCRATCHPAD 0x08
#define AXI_PWMGEN_REG_CORE_MAGIC 0x0C
#define AXI_PWMGEN_REG_CONFIG     0x10
#define AXI_PWMGEN_REG_NPWM       0x14

#define AXI_PWMGEN_CONFIG_RESET BIT(0)
#define AXI_PWMGEN_CONFIG_LOAD  BIT(1)

#define AXI_PWMGEN_SCRATCH_TEST 0x5A0F0081

/* Per-channel register layout differs between v1 and v2+ hardware (no-OS axi_pwm.c) */
#define AXI_PWMGEN_V1_CH_STRIDE   12
#define AXI_PWMGEN_V1_PERIOD_BASE 0x40
#define AXI_PWMGEN_V1_DUTY_BASE   0x44
#define AXI_PWMGEN_V1_PHASE_BASE  0x48

#define AXI_PWMGEN_V2_CH_STRIDE   4
#define AXI_PWMGEN_V2_PERIOD_BASE 0x40
#define AXI_PWMGEN_V2_DUTY_BASE   0x80
#define AXI_PWMGEN_V2_PHASE_BASE  0xC0

#define AXI_PWMGEN_HW_MAJOR_VER_V2 2

struct pwmgen_config {
	DEVICE_MMIO_ROM;
	uint32_t ref_clock_hz;
};

struct pwmgen_data {
	DEVICE_MMIO_RAM;
	uint32_t hw_major_ver;
	uint32_t npwm;
};

static inline uint32_t pwmgen_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void pwmgen_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static uint32_t ch_period_reg(const struct pwmgen_data *data, uint32_t ch)
{
	if (data->hw_major_ver >= AXI_PWMGEN_HW_MAJOR_VER_V2) {
		return AXI_PWMGEN_V2_PERIOD_BASE + AXI_PWMGEN_V2_CH_STRIDE * ch;
	}
	return AXI_PWMGEN_V1_PERIOD_BASE + AXI_PWMGEN_V1_CH_STRIDE * ch;
}

static uint32_t ch_duty_reg(const struct pwmgen_data *data, uint32_t ch)
{
	if (data->hw_major_ver >= AXI_PWMGEN_HW_MAJOR_VER_V2) {
		return AXI_PWMGEN_V2_DUTY_BASE + AXI_PWMGEN_V2_CH_STRIDE * ch;
	}
	return AXI_PWMGEN_V1_DUTY_BASE + AXI_PWMGEN_V1_CH_STRIDE * ch;
}

static uint32_t ch_phase_reg(const struct pwmgen_data *data, uint32_t ch)
{
	if (data->hw_major_ver >= AXI_PWMGEN_HW_MAJOR_VER_V2) {
		return AXI_PWMGEN_V2_PHASE_BASE + AXI_PWMGEN_V2_CH_STRIDE * ch;
	}
	return AXI_PWMGEN_V1_PHASE_BASE + AXI_PWMGEN_V1_CH_STRIDE * ch;
}

static int pwmgen_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cycles,
			     uint32_t pulse_cycles, pwm_flags_t flags)
{
	struct pwmgen_data *data = dev->data;

	if (flags != 0) {
		LOG_ERR("PWM flags 0x%x not supported", flags);
		return -ENOTSUP;
	}

	if (channel >= data->npwm) {
		LOG_ERR("invalid channel %u (%u channels)", channel, data->npwm);
		return -EINVAL;
	}

	/*
	 * A zero pulse width means "output stays low" — disable the channel
	 * by zeroing its period and committing so the core stops toggling.
	 */
	if (pulse_cycles == 0U) {
		pwmgen_write(dev, ch_period_reg(data, channel), 0);
		pwmgen_write(dev, AXI_PWMGEN_REG_CONFIG, AXI_PWMGEN_CONFIG_LOAD);
		return 0;
	}

	pwmgen_write(dev, ch_period_reg(data, channel), period_cycles);
	pwmgen_write(dev, ch_duty_reg(data, channel), pulse_cycles);
	/* AD463x drives a single CNV train; phase alignment is not used. */
	pwmgen_write(dev, ch_phase_reg(data, channel), 0);

	/* LOAD is global and commits every channel's shadow registers. */
	pwmgen_write(dev, AXI_PWMGEN_REG_CONFIG, AXI_PWMGEN_CONFIG_LOAD);
	return 0;
}

static int pwmgen_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	const struct pwmgen_config *cfg = dev->config;
	struct pwmgen_data *data = dev->data;

	if (cycles == NULL) {
		LOG_ERR("cycles pointer is NULL");
		return -EINVAL;
	}

	if (channel >= data->npwm) {
		LOG_ERR("invalid channel %u (%u channels)", channel, data->npwm);
		return -EINVAL;
	}

	*cycles = cfg->ref_clock_hz;
	return 0;
}

static DEVICE_API(pwm, pwmgen_api) = {
	.set_cycles = pwmgen_set_cycles,
	.get_cycles_per_sec = pwmgen_get_cycles_per_sec,
};

static int pwmgen_init(const struct device *dev)
{
	const struct pwmgen_config *cfg = dev->config;
	struct pwmgen_data *data = dev->data;
	uint32_t version;
	uint32_t scratch;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	version = pwmgen_read(dev, AXI_PWMGEN_REG_VERSION);
	data->hw_major_ver = version >> 16;
	data->npwm = pwmgen_read(dev, AXI_PWMGEN_REG_NPWM);

	if (data->npwm == 0) {
		LOG_ERR("no PWM channels detected");
		return -EIO;
	}

	pwmgen_write(dev, AXI_PWMGEN_REG_CONFIG, 0);

	pwmgen_write(dev, AXI_PWMGEN_REG_SCRATCHPAD, AXI_PWMGEN_SCRATCH_TEST);
	scratch = pwmgen_read(dev, AXI_PWMGEN_REG_SCRATCHPAD);
	if (scratch != AXI_PWMGEN_SCRATCH_TEST) {
		LOG_ERR("scratchpad mismatch: wrote 0x%08x, read 0x%08x", AXI_PWMGEN_SCRATCH_TEST,
			scratch);
		return -EIO;
	}

	LOG_INF("AXI PWMGEN v%d.%d.%c — %u channel(s), ref_clk=%u Hz", version >> 16,
		(version >> 8) & 0xff, version & 0xff, data->npwm, cfg->ref_clock_hz);

	return 0;
}

#define PWMGEN_INIT(n)                                                                             \
	static struct pwmgen_data pwmgen_data_##n;                                                 \
	static const struct pwmgen_config pwmgen_config_##n = {                                    \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.ref_clock_hz = DT_INST_PROP(n, adi_ref_clock_hz),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, pwmgen_init, NULL, &pwmgen_data_##n, &pwmgen_config_##n,          \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwmgen_api);

DT_INST_FOREACH_STATUS_OKAY(PWMGEN_INIT)

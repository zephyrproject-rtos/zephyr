/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_pwm

#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/it51xxx_pwm.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_ite_it51xxx, CONFIG_PWM_LOG_LEVEL);

#define PWM_CTX_MIN     100
#define PWM_FREQ        IT51XXX_EC_FREQ
#define PWM_CH_SPS_MASK GENMASK(1, 0)

/* 0x00/0x10/0x20/0x30/0x40/0x50/0x60/0x70: PWM channel 0~7 duty cycle low byte */
#define REG_PWM_CH_DC_L  0x00
/* 0x01/0x11/0x21/0x31/0x41/0x51/0x61/0x71: PWM channel 0~7 duty cycle high byte */
#define REG_PWM_CH_DC_H  0x01
/* 0x04/0x14/0x24/0x34/0x44/0x54/0x64/0x74: PWM channel 0~7 control 0 */
#define REG_PWM_CH_CTRL0 0x04
#define PWM_CH_PWMODEN   BIT(2)
#define PWM_CH_PCSG      BIT(1)
#define PWM_CH_INVP      BIT(0)
/* 0x05/0x15/0x25/0x35/0x45/0x55/0x65/0x75: PWM channel 0~7 select prescaler source */
#define REG_PWM_CH_SPS   0x05

/* 0x84/0x88/0x8C: PWM prescaler 4/6/7 clock low byte */
#define REG_PWM_PXC_L(prs_sel)   (0x04 * (prs_sel))
/* 0x85/0x89/0x8D: PWM prescaler 4/6/7 clock high byte */
#define REG_PWM_PXC_H(prs_sel)   (0x04 * (prs_sel) + 0x01)
/* 0x86/0x8A/0x8E: PWM prescaler 4/6/7 clock source select low byte */
#define REG_PWM_PXCSS_L(prs_sel) (0x04 * (prs_sel) + 0x02)
#define PWM_PCFS_EC              BIT(0)
/* 0xA4/0xA8/0xAC: PWM cycle timer 1/2/3 low byte */
#define REG_PWM_CTX_L(prs_sel)   (0x20 + 0x04 * (prs_sel))
/* 0xA5/0xA9/0xAD: PWM cycle timer 1/2/3 high byte */
#define REG_PWM_CTX_H(prs_sel)   (0x20 + 0x04 * (prs_sel) + 0x01)
/* 0xF0: PWM global control */
#define REG_PWM_GCTRL            0x70
#define PWM_PCCE                 BIT(1)

struct pwm_it51xxx_cfg {
	/* PWM channel register base address */
	uintptr_t base_ch;
	/* PWM prescaler register base address */
	uintptr_t base_prs;
	/* Select PWM prescaler that output to PWM channel */
	int prs_sel;
	/* PWM alternate configuration */
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_it51xxx_data {
	uint32_t ctx;
	uint32_t pxc;
	uint32_t target_freq_prev;
};

static void pwm_enable(const struct device *dev, int enabled)
{
	const struct pwm_it51xxx_cfg *config = dev->config;
	const uintptr_t base_ch = config->base_ch;
	uint8_t reg_val;

	if (enabled) {
		/* PWM channel clock source not gating */
		reg_val = sys_read8(base_ch + REG_PWM_CH_CTRL0);
		sys_write8(reg_val & ~PWM_CH_PCSG, base_ch + REG_PWM_CH_CTRL0);
	} else {
		/* PWM channel clock source gating */
		reg_val = sys_read8(base_ch + REG_PWM_CH_CTRL0);
		sys_write8(reg_val | PWM_CH_PCSG, base_ch + REG_PWM_CH_CTRL0);
	}
}

static int pwm_it51xxx_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					  uint64_t *cycles)
{
	ARG_UNUSED(channel);

	/*
	 * Path of pwm_it51xxx_set_cycles() called from pwm api:
	 * 1) pwm_set() -> pwm_set_cycles_cycles() -> pwm_it51xxx_set_cycles()
	 *    target_freq = pwm_clk_src / period_cycles
	 *                = cycles / (period * cycles / NSEC_PER_SEC)
	 *                = NSEC_PER_SEC / period
	 * 2) pwm_set_cycles() -> pwm_it51xxx_set_cycles()
	 *    target_freq = pwm_clk_src / period_cycles
	 *                = cycles / period
	 *
	 * When pwm needs output in EC power saving mode, we switch the prescaler
	 * clock source (cycles) from 9.2MHz to 32.768kHz. Whether in power saving
	 * mode or not, we need to get the same target_freq on above two cases,
	 * so here always return PWM_FREQ.
	 */
	*cycles = (uint64_t)PWM_FREQ;

	return 0;
}

static int pwm_it51xxx_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_it51xxx_cfg *config = dev->config;
	const uintptr_t base_ch = config->base_ch;
	const uintptr_t base_prs = config->base_prs;
	struct pwm_it51xxx_data *data = dev->data;
	int prs_sel = config->prs_sel;
	uint32_t actual_freq = 0xffffffff, target_freq, deviation, dc_val;
	uint64_t pwm_clk_src;
	uint8_t reg_val;

	/* Select PWM inverted polarity (ex. active-low pulse) */
	if (flags & PWM_POLARITY_INVERTED) {
		reg_val = sys_read8(base_ch + REG_PWM_CH_CTRL0);
		sys_write8(reg_val | PWM_CH_INVP, base_ch + REG_PWM_CH_CTRL0);
	} else {
		reg_val = sys_read8(base_ch + REG_PWM_CH_CTRL0);
		sys_write8(reg_val & ~PWM_CH_INVP, base_ch + REG_PWM_CH_CTRL0);
	}

	/* Enable PWM output open-drain */
	if (flags & PWM_IT51XXX_OPEN_DRAIN) {
		reg_val = sys_read8(base_ch + REG_PWM_CH_CTRL0);
		sys_write8(reg_val | PWM_CH_PWMODEN, base_ch + REG_PWM_CH_CTRL0);
	}

	/* If pulse cycles is 0, set duty cycle 0 and enable pwm channel */
	if (pulse_cycles == 0) {
		/* DC_H will be valid when the next time write DC_L */
		sys_write8(0, base_ch + REG_PWM_CH_DC_H);
		sys_write8(0, base_ch + REG_PWM_CH_DC_L);
		pwm_enable(dev, 1);
		return 0;
	}

	pwm_it51xxx_get_cycles_per_sec(dev, channel, &pwm_clk_src);
	target_freq = ((uint32_t)pwm_clk_src) / period_cycles;

	/*
	 * Support PWM output frequency:
	 * 1) 9.2MHz clock source: 1Hz <= target_freq <= 91089Hz
	 * 2) 32.768KHz clock source: 1Hz <= target_freq <= 324Hz
	 * NOTE: PWM output signal maximum supported frequency comes from
	 *       [9.2MHz or 32.768KHz] / 1 / (PWM_CTX_MIN + 1).
	 *       PWM output signal minimum supported frequency comes from
	 *       [9.2MHz or 32.768KHz] / 65536 / 1024, the minimum integer is 1.
	 */
	if (target_freq < 1) {
		LOG_ERR("PWM output frequency is < 1 !");
		return -EINVAL;
	}

	deviation = (target_freq / 100) + 1;

	reg_val = sys_read8(base_prs + REG_PWM_PXCSS_L(prs_sel));
	if (target_freq <= 324) {
		/*
		 * Default clock source setting is 9.2MHz. When ITE chip is in power
		 * saving mode, 9.2MHz clock source will be gated (32.768KHz won't).
		 * So if we still need pwm output in mode, then we should set frequency
		 * <=324Hz in board dts. Now change prescaler clock source from 9.2MHz to
		 * 32.768KHz to support pwm output in power saving mode.
		 */
		if (reg_val & PWM_PCFS_EC) {
			sys_write8(reg_val & ~PWM_PCFS_EC, base_prs + REG_PWM_PXCSS_L(prs_sel));
		}
		pwm_clk_src = (uint64_t)32768;
	} else {
		if ((reg_val & PWM_PCFS_EC) == 0) {
			sys_write8(reg_val | PWM_PCFS_EC, base_prs + REG_PWM_PXCSS_L(prs_sel));
		}
	}

	/*
	 * PWM output signal frequency is
	 * pwm_clk_src / ((PxC[15:0] + 1) * (CTx[9:0] + 1))
	 * NOTE: 1) define CT minimum is 100 for more precisely when
	 *          calculate DCR
	 *       2) PxC[15:0] value 0001h results in a divisor 2
	 *          PxC[15:0] value FFFFh results in a divisor 65536
	 *          CTx[9:0] value 00h results in a divisor 1
	 *          CTx[9:0] value FFh results in a divisor 256
	 */
	if (target_freq != data->target_freq_prev) {
		uint32_t ctx, pxc;

		for (ctx = 0x3FF; ctx >= PWM_CTX_MIN; ctx--) {
			pxc = (((uint32_t)pwm_clk_src) / (ctx + 1) / target_freq);
			/*
			 * Make sure pxc isn't zero, or we will have
			 * divide-by-zero on calculating actual_freq.
			 */
			if (pxc != 0) {
				actual_freq = ((uint32_t)pwm_clk_src) / (ctx + 1) / pxc;
				if (abs(actual_freq - target_freq) < deviation) {
					/* PxC[15:0] = pxc - 1 */
					pxc--;
					break;
				}
			}
		}

		if (pxc > UINT16_MAX) {
			LOG_ERR("PWM prescaler PxC only support 2 bytes !");
			return -EINVAL;
		}

		/* Store ctx and pxc with successful frequency change */
		data->ctx = ctx;
		data->pxc = pxc;
	}

	/* Set PWM prescaler clock divide register */
	sys_write8(data->pxc & 0xFF, base_prs + REG_PWM_PXC_L(prs_sel));
	sys_write8((data->pxc >> 8) & 0xFF, base_prs + REG_PWM_PXC_H(prs_sel));

	/*
	 * Set PWM prescaler cycle time register.
	 * CTx must be written high bytes first.
	 */
	sys_write8((data->ctx >> 8) & 0xFF, base_prs + REG_PWM_CTX_H(prs_sel));
	sys_write8(data->ctx & 0xFF, base_prs + REG_PWM_CTX_L(prs_sel));

	/*
	 * Set PWM channel duty cycle register.
	 * DC_H will be valid when the next time write DC_L.
	 */
	dc_val = (data->ctx * pulse_cycles) / period_cycles;
	sys_write8((dc_val >> 8) & 0xFF, base_ch + REG_PWM_CH_DC_H);
	sys_write8(dc_val & 0xFF, base_ch + REG_PWM_CH_DC_L);

	/* PWM channel clock source not gating */
	pwm_enable(dev, 1);

	/* Store the frequency to be compared */
	data->target_freq_prev = target_freq;

	LOG_DBG("clock source freq %d, target freq %d", (uint32_t)pwm_clk_src, target_freq);

	return 0;
}

static int pwm_it51xxx_init(const struct device *dev)
{
	const struct pwm_it51xxx_cfg *config = dev->config;
	const uintptr_t base_ch = config->base_ch;
	const uintptr_t base_prs = config->base_prs;
	int prs_sel = config->prs_sel;
	int status;
	uint8_t reg_val;

	/* PWM channel clock source gating before configuring */
	pwm_enable(dev, 0);

	/* Select clock source from EC 9.2MHz to prescaler */
	reg_val = sys_read8(base_prs + REG_PWM_PXCSS_L(prs_sel));
	sys_write8(reg_val | PWM_PCFS_EC, base_prs + REG_PWM_PXCSS_L(prs_sel));

	/* Clear default value and select prescaler output to PWM channel */
	reg_val = sys_read8(base_ch + REG_PWM_CH_SPS);
	reg_val &= ~PWM_CH_SPS_MASK;
	sys_write8(reg_val | prs_sel, base_ch + REG_PWM_CH_SPS);

	/* Enable PWMs clock counter */
	reg_val = sys_read8(base_prs + REG_PWM_GCTRL);
	sys_write8(reg_val | PWM_PCCE, base_prs + REG_PWM_GCTRL);

	/* Set alternate mode of PWM pin */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure PWM pins");
		return status;
	}

	return 0;
}

static DEVICE_API(pwm, pwm_it51xxx_api) = {
	.set_cycles = pwm_it51xxx_set_cycles,
	.get_cycles_per_sec = pwm_it51xxx_get_cycles_per_sec,
};

/* Device Instance */
#define PWM_IT51XXX_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct pwm_it51xxx_cfg pwm_it51xxx_cfg_##inst = {                             \
		.base_ch = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                       \
		.base_prs = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                      \
		.prs_sel = DT_PROP(DT_INST(inst, ite_it51xxx_pwm), prescaler_cx),                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	static struct pwm_it51xxx_data pwm_it51xxx_data_##inst;                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pwm_it51xxx_init, NULL, &pwm_it51xxx_data_##inst,             \
			      &pwm_it51xxx_cfg_##inst, PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,     \
			      &pwm_it51xxx_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_IT51XXX_INIT)

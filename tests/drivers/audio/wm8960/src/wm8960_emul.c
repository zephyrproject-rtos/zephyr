/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "wm8960_emul.h"

LOG_MODULE_REGISTER(wolfson_wm8960_emul, CONFIG_WM8960_EMUL_LOG_LEVEL);

#define DT_DRV_COMPAT wolfson_wm8960

/* WM8960 Register definitions */
#define WM8960_REG_LEFT_INPUT_VOLUME    0x00
#define WM8960_REG_RIGHT_INPUT_VOLUME   0x01
#define WM8960_REG_LOUT1_VOLUME         0x02
#define WM8960_REG_ROUT1_VOLUME         0x03
#define WM8960_REG_CLOCKING_1           0x04
#define WM8960_REG_ADC_DAC_CTRL_1       0x05
#define WM8960_REG_ADC_DAC_CTRL_2       0x06
#define WM8960_REG_AUDIO_INTERFACE_1    0x07
#define WM8960_REG_CLOCKING_2           0x08
#define WM8960_REG_AUDIO_INTERFACE_2    0x09
#define WM8960_REG_LEFT_DAC_VOLUME      0x0A
#define WM8960_REG_RIGHT_DAC_VOLUME     0x0B
#define WM8960_REG_RESET                0x0F
#define WM8960_REG_3D_CONTROL           0x10
#define WM8960_REG_ALC1                 0x11
#define WM8960_REG_ALC2                 0x12
#define WM8960_REG_ALC3                 0x13
#define WM8960_REG_NOISE_GATE           0x14
#define WM8960_REG_LEFT_ADC_VOLUME      0x15
#define WM8960_REG_RIGHT_ADC_VOLUME     0x16
#define WM8960_REG_ADDITIONAL_CONTROL_1 0x17
#define WM8960_REG_ADDITIONAL_CONTROL_2 0x18
#define WM8960_REG_POWER_MGMT_1         0x19
#define WM8960_REG_POWER_MGMT_2         0x1A
#define WM8960_REG_ADDITIONAL_CONTROL_3 0x1B
#define WM8960_REG_ANTI_POP_1           0x1C
#define WM8960_REG_ANTI_POP_2           0x1D
#define WM8960_REG_ADCL_SIGNAL_PATH     0x20
#define WM8960_REG_ADCR_SIGNAL_PATH     0x21
#define WM8960_REG_LEFT_OUT_MIX_1       0x22
#define WM8960_REG_RIGHT_OUT_MIX_2      0x25
#define WM8960_REG_MONO_OUT_MIX_1       0x26
#define WM8960_REG_MONO_OUT_MIX_2       0x27
#define WM8960_REG_LOUT2_VOLUME         0x28
#define WM8960_REG_ROUT2_VOLUME         0x29
#define WM8960_REG_MONO_OUT_VOLUME      0x2A
#define WM8960_REG_INPUT_BOOST_MIXER_1  0x2B
#define WM8960_REG_INPUT_BOOST_MIXER_2  0x2C
#define WM8960_REG_BYPASS_1             0x2D
#define WM8960_REG_BYPASS_2             0x2E
#define WM8960_REG_POWER_MGMT_3         0x2F
#define WM8960_REG_ADDITIONAL_CONTROL_4 0x30
#define WM8960_REG_CLASS_D_CONTROL_1    0x31
#define WM8960_REG_CLASS_D_CONTROL_3    0x33
#define WM8960_REG_PLL_N                0x34
#define WM8960_REG_PLL_K1               0x35
#define WM8960_REG_PLL_K2               0x36
#define WM8960_REG_PLL_K3               0x37

/* Maximum register address */
#define WM8960_NUM_REGS 0x100

/* Register bit masks */
#define WM8960_REGMASK_OUT_VOL  0x00FF
#define WM8960_REGMASK_OUT_MUTE 0x0100
#define WM8960_REGMASK_OUT_VU   0x0200
#define WM8960_REGMASK_OUT_BOTH 0x0080 /* Update both channels */

/* Audio interface formats */
#define WM8960_AIF_FMT_MASK            0x0003
#define WM8960_AIF_FMT_RIGHT_JUSTIFIED 0x0000
#define WM8960_AIF_FMT_LEFT_JUSTIFIED  0x0001
#define WM8960_AIF_FMT_I2S             0x0002
#define WM8960_AIF_FMT_DSP             0x0003

/* Word lengths */
#define WM8960_AIF_WL_MASK   0x000C
#define WM8960_AIF_WL_16BITS 0x0000
#define WM8960_AIF_WL_20BITS 0x0004
#define WM8960_AIF_WL_24BITS 0x0008
#define WM8960_AIF_WL_32BITS 0x000C

/* Power management bits */
#define WM8960_POWER1_VREF 0x0040
#define WM8960_POWER1_AINL 0x0010
#define WM8960_POWER1_AINR 0x0008
#define WM8960_POWER1_ADCL 0x0004
#define WM8960_POWER1_ADCR 0x0002

#define WM8960_POWER2_DACL  0x0100
#define WM8960_POWER2_DACR  0x0080
#define WM8960_POWER2_LOUT1 0x0040
#define WM8960_POWER2_ROUT1 0x0020
#define WM8960_POWER2_SPKL  0x0010
#define WM8960_POWER2_SPKR  0x0008

#define WM8960_POWER3_LOUT2 0x0080
#define WM8960_POWER3_ROUT2 0x0040
#define WM8960_POWER3_OUT3  0x0020
#define WM8960_POWER3_SPKL  0x0010
#define WM8960_POWER3_SPKR  0x0008

/* Clock dividers */
#define WM8960_CLOCK1_SYSCLK_DIV_MASK 0x0070
#define WM8960_CLOCK1_SYSCLK_DIV_1    0x0000
#define WM8960_CLOCK1_SYSCLK_DIV_2    0x0010
#define WM8960_CLOCK1_SYSCLK_DIV_3    0x0020
#define WM8960_CLOCK1_SYSCLK_DIV_4    0x0030
#define WM8960_CLOCK1_SYSCLK_DIV_5    0x0040
#define WM8960_CLOCK1_SYSCLK_DIV_6    0x0050

#define WM8960_CLOCK1_CLKSEL_MCLK 0x0000
#define WM8960_CLOCK1_CLKSEL_PLL  0x0001

struct wm8960_emul_data {
	uint16_t reg[WM8960_NUM_REGS];
	uint8_t current_reg;
	bool reg_addr_set;
	bool powered_up;
	bool dac_enabled;
	bool adc_enabled;
	bool headphone_enabled;
	bool speaker_enabled;
};

struct wm8960_emul_cfg {
	uint16_t addr;
	const struct device *i2c_dev;
};

static void wm8960_emul_reset_registers(struct wm8960_emul_data *data)
{
	/* Initialize registers with default values */
	memset(data->reg, 0, sizeof(data->reg));

	/* Set default values for key registers */
	data->reg[WM8960_REG_LEFT_INPUT_VOLUME] = 0x0097;    /* 0dB, IPVU=0, LINMUTE=0, LIZC=0 */
	data->reg[WM8960_REG_RIGHT_INPUT_VOLUME] = 0x0097;   /* 0dB, IPVU=0, RINMUTE=0, RIZC=0 */
	data->reg[WM8960_REG_LOUT1_VOLUME] = 0x0079;         /* 0dB, HPVU=0, LZCEN=0 */
	data->reg[WM8960_REG_ROUT1_VOLUME] = 0x0079;         /* 0dB, HPVU=0, RZCEN=0 */
	data->reg[WM8960_REG_CLOCKING_1] = 0x0000;           /* MCLK input, no divide */
	data->reg[WM8960_REG_ADC_DAC_CTRL_1] = 0x0000;       /* No deemphasis, ADC/DAC normal */
	data->reg[WM8960_REG_ADC_DAC_CTRL_2] = 0x0000;       /* No soft mute, no oversample */
	data->reg[WM8960_REG_AUDIO_INTERFACE_1] = 0x0000;    /* I2S format, 16 bits */
	data->reg[WM8960_REG_CLOCKING_2] = 0x0000;           /* Normal clocking mode */
	data->reg[WM8960_REG_AUDIO_INTERFACE_2] = 0x0000;    /* Normal operation */
	data->reg[WM8960_REG_LEFT_DAC_VOLUME] = 0x00FF;      /* 0dB, DACVU=0 */
	data->reg[WM8960_REG_RIGHT_DAC_VOLUME] = 0x00FF;     /* 0dB, DACVU=0 */
	data->reg[WM8960_REG_3D_CONTROL] = 0x0000;           /* No 3D enhancement */
	data->reg[WM8960_REG_ALC1] = 0x0000;                 /* ALC off */
	data->reg[WM8960_REG_ALC2] = 0x0000;                 /* ALC settings */
	data->reg[WM8960_REG_ALC3] = 0x0000;                 /* ALC settings */
	data->reg[WM8960_REG_NOISE_GATE] = 0x0000;           /* Noise gate off */
	data->reg[WM8960_REG_LEFT_ADC_VOLUME] = 0x00C3;      /* 0dB, ADCVU=0 */
	data->reg[WM8960_REG_RIGHT_ADC_VOLUME] = 0x00C3;     /* 0dB, ADCVU=0 */
	data->reg[WM8960_REG_ADDITIONAL_CONTROL_1] = 0x0000; /* Normal operation */
	data->reg[WM8960_REG_ADDITIONAL_CONTROL_2] =
		0x0000;                              /* VREF to analog output resistance 5k */
	data->reg[WM8960_REG_POWER_MGMT_1] = 0x0000; /* All power off */
	data->reg[WM8960_REG_POWER_MGMT_2] = 0x0000; /* All power off */
	data->reg[WM8960_REG_ADDITIONAL_CONTROL_3] = 0x0000; /* No DC offset */
	data->reg[WM8960_REG_ANTI_POP_1] = 0x0000;           /* Anti-pop off */
	data->reg[WM8960_REG_ANTI_POP_2] = 0x0000;           /* Anti-pop off */
	data->reg[WM8960_REG_ADCL_SIGNAL_PATH] = 0x0000;     /* No input selected */
	data->reg[WM8960_REG_ADCR_SIGNAL_PATH] = 0x0000;     /* No input selected */
	data->reg[WM8960_REG_LEFT_OUT_MIX_1] = 0x0000;       /* No mix inputs */
	data->reg[WM8960_REG_RIGHT_OUT_MIX_2] = 0x0000;      /* No mix inputs */
	data->reg[WM8960_REG_MONO_OUT_MIX_1] = 0x0000;       /* No mix inputs */
	data->reg[WM8960_REG_MONO_OUT_MIX_2] = 0x0000;       /* No mix inputs */
	data->reg[WM8960_REG_LOUT2_VOLUME] = 0x0079;         /* 0dB, SPKVU=0 */
	data->reg[WM8960_REG_ROUT2_VOLUME] = 0x0079;         /* 0dB, SPKVU=0 */
	data->reg[WM8960_REG_MONO_OUT_VOLUME] = 0x0079;      /* 0dB, SPKVU=0 */
	data->reg[WM8960_REG_POWER_MGMT_3] = 0x0000;         /* All power off */
	data->reg[WM8960_REG_CLASS_D_CONTROL_1] = 0x0000;    /* Class D off */
	data->reg[WM8960_REG_PLL_N] = 0x0000;                /* PLL off */
	data->reg[WM8960_REG_PLL_K1] = 0x0000;               /* PLL off */
	data->reg[WM8960_REG_PLL_K2] = 0x0000;               /* PLL off */
	data->reg[WM8960_REG_PLL_K3] = 0x0000;               /* PLL off */

	/* Reset state variables */
	data->current_reg = 0;
	data->reg_addr_set = false;
	data->powered_up = false;
	data->dac_enabled = false;
	data->adc_enabled = false;
	data->headphone_enabled = false;
	data->speaker_enabled = false;
}

uint16_t wm8960_emul_get_reg(const struct emul *target, uint8_t reg_addr)
{
	struct wm8960_emul_data *data = target->data;

	if (reg_addr < WM8960_NUM_REGS) {
		return data->reg[reg_addr];
	}
	return 0;
}

void wm8960_emul_set_reg(const struct emul *target, uint8_t reg_addr, uint16_t val)
{
	struct wm8960_emul_data *data = target->data;

	if (reg_addr < WM8960_NUM_REGS) {
		data->reg[reg_addr] = val;

		/* Special handling for reset register */
		if (reg_addr == WM8960_REG_RESET) {
			LOG_INF("WM8960 emulator: Software reset triggered via direct register "
				"write");
			wm8960_emul_reset_registers(data);
			return;
		}

		/* Update internal state based on register changes */
		switch (reg_addr) {
		case WM8960_REG_POWER_MGMT_1:
			data->adc_enabled = (val & (WM8960_POWER1_ADCL | WM8960_POWER1_ADCR)) != 0;
			break;
		case WM8960_REG_POWER_MGMT_2:
			data->dac_enabled = (val & (WM8960_POWER2_DACL | WM8960_POWER2_DACR)) != 0;
			data->headphone_enabled =
				(val & (WM8960_POWER2_LOUT1 | WM8960_POWER2_ROUT1)) != 0;
			data->speaker_enabled =
				(val & (WM8960_POWER2_SPKL | WM8960_POWER2_SPKR)) != 0;
			data->powered_up = data->dac_enabled || data->adc_enabled ||
					   data->headphone_enabled || data->speaker_enabled;
			break;
		case WM8960_REG_POWER_MGMT_3:
			data->speaker_enabled = data->speaker_enabled ||
				((val & (WM8960_POWER3_LOUT2 | WM8960_POWER3_ROUT2 |
					WM8960_POWER3_SPKL | WM8960_POWER3_SPKR)) != 0);
			data->powered_up = data->dac_enabled || data->adc_enabled ||
					   data->headphone_enabled || data->speaker_enabled;
			break;
		default:
			break;
		}
	}
}

bool wm8960_emul_is_powered_up(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->powered_up;
}

bool wm8960_emul_is_dac_enabled(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->dac_enabled;
}

bool wm8960_emul_is_adc_enabled(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->adc_enabled;
}

bool wm8960_emul_is_headphone_enabled(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->headphone_enabled;
}

bool wm8960_emul_is_speaker_enabled(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->speaker_enabled;
}

static int wm8960_emul_write(const struct emul *target, int reg, int val, int bytes,
			     void *unused_data)
{
	struct wm8960_emul_data *data = target->data;

	ARG_UNUSED(unused_data);

	if (!data->reg_addr_set) {
		/* First byte is register address and MSB of data */
		data->current_reg = (reg >> 1) & 0x7F;
		/* Store MSB of data (bit 8) */
		data->reg[data->current_reg] =
			(data->reg[data->current_reg] & 0x00FF) | ((reg & 0x01) << 8);
		data->reg_addr_set = true;
		LOG_DBG("WM8960 emulator: Set register address 0x%02x, MSB=0x%01x",
			data->current_reg, reg & 0x01);
		return 0;
	}

	/* Second byte is data (bits 0-7) */
	if (data->current_reg < WM8960_NUM_REGS) {
		/* Update lower 8 bits */
		data->reg[data->current_reg] = (data->reg[data->current_reg] & 0x0100) | val;
		LOG_DBG("WM8960 emulator: Write 0x%02x to reg 0x%02x, final value 0x%04x", val,
			data->current_reg, data->reg[data->current_reg]);

		/* Handle special registers that trigger actions */
		switch (data->current_reg) {
		case WM8960_REG_RESET:
			LOG_INF("WM8960 emulator: Software reset triggered");
			wm8960_emul_reset_registers(data);
			break;

		case WM8960_REG_POWER_MGMT_1:
			/* Update ADC power state */
			data->adc_enabled = (data->reg[data->current_reg] &
					     (WM8960_POWER1_ADCL | WM8960_POWER1_ADCR)) != 0;
			LOG_DBG("WM8960 emulator: ADC power state: %s",
				data->adc_enabled ? "ON" : "OFF");
			break;

		case WM8960_REG_POWER_MGMT_2:
			/* Update DAC and output power states */
			data->dac_enabled = (data->reg[data->current_reg] &
					     (WM8960_POWER2_DACL | WM8960_POWER2_DACR)) != 0;
			data->headphone_enabled =
				(data->reg[data->current_reg] &
				 (WM8960_POWER2_LOUT1 | WM8960_POWER2_ROUT1)) != 0;
			data->speaker_enabled = (data->reg[data->current_reg] &
						 (WM8960_POWER2_SPKL | WM8960_POWER2_SPKR)) != 0;
			data->powered_up = data->dac_enabled || data->adc_enabled ||
					   data->headphone_enabled || data->speaker_enabled;
			LOG_DBG("WM8960 emulator: Power states - DAC: %s, Headphone: %s, Speaker: "
				"%s",
				data->dac_enabled ? "ON" : "OFF",
				data->headphone_enabled ? "ON" : "OFF",
				data->speaker_enabled ? "ON" : "OFF");
			break;

		case WM8960_REG_POWER_MGMT_3:
			/* Update additional speaker power state */
			data->speaker_enabled = data->speaker_enabled || (
				(data->reg[data->current_reg] &
					(WM8960_POWER3_LOUT2 | WM8960_POWER3_ROUT2 |
					WM8960_POWER3_SPKL | WM8960_POWER3_SPKR))
				!= 0);
			data->powered_up = data->dac_enabled || data->adc_enabled ||
					   data->headphone_enabled || data->speaker_enabled;
			LOG_DBG("WM8960 emulator: Speaker power state updated: %s",
				data->speaker_enabled ? "ON" : "OFF");
			break;

		case WM8960_REG_AUDIO_INTERFACE_1:
			LOG_DBG("WM8960 emulator: Audio interface format set to 0x%04x",
				data->reg[data->current_reg]);
			break;

		case WM8960_REG_LEFT_DAC_VOLUME:
		case WM8960_REG_RIGHT_DAC_VOLUME:
			LOG_DBG("WM8960 emulator: DAC volume reg 0x%02x set to 0x%04x",
				data->current_reg, data->reg[data->current_reg]);

			/* Handle DACVU bit (update both channels) */
			if ((data->reg[data->current_reg] & WM8960_REGMASK_OUT_VU) &&
			    (data->current_reg == WM8960_REG_LEFT_DAC_VOLUME)) {
				/* Copy left volume to right volume, preserving VU bit */
				data->reg[WM8960_REG_RIGHT_DAC_VOLUME] =
					(data->reg[WM8960_REG_RIGHT_DAC_VOLUME] &
					 WM8960_REGMASK_OUT_VU) |
					(data->reg[WM8960_REG_LEFT_DAC_VOLUME] &
					 ~WM8960_REGMASK_OUT_VU);
				LOG_DBG("WM8960 emulator: Copied left DAC volume to right: 0x%04x",
					data->reg[WM8960_REG_RIGHT_DAC_VOLUME]);
			}
			break;

		case WM8960_REG_LOUT1_VOLUME:
		case WM8960_REG_ROUT1_VOLUME:
			LOG_DBG("WM8960 emulator: Headphone output reg 0x%02x set to 0x%04x",
				data->current_reg, data->reg[data->current_reg]);

			/* Handle HPVU bit (update both channels) */
			if ((data->reg[data->current_reg] & WM8960_REGMASK_OUT_VU) &&
			    (data->current_reg == WM8960_REG_LOUT1_VOLUME)) {
				/* Copy left volume to right volume, preserving VU bit */
				data->reg[WM8960_REG_ROUT1_VOLUME] =
					(data->reg[WM8960_REG_ROUT1_VOLUME] &
					 WM8960_REGMASK_OUT_VU) |
					(data->reg[WM8960_REG_LOUT1_VOLUME] &
					 ~WM8960_REGMASK_OUT_VU);
				LOG_DBG("WM8960 emulator: Copied left HP volume to right: 0x%04x",
					data->reg[WM8960_REG_ROUT1_VOLUME]);
			}
			break;

		case WM8960_REG_LOUT2_VOLUME:
		case WM8960_REG_ROUT2_VOLUME:
			LOG_DBG("WM8960 emulator: Speaker output reg 0x%02x set to 0x%04x",
				data->current_reg, data->reg[data->current_reg]);

			/* Handle SPKVU bit (update both channels) */
			if ((data->reg[data->current_reg] & WM8960_REGMASK_OUT_VU) &&
			    (data->current_reg == WM8960_REG_LOUT2_VOLUME)) {
				/* Copy left volume to right volume, preserving VU bit */
				data->reg[WM8960_REG_ROUT2_VOLUME] =
					(data->reg[WM8960_REG_ROUT2_VOLUME] &
					 WM8960_REGMASK_OUT_VU) |
					(data->reg[WM8960_REG_LOUT2_VOLUME] &
					 ~WM8960_REGMASK_OUT_VU);
				LOG_DBG("WM8960 emulator: Copied left SPK volume to right: 0x%04x",
					data->reg[WM8960_REG_ROUT2_VOLUME]);
			}
			break;

		case WM8960_REG_CLOCKING_1:
		case WM8960_REG_CLOCKING_2:
			LOG_DBG("WM8960 emulator: Clock configuration reg 0x%02x set to 0x%04x",
				data->current_reg, data->reg[data->current_reg]);
			break;

		case WM8960_REG_PLL_N:
		case WM8960_REG_PLL_K1:
		case WM8960_REG_PLL_K2:
		case WM8960_REG_PLL_K3:
			LOG_DBG("WM8960 emulator: PLL control reg 0x%02x set to 0x%04x",
				data->current_reg, data->reg[data->current_reg]);
			break;

		default:
			/* No special handling needed */
			break;
		}
	} else {
		LOG_WRN("WM8960 emulator: Write to invalid register 0x%02x", data->current_reg);
		return -EINVAL;
	}

	return 0;
}

static int wm8960_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct wm8960_emul_data *data = target->data;
	int ret = 0;

	ARG_UNUSED(addr);

	/* Reset register address tracking for new transfer */
	data->reg_addr_set = false;

	LOG_DBG("WM8960 emulator: I2C transfer with %d messages", num_msgs);

	for (int i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			/*
			 * WM8960 doesn't support reads in real hardware, but we'll implement it
			 * for testing purposes
			 */
			LOG_WRN("WM8960 emulator: Read operation requested (not supported in real "
				"hardware)");

			/* Fill buffer with register value */
			if (data->current_reg < WM8960_NUM_REGS) {
				if (msgs[i].len >= 1) {
					msgs[i].buf[0] = (data->reg[data->current_reg] >> 8) & 0x01;
				}
				if (msgs[i].len >= 2) {
					msgs[i].buf[1] = data->reg[data->current_reg] & 0xFF;
				}
			} else {
				ret = -EINVAL;
				break;
			}
		} else {
			/* Write operation */
			for (int j = 0; j < msgs[i].len; j++) {
				ret = wm8960_emul_write(target, msgs[i].buf[j], msgs[i].buf[j],
							j + 1, NULL);
				if (ret != 0) {
					break;
				}
			}
		}
		if (ret != 0) {
			LOG_ERR("WM8960 emulator: Transfer failed with error %d", ret);
			break;
		}
	}

	return ret;
}

static const struct i2c_emul_api wm8960_emul_api_i2c = {
	.transfer = wm8960_emul_transfer_i2c,
};

static int wm8960_emul_init(const struct emul *target, const struct device *parent)
{
	struct wm8960_emul_data *data = target->data;

	ARG_UNUSED(parent);

	wm8960_emul_reset_registers(data);

	LOG_INF("WM8960 emulator initialized with %d registers", WM8960_NUM_REGS);
	return 0;
}

/* Additional helper functions for testing */
uint16_t wm8960_emul_get_volume(const struct emul *target, uint8_t channel)
{
	struct wm8960_emul_data *data = target->data;

	switch (channel) {
	case 0: /* Left DAC */
		return data->reg[WM8960_REG_LEFT_DAC_VOLUME];
	case 1: /* Right DAC */
		return data->reg[WM8960_REG_RIGHT_DAC_VOLUME];
	case 2: /* Left Headphone */
		return data->reg[WM8960_REG_LOUT1_VOLUME];
	case 3: /* Right Headphone */
		return data->reg[WM8960_REG_ROUT1_VOLUME];
	case 4: /* Left Speaker */
		return data->reg[WM8960_REG_LOUT2_VOLUME];
	case 5: /* Right Speaker */
		return data->reg[WM8960_REG_ROUT2_VOLUME];
	case 6: /* Left ADC */
		return data->reg[WM8960_REG_LEFT_ADC_VOLUME];
	case 7: /* Right ADC */
		return data->reg[WM8960_REG_RIGHT_ADC_VOLUME];
	default:
		return 0;
	}
}

bool wm8960_emul_is_muted(const struct emul *target, uint8_t channel)
{
	struct wm8960_emul_data *data = target->data;
	uint16_t reg_val;

	switch (channel) {
	case 0: /* Left DAC */
		reg_val = data->reg[WM8960_REG_LEFT_DAC_VOLUME] & 0xFF;
		break;
	case 1: /* Right DAC */
		reg_val = data->reg[WM8960_REG_RIGHT_DAC_VOLUME] & 0xFF;
		break;
	case 2: /* Left Headphone */
		reg_val = data->reg[WM8960_REG_LOUT1_VOLUME] & 0xFF;
		break;
	case 3: /* Right Headphone */
		reg_val = data->reg[WM8960_REG_ROUT1_VOLUME] & 0xFF;
		break;
	case 4: /* Left Speaker */
		reg_val = data->reg[WM8960_REG_LOUT2_VOLUME] & 0xFF;
		break;
	case 5: /* Right Speaker */
		reg_val = data->reg[WM8960_REG_ROUT2_VOLUME] & 0xFF;
		break;
	default:
		return false;
	}

	return reg_val == 0;
}

uint8_t wm8960_emul_get_audio_format(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return data->reg[WM8960_REG_AUDIO_INTERFACE_1] & WM8960_AIF_FMT_MASK;
}

uint8_t wm8960_emul_get_word_length(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;
	uint8_t wl_bits = (data->reg[WM8960_REG_AUDIO_INTERFACE_1] & WM8960_AIF_WL_MASK) >> 2;

	switch (wl_bits) {
	case 0:
		return 16;
	case 1:
		return 20;
	case 2:
		return 24;
	case 3:
		return 32;
	default:
		return 0;
	}
}

bool wm8960_emul_is_pll_enabled(const struct emul *target)
{
	struct wm8960_emul_data *data = target->data;

	return (data->reg[WM8960_REG_CLOCKING_1] & WM8960_CLOCK1_CLKSEL_PLL) != 0;
}

#define WM8960_EMUL_DEFINE(inst)                                                                   \
	static struct wm8960_emul_data wm8960_emul_data_##inst;                                    \
	static const struct wm8960_emul_cfg wm8960_emul_cfg_##inst = {                             \
		.addr = DT_INST_REG_ADDR(inst),                                                    \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, wm8960_emul_init, &wm8960_emul_data_##inst,                      \
			    &wm8960_emul_cfg_##inst, (struct i2c_emul_api *)&wm8960_emul_api_i2c,  \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(WM8960_EMUL_DEFINE)

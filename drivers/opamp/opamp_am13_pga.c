/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am13_pga

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/opamp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <soc.h>

#define AM13_SYSCTL_PWREN_SYSPERIPH_OFFSET 0x1434U
#define AM13_SYSCTL_PWREN_KEY_MASK         GENMASK(31, 24)
#define AM13_SYSCTL_PWREN_KEY              FIELD_PREP(AM13_SYSCTL_PWREN_KEY_MASK, 0x26U)

/* Register offsets from instance base */
#define AM13_PGA_PGACTL_OFFSET 0x00U
#define AM13_PGA_MUXSEL_OFFSET 0x04U

/* PGACTL bit fields */
#define AM13_PGA_PGACTL_PGAEN         BIT(0)
#define AM13_PGA_PGACTL_FILT_MASK     GENMASK(3, 1)
#define AM13_PGA_PGACTL_GAIN_MASK     GENMASK(7, 5)
#define AM13_PGA_PGACTL_OUTENABLE     BIT(8)
#define AM13_PGA_PGACTL_OUTEN_INTGAIN BIT(9)

/* MUXSEL bit fields */
#define AM13_PGA_MUXSEL_PMUX_MASK GENMASK(3, 0)
#define AM13_PGA_MUXSEL_NMUX_MASK GENMASK(6, 4)
#define AM13_PGA_MUXSEL_MMUX_MASK GENMASK(10, 8)
#define AM13_PGA_MUXSEL_FBONPIN   BIT(12)
#define AM13_PGA_MUXSEL_CHOP_MASK GENMASK(14, 13)

struct am13_pga_config {
	mm_reg_t base;
	const struct device *sysctl;
	const struct pinctrl_dev_config *pinctrl;
	uint16_t pgactl_init; /* pre-computed; excludes PGAEN bit */
	uint16_t muxsel_init;
	uint8_t power_mask;
	uint8_t max_gain_code; /* limited by the output filter resistor */
	uint8_t mode;
};

struct am13_pga_data {
	struct k_spinlock lock;
};

static int am13_pga_set_gain(const struct device *dev, enum opamp_gain gain)
{
	const struct am13_pga_config *config = dev->config;
	struct am13_pga_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t code;
	uint16_t val;

	if (config->mode == OPAMP_FUNCTIONAL_MODE_STANDALONE) {
		return 0;
	}

	if (config->mode == OPAMP_FUNCTIONAL_MODE_FOLLOWER) {
		return gain == OPAMP_GAIN_1 ? 0 : -ENOTSUP;
	}

	if (config->mode == OPAMP_FUNCTIONAL_MODE_INVERTING) {
		switch (gain) {
		case OPAMP_GAIN_1:
			code = 1;
			break;
		case OPAMP_GAIN_3:
			code = 2;
			break;
		case OPAMP_GAIN_7:
			code = 3;
			break;
		case OPAMP_GAIN_15:
			code = 4;
			break;
		case OPAMP_GAIN_31:
			code = 5;
			break;
		case OPAMP_GAIN_63:
			code = 6;
			break;
		default:
			return -ENOTSUP;
		}
	} else {
		switch (gain) {
		case OPAMP_GAIN_1:
			code = 0;
			break;
		case OPAMP_GAIN_2:
			code = 1;
			break;
		case OPAMP_GAIN_4:
			code = 2;
			break;
		case OPAMP_GAIN_8:
			code = 3;
			break;
		case OPAMP_GAIN_16:
			code = 4;
			break;
		case OPAMP_GAIN_32:
			code = 5;
			break;
		case OPAMP_GAIN_64:
			code = 6;
			break;
		default:
			return -ENOTSUP;
		}
	}

	if (code > config->max_gain_code) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	val = sys_read16(config->base + AM13_PGA_PGACTL_OFFSET);
	val &= ~AM13_PGA_PGACTL_GAIN_MASK;
	val |= FIELD_PREP(AM13_PGA_PGACTL_GAIN_MASK, code);
	sys_write16(val, config->base + AM13_PGA_PGACTL_OFFSET);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static DEVICE_API(opamp, am13_pga_api) = {
	.set_gain = am13_pga_set_gain,
};

static int am13_pga_init(const struct device *dev)
{
	const struct am13_pga_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->sysctl)) {
		return -ENODEV;
	}

	if (config->pinctrl != NULL) {
		ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}
	}

	/* Enable peripheral power before accessing PGA registers. */
	ret = syscon_update_bits(config->sysctl, AM13_SYSCTL_PWREN_SYSPERIPH_OFFSET,
				 AM13_SYSCTL_PWREN_KEY_MASK | config->power_mask,
				 AM13_SYSCTL_PWREN_KEY | config->power_mask);
	if (ret < 0) {
		return ret;
	}

	msp_delay_peripheral_startup();

	/* Configure the input topology before enabling the amplifier. */
	sys_write16(config->muxsel_init, config->base + AM13_PGA_MUXSEL_OFFSET);
	sys_write16(config->pgactl_init, config->base + AM13_PGA_PGACTL_OFFSET);
	sys_write16(config->pgactl_init | AM13_PGA_PGACTL_PGAEN,
		    config->base + AM13_PGA_PGACTL_OFFSET);

	return 0;
}

#define AM13_PGA_PINCTRL_DT_INST_DEFINE(inst)                                                      \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(inst, default), (PINCTRL_DT_INST_DEFINE(inst)), ())

#define AM13_PGA_PINCTRL_DT_INST_GET(inst)                                                         \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(inst, default),                                     \
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL))

#define AM13_PGA_NMUX(inst)        DT_INST_ENUM_IDX(inst, ti_nmux_input)
#define AM13_PGA_MMUX(inst)        DT_INST_ENUM_IDX(inst, ti_mmux_input)
#define AM13_PGA_MODE(inst, value) DT_INST_ENUM_HAS_VALUE(inst, functional_mode, value)
#define AM13_PGA_LADDER(inst)      (AM13_PGA_NMUX(inst) == 1)
#define AM13_PGA_VALID_MODE(inst)                                                                  \
	((AM13_PGA_MODE(inst, non_inverting) && AM13_PGA_LADDER(inst) &&                           \
	  AM13_PGA_MMUX(inst) == 1) ||                                                             \
	 ((AM13_PGA_MODE(inst, inverting) || AM13_PGA_MODE(inst, differential)) &&                 \
	  AM13_PGA_LADDER(inst) && AM13_PGA_MMUX(inst) >= 2) ||                                    \
	 (AM13_PGA_MODE(inst, follower) && AM13_PGA_NMUX(inst) == 0 &&                             \
	  AM13_PGA_MMUX(inst) == 0) ||                                                             \
	 (AM13_PGA_MODE(inst, standalone) && AM13_PGA_NMUX(inst) >= 2 &&                           \
	  AM13_PGA_MMUX(inst) == 0))

#define AM13_PGA_FILT_IDX(inst) DT_INST_ENUM_IDX(inst, ti_filter_resistor_ohms)

/*
 * Closes the internal feedback path when the output is on the pin without the filter resistor.
 * Without it the feedback loop is open and the output saturates. Standalone mode uses external
 * feedback.
 */
#define AM13_PGA_INTGAIN(inst)                                                                     \
	(DT_INST_PROP(inst, ti_output_to_pin) && AM13_PGA_FILT_IDX(inst) == 0 &&                   \
	 !AM13_PGA_MODE(inst, standalone))

/* DT chop enum {disabled, adc-soc} -> CHOP field {0, 2} */
#define AM13_PGA_CHOP(inst) (DT_INST_ENUM_IDX(inst, ti_chop_mode) * 2)

/* Highest GAIN code allowed by the filter resistor, TRM Table 23-6 (index 0 = no filter) */
#define AM13_PGA_MAX_GAIN_CODE(inst)                                                               \
	(AM13_PGA_FILT_IDX(inst) == 1   ? 2                                                        \
	 : AM13_PGA_FILT_IDX(inst) == 2 ? 4                                                        \
	 : AM13_PGA_FILT_IDX(inst) == 3 ? 5                                                        \
					: 6)

#define AM13_PGA_INIT(inst)                                                                        \
	BUILD_ASSERT(AM13_PGA_VALID_MODE(inst),                                                    \
		     "functional-mode does not match PGA mux configuration");                      \
	BUILD_ASSERT(!DT_INST_PROP(inst, ti_feedback_on_pin) ||                                    \
			     AM13_PGA_MODE(inst, non_inverting),                                   \
		     "feedback-on-pin requires non_inverting mode");                               \
	AM13_PGA_PINCTRL_DT_INST_DEFINE(inst);                                                     \
	static const struct am13_pga_config am13_pga_cfg_##inst = {                                \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.sysctl = DEVICE_DT_GET(DT_INST_PHANDLE(inst, sysctl)),                            \
		.power_mask = BIT(DT_INST_PROP(inst, ti_power_bit)),                               \
		.pinctrl = AM13_PGA_PINCTRL_DT_INST_GET(inst),                                     \
		.pgactl_init =                                                                     \
			FIELD_PREP(AM13_PGA_PGACTL_GAIN_MASK,                                      \
				   DT_INST_ENUM_HAS_VALUE(inst, functional_mode, inverting)) |     \
			FIELD_PREP(AM13_PGA_PGACTL_FILT_MASK,                                      \
				   DT_INST_ENUM_IDX(inst, ti_filter_resistor_ohms)) |              \
			FIELD_PREP(AM13_PGA_PGACTL_OUTENABLE,                                      \
				   DT_INST_PROP(inst, ti_output_to_pin)) |                         \
			FIELD_PREP(AM13_PGA_PGACTL_OUTEN_INTGAIN, AM13_PGA_INTGAIN(inst)),         \
		.muxsel_init = FIELD_PREP(AM13_PGA_MUXSEL_PMUX_MASK,                               \
					  DT_INST_ENUM_IDX(inst, ti_pmux_input)) |                 \
			       FIELD_PREP(AM13_PGA_MUXSEL_NMUX_MASK,                               \
					  DT_INST_ENUM_IDX(inst, ti_nmux_input)) |                 \
			       FIELD_PREP(AM13_PGA_MUXSEL_MMUX_MASK,                               \
					  DT_INST_ENUM_IDX(inst, ti_mmux_input)) |                 \
			       FIELD_PREP(AM13_PGA_MUXSEL_FBONPIN,                                 \
					  DT_INST_PROP(inst, ti_feedback_on_pin)) |                \
			       FIELD_PREP(AM13_PGA_MUXSEL_CHOP_MASK, AM13_PGA_CHOP(inst)),         \
		.max_gain_code = AM13_PGA_MAX_GAIN_CODE(inst),                                     \
		.mode = DT_INST_ENUM_IDX(inst, functional_mode),                                   \
	};                                                                                         \
	static struct am13_pga_data am13_pga_data_##inst;                                          \
	DEVICE_DT_INST_DEFINE(inst, am13_pga_init, NULL, &am13_pga_data_##inst,                    \
			      &am13_pga_cfg_##inst, POST_KERNEL, CONFIG_OPAMP_INIT_PRIORITY,       \
			      &am13_pga_api);

DT_INST_FOREACH_STATUS_OKAY(AM13_PGA_INIT)

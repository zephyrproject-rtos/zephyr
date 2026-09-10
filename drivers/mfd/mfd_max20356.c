/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/max20356.h>

#include "mfd_max20356.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_max20356, CONFIG_REGULATOR_LOG_LEVEL);

#define MAX20356_PWRCMD_OFF        0xB2U
#define MAX20356_PWRCMD_HARD_RESET 0xC3U
#define MAX20356_PWRCMD_SOFT_RESET 0xD4U
#define MAX20356_PWRCMD_SEAL       0xE5U

#define MAX20356_MONCFG_MONCTR_MAX      0x0FU
#define MAX20356_MONCFG_MONRATIOCFG_MAX 0x03U

/* LockUnlock passwords (LockUnlock1-3). */
#define MAX20356_LOCK_PASSWD_UNLOCK 0x55U
#define MAX20356_LOCK_PASSWD_LOCK   0xAAU

/* MPC-select field common to every ITRCfg register (MPC0..MPC6).
 * The top bit of each ITRCfg register is the source's own interrupt-enable.
 */
#define MAX20356_ITRCFG_MPCSEL_MSK GENMASK(6, 0)
#define MAX20356_ITRCFG_INT_MSK    BIT(7)

/* Maps a lock domain to its LockMsk register, bit mask, and LockUnlock register. */
struct max20356_lock_desc {
	uint8_t lockmsk_reg;
	uint8_t lockmsk_bit;
	uint8_t unlock_reg;
};

static const struct max20356_lock_desc max20356_lock_table[MAX20356_LOCK_MAX] = {
	[MAX20356_LOCK_BUCK1] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_BK1LCK_MSK,
				 MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_BUCK2] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_BK2LCK_MSK,
				 MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_BUCK3] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_BK3LCK_MSK,
				 MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_BBST] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_BBLCK_MSK,
				MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_LDO1] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_LD1LCK_MSK,
				MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_LDO2] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_LD2LCK_MSK,
				MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_LDO3] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_LD3LCK_MSK,
				MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_LDO4] = {MAX20356_REG_LOCKMSK1, MAX20356_LOCKMSK1_LD4LCK_MSK,
				MAX20356_REG_LOCKUNLOCK1},
	[MAX20356_LOCK_CHG] = {MAX20356_REG_LOCKMSK3, MAX20356_LOCKMSK3_CHGLCK_MSK,
			       MAX20356_REG_LOCKUNLOCK3},
	[MAX20356_LOCK_LIM] = {MAX20356_REG_LOCKMSK3, MAX20356_LOCKMSK3_LIMLCK_MSK,
			       MAX20356_REG_LOCKUNLOCK3},
	[MAX20356_LOCK_WD] = {MAX20356_REG_LOCKMSK3, MAX20356_LOCKMSK3_WDLCK_MSK,
			      MAX20356_REG_LOCKUNLOCK3},
	[MAX20356_LOCK_BUCK1_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_BK1SEQLCK_MSK,
				     MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_BUCK2_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_BK2SEQLCK_MSK,
				     MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_BUCK3_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_BK3SEQLCK_MSK,
				     MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_BBST_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_BBSEQLCK_MSK,
				    MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_LDO1_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_LD1SEQLCK_MSK,
				    MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_LDO2_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_LD2SEQLCK_MSK,
				    MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_LDO3_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_LD3SEQLCK_MSK,
				    MAX20356_REG_LOCKUNLOCK2},
	[MAX20356_LOCK_LDO4_SEQ] = {MAX20356_REG_LOCKMSK2, MAX20356_LOCKMSK2_LD4SEQLCK_MSK,
				    MAX20356_REG_LOCKUNLOCK2},
};

static inline const struct i2c_dt_spec *mfd_max20356_get_i2c(const struct device *dev)
{
	const struct mfd_max20356_config *config = dev->config;

	return &config->i2c;
}

int mfd_max20356_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	return i2c_reg_read_byte_dt(mfd_max20356_get_i2c(dev), reg, val);
}

int mfd_max20356_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(mfd_max20356_get_i2c(dev), reg, val);
}

int mfd_max20356_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	return i2c_reg_update_byte_dt(mfd_max20356_get_i2c(dev), reg, mask, val);
}

int mfd_max20356_dvs_spi_write(const struct device *dev, uint8_t cmd)
{
#ifdef CONFIG_SPI
	const struct mfd_max20356_config *config = dev->config;
	const struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &buf, .count = 1U};

	if (!spi_is_ready_dt(&config->dvs_spi)) {
		return -ENODEV;
	}

	return spi_write_dt(&config->dvs_spi, &tx);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);

	return -ENOTSUP;
#endif /* CONFIG_SPI */
}

int mfd_max20356_reg_update_locked(const struct device *dev, enum max20356_lock_domain domain,
				   uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct max20356_lock_desc *desc;
	int ret;

	if (domain >= MAX20356_LOCK_MAX) {
		return -EINVAL;
	}

	desc = &max20356_lock_table[domain];

	/* Unmask only this domain (its LockMsk bit = 0, all others = 1) so the
	 * password affects a single function, then unlock, write and re-lock.
	 */
	ret = mfd_max20356_reg_write(dev, desc->lockmsk_reg, (uint8_t)~desc->lockmsk_bit);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_max20356_reg_write(dev, desc->unlock_reg, MAX20356_LOCK_PASSWD_UNLOCK);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_max20356_reg_update(dev, reg, mask, val);
	if (ret != 0) {
		return ret;
	}

	ret = mfd_max20356_reg_write(dev, desc->unlock_reg, MAX20356_LOCK_PASSWD_LOCK);

	return ret;
}

enum max20356_variant mfd_max20356_get_variant(const struct device *dev)
{
	const struct mfd_max20356_config *config = dev->config;

	return config->variant;
}

int mfd_max20356_power_command(const struct device *dev, enum max20356_power_cmd cmd)
{
	uint8_t val;

	switch (cmd) {
	case MAX20356_PWR_OFF:
		val = MAX20356_PWRCMD_OFF;
		break;
	case MAX20356_PWR_HARD_RESET:
		val = MAX20356_PWRCMD_HARD_RESET;
		break;
	case MAX20356_PWR_SOFT_RESET:
		val = MAX20356_PWRCMD_SOFT_RESET;
		break;
	case MAX20356_PWR_SEAL:
		val = MAX20356_PWRCMD_SEAL;
		break;
	default:
		return -EINVAL;
	}

	return mfd_max20356_reg_write(dev, MAX20356_REG_PWRCMD, val);
}

int mfd_max20356_mon_select(const struct device *dev, uint8_t channel, uint8_t ratio)
{
	uint8_t mask = MAX20356_MONCFG_MONCTR_MSK | MAX20356_MONCFG_MONRATIOCFG_MSK;
	uint8_t val;

	if (channel > MAX20356_MONCFG_MONCTR_MAX || ratio > MAX20356_MONCFG_MONRATIOCFG_MAX) {
		return -EINVAL;
	}

	val = FIELD_PREP(MAX20356_MONCFG_MONCTR_MSK, channel) |
	      FIELD_PREP(MAX20356_MONCFG_MONRATIOCFG_MSK, ratio);

	return mfd_max20356_reg_update(dev, MAX20356_REG_MONCFG, mask, val);
}

int mfd_max20356_pfn_status(const struct device *dev, enum max20356_pfn pfn, bool *active)
{
	uint8_t mask, val;
	int ret;

	if (active == NULL) {
		return -EINVAL;
	}

	switch (pfn) {
	case MAX20356_PFN1:
		mask = MAX20356_PFN_PFN1PIN_MSK;
		break;
	case MAX20356_PFN2:
		mask = MAX20356_PFN_PFN2PIN_MSK;
		break;
	default:
		return -EINVAL;
	}

	ret = mfd_max20356_reg_read(dev, MAX20356_REG_PFN, &val);
	if (ret != 0) {
		return ret;
	}

	*active = (val & mask) != 0U;

	return 0;
}

int mfd_max20356_mpc_int_route(const struct device *dev, enum max20356_mpc_int_source source,
			       uint8_t mpc_mask, bool int_enable)
{
	uint8_t reg, mask, val;

	switch (source) {
	case MAX20356_MPC_INT_BUCK1_PGOOD:
		reg = MAX20356_REG_BK1ITRCFG;
		break;
	case MAX20356_MPC_INT_BUCK2_PGOOD:
		reg = MAX20356_REG_BK2ITRCFG;
		break;
	case MAX20356_MPC_INT_BUCK3_PGOOD:
		reg = MAX20356_REG_BK3ITRCFG;
		break;
	case MAX20356_MPC_INT_USBOK:
		reg = MAX20356_REG_USBOKITRCFG;
		break;
	default:
		return -EINVAL;
	}

	mask = MAX20356_ITRCFG_MPCSEL_MSK | MAX20356_ITRCFG_INT_MSK;
	val = (mpc_mask & MAX20356_ITRCFG_MPCSEL_MSK) |
	      (int_enable ? MAX20356_ITRCFG_INT_MSK : 0U);

	return mfd_max20356_reg_update(dev, reg, mask, val);
}

#ifdef CONFIG_WDT_MAX20356
int mfd_max20356_wdt_set_rsttype(const struct device *dev, enum max20356_wdt_rsttype rsttype)
{
	return mfd_max20356_reg_update(dev, MAX20356_REG_WDCNTL, MAX20356_WDCNTL_WDRSTTYPE_MSK,
				       FIELD_PREP(MAX20356_WDCNTL_WDRSTTYPE_MSK, rsttype));
}

int mfd_max20356_wdt_feed(const struct device *dev)
{
	uint8_t int5;

	/* read INT5 watchdog bit to reset the timer */
	return mfd_max20356_reg_read(dev, MAX20356_REG_INT5, &int5);
}

int mfd_max20356_wdt_claim(const struct device *dev, bool claim)
{
	struct mfd_max20356_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->cb_lock, K_FOREVER);

	/* refuse access if INTB is occupied by any other interrupt is configured */
	if (claim) {
		for (uint8_t evt = 0; evt < MAX20356_EVT_MAX; evt++) {
			if (data->cb[evt] != NULL) {
				ret = -EBUSY;
				break;
			}
		}

		if (ret == 0) {
			data->wdt_active = true;
		}
	} else {
		data->wdt_active = false;
	}

	k_mutex_unlock(&data->cb_lock);

	return ret;
}
#endif /* CONFIG_WDT_MAX20356 */

static int mfd_max20356_init_regs(const struct device *dev)
{
	const struct mfd_max20356_config *config = dev->config;
	int ret;

	for (uint8_t i = 0U; i < config->num_init_regs; i++) {
		const struct mfd_max20356_init_reg *e = &config->init_regs[i];

		if (e->mask == 0U) {
			continue;
		}

		ret = mfd_max20356_reg_update(dev, e->reg, e->mask, e->val);
		if (ret != 0) {
			LOG_ERR("Failed to write reg %d with val %d: %d", e->reg, e->val, ret);
			return ret;
		}
	}

	return 0;
}

static int mfd_max20356_init(const struct device *dev)
{
	const struct mfd_max20356_config *config = dev->config;
	uint8_t revid;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C Bus is not ready");
		return -ENODEV;
	}

	/* The MAX20356/20358 do not have a device ID */
	ret = mfd_max20356_reg_read(dev, MAX20356_REG_REVID, &revid);
	if (ret < 0) {
		return ret;
	}

	ret = mfd_max20356_init_regs(dev);
	if (ret != 0) {
		LOG_ERR("Failed to init registers: %d", ret);
		return ret;
	}

#ifdef CONFIG_MFD_MAX20356_TRIGGER
	ret = mfd_max20356_trigger_init(dev);
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_MFD_MAX20356_TRIGGER */

	return 0;
}

/* boolean fields are written only when property is present; otherwise the OTP default is kept. */
#define MFD_MAX20356_DT_BOOL(inst, prop, msk) (DT_INST_PROP(inst, prop) ? (uint8_t)(msk) : 0U)

/* enum fields contribute to the mask only when the property is present; the
 * field value is the enum index, whose ordering matches the register encoding.
 */
#define MFD_MAX20356_DT_ENUM_MASK(inst, prop, msk)                                                 \
	(DT_INST_NODE_HAS_PROP(inst, prop) ? (uint8_t)(msk) : 0U)
#define MFD_MAX20356_DT_ENUM_VALUE(inst, prop, msk)                                                \
	((uint8_t)FIELD_PREP(msk, DT_INST_ENUM_IDX_OR(inst, prop, 0)))

#define MFD_MAX20356_ILIMCTRL1_MASK(inst)                                                          \
	(MFD_MAX20356_DT_ENUM_MASK(inst, adi_input_current_limit_milliamp,                         \
				MAX20356_ILIMCTRL1_ILIMCNTL_MSK) |                                 \
	 MFD_MAX20356_DT_ENUM_MASK(inst, adi_input_current_limit_max_milliamp,                     \
				MAX20356_ILIMCTRL1_ILIMMAX_MSK) |                                  \
	 MFD_MAX20356_DT_ENUM_MASK(inst, adi_input_current_limit_blanking_microsec,                \
				MAX20356_ILIMCTRL1_ILIMBLANK_MSK))
#define MFD_MAX20356_ILIMCTRL1_VALUE(inst)                                                         \
	(MFD_MAX20356_DT_ENUM_VALUE(inst, adi_input_current_limit_milliamp,                        \
				MAX20356_ILIMCTRL1_ILIMCNTL_MSK) |                                 \
	 MFD_MAX20356_DT_ENUM_VALUE(inst, adi_input_current_limit_max_milliamp,                    \
				MAX20356_ILIMCTRL1_ILIMMAX_MSK) |                                  \
	 MFD_MAX20356_DT_ENUM_VALUE(inst, adi_input_current_limit_blanking_microsec,               \
				MAX20356_ILIMCTRL1_ILIMBLANK_MSK))

#define MFD_MAX20356_ILIMCTRL2_MASK(inst)                                                          \
	(MFD_MAX20356_DT_BOOL(inst, adi_sys_discharge_in_recovery,                                 \
			      MAX20356_ILIMCTRL2_SYSDSCEN_MSK) |                                   \
	 MFD_MAX20356_DT_ENUM_MASK(inst, adi_sys_min_voltage_microvolt,                            \
				MAX20356_ILIMCTRL2_SYSMINVLT_MSK))
#define MFD_MAX20356_ILIMCTRL2_VALUE(inst)                                                         \
	(MFD_MAX20356_DT_BOOL(inst, adi_sys_discharge_in_recovery,                                 \
			      MAX20356_ILIMCTRL2_SYSDSCEN_MSK) |                                   \
	 MFD_MAX20356_DT_ENUM_VALUE(inst, adi_sys_min_voltage_microvolt,                           \
				MAX20356_ILIMCTRL2_SYSMINVLT_MSK))

#define MFD_MAX20356_DROPCTRL_MASK(inst)                                                           \
	MFD_MAX20356_DT_ENUM_MASK(inst, adi_sys_uvlo_threshold_microvolt,                          \
			       MAX20356_DROPCTRL_SYSUVLOTHSEL_MSK)
#define MFD_MAX20356_DROPCTRL_VALUE(inst)                                                          \
	MFD_MAX20356_DT_ENUM_VALUE(inst, adi_sys_uvlo_threshold_microvolt,                         \
			       MAX20356_DROPCTRL_SYSUVLOTHSEL_MSK)

#define MFD_MAX20356_BOOTCFG_MASK(inst)                                                            \
	(MFD_MAX20356_DT_ENUM_MASK(inst, adi_power_reset_config,                                   \
				MAX20356_BOOTCFG_PWRRSTCFG_MSK) |                                  \
	 MFD_MAX20356_DT_BOOL(inst, adi_soft_reset_keeps_registers,                                \
			      MAX20356_BOOTCFG_SFTRSTCFG_MSK) |                                    \
	 MFD_MAX20356_DT_ENUM_MASK(inst, adi_boot_delay, MAX20356_BOOTCFG_BOOTDLY_MSK) |           \
	 MFD_MAX20356_DT_BOOL(inst, adi_sys_uvlo_auto_retry, MAX20356_BOOTCFG_CHGALWTRY_MSK))
#define MFD_MAX20356_BOOTCFG_VALUE(inst)                                                           \
	(MFD_MAX20356_DT_ENUM_VALUE(inst, adi_power_reset_config,                                  \
				MAX20356_BOOTCFG_PWRRSTCFG_MSK) |                                  \
	 MFD_MAX20356_DT_BOOL(inst, adi_soft_reset_keeps_registers,                                \
			      MAX20356_BOOTCFG_SFTRSTCFG_MSK) |                                    \
	 MFD_MAX20356_DT_ENUM_VALUE(inst, adi_boot_delay, MAX20356_BOOTCFG_BOOTDLY_MSK) |          \
	 MFD_MAX20356_DT_BOOL(inst, adi_sys_uvlo_auto_retry, MAX20356_BOOTCFG_CHGALWTRY_MSK))

#define MFD_MAX20356_PWRCFG_BITS(inst)                                                             \
	(MFD_MAX20356_DT_BOOL(inst, adi_intb_unmasked_in_shutdown,                                 \
			      MAX20356_PWRCFG_INTBOOTMSK_MSK) |                                    \
	 MFD_MAX20356_DT_BOOL(inst, adi_stay_on, MAX20356_PWRCFG_STAYON_MSK))

#define MFD_MAX20356_MISCFUNC_BITS(inst)                                                           \
	(MFD_MAX20356_DT_BOOL(inst, adi_active_discharge_constant,                                 \
			      MAX20356_MISCFUNCTIONS_DISCHARGECONST_MSK) |                         \
	 MFD_MAX20356_DT_BOOL(inst, adi_rtc_ldo_off, MAX20356_MISCFUNCTIONS_RTCLDOOFF_MSK) |       \
	 MFD_MAX20356_DT_BOOL(inst, adi_factory_mode_disabled,                                     \
			      MAX20356_MISCFUNCTIONS_FACTORYMODEDIS_MSK))

#define MFD_MAX20356_INIT_ENTRY(regmac, bits) {(regmac), (bits), (bits)}

/* SPI DVS controller spec, only for a parent carrying spi-dvs; empty otherwise. */
#define MFD_MAX20356_DVS_SPI_INIT(inst)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, spi_dvs),                                          \
		    (.dvs_spi = SPI_DT_SPEC_GET(DT_INST_PHANDLE(inst, spi_dvs),                    \
						SPI_WORD_SET(8) | SPI_TRANSFER_MSB),), ())

#define MFD_MAX20356_DEFINE(inst, variant_id)                                                      \
	static const struct mfd_max20356_init_reg mfd_max20356_init_regs_##variant_id##_##inst[] = \
	{                                                                                          \
		MFD_MAX20356_INIT_ENTRY(MAX20356_REG_PWRCFG, MFD_MAX20356_PWRCFG_BITS(inst)),      \
		MFD_MAX20356_INIT_ENTRY(MAX20356_REG_MISCFUNCTIONS,                                \
					MFD_MAX20356_MISCFUNC_BITS(inst)),                         \
		{MAX20356_REG_ILIMCTRL1, MFD_MAX20356_ILIMCTRL1_MASK(inst),                        \
		 MFD_MAX20356_ILIMCTRL1_VALUE(inst)},                                              \
		{MAX20356_REG_ILIMCTRL2, MFD_MAX20356_ILIMCTRL2_MASK(inst),                        \
		 MFD_MAX20356_ILIMCTRL2_VALUE(inst)},                                              \
		{MAX20356_REG_DROPCTRL, MFD_MAX20356_DROPCTRL_MASK(inst),                          \
		 MFD_MAX20356_DROPCTRL_VALUE(inst)},                                               \
		{MAX20356_REG_BOOTCFG, MFD_MAX20356_BOOTCFG_MASK(inst),                            \
		 MFD_MAX20356_BOOTCFG_VALUE(inst)},                                                \
		{MAX20356_REG_MONCFG,                                                              \
		 MFD_MAX20356_DT_BOOL(inst, adi_ivmon_high_impedance, MAX20356_MONCFG_MONHIZ_MSK), \
		 MFD_MAX20356_DT_BOOL(inst, adi_ivmon_high_impedance,                              \
				      MAX20356_MONCFG_MONHIZ_MSK)},                                \
	};                                                                                         \
                                                                                                   \
	static struct mfd_max20356_data mfd_max20356_data_##variant_id##_##inst;                   \
                                                                                                   \
	static const struct mfd_max20356_config mfd_max20356_config_##variant_id##_##inst = {      \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.variant = (variant_id),                                                           \
		.init_regs = mfd_max20356_init_regs_##variant_id##_##inst,                         \
		.num_init_regs = ARRAY_SIZE(mfd_max20356_init_regs_##variant_id##_##inst),         \
		IF_ENABLED(CONFIG_SPI, (MFD_MAX20356_DVS_SPI_INIT(inst)))                          \
		IF_ENABLED(CONFIG_MFD_MAX20356_TRIGGER,                                            \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),            \
			    .mpc_int_gpio = {                                                      \
				    GPIO_DT_SPEC_INST_GET_OR(inst, adi_buck1_pgood_int_gpios,      \
							     {0}),                                 \
				    GPIO_DT_SPEC_INST_GET_OR(inst, adi_buck2_pgood_int_gpios,      \
							     {0}),                                 \
				    GPIO_DT_SPEC_INST_GET_OR(inst, adi_buck3_pgood_int_gpios,      \
							     {0}),                                 \
				    GPIO_DT_SPEC_INST_GET_OR(inst, adi_usbok_int_gpios, {0}),      \
			    },))                                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_max20356_init, NULL,                                       \
			      &mfd_max20356_data_##variant_id##_##inst,                            \
			      &mfd_max20356_config_##variant_id##_##inst, POST_KERNEL,             \
			      CONFIG_MFD_INIT_PRIORITY, NULL);

#define DT_DRV_COMPAT adi_max20356
#define MFD_MAX20356_DEFINE_356(inst) MFD_MAX20356_DEFINE(inst, MAX20356_VARIANT_MAX20356)
DT_INST_FOREACH_STATUS_OKAY(MFD_MAX20356_DEFINE_356)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_max20358
#define MFD_MAX20356_DEFINE_358(inst) MFD_MAX20356_DEFINE(inst, MAX20356_VARIANT_MAX20358)
DT_INST_FOREACH_STATUS_OKAY(MFD_MAX20356_DEFINE_358)
#undef DT_DRV_COMPAT

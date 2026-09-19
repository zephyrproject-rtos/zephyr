/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc522x

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "tmc522x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc522x, CONFIG_STEPPER_LOG_LEVEL);

struct tmc522x_mfd_data {
	struct k_sem sem;
	const struct device *dev;
	struct k_work_delayable rampstat_callback_dwork;
	struct gpio_callback diag0_cb;
	struct gpio_callback diag1_cb;
	gpio_pin_t diag0_pin;
	gpio_pin_t diag1_pin;
};

struct tmc522x_mfd_config {
	union tmc522x_bus bus;
	const struct tmc522x_bus_io *bus_io;
	const uint32_t clock_frequency;
	const struct device **stepper_drivers;
	uint8_t num_stepper_drivers;
	const struct device **motion_controllers;
	uint8_t num_motion_controllers;
	/* GPIO pins */
	struct gpio_dt_spec drven_gpio;
	struct gpio_dt_spec sleepn_gpio;
	struct gpio_dt_spec diag0_gpio;
	struct gpio_dt_spec diag1_gpio;
};

/* Bus I/O functions for child devices */
int tmc522x_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
{
	const struct tmc522x_mfd_config *config = dev->config;
	struct tmc522x_mfd_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);
	err = config->bus_io->write(&config->bus, reg_addr, reg_val);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}

	return 0;
}

int tmc522x_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc522x_mfd_config *config = dev->config;
	struct tmc522x_mfd_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);
	err = config->bus_io->read(&config->bus, reg_addr, reg_val);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}

	return 0;
}

int tmc522x_get_clock_frequency(const struct device *dev)
{
	const struct tmc522x_mfd_config *config = dev->config;

	return config->clock_frequency;
}

int tmc522x_stepper_set_chopper_mode(const struct device *dev, bool stealthchop2)
{
	uint32_t gconf;
	int err;

	/* Read current GCONF register */
	err = tmc522x_read(dev, TMC522X_REG_GCONF, &gconf);
	if (err != 0) {
		LOG_ERR("Failed to read GCONF register");
		return err;
	}

	/* Modify EN_PWM_MODE bit (bit 0) */
	if (stealthchop2) {
		gconf |= TMC522X_GCONF_EN_PWM_MODE; /* Set bit 0 = StealthChop2 */
		LOG_DBG("Switching to StealthChop2 mode");
	} else {
		gconf &= ~TMC522X_GCONF_EN_PWM_MODE; /* Clear bit 0 = SpreadCycle */
		LOG_DBG("Switching to SpreadCycle mode");
	}

	/* Write modified GCONF back */
	err = tmc522x_write(dev, TMC522X_REG_GCONF, gconf);
	if (err != 0) {
		LOG_ERR("Failed to write GCONF register");
		return err;
	}

	LOG_DBG("Chopper mode set: %s", stealthchop2 ? "StealthChop2" : "SpreadCycle");
	return 0;
}

int tmc522x_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf, size_t num_regs)
{
	uint32_t val;
	int err;

	for (size_t i = 0; i < num_regs; i++) {
		err = tmc522x_read(dev, start_addr + i, &val);
		if (err != 0) {
			LOG_ERR("Burst read failed at reg 0x%02X", start_addr + (uint8_t)i);
			return err;
		}
		/* Store as big-endian (MSB first) */
		buf[i * 4 + 0] = (uint8_t)(val >> 24);
		buf[i * 4 + 1] = (uint8_t)(val >> 16);
		buf[i * 4 + 2] = (uint8_t)(val >> 8);
		buf[i * 4 + 3] = (uint8_t)(val);
	}

	return 0;
}

int tmc522x_set_chm_mode(const struct device *dev, bool classic_mode)
{
	uint32_t chopconf;
	int err;

	err = tmc522x_read(dev, TMC522X_REG_CHOPCONF, &chopconf);
	if (err != 0) {
		LOG_ERR("Failed to read CHOPCONF");
		return err;
	}

	if (classic_mode) {
		chopconf |= TMC522X_CHOPCONF_CHM_MASK;
		LOG_DBG("Switching to Classic chopper (CHM=1)");
	} else {
		chopconf &= ~TMC522X_CHOPCONF_CHM_MASK;
		LOG_DBG("Switching to SpreadCycle chopper (CHM=0)");
	}

	err = tmc522x_write(dev, TMC522X_REG_CHOPCONF, chopconf);
	if (err != 0) {
		LOG_ERR("Failed to write CHOPCONF");
		return err;
	}

	return 0;
}

int tmc522x_drven_enable(const struct device *dev)
{
	const struct tmc522x_mfd_config *config = dev->config;

	if (config->drven_gpio.port == NULL) {
		LOG_DBG("No DRVEN GPIO configured, skipping");
		return 0;
	}

	int err = gpio_pin_set_dt(&config->drven_gpio, 1);

	if (err != 0) {
		LOG_ERR("Failed to set DRVEN GPIO high");
		return err;
	}

	LOG_DBG("DRVEN pin set HIGH (driver enabled)");
	return 0;
}

int tmc522x_drven_disable(const struct device *dev)
{
	const struct tmc522x_mfd_config *config = dev->config;

	if (config->drven_gpio.port == NULL) {
		LOG_DBG("No DRVEN GPIO configured, skipping");
		return 0;
	}

	int err = gpio_pin_set_dt(&config->drven_gpio, 0);

	if (err != 0) {
		LOG_ERR("Failed to set DRVEN GPIO low");
		return err;
	}

	LOG_DBG("DRVEN pin set LOW (driver disabled - emergency stop)");
	return 0;
}

int tmc522x_get_actual_velocity(const struct device *dev, int32_t *velocity)
{
	uint32_t val;
	int err;

	if (velocity == NULL) {
		return -EINVAL;
	}

	err = tmc522x_read(dev, TMC522X_REG_VACTUAL, &val);
	if (err != 0) {
		LOG_ERR("Failed to read VACTUAL");
		return err;
	}

	/* VACTUAL is a 24-bit signed value, sign-extend to 32-bit */
	if (val & BIT(23)) {
		*velocity = (int32_t)(val | 0xFF000000);
	} else {
		*velocity = (int32_t)val;
	}

	return 0;
}

int tmc522x_get_actual_acceleration(const struct device *dev, int32_t *accel)
{
	uint32_t val;
	int err;

	if (accel == NULL) {
		return -EINVAL;
	}

	err = tmc522x_read(dev, TMC522X_REG_AACTUAL, &val);
	if (err != 0) {
		LOG_ERR("Failed to read AACTUAL");
		return err;
	}

	/* AACTUAL is a 24-bit signed value, sign-extend to 32-bit */
	if (val & BIT(23)) {
		*accel = (int32_t)(val | 0xFF000000);
	} else {
		*accel = (int32_t)val;
	}

	return 0;
}

int tmc522x_set_stealthchop_threshold(const struct device *dev, uint32_t velocity_pps)
{
	uint32_t tpwmthrs;

	if (velocity_pps == 0) {
		/* Disable StealthChop velocity switching */
		tpwmthrs = 0;
	} else {
		/*
		 * TPWMTHRS = fCLK / (velocity * 256)
		 * StealthChop2 active when TSTEP > TPWMTHRS
		 */
		uint32_t fclk = tmc522x_get_clock_frequency(dev);

		tpwmthrs = fclk / (velocity_pps * 256);
		if (tpwmthrs > TMC522X_VELOCITY_THRESH_MAX) {
			tpwmthrs = TMC522X_VELOCITY_THRESH_MAX;
		}
	}

	LOG_DBG("Setting TPWMTHRS = 0x%05X (velocity threshold: %u pps)", tpwmthrs, velocity_pps);
	return tmc522x_write(dev, TMC522X_REG_TPWMTHRS, tpwmthrs);
}

int tmc522x_set_coolstep_threshold(const struct device *dev, uint32_t velocity_pps)
{
	uint32_t tcoolthrs;

	if (velocity_pps == 0) {
		/* TCOOLTHRS = max: StallGuard/CoolStep active at all velocities */
		tcoolthrs = TMC522X_VELOCITY_THRESH_MAX;
	} else {
		/*
		 * TCOOLTHRS = fCLK / (velocity * 256)
		 * CoolStep/StallGuard active when TSTEP <= TCOOLTHRS
		 */
		uint32_t fclk = tmc522x_get_clock_frequency(dev);

		tcoolthrs = fclk / (velocity_pps * 256);
		if (tcoolthrs > TMC522X_VELOCITY_THRESH_MAX) {
			tcoolthrs = TMC522X_VELOCITY_THRESH_MAX;
		}
	}

	LOG_DBG("Setting TCOOLTHRS = 0x%05X (velocity threshold: %u pps)", tcoolthrs, velocity_pps);
	return tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, tcoolthrs);
}

int tmc522x_set_fullstep_threshold(const struct device *dev, uint32_t velocity_pps)
{
	uint32_t thigh;

	if (velocity_pps == 0) {
		/* Disable fullstep switching */
		thigh = 0;
	} else {
		/*
		 * THIGH = fCLK / (velocity * 256)
		 * Fullstep mode when TSTEP <= THIGH
		 */
		uint32_t fclk = tmc522x_get_clock_frequency(dev);

		thigh = fclk / (velocity_pps * 256);
		if (thigh > TMC522X_VELOCITY_THRESH_MAX) {
			thigh = TMC522X_VELOCITY_THRESH_MAX;
		}
	}

	LOG_DBG("Setting THIGH = 0x%05X (velocity threshold: %u pps)", thigh, velocity_pps);
	return tmc522x_write(dev, TMC522X_REG_THIGH, thigh);
}

int tmc522x_enable_stallguard_event(const struct device *dev, bool enable)
{
	uint32_t sw_mode;
	int err;

	err = tmc522x_read(dev, TMC522X_REG_SW_MODE, &sw_mode);
	if (err != 0) {
		LOG_ERR("Failed to read SW_MODE");
		return err;
	}

	if (enable) {
		sw_mode |= TMC522X_SW_MODE_SG_STOP_ENABLE;
		LOG_DBG("StallGuard stop-on-stall enabled (SG_STOP=1)");
	} else {
		sw_mode &= ~TMC522X_SW_MODE_SG_STOP_ENABLE;
		LOG_DBG("StallGuard stop-on-stall disabled (SG_STOP=0)");
	}

	return tmc522x_write(dev, TMC522X_REG_SW_MODE, sw_mode);
}

int tmc522x_set_stallguard2_threshold(const struct device *dev, int8_t sgt, bool sfilt)
{
	uint32_t coolconf;
	int err;

	if (sgt < TMC522X_SGT_MIN || sgt > TMC522X_SGT_MAX) {
		LOG_ERR("SGT out of range (%d to %d): %d", TMC522X_SGT_MIN, TMC522X_SGT_MAX, sgt);
		return -EINVAL;
	}

	/* Read-modify-write COOLCONF to preserve CoolStep fields (SEMIN, etc.) */
	err = tmc522x_read(dev, TMC522X_REG_COOLCONF, &coolconf);
	if (err != 0) {
		LOG_ERR("Failed to read COOLCONF");
		return err;
	}

	/* Clear SGT and SFILT fields, then set new values */
	coolconf &= ~(TMC522X_COOLCONF_SGT_MASK | TMC522X_COOLCONF_SFILT_MASK);
	coolconf |= ((uint32_t)(sgt & TMC522X_SGT_FIELD_MASK)) << TMC522X_COOLCONF_SGT_SHIFT;
	if (sfilt) {
		coolconf |= TMC522X_COOLCONF_SFILT_MASK;
	}

	err = tmc522x_write(dev, TMC522X_REG_COOLCONF, coolconf);
	if (err != 0) {
		LOG_ERR("Failed to write COOLCONF");
		return err;
	}

	LOG_DBG("StallGuard2 threshold set: SGT=%d, SFILT=%s", sgt, sfilt ? "on" : "off");

	return 0;
}

int tmc522x_set_stallguard4_threshold(const struct device *dev, uint16_t sg4_thrs, bool sg4_filt_en)
{
	uint32_t pwmconf;
	int err;

	if (sg4_thrs > TMC522X_SG4_THRS_MAX) {
		LOG_ERR("SG4_THRS out of range (0-%u): %u", TMC522X_SG4_THRS_MAX, sg4_thrs);
		return -EINVAL;
	}

	/* Write SG4_THRS register */
	err = tmc522x_write(dev, TMC522X_REG_SG4_THRS, sg4_thrs & TMC522X_SG4_THRS_MASK);
	if (err != 0) {
		LOG_ERR("Failed to write SG4_THRS");
		return err;
	}

	/* Read-modify-write PWMCONF to set/clear SG4_FILT_EN */
	err = tmc522x_read(dev, TMC522X_REG_PWMCONF, &pwmconf);
	if (err != 0) {
		LOG_ERR("Failed to read PWMCONF");
		return err;
	}

	if (sg4_filt_en) {
		pwmconf |= TMC522X_PWMCONF_SG4_FILT_EN_MASK;
	} else {
		pwmconf &= ~TMC522X_PWMCONF_SG4_FILT_EN_MASK;
	}

	err = tmc522x_write(dev, TMC522X_REG_PWMCONF, pwmconf);
	if (err != 0) {
		LOG_ERR("Failed to write PWMCONF");
		return err;
	}

	LOG_DBG("StallGuard4 threshold set: SG4_THRS=%u, SG4_FILT_EN=%s", sg4_thrs,
		sg4_filt_en ? "on" : "off");

	return 0;
}

int tmc522x_enable_stallguard(const struct device *dev, bool enable)
{
	uint32_t tcoolthrs;

	if (enable) {
		/* TCOOLTHRS = 0xFFFFF: StallGuard active at all velocities */
		tcoolthrs = TMC522X_STALLGUARD_ENABLED_ALL_SPEEDS;
		LOG_DBG("StallGuard enabled (TCOOLTHRS=0xFFFFF, active at all velocities)");
	} else {
		/* TCOOLTHRS = 0: StallGuard never active */
		tcoolthrs = TMC522X_STALLGUARD_DISABLED;
		LOG_DBG("StallGuard disabled (TCOOLTHRS=0)");
	}

	return tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, tcoolthrs);
}

int tmc522x_check_stallguard_status(const struct device *dev)
{
	uint32_t tcoolthrs, coolconf, sw_mode, drv_status, gconf;
	int err;

	err = tmc522x_read(dev, TMC522X_REG_GCONF, &gconf);
	if (err != 0) {
		return err;
	}

	err = tmc522x_read(dev, TMC522X_REG_TCOOLTHRS, &tcoolthrs);
	if (err != 0) {
		return err;
	}

	err = tmc522x_read(dev, TMC522X_REG_COOLCONF, &coolconf);
	if (err != 0) {
		return err;
	}

	err = tmc522x_read(dev, TMC522X_REG_SW_MODE, &sw_mode);
	if (err != 0) {
		return err;
	}

	err = tmc522x_read(dev, TMC522X_REG_DRV_STATUS, &drv_status);
	if (err != 0) {
		return err;
	}

	bool en_pwm = (gconf & TMC522X_GCONF_EN_PWM_MODE) != 0;
	bool sg_stop = (sw_mode & TMC522X_SW_MODE_SG_STOP_ENABLE) != 0;
	uint8_t cs_actual = TMC522X_GET_CS_ACTUAL(drv_status);
	bool sg_flag = (drv_status & TMC522X_DRV_STATUS_STALLGUARD) != 0;

	LOG_INF("=== StallGuard Status ===");
	LOG_INF("  Mode: %s", en_pwm ? "StealthChop2 (SG4)" : "SpreadCycle (SG2)");
	LOG_INF("  TCOOLTHRS: 0x%05X (%s)", tcoolthrs,
		tcoolthrs == TMC522X_STALLGUARD_DISABLED           ? "DISABLED"
		: tcoolthrs >= TMC522X_STALLGUARD_ENABLED_ALL_SPEEDS ? "active at all speeds"
								   : "velocity-dependent");
	LOG_INF("  SG_STOP: %s", sg_stop ? "enabled" : "disabled");
	LOG_INF("  STALLGUARD flag: %s", sg_flag ? "ACTIVE" : "inactive");
	LOG_INF("  CS_ACTUAL: %u", cs_actual);

	if (en_pwm) {
		/* StealthChop2 mode — show StallGuard4 info */
		uint32_t sg4_thrs_reg, sg4_result_reg, pwmconf;

		err = tmc522x_read(dev, TMC522X_REG_SG4_THRS, &sg4_thrs_reg);
		if (err != 0) {
			return err;
		}

		err = tmc522x_read(dev, TMC522X_REG_SG4_RESULT, &sg4_result_reg);
		if (err != 0) {
			return err;
		}

		err = tmc522x_read(dev, TMC522X_REG_PWMCONF, &pwmconf);
		if (err != 0) {
			return err;
		}

		uint16_t sg4_thrs = sg4_thrs_reg & TMC522X_SG4_THRS_MASK;
		uint16_t sg4_result = TMC522X_GET_SG4_RESULT(sg4_result_reg);
		bool sg4_filt = (pwmconf & TMC522X_PWMCONF_SG4_FILT_EN_MASK) != 0;

		LOG_INF("  SG4_THRS: %u", sg4_thrs);
		LOG_INF("  SG4_RESULT: %u (0=max load, %u=no load)", sg4_result,
			TMC522X_SG4_RESULT_MAX);
		LOG_INF("  SG4_FILT_EN: %s", sg4_filt ? "on (averaged)" : "off (raw)");
	} else {
		/* SpreadCycle mode — show StallGuard2 info */
		uint16_t sg_result = drv_status & TMC522X_DRV_STATUS_SG_RESULT_MASK;
		int8_t sgt = (int8_t)((coolconf & TMC522X_COOLCONF_SGT_MASK) >>
				      TMC522X_COOLCONF_SGT_SHIFT);

		/* Sign-extend SGT from 7-bit */
		if (sgt & TMC522X_SGT_SIGN_BIT) {
			sgt |= TMC522X_SGT_SIGN_EXTEND;
		}

		LOG_INF("  SGT threshold: %d", sgt);
		LOG_INF("  SG_RESULT: %u (0=max load, %u=no load)", sg_result,
			TMC522X_SG_RESULT_MAX);
		LOG_INF("  SFILT: %s", (coolconf & TMC522X_COOLCONF_SFILT_MASK) ? "on" : "off");
	}

	return 0;
}

int tmc522x_clear_stall(const struct device *dev)
{
	uint32_t ramp_stat;
	int err;

	/*
	 * Read RAMP_STAT first to check if EVENT_STOP_SG is set,
	 * then write EVENT_STOP_SG bit to clear it (write-1-to-clear).
	 */
	err = tmc522x_read(dev, TMC522X_REG_RAMP_STAT, &ramp_stat);
	if (err != 0) {
		LOG_ERR("Failed to read RAMP_STAT");
		return err;
	}

	if (ramp_stat & TMC522X_EVENT_STOP_SG_MASK) {
		LOG_DBG("Stall event pending (RAMP_STAT=0x%08X), clearing...", ramp_stat);
	} else {
		LOG_DBG("No stall event pending (RAMP_STAT=0x%08X)", ramp_stat);
		return 0;
	}

	/* Write-1-to-clear the EVENT_STOP_SG flag */
	err = tmc522x_write(dev, TMC522X_REG_RAMP_STAT, TMC522X_EVENT_STOP_SG_MASK);
	if (err != 0) {
		LOG_ERR("Failed to clear EVENT_STOP_SG");
		return err;
	}

	LOG_DBG("Stall event cleared");
	return 0;
}

int tmc522x_enable_coolstep_spreadcycle(const struct device *dev,
					const struct tmc522x_coolstep_config *cfg)
{
	uint32_t coolconf;
	uint32_t gconf;
	int err;

	/* Read GCONF to check current chopper mode */
	err = tmc522x_read(dev, TMC522X_REG_GCONF, &gconf);
	if (err != 0) {
		LOG_ERR("Failed to read GCONF register");
		return err;
	}

	if (cfg == NULL) {
		/* Disable CoolStep: clear COOLCONF and set TCOOLTHRS to disabled */
		LOG_DBG("Disabling CoolStep (SEMIN=0, TCOOLTHRS=0xFFFFF)");

		err = tmc522x_write(dev, TMC522X_REG_COOLCONF, 0);
		if (err != 0) {
			LOG_ERR("Failed to clear COOLCONF");
			return err;
		}

		return tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, TMC522X_COOLSTEP_DISABLED);
	}

	/* Verify SpreadCycle mode - CoolStep with SG2 requires EN_PWM_MODE=0 */
	if (gconf & TMC522X_GCONF_EN_PWM_MODE) {
		LOG_ERR("CoolStep/SG2 requires SpreadCycle mode, "
			"but StealthChop2 is active (GCONF=0x%08X). "
			"Use tmc522x_enable_coolstep_stealthchop2() for StealthChop2, "
			"or switch to SpreadCycle first.",
			gconf);
		return -EINVAL;
	}

	/* Validate parameters */
	if (cfg->semin > 15) {
		LOG_ERR("SEMIN out of range (0-15): %u", cfg->semin);
		return -EINVAL;
	}
	if (cfg->semax > 15) {
		LOG_ERR("SEMAX out of range (0-15): %u", cfg->semax);
		return -EINVAL;
	}
	if (cfg->seup > 3) {
		LOG_ERR("SEUP out of range (0-3): %u", cfg->seup);
		return -EINVAL;
	}
	if (cfg->sedn > 3) {
		LOG_ERR("SEDN out of range (0-3): %u", cfg->sedn);
		return -EINVAL;
	}
	if (cfg->sgt < TMC522X_SGT_MIN || cfg->sgt > TMC522X_SGT_MAX) {
		LOG_ERR("SGT out of range (%d to %d): %d", TMC522X_SGT_MIN, TMC522X_SGT_MAX,
			cfg->sgt);
		return -EINVAL;
	}

	/* Build COOLCONF register value */
	coolconf = 0;
	coolconf |= (cfg->semin & 0xF);                  /* SEMIN: bits 3:0 */
	coolconf |= ((uint32_t)(cfg->semax & 0xF)) << 4; /* SEMAX: bits 7:4 */
	coolconf |= ((uint32_t)(cfg->seup & 0x3)) << 8;  /* SEUP:  bits 9:8 */
	coolconf |= ((uint32_t)(cfg->sedn & 0x3)) << 13; /* SEDN:  bits 14:13 */
	if (cfg->seimin) {
		coolconf |= TMC522X_COOLCONF_SEIMIN_MASK; /* SEIMIN: bit 15 */
	}
	coolconf |= ((uint32_t)(cfg->sgt & TMC522X_SGT_FIELD_MASK))
		    << TMC522X_COOLCONF_SGT_SHIFT; /* SGT: bits 22:16 */
	if (cfg->sfilt) {
		coolconf |= TMC522X_COOLCONF_SFILT_MASK; /* SFILT: bit 24 */
	}

	err = tmc522x_write(dev, TMC522X_REG_COOLCONF, coolconf);
	if (err != 0) {
		LOG_ERR("Failed to write COOLCONF");
		return err;
	}

	/* Set TCOOLTHRS - CoolStep/StallGuard active when TSTEP <= TCOOLTHRS */
	err = tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, cfg->tcoolthrs);
	if (err != 0) {
		LOG_ERR("Failed to write TCOOLTHRS");
		return err;
	}

	LOG_DBG("CoolStep enabled: COOLCONF=0x%08X, TCOOLTHRS=0x%05X", coolconf, cfg->tcoolthrs);
	LOG_DBG("  SEMIN=%u, SEMAX=%u, SEUP=%u, SEDN=%u, SEIMIN=%u, SGT=%d, SFILT=%u", cfg->semin,
		cfg->semax, cfg->seup, cfg->sedn, cfg->seimin ? 1 : 0, cfg->sgt,
		cfg->sfilt ? 1 : 0);

	return 0;
}

int tmc522x_enable_coolstep_stealthchop2(const struct device *dev,
					 const struct tmc522x_coolstep_sg4_config *cfg)
{
	uint32_t coolconf;
	uint32_t gconf;
	int err;

	/* Read GCONF to check current chopper mode */
	err = tmc522x_read(dev, TMC522X_REG_GCONF, &gconf);
	if (err != 0) {
		LOG_ERR("Failed to read GCONF register");
		return err;
	}

	if (cfg == NULL) {
		/* Disable CoolStep: clear COOLCONF, TCOOLTHRS, and SG4_THRS */
		LOG_DBG("Disabling CoolStep/SG4 (SEMIN=0, TCOOLTHRS=0xFFFFF, SG4_THRS=0)");

		err = tmc522x_write(dev, TMC522X_REG_COOLCONF, 0);
		if (err != 0) {
			LOG_ERR("Failed to clear COOLCONF");
			return err;
		}

		err = tmc522x_write(dev, TMC522X_REG_SG4_THRS, 0);
		if (err != 0) {
			LOG_ERR("Failed to clear SG4_THRS");
			return err;
		}

		return tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, TMC522X_COOLSTEP_DISABLED);
	}

	/* Verify StealthChop2 mode - CoolStep with SG4 requires EN_PWM_MODE=1 */
	if (!(gconf & TMC522X_GCONF_EN_PWM_MODE)) {
		LOG_ERR("CoolStep/SG4 requires StealthChop2 mode, "
			"but SpreadCycle is active (GCONF=0x%08X). "
			"Use tmc522x_enable_coolstep_spreadcycle() for SpreadCycle, "
			"or switch to StealthChop2 first.",
			gconf);
		return -EINVAL;
	}

	/* Validate parameters */
	if (cfg->semin > 15) {
		LOG_ERR("SEMIN out of range (0-15): %u", cfg->semin);
		return -EINVAL;
	}
	if (cfg->semax > 15) {
		LOG_ERR("SEMAX out of range (0-15): %u", cfg->semax);
		return -EINVAL;
	}
	if (cfg->seup > 3) {
		LOG_ERR("SEUP out of range (0-3): %u", cfg->seup);
		return -EINVAL;
	}
	if (cfg->sedn > 3) {
		LOG_ERR("SEDN out of range (0-3): %u", cfg->sedn);
		return -EINVAL;
	}
	if (cfg->sg4_thrs > 511) {
		LOG_ERR("SG4_THRS out of range (0-511): %u", cfg->sg4_thrs);
		return -EINVAL;
	}

	/*
	 * Build COOLCONF register value.
	 * For StealthChop2/SG4 mode, SGT and SFILT fields are not used
	 * (StallGuard4 uses SG4_THRS register instead).
	 */
	coolconf = 0;
	coolconf |= (cfg->semin & 0xF);                  /* SEMIN: bits 3:0 */
	coolconf |= ((uint32_t)(cfg->semax & 0xF)) << 4; /* SEMAX: bits 7:4 */
	coolconf |= ((uint32_t)(cfg->seup & 0x3)) << 8;  /* SEUP:  bits 9:8 */
	coolconf |= ((uint32_t)(cfg->sedn & 0x3)) << 13; /* SEDN:  bits 14:13 */
	if (cfg->seimin) {
		coolconf |= TMC522X_COOLCONF_SEIMIN_MASK; /* SEIMIN: bit 15 */
	}

	err = tmc522x_write(dev, TMC522X_REG_COOLCONF, coolconf);
	if (err != 0) {
		LOG_ERR("Failed to write COOLCONF");
		return err;
	}

	/* Set SG4_THRS - stall detected when SG4_RESULT < SG4_THRS */
	err = tmc522x_write(dev, TMC522X_REG_SG4_THRS, cfg->sg4_thrs & TMC522X_SG4_THRS_MASK);
	if (err != 0) {
		LOG_ERR("Failed to write SG4_THRS");
		return err;
	}

	/* Set TCOOLTHRS - CoolStep/SG4 active when TSTEP <= TCOOLTHRS */
	err = tmc522x_write(dev, TMC522X_REG_TCOOLTHRS, cfg->tcoolthrs);
	if (err != 0) {
		LOG_ERR("Failed to write TCOOLTHRS");
		return err;
	}

	LOG_DBG("CoolStep/SG4 enabled: COOLCONF=0x%08X, TCOOLTHRS=0x%05X, SG4_THRS=%u", coolconf,
		cfg->tcoolthrs, cfg->sg4_thrs);
	LOG_DBG("  SEMIN=%u, SEMAX=%u, SEUP=%u, SEDN=%u, SEIMIN=%u", cfg->semin, cfg->semax,
		cfg->seup, cfg->sedn, cfg->seimin ? 1 : 0);

	return 0;
}

static int rampstat_read_clear(const struct device *dev, uint32_t *rampstat_value)
{
	int err;

	err = tmc522x_read(dev, TMC522X_REG_RAMP_STAT, rampstat_value);
	if (err == 0) {
		/* Write back to clear event bits (write-1-to-clear) */
		err = tmc522x_write(dev, TMC522X_REG_RAMP_STAT,
				    *rampstat_value & TMC522X_RAMP_STAT_EVENT_MASK);
	}
	return err;
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc522x_mfd_data *data =
		CONTAINER_OF(dwork, struct tmc522x_mfd_data, rampstat_callback_dwork);
	const struct device *dev = data->dev;
	const struct tmc522x_mfd_config *config = dev->config;
	uint32_t rampstat_value;
	int err;
	bool event_handled = false;

	__ASSERT_NO_MSG(dev);

	err = rampstat_read_clear(dev, &rampstat_value);
	if (err != 0) {
		LOG_ERR("Failed to read/clear RAMP_STAT: %d", err);
		return;
	}

	/*
	 * Dispatch events to the first motion controller child.
	 * TMC522x is single-axis, so there is at most one motion controller.
	 */
	if (config->num_motion_controllers > 0 && config->motion_controllers[0] != NULL) {
		const struct device *mc = config->motion_controllers[0];

		if (rampstat_value & TMC522X_RAMP_STAT_EVENT_STOP_L) {
			LOG_DBG("RAMPSTAT: Left end-stop event");
			tmc522x_stepper_trigger_cb(mc, STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED);
			event_handled = true;
		}

		if (rampstat_value & TMC522X_RAMP_STAT_EVENT_STOP_R) {
			LOG_DBG("RAMPSTAT: Right end-stop event");
			tmc522x_stepper_trigger_cb(mc, STEPPER_CTRL_EVENT_RIGHT_END_STOP_DETECTED);
			event_handled = true;
		}

		if (rampstat_value & TMC522X_EVENT_STOP_SG_MASK) {
			LOG_DBG("RAMPSTAT: StallGuard event");
			tmc522x_stepper_trigger_cb(mc, STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED);
			event_handled = true;
		}

		if (rampstat_value & TMC522X_EVENT_POS_REACH_MASK) {
			LOG_DBG("RAMPSTAT: Position reached event");
			tmc522x_stepper_trigger_cb(mc, STEPPER_CTRL_EVENT_STEPS_COMPLETED);
			event_handled = true;
		}
	}

	if (!event_handled) {
		/*
		 * No events detected — if both DIAG0 and DIAG1 interrupts are
		 * configured, all event sources are covered by hardware and we
		 * can stop polling (the next interrupt will re-trigger).
		 * Otherwise, reschedule the timer-based poll.
		 */
		if (config->diag0_gpio.port != NULL && config->diag1_gpio.port != NULL) {
			/* Fully interrupt-driven: wait for next DIAG edge */
			return;
		}

#ifdef CONFIG_STEPPER_ADI_TMC522X_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC522X_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif
	}
}

static void tmc522x_diag0_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
						gpio_port_pins_t pins)
{
	ARG_UNUSED(pins);

	LOG_DBG(">>> DIAG0 ISR fired!");

	struct tmc522x_mfd_data *data = CONTAINER_OF(cb, struct tmc522x_mfd_data, diag0_cb);

	/* Disable interrupt to prevent re-triggering while the event
	 * is being processed. It will be re-enabled when the next
	 * motion command calls tmc522x_reschedule_rampstat_work().
	 */
	gpio_pin_interrupt_configure(port, data->diag0_pin, GPIO_INT_DISABLE);

	k_work_reschedule(&data->rampstat_callback_dwork, K_NO_WAIT);
}

static void tmc522x_diag1_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
						gpio_port_pins_t pins)
{
	ARG_UNUSED(pins);

	LOG_DBG(">>> DIAG1 ISR fired!");

	struct tmc522x_mfd_data *data = CONTAINER_OF(cb, struct tmc522x_mfd_data, diag1_cb);

	/* Disable interrupt to prevent re-triggering while the event
	 * is being processed. It will be re-enabled when the next
	 * motion command calls tmc522x_reschedule_rampstat_work().
	 */
	gpio_pin_interrupt_configure(port, data->diag1_pin, GPIO_INT_DISABLE);

	k_work_reschedule(&data->rampstat_callback_dwork, K_NO_WAIT);
}

void tmc522x_reschedule_rampstat_work(const struct device *dev)
{
	struct tmc522x_mfd_data *data = dev->data;

	if (data == NULL) {
		return;
	}

	const struct tmc522x_mfd_config *config = dev->config;

	/*
	 * Clear any stale RAMP_STAT event bits before starting event
	 * monitoring. Without this, a leftover position_reached or
	 * event_stop_sg from a previous motion would be dispatched
	 * immediately on the first poll.
	 */
	uint32_t stale;

	(void)rampstat_read_clear(dev, &stale);
	if (stale & TMC522X_RAMP_STAT_EVENT_MASK) {
		LOG_DBG("Cleared stale RAMP_STAT events: 0x%08x", stale);
	}

	if (config->diag0_gpio.port != NULL && config->diag1_gpio.port != NULL) {
		/* Both DIAG GPIOs present — rely on interrupts.
		 * Re-enable the GPIO interrupts (they are disabled in the ISR
		 * after firing to prevent re-trigger floods).
		 */
		gpio_pin_interrupt_configure_dt(&config->diag0_gpio, GPIO_INT_EDGE_RISING);
		gpio_pin_interrupt_configure_dt(&config->diag1_gpio, GPIO_INT_EDGE_RISING);
		return;
	}

#ifdef CONFIG_STEPPER_ADI_TMC522X_RAMPSTAT_POLL_INTERVAL_IN_MSEC
	k_work_reschedule(&data->rampstat_callback_dwork,
			  K_MSEC(CONFIG_STEPPER_ADI_TMC522X_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif
}

static int tmc522x_mfd_init(const struct device *dev)
{
	struct tmc522x_mfd_data *data = dev->data;
	const struct tmc522x_mfd_config *config = dev->config;
	int err;

	LOG_DBG("Initializing TMC522x MFD parent controller %s", dev->name);

	k_sem_init(&data->sem, 1, 1);
	data->dev = dev;

	/* Configure hardware enable GPIOs (CRITICAL for operation) */
	if (config->drven_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->drven_gpio)) {
			LOG_ERR("DRVEN GPIO not ready");
			return -ENODEV;
		}
		err = gpio_pin_configure_dt(&config->drven_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure DRVEN GPIO");
			return err;
		}
		LOG_DBG("DRVEN GPIO configured and set HIGH");
	}

	if (config->sleepn_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->sleepn_gpio)) {
			LOG_ERR("SLEEPn GPIO not ready");
			return -ENODEV;
		}
		err = gpio_pin_configure_dt(&config->sleepn_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure SLEEPn GPIO");
			return err;
		}
		LOG_DBG("SLEEPn GPIO configured and set HIGH");
	}

	/* Log child devices */
	LOG_DBG("Num of motion controllers: %d", config->num_motion_controllers);
	LOG_DBG("Num of stepper drivers: %d", config->num_stepper_drivers);

	for (uint8_t i = 0; i < config->num_motion_controllers; i++) {
		if (config->motion_controllers[i] != NULL) {
			LOG_DBG("Motion controller child: %s", config->motion_controllers[i]->name);
		}
	}

	for (uint8_t i = 0; i < config->num_stepper_drivers; i++) {
		if (config->stepper_drivers[i] != NULL) {
			LOG_DBG("Stepper driver child: %s", config->stepper_drivers[i]->name);
		}
	}

	/* Initialize RAMP_STAT polling work item */
	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);

	/*
	 * Clear stale RAMP_STAT event bits BEFORE programming DIAG_CONF.
	 *
	 * On power-up or after previous tests, RAMP_STAT may contain latched
	 * event bits (e.g. event_pos_reached, event_stop_sg). When DIAG_CONF
	 * is programmed, these stale bits immediately assert the DIAG output
	 * pins LOW (active). If we then configure GPIO_INT_EDGE_FALLING, the
	 * falling edge has already occurred and the ISR will never fire.
	 *
	 * By clearing RAMP_STAT first, we ensure the DIAG pins start HIGH
	 * (inactive) after DIAG_CONF is written, so the next real event
	 * produces a clean falling edge.
	 *
	 * Read-clear twice: the datasheet notes that reading RAMP_STAT clears
	 * the event bits, but some bits (like position_reached) reflect live
	 * status and remain set. Two reads ensures all clearable bits are gone.
	 */
	{
		uint32_t stale1, stale2;

		(void)rampstat_read_clear(dev, &stale1);
		(void)rampstat_read_clear(dev, &stale2);
		LOG_DBG("Pre-DIAG_CONF RAMP_STAT clear: 0x%08x → 0x%08x", stale1, stale2);

		/*
		 * Also clear GSTAT — after power-up, GSTAT.reset is set (BIT 0),
		 * which the chip treats as a driver error. If ERROR is routed
		 * to a DIAG pin, this stale reset flag keeps the pin permanently
		 * asserted. Reading GSTAT clears these latched flags.
		 */
		uint32_t gstat;

		(void)tmc522x_read(dev, TMC522X_REG_GSTAT, &gstat);
		LOG_DBG("GSTAT cleared: 0x%08x (reset=%d, drv_err=%d)", gstat,
			(gstat & BIT(0)) ? 1 : 0, (gstat & BIT(1)) ? 1 : 0);
	}

	/*
	 * Program DIAG_CONF register to route events to DIAG0/DIAG1 pins.
	 * DIAG0: stall stop event
	 * DIAG1: position reached event
	 * Output mode: push-pull, active-high (NOD_PP set, INVPP clear).
	 * Pin idles LOW and goes HIGH when an event fires.
	 * GPIO is configured with GPIO_INT_EDGE_RISING to catch the
	 * LOW→HIGH transition.
	 *
	 * NOTE: Do NOT route ERROR to DIAG pins — GSTAT.reset is latched
	 * after power-up and would keep DIAG permanently asserted.
	 * Only configure event routing for pins that are actually wired.
	 */
	{
		uint32_t diag_conf = 0;

		if (config->diag0_gpio.port != NULL) {
			/* Push-pull, active-high output (no INVPP) */
			diag_conf |= TMC522X_DIAG0_NOD_PP_MASK; /* Push-pull output */
			/* Event routing — motion events only */
			diag_conf |= TMC522X_DIAG0_EV_STOP_SG_MASK; /* event_stop_sg on DIAG0 */
		}

		if (config->diag1_gpio.port != NULL) {
			/* Push-pull, active-high output (no INVPP) */
			diag_conf |= TMC522X_DIAG1_NOD_PP_MASK; /* Push-pull output */
			/* Event routing — motion events only */
			diag_conf |= TMC522X_DIAG1_EV_POS_REACHED_MASK; /* event_pos_reached */
		}

		if (diag_conf != 0) {
			err = tmc522x_write(dev, TMC522X_REG_DIAG_CONF, diag_conf);
			if (err < 0) {
				LOG_ERR("Failed to write DIAG_CONF: %d", err);
				return err;
			}
			LOG_DBG("DIAG_CONF programmed: 0x%08x", diag_conf);

			/*
			 * Clear RAMP_STAT again after DIAG_CONF is written.
			 * Programming DIAG_CONF may cause latched status bits
			 * to immediately assert the DIAG pins. Clear events
			 * now so pins return to HIGH (inactive) before we
			 * configure the edge-triggered GPIO interrupts.
			 */
			uint32_t post_clear1, post_clear2;

			(void)rampstat_read_clear(dev, &post_clear1);
			(void)rampstat_read_clear(dev, &post_clear2);
			LOG_DBG("Post-DIAG_CONF RAMP_STAT clear: 0x%08x → 0x%08x", post_clear1,
				post_clear2);
		}
	}

	/* Configure DIAG0 GPIO interrupt (optional — for interrupt-driven events) */
	if (config->diag0_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->diag0_gpio)) {
			LOG_ERR("DIAG0 GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->diag0_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Failed to configure DIAG0 GPIO: %d", err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->diag0_gpio, GPIO_INT_EDGE_RISING);
		if (err < 0) {
			LOG_ERR("Failed to configure DIAG0 interrupt: %d", err);
			return err;
		}

		gpio_init_callback(&data->diag0_cb, tmc522x_diag0_gpio_callback_handler,
				   BIT(config->diag0_gpio.pin));
		data->diag0_pin = config->diag0_gpio.pin;

		err = gpio_add_callback(config->diag0_gpio.port, &data->diag0_cb);
		if (err < 0) {
			LOG_ERR("Failed to add DIAG0 GPIO callback: %d", err);
			return err;
		}

		LOG_DBG("DIAG0 GPIO interrupt configured");
	} else {
		LOG_DBG("No DIAG0 GPIO — using timer-based RAMP_STAT polling for stall events");
	}

	/* Configure DIAG1 GPIO interrupt (optional — for position-reached events) */
	if (config->diag1_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->diag1_gpio)) {
			LOG_ERR("DIAG1 GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->diag1_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Failed to configure DIAG1 GPIO: %d", err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->diag1_gpio, GPIO_INT_EDGE_RISING);
		if (err < 0) {
			LOG_ERR("Failed to configure DIAG1 interrupt: %d", err);
			return err;
		}

		gpio_init_callback(&data->diag1_cb, tmc522x_diag1_gpio_callback_handler,
				   BIT(config->diag1_gpio.pin));
		data->diag1_pin = config->diag1_gpio.pin;

		err = gpio_add_callback(config->diag1_gpio.port, &data->diag1_cb);
		if (err < 0) {
			LOG_ERR("Failed to add DIAG1 GPIO callback: %d", err);
			return err;
		}

		LOG_DBG("DIAG1 GPIO interrupt configured");
	} else {
		LOG_DBG("No DIAG1 GPIO — polling RAMP_STAT for position events");
	}

	LOG_DBG("Device %s initialized", dev->name);
	return 0;
}

/* Helper macros for bus initialization */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define TMC522X_BUS_INIT_SPI(inst)                                                                 \
	.bus = {.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB |           \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8)))},   \
	.bus_io = &tmc522x_bus_io_spi,
#else
#define TMC522X_BUS_INIT_SPI(inst)
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#define TMC522X_BUS_INIT_I2C(inst)                                                                 \
	.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)}, .bus_io = &tmc522x_bus_io_i2c,
#else
#define TMC522X_BUS_INIT_I2C(inst)
#endif

#define TMC522X_CHILD_DEVICES_ARRAY(inst, compat)                                                  \
	{                                                                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, TMC522X_CHILD_DEVICE_GET, compat)    \
	}

#define TMC522X_CHILD_DEVICE_GET(node_id, compat)                                                  \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, compat), (DEVICE_DT_GET(node_id),), ())

#define TMC522X_DEFINE(inst)                                                                       \
	static const struct device *tmc522x_stepper_drivers_##inst[] =                             \
		TMC522X_CHILD_DEVICES_ARRAY(inst, adi_tmc522x_stepper_driver);                     \
	static const struct device *tmc522x_motion_controllers_##inst[] =                          \
		TMC522X_CHILD_DEVICES_ARRAY(inst, adi_tmc522x_stepper_ctrl);                       \
	static struct tmc522x_mfd_data tmc522x_data_##inst = {                                     \
		.dev = DEVICE_DT_GET(DT_DRV_INST(inst))};                                          \
	static const struct tmc522x_mfd_config tmc522x_config_##inst = {                           \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (TMC522X_BUS_INIT_SPI(inst)),              \
			    (TMC522X_BUS_INIT_I2C(inst)))                                          \
		    .clock_frequency = DT_INST_PROP(inst, clock_frequency),                       \
		    .stepper_drivers = tmc522x_stepper_drivers_##inst,                            \
		    .num_stepper_drivers = ARRAY_SIZE(tmc522x_stepper_drivers_##inst),            \
		    .motion_controllers = tmc522x_motion_controllers_##inst,                      \
		    .num_motion_controllers = ARRAY_SIZE(tmc522x_motion_controllers_##inst),      \
		    .drven_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drven_gpios, {0}),               \
		    .sleepn_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sleepn_gpios, {0}),             \
		    .diag0_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, diag0_gpios, {0}),               \
		    .diag1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, diag1_gpios, {0}),               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc522x_mfd_init, NULL, &tmc522x_data_##inst,                  \
			      &tmc522x_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC522X_DEFINE)

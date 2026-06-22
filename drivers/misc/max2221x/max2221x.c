/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max2221x_misc

#include <zephyr/drivers/mfd/max2221x.h>
#include <zephyr/drivers/misc/max2221x/max2221x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

struct misc_max2221x_data {
	uint16_t on_time_us[MAX2221X_NUM_CHANNELS];
	uint16_t off_time_us[MAX2221X_NUM_CHANNELS];
	bool stop_state[MAX2221X_NUM_CHANNELS];
	uint16_t repetitions[MAX2221X_NUM_CHANNELS];
};

struct misc_max2221x_config {
	const struct device *parent;
};

LOG_MODULE_REGISTER(max2221x, CONFIG_MISC_MAX2221X_LOG_LEVEL);

int max2221x_set_master_chop_freq(const struct device *dev, enum max2221x_master_chop_freq freq)
{
	const struct misc_max2221x_config *config = dev->config;

	if (freq >= MAX2221X_FREQ_INVALID) {
		LOG_ERR("Invalid master chopping frequency");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CTRL, MAX2221X_F_PWM_M_MASK,
				   freq);
}

int max2221x_get_master_chop_freq(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_GLOBAL_CTRL, &reg);
	if (ret) {
		return ret;
	}

	switch (FIELD_GET(MAX2221X_F_PWM_M_MASK, reg)) {
	case MAX2221X_FREQ_100KHZ:
		return 100000;
	case MAX2221X_FREQ_80KHZ:
		return 80000;
	case MAX2221X_FREQ_60KHZ:
		return 60000;
	case MAX2221X_FREQ_50KHZ:
		return 50000;
	case MAX2221X_FREQ_40KHZ:
		return 40000;
	case MAX2221X_FREQ_30KHZ:
		return 30000;
	case MAX2221X_FREQ_25KHZ:
		return 25000;
	case MAX2221X_FREQ_20KHZ:
		return 20000;
	case MAX2221X_FREQ_15KHZ:
		return 15000;
	case MAX2221X_FREQ_10KHZ:
		return 10000;
	case MAX2221X_FREQ_7500HZ:
		return 7500;
	case MAX2221X_FREQ_5000HZ:
		return 5000;
	case MAX2221X_FREQ_2500HZ:
		return 2500;
	default:
		LOG_ERR("Unknown master chopping frequency");
		return -EINVAL;
	}
}

int max2221x_get_channel_freq(const struct device *dev, uint8_t channel)
{
	int ret, master_chop_freq;
	uint16_t reg, master_freq_divisor;
	const struct misc_max2221x_config *config = dev->config;

	master_chop_freq = max2221x_get_master_chop_freq(dev);

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		LOG_ERR("Failed to read register for channel: %u", channel);
		return ret;
	}

	master_freq_divisor = FIELD_GET(MAX2221X_F_PWM_MASK, reg);

	switch (master_freq_divisor) {
	case MAX2221X_FREQ_M:
		return master_chop_freq;
	case MAX2221X_FREQ_M_2:
		return master_chop_freq / 2;
	case MAX2221X_FREQ_M_4:
		return master_chop_freq / 4;
	case MAX2221X_FREQ_M_8:
		return master_chop_freq / 8;
	default:
		LOG_ERR("Unknown channel frequency");
		return -EINVAL;
	}
}

int max2221x_set_part_state(const struct device *dev, bool enable)
{
	const struct misc_max2221x_config *config = dev->config;

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CFG, MAX2221X_ACTIVE_MASK,
				   enable ? 1 : 0);
}

int max2221x_set_channel_state(const struct device *dev, uint8_t channel, bool enable)
{
	const struct misc_max2221x_config *config = dev->config;

	if (!dev) {
		LOG_ERR("Device pointer is NULL");
		return -EINVAL;
	}

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CTRL,
				   MAX2221X_CNTL0_MASK << channel, enable ? 1 : 0);
}

int max2221x_mask_fault_pin(const struct device *dev, enum max2221x_fault_pin_masks mask)
{
	const struct misc_max2221x_config *config = dev->config;

	if (mask >= MAX2221X_FAULT_PIN_INVALID) {
		LOG_ERR("Invalid fault pin mask");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CFG,
				   MAX2221X_M_UVM_MASK << mask, 1);
}

int max2221x_unmask_fault_pin(const struct device *dev, enum max2221x_fault_pin_masks mask)
{
	const struct misc_max2221x_config *config = dev->config;

	if (mask >= MAX2221X_FAULT_PIN_INVALID) {
		LOG_ERR("Invalid fault pin mask");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CFG,
				   MAX2221X_M_UVM_MASK << mask, 0);
}

int max2221x_set_vdr_mode(const struct device *dev, enum max2221x_vdr_mode mode)
{
	const struct misc_max2221x_config *config = dev->config;

	if (mode >= MAX2221X_VDR_MODE_INVALID) {
		LOG_ERR("Invalid VDR mode");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_GLOBAL_CFG,
				   MAX2221X_VDRNVDRDUTY_MASK, mode);
}

int max2221x_get_vdr_mode(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_GLOBAL_CFG, &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_VDRNVDRDUTY_MASK, reg);
}

int max2221x_read_status(const struct device *dev, uint16_t *status)
{
	int ret;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_GLOBAL_CFG, status);
	if (ret) {
		return ret;
	}

	if (*status & MAX2221X_STATUS_STT3_MASK) {
		LOG_DBG("Channel 3: I_AC < I_AC_THLD");
	} else {
		LOG_DBG("Channel 3: I_AC > I_AC_THLD");
	}

	if (*status & MAX2221X_STATUS_STT2_MASK) {
		LOG_DBG("Channel 2: I_AC < I_AC_THLD");
	} else {
		LOG_DBG("Channel 2: I_AC > I_AC_THLD");
	}

	if (*status & MAX2221X_STATUS_STT1_MASK) {
		LOG_DBG("Channel 1: I_AC < I_AC_THLD");
	} else {
		LOG_DBG("Channel 1: I_AC > I_AC_THLD");
	}

	if (*status & MAX2221X_STATUS_STT0_MASK) {
		LOG_DBG("Channel 0: I_AC < I_AC_THLD");
	} else {
		LOG_DBG("Channel 0: I_AC > I_AC_THLD");
	}

	if (*status & MAX2221X_STATUS_MIN_T_ON_MASK) {
		LOG_DBG("MIN_T_ON not compliant");
	}

	if (*status & MAX2221X_STATUS_RES_MASK) {
		LOG_DBG("Measured resistance not compliant");
	}

	if (*status & MAX2221X_STATUS_IND_MASK) {
		LOG_DBG("Measured inductance not compliant");
	}

	if (*status & MAX2221X_STATUS_OVT_MASK) {
		LOG_DBG("Over-temperature detected");
	}

	if (*status & MAX2221X_STATUS_OCP_MASK) {
		LOG_DBG("Over-current detected");
	}

	if (*status & MAX2221X_STATUS_OLF_MASK) {
		LOG_DBG("Open-loop detected");
	}

	if (*status & MAX2221X_STATUS_HHF_MASK) {
		LOG_DBG("Hit current not reached");
	}

	if (*status & MAX2221X_STATUS_DPM_MASK) {
		LOG_DBG("Plunger did not move");
	}

	if (*status & MAX2221X_STATUS_COMER_MASK) {
		LOG_DBG("Communication error detected");
	}

	if (*status & MAX2221X_STATUS_UVM_MASK) {
		LOG_DBG("Under-voltage detected");
	}

	return 0;
}

int max2221x_read_vm_monitor(const struct device *dev, uint16_t *vm_monitor)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_VM_MONITOR, &reg);
	if (ret) {
		return ret;
	}

	*vm_monitor = FIELD_GET(MAX2221X_VM_MONITOR_MASK, reg);

	return 0;
}

int max2221x_set_dc_h2l(const struct device *dev, uint16_t dc_hdl)
{
	const struct misc_max2221x_config *config = dev->config;

	return max2221x_reg_write(config->parent, MAX2221X_REG_DC_H2L, dc_hdl);
}

int max2221x_get_dc_h2l(const struct device *dev, uint16_t *dc_hdl)
{
	const struct misc_max2221x_config *config = dev->config;

	return max2221x_reg_read(config->parent, MAX2221X_REG_DC_H2L, dc_hdl);
}

int max2221x_set_vm_upper_threshold(const struct device *dev, enum max2221x_vm_threshold threshold)
{
	const struct misc_max2221x_config *config = dev->config;

	if (threshold >= MAX2221X_VM_THRESHOLD_INVALID) {
		LOG_ERR("Invalid upper threshold");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_VM_THRESHOLD,
				   MAX2221X_VM_THLD_UP_MASK, threshold);
}

int max2221x_get_vm_upper_threshold(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_VM_THRESHOLD, &reg);
	if (ret) {
		return ret;
	}

	switch (FIELD_GET(MAX2221X_VM_THLD_UP_MASK, reg)) {
	case MAX2221X_VM_THRESHOLD_DISABLED:
		return 0;
	case MAX2221X_VM_THRESHOLD_4500MV:
		return 4500;
	case MAX2221X_VM_THRESHOLD_6500MV:
		return 6500;
	case MAX2221X_VM_THRESHOLD_8500MV:
		return 8500;
	case MAX2221X_VM_THRESHOLD_10500MV:
		return 10500;
	case MAX2221X_VM_THRESHOLD_12500MV:
		return 12500;
	case MAX2221X_VM_THRESHOLD_14500MV:
		return 14500;
	case MAX2221X_VM_THRESHOLD_16500MV:
		return 16500;
	case MAX2221X_VM_THRESHOLD_18500MV:
		return 18500;
	case MAX2221X_VM_THRESHOLD_20500MV:
		return 20500;
	case MAX2221X_VM_THRESHOLD_22500MV:
		return 22500;
	case MAX2221X_VM_THRESHOLD_24500MV:
		return 24500;
	case MAX2221X_VM_THRESHOLD_26500MV:
		return 26500;
	case MAX2221X_VM_THRESHOLD_28500MV:
		return 28500;
	case MAX2221X_VM_THRESHOLD_30500MV:
		return 30500;
	case MAX2221X_VM_THRESHOLD_32500MV:
		return 32500;
	default:
		LOG_ERR("Unknown VM upper threshold");
		return -EINVAL;
	}
}

int max2221x_set_vm_lower_threshold(const struct device *dev, enum max2221x_vm_threshold threshold)
{
	const struct misc_max2221x_config *config = dev->config;

	if (threshold >= MAX2221X_VM_THRESHOLD_INVALID) {
		LOG_ERR("Invalid lower threshold");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_VM_THRESHOLD,
				   MAX2221X_VM_THLD_DOWN_MASK, threshold);
}

int max2221x_get_vm_lower_threshold(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_VM_THRESHOLD, &reg);
	if (ret) {
		return ret;
	}

	switch (FIELD_GET(MAX2221X_VM_THLD_DOWN_MASK, reg)) {
	case MAX2221X_VM_THRESHOLD_DISABLED:
		return 0;
	case MAX2221X_VM_THRESHOLD_4500MV:
		return 4500;
	case MAX2221X_VM_THRESHOLD_6500MV:
		return 6500;
	case MAX2221X_VM_THRESHOLD_8500MV:
		return 8500;
	case MAX2221X_VM_THRESHOLD_10500MV:
		return 10500;
	case MAX2221X_VM_THRESHOLD_12500MV:
		return 12500;
	case MAX2221X_VM_THRESHOLD_14500MV:
		return 14500;
	case MAX2221X_VM_THRESHOLD_16500MV:
		return 16500;
	case MAX2221X_VM_THRESHOLD_18500MV:
		return 18500;
	case MAX2221X_VM_THRESHOLD_20500MV:
		return 20500;
	case MAX2221X_VM_THRESHOLD_22500MV:
		return 22500;
	case MAX2221X_VM_THRESHOLD_24500MV:
		return 24500;
	case MAX2221X_VM_THRESHOLD_26500MV:
		return 26500;
	case MAX2221X_VM_THRESHOLD_28500MV:
		return 28500;
	case MAX2221X_VM_THRESHOLD_30500MV:
		return 30500;
	case MAX2221X_VM_THRESHOLD_32500MV:
		return 32500;
	default:
		LOG_ERR("Unknown VM lower threshold");
		return -EINVAL;
	}
}

int max2221x_read_dc_l2h(const struct device *dev, uint16_t *dc_l2h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_read(config->parent, MAX2221X_REG_CFG_DC_L2H(channel), dc_l2h);
}

int max2221x_write_dc_l2h(const struct device *dev, uint16_t dc_l2h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_write(config->parent, MAX2221X_REG_CFG_DC_L2H(channel), dc_l2h);
}

int max2221x_read_dc_h(const struct device *dev, uint16_t *dc_h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_read(config->parent, MAX2221X_REG_CFG_DC_H(channel), dc_h);
}

int max2221x_write_dc_h(const struct device *dev, uint16_t dc_h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_write(config->parent, MAX2221X_REG_CFG_DC_H(channel), dc_h);
}

int max2221x_read_dc_l(const struct device *dev, uint16_t *dc_l, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_read(config->parent, MAX2221X_REG_CFG_DC_L(channel), dc_l);
}

int max2221x_write_dc_l(const struct device *dev, uint16_t dc_l, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_write(config->parent, MAX2221X_REG_CFG_DC_L(channel), dc_l);
}

int max2221x_read_time_l2h(const struct device *dev, uint16_t *time_l2h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_read(config->parent, MAX2221X_REG_CFG_TIME_L2H(channel), time_l2h);
}

int max2221x_write_time_l2h(const struct device *dev, uint16_t time_l2h, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_write(config->parent, MAX2221X_REG_CFG_TIME_L2H(channel), time_l2h);
}

int max2221x_set_ctrl_mode(const struct device *dev, enum max2221x_ctrl_mode mode, uint8_t channel)
{
	const struct misc_max2221x_config *config = dev->config;

	if (mode >= MAX2221X_CTRL_MODE_INVALID) {
		switch (channel) {
		case 0:
			LOG_ERR("Ch 0: Invalid control mode");
			break;
		case 1:
			LOG_ERR("Ch 1: Invalid control mode");
			break;
		case 2:
			LOG_ERR("Ch 2: Invalid control mode");
			break;
		case 3:
			LOG_ERR("Ch 3: Invalid control mode");
			break;
		default:
			LOG_ERR("Invalid channel");
			break;
		}

		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL0(channel),
				   MAX2221X_CTRL_MODE_MASK, mode);
}

int max2221x_get_ctrl_mode(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL0(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_CTRL_MODE_MASK, reg);
}

int max2221x_set_ramps(const struct device *dev, uint8_t channel, uint8_t ramp_mask, bool enable)
{
	const struct misc_max2221x_config *config = dev->config;
	int ret = 0;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (ramp_mask & MAX2221X_RAMP_DOWN_MASK) {
		ret = max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL0(channel),
					  MAX2221X_RDWE_MASK, enable ? 1 : 0);
		if (ret) {
			return ret;
		}
	}

	if (ramp_mask & MAX2221X_RAMP_MID_MASK) {
		ret = max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL0(channel),
					  MAX2221X_RMDE_MASK, enable ? 1 : 0);
		if (ret) {
			return ret;
		}
	}

	if (ramp_mask & MAX2221X_RAMP_UP_MASK) {
		ret = max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL0(channel),
					  MAX2221X_RUPE_MASK, enable ? 1 : 0);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int max2221x_set_ramp_slew_rate(const struct device *dev, uint8_t channel, uint8_t ramp_slew_rate)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL0(channel),
				   MAX2221X_RAMP_MASK, ramp_slew_rate);
}

int max2221x_get_ramp_slew_rate(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL0(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_RAMP_MASK, reg);
}

int max2221x_set_channel_chop_freq_div(const struct device *dev, uint8_t channel,
				       enum max2221x_ch_freq_div freq_div)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (freq_div >= MAX2221X_CH_FREQ_DIV_INVALID) {
		LOG_ERR("Invalid chopping frequency divider");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL1(channel),
				   MAX2221X_F_PWM_MASK, freq_div);
}

int max2221x_get_channel_chop_freq_div(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_F_PWM_MASK, reg);
}

int max2221x_set_slew_rate(const struct device *dev, uint8_t channel,
			   enum max2221x_slew_rate slew_rate)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (slew_rate >= MAX2221X_SLEW_RATE_INVALID) {
		LOG_ERR("Invalid slew rate");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL1(channel),
				   MAX2221X_SLEW_RATE_MASK, slew_rate);
}

int max2221x_get_slew_rate(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_SLEW_RATE_MASK, reg);
}

int max2221x_set_gain(const struct device *dev, uint8_t channel, enum max2221x_gain gain)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (gain >= MAX2221X_GAIN_INVALID) {
		LOG_ERR("Invalid gain");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL1(channel),
				   MAX2221X_GAIN_MASK, gain);
}

int max2221x_get_gain(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_GAIN_MASK, reg);
}

int max2221x_set_snsf(const struct device *dev, uint8_t channel, enum max2221x_snsf snsf)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (snsf >= MAX2221X_SNSF_INVALID) {
		LOG_ERR("Invalid SNSF");
		return -EINVAL;
	}

	return max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL1(channel),
				   MAX2221X_SNSF_MASK, snsf);
}

int max2221x_get_snsf(const struct device *dev, uint8_t channel)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		return ret;
	}

	return FIELD_GET(MAX2221X_SNSF_MASK, reg);
}

int max2221x_read_pwm_dutycycle(const struct device *dev, uint8_t channel, uint16_t *duty_cycle)
{
	const struct misc_max2221x_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_reg_read(config->parent, MAX2221X_REG_PWM_DUTY(channel), duty_cycle);
}

int max2221x_read_fault0(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_FAULT0, &reg);
	if (ret) {
		return ret;
	}

	if (reg & MAX2221X_FAULT_DPM3_MASK) {
		LOG_DBG("Channel 3: Plunger did not move");
	}

	if (reg & MAX2221X_FAULT_DPM2_MASK) {
		LOG_DBG("Channel 2: Plunger did not move");
	}

	if (reg & MAX2221X_FAULT_DPM1_MASK) {
		LOG_DBG("Channel 1: Plunger did not move");
	}

	if (reg & MAX2221X_FAULT_DPM0_MASK) {
		LOG_DBG("Channel 0: Plunger did not move");
	}

	if (reg & MAX2221X_FAULT_OLF3_MASK) {
		LOG_DBG("Channel 3: Open-loop detected");
	}

	if (reg & MAX2221X_FAULT_OLF2_MASK) {
		LOG_DBG("Channel 2: Open-loop detected");
	}

	if (reg & MAX2221X_FAULT_OLF1_MASK) {
		LOG_DBG("Channel 1: Open-loop detected");
	}

	if (reg & MAX2221X_FAULT_OLF0_MASK) {
		LOG_DBG("Channel 0: Open-loop detected");
	}

	if (reg & MAX2221X_FAULT_HHF3_MASK) {
		LOG_DBG("Channel 3: Hit current not reached");
	}

	if (reg & MAX2221X_FAULT_HHF2_MASK) {
		LOG_DBG("Channel 2: Hit current not reached");
	}

	if (reg & MAX2221X_FAULT_HHF1_MASK) {
		LOG_DBG("Channel 1: Hit current not reached");
	}

	if (reg & MAX2221X_FAULT_HHF0_MASK) {
		LOG_DBG("Channel 0: Hit current not reached");
	}

	if (reg & MAX2221X_FAULT_OCP3_MASK) {
		LOG_DBG("Channel 3: Over-current detected");
	}

	if (reg & MAX2221X_FAULT_OCP2_MASK) {
		LOG_DBG("Channel 2: Over-current detected");
	}

	if (reg & MAX2221X_FAULT_OCP1_MASK) {
		LOG_DBG("Channel 1: Over-current detected");
	}

	if (reg & MAX2221X_FAULT_OCP0_MASK) {
		LOG_DBG("Channel 0: Over-current detected");
	}

	return 0;
}

int max2221x_read_fault1(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct misc_max2221x_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_FAULT1, &reg);
	if (ret) {
		return ret;
	}

	if (reg & MAX2221X_FAULT_RES3_MASK) {
		LOG_DBG("Channel 3: Measured resistance not compliant");
	}

	if (reg & MAX2221X_FAULT_RES2_MASK) {
		LOG_DBG("Channel 2: Measured resistance not compliant");
	}

	if (reg & MAX2221X_FAULT_RES1_MASK) {
		LOG_DBG("Channel 1: Measured resistance not compliant");
	}

	if (reg & MAX2221X_FAULT_IND3_MASK) {
		LOG_DBG("Channel 3: Measured inductance not compliant");
	}

	if (reg & MAX2221X_FAULT_OVT_MASK) {
		LOG_DBG("Over-temperature detected");
	}

	if (reg & MAX2221X_FAULT_COMER_MASK) {
		LOG_DBG("Communication error detected");
	}

	if (reg & MAX2221X_FAULT_UVM_MASK) {
		LOG_DBG("Under-voltage detected");
	}

	if (reg & MAX2221X_FAULT_IND3_MASK) {
		LOG_DBG("Channel 3: Measured inductance not compliant");
	}

	if (reg & MAX2221X_FAULT_IND2_MASK) {
		LOG_DBG("Channel 2: Measured inductance not compliant");
	}

	if (reg & MAX2221X_FAULT_IND1_MASK) {
		LOG_DBG("Channel 1: Measured inductance not compliant");
	}

	if (reg & MAX2221X_FAULT_IND0_MASK) {
		LOG_DBG("Channel 0: Measured inductance not compliant");
	}

	return 0;
}

int max2221x_set_on_time(const struct device *dev, uint8_t channel, uint16_t on_time_us)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	data->on_time_us[channel] = on_time_us;

	return 0;
}

int max2221x_get_on_time(const struct device *dev, uint8_t channel)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return data->on_time_us[channel];
}

int max2221x_set_off_time(const struct device *dev, uint8_t channel, uint16_t off_time_us)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	data->off_time_us[channel] = off_time_us;

	return 0;
}

int max2221x_get_off_time(const struct device *dev, uint8_t channel)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return data->off_time_us[channel];
}

int max2221x_set_stop_state(const struct device *dev, uint8_t channel, bool stop_state)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	data->stop_state[channel] = stop_state;

	return 0;
}

int max2221x_get_stop_state(const struct device *dev, uint8_t channel)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return data->stop_state[channel];
}

int max2221x_set_repetitions(const struct device *dev, uint8_t channel, uint16_t repetitions)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	data->repetitions[channel] = repetitions;

	return 0;
}

int max2221x_get_repetitions(const struct device *dev, uint8_t channel)
{
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return data->repetitions[channel];
}

int max2221x_start_rapid_fire(const struct device *dev, uint8_t channel)
{
	int ret;
	struct misc_max2221x_data *data = dev->data;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	/* Safety check to prevent issues with zero repetitions */
	if (data->repetitions[channel] == 0) {
		LOG_WRN("Channel %u: Zero repetitions configured, setting to 1", channel);
		data->repetitions[channel] = 1;
	}

	for (int i = 0; i < data->repetitions[channel]; i++) {
		ret = max2221x_set_channel_state(dev, channel, true);
		if (ret) {
			return ret;
		}

		k_usleep(data->on_time_us[channel]);

		ret = max2221x_set_channel_state(dev, channel, false);
		if (ret) {
			return ret;
		}

		k_usleep(data->off_time_us[channel]);
	}

	if (!data->stop_state[channel]) {
		return max2221x_set_channel_state(dev, channel, true);
	}

	return 0;
}

int max2221x_stop_rapid_fire(const struct device *dev, uint8_t channel)
{
	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	return max2221x_set_channel_state(dev, channel, false);
}

static int misc_max2221x_init(const struct device *dev)
{
	const struct misc_max2221x_config *config = dev->config;

	LOG_DBG("Initialize MAX2221X Misc instance %s", dev->name);

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent device '%s' not ready", config->parent->name);
		return -EINVAL;
	}

	LOG_DBG("MAX2221X Misc Initialized");

	return 0;
}

#define MISC_MAX2221X_DEFINE(inst)                                                                 \
	static struct misc_max2221x_data misc_max2221x_data_##inst;                                \
	static const struct misc_max2221x_config misc_max2221x_config_##inst = {                   \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, misc_max2221x_init, NULL, &misc_max2221x_data_##inst,          \
			      &misc_max2221x_config_##inst, POST_KERNEL,                           \
			      CONFIG_MISC_MAX2221X_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MISC_MAX2221X_DEFINE);

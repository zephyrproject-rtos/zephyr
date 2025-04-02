/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for MAX2221X MISC driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_MAX2221X_MAX2221X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_MAX2221X_MAX2221X_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Master chopping frequency options */
enum max2221x_master_chop_freq {
	MAX2221X_FREQ_100KHZ = 0,
	MAX2221X_FREQ_80KHZ,
	MAX2221X_FREQ_60KHZ,
	MAX2221X_FREQ_50KHZ,
	MAX2221X_FREQ_40KHZ,
	MAX2221X_FREQ_30KHZ,
	MAX2221X_FREQ_25KHZ,
	MAX2221X_FREQ_20KHZ,
	MAX2221X_FREQ_15KHZ,
	MAX2221X_FREQ_10KHZ,
	MAX2221X_FREQ_7500HZ,
	MAX2221X_FREQ_5000HZ,
	MAX2221X_FREQ_2500HZ,

	MAX2221X_FREQ_INVALID,
};

/** @brief Individual channel chopping frequency divisor options */
enum max2221x_individual_chop_freq {
	MAX2221X_FREQ_M = 0,
	MAX2221X_FREQ_M_2,
	MAX2221X_FREQ_M_4,
	MAX2221X_FREQ_M_8,

	MAX2221X_FREQ_M_INVALID,
};

/** @brief Fault pin mask definitions */
enum max2221x_fault_pin_masks {
	MAX2221X_FAULT_PIN_UVM = 0, /**< Under Voltage Monitor fault */
	MAX2221X_FAULT_PIN_COMF,    /**< Communication Error fault */
	MAX2221X_FAULT_PIN_DPM,     /**< DPM Error fault */
	MAX2221X_FAULT_PIN_HHF,     /**< Hit current not reached error fault */
	MAX2221X_FAULT_PIN_OLF,     /**< Open-Load Fault */
	MAX2221X_FAULT_PIN_OCP,     /**< Over-Current Protection fault */
	MAX2221X_FAULT_PIN_OVT,     /**< Over-Temperature Protection fault */

	MAX2221X_FAULT_PIN_INVALID,
};

/** @brief VDR mode options */
enum max2221x_vdr_mode {
	MAX2221X_VDR_MODE_NORMAL = 0, /**< Normal Voltage mode */
	MAX2221X_VDR_MODE_DUTY,       /**< Duty Cycle mode */

	MAX2221X_VDR_MODE_INVALID,
};

/** @brief VM threshold voltage levels */
enum max2221x_vm_threshold {
	MAX2221X_VM_THRESHOLD_DISABLED = 0,
	MAX2221X_VM_THRESHOLD_4500MV,
	MAX2221X_VM_THRESHOLD_6500MV,
	MAX2221X_VM_THRESHOLD_8500MV,
	MAX2221X_VM_THRESHOLD_10500MV,
	MAX2221X_VM_THRESHOLD_12500MV,
	MAX2221X_VM_THRESHOLD_14500MV,
	MAX2221X_VM_THRESHOLD_16500MV,
	MAX2221X_VM_THRESHOLD_18500MV,
	MAX2221X_VM_THRESHOLD_20500MV,
	MAX2221X_VM_THRESHOLD_22500MV,
	MAX2221X_VM_THRESHOLD_24500MV,
	MAX2221X_VM_THRESHOLD_26500MV,
	MAX2221X_VM_THRESHOLD_28500MV,
	MAX2221X_VM_THRESHOLD_30500MV,
	MAX2221X_VM_THRESHOLD_32500MV,

	MAX2221X_VM_THRESHOLD_INVALID,
};

/** @brief Control mode options */
enum max2221x_ctrl_mode {
	MAX2221X_CTRL_MODE_VOLT = 0,
	MAX2221X_CTRL_MODE_CDR,
	MAX2221X_CTRL_MODE_LIMITER_VOLT,
	MAX2221X_CTRL_MODE_VOLT_CDR,

	MAX2221X_CTRL_MODE_INVALID,
};

/** @brief Channel frequency divisor options */
enum max2221x_ch_freq_div {
	MAX2221X_CH_FREQ_DIV_1 = 0,
	MAX2221X_CH_FREQ_DIV_2,
	MAX2221X_CH_FREQ_DIV_4,
	MAX2221X_CH_FREQ_DIV_8,

	MAX2221X_CH_FREQ_DIV_INVALID,
};

/** @brief Slew rate options */
enum max2221x_slew_rate {
	MAX2221X_SLEW_RATE_FAST = 0,
	MAX2221X_SLEW_RATE_400V_PER_US,
	MAX2221X_SLEW_RATE_200V_PER_US,
	MAX2221X_SLEW_RATE_100V_PER_US,

	MAX2221X_SLEW_RATE_INVALID,
};

/** @brief Gain scale options */
enum max2221x_gain {
	MAX2221X_SCALE_1 = 0,
	MAX2221X_SCALE_2,
	MAX2221X_SCALE_3,
	MAX2221X_SCALE_4,

	MAX2221X_GAIN_INVALID,
};

/** @brief Sense filter options */
enum max2221x_snsf {
	MAX2221X_SNSF_FULL_SCALE = 0,
	MAX2221X_SNSF_2_3,
	MAX2221X_SNSF_1_3,

	MAX2221X_SNSF_INVALID,
};

/**
 * @name MAX2221X Ramp Enable/Disable Masks
 * @{
 */
#define MAX2221X_RAMP_DOWN_MASK BIT(2)
#define MAX2221X_RAMP_MID_MASK  BIT(1)
#define MAX2221X_RAMP_UP_MASK   BIT(0)
/** @} */

/**
 * @brief Set the master chopping frequency for the MAX2221X device.
 *
 * Set master chop frequency to 50kHz
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_master_chop_freq(dev, MASTER_CHOP_FREQ_50KHZ);
 * if (ret)
 *     printk("Failed to set master chop frequency: %d\n", ret);
 * else
 *     printk("Master chop frequency set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param freq The desired master chopping frequency to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_master_chop_freq(const struct device *dev, enum max2221x_master_chop_freq freq);

/**
 * @brief Get the master chop frequency of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The master chop frequency in Hz on success, or a negative error code on failure.
 */
int max2221x_get_master_chop_freq(const struct device *dev);

/**
 * @brief Get the channel frequency of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to read.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_get_channel_freq(const struct device *dev, uint8_t channel);

/**
 * @brief Set the part state (enable/disable) of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable True to enable the part, false to disable.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_part_state(const struct device *dev, bool enable);

/**
 * @brief Set the channel state (enable/disable) of a channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the state for.
 * @param enable True to enable the channel, false to disable.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_channel_state(const struct device *dev, uint8_t channel, bool enable);

/**
 * @brief Mask detection of a fault condition on the MAX2221X device.
 *
 * Mask fault pin for overcurrent condition
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_mask_fault_pin(dev, MAX2221X_FAULT_PIN_OCP);
 * if (ret)
 *     printk("Failed to mask fault pin: %d\n", ret);
 * else
 *     printk("Fault pin masked successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask The fault condition to mask.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_mask_fault_pin(const struct device *dev, enum max2221x_fault_pin_masks mask);

/**
 * @brief Unmask detection of a fault condition on the MAX2221X device.
 *
 * Unmask fault pin for overcurrent condition
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_unmask_fault_pin(dev, MAX2221X_FAULT_PIN_OCP);
 * if (ret)
 *     printk("Failed to unmask fault pin: %d\n", ret);
 * else
 *     printk("Fault pin unmasked successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask The fault condition to unmask.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_unmask_fault_pin(const struct device *dev, enum max2221x_fault_pin_masks mask);

/**
 * @brief Set the VDR mode of the MAX2221X device.
 *
 * Set VDR mode to VDRDUTY
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_vdr_mode(dev, MAX2221X_VDR_MODE_DUTY);
 * if (ret)
 *     printk("Failed to set VDR mode: %d\n", ret);
 * else
 *     printk("VDR mode set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode The VDR mode to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_vdr_mode(const struct device *dev, enum max2221x_vdr_mode mode);

/**
 * @brief Get the VDR mode of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The VDR mode on success, or a negative error code on failure.
 */
int max2221x_get_vdr_mode(const struct device *dev);

/**
 * @brief Read the status of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param status Pointer to the variable to store the status.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_status(const struct device *dev, uint16_t *status);

/**
 * @brief Read the voltage monitor of the MAX2221X device.
 *
 * VM Measurement is calculated as:
 *
 * VM = KVM x vm_monitor
 * where KVM = 9.73 mV
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param vm_monitor Pointer to the variable to store the voltage monitor.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_vm_monitor(const struct device *dev, uint16_t *vm_monitor);

/**
 * @brief Set the demagnetization voltage of the MAX2221X device.
 *
 * If mode is MAX2221X_VDR_MODE_DUTY, dc_hdl represents a voltage. The voltage value is
 * calculated as VOUT (V) = KVDR x 36 x dc_hdl.
 * If mode is MAX2221X_VDR_MODE_NORMAL, dc_hdl represents a duty cycle. The voltage value is
 * calculated as VOUT (V) = KVDR x VM x dc_hdl.
 * Here, KVDR = 30.518 uV.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_hdl The desired demagnetization voltage to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_dc_h2l(const struct device *dev, uint16_t dc_hdl);

/**
 * @brief Get the demagnetization voltage of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_hdl Pointer to the variable to store the demagnetization voltage.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_get_dc_h2l(const struct device *dev, uint16_t *dc_hdl);

/**
 * @brief Set the upper threshold of the supply voltage monitor of the MAX2221X device.
 *
 * Set upper threshold to 3.3V
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_vm_upper_threshold(dev, MAX2221X_VM_THRESHOLD_3_3V);
 * if (ret)
 *     printk("Failed to set upper threshold: %d\n", ret);
 * else
 *     printk("Upper threshold set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param threshold The desired upper threshold to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_vm_upper_threshold(const struct device *dev, enum max2221x_vm_threshold threshold);

/**
 * @brief Get the upper threshold of the supply voltage monitor of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The upper threshold in millivolts on success, or a negative error code on failure.
 */
int max2221x_get_vm_upper_threshold(const struct device *dev);

/**
 * @brief Set the lower threshold of the supply voltage monitor of the MAX2221X device.
 *
 * Set lower threshold to 2.5V
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_vm_lower_threshold(dev, MAX2221X_VM_THRESHOLD_2_5V);
 * if (ret)
 *     printk("Failed to set lower threshold: %d\n", ret);
 * else
 *     printk("Lower threshold set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param threshold The desired lower threshold to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_vm_lower_threshold(const struct device *dev, enum max2221x_vm_threshold threshold);

/**
 * @brief Get the lower threshold of the supply voltage monitor of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return The lower threshold in millivolts on success, or a negative error code on failure.
 */
int max2221x_get_vm_lower_threshold(const struct device *dev);

/**
 * @brief Read the DC_L2H value of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_l2h Pointer to the variable to store the DC_L2H value.
 * @param channel The channel to read the DC_L2H value from.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_dc_l2h(const struct device *dev, uint16_t *dc_l2h, uint8_t channel);

/**
 * @brief Write the DC_L2H value of channel on the MAX2221X device.
 *
 * Sets the DC_L2H level:
 * VDR: VOUT (V) = KVDR x 36 x dc_l2h
 * VDRDUTY: VOUT (V) = KVDR x VM x dc_l2h
 * CDR: IOUT (mA) = KCDR x GAIN x SNSF x dc_l2h
 * where KVDR = 30.518 uV, KCDR = 1.017 mA (MAX22216) or 0.339 mA (MAX22217).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_l2h The desired DC_L2H value to write.
 * @param channel The channel to write the DC_L2H value to.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_write_dc_l2h(const struct device *dev, uint16_t dc_l2h, uint8_t channel);

/**
 * @brief Read the DC_H value of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_h Pointer to the variable to store the DC_H value.
 * @param channel The channel to read the DC_H value from.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_dc_h(const struct device *dev, uint16_t *dc_h, uint8_t channel);

/**
 * @brief Write the DC_H value of channel on the MAX2221X device.
 *
 * Sets the DC_H level:
 * VDR: VOUT (V) = KVDR x 36 x dc_h
 * VDRDUTY: VOUT (V) = KVDR x VM x dc_h
 * CDR: IOUT (mA) = KCDR x GAIN x SNSF x dc_h
 * where KVDR = 30.518 uV, KCDR = 1.017 mA (MAX22216) or 0.339 mA (MAX22217).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_h The desired DC_H value to write.
 * @param channel The channel to write the DC_H value to.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_write_dc_h(const struct device *dev, uint16_t dc_h, uint8_t channel);

/**
 * @brief Read the DC_L value of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_l Pointer to the variable to store the DC_L value.
 * @param channel The channel to read the DC_L value from.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_dc_l(const struct device *dev, uint16_t *dc_l, uint8_t channel);

/**
 * @brief Write the DC_L value of channel on the MAX2221X device.
 *
 * Sets the DC_L level:
 * VDR: VOUT (V) = KVDR x 36 x dc_l
 * VDRDUTY: VOUT (V) = KVDR x VM x dc_l
 * CDR: IOUT (mA) = KCDR x GAIN x SNSF x dc_l
 * where KVDR = 30.518 uV, KCDR = 1.017 mA (MAX22216) or 0.339 mA (MAX22217).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dc_l The desired DC_L value to write.
 * @param channel The channel to write the DC_L value to.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_write_dc_l(const struct device *dev, uint16_t dc_l, uint8_t channel);

/**
 * @brief Read the time L2H value of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param time_l2h Pointer to the variable to store the time L2H value.
 * @param channel The channel to read the time L2H value from.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_time_l2h(const struct device *dev, uint16_t *time_l2h, uint8_t channel);

/**
 * @brief Write the time L2H value of channel on the MAX2221X device.
 *
 * Sets the time L2H level:
 * TIME_L2H (s) = time_l2h / channel chopping frequency
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param time_l2h The desired time L2H value to write.
 * @param channel The channel to write the time L2H value to.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_write_time_l2h(const struct device *dev, uint16_t time_l2h, uint8_t channel);

/**
 * @brief Set the control mode of channel on the MAX2221X device.
 *
 * Set control mode for DC motor drive for channel 1
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_ctrl_mode(dev, MAX2221X_CTRL_MODE_LIMITER_VOLT, 1);
 * if (ret)
 *     printk("Failed to set control mode: %d\n", ret);
 * else
 *     printk("Control mode set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mode The desired control mode to set.
 * @param channel The channel to set the control mode for.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_ctrl_mode(const struct device *dev, enum max2221x_ctrl_mode mode, uint8_t channel);

/**
 * @brief Get the control mode of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the control mode for.
 *
 * @return The control mode on success, or a negative error code on failure.
 */
int max2221x_get_ctrl_mode(const struct device *dev, uint8_t channel);

/**
 * @brief Set ramps on channel of the MAX2221X device.
 *
 * This is the base function that enables or disables ramps based on the enable parameter.
 * Use max2221x_enable_ramps() or max2221x_disable_ramps() for specific enable/disable operations.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the ramps for.
 * @param ramp_mask The ramp mask indicating which ramps to modify.
 * @param enable True to enable the specified ramps, false to disable.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_ramps(const struct device *dev, uint8_t channel, uint8_t ramp_mask, bool enable);

/**
 * @brief Set the ramp slew rate of channel on the MAX2221X device.
 *
 * Sets the Ramp Slew Rate.
 * VDRDUTY:
 * Ramp Slew Rate (V/ms) = KVDR x VM x (ramp_slew_rate + 1) x F_PWM (kHz)
 * VDR:
 * Ramp Slew Rate (V/ms) = KVDR x 36 x (ramp_slew_rate + 1) x F_PWM (kHz)
 * CDR:
 * Ramp Slew Rate (mA/ms) = KCDR x GAIN x SNSF x (ramp_slew_rate + 1) x F_PWM (kHz)
 *
 * where KVDR = 30.518 uV, KCDR = 1.017 mA (MAX22216) or 0.339 mA (MAX22217), F_PWM is the channel
 * chopping frequency.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the ramp slew rate for.
 * @param ramp_slew_rate The desired ramp slew rate to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_ramp_slew_rate(const struct device *dev, uint8_t channel, uint8_t ramp_slew_rate);

/**
 * @brief Get the ramp slew rate of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the ramp slew rate for.
 *
 * @return The ramp slew rate on success, or a negative error code on failure.
 */
int max2221x_get_ramp_slew_rate(const struct device *dev, uint8_t channel);

/**
 * @brief Set the chopping frequency divider of channel on the MAX2221X device.
 *
 * Set channel chopping frequency divider to 1
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_channel_chop_freq_div(dev, channel, MAX2221X_CH_FREQ_DIV_1);
 * if (ret)
 *     printk("Failed to set channel chopping frequency divider: %d\n", ret);
 * else
 *     printk("Channel chopping frequency divider set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the chopping frequency divider for.
 * @param freq_div The desired chopping frequency divider to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_channel_chop_freq_div(const struct device *dev, uint8_t channel,
				       enum max2221x_ch_freq_div freq_div);

/**
 * @brief Get the chopping frequency divider of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the chopping frequency divider for.
 *
 * @return The chopping frequency divider on success, or a negative error code on failure.
 */
int max2221x_get_channel_chop_freq_div(const struct device *dev, uint8_t channel);

/**
 * @brief Set the slew rate of channel on the MAX2221X device.
 *
 * Set slew rate to 400V/us for channel 1
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_slew_rate(dev, 1, MAX2221X_SLEW_RATE_400V_PER_US);
 * if (ret)
 *     printk("Failed to set slew rate: %d\n", ret);
 * else
 *     printk("Slew rate set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the slew rate for.
 * @param slew_rate The desired slew rate to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_slew_rate(const struct device *dev, uint8_t channel,
			   enum max2221x_slew_rate slew_rate);

/**
 * @brief Get the slew rate of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the slew rate for.
 *
 * @return The slew rate on success, or a negative error code on failure.
 */
int max2221x_get_slew_rate(const struct device *dev, uint8_t channel);

/**
 * @brief Set the digital gain for the CDR of channel on the MAX2221X device.
 *
 * Set gain to 1 for channel 1
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_gain(dev, 1, MAX2221X_SCALE_1);
 * if (ret)
 *     printk("Failed to set gain: %d\n", ret);
 * else
 *     printk("Gain set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the gain for.
 * @param gain The desired gain to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_gain(const struct device *dev, uint8_t channel, enum max2221x_gain gain);

/**
 * @brief Get the digital gain for the CDR of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the gain for.
 *
 * @return The gain on success, or a negative error code on failure.
 */
int max2221x_get_gain(const struct device *dev, uint8_t channel);

/**
 * @brief Set the sense scaling factor of channel on the MAX2221X device.
 *
 * Set sense scaling factor 2/3 for channel 1
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret = max2221x_set_sense_scaling_factor(dev, 1, MAX2221X_SNSF_2_3);
 * if (ret)
 *     printk("Failed to set sense scaling factor: %d\n", ret);
 * else
 *     printk("Sense scaling factor set successfully\n");
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the sense scaling factor for.
 * @param snsf The desired sense scaling factor to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_snsf(const struct device *dev, uint8_t channel, enum max2221x_snsf snsf);

/**
 * @brief Get the sense scaling factor of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the sense scaling factor for.
 *
 * @return The sense scaling factor on success, or a negative error code on failure.
 */
int max2221x_get_snsf(const struct device *dev, uint8_t channel);

/**
 * @brief Read the PWM duty cycle of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to read the PWM duty cycle from.
 * @param duty_cycle Pointer to the variable to store the PWM duty cycle.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_pwm_dutycycle(const struct device *dev, uint8_t channel, uint16_t *duty_cycle);

/**
 * @brief Read the fault status from FAULT0 register of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_fault0(const struct device *dev);

/**
 * @brief Read the fault status from FAULT1 register of the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_read_fault1(const struct device *dev);

/**
 * @brief Set the on time of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the on time for.
 * @param on_time_us The desired on time to set in microseconds.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_on_time(const struct device *dev, uint8_t channel, uint16_t on_time_us);

/**
 * @brief Get the on time of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the on time for.
 *
 * @return The on time in microseconds on success, or a negative error code on failure.
 */
int max2221x_get_on_time(const struct device *dev, uint8_t channel);

/**
 * @brief Set the off time of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the off time for.
 * @param off_time_us The desired off time to set in microseconds.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_off_time(const struct device *dev, uint8_t channel, uint16_t off_time_us);

/**
 * @brief Get the off time of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the off time for.
 *
 * @return The off time in microseconds on success, or a negative error code on failure.
 */
int max2221x_get_off_time(const struct device *dev, uint8_t channel);

/**
 * @brief Set the stop state of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the stop state for.
 * @param stop_state The desired stop state to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_stop_state(const struct device *dev, uint8_t channel, bool stop_state);

/**
 * @brief Get the stop state of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the stop state for.
 *
 * @return The stop state on success, or a negative error code on failure.
 */
int max2221x_get_stop_state(const struct device *dev, uint8_t channel);

/**
 * @brief Set the repetitions of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to set the repetitions for.
 * @param repetitions The desired repetitions to set.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_set_repetitions(const struct device *dev, uint8_t channel, uint16_t repetitions);

/**
 * @brief Get the repetitions of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to get the repetitions for.
 *
 * @return The repetitions on success, or a negative error code on failure.
 */
int max2221x_get_repetitions(const struct device *dev, uint8_t channel);

/**
 * @brief Start the rapid fire mode of channel on the MAX2221X device.
 *
 * Set rapid fire parameters and start rapid fire mode. The example below demonstrates
 * how to configure the on time (1000 us), off time (1000 us), stop state (true), and
 * repetitions (10) before starting the rapid fire mode.
 *
 * @note Usually, set on time, off time, repetitions, and stop state before starting rapid fire
 * mode.
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/misc/max2221x/max2221x.h>
 *
 * int ret;
 * ret = max2221x_set_on_time(dev, channel, 1000);
 * if (ret) {
 *     printk("Failed to set on time: %d\n", ret);
 *     return ret;
 * }
 * ret = max2221x_set_off_time(dev, channel, 1000);
 * if (ret) {
 *     printk("Failed to set off time: %d\n", ret);
 *     return ret;
 * }
 * ret = max2221x_set_stop_state(dev, channel, true);
 * if (ret) {
 *     printk("Failed to set stop state: %d\n", ret);
 *     return ret;
 * }
 * ret = max2221x_set_repetitions(dev, channel, 10);
 * if (ret) {
 *     printk("Failed to set repetitions: %d\n", ret);
 *     return ret;
 * }
 * ret = max2221x_start_rapid_fire(dev, channel);
 * if (ret) {
 *     printk("Failed to start rapid fire mode: %d\n", ret);
 * } else {
 *     printk("Rapid fire mode started successfully\n");
 * }
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to start the rapid fire mode for.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on failure.
 */
int max2221x_start_rapid_fire(const struct device *dev, uint8_t channel);

/**
 * @brief Stop the rapid fire mode of channel on the MAX2221X device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param channel The channel to stop the rapid fire mode for.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int max2221x_stop_rapid_fire(const struct device *dev, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_MAX2221X_MAX2221X_H_ */

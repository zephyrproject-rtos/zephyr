/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42670.h"
#include "imu/inv_imu_apex.h"

int icm42670_apex_enable(inv_imu_device_t *s)
{
	int err = 0;
	inv_imu_apex_parameters_t apex_inputs;
	inv_imu_interrupt_parameter_t config_int = {(inv_imu_interrupt_value)0};

	/* Disabling FIFO to avoid extra power consumption due to ALP config */
	err |= inv_imu_configure_fifo(s, INV_IMU_FIFO_DISABLED);

	/* Enable Pedometer, Tilt and SMD interrupts */
	config_int.INV_STEP_DET = INV_IMU_ENABLE;
	config_int.INV_STEP_CNT_OVFL = INV_IMU_ENABLE;
	config_int.INV_TILT_DET = INV_IMU_ENABLE;
	config_int.INV_SMD = INV_IMU_ENABLE;
	err |= inv_imu_set_config_int1(s, &config_int);

	/* Enable accelerometer to feed the APEX Pedometer algorithm */
	err |= inv_imu_set_accel_frequency(s, ACCEL_CONFIG0_ODR_50_HZ);

	/* Set 2x averaging, in order to minimize power consumption (16x by default) */
	err |= inv_imu_set_accel_lp_avg(s, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);
	err |= inv_imu_enable_accel_low_power_mode(s);

	/* Get the default parameters for the APEX features */
	err |= inv_imu_apex_init_parameters_struct(s, &apex_inputs);

	/*
	 * Configure the power mode Normal mode.
	 *  Avalaible mode : Low Power mode (WoM+Pedometer),
	 *  configure the WoM to wake-up the DMP once it goes in power save mode
	 */
	apex_inputs.power_save = APEX_CONFIG0_DMP_POWER_SAVE_DIS;
	err |= inv_imu_apex_configure_parameters(s, &apex_inputs);

	/* Configure sampling frequency to 50Hz */
	err |= inv_imu_apex_set_frequency(s, APEX_CONFIG1_DMP_ODR_50Hz);

	return err;
}

int icm42670_apex_fetch_from_dmp(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	int rc = 0;
	uint8_t int_status2, int_status3;

	/* Read APEX interrupt status */
	rc |= inv_imu_read_reg(&data->driver, INT_STATUS2, 1, &int_status2);
	rc |= inv_imu_read_reg(&data->driver, INT_STATUS3, 1, &int_status3);

	/* Test Pedometer interrupt */
	if (int_status3 & (INT_STATUS3_STEP_DET_INT_MASK)) {
		inv_imu_apex_step_activity_t apex_pedometer;
		uint8_t step_cnt_ovflw = 0;

		if (int_status3 & INT_STATUS3_STEP_CNT_OVF_INT_MASK) {
			step_cnt_ovflw = 1;
		}

		rc |= inv_imu_apex_get_data_activity(&data->driver, &apex_pedometer);

		if (data->pedometer_cnt !=
		    apex_pedometer.step_cnt + step_cnt_ovflw * (uint64_t)UINT16_MAX) {
			data->pedometer_cnt =
				apex_pedometer.step_cnt + step_cnt_ovflw * (uint64_t)UINT16_MAX;
			data->pedometer_activity = apex_pedometer.activity_class;
			data->pedometer_cadence = apex_pedometer.step_cadence;
		} else {
			/* Pedometer data processing */
			rc = 1;
		}
	}
	/* Test Tilt interrupt */
	if (int_status3 & (INT_STATUS3_TILT_DET_INT_MASK)) {
		data->apex_status = ICM42670_APEX_STATUS_MASK_TILT;
	}
	/* Test SMD interrupt */
	if ((int_status2 & (INT_STATUS2_SMD_INT_MASK)) || (rc != 0)) {
		data->apex_status = ICM42670_APEX_STATUS_MASK_SMD;
	}
	/* Test WOM interrupts */
	if (int_status2 & (INT_STATUS2_WOM_X_INT_MASK | INT_STATUS2_WOM_Y_INT_MASK |
			   INT_STATUS2_WOM_Z_INT_MASK)) {
		data->apex_status = 0;
		if (int_status2 & INT_STATUS2_WOM_X_INT_MASK) {
			data->apex_status |= ICM42670_APEX_STATUS_MASK_WOM_X;
		}
		if (int_status2 & INT_STATUS2_WOM_Y_INT_MASK) {
			data->apex_status |= ICM42670_APEX_STATUS_MASK_WOM_Y;
		}
		if (int_status2 & INT_STATUS2_WOM_Z_INT_MASK) {
			data->apex_status |= ICM42670_APEX_STATUS_MASK_WOM_Z;
		}
	}
	return rc;
}

void icm42670_apex_pedometer_cadence_convert(struct sensor_value *val, uint8_t raw_val,
					     uint8_t dmp_odr_hz)
{
	int64_t conv_val;

	/* Converting u6.2 */
	conv_val = (int64_t)(dmp_odr_hz << 2) * 1000000 / (raw_val + (raw_val & 0x03));
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

int icm42670_apex_enable_pedometer(const struct device *dev, inv_imu_device_t *s)
{
	struct icm42670_data *data = dev->data;

	data->dmp_odr_hz = 50;
	/* Enable the pedometer */
	return inv_imu_apex_enable_pedometer(s);
}

int icm42670_apex_enable_tilt(inv_imu_device_t *s)
{
	/* Enable Tilt */
	return inv_imu_apex_enable_tilt(s);
}

int icm42670_apex_enable_smd(inv_imu_device_t *s)
{
	int rc = 0;

	/* Enable SMD (and Pedometer as SMD uses it) */
	rc |= inv_imu_apex_enable_pedometer(s);
	rc |= inv_imu_apex_enable_smd(s);

	return rc;
}

int icm42670_apex_enable_wom(inv_imu_device_t *s)
{
	int rc = 0;
	inv_imu_interrupt_parameter_t config_int = {(inv_imu_interrupt_value)0};

	/*
	 * Optimize power consumption:
	 * - Disable FIFO usage.
	 * - Disable data ready interrupt and enable WOM interrupts.
	 * - Set 2X averaging.
	 * - Use Low-Power mode at low frequency.
	 */
	rc |= inv_imu_configure_fifo(s, INV_IMU_FIFO_DISABLED);

	config_int.INV_WOM_X = INV_IMU_ENABLE;
	config_int.INV_WOM_Y = INV_IMU_ENABLE;
	config_int.INV_WOM_Z = INV_IMU_ENABLE;
	rc |= inv_imu_set_config_int1(s, &config_int);

	rc |= inv_imu_set_accel_lp_avg(s, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);
	rc |= inv_imu_set_accel_frequency(s, ACCEL_CONFIG0_ODR_12_5_HZ);
	rc |= inv_imu_enable_accel_low_power_mode(s);

	/*
	 * Configure WOM thresholds for each axis to 195 mg (Resolution 1g/256)
	 * WOM threshold = 50 * 1000 / 256 = 195 mg
	 * and enable WOM
	 */
	rc |= inv_imu_configure_wom(s, 50, 50, 50, WOM_CONFIG_WOM_INT_MODE_ORED,
				    WOM_CONFIG_WOM_INT_DUR_1_SMPL);
	rc |= inv_imu_enable_wom(s);

	return rc;
}

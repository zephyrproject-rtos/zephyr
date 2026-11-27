/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42x70.h"
#include "imu/inv_imu_apex.h"
#include "imu/inv_imu_tap.h"
//#define USE_POWER_SAVE

int icm42x70_apex_enable(inv_imu_device_t *s)
{
	int err = 0;
	inv_imu_apex_parameters_t apex_inputs;
	inv_imu_tap_parameters_t tap_inputs;
	inv_imu_interrupt_parameter_t config_int;
	inv_imu_tap_interrupt_parameter_t config_tap_int = {INV_IMU_DISABLE, INV_IMU_DISABLE,
							    INV_IMU_DISABLE};

	/* Disabling FIFO to avoid extra power consumption due to ALP config */
	err |= inv_imu_configure_fifo(s, INV_IMU_FIFO_DISABLED);
	err |= inv_imu_get_config_int1(s, &config_int);

	/* Enable Pedometer, Tilt and SMD interrupts */
	config_int.INV_STEP_DET = INV_IMU_ENABLE;
	config_int.INV_STEP_CNT_OVFL = INV_IMU_ENABLE;
	config_int.INV_TILT_DET = INV_IMU_ENABLE;
	config_int.INV_SMD = INV_IMU_ENABLE;
	err |= inv_imu_set_config_int1(s, &config_int);

	/* Enable Tap interrupts */
	config_tap_int.INV_SINGLE_TAP_DET = INV_IMU_ENABLE;
	config_tap_int.INV_DOUBLE_TAP_DET = INV_IMU_ENABLE;
	config_tap_int.INV_TRIPLE_TAP_DET = INV_IMU_ENABLE;
	err |= inv_imu_tap_set_config_int1(s, &config_tap_int);

	/* Enable accelerometer to feed the APEX Pedometer algorithm */
	err |= inv_imu_set_accel_frequency(s, ACCEL_CONFIG0_ODR_400_HZ);
	err |= inv_imu_apex_set_frequency(s, APEX_CONFIG1_DMP_ODR_400Hz);

	/* Set 2x averaging, in order to minimize power consumption (16x by default) */
	err |= inv_imu_set_accel_lp_avg(s, ACCEL_CONFIG1_ACCEL_FILT_AVG_2);

	/* Get the default parameters for the APEX features */
	err |= inv_imu_apex_init_parameters_struct(s, &apex_inputs);

	/* Disable TAP before configuring it */
	err |= inv_imu_tap_disable(s);
	err |= inv_imu_reset_dmp(s, APEX_CONFIG0_DMP_MEM_RESET_APEX_ST_EN);

	/* Set APEX parameters */
	err |= inv_imu_tap_init_parameters_struct(s, &tap_inputs);

#ifdef USE_POWER_SAVE
	tap_inputs.power_save = APEX_CONFIG0_DMP_POWER_SAVE_EN;
	tap_inputs.power_save_time = APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_4_S;
	/* Configure and enable WOM to wake-up the DMP once it goes in power save mode */
	err |= inv_imu_configure_wom(s, WOM_THRESHOLD, WOM_THRESHOLD, WOM_THRESHOLD,
			WOM_CONFIG_WOM_INT_MODE_ANDED, WOM_CONFIG_WOM_INT_DUR_1_SMPL);
	err |= inv_imu_enable_wom(s);
#else
	tap_inputs.power_save =  APEX_CONFIG0_DMP_POWER_SAVE_DIS;
#endif

	/* Load TAP parameters. Make sure to call this function before inv_imu_tap_load_dmp() */
	err |= inv_imu_tap_configure_parameters(s, &tap_inputs);

	/* Loads DMP image to SRAM */
	err |= inv_imu_tap_load_dmp(s);

	/*
	 * Configure the power mode Normal mode.
	 *  Available mode : Low Power mode (WoM+Pedometer),
	 *  configure the WoM to wake-up the DMP once it goes in power save mode
	 */
	apex_inputs.power_save = APEX_CONFIG0_DMP_POWER_SAVE_DIS;
	err |= inv_imu_apex_configure_parameters(s, &apex_inputs);
	err |= inv_imu_enable_accel_low_power_mode(s);

	return err;
}

int icm42x70_apex_fetch_from_dmp(const struct device *dev)
{
	struct icm42x70_data *data = dev->data;
	int rc = 0;

	/* Test Pedometer interrupt */
	if (data->int_status3 & (INT_STATUS3_STEP_DET_INT_MASK)) {
		inv_imu_apex_step_activity_t apex_pedometer;
		uint8_t step_cnt_ovflw = 0;

		if (data->int_status3 & INT_STATUS3_STEP_CNT_OVF_INT_MASK) {
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
	if (data->int_status3 & (INT_STATUS3_TILT_DET_INT_MASK)) {
		data->apex_status = ICM42X70_APEX_STATUS_MASK_TILT;
	}
	/* Test SMD interrupt */
	if ((data->int_status2 & (INT_STATUS2_SMD_INT_MASK)) || (rc != 0)) {
		data->apex_status = ICM42X70_APEX_STATUS_MASK_SMD;
	}
	/* Test WOM interrupts */
	if (data->int_status2 & (INT_STATUS2_WOM_X_INT_MASK | INT_STATUS2_WOM_Y_INT_MASK |
			   INT_STATUS2_WOM_Z_INT_MASK)) {
		data->apex_status = 0;
		if (data->int_status2 & INT_STATUS2_WOM_X_INT_MASK) {
			data->apex_status |= ICM42X70_APEX_STATUS_MASK_WOM_X;
		}
		if (data->int_status2 & INT_STATUS2_WOM_Y_INT_MASK) {
			data->apex_status |= ICM42X70_APEX_STATUS_MASK_WOM_Y;
		}
		if (data->int_status2 & INT_STATUS2_WOM_Z_INT_MASK) {
			data->apex_status |= ICM42X70_APEX_STATUS_MASK_WOM_Z;
		}
	}
	if ((data->int_status3 & (INT_STATUS3_SINGLE_TAP_INT_MASK)) || (rc != 0)) {
		inv_imu_tap_get_data(&data->driver, &data->inv_imu_tap_info);
		data->apex_status = ICM42X70_APEX_STATUS_SINGLE_TAP;
	}
	if ((data->int_status3 & (INT_STATUS3_DOUBLE_TAP_INT_MASK)) || (rc != 0)) {
		inv_imu_tap_get_data(&data->driver, &data->inv_imu_tap_info);
		data->apex_status = ICM42X70_APEX_STATUS_DOUBLE_TAP;
	}
	if ((data->int_status3 & (INT_STATUS3_TRIPLE_TAP_INT_MASK)) || (rc != 0)) {
		inv_imu_tap_get_data(&data->driver, &data->inv_imu_tap_info);
		data->apex_status = ICM42X70_APEX_STATUS_TRIPLE_TAP;
	}

	return rc;
}

void icm42x70_apex_pedometer_cadence_convert(struct sensor_value *val, uint8_t raw_val,
					     uint16_t dmp_odr_hz)
{
	int64_t conv_val;

	/*
	 * raw_val is the number of samples between two steps, in u6.2 format:
	 * cadence (steps/s) = dmp_odr_hz / (raw_val / 4)
	 */
	if (raw_val == 0) {
		val->val1 = 0;
		val->val2 = 0;
		return;
	}

	conv_val = (int64_t)dmp_odr_hz * 4 * 1000000 / raw_val;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

int icm42x70_apex_enable_pedometer(const struct device *dev, inv_imu_device_t *s)
{
	struct icm42x70_data *data = dev->data;

	data->dmp_odr_hz = 50;
	/* Enable the pedometer */
	return inv_imu_apex_enable_pedometer(s);
}

int icm42x70_apex_enable_tilt(inv_imu_device_t *s)
{
	/* Enable Tilt */
	return inv_imu_apex_enable_tilt(s);
}

int icm42x70_apex_enable_smd(inv_imu_device_t *s)
{
	int rc = 0;

	/* Enable SMD (and Pedometer as SMD uses it) */
	rc |= inv_imu_apex_enable_pedometer(s);
	rc |= inv_imu_apex_enable_smd(s);

	return rc;
}

int icm42x70_apex_enable_wom(inv_imu_device_t *s)
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

int icm42x70_apex_enable_tap(const struct device *dev, inv_imu_device_t *s)
{
	int rc = 0;
	struct icm42x70_data *data = dev->data;
	/* Enable TAP */
	rc |= inv_imu_tap_enable(s);
	data->dmp_odr_hz = 400;

	return rc;
}

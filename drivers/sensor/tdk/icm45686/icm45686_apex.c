/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "icm45686.h"
#include "icm456xx_h/imu/inv_imu.h"
#include "imu/inv_imu_driver.h"
#include "imu/inv_imu_edmp.h"
#include "imu/inv_imu_driver_advanced.h"

int icm45686_apex_enable(inv_imu_device_t *s)
{
	int rc = 0;
	inv_imu_int_state_t int_config;
	inv_imu_edmp_apex_parameters_t apex_parameters;
	inv_imu_edmp_int_state_t apex_int_config;

	rc |= icm456xx_get_config_int(s, INV_IMU_INT1, &int_config);
	int_config.INV_FIFO_THS = INV_IMU_DISABLE;
	int_config.INV_UI_DRDY = INV_IMU_DISABLE;
	int_config.INV_EDMP_EVENT = INV_IMU_ENABLE;
	rc |= icm456xx_set_config_int(s, INV_IMU_INT1, &int_config);

	/* Set EDMP ODR */
	rc |= icm456xx_edmp_set_frequency(s, DMP_EXT_SEN_ODR_CFG_APEX_ODR_50_HZ);

	/* Set ODR */
	rc |= icm456xx_set_accel_frequency(s, ACCEL_CONFIG0_ACCEL_ODR_50_HZ);

	/* Set BW = ODR/4 */
	rc |= icm456xx_set_accel_ln_bw(s, IPREG_SYS2_REG_131_ACCEL_UI_LPFBW_DIV_4);

	/* Select WUOSC clock to have accel in ULP (lowest power mode) */
	rc |= icm456xx_select_accel_lp_clk(s, SMC_CONTROL_0_ACCEL_LP_CLK_WUOSC);

	/* Set AVG to 1x */
	rc |= icm456xx_set_accel_lp_avg(s, IPREG_SYS2_REG_129_ACCEL_LP_AVG_1);

	/* Ensure all DMP features are disabled before running init procedure */
	rc |= icm456xx_edmp_disable_pedometer(s);
	rc |= icm456xx_edmp_disable_tilt(s);
	rc |= icm456xx_edmp_disable_tap(s);
	rc |= icm456xx_edmp_disable(s);

	/* Request DMP to re-initialize APEX */
	rc |= icm456xx_edmp_recompute_apex_decimation(s);

	/* Configure APEX parameters */
	rc |= icm456xx_edmp_get_apex_parameters(s, &apex_parameters);
	apex_parameters.power_save_en = INV_IMU_DISABLE;
	rc |= icm456xx_adv_disable_wom(s);
	rc |= icm456xx_edmp_set_apex_parameters(s, &apex_parameters);

	/* Set accel in low-power mode if ODR slower than 800 Hz, otherwise in low-noise mode */
	rc |= icm456xx_set_accel_mode(s, PWR_MGMT0_ACCEL_MODE_LP);

	/* Wait for accel startup time */
	k_msleep(10);

	/* Disable all APEX interrupt and enable only the one we need */
	memset(&apex_int_config, INV_IMU_DISABLE, sizeof(apex_int_config));

	/* Apply interrupt configuration */
	rc |= icm456xx_edmp_set_config_int_apex(s, &apex_int_config);

	return rc;
}

int icm45686_apex_fetch_from_dmp(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	int rc = 0;
	inv_imu_int_state_t int_state;
	inv_imu_edmp_int_state_t apex_state = {0};

	/* Read APEX interrupt status */
	rc |= icm456xx_get_int_status(&data->driver, INV_IMU_INT1, &int_state);

	/* Test Pedometer interrupt */
	if (int_state.INV_EDMP_EVENT) {
		uint8_t step_cnt_ovflw = 0;
		/* Read APEX interrupt status */
		rc |= icm456xx_edmp_get_int_apex_status(&data->driver, &apex_state);

		/* Pedometer */
		if (apex_state.INV_STEP_CNT_OVFL) {
			step_cnt_ovflw = 1;
		}

		if (apex_state.INV_STEP_DET) {
			inv_imu_edmp_pedometer_data_t ped_data;

			rc |= icm456xx_edmp_get_pedometer_data(&data->driver, &ped_data);
			if (rc == INV_IMU_OK) {
				data->pedometer_cnt = (unsigned long)ped_data.step_cnt +
						      (step_cnt_ovflw * UINT16_MAX);
				data->pedometer_activity = ped_data.activity_class;
				data->pedometer_cadence = ped_data.step_cadence;
			}
		}

		/* SMD */
		if (apex_state.INV_SMD) {
			data->apex_status = ICM45686_APEX_STATUS_MASK_SMD;
		}

		/* Tilt */
		if (apex_state.INV_TILT_DET) {
			data->apex_status = ICM45686_APEX_STATUS_MASK_TILT;
		}

		/* Tap & Double Tap */
		if (apex_state.INV_TAP) {
			inv_imu_edmp_tap_data_t tap_data;

			icm456xx_edmp_get_tap_data(&data->driver, &tap_data);
			if (tap_data.double_tap_timing == 0) {
				data->apex_status = ICM45686_APEX_STATUS_MASK_TAP;
			} else {
				data->apex_status = ICM45686_APEX_STATUS_MASK_DOUBLE_TAP;
			}
		}
	}

	return rc;
}

void icm45686_apex_pedometer_cadence_convert(struct sensor_value *val, uint8_t raw_val,
					     uint8_t dmp_odr_hz)
{
	int64_t conv_val;

	/* Converting u6.2 */
	conv_val = (int64_t)(dmp_odr_hz << 2) * 1000000 / (raw_val + (raw_val & 0x03));
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

int icm45686_apex_enable_pedometer(const struct device *dev, inv_imu_device_t *s)
{

	struct icm45686_data *data = dev->data;
	int rc = 0;
	inv_imu_edmp_int_state_t apex_int_config;

	data->dmp_odr_hz = 50;

	rc = icm456xx_edmp_get_config_int_apex(s, &apex_int_config);
	apex_int_config.INV_STEP_CNT_OVFL = INV_IMU_ENABLE;
	apex_int_config.INV_STEP_DET = INV_IMU_ENABLE;
	/* Apply interrupt configuration */
	rc |= icm456xx_edmp_set_config_int_apex(s, &apex_int_config);
	rc |= icm456xx_edmp_enable_pedometer(s);
	/* Enable EDMP if at least one feature is enabled */
	rc |= icm456xx_edmp_enable(s);
	return rc;
}

int icm45686_apex_enable_tilt(inv_imu_device_t *s)
{
	int rc = 0;
	inv_imu_edmp_int_state_t apex_int_config;

	rc = icm456xx_edmp_get_config_int_apex(s, &apex_int_config);
	apex_int_config.INV_TILT_DET = INV_IMU_ENABLE;
	/* Apply interrupt configuration */
	rc |= icm456xx_edmp_set_config_int_apex(s, &apex_int_config);
	rc |= icm456xx_edmp_enable_tilt(s);
	/* Enable EDMP if at least one feature is enabled */
	rc |= icm456xx_edmp_enable(s);
	return rc;
}

int icm45686_apex_enable_smd(inv_imu_device_t *s)
{
	int rc = 0;
	inv_imu_edmp_int_state_t apex_int_config;

	rc = icm456xx_edmp_get_config_int_apex(s, &apex_int_config);
	apex_int_config.INV_SMD = INV_IMU_ENABLE;
	/* Apply interrupt configuration */
	rc |= icm456xx_edmp_set_config_int_apex(s, &apex_int_config);
	rc |= icm456xx_edmp_enable_smd(s);
	/* Enable EDMP if at least one feature is enabled */
	rc |= icm456xx_edmp_enable(s);

	return rc;
}

int icm45686_apex_enable_wom(inv_imu_device_t *s)
{
	int rc = 0;

	rc |= icm456xx_adv_configure_wom(s, DEFAULT_WOM_THS_MG, DEFAULT_WOM_THS_MG,
					 DEFAULT_WOM_THS_MG, TMST_WOM_CONFIG_WOM_INT_MODE_ANDED,
					 TMST_WOM_CONFIG_WOM_INT_DUR_1_SMPL);
	rc |= icm456xx_adv_enable_wom(s);

	return rc;
}

int icm45686_apex_enable_tap(inv_imu_device_t *s)
{
	int rc = 0;
	inv_imu_edmp_int_state_t apex_int_config;

	rc = icm456xx_edmp_get_config_int_apex(s, &apex_int_config);
	apex_int_config.INV_TAP = INV_IMU_ENABLE;
	/* Apply interrupt configuration */
	rc |= icm456xx_edmp_set_config_int_apex(s, &apex_int_config);
	rc |= icm456xx_edmp_enable_tap(s);
	/* Enable EDMP if at least one feature is enabled */
	rc |= icm456xx_edmp_enable(s);
	return rc;
}

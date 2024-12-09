/*
 * Copyright (c) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "bmi270.h"
#include "bmi270_config_file.h"
LOG_MODULE_DECLARE(bmi270, CONFIG_SENSOR_LOG_LEVEL);

#define BMI270_REG_FEATURES_0_END   0x3F
#define BMI270_INTER_WRITE_DELAY_US 1000

#define BMI270_CRT_STATUS_CHECK_RETRIES    15
#define BMI270_NVM_STATUS_CHECK_RETRIES    100
#define BMI270_STATUS_CHECK_POLL_PERIOD_US 10000

/**
 * @brief This function verifies if advanced power saving mode is enabled
 *        If it enabled, it disables it.
 *
 * @param[in] status        : status to set advanced power saving mode
 * @param[in] dev           : Structure instance of bmi270 device
 *
 * @return Result of disable aps execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int bmi270_set_aps(const uint8_t status, const struct device *dev)
{
	uint8_t adv_pwr_save;
	int ret;
	/* Get status of advance power save mode */
	ret = bmi270_reg_read(dev, BMI270_REG_PWR_CONF, &adv_pwr_save, 1);
	if (ret != 0) {
		LOG_ERR("Read power config register failed w/ error: %d", ret);
		return ret;
	}

	/* Check if apsm is already the one being set */
	if ((adv_pwr_save & BMI270_PWR_CONF_ADV_PWR_SAVE_MSK) != status) {
		/* Change advance power save mode if needed */
		adv_pwr_save =
			BMI270_SET_BITS_POS_0(adv_pwr_save, BMI270_PWR_CONF_ADV_PWR_SAVE, status);
		ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CONF, &adv_pwr_save, 1,
						  BMI270_INTER_WRITE_DELAY_US);
		if (ret != 0) {
			LOG_ERR("Failed to disable advance power save, err: %d", ret);
			return ret;
		}
		LOG_DBG("advance power save mode set to: %d", status);
	} else {
		LOG_DBG("advance power save mode already in the intended state");
	}
	return 0;
}

/**
 * @brief Helper function to check if a feature address respects addressing rules
 *
 * @param[in] addr    : feature addr, in the respective page
 *
 * @note Writes to a FEATURES register must be 16-bit word oriented,
 *       i.e. writes should start at an even address (2m) and the last
 *       byte written should be at an odd address (2n+1), where 0x30<=2m<=2n<0x3F.
 *
 * @return Result of feature rmw execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int bmi270_feature_addr_check(uint8_t addr)
{
	/* Check that the start address is even */
	if (addr % 2 != 0) {
		LOG_ERR("Start address must be even: Address 0x%x", addr);
		return -EINVAL;
	}

	/* Check that the start address is greater than 0x30 */
	if (addr < (uint8_t)BMI270_REG_FEATURES_0) {
		LOG_ERR("Feature start address must be greater than 0x30: Address 0x%02x", addr);
		return -EINVAL;
	}

	/* Check that end address is less than 0x3F */
	if (addr + 0x01 > (uint8_t)BMI270_REG_FEATURES_0_END) {
		LOG_ERR("End address must be less than 0x3F: Address0x%02x", addr + 1);
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief This function is used to read-modify-write a feature
 *
 * @param[in] dev      : Structure instance of bmi270 device
 * @param[in] feature  : Pointer to the feature
 * @param[in] mask     : feature mask
 * @param[in] pos      : feature starting position bit
 * @param[in] value    : new feature value to be written
 *
 * @return Result of feature rmw execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int bmi270_feature_reg_rmw(const struct device *dev,
				  const struct bmi270_feature_reg *feature, uint16_t mask,
				  uint8_t pos, uint8_t value)
{
	int ret;
	uint8_t feat_page = feature->page;
	uint16_t feature_value;

	/* Check if feature address respects addressing rules */
	ret = bmi270_feature_addr_check(feature->addr);
	if (ret < 0) {
		LOG_ERR("Feature addr check failed, addr:%d at page %d, err:%d", feature->addr,
			feat_page, ret);
		return ret;
	}

	/* Disable advanced power save mode */
	ret = bmi270_set_aps(BMI270_PWR_CONF_ADV_PWR_SAVE_DIS, dev);
	if (ret != 0) {
		LOG_ERR("Failed bmi270_set_aps, err %d", ret);
		return ret;
	}

	/* Select feature page */
	ret = bmi270_reg_write(dev, BMI270_REG_FEAT_PAGE, &feat_page, 1);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", BMI270_REG_FEAT_PAGE, ret);
		return ret;
	}

	/* Read feature, 16-bit word oriented */
	ret = bmi270_reg_read(dev, feature->addr, (uint8_t *)&feature_value, 2);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_read (0x%02x) failed: %d", feature->addr, ret);
		return ret;
	}
	LOG_DBG("Read feature reg[0x%02x]@%d = 0x%04x", feature->addr, feature->page,
		feature_value);

	/* Modify feature Value */
	feature_value = ((feature_value & ~(mask)) | ((value << pos) & mask));

	/* Write feature, 16-bit word oriented */
	ret = bmi270_reg_write(dev, feature->addr, (uint8_t *)&feature_value, 2);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", feature->addr, ret);
		return ret;
	}
	LOG_DBG("Wrote feature reg[0x%02x]@%d = 0x%04x", feature->addr, feature->page,
		feature_value);

	return 0;
}

/**
 * @brief This function is used to disable block feature.
 *
 * @param[in] dev           : Structure instance of bmi270 device
 *
 * @return Result of disable block feature execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static inline int bmi270_g_trig_1_block_unblock(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;
	int ret = bmi270_feature_reg_rmw(dev, cfg->feature->g_trig_1, BMI270_G_TRIG_1_BLOCK_MASK,
					 BMI270_G_TRIG_1_BLOCK_POS, BMI270_G_TRIG_1_BLOCK_UNBLOCK);
	return ret;
}

/**
 * @brief This function is used to enable the CRT.
 *
 * @param[in] dev   : Structure instance of bmi270 device
 *
 * @return Result of select crt enable execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static inline int bmi270_g_trig_1_select_crt(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;
	int ret = bmi270_feature_reg_rmw(dev, cfg->feature->g_trig_1, BMI270_G_TRIG_1_SELECT_MASK,
					 BMI270_G_TRIG_1_SELECT_POS, BMI270_G_TRIG_1_SELECT_CRT);
	return ret;
}

/** @brief Helper method to LOG CRT cmd status */
static void crt_error_log(gtrigger_status status)
{
	switch (status) {
	case BMI270_CRT_TRIGGER_STAT_SUCCESS:
		LOG_INF("CRT was successful!\n");
		break;
	case BMI270_CRT_TRIGGER_STAT_PRECON_ERR:
		LOG_ERR("LOG_ERR: Pre-condition error, command is aborted.\n");
		break;
	case BMI270_CRT_TRIGGER_STAT_DL_ERR:
		LOG_ERR("LOG_ERR: Download error, command is aborted.\n");
		break;
	case BMI270_CRT_TRIGGER_STAT_ABORT_ERR:
		LOG_ERR("LOG_ERR: Command aborted by host or due to motion detection.\n");
		break;
	default:
		LOG_ERR("LOG_ERR: Unknown error code.\n");
		break;
	}
}

/**
 * @brief This internal function gets the saturation status for the gyroscope user
 * gain update.
 *
 * @param[in] dev                         : Structure instance of bmi270 device
 *
 * @return Result of select crt enable execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int bmi270_get_gyro_gain_update_status(const struct device *dev)
{
	int ret;
	const struct bmi270_config *cfg = dev->config;
	struct bmi270_data *data = dev->data;
	const uint8_t result_sts_page = cfg->feature->gyr_gain_status->page;
	uint16_t status_value;

	/* Check address rules */
	ret = bmi270_feature_addr_check(cfg->feature->gyr_gain_status->addr);
	if (ret < 0) {
		LOG_ERR("gyr_gain_status addr check failed, err:%d", ret);
		return ret;
	}

	/* Disable advanced power save mode */
	ret = bmi270_set_aps(BMI270_PWR_CONF_ADV_PWR_SAVE_DIS, dev);
	if (ret != 0) {
		LOG_ERR("Failed bmi270_set_aps, err %d", ret);
		return ret;
	}

	/* Select result_sts_page */
	ret = bmi270_reg_write(dev, BMI270_REG_FEAT_PAGE, &result_sts_page, 1);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_write (0x%02x) failed: %d", BMI270_REG_FEAT_PAGE, ret);
		return ret;
	}

	/* Read feature gyr_gain_status*/
	ret = bmi270_reg_read(dev, cfg->feature->gyr_gain_status->addr, (uint8_t *)&status_value,
			      2);
	if (ret < 0) {
		LOG_ERR("bmi270_reg_read (0x%02x) failed: %d", cfg->feature->gyr_gain_status->addr,
			ret);
		return ret;
	}
	LOG_DBG("Read feature reg[0x%02x]@%d = 0x%04x", cfg->feature->gyr_gain_status->addr,
		result_sts_page, status_value);


	/* Get the saturation status for x-axis */
	data->crt_result_sts.sat_x = (status_value & BMI270_GYR_GAIN_STATUS_SAT_X_MASK) >>
			    BMI270_GYR_GAIN_STATUS_SAT_X_POS;
	/* Get the saturation status for y-axis */
	data->crt_result_sts.sat_y = (status_value & BMI270_GYR_GAIN_STATUS_SAT_Y_MASK) >>
			    BMI270_GYR_GAIN_STATUS_SAT_Y_POS;
	/* Get the saturation status for z-axis */
	data->crt_result_sts.sat_z = (status_value & BMI270_GYR_GAIN_STATUS_SAT_Z_MASK) >>
			    BMI270_GYR_GAIN_STATUS_SAT_Z_POS;
	/* Get g trigger status */
	data->crt_result_sts.g_trigger_status =
		(gtrigger_status)((status_value & BMI270_GYR_GAIN_STATUS_G_TRIG_MASK) >>
				 BMI270_GYR_GAIN_STATUS_G_TRIG_POS);

	LOG_DBG("Status in x-axis: %d y-axis: %d z-axis %d, gtrigger: %d",
			data->crt_result_sts.sat_x,
			data->crt_result_sts.sat_y,
			data->crt_result_sts.sat_z,
			data->crt_result_sts.g_trigger_status);

	return 0;
}

/**
 * @brief This internal function gets the compensated user-gain data of gyroscope
 * gain update.
 *
 * @param[in] dev                         : Structure instance of bmi270 device
 *
 * @return Result of read compensated user-gain data execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int bmi270_read_gyro_user_gain(const struct device *dev)
{
	int ret;
	/* Variable to define register data */
	uint8_t reg_data[3] = {0};
	struct bmi270_data *data = dev->data;

	/* Get the gyroscope compensated gain values */
	ret = bmi270_reg_read(dev, BMI270_GYR_USR_GAIN_0, reg_data, 3);
	if (ret < 0) {
		LOG_ERR("failed to get the gyroscope compensated gain values, err: %d", ret);
		return ret;
	}

	/* Gyroscope user gain correction X-axis */
	data->crt_gain.x = (int8_t)(reg_data[0] & BMI270_GYR_USR_GAIN_MASK);

	/* Gyroscope user gain correction Y-axis */
	data->crt_gain.y = (int8_t)(reg_data[1] & BMI270_GYR_USR_GAIN_MASK);

	/* Gyroscope user gain correction z-axis */
	data->crt_gain.z = (int8_t)(reg_data[2] & BMI270_GYR_USR_GAIN_MASK);

	LOG_INF("Gyroscope user gain correction, X: %d Y: %d Z: %d",
			data->crt_gain.x, data->crt_gain.y, data->crt_gain.z);

	return ret;
}

/**
 * @brief This public method gets the compensated user-gain data of gyroscope
 * converted into sensor_value type
 *
 * @param[in] dev : Structure instance of bmi270 device
 * @param[in] gain : Compensated user-gain data of gyroscope
 *                   converted into sensor_value type.
 *                   sensor_value.val1 & 0xFF: x-axis
 *                   (sensor_value.val1 >> 8) & 0xFF: y-axis
 *                   (sensor_value.val1 >> 16) & 0xFF: z-axis
 *
 * @return Result of get compensated user-gain data of gyro
 * @retval 0 -> Success
 * @retval -EAGAIN -> CRT has not been performed yet.
 * @retval < 0 -> Fail
 */
int bmi270_get_gyro_user_gain(const struct device *dev, struct sensor_value *gain)
{
	int ret;
	struct bmi270_data *data = dev->data;

	/* Read user-gain data of gyroscope */
	ret = bmi270_read_gyro_user_gain(dev);
	if (ret != 0) {
		LOG_ERR("Failed in bmi270_read_gyro_user_gain: Error code %d", ret);
		return ret;
	}

	/* Unchanged user-gain data means CRT has not been performed yet */
	if ((data->crt_gain.x == 0) && (data->crt_gain.y == 0) && (data->crt_gain.z == 0)) {
		LOG_ERR("CRT has not yet been performed");
		return -EAGAIN;
	}

	/* Clean sensor value */
	gain->val1 = 0;
	gain->val2 = 0;

	/* Convert compensated user-gain data
	 * into sensor_value type
	 */
	gain->val1 |= data->crt_gain.x;
	gain->val1 |= (uint32_t)data->crt_gain.y << 8;
	gain->val1 |= (uint32_t)data->crt_gain.z << 16;

	return ret;
}

/**
 * @brief Method to prepare the setup for crt processing
 *
 * @param[in] dev   : Structure instance of bmi270 device
 *
 * @return Result of prepare the setup for crt execution
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
static int crt_prepare_setup(const struct device *dev)
{
	int ret;
	uint8_t pwr_ctrl;
	uint8_t fifo_config1;

	/*  Get Power mode control register */
	ret = bmi270_reg_read(dev, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1);
	if (ret != 0) {
		LOG_ERR("Read power config register failed w/ error: %d", ret);
		return ret;
	}
	/* Clears any bits above bit 3, unused */
	pwr_ctrl &= BMI270_PWR_CTRL_MSK;

	/* Disable gyroscope */
	pwr_ctrl &= ~BMI270_PWR_CTRL_GYR_EN;
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to disable gyroscope, err: %d", ret);
		return ret;
	}

	/* Disable FIFO for all sensors */
	ret = bmi270_reg_read(dev, BMI270_REG_FIFO_CONFIG_1, &fifo_config1, 1);
	if (ret != 0) {
		LOG_ERR("Read BMI270_REG_FIFO_CONFIG_1 failed w/ error: %d", ret);
		return ret;
	}
	fifo_config1 &= ~BMI270_FIFO_CONFIG_1_SENSORS_MSK;
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_FIFO_CONFIG_1, &fifo_config1, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to disable FIFO for all sensors, err: %d", ret);
		return ret;
	}

	/* Enable accelerometer */
	pwr_ctrl |= BMI270_PWR_CTRL_ACC_EN;
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to enable accelerometer, Error code %d", ret);
		return ret;
	}

	/* Set G_TRIG_1.block=0 / Disable Abort */
	ret = bmi270_g_trig_1_block_unblock(dev);
	if (ret != 0) {
		LOG_ERR("Failed to unblock crt g_trig feature, err: %d", ret);
	}

	return ret;
}

/**
 * @brief This function programs the non volatile memory(nvm)
 *
 * @param[in] dev:  Structure instance of bmi270 device.
 *
 * @return Result of execution status
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int bmi270_nvm_prog(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;
	const uint8_t nvm_prog_en = BMI270_NVM_PROG_EN;
	const uint8_t nvm_prog_cmd = BMI270_CMD_NVM_PROG;
	int ret;
	uint8_t status;
	uint8_t cmd_rdy;
	uint8_t tries;

	/* Disable advanced power save mode */
	ret = bmi270_set_aps(BMI270_PWR_CONF_ADV_PWR_SAVE_DIS, dev);
	if (ret != 0) {
		LOG_ERR("Failed bmi270_set_aps, err %d", ret);
		return ret;
	}

	/* Read Sensor status flags */
	ret = bmi270_reg_read(dev, BMI270_REG_STATUS, &status, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read BMI270_REG_STATUS, error: %d", ret);
		return ret;
	}

	/* cmd_rdy tells if a nvm prog is already in progress*/
	cmd_rdy = ((status & BMI270_CMD_RDY_MSK) >> BMI270_CMD_RDY_POS);
	if (cmd_rdy == 0) {
		LOG_ERR("NVM prog already running, canceling new request");
		return -ECANCELED;
	}

	/* Prepare NVM write by setting GEN_SET_1.nvm_prog_prep =0b1 */
	ret = bmi270_feature_reg_rmw(
		dev, cfg->feature->gen_set_1, BMI270_GEN_SET_1_NVM_PROG_PREP_MASK,
		BMI270_GEN_SET_1_NVM_PROG_PREP_POS, BMI270_GEN_SET_1_NVM_PROG_PREP_EN);
	if (ret != 0) {
		LOG_ERR("Failed set_nvm_prep_prog, err: %d", ret);
		return ret;
	}

	/* Wait 40 ms  */
	k_usleep(40000);

	/* Set bit 1 NVM_CONF.nvm_prog_en in order to unlock the NVM.  */
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_NVM_CONF, &nvm_prog_en, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to set the NVM_CONF.nvm_prog_en bit, err: %d", ret);
		return ret;
	}

	/* Write prog_nvm to the CMD register to trigger the write process */
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_CMD, &nvm_prog_cmd, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to Send NVM prog command to command register, err: %d", ret);
		return ret;
	}
	LOG_INF("Programming NVM ...");
	/* Wait till write operation is completed */
	for (tries = 0; tries <= BMI270_NVM_STATUS_CHECK_RETRIES; tries++) {
		ret = bmi270_reg_read(dev, BMI270_REG_STATUS, &status, 1);
		if (ret != 0) {
			LOG_ERR("Failed to read BMI270_REG_STATUS, error: %d", ret);
			return ret;
		}

		cmd_rdy = ((status & BMI270_CMD_RDY_MSK) >> BMI270_CMD_RDY_POS);
		/* Nvm is complete once cmd_rdy is 1, break if 1 */
		if (cmd_rdy) {
			LOG_INF("NVM prog Completed!");
			break;
		}
		/* Wait till cmd_rdy becomes 1 indicating
		 * nvm process completes
		 */
		k_usleep(BMI270_STATUS_CHECK_POLL_PERIOD_US);
	}

	/* Check if write operation timed-out */
	if (tries == BMI270_NVM_STATUS_CHECK_RETRIES) {
		LOG_ERR("Failed in CRT status check: Reached max number of retries");
		return -ETIME;
	}

	/* perform soft reset after nvm prog */
	ret = bmi270_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("Soft reset failed, err: %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief This function enables/disables gain compensation
 * with the gain  defined in the gyr_usr_gain_[xyz] register
 * to filtered and unfiltered Gyroscope data
 *
 * @param[in] dev    : Structure instance of bmi270 device.
 * @param[in] status : sensor_value.val1: 0x01-enable, 0x00-disable,
 *
 * @return Result of execution status
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int bmi270_set_gyro_gain(const struct device *dev, const struct sensor_value *status)
{
	int ret;
	uint8_t reg_data;

	/* Read gyr_gain_en register */
	ret = bmi270_reg_read(dev, BMI270_REG_OFFSET_6, &reg_data, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read BMI270_REG_STATUS, error: %d", ret);
		return ret;
	}

	/* Read gyr_gain_en register */
	reg_data = BMI270_SET_BITS(reg_data, BMI270_GYR_GAIN_EN, status->val1 & BMI270_GYR_GAIN_EN);

	/* Enable/Disable gyroscope gain */
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_OFFSET_6, &reg_data, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to set the BMI270_REG_OFFSET_6, err: %d", ret);
		return ret;
	}
	LOG_DBG("Gyro usr gain compensation status:%d", status->val1 & BMI270_GYR_GAIN_EN);
	return ret;
}

/**
 * @brief Method to run the crt process
 *
 * @param[in] dev           : Structure instance of bmi270 device
 *
 * @note CRT may run in the full operating temperature range.
 *       datasheet recommends to run CRT at the operating
 *       temperature of the device.
 *       The sensitivity error is typically minimal at the
 *       temperature CRT was performed at.
 *
 * @param[in] dev : Structure instance of bmi270 device.
 *
 * @return Result of CRT execution status
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 */
int bmi270_gyro_crt(const struct device *dev)
{
	int ret;
	uint8_t gyro_crt_conf;
	uint8_t tries;
	const uint8_t g_trigger_cmd = BMI270_CMD_G_TRIGGER;
	struct bmi2_gyro_user_gain_data before_crt_gain = {0};
	struct bmi270_data *data = dev->data;

	/* Get initial user gain state */
	ret = bmi270_read_gyro_user_gain(dev);
	if (ret != 0) {
		LOG_ERR("Failed in bmi270_read_gyro_user_gain: Error code %d", ret);
		return ret;
	}

	/* Save initial user gain state */
	memcpy(&before_crt_gain, &(data->crt_gain), sizeof(before_crt_gain));

	/* Disable advanced power save mode */
	ret = bmi270_set_aps(BMI270_PWR_CONF_ADV_PWR_SAVE_DIS, dev);
	if (ret != 0) {
		LOG_ERR("Failed bmi270_set_aps, err %d", ret);
		return ret;
	}

	/*  Get crt running status */
	ret = bmi270_reg_read(dev, BMI270_REG_GYR_CRT_CONF, &gyro_crt_conf, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read GYR_CRT_CONF, err: %d", ret);
		return ret;
	}

	if ((gyro_crt_conf & BMI270_GYR_CRT_CONF_RUNNING_MSK) >> BMI270_GYR_CRT_CONF_RUNNING_POS) {
		LOG_ERR("CRT already running!");
		return -ECANCELED;
	}

	/* Set GYR_CRT_CONF.crt_running=0b1  */
	gyro_crt_conf = BMI270_SET_BITS(gyro_crt_conf, BMI270_GYR_CRT_CONF_RUNNING,
					BMI270_GYR_CRT_CONF_RUNNING_EN);

	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_GYR_CRT_CONF, &gyro_crt_conf, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("Failed to enable CRT running, err: %d", ret);
		return ret;
	}

	/* CRT prepare setup */
	ret = crt_prepare_setup(dev);
	if (ret != 0) {
		LOG_ERR("CRT prepare setup failed, err: %d", ret);
		return ret;
	}

	/* Ensure that the device is at rest during CRT execution */
	LOG_WRN("Ensure that the device is at rest during CRT execution!");

	/* Execute CRT / Set G_TRIG_1.select=1  */
	ret = bmi270_g_trig_1_select_crt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable select crt in g_trig feature, err: %d", ret);
		return ret;
	}

	/* Send g_trigger command using the register CMD */
	ret = bmi270_reg_write(dev, BMI270_REG_CMD, &g_trigger_cmd, 1);
	if (ret != 0) {
		LOG_ERR("Failed to send g_trigger_cmd, err: %d", ret);
		return ret;
	}

	LOG_INF("CRT running...");
	/* CRT is complete, after the device sets GYR_CRT_CONF.crt_running=0b0
	 *
	 * Timeout after @ref BMI270_CRT_STATUS_CHECK_RETRIES x
	 * @ref BMI270_CRT_STATUS_CHECK_POLL_PERIOD_US microseconds.
	 * If tries is equal to @ref BMI270_CRT_STATUS_CHECK_RETRIES
	 * by the end of the loop, report an error
	 */
	for (tries = 0; tries <= BMI270_CRT_STATUS_CHECK_RETRIES; tries++) {
		ret = bmi270_reg_read(dev, BMI270_REG_GYR_CRT_CONF, &gyro_crt_conf, 1);
		if (ret != 0) {
			LOG_ERR("Failed to read Gyro CRT config, err: %d", ret);
			return ret;
		}
		gyro_crt_conf &= BMI270_GYR_CRT_CONF_RUNNING_MSK;
		if (gyro_crt_conf == BMI270_GYR_CRT_CONF_RUNNING_DIS) {
			LOG_INF("CRT Completed!");
			break;
		}
		k_usleep(BMI270_STATUS_CHECK_POLL_PERIOD_US);
	}

	/* Check timed-out */
	if (tries == BMI270_CRT_STATUS_CHECK_RETRIES) {
		LOG_ERR("Failed in CRT status check: Reached max number of retries");
		return -ETIME;
	}

	/* Get CRT results */
	ret = bmi270_get_gyro_gain_update_status(dev);
	if (ret != 0) {
		LOG_ERR("Failed in get_bmi270_crt_results: Error code %d", ret);
		return ret;
	}

	/* Print CRT result status */
	crt_error_log(data->crt_result_sts.g_trigger_status);

	/* Wait for the gyro gain data to be updated */
	k_sleep(K_MSEC(350));

	/* Get the gyroscope gain update data */
	ret = bmi270_read_gyro_user_gain(dev);
	if (ret != 0) {
		LOG_ERR("Failed in bmi270_read_gyro_user_gain: Error code %d", ret);
		return ret;
	}

	/* Check if new gain values are different */
	if ((before_crt_gain.x == data->crt_gain.x) &&
		(before_crt_gain.y == data->crt_gain.y) &&
	    (before_crt_gain.z == data->crt_gain.z)) {
		LOG_WRN("CRT new user-gyro gains remained the same");
	}

	/* Enable Advance power save if disabled while configuring and not when already disabled */
	ret = bmi270_set_aps(BMI270_PWR_CONF_ADV_PWR_SAVE_EN, dev);
	if (ret != 0) {
		return ret;
	}

	/* The new gain values are applied automatically at the next start of the gyroscope */
	return 0;
}

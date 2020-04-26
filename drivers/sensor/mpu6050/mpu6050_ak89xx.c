/*
 * Copyright (c) 2020 Pauli Salmenrinne
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <logging/log.h>
#include <drivers/sensor.h>

#include "mpu6050.h"

LOG_MODULE_DECLARE(MPU6050, CONFIG_SENSOR_LOG_LEVEL);



void mpu6050_ak89xx_convert_magn(struct sensor_value *val, s16_t raw_val,
				  s16_t scale)
{
	/* The sensor returns 10^-9 Teslas after scaling.
	 * The unit here is defined to be Gauss, and 1T = 10^4 Gauss
         * So sensor returns 10^-9 * 10^4 = 10^-5 Gauss units
	 */

	s32_t conv_val_cg = raw_val * scale ;
	val->val1 = conv_val_cg / 1000000;
	val->val2 = conv_val_cg % 1000000;
}


static int mpu6050_ak89xx_register_prepare( struct device *dev, u8_t regaddress,
					bool write_mode, u8_t count )
{

	/* Instruct the MPU9250 to access over its external i2c bus
	 * given device register with given details
	 */
	const struct mpu6050_config *cfg = dev->config->config_info;
	struct mpu6050_data *drv_data = dev->driver_data;
	u8_t mode_bit = 0x00;

	if (write_mode == false) {
		mode_bit = 0x80;
	}

	/* Set target register and operation type */
	__ASSERT_NO_MSG( (MPU9250_REG_VALUE_I2C_SLV0_ADDR_AK89XX & 0x80) == 0x00 );

	/* Set target i2c address  */
	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_I2C_SLV0_ADDR,
			       MPU9250_REG_VALUE_I2C_SLV0_ADDR_AK89XX | mode_bit  ) < 0 ) {
		return -EIO;
	}

	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_I2C_SLV0_REG,
			       regaddress ) < 0 ) {
		return -EIO;
	}

	/* Enable and write N bytes. Datasheet of MPU9250 is not too clear on Then
	 *  enable bit for write, but this is how it seems to work
	*/

	__ASSERT_NO_MSG( count <= 0x7 );
	__ASSERT_NO_MSG( count >= 0x1 );

	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_I2C_SLV0_CTRL,
			       MPU9250_REG_VALUE_I2C_SLV0_CTRL | count ) <0) {
		return -EIO;
	}

	/* Wait for a while to get read/write done */
	k_sleep(K_MSEC(1));
	return 0;

}

static int mpu6050_ak89xx_read_register( struct device *dev,
		u8_t regaddress, u8_t* data, u8_t read_n )
{
	const struct mpu6050_config *cfg = dev->config->config_info;
	struct mpu6050_data *drv_data = dev->driver_data;

	if (mpu6050_ak89xx_register_prepare( dev, regaddress, false, read_n) <0) {
		return -EIO;
	}

	/* Read the result */
	if (i2c_reg_read_byte(drv_data->i2c, cfg->i2c_addr,
			      MPU9250_REG_EXT_DATA00, data) < 0) {
		return -EIO;
	}

	return 0;
}

static int mpu6050_ak89xx_write_register( struct device *dev,
		u8_t regaddress, u8_t data, bool verify )
{
	const struct mpu6050_config *cfg = dev->config->config_info;
	struct mpu6050_data *drv_data = dev->driver_data;
	u8_t buffer;

	/* Set the data to be written */
	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_I2C_SLV0_DATA0,
			       data ) < 0 ) {
		return -EIO;
	}

	/* Instruct the write operation and execute */
	if (mpu6050_ak89xx_register_prepare( dev, regaddress, true, 1) <0 ) {
		return -EIO;
	}

	if ( verify )
	{
		/* Read to verify */
		if (mpu6050_ak89xx_read_register( dev,regaddress, &buffer, 1) <0 ) {
			return -EIO;
		}

		if( buffer != data) {
			return -EIO;
		}
	}

	return 0;
}


static int mpu6050_ak89xx_change_mode(struct device *dev, u8_t mode)
{
	/* Reset the chip -> reset all settings. */
	if ( mpu6050_ak89xx_write_register( dev, MPU9250_AK89XX_REG_CNTL1,
			mode, true ) < 0) {
		return -EIO;
	}
	/* Datasheet for AK8963 says at least 100uS wait is required */
	k_sleep(K_MSEC(1));
	return 0;
}

static s16_t mpu6050_ak89xx_calc_adj( s16_t reg_value )
{

	/** Datasheet says the actual register value is in 16bit output max value of 32760
	 * that corresponds to 4912 uT flux, yielding factor of 0.149938. We make no damage
	 * assume its 0.15.
	 *
	 * Now Zephyr unit is Gauss, and conversion is 1T = 10^4G
	 * -> 0.15 * 10^4 = 1500
	 * So if we multiply with scaling with 1500 the unit is uG.
	 */
	static const s16_t MPU6050_AK98XX_SCALE_TO_UG = 1500;

	/** The scaling formulation is: ( ( (s16_t)buffer[0] - 128 ) / 256 + 1 ) , that yields range 0.5 - 1.5. We can safely scale
	 *  it with 1500, due integer calculations, we reorder it a bit (multiply before divide)
	 */
	return (MPU6050_AK98XX_SCALE_TO_UG*( reg_value - 128 )) / 256 + MPU6050_AK98XX_SCALE_TO_UG;
}

static int mpu6050_ak89xx_fetch_adj( struct device *dev  )
{
	/* Read magnetometer adjustment data from the AK89XX chip */
	struct mpu6050_data *drv_data = dev->driver_data;
	u8_t buffer[3];

	/* We need to change to FUSE access mode before we can access
	 * adjustment registers
	 */

	if (mpu6050_ak89xx_change_mode( dev,
			MPU9250_AK89XX_REG_VALUE_CNTL1_FUSE_ROM ) < 0) {
		LOG_ERR("Failed to set chip in fuse access mode.");
		return -EIO;
	}

	if (mpu6050_ak89xx_read_register(dev,
			   MPU9250_AK89XX_REG_ADJ_DATA, buffer, 3) < 0) {
		LOG_ERR("Failed to read adjustment data.");
		return -EIO;
	}

	/* Change back to the powerdown mode, so we can later change it */
	if (mpu6050_ak89xx_change_mode( dev,
			MPU9250_AK89XX_REG_VALUE_CNTL1_POWERDOWN ) < 0) {
		LOG_ERR("Failed to set chip in power down mode.");
		return -EIO;
	}

	drv_data->magn_scale_x = mpu6050_ak89xx_calc_adj(buffer[0]);
	drv_data->magn_scale_y = mpu6050_ak89xx_calc_adj(buffer[1]);
	drv_data->magn_scale_z = mpu6050_ak89xx_calc_adj(buffer[2]);

	LOG_DBG("Adjustment values %d %d %d", drv_data->magn_scale_x,
		drv_data->magn_scale_y, drv_data->magn_scale_z );

	return 0;
}

static int mpu6050_ak89xx_reset(struct device *dev)
{
	/* Reset the chip -> reset all settings. */
	if ( mpu6050_ak89xx_write_register( dev,
			MPU9250_AK89XX_REG_CNTL2,
			MPU9250_AK89XX_REG_VALUE_CNTL2_RESET, false )  < 0 ) {
		LOG_ERR("Failed to reset AK89XX.");
		return -EIO;
	}
	return 0;
}



static int mpu6050_ak89xx_init_comm(struct device *dev)
{
	const struct mpu6050_config *cfg = dev->config->config_info;
	struct mpu6050_data *drv_data = dev->driver_data;

	/* Instruct MPU9250 to use its external I2C bus as master */
	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_USER_CTRL,
			       MPU9250_REG_VALUE_USER_CTRL_I2C_MASTERMODE ) < 0 ) {
		LOG_ERR("Failed to set MPU9250 external i2c mode.");
		return -EIO;
	}
	/* Set MPU9250 external I2C bus at 400khz and wait for result before
	 * issuing possible data ready interupt.
	 */
	if (i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			       MPU9250_REG_I2C_MST_CTRL,
			       MPU9250_REG_VALUE_I2C_MST_CTRL_WAIT_MAG_400KHZ ) < 0 ) {
		LOG_ERR("Failed to set MPU9250 external i2c speed.");
		return -EIO;
	}

	if (mpu6050_ak89xx_change_mode( dev,
			MPU9250_AK89XX_REG_VALUE_CNTL1_POWERDOWN ) < 0) {
		LOG_ERR("Failed to set chip in power down mode.");
		return -EIO;
	}

	return 0;
}
int mpu6050_ak89xx_init(struct device *dev)
{

	/* We could also set MPU9250 I2C to pass-thourgh mode for directly to chat with AK, but
	 * here we are using the MPU9250 registers to communicate with the AK, since that allows
	 * this driver later to be more easily to be ported for SPI access (that dont have
	 * possibility to talk I2C directly to the AK
	 */
	u8_t buffer[7];

	if ( mpu6050_ak89xx_init_comm(dev) <0) {
		return -EIO;
	}

	if ( mpu6050_ak89xx_reset(dev) <0 ) {
		return -EIO;
	}

	/* First check that the chip says hello */
	if ( mpu6050_ak89xx_read_register( dev,
			MPU9250_AK89XX_REG_ID, buffer, 1 ) < 0 ) {
		LOG_ERR("Failed to read AK89XX chip id.");
		return -EIO;
	}
	if ( buffer[0] != MPU9250_AK89XX_REG_VALUE_ID ) {
		LOG_ERR("Invalid AK89XX chip id (0x%X).", buffer[0] );
		return -EIO;
	}

	/* Fetch calibration data */
	if ( mpu6050_ak89xx_fetch_adj( dev ) < 0 ) {
		return -EIO;
	}

	/* Set AK sample rate and resolution */
	if ( mpu6050_ak89xx_change_mode( dev,
			MPU9250_AK89XX_REG_VALUE_CNTL1_16BIT_100HZ ) < 0 ) {
		LOG_ERR("Failed set sample rate for AK89XX.");
		return -EIO;
	}

	/* Fetch a sample from AK89XX. Then the MPU continues to
	 * make the reads at the sample rate
	 */
	if ( mpu6050_ak89xx_read_register( dev,
		MPU9250_AK89XX_REG_DATA, buffer, 7 ) < 0 ) {
		LOG_ERR("Failed read sample from AK89XX.");
		return -EIO;
	}

	return 0;
}



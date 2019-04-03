
#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <logging/log.h>
#include "ltr329als01.h"
#include <i2c.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ltr329als01);

#define RESERVED -1

struct ltr329als01_data {
	struct device *i2c_dev;
	uint16_t lux_val;
};

static int write_bytes(struct device *i2c_dev, u16_t addr,
		       u8_t *data, u32_t num_bytes)
{
	u8_t wr_addr;
	struct i2c_msg msgs[2];

	/* FRAM address */
	wr_addr = addr;

	/* Setup I2C messages */

	/* Send the address to write to */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Data to be written, and STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, ALS_ADDR);
}

static int read_bytes(struct device *i2c_dev, u16_t addr,
		      u8_t *data, u32_t num_bytes)
{
	u8_t wr_addr;
	struct i2c_msg msgs[2];

	/* Now try to read back from FRAM */

	/* FRAM address */
	wr_addr = addr;

	/* Setup I2C messages */

	/* Send the address to read from */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 2;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], 2, ALS_ADDR);
}

/**
 * @brief check for new data
 * 
 * @return true 
 * @return false 
 */

bool ALS_check_for_new_valid_data(struct device *i2c_dev)
{
  uint8_t status;
 
  read_bytes(i2c_dev, ALS_STATUS_REG, &status, 1);

  if ((status & 0x04) && !(status & 0x80))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief get integration time in ms from ALS_MEAS_RATE register
 * 
 * @return int 
 */
int ALS_get_integration_time(struct device *i2c_dev)
{
  int integration_time_lookup[8] = {100, 50, 200, 400, 150, 250, 300, 350};

  uint8_t meas_rate_reg_data;

  if(read_bytes(i2c_dev, ALS_MEAS_RATE_REG, &meas_rate_reg_data, 1) != 0)
  {
	  LOG_ERR("get ch err");
  }
  

  uint8_t integration_time_byte = (meas_rate_reg_data & 0x38) >> 3;

  int als_integration_time_ms = integration_time_lookup[integration_time_byte];

  return als_integration_time_ms;
}

/**
 * @brief get adc values of channel-0 and channel-1 from als data registers
 * 
 * @return uint16_t* 
 */
uint16_t* ALS_get_channels_data(struct device *i2c_dev)
{
  uint8_t als_buffer[4];
  static uint16_t als_channels_data[2];

  if(read_bytes(i2c_dev, ALS_DATA_CH1_0_REG, als_buffer, sizeof(als_buffer)) != 0)
  {
	  LOG_ERR("get ch err");
  }
  
  als_channels_data[0] = (als_buffer[1] << 8) | als_buffer[0];
  als_channels_data[1] = (als_buffer[3] << 8) | als_buffer[2];

  LOG_DBG("channels %d %d", als_channels_data[0], als_channels_data[1]);

  return als_channels_data;
}

/**
 * @brief get gain values from ALS_CONTR register
 * 
 * @return int 
 */
int ALS_get_gain(struct device *i2c_dev)
{
  int gain_lookup[8] = {1, 2, 4, 8, RESERVED, RESERVED, 48, 96};

  uint8_t contr_reg_data;
  read_bytes(i2c_dev, ALS_CONTR_REG, &contr_reg_data, sizeof(contr_reg_data));

  uint8_t als_gain_byte = (contr_reg_data & 0x1C) >> 2;

  int als_gain = gain_lookup[als_gain_byte];

  return als_gain;
}


/**
 * @brief calculate lux values from lux buffer
 * 
 * @return float 
 */
float ALS_get_lux(struct device *i2c_dev)
{
  uint16_t *als_channels_data;

  als_channels_data = ALS_get_channels_data(i2c_dev);

  //LOG_DBG("LTR  %x %x", als_channels_data[0], als_channels_data[1]);

  float ratio = (float)als_channels_data[1]/(float)(als_channels_data[0]+als_channels_data[1]);

  int ALS_GAIN = ALS_get_gain(i2c_dev);

  float ALS_INT = (float)ALS_get_integration_time(i2c_dev)/100;

  float als_lux;

  if(ratio<0.45)
  {
    als_lux = ((1.7743*(float)als_channels_data[0]) + (1.1059*(float)als_channels_data[1]))/(ALS_GAIN*ALS_INT);
  }
  else if(ratio<0.64 && ratio>=0.45)
  {
    als_lux = ((4.2785*(float)als_channels_data[0]) - (1.9548*(float)als_channels_data[1]))/(ALS_GAIN*ALS_INT);
  }
  else if(ratio<0.85 && ratio >=0.64)
  {
    als_lux = ((0.5926*(float)als_channels_data[0]) + (0.1185*(float)als_channels_data[1]))/(ALS_GAIN*ALS_INT);
  }
  else
  {
    als_lux = 0;
  }

  return als_lux;
}


static int ltr329als01_sample_fetch(struct device *dev,
				    enum sensor_channel chan)
{
	struct ltr329als01_data *ltr_data = dev->driver_data; 
	float als_val = ALS_get_lux(ltr_data->i2c_dev);
	ltr_data->lux_val = als_val;

	return 0;
}

static int ltr329als01_channel_get(struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct ltr329als01_data *ltr_data = dev->driver_data; 
	
    // TODO do this only on all channels or light channel
	float als_val = ltr_data->lux_val;
	val->val1 = (s32_t)als_val;
	val->val2 = ((s32_t)(als_val * 1000000)) % 1000000;
	//LOG_DBG("val1=%d, val2=%d", val->val1, val->val2);

	return 0;
}

static const struct sensor_driver_api ltr329als01_api = {
	.sample_fetch = &ltr329als01_sample_fetch,
	.channel_get = &ltr329als01_channel_get,
};

static int ltr329als01_init(struct device *dev)
{
	struct ltr329als01_data *drv_data = dev->driver_data;
	uint8_t part_id = 0;

	LOG_INF("ltr329als01_init\r\n");

	drv_data->i2c_dev = device_get_binding("I2C_0");
	if (!drv_data->i2c_dev) {
		LOG_ERR("i2c master not found.");
		return -EINVAL;
	}
	
	if(read_bytes(drv_data->i2c_dev, MANUFAC_ID_REG, &part_id, 1) == 0)
	{
		LOG_DBG("found! %x \r\n", part_id);
	}
	else
	{
		return -EIO;
	}

	read_bytes(drv_data->i2c_dev, MANUFAC_ID_REG, &part_id, 1);
	LOG_DBG("manf id=%x\r\n",part_id);

	//enable ltr sensor
	uint8_t enable_ltr = 0x01;
	write_bytes(drv_data->i2c_dev,ALS_CONTR_REG, &enable_ltr,1);

	return 0;
}

static struct ltr329als01_data ltr_data;

DEVICE_AND_API_INIT(ltr329als01, "LTR_0", ltr329als01_init, &ltr_data, NULL,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ltr329als01_api);
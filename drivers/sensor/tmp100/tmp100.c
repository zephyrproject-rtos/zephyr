#define DT_DRV_COMPAT ti_tmp100

#include <stdint.h>
#include <errno.h>
#include <device.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(TMP100, CONFIG_TMP100_LOG_LEVEL);

#define TMP100_I2C_ADDRESS DT_INST_REG_ADDR(0)

#define TMP100_REG_TEMPERATURE 0x00
#define TMP100_REG_CONFIG 0x01
#define TMP100_TEMP_SCALE 62500

struct tmp100_data {
    const struct device *i2c;
    int16_t sample;
};

static int tmp100_reg_read(struct tmp100_data *drv_data,
			   uint8_t reg, uint16_t *val)
{
    if (i2c_burst_read(drv_data->i2c, TMP100_I2C_ADDRESS,
               reg, (uint8_t *) val, 2) < 0) {
        return -EIO;
    }
	*val = sys_be16_to_cpu(*val);
	return 0;
}

static int tmp100_reg_write(struct tmp100_data *drv_data,
                uint8_t reg, uint16_t val)
{
    uint16_t val_be = sys_cpu_to_be16(val);
    return i2c_burst_write(drv_data->i2c, TMP100_I2C_ADDRESS,
                   reg, (uint8_t *)&val_be, 2);
}

static int tmp100_reg_update(struct tmp100_data *drv_data, uint8_t reg,
                 uint16_t mask, uint16_t val)
{
    uint16_t old_val = 0U;
    uint16_t new_val;

    if (tmp100_reg_read(drv_data, reg, &old_val) < 0) {
        return -EIO;
    }

    new_val = old_val & ~mask;
    new_val |= val & mask;

    return tmp100_reg_write(drv_data, reg, new_val);
}

static int tmp100_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
    struct tmp100_data *drv_data = dev->data;
	int64_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if(attr == SENSOR_ATTR_FULL_SCALE) {
		if (val->val1 == 128) {
			value = 0x00;
		}
		else {
			return -ENOTSUP;
		}
    }	

    if (tmp100_reg_update(drv_data, TMP100_REG_CONFIG,
                1, value) < 0) {
            LOG_DBG("Failed to set attribute!");
            return -EIO;
        }
    return 0;
}

static int tmp100_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp100_data *drv_data = dev->data;
	uint16_t val;
    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);
    if (tmp100_reg_read(drv_data, TMP100_REG_TEMPERATURE, &val) < 0) {
        return -EIO;
    }

	drv_data->sample = arithmetic_shift_right((int16_t)val, 4);
	return 0;
}

static int tmp100_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp100_data *drv_data = dev->data;
    int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

    uval = (int32_t)drv_data->sample * TMP100_TEMP_SCALE;
    val->val1 = uval / 1000000;
    val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api tmp100_driver_api = {
    .attr_set = tmp100_attr_set,
	.sample_fetch = tmp100_sample_fetch,
	.channel_get = tmp100_channel_get,
};

int tmp100_init(const struct device *dev)
{
	struct tmp100_data *drv_data = dev->data;
	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
    if (drv_data->i2c == NULL) {
        LOG_DBG("Failed to get pointer to %s device!",
                DT_INST_BUS_LABEL(0));
        return -EINVAL;
    }
	return 0;
}

static struct tmp100_data tmp100_driver;
DEVICE_DT_INST_DEFINE(0, tmp100_init, NULL, &tmp100_driver, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tmp100_driver_api);

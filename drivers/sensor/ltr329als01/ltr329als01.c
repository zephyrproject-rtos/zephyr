
#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <logging/log.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include <logging/log.h>
LOG_MODULE_REGISTER(ltr329als01);

static struct device *i2c_dev;

static int ltr329als01_init(struct device *dev)
{
	i2c_dev = device_get_binding("I2C_0");
	if (!i2c_dev) {
		LOG_DBG("i2c master not found.");
		return -EINVAL;
	}

    LOG_ERR("ltr329als01_init");

	return 0;
}

DEVICE_AND_API_INIT(ltr329als01, "LTR_0", ltr329als01_init,
		    &i2c_dev, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, NULL);
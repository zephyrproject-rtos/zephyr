#include <device.h>
#include <i2c.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>

#include "icm20948.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(ICM20948);


int icm20948_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
    struct icm20948_data *drv_data = dev->driver_data;

    __ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

    gpio_pin_disable_callback(drv_data->gpio, );
    
}
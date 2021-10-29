//
// Created by jonasotto on 10/29/21.
//

#include "as5x47.h"

#define DT_DRV_COMPAT ams_as5x47

#include <devicetree.h>
#include <drivers/sensor.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "AS5x47 driver enabled without any devices"
#endif


int as5x47_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    // TODO: Read data from sensor
    // spi_transceive_dt(...);
    return -ENOTSUP;
}

int as5x47_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    // TODO: Return data
    // val = dev->data...
    return -ENOTSUP;
}

int as5x47_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
                    const struct sensor_value *val) {
    return -ENOTSUP;
}

int as5x47_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
                    struct sensor_value *val) {
    return -ENOTSUP;
}

int as5x47_trigger_set(const struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler) {
    return -ENOTSUP;
}


const struct sensor_driver_api as5x47_sensor_api = {
        .sample_fetch = as5x47_sample_fetch,
        .channel_get = as5x47_channel_get,
        .attr_set = as5x47_attr_set,
        .attr_get = as5x47_attr_get,
        .trigger_set = as5x47_trigger_set,
}

#define AS5x47_DEVICE_INIT(inst) \
    static as5x47_data as5x47_data_##inst; \
    static as5x47_config as5x47_cfg_##inst = { \
        .spi_spec = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(16), 0), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, as5x47_init, NULL, &as5x47_data_##inst, &as5x47_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &as5x47_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(AS5x47_DEVICE_INIT);
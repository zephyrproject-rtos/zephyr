//
// Created by jonasotto on 10/29/21.
//

#include "as5x47.h"

#define DT_DRV_COMPAT ams_as5x47

#include <devicetree.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "AS5x47 driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(as5x47_driver, LOG_LEVEL_INF);

const as5x47_config *get_config(const struct device *dev) {
    return dev->config;
}

as5x47_data *get_data(const struct device *dev) {
    return dev->data;
}


int as5x47_init(const struct device *dev) {
    const as5x47_config *cfg = dev->config;
    bool initSuccessful = initializeSensor(cfg->sensor, cfg->useUVW, cfg->uvwPolePairs);
    if (!initSuccessful) {
        LOG_ERR("AS5x47 initialization of device \"%s\" unsuccessful", dev->name);
        return -EIO;
    }
    LOG_INF("AS5x47 initialization of device \"%s\" successful", dev->name);
    return 0;
}

int as5x47_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_ROTATION) {
        if (!readAngleDegree(get_config(dev)->sensor,
                             &get_data(dev)->angle_deg,
                             true,
                             true, false, false)) {
            LOG_ERR("Error reading from sensor");
            get_data(dev)->angle_deg = 0;
            return -EIO;
        }
        return 0;
    }
    return -ENOTSUP;
}

int as5x47_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    if (chan == SENSOR_CHAN_ROTATION) {
        sensor_value_from_double(val, get_data(dev)->angle_deg);
        return 0;
    }

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
        .attr_set = as5x47_attr_set,
        .attr_get = as5x47_attr_get,
        .trigger_set = as5x47_trigger_set,
        .sample_fetch = as5x47_sample_fetch,
        .channel_get = as5x47_channel_get,
};

#define AS5x47_DEVICE_INIT(inst) \
    static as5x47_data as5x47_data_##inst; \
    static as5x47_config as5x47_cfg_##inst = { \
        .spi_spec = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(16) | SPI_TRANSFER_MSB | SPI_MODE_CPHA, 0), \
        .sensor = &as5x47_cfg_##inst.spi_spec, \
        .useUVW = __builtin_strcmp(DT_INST_PROP(inst, output_interface), "uvw"), \
        .uvwPolePairs = DT_INST_PROP(inst, uvw_polepairs) \
    }; \
    DEVICE_DT_INST_DEFINE(inst, as5x47_init, NULL, &as5x47_data_##inst, &as5x47_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &as5x47_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(AS5x47_DEVICE_INIT);

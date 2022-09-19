#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#define DT_DRV_COMPAT leuze_odsl8

LOG_MODULE_REGISTER(LASER, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "Laser driver enabled without any devices"
#endif

struct sensor_config {
    const struct device *adc;
    struct adc_channel_cfg channel_config;
    struct adc_sequence adc_seq;
    const int meas_resistor;
};

struct adc_data {
    uint16_t buffer[1];
};

static int laser_sample_fetch(const struct device *dev, enum sensor_channel channel) {

    const struct sensor_config *config = dev->config;

    if (!device_is_ready(config->adc)) {
        LOG_ERR("Device ADC not ready!");
        return -EINVAL;
    }

    LOG_INF("start adc reading!");
    int err = adc_read(config->adc, &config->adc_seq);
    LOG_INF("finished adc reading!");

    if (err) {
        LOG_ERR("Failed to read ADC channel (error %d)", err);
        return err;
    }
    return 0;

}

static int laser_channel_get(const struct device *dev,
                             enum sensor_channel channel,
                             struct sensor_value *val) {

    if (channel != SENSOR_CHAN_DISTANCE) {
        val->val1 = 0;
        val->val2 = 0;
        LOG_ERR("Wrong channel selection! Sensor only supports channel distance.");
        return -ENOTSUP;
    }

    LOG_INF("start channel get!");
    struct adc_data *data = dev->data;
    const struct sensor_config *config = dev->config;

    int32_t raw_value = (int32_t) data->buffer[0];

    adc_raw_to_millivolts(adc_ref_internal(config->adc),
                          config->channel_config.gain,
                          config->adc_seq.resolution,
                          &raw_value);

    LOG_INF("finished channel get!");
    // 4..20mA for 20..500mm (linear)
    const double slope = (500.0 - 20.0) / (20.0 - 4.0);
    double distance_in_m = (((double) raw_value / (double) config->meas_resistor - 4 )  * slope + 20.0) / 1000.0; // distance in m

    if (distance_in_m < 0.02 || distance_in_m > 0.500) {
        LOG_WRN("Sensor not in linear region between 20..500mm!");
    }

    sensor_value_from_double(val, distance_in_m);

    return 0;
}

int laser_init(struct device *dev) {

    const struct sensor_config *config = dev->config;
    struct adc_data *data = dev->data;

    if (!device_is_ready(config->adc)) {
        LOG_ERR("Device ADC not ready!");
        return -EINVAL;
    }
    int err = adc_channel_setup(config->adc, &config->channel_config);
    if (err) {
        LOG_ERR("Failed to configure ADC channel (error %d)", err);
        return err;
    }
    return 0;
}

static const struct sensor_driver_api laser_driver_api = {
        .sample_fetch = &laser_sample_fetch,
        .channel_get = &laser_channel_get,
};

#define LEUZE_ODSL8_INIT(inst) \
    static struct adc_data data_##inst; \
    static struct sensor_config cfg_##inst = { \
        .adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)), \
        .channel_config= { \
            .gain = ADC_GAIN_1, \
            .reference = ADC_REF_INTERNAL, \
            .acquisition_time = ADC_ACQ_TIME_DEFAULT, \
            .channel_id = DT_INST_IO_CHANNELS_INPUT(inst), \
            }, \
        .adc_seq = { \
                .options = NULL, \
                .channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)), \
                .buffer = &data_##inst.buffer, \
                .buffer_size = sizeof(data_##inst.buffer), \
                .resolution = 10,\
            },                 \
        .meas_resistor = DT_INST_PROP(inst, meas_resistor)    \
    };                         \
    DEVICE_DT_INST_DEFINE(inst, laser_init, NULL, \
                        &data_##inst, &cfg_##inst, POST_KERNEL, \
                        CONFIG_SENSOR_INIT_PRIORITY, \
                        &laser_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LEUZE_ODSL8_INIT)

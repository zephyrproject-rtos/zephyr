#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Get GPIO spec directly from device tree node */
static const struct gpio_dt_spec radar_gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(radar_detect_pin), gpios);
static struct gpio_callback radar_cb_data;

void radar_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    int val = gpio_pin_get_dt(&radar_gpio);
    if (val == 0) {
        LOG_INF(">>> MOTION DETECTED!");
    } else {
        LOG_INF("<<< Area Clear");
    }
}

int main(void) {
    const struct device *radar_dev = DEVICE_DT_GET(DT_NODELABEL(at581x));
    struct sensor_value detection_val;
    
    if (!device_is_ready(radar_dev)) {
        LOG_ERR("AT581X radar sensor not ready");
        return -1;
    }
    
    if (!gpio_is_ready_dt(&radar_gpio)) {
        LOG_ERR("GPIO pin IO21 not ready");
        return -1;
    }

    /* Configure GPIO */
    gpio_pin_configure_dt(&radar_gpio, GPIO_INPUT);
    gpio_init_callback(&radar_cb_data, radar_handler, BIT(radar_gpio.pin));
    gpio_add_callback(radar_gpio.port, &radar_cb_data);
    gpio_pin_interrupt_configure_dt(&radar_gpio, GPIO_INT_EDGE_BOTH);

    LOG_INF("AT581X radar sensor initialized");
    LOG_INF("Monitoring Radar on IO21...");
    
    while (1) {
        /* Periodically read sensor data */
        if (sensor_sample_fetch(radar_dev) == 0) {
            if (sensor_channel_get(radar_dev, SENSOR_CHAN_DISTANCE, &detection_val) == 0) {
                LOG_DBG("Detection value: %d", detection_val.val1);
            }
        }
        k_sleep(K_SECONDS(5));
    }
    return 0;
}
/*
 * Copyright (c) 2022 tangchunhui@coros.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_keys

/**
 * @file
 * @brief GPIO driven KEYs
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/key.h>
#include <zephyr/dt-bindings/key/key.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(key_gpio);

#ifndef MAX
#define MAX(x, y)   (((x) > (y)) ? (x) : (y))
#endif

#define SCAN_INTERVAL   CONFIG_KEY_GPIO_SCAN_INTERVAL
#define TIME_DEBOUNCE   CONFIG_KEY_GPIO_TIME_DEBOUNCE
#define TIME_LONG       CONFIG_KEY_GPIO_TIME_LONG
#define TIME_HOLD       CONFIG_KEY_GPIO_TIME_HOLD

/**
 * @brief KEY information structure
 *
 * This structure gathers useful information about KEY controller.
 *
 * @param label KEY label.
 * @param code  KEY code.
 * @param ms_debounce KEY debounce time
 * @param ms_long KEY long time
 * @param ms_hold KEY hold time
 */
struct key_info_dt_spec {
    const char *label;
    uint16_t code;
    uint16_t ms_debounce;
    uint16_t ms_long;
    uint16_t ms_hold;
};

/**
 * @brief Get KEY INFO DT SPEC
 *
 * @param node_id devicetree node identifier
 */
#define KEY_INFO_DT_SPEC_GET(node_id)                   \
{                                                       \
    .label       = DT_LABEL(node_id),                   \
    .code        = DT_PROP_OR(node_id, code, 0),        \
    .ms_debounce = DT_PROP_OR(node_id, ms_debounce, 0), \
    .ms_long     = DT_PROP_OR(node_id, ms_long, 0),     \
    .ms_hold     = DT_PROP_OR(node_id, ms_hold, 0),     \
}

typedef enum
{
    KEY_STATE_NONE,
    KEY_STATE_PRESSED,
    KEY_STATE_LONG_PRESSED,
    KEY_STAT_MAXNBR
} key_state_t;

struct key_gpio_driver
{
    struct gpio_callback gpio_data;

    void *pdata;
    bool  pressed;
    key_state_t state;
    key_event_t event;

    uint32_t count_debounce;
    uint32_t count_cycle;
};

struct key_gpio_data
{
    const struct device *dev;

    key_callback_t callback;
    struct k_work_delayable delayed_work;

    struct key_gpio_driver *driver;
};

struct key_gpio_config
{
    uint8_t num_keys;
    const struct gpio_dt_spec *gpio;
    const struct key_info_dt_spec *info;
};

#define MS_TO_CYCLE(ms, default)            (((ms) ? (ms) : (default)) / SCAN_INTERVAL)
#define KEY_EVENT_CODE(code, default)       ((code != KEY_RESERVED) ? code : default)
#define KEY_EVENT_CALL(dev, code, event)    { if (data->callback) data->callback(dev, code, event); }

static bool key_gpio_one_key_proc(const struct device *dev, int idx)
{
    const struct key_gpio_config *config = dev->config;
    struct key_gpio_data *data = dev->data;

    const struct gpio_dt_spec *gpio = &config->gpio[idx];
    const struct key_info_dt_spec *info = &config->info[idx];
    struct key_gpio_driver *driver = &data->driver[idx];

    bool pressed = (gpio_pin_get(gpio->port, gpio->pin) ^ gpio->dt_flags);

    if (driver->state != KEY_STATE_NONE)
        driver->count_cycle++;

    if (driver->pressed != pressed)
    {
        if (driver->count_debounce++ >= MS_TO_CYCLE(info->ms_debounce, TIME_DEBOUNCE))
        {
            driver->pressed = pressed;
            driver->count_debounce = 0;
        }
    }
    else
    {
        driver->count_debounce = 0;
    }

    switch(driver->state)
    {
    case KEY_STATE_NONE:
        if (driver->pressed == true)
        {
            driver->event = KEY_EVENT_PRESSED;
            driver->state = KEY_STATE_PRESSED;
            driver->count_cycle = 0;
            KEY_EVENT_CALL(dev, KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
        }
        else
        {
            driver->event = KEY_EVENT_NONE;
        }
        break;

    case KEY_STATE_PRESSED:
        if (driver->pressed == false)
        {
            driver->event = KEY_EVENT_RELEASE;
            driver->state = KEY_STATE_NONE;
            KEY_EVENT_CALL(dev, KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
        }
        else if (MS_TO_CYCLE(info->ms_long, TIME_LONG) > 0 &&
                 driver->count_cycle >= MS_TO_CYCLE(info->ms_long, TIME_LONG))
        {
            driver->event = KEY_EVENT_LONG_PRESSED;
            driver->state = KEY_STATE_LONG_PRESSED;
            driver->count_cycle = 0;
            KEY_EVENT_CALL(dev, KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
        }
        break;

    case KEY_STATE_LONG_PRESSED:
        if (driver->pressed == true)
        {
            if (MS_TO_CYCLE(info->ms_hold, TIME_HOLD) > 0 &&
                driver->count_cycle >= MS_TO_CYCLE(info->ms_hold, TIME_HOLD))
            {
                driver->event = KEY_EVENT_HOLD_PRESSED;
                driver->count_cycle  = 0;
                KEY_EVENT_CALL(dev, KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
            }
        }
        else
        {
            driver->event = KEY_EVENT_LONG_RELEASE;
            driver->state = KEY_STATE_NONE;
            KEY_EVENT_CALL(dev, KEY_EVENT_CODE(info->code, (idx + 1)), driver->event);
        }
        break;

    default:
        LOG_ERR("Unknown key state[%d]", driver->state);
        break;
    }

    // return key is pressed.
    return ((pressed || (driver->state != KEY_STATE_NONE)) ? true : false);
}

static void gpio_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    struct key_gpio_driver *driver = CONTAINER_OF(cb, struct key_gpio_driver, gpio_data);
    struct key_gpio_data *data = driver->pdata;
    int work_busy = k_work_delayable_busy_get(&data->delayed_work);

    if (!(work_busy & (K_WORK_DELAYED | K_WORK_QUEUED)))
        k_work_schedule(&data->delayed_work, K_MSEC(SCAN_INTERVAL));
}

static void delayed_work_handler(struct k_work *work)
{
    struct k_work_delayable *delayed_work = k_work_delayable_from_work(work);
    struct key_gpio_data *data = CONTAINER_OF(delayed_work, struct key_gpio_data, delayed_work);
    const struct key_gpio_config *config = data->dev->config;
    int key_count = 0;

    for (int i = 0; i < config->num_keys; i++)
    {
        if (key_gpio_one_key_proc(data->dev, i))
        {
            key_count++;
        }
    }

    if (key_count > 0)
    {
        k_work_schedule(&data->delayed_work, K_MSEC(SCAN_INTERVAL));
    }
}

static int key_gpio_setup(const struct device *dev, key_callback_t callback)
{
    const struct key_gpio_config *config = dev->config;
    struct key_gpio_data *data = dev->data;

    data->callback = callback;

    for (int i = 0; i < config->num_keys; i++)
    {
        const struct gpio_dt_spec *gpio = &config->gpio[i];
        struct key_gpio_driver *driver = &data->driver[i];

        gpio_add_callback(gpio->port, &driver->gpio_data);
    }

    // Maybe key pressed before driver enable.
    k_work_schedule(&data->delayed_work, K_NO_WAIT);

    return 0;
}

static int key_gpio_remove(const struct device *dev)
{
    const struct key_gpio_config *config = dev->config;
    struct key_gpio_data *data = dev->data;

    for (int i = 0; i < config->num_keys; i++)
    {
        const struct gpio_dt_spec *gpio = &config->gpio[i];
        struct key_gpio_driver *driver = &data->driver[i];

        gpio_remove_callback(gpio->port, &driver->gpio_data);
    }

    k_work_cancel_delayable(&data->delayed_work);

    data->callback = NULL;

    return 0;
}

static int key_gpio_init(const struct device *dev)
{
    const struct key_gpio_config *config = dev->config;
    struct key_gpio_data *data = dev->data;
    int err = 0;

    if (config->num_keys <= 0)
    {
        LOG_ERR("%s: no KEYs found (DT child nodes missing)", dev->name);
        return -ENODEV;
    }

    data->dev      = dev;
    data->callback = NULL;

    k_work_init_delayable(&data->delayed_work, delayed_work_handler);

    memset(data->driver, 0x00, config->num_keys * sizeof(struct key_gpio_driver));

    LOG_DBG("gpio key map:");

    for (int i = 0; i < config->num_keys; i++)
    {
        const struct gpio_dt_spec *gpio = &config->gpio[i];
        const struct key_info_dt_spec *info = &config->info[i];
        struct key_gpio_driver *driver = &data->driver[i];

        driver->pdata = data;

        if (!device_is_ready(gpio->port))
        {
            LOG_WRN("gpio port[%s] is not ready", gpio->port->name);
            continue;
        }

        err = gpio_pin_configure_dt(gpio, GPIO_INPUT);
        if (err != 0)
        {
            LOG_WRN("configure extra_flags on gpio[%s %d] fail[%d]",
                      gpio->port->name, gpio->pin, err);
            continue;
        }

        err = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
        if (err != 0)
        {
            LOG_WRN("Configure interrupt on gpio[%s %d] fail[%d]",
                      gpio->port->name, gpio->pin, err);
            continue;
        }

        gpio_init_callback(&driver->gpio_data, gpio_isr_handler, BIT(gpio->pin));

        LOG_DBG("KEY%d: label[%s] gpio[%p %d 0x%04x] code[0x%04x] interval[%d %d %d %d]",
                i, info->label, gpio->port, gpio->pin, gpio->dt_flags, info->code, SCAN_INTERVAL,
                (MS_TO_CYCLE(info->ms_debounce, TIME_DEBOUNCE) * SCAN_INTERVAL),
                (MS_TO_CYCLE(info->ms_long,     TIME_LONG)     * SCAN_INTERVAL),
                (MS_TO_CYCLE(info->ms_hold,     TIME_HOLD)     * SCAN_INTERVAL));
    }

    return err;
}

static const struct key_driver_api key_gpio_api =
{
    .setup  = key_gpio_setup,
    .remove = key_gpio_remove,
};

#define KEY_GPIO_DT_SPEC(key_node_id)     \
    GPIO_DT_SPEC_GET(key_node_id, gpios), \

#define KEY_INFO_DT_SPEC(key_node_id)     \
    KEY_INFO_DT_SPEC_GET(key_node_id),    \

#define KEY_GPIO_DEVICE(i)                                  \
                                                            \
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {     \
    DT_INST_FOREACH_CHILD(i, KEY_GPIO_DT_SPEC)              \
};                                                          \
                                                            \
static const struct key_info_dt_spec info_dt_sepc_##i[] = { \
    DT_INST_FOREACH_CHILD(i, KEY_INFO_DT_SPEC)              \
};                                                          \
                                                            \
static struct key_gpio_driver                               \
key_gpio_driver_##i[ARRAY_SIZE(gpio_dt_spec_##i)];          \
                                                            \
static struct key_gpio_data key_gpio_data_##i = {           \
    .driver = key_gpio_driver_##i,                          \
};                                                          \
                                                            \
static const struct key_gpio_config key_gpio_config_##i = { \
    .num_keys   = ARRAY_SIZE(gpio_dt_spec_##i),             \
    .gpio       = gpio_dt_spec_##i,                         \
    .info       = info_dt_sepc_##i,                         \
};                                                          \
                                                            \
DEVICE_DT_INST_DEFINE(i, key_gpio_init, NULL,               \
              &key_gpio_data_##i, &key_gpio_config_##i,     \
              POST_KERNEL, CONFIG_KEY_INIT_PRIORITY,        \
              &key_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(KEY_GPIO_DEVICE)

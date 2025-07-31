/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * LPC54S018 CTIMER Sample
 * 
 * This sample demonstrates the use of the CTIMER counter/timer peripheral
 * on the LPC54S018. It sets up a periodic timer interrupt that toggles
 * an LED and prints the elapsed time.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(ctimer_sample, LOG_LEVEL_INF);

/* Timer configuration */
#define TIMER_NODE DT_NODELABEL(ctimer0)
#define TIMER_PERIOD_SEC 1  /* 1 second period */

/* LED configuration */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Global variables */
static uint32_t seconds_elapsed = 0;

/* Timer callback function */
static void timer_callback(const struct device *counter_dev,
                          uint8_t chan_id, uint32_t ticks,
                          void *user_data)
{
    int ret;
    
    seconds_elapsed++;
    
    /* Toggle LED */
    ret = gpio_pin_toggle_dt(&led);
    if (ret < 0) {
        LOG_ERR("Failed to toggle LED");
    }
    
    LOG_INF("Timer fired! Elapsed time: %u seconds", seconds_elapsed);
}

int main(void)
{
    const struct device *timer_dev;
    struct counter_alarm_cfg alarm_cfg;
    uint32_t ticks;
    int ret;
    
    LOG_INF("LPC54S018 CTIMER Sample");
    
    /* Initialize LED */
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED device not ready");
        return -1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED pin");
        return -1;
    }
    
    /* Get timer device */
    timer_dev = DEVICE_DT_GET(TIMER_NODE);
    if (!device_is_ready(timer_dev)) {
        LOG_ERR("Timer device not ready");
        return -1;
    }
    
    /* Get timer information */
    uint32_t freq = counter_get_frequency(timer_dev);
    LOG_INF("Timer frequency: %u Hz", freq);
    
    /* Start the timer */
    ret = counter_start(timer_dev);
    if (ret < 0) {
        LOG_ERR("Failed to start timer");
        return -1;
    }
    
    /* Configure alarm for periodic interrupts */
    ticks = counter_us_to_ticks(timer_dev, TIMER_PERIOD_SEC * USEC_PER_SEC);
    
    alarm_cfg.flags = COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
    alarm_cfg.ticks = ticks;
    alarm_cfg.callback = timer_callback;
    alarm_cfg.user_data = NULL;
    
    /* Set the alarm */
    ret = counter_set_channel_alarm(timer_dev, 0, &alarm_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to set alarm");
        return -1;
    }
    
    LOG_INF("Timer configured for %d second period", TIMER_PERIOD_SEC);
    LOG_INF("LED will toggle every second...");
    
    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(10));
        
        /* Get current timer value */
        uint32_t current_ticks;
        ret = counter_get_value(timer_dev, &current_ticks);
        if (ret == 0) {
            uint64_t current_us = counter_ticks_to_us(timer_dev, current_ticks);
            LOG_INF("Current timer value: %llu us", current_us);
        }
    }
    
    return 0;
}
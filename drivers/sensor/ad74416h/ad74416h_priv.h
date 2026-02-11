/*
* Copyright (c) 2026 William Fish (Manulytica ltd)
*
* SPDX-License-Identifier: Apache-2.0
*/
#ifndef ZEPHYR_DRIVERS_SENSOR_AD74416H_PRIV_H_
#define ZEPHYR_DRIVERS_SENSOR_AD74416H_PRIV_H_

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

/* Register Map (Table 39) */
#define AD74416H_REG_NOP                0x00
#define AD74416H_REG_CH_FUNC_SETUP(n)   (0x01 + (n * 0x0C))
#define AD74416H_REG_ADC_CONFIG(n)      (0x02 + (n * 0x0C))
#define AD74416H_REG_DIN_CONFIG0(n)     (0x03 + (n * 0x0C))
#define AD74416H_REG_OUTPUT_CONFIG(n)   (0x05 + (n * 0x0C))
#define AD74416H_REG_RTD_CONFIG(n)      (0x06 + (n * 0x0C))
#define AD74416H_REG_DO_EXT_CONFIG(n)   (0x08 + (n * 0x0C))
#define AD74416H_REG_DAC_CODE(n)        (0x0A + (n * 0x0C))
#define AD74416H_REG_ADC_CONV_CTRL      0x39
#define AD74416H_REG_WDT_CONFIG         0x3B
#define AD74416H_REG_DIN_COMP_OUT       0x3E
#define AD74416H_REG_ALERT_STATUS       0x3F
#define AD74416H_REG_ADC_RESULT_UPR(n)  (0x41 + (n * 2))
#define AD74416H_REG_ADC_RESULT(n)      (0x42 + (n * 2))
#define AD74416H_REG_BURST_READ_SEL     0x6F
#define AD74416H_REG_SILICON_REV        0x7B

struct ad74416h_cal {
    double gain;
    double offset;
};

struct ad74416h_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec adc_rdy_gpio;
    struct gpio_dt_spec alert_gpio;
    bool adaptive_power;
    uint32_t wdt_val;
};

struct ad74416h_data {
    const struct device *dev;
    struct k_mutex lock;
    struct k_sem adc_sem;
    int32_t raw_results[4];
    struct ad74416h_cal cal[4];
    struct gpio_callback adc_rdy_cb;
    struct gpio_callback alert_cb;
};

#endif

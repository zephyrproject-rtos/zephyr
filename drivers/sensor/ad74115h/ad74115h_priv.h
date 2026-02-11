/*
* Copyright (c) 2026 William Fish (Manulytica ltd)
*
* SPDX-License-Identifier: Apache-2.0
*/
#ifndef ZEPHYR_DRIVERS_SENSOR_AD74115H_PRIV_H_
#define ZEPHYR_DRIVERS_SENSOR_AD74115H_PRIV_H_

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define AD74115H_REG_CH_FUNC_SETUP    0x01
#define AD74115H_REG_ADC_CONFIG       0x02
#define AD74115H_REG_OUTPUT_CONFIG    0x06
#define AD74115H_REG_RTD_CONFIG       0x07
#define AD74115H_REG_DO_EXT_CONFIG    0x09
#define AD74115H_REG_DAC_CODE         0x0B
#define AD74115H_REG_CHARGE_PUMP      0x3A
#define AD74115H_REG_ADC_CONV_CTRL    0x3B
#define AD74115H_REG_DIAG_ASSIGN      0x3C
#define AD74115H_REG_ALERT_STATUS     0x41
#define AD74115H_REG_ADC_RESULT1      0x44
#define AD74115H_REG_ADC_RESULT2      0x46
#define AD74115H_REG_ADC_DIAG_RES(n)  (0x53 + n)
#define AD74115H_REG_READ_SELECT      0x64
#define AD74115H_REG_SILICON_REV      0x7B

struct ad74115h_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec reset_gpio;
    struct gpio_dt_spec adc_rdy_gpio;
    bool use_charge_pump;
};

struct ad74115h_data {
    struct k_mutex lock;
    struct k_sem adc_sem;
    uint16_t adc_val;
    uint16_t diag_vals[4];
    struct gpio_callback adc_rdy_cb;
    double gain_cal;
    double offset_cal;
};

#endif

/*
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 * 
 * LPC54S018 PWM Sample Application
 * Demonstrates PWM control using SCTimer
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_sample, LOG_LEVEL_INF);

/* PWM period in microseconds (50Hz = 20ms period) */
#define PWM_PERIOD_US    20000

/* Servo motor pulse widths */
#define SERVO_MIN_PULSE_US  1000  /* 1ms - 0 degrees */
#define SERVO_MID_PULSE_US  1500  /* 1.5ms - 90 degrees */
#define SERVO_MAX_PULSE_US  2000  /* 2ms - 180 degrees */

/* Get PWM device */
static const struct device *pwm_dev = DEVICE_DT_GET(DT_ALIAS(pwm_0));

/* Function to set PWM duty cycle as percentage */
static int pwm_set_duty_cycle(uint32_t channel, uint8_t duty_percent)
{
    uint32_t pulse_width;
    int ret;
    
    if (duty_percent > 100) {
        duty_percent = 100;
    }
    
    /* Calculate pulse width from percentage */
    pulse_width = (PWM_PERIOD_US * duty_percent) / 100;
    
    /* Set PWM */
    ret = pwm_set(pwm_dev, channel, PWM_PERIOD_US, pulse_width, 0);
    if (ret < 0) {
        LOG_ERR("Failed to set PWM on channel %d: %d", channel, ret);
        return ret;
    }
    
    LOG_INF("Channel %d: duty cycle = %d%%, pulse = %d us", 
            channel, duty_percent, pulse_width);
    
    return 0;
}

/* Function to control servo position */
static int servo_set_position(uint32_t channel, uint8_t angle)
{
    uint32_t pulse_width;
    int ret;
    
    if (angle > 180) {
        angle = 180;
    }
    
    /* Map angle (0-180) to pulse width (1000-2000 us) */
    pulse_width = SERVO_MIN_PULSE_US + 
                  ((SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * angle) / 180;
    
    /* Set PWM */
    ret = pwm_set(pwm_dev, channel, PWM_PERIOD_US, pulse_width, 0);
    if (ret < 0) {
        LOG_ERR("Failed to set servo on channel %d: %d", channel, ret);
        return ret;
    }
    
    LOG_INF("Servo channel %d: angle = %d degrees, pulse = %d us", 
            channel, angle, pulse_width);
    
    return 0;
}

int main(void)
{
    int ret;
    uint8_t duty = 0;
    uint8_t angle = 0;
    bool increasing = true;
    
    LOG_INF("PWM sample application for LPC54S018");
    
    /* Check if PWM device is ready */
    if (!device_is_ready(pwm_dev)) {
        LOG_ERR("PWM device not ready");
        return -ENODEV;
    }
    
    LOG_INF("PWM device ready, starting demonstration");
    
    /* Initialize all channels to 0% */
    for (int i = 0; i < 6; i++) {
        pwm_set_duty_cycle(i, 0);
    }
    
    k_sleep(K_SECONDS(1));
    
    while (1) {
        /* Demonstrate LED dimming on channel 0 */
        pwm_set_duty_cycle(0, duty);
        
        /* Demonstrate servo control on channel 1 */
        servo_set_position(1, angle);
        
        /* Demonstrate motor speed control on channel 2 */
        pwm_set_duty_cycle(2, duty);
        
        /* Update values */
        if (increasing) {
            duty += 5;
            angle += 9;
            if (duty >= 100) {
                duty = 100;
                angle = 180;
                increasing = false;
            }
        } else {
            if (duty >= 5) {
                duty -= 5;
            } else {
                duty = 0;
            }
            if (angle >= 9) {
                angle -= 9;
            } else {
                angle = 0;
            }
            if (duty == 0) {
                increasing = true;
            }
        }
        
        k_sleep(K_MSEC(100));
    }
    
    return 0;
}
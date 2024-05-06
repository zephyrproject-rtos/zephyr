#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>

/* pwmcfg Bit Offsets */
#define PWM_0 0
#define PWM_1 1
#define PWM_2 2
#define PWM_3 3
#define PWM_4 4
#define PWM_5 5
#define PWM_6 6
#define PWM_7 7

void main() {

    // const struct device *dev;
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pwm0));

    // static int pwm_shakti_set_cycles(const struct device *dev, uint32_t channel,uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
    pwm_set_cycles(dev,PWM_0, 10, 7, 1);
    // pwm_set_cycles(dev,PWM_1, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_2, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_3, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_4, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_5, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_6, 1000, 750, 1);
    // pwm_set_cycles(dev,PWM_7, 1000, 750, 1);
}



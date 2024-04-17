#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>

#define PWM_0 0

void main() {

    // const struct device *dev;
	printf("\n...........................................");
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pwm0));

    // pinmux_enable_pwm(PWM_0);
    // printf("Pinmux Set");
    // pwm_shakti_init(dev,PWM_0);
    // pwm_configure(dev,PWM_0, 10000, 5000, no_interrupt, 0x1, false);
    // pwm_set_control(dev,PWM_0, 4);
    // pwm_start(dev,PWM_0);
    int x = pwm_set_cycles(dev,PWM_0, 10000, 5000, 0);
}

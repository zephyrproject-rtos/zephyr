#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
#include <zephyr/drivers/pwm.h>

/* pwmcfg Bit Offsets */
#define PWM_0 0


void main()
{
    
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)); //DEVICE_DT_GET_ONE(shakti_gpio0) This can also be used to define the device.
    const struct device *dev1 = DEVICE_DT_GET(DT_NODELABEL(pwm0));
    int ret;
    //ret = gpio_pin_configure(dev, 9, 0);
    ret = gpio_pin_configure(dev, 2, 1);
    pwm_set_cycles(dev1,PWM_0, 100, 50, PWM_POLARITY_NORMAL);
    ret = gpio_port_set_bits_raw(dev,2);
    while(1)
    {
    for(int i=0;i<10000;i++);
    ret = gpio_port_toggle_bits(dev, 2);
    
    }
}
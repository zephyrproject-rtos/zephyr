#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>



void main(){
    
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)); //DEVICE_DT_GET_ONE(shakti_gpio0) This can also be used to define the device.
    
    const struct device *dev2 = DEVICE_DT_GET(DT_NODELABEL(gc9x01_display));

    int ret;
    ret = gpio_pin_configure(dev, 9, 1);
    ret = gpio_pin_configure(dev, 10, 1);
    ret = gpio_pin_configure(dev, 11, 1);

    while (1){
        printf("set\n");
        ret = gpio_port_toggle_bits(dev, 9);
        ret = gpio_port_toggle_bits(dev, 10);
        ret = gpio_port_toggle_bits(dev, 11);
    }
}
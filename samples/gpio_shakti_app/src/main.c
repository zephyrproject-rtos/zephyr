#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/gpio_shakti.h>

void main(){
    
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)); //DEVICE_DT_GET_ONE(shakti_gpio0) This can also be used to define the device.
    printk("Hello World! %s\n", CONFIG_BOARD);
    gpio_shakti_init(dev);
    gpio_shakti_pin_configure(dev, 1, GPIO_OUTPUT);
    gpio_shakti_pin_set_raw(dev, 1, 1);

    // while(1){
    //     gpio_shakti_pin_set_raw(dev, 1, 1);
    //     for (int i=0; i<20; i++){}
    //     gpio_shakti_pin_clear_raw(dev, 1);
    // }
}
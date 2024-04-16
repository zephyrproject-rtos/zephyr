//#include "/home/kishore/work/zephyrproject/zephyr/include/zephyr/drivers/gpio/gpio_shakti.h"

#include <zephyr/kernel.h>
//#include "zephyr/gpio_shakti.h"
#include "/home/kishore/work/zephyr/drivers/gpio/gpio_shakti.c"

#define GPIO_OUTPUT 1
#define GPIO0  (1 <<  0)
#define GPIO1  (1 <<  1)


void main(){
    
    const struct device *dev;
    printk("*******Hello World!******** %s\n", CONFIG_ARCH);
    gpio_shakti_init(dev);
    printk("*******Hello World!******** %s\n", CONFIG_ARCH);
    gpio_shakti_pin_configure(dev, GPIO0, GPIO_OUTPUT);
    gpio_shakti_pin_configure(dev, GPIO1, GPIO_OUTPUT);

    while(1){
        gpio_shakti_pin_set_raw(dev, GPIO0, 1);
        gpio_shakti_pin_set_raw(dev, GPIO1, 1);
        
        for (int i=0; i<20; i++){}
        
        gpio_shakti_pin_clear_raw(dev, GPIO0);
        gpio_shakti_pin_clear_raw(dev, GPIO1);
    }
}

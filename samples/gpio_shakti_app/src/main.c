#include <zephyr/kernel.h>
#include <stdio.h>
// #include <zephyr/drivers/gpio/gpio_shakti.h>
#include <zephyr/drivers/gpio/gpio_shakti.h>
#include "../drivers/gpio/gpio_shakti.c"

void main(){
    
    const struct device *dev;
    printk("Hello World! %s\n", CONFIG_BOARD);
    gpio_shakti_init(dev);
    printk("Hello World! %s\n", CONFIG_BOARD);
    gpio_shakti_pin_configure(dev, GPIO0, GPIO_OUTPUT);
    printk("*************\n");

    while(1){
        gpio_shakti_pin_set_raw(dev, GPIO0, 1);
        for (int i=0; i<20; i++){}
        gpio_shakti_pin_clear_raw(dev, GPIO0);
    }
}
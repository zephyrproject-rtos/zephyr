#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>

#define GPIO_PIN 2 
#define GPIO_PIN_FLAG 0

// struct _isr_table_entry _sw_isr_table[58];
// enum gpio_int_mode gpio_mode;
// enum gpio_int_trig gpio_trig;

// struct _isr_table_entry isr_param;

int gpio_mode = GPIO_INT_MODE_LEVEL;
int gpio_trig = GPIO_INT_TRIG_HIGH;

void gpio_application_isr(const void*)
{
    printk("Entered ISR from application side\n");
    // return 0;
}

// _sw_isr_table[3] = gpio_application_isr;

int main()
{   
    struct _isr_table_entry isr_param = {
        .isr = gpio_application_isr
    };
    _sw_isr_table[1] = isr_param;
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    //plic_init(dev);
    gpio_pin_configure(dev, GPIO_PIN, GPIO_PIN_FLAG);
    gpio_pin_interrupt_configure(dev, GPIO_PIN, 1);
    
    while(1)
    {
        printk("Testing GPIO Interrupt\n");
    }

    return 0;
}
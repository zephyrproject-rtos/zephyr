#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>`
#include <zephyr/drivers/gpio.h>
#define I2C_NODE DT_NODELABEL(hdc2010)
#include "hdc2010.h"
// config reg
// config reg

#define SW0_NODE DT_ALIAS(sw5)
int temp;
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct i2c_dt_spec hdc2010 = I2C_DT_SPEC_GET(I2C_NODE);

volatile int dataReady = 0;
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
        dataReady = 1;
        printk("interrupt triggered\n");
}
int main(void)
{
        int ret;
        if (!device_is_ready(button.port))
        {
                printk("inti problem\n");
                return 0;
        }
        //    static const struct i2c_dt_spec hdc2010 = I2C_DT_SPEC_GET(I2C_NODE);
        if (!device_is_ready(hdc2010.bus))
        {
                printk("device is not ready\n");
                return 0;
        }

        hdc2010Init(&hdc2010);

        k_msleep(3);
        printk("init completed\n");
        ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
        ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
        if (ret)
        {
                printk("error\n");
                return 0;
        }
        gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
        gpio_add_callback(button.port, &button_cb_data);
        int temp;
        uint8_t tempReg[2] = {MEASUREMENT_CONFIG, 0};
        uint8_t oldConfig;
        ret = i2c_write_read_dt(&hdc2010, &tempReg[0], 1, &oldConfig, 1);
        if (ret)
        {
                printk("writing in reg problem\n");
                return -1;
        }
        printk("%x measurement\n", oldConfig);
        tempReg[0] = CONFIG_REG;

        ret = i2c_write_read_dt(&hdc2010, &tempReg[0], 1, &oldConfig, 1);
        if (ret)
        {
                printk("writing in reg problem\n");
                return -1;
        }

        printk("%x config\n", oldConfig);
        if (ret)
        {
                printk("error\n");
                return 0;
        }
        hdc2010Init(&hdc2010);

        k_msleep(3);
        hdc_enable_interrupt(&hdc2010);
        ret = setThresholdTempMax(&hdc2010, 110);
        ret = setThresholdTempMin(&hdc2010, 110);

        hdc2010_enableThresMaxInterrupt(&hdc2010);
        hdc2010_enableThresMinInterrupt(&hdc2010);

        hdc2010_setInterruptPolarity(&hdc2010, 1);
        while (1)
        {

                if (dataReady)
                {
                        Hdc2010ReadingTemperature(&hdc2010, &temp);
                        dataReady = 0;
                }
        }
        return 0;
}

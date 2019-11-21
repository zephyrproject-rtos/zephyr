
#include <zephyr.h>
#include <drivers/gpio.h>

#define LED_PORT  DT_ALIAS_STATUS_LED_GPIOS_CONTROLLER
#define LED_PIN   DT_ALIAS_STATUS_LED_GPIOS_PIN

#define BTN_PORT  DT_ALIAS_KEY_IN_GPIOS_CONTROLLER
#define BTN_PIN   DT_ALIAS_KEY_IN_GPIOS_PIN

int main(void)
{
    struct device *dev_led = device_get_binding(LED_PORT);
    gpio_pin_configure(dev_led, LED_PIN, GPIO_DIR_OUT);

    struct device *dev_btn = device_get_binding(BTN_PORT);
    gpio_pin_configure(dev_btn, BTN_PIN, GPIO_DIR_IN);

    while (1)
    {
        // button is pressed ==> turn on status LED
        u32_t val = 0U;
        gpio_pin_read(dev_btn, BTN_PIN, &val);
        gpio_pin_write(dev_led, LED_PIN, val);

        // dont burn the CPU
        k_sleep(10);
    }

    return 0;
}


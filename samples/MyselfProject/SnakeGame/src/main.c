#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "drivers/lcd_display.h"


#define LED0_NODE DT_ALIAS(led0)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)

void main(void)
{
    #if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7735), okay)
    #define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))
    #else 
    #define DISPLAY_DEV_NAME ""
    #pragma message("sitronix_st7735 not find")
    #endif

    // const struct device *dev;
    
	// dev = device_get_binding(LED0);
	// gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	// gpio_pin_set(dev, PIN, 1);



    const struct device* display_dev=device_get_binding(DISPLAY_DEV_NAME);
    st7735_LcdInit(display_dev);
    while(1)
    {
        // gpio_pin_set(dev, PIN, 1);
        // k_msleep(20);
        // gpio_pin_set(dev, PIN, 0);
        // k_msleep(20);
    }
}
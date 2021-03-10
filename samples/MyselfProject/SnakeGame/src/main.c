#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "drivers/lcd_display.h"




void main(void)
{
    #if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7735), okay)
    #define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))
    #else 
    #define DISPLAY_DEV_NAME ""
    #pragma message("sitronix_st7735 not find")
    #endif

    const struct device* display_dev=device_get_binding(DISPLAY_DEV_NAME);
    st7735_LcdInit(display_dev);
    while(1)
    {
        // printk("zephyr is low\n");
        // high(display_dev);
        // k_msleep(500);
        // printk("zephyr is high\n");
        // low(display_dev);
    }
}
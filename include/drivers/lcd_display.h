#ifndef __LCD_DISPLAY_H__
#define __LCD_DISPLAY_H__
#include <device.h>
struct lcd_display_driver_api
{
    void (*st7735_lcd_init)(void);
    void (*high)(void);
    void (*low)(void);
};




static inline void st7735_LcdInit(const struct device *dev)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->st7735_lcd_init();
}


static inline void high(const struct device *dev)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->high();
}

static inline void low(const struct device *dev)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->low();
}
#endif 


#ifndef __LCD_DRIVER_API_H__
#define __LCD_DRIVER_API_H__
#include <device.h>
#include <sys/byteorder.h>

struct lcd_driver_api
{
    void (*write)(const struct device* dev, uint8_t cmd,const uint8_t *tx_data,uint8_t tx_cnt);
};


static inline void Adafruit_display_write(const struct device* dev, uint8_t cmd,const uint8_t *tx_data,uint8_t tx_cnt)
{
    struct lcd_driver_api *api =(struct lcd_driver_api *)dev->api;
    api->write(dev, cmd, tx_data, tx_cnt);
}

#endif
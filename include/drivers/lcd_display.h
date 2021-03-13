#ifndef __LCD_DISPLAY_H__
#define __LCD_DISPLAY_H__
#include <device.h>

// 颜色定义， 移植时不用修改
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色 
#define LIGHTGREEN     	 0X841F //浅绿色
#define LIGHTGRAY        0XEF5B //浅灰色(PANNEL)
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)



struct lcd_display_driver_api
{
    void (*st7735_lcd_init)(void);
    void (*writeAscii)(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint32_t fColor, uint32_t bColor);
    void (*showclear)(uint32_t color);
    void (*lcd_fill_area)(uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend,uint16_t color);
};

static inline void st7735_lcdclear(struct device *dev,uint32_t color)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->showclear(color);
}
static inline void st7735_LcdInit(struct device *dev)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->st7735_lcd_init();
}


static inline void st7735_drawAscii(struct device *dev,uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint32_t fColor, uint32_t bColor)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->writeAscii(x,y,num,size,fColor,bColor);
}

static inline void st7735_fill_area(struct device *dev,uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend,uint16_t color)
{
    struct lcd_display_driver_api *api=(struct lcd_display_driver_api *)dev->api;
    api->lcd_fill_area(xstart,ystart,xend,yend,color);
}
#endif 
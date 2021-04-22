
#ifndef __DISPLAY_ST7735_H__
#define __DISPLAY_ST7735_H__




#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/display.h>
#include <devicetree.h>



struct st7735_data
{
    const struct device* spi_dev;
    struct spi_config spi_config;
    struct spi_cs_control cs_ctrl;

    const struct device* reset_gpio;
    const struct device* cmd_data_gpio;
    const struct device* blk_gpio;

    uint16_t height;
    uint16_t width;

};



//128*160

#define LCD_WIDTH         129               // 设置屏幕的像素大小，
#define LCD_HIGH          160               // 注意：实际像素好像要比标示值大1~2像素，
#define LCD_DIR           1                 // 四种显示方向，参数：1、2、3、4

#define ST7735_CMD_SLEEP_OUT    0x11
#define ST7735_CMD_RGBCTRL      0xB1
#define ST7735_CMD_PORCTRL      0xB2
#define ST7735_CMD_B3           0xB3
#define ST7735_CMD_B4           0xB4
#define ST7735_CMD_LCMCTRL      0xC0
#define ST7735_CMD_C1           0xC1
#define ST7735_CMD_VDVVRHEN     0xC2
#define ST7735_CMD_VRH          0xC3
#define ST7735_CMD_VDS          0xC4
#define ST7735_CMD_C5           0xC5
#define ST7735_CMD_MADCTL       0x36
#define ST7735_CMD_PVGAMCTRL    0xE0
#define ST7735_CMD_NVGAMCTRL    0xE1
#define ST7735_CMD_CASET        0x2A
#define ST7735_CMD_RASET        0x2B
#define ST7735_CMD_COLMOD       0x3A
#define ST7735_CMD_DISP_ON      0x29

#define ST7735_CMD_RAMWR        0x2C

#define ST7735_CMD_NULL         0

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


#define read_byte(addr) (*(const unsigned char *)(addr))

#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

                              
#endif
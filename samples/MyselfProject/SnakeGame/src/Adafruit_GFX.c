/**
 * 图形库里用来实现直线,圆等
 * 调用驱动里面的写函数
*/
#include <sys/byteorder.h>
#include "drivers/lcd_driver_api.h"

#include <devicetree.h>
// #include "display_st7735.h"
#include "/home/shuiyihang/zephyrproject/zephyr/drivers/display/display_st7735.h"

#include "Adafruit_GFX_api.h"

#define DT_DRV_COMPAT sitronix_st7735
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))


// #define adafruit_abs(a)         
//                 ({((a)<0)?(-(a)):(a);})

inline int adafruit_abs(int a)
{
    return a<0?-a:a;
}


static const uint8_t
    initcmd[] ={
        16,
        ST7735_CMD_RGBCTRL, 3,
        0x05,
        0x3C,
        0x3C,
        ST7735_CMD_PORCTRL, 3,
        0x05,
        0x3C,
        0x3C,
        ST7735_CMD_B3,  6,
        0x05,0x3C,0x3C,0x05,0x3C,0x3C,
        ST7735_CMD_B4,  1,
        0x03,
        ST7735_CMD_LCMCTRL, 3,
        0x28,0x08,0x04,
        ST7735_CMD_C1,  1,
        0xC0,
        ST7735_CMD_VDVVRHEN,    2,
        0x0D,0x00,
        ST7735_CMD_VRH,   2,
        0x8D,0x2A,
        ST7735_CMD_VDS,   2,
        0x8D,0xEE,
        ST7735_CMD_C5,  1,
        0x1A,
        ST7735_CMD_MADCTL,  1,
        0xC0,
        ST7735_CMD_PVGAMCTRL,  16, 
        0x04,0x22,0x07,0x0A,0x2E,
        0x30,0x25,0x2A,0x28,0x26,
        0x2E,0x3A,0x00,0x01,0x03,
        0x13,
        ST7735_CMD_NVGAMCTRL,  16,
        0x04,0x16,0x06,0x0D,0x2D,
        0x26,0x23,0x27,0x27,0x25,
        0x2D,0x3B,0x00,0x01,0x04,
        0x13, 
        ST7735_CMD_CASET,   4,
        0,0,0,0x7f,
        ST7735_CMD_RASET,   4,
        0,0,0,0x9f,
        ST7735_CMD_COLMOD,  1,
        0x05,

    };




const struct device* display_dev;



void Adafruit_displayInit(uint8_t options)//addr指向的值不可修改,但是addr可以修改    const uint8_t *addr
{
    uint8_t numCmds, cmd, numArgs;

    const uint8_t *addr;
    if(options == INIT_ST7735){
        addr = initcmd;
    }

    display_dev=device_get_binding(DISPLAY_DEV_NAME);
    if(!display_dev){
        printk("displaly is null\n");
        return;
    }
    Adafruit_display_write(display_dev,ST7735_CMD_SLEEP_OUT,NULL,0);
    k_sleep(K_MSEC(100));//唤醒屏幕

    numCmds = read_byte(addr++);
    while(numCmds--){
        cmd = read_byte(addr++);
        numArgs = read_byte(addr++);
        // printk("cmd:%d==numargs:%d\n",cmd,numArgs);
        Adafruit_display_write(display_dev,cmd,addr,numArgs);
        addr += numArgs;
    }
    Adafruit_display_write(display_dev,ST7735_CMD_DISP_ON,NULL,0);
}



static void __setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint32_t xa = ((uint32_t)x << 16) | (x + w - 1);
    uint32_t ya = ((uint32_t)y << 16) | (y + h - 1);
    uint8_t CASET_DATA[] = {xa>>24, xa>>16, xa>>8, xa};
    uint8_t RASET_DATA[] = {ya>>24, ya>>16, ya>>8, ya};
    Adafruit_display_write(display_dev,ST7735_CMD_CASET,CASET_DATA,sizeof(CASET_DATA));

    Adafruit_display_write(display_dev,ST7735_CMD_RASET,RASET_DATA,sizeof(RASET_DATA));

    Adafruit_display_write(display_dev,ST7735_CMD_RAMWR,NULL,0);
}


/**
 * 画点
*/
void Adafruit_drawPixel(int x, int y, uint16_t color)
{
    uint8_t COLOR_DATA[] = {color>>8,color}; 
    if(!display_dev){
        return;
    }
    struct st7735_data* st7735_info = (struct st7735_data*)display_dev->data;
    if((x >= 0)&&(x <=st7735_info->width)&&(y >=0)&&(y <=st7735_info->height)){

        __setAddrWindow(x,y,1,1);
        Adafruit_display_write(display_dev,ST7735_CMD_NULL,COLOR_DATA,sizeof(COLOR_DATA));
    }
}

/**
 * 画线,应该与下层的画点解耦
*/

void Adafruit_drawLine(int16_t x0, int16_t x1, int16_t y0, int16_t y1, uint16_t color)
{

    int16_t steep = adafruit_abs(y1 - y0) > adafruit_abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = adafruit_abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
        Adafruit_drawPixel(y0, x0, color);
        } else {
        Adafruit_drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
        y0 += ystep;
        err += dx;
        }
    }
}


void Adafruit_drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color)
{
    Adafruit_drawLine(x, x, y, y + h - 1, color);
}

void Adafruit_drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color)
{
    Adafruit_drawLine(x, x + w - 1, y, y, color);
}
/**
 * 无填充画圆
*/
void Adafruit_drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    Adafruit_drawPixel(x0, y0 + r, color);
    Adafruit_drawPixel(x0, y0 - r, color);
    Adafruit_drawPixel(x0 + r, y0, color);
    Adafruit_drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        Adafruit_drawPixel(x0 + x, y0 + y, color);
        Adafruit_drawPixel(x0 - x, y0 + y, color);
        Adafruit_drawPixel(x0 + x, y0 - y, color);
        Adafruit_drawPixel(x0 - x, y0 - y, color);
        Adafruit_drawPixel(x0 + y, y0 + x, color);
        Adafruit_drawPixel(x0 - y, y0 + x, color);
        Adafruit_drawPixel(x0 + y, y0 - x, color);
        Adafruit_drawPixel(x0 - y, y0 - x, color);
    }
}



/**
 * 1-->2
 * |   |
 * 8<--4
*/

static void Adafruit_drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
                                    uint8_t cornername, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
        Adafruit_drawPixel(x0 + x, y0 + y, color);
        Adafruit_drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
        Adafruit_drawPixel(x0 + x, y0 - y, color);
        Adafruit_drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
        Adafruit_drawPixel(x0 - y, y0 + x, color);
        Adafruit_drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
        Adafruit_drawPixel(x0 - y, y0 - x, color);
        Adafruit_drawPixel(x0 - x, y0 - y, color);
        }
  }
}
/**
 * 无填充画圆角矩形
*/

void Adafruit_drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    Adafruit_drawFastHLine(x + r, y, w - 2 * r, color);         // Top
    Adafruit_drawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
    Adafruit_drawFastVLine(x, y + r, h - 2 * r, color);         // Left
    Adafruit_drawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
    // draw four corners
    Adafruit_drawCircleHelper(x + r, y + r, r, 1, color);
    Adafruit_drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    Adafruit_drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    Adafruit_drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/**
 * 无填充矩形
*/
void Adafruit_drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color)
{
    Adafruit_drawFastHLine(x, y, w, color);
    Adafruit_drawFastHLine(x, y + h - 1, w, color);
    Adafruit_drawFastVLine(x, y, h, color);
    Adafruit_drawFastVLine(x + w - 1, y, h, color);
}

/**
 * 无填充三角形
*/
void Adafruit_drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color)
{
    Adafruit_drawLine(x0, y0, x1, y1, color);
    Adafruit_drawLine(x1, y1, x2, y2, color);
    Adafruit_drawLine(x2, y2, x0, y0, color);
}

//填充矩形
void Adafruit_fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color)
{
    for (int16_t i = x; i < x + w; i++) {
    Adafruit_drawFastVLine(i, y, h, color);
  }
}

void Adafruit_clear(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color)
{
    uint32_t pixel=(w-x+1)*(h-y+1);
    uint8_t COLOR_DATA[] = {color>>8,color}; 
    if(!display_dev){
        return;
    }
    struct st7735_data* st7735_info = (struct st7735_data*)display_dev->data;
    if((x >= 0)&&(x <= st7735_info->width)&&(y >=0)&&(y <= st7735_info->height)){

        __setAddrWindow(x,y,w-x+1,h-y+1);
        while(pixel>0)
        {
            pixel--;
            Adafruit_display_write(display_dev,ST7735_CMD_NULL,COLOR_DATA,sizeof(COLOR_DATA));
        }
    }

}

void Adafruit_fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
                                    uint8_t corners, int16_t delta,
                                    uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++; // Avoid some +1's in the loop

    while (x < y) {
        if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if (x < (y + 1)) {
        if (corners & 1)
            Adafruit_drawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
        if (corners & 2)
            Adafruit_drawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
        if (corners & 1)
            Adafruit_drawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
        if (corners & 2)
            Adafruit_drawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
        py = y;
        }
        px = x;
    }
}

//填充圆角矩形

void Adafruit_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    Adafruit_fillRect(x + r, y, w - 2 * r, h, color);
    // draw four corners
    Adafruit_fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    Adafruit_fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}










//填充圆


void Adafruit_fillCircle(int16_t x0, int16_t y0, int16_t r,
                              uint16_t color)
{
    Adafruit_drawFastVLine(x0, y0 - r, 2 * r + 1, color);//竖着画一条线
    Adafruit_fillCircleHelper(x0, y0, r, 3, 0, color);
}                              

//填充三角

void Adafruit_fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                int16_t x2, int16_t y2, uint16_t color)
{

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1);
        _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }

    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
        a = x1;
        else if (x1 > b)
        b = x1;
        if (x2 < a)
        a = x2;
        else if (x2 > b)
        b = x2;
        Adafruit_drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
            dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1; // Include y1 scanline
    else
        last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
        _swap_int16_t(a, b);
        Adafruit_drawFastHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
        _swap_int16_t(a, b);
        Adafruit_drawFastHLine(a, y, b - a + 1, color);
    }
}


//字符,数字,图片
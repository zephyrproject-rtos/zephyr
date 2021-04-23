#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdlib.h>
#include "drivers/lcd_display.h"

#include "snake.h"
#include "Adafruit_GFX_api.h"
// #include "drivers/display/Adafruit_GFX_api.h"
// #include "drivers/display/display_st7735.h"

#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x"="PRINT_MACRO_HELPER(x)

struct k_msgq input_key_message;
// uint16_t message_buffer[10];

// Snake littlesnake;
// food  delicious_fd;

void main(void)
{
    // uint16_t key_value=0;


    //消息队列初始化
    // k_msgq_init(&input_key_message, message_buffer, sizeof(uint16_t), 10);

    //屏幕初始化
    const struct device* display_dev=device_get_binding("ST7735");

    #pragma message(PRINT_MACRO(DISPLAY_DEV_NAME))

    if(!display_dev){
        printk("===display dev is so null===\n");
    }
    // st7735_LcdInit(display_dev);
    Adafruit_displayInit(INIT_ST7735);
    for(int i=3;i<120;i+=3)
        Adafruit_drawLine(0,i,0,150,BLACK);

    while (1)
    {
        
    }
    
    
    //蛇初始化
    // SnakeInit(display_dev,&littlesnake,&delicious_fd);

    // st7735_drawAscii(display_dev,10,10,0x41,24,WHITE,GREEN);
    // st7735_drawAscii(display_dev,40,10,0x1A,24,WHITE,GREEN);
    // map_init();
    // while(1)
    // {
    //     // while(!k_msgq_get(&input_key_message, &key_value, K_NO_WAIT))
    //     // {
    //     //     printk("key_value:%0x\n",key_value);
    //     //     st7735_drawAscii(display_dev,10,10+cnt*30,'0'+((key_value>>8)&0xff),24,BLACK,WHITE);
    //     //     st7735_drawAscii(display_dev,40,10+cnt*30,'0'+((key_value)&0xff),24,BLACK,WHITE);
    //     //     cnt++;
    //     // }
    //     // cnt=0;
        
    //     k_msgq_get(&input_key_message, &key_value, K_NO_WAIT);
    //     updata_control_input(&littlesnake,key_value);
    //     key_value=0;
    //     if(SnakeAutoMove(display_dev,&littlesnake,&delicious_fd))
    //     {
    //         CreatFood(display_dev,&littlesnake,&delicious_fd);
    //         k_sleep(K_MSEC(200));
    //     }
    //     while(1);
    //     // if(SnakeMove(display_dev,&littlesnake,&delicious_fd))
    //     // {
    //     //     CreatFood(display_dev,&littlesnake,&delicious_fd);
    //     //     k_sleep(K_MSEC(200));
    //     // }else
    //     // {
    //     //     st7735_LcdInit(display_dev);
    //     //     SnakeInit(display_dev,&littlesnake,&delicious_fd);
    //     // }
        
        
    // }
}
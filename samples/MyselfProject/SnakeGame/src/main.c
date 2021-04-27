#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdlib.h>
#include "drivers/lcd_display.h"

#include "snake.h"
#include "Adafruit_GFX_api.h"
#include "coll_ball.h"
// #include "drivers/display/Adafruit_GFX_api.h"
// #include "drivers/display/display_st7735.h"

#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x"="PRINT_MACRO_HELPER(x)

struct k_msgq input_key_message;
void main(void)
{

    #if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7735), okay)
    #define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))
    #else 
    #define DISPLAY_DEV_NAME ""
    #pragma message("sitronix_st7735 not find")
    #endif

    #pragma message(PRINT_MACRO(DISPLAY_DEV_NAME))


    Adafruit_displayInit(INIT_ST7735);
    Adafruit_clear(0,0,129,160,WHITE);

    // LIST_HEAD(list_handle);

    struct coll_ball ball[3];
    rand_init_ball(&ball[0]);
    printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[0].x,ball[0].y,ball[0].r,ball[0].speed_x,ball[0].speed_y);
    rand_init_ball(&ball[1]);
    printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[1].x,ball[1].y,ball[1].r,ball[1].speed_x,ball[1].speed_y);
    rand_init_ball(&ball[2]);
    printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[2].x,ball[2].y,ball[2].r,ball[2].speed_x,ball[2].speed_y);
    // Adafruit_fillRect(0,0,129,160,WHITE);
    // for(int i=3;i<120;i+=10)
    //     Adafruit_drawLine(0,i,0,150,GREEN);
    
    // Adafruit_drawCircle(60,60,20,RED);

    // Adafruit_fillRoundRect(20,60,80,30,15,BLUE);
    // for(int x=4,w=120;x<=w;x+=4)
    // Adafruit_drawRect(x,x,w-2*x,w-2*x,RED);//这个错误竟然产生了漂亮的效果
    int cnt=0;
    while (1)
    {
        for(int i=0;i<3;i++){
            hit_wall_detect(&ball[i]);
        }
        Adafruit_clear(0,0,129,160,WHITE);
        for(int i=0;i<3;i++){
            Adafruit_drawCircle(ball[i].x,ball[i].y,ball[i].r,BLUE);
        }
        if(cnt>20){
            for(int i=0;i<3;i++){
                for (int j=i+1;j<3;j++){
                    hit_body_detect(&ball[i],&ball[j]);
                }
            }
        }else{
            cnt++;
        }
        // k_sleep(K_MSEC(10));
        
    }
    
    

}
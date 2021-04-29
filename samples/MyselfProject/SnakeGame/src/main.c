#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <stdlib.h>
#include "drivers/lcd_display.h"

#include "snake.h"
#include "Adafruit_GFX_api.h"
#include "coll_ball.h"
#include "drivers/thread_timer.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(main,LOG_LEVEL_DBG);
#define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))

#define PRINT_MACRO_HELPER(x) #x
#define PRINT_MACRO(x) #x"="PRINT_MACRO_HELPER(x)

struct k_msgq input_key_message;

static void _check_system_status(struct thread_timer *ttimer,
                                 void *expiry_fn_arg)
{
    printk("thread timer test\n");
}
void main(void)
{

    #if DT_NODE_HAS_STATUS(DT_INST(0, sitronix_st7735), okay)
    #define DISPLAY_DEV_NAME DT_LABEL(DT_INST(0, sitronix_st7735))
    #else 
    #define DISPLAY_DEV_NAME ""
    #pragma message("sitronix_st7735 not find")
    #endif

    #pragma message(PRINT_MACRO(DISPLAY_DEV_NAME))


    // Adafruit_displayInit(INIT_ST7735);
    // Adafruit_clear(0,0,129,160,WHITE);

    // // LIST_HEAD(list_handle);

    // struct coll_ball ball[3];
    // rand_init_ball(&ball[0]);
    // printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[0].x,ball[0].y,ball[0].r,ball[0].speed_x,ball[0].speed_y);
    // rand_init_ball(&ball[1]);
    // printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[1].x,ball[1].y,ball[1].r,ball[1].speed_x,ball[1].speed_y);
    // rand_init_ball(&ball[2]);
    // printk("x:%d,y:%d,r:%d,speed_x:%d,speed_y:%d\n",ball[2].x,ball[2].y,ball[2].r,ball[2].speed_x,ball[2].speed_y);

    // for(int x=4,w=120;x<=w;x+=4)
    // Adafruit_drawRect(x,x,w-2*x,w-2*x,RED);//这个错误竟然产生了漂亮的效果
    struct thread_timer status_check_timer;
    thread_timer_init(&status_check_timer,
	                  _check_system_status, NULL);
    thread_timer_start(&status_check_timer,1000,1000);
    int cnt=0;
    while (1)
    {
        if(cnt==0)
        Adafruit_clear(0,0,129,160,BLUE);
        else if(cnt==1)
            Adafruit_clear(0,0,129,160,RED);
        else
            Adafruit_clear(0,0,129,160,GREEN);
        cnt++;
        cnt=cnt%3;
        // for(int i=0;i<3;i++){
        //     hit_wall_detect(&ball[i]);
        // }
        // Adafruit_clear(0,0,129,160,WHITE);
        // for(int i=0;i<3;i++){
        //     Adafruit_drawCircle(ball[i].x,ball[i].y,ball[i].r,BLUE);
        // }
        // if(cnt>20){
        //     for(int i=0;i<3;i++){
        //         for (int j=i+1;j<3;j++){
        //             hit_body_detect(&ball[i],&ball[j]);
        //         }
        //     }
        // }else{
        //     cnt++;
        // }
        printk("%d\n",k_uptime_get_32());
        LOG_INF("main");
        thread_timer_handle_expired();
        // k_sleep(K_MSEC(10));
        
    }
    
    

}
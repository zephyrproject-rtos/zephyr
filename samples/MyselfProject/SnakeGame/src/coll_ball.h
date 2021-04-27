#ifndef __COLL_BALL_H__
#define __COLL_BALL_H__
#include <sys/byteorder.h>

// #include "Hi_list.h"
#include "Adafruit_GFX_api.h"
#include "drivers/lcd_display.h"
#define SCREEN_WIDTH    129
#define SCREEN_HEIGHT   160

#define LINK_DIS        20

#define RATE_MID        10

struct coll_ball
{
    // struct list_head clue;
    int x;
    int y;
    uint8_t r;
    int speed_x;
    int speed_y;
};

void hit_wall_detect(struct coll_ball *instance);
void speed_updata_after_coll(struct coll_ball *body1, struct coll_ball *body2);

void hit_body_detect(struct coll_ball *body1, struct coll_ball *body2);
void rand_init_ball(struct coll_ball *body);

#endif
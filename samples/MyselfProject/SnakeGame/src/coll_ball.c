
#include "coll_ball.h"
#include "stdlib.h"

static inline int math_pow(uint8_t data, uint8_t cnt)
{
    int ret=1;
    for(int i=0;i<cnt;i++){
        ret*=data;
    }
    return ret;
}
void hit_wall_detect(struct coll_ball *instance)
{
    instance->x += instance->speed_x;
    if((instance->x+instance->r)>=SCREEN_WIDTH||
        (instance->x-instance->r)<=0){
        if((instance->x+instance->r)>=SCREEN_WIDTH){
            instance->x=SCREEN_WIDTH-instance->r;
        }else{
            instance->x=instance->r;
        }
        instance->speed_x= -instance->speed_x;
    }


    instance->y += instance->speed_y;
    if((instance->y+instance->r)>=SCREEN_HEIGHT||
        (instance->y-instance->r)<=0){
        if((instance->y+instance->r)>=SCREEN_HEIGHT){
            instance->y=SCREEN_HEIGHT-instance->r;
        }else{
            instance->y=instance->r;
        }
        instance->speed_y= -instance->speed_y;
    }

}


void speed_updata_after_coll(struct coll_ball *body1, struct coll_ball *body2)
{
    //由动能守恒
    int quality_sum = body1->r + body2->r;
    int value1_x = ((body1->r - body2->r)*body1->speed_x +2*body2->r*body2->speed_x)/quality_sum;
    int value1_y = ((body1->r - body2->r)*body1->speed_y +2*body2->r*body2->speed_y)/quality_sum;
    int value2_x = ((body2->r - body1->r)*body2->speed_x +2*body1->r*body1->speed_x)/quality_sum;
    int value2_y = ((body2->r - body1->r)*body2->speed_y +2*body1->r*body1->speed_y)/quality_sum;
    body1->speed_x = value1_x;
    body1->speed_y = value1_y;
    body2->speed_x = value2_x;
    body2->speed_y = value2_y;

}

void hit_body_detect(struct coll_ball *body1, struct coll_ball *body2)
{
    int delta_x = abs(((body1->x+body1->speed_x)-(body2->x+body2->speed_x)));
    int delta_y = abs(((body1->y+body1->speed_y)-(body2->y+body2->speed_y)));
    int x_pow_2 = math_pow(delta_x,2);
    int y_pow_2 = math_pow(delta_y,2);
    if(x_pow_2+y_pow_2<math_pow((body1->r+body2->r+LINK_DIS),2)){
        Adafruit_drawLine(body1->x,body2->x,body1->y,body2->y,RED);
    }
    if(x_pow_2+y_pow_2<=math_pow((body1->r+body2->r),2)){
        speed_updata_after_coll(body1,body2);
    }
}

static inline int randint(int llimit, int hlimit)
{
    int delat = hlimit - llimit;
    int value = k_uptime_get_32()%delat;
    return value+llimit;
}
void rand_init_ball(struct coll_ball *body)
{
    body->speed_x = RATE_MID - randint(0,2*RATE_MID);
    body->speed_y = RATE_MID - randint(0,2*RATE_MID);
    body->r = randint(6,16);
    body->x = randint(0,SCREEN_WIDTH);
    body->y = randint(0,SCREEN_HEIGHT);
}
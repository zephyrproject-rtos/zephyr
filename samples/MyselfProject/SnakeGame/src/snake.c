/**
 * 食物不能产生在身子上，
 * 蛇头碰到身体任意部分=>死
 * 使用循环队列
*/


#include "snake.h"

#define KEY_RESERVED		0
#define KEY_LEFT			1
#define KEY_MIDDLE          2
#define KEY_RIGHT           3

enum{
    KEY_TYPE_SHORT_UP,
    KEY_TYPE_DOUBLE_CLICK,
    KEY_TYPE_TRIPLE_CLICK,
};


void SnakeInit(struct device* lcd_dev,Snake* S,food* fd)
{
    S->head=0;//队头是蛇尾
    S->rear=1;//队尾是蛇头

    S->body[S->head].x_pos=2;
    S->body[S->head].y_pos=2;
    S->curdir=S->lastdir=MOVE_LEFT;
    S->score=0;

    fd->exist=0;

    CreatFood(lcd_dev,S,fd);

}

uint8_t is_overlap(struct pos* Object,Snake* S)
{
    
    for(int head=S->head;head<(S->rear);head++){
        if((Object->x_pos==S->body[head].x_pos)&&(Object->y_pos==S->body[head].y_pos))
        {
            return true;
        }
    }
    return false;
}

uint8_t SnakeMove(struct device* lcd_dev,Snake* S,food* fd)
{
    int cur_head=(S->rear-1+MAX_BODY)%MAX_BODY;
    switch (S->curdir)
    {
    case MOVE_UP:
        S->body[S->rear].x_pos=S->body[cur_head].x_pos;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos-1;
        
        break;
    case MOVE_DOWN:
        S->body[S->rear].x_pos=S->body[cur_head].x_pos;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos+1;
        break;
    case MOVE_LEFT:
        S->body[S->rear].x_pos=S->body[cur_head].x_pos-1;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos;
    break;
    case MOVE_RIGHT:
        S->body[S->rear].x_pos=S->body[cur_head].x_pos+1;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos;
    
    break;
    default:
        break;
    }


    if(S->body[S->rear].x_pos<0)
    {
        S->body[S->rear].x_pos=MAX_STEP_X;
    }
    if(S->body[S->rear].x_pos>MAX_STEP_X)
    {
        S->body[S->rear].x_pos=0;
    }
    if(S->body[S->rear].y_pos>MAX_STEP_Y)
    {
        S->body[S->rear].y_pos=0;
    }
    if(S->body[S->rear].y_pos<0)
    {
        S->body[S->rear].y_pos=MAX_STEP_Y-1;
    }
    


    if(is_overlap(&(S->body[S->rear]),S))
    {
        printk("game over\n");
        return false;//游戏结束
    }

    st7735_fill_area(lcd_dev,(S->body[S->rear].x_pos)*X_STEP,(S->body[S->rear].y_pos)*Y_STEP,
    (S->body[S->rear].x_pos+1)*X_STEP,(S->body[S->rear].y_pos+1)*Y_STEP,BLUE);

    if(fd->exist&&(S->body[S->rear].x_pos==fd->position.x_pos)&&(S->body[S->rear].y_pos==fd->position.y_pos))
    {
        S->score++;
        fd->exist=0;

    }else
    {
        //队头出队
        //清屏

        st7735_fill_area(lcd_dev,(S->body[S->head].x_pos)*X_STEP,(S->body[S->head].y_pos)*Y_STEP,
        (S->body[S->head].x_pos+1)*X_STEP,(S->body[S->head].y_pos+1)*Y_STEP,GREEN);

        S->head=(S->head+1)%MAX_BODY;
    }
    S->rear=(S->rear+1)%MAX_BODY;
    return true;
}


void CreatFood(struct device* lcd_dev,Snake* S,food* fd)
{
    struct pos food_pos={
        .x_pos=k_cycle_get_32()%X_STEP,
        .y_pos=k_cycle_get_32()%Y_STEP,
    };
    if(fd->exist==0)
    {
        while(is_overlap(&food_pos,S))
        {
            food_pos.x_pos=k_cycle_get_32()%(MAX_STEP_X-1);
            food_pos.y_pos=k_cycle_get_32()%MAX_STEP_Y;
        }
        fd->exist=1;
        fd->position.x_pos=food_pos.x_pos;
        fd->position.y_pos=food_pos.y_pos;

        st7735_fill_area(lcd_dev,(fd->position.x_pos)*X_STEP,(fd->position.y_pos)*Y_STEP,
            (fd->position.x_pos+1)*X_STEP,(fd->position.y_pos+1)*Y_STEP,RED);
    }
    
    
}



void updata_control_input(Snake* S,uint16_t key_value)
{
    uint8_t keycode=(key_value>>8)&0xff;//哪一个按键
    uint8_t keytype=key_value&0xff;//单双击

    if(keycode)
    {
        S->lastdir=S->curdir;////
    }else{
        return;
    }

    if(keycode==KEY_LEFT)
    {
        if(keytype==KEY_TYPE_SHORT_UP)
        {
            
            S->curdir=MOVE_LEFT;
        }else{
            S->curdir=MOVE_UP;
        }
    }else if(keycode==KEY_RIGHT)
    {
        if(keytype==KEY_TYPE_SHORT_UP)
        {
            
            S->curdir=MOVE_RIGHT;
        }else{
            S->curdir=MOVE_DOWN;
        }
    }else if(keycode==KEY_MIDDLE)//短按暂停之类
    {

    }

}
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
    S->play_pause=1;//默认暂停

    fd->exist=0;
    st7735_drawAscii(lcd_dev,(SCREEN_SIZE_X/2+20),1,'0',24,WHITE,GREEN);
    CreatFood(lcd_dev,S,fd);
    st7735_fill_area(lcd_dev,(S->body[S->head].x_pos)*X_STEP,(S->body[S->head].y_pos)*Y_STEP,
    (S->body[S->head].x_pos+1)*X_STEP,(S->body[S->head].y_pos+1)*Y_STEP,BLUE);

}

uint8_t is_overlap(struct pos* Object,Snake* S)
{
    
    for(int head=S->head;head<(S->rear);head++){
        if((Object->x_pos==S->body[head].x_pos)&&(Object->y_pos==S->body[head].y_pos))
        {
            return true;
        }
    }
    // if(Object->x_pos<0)
    // {
    //     return true;
    // }
    // if(Object->x_pos>MAX_STEP_X)
    // {
    //     return true;
    // }
    // if(Object->y_pos>MAX_STEP_Y)//13
    // {
    //     return true;
    // }
    // if(Object->y_pos<(Forehead_HEIGHT/Y_STEP))//2
    // {
    //     return true;
    // }
    return false;
}

uint8_t SnakeMove(struct device* lcd_dev,Snake* S,food* fd)
{
    int cur_head=(S->rear-1+MAX_BODY)%MAX_BODY;
    if(S->play_pause)
        return 1;
label:
    switch (S->curdir)
    {
    case MOVE_UP:
        if(S->lastdir==MOVE_DOWN)
        {
            S->curdir=MOVE_DOWN;//如果没有这一行，就会产生暂停效果，也就是暂停功能就是从这个函数直接返回
            goto label;
        }
        S->body[S->rear].x_pos=S->body[cur_head].x_pos;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos-1;
        
        break;
    case MOVE_DOWN:
        if(S->lastdir==MOVE_UP)
        {
            S->curdir=MOVE_UP;
            goto label;
        }
        S->body[S->rear].x_pos=S->body[cur_head].x_pos;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos+1;
        break;
    case MOVE_LEFT:
        if(S->lastdir==MOVE_RIGHT)
        {
            S->curdir=MOVE_RIGHT;
            goto label;
        }
        S->body[S->rear].x_pos=S->body[cur_head].x_pos-1;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos;
    break;
    case MOVE_RIGHT:
        if(S->lastdir==MOVE_LEFT)
        {
            S->curdir=MOVE_LEFT;
            goto label;
        }
        S->body[S->rear].x_pos=S->body[cur_head].x_pos+1;
        S->body[S->rear].y_pos=S->body[cur_head].y_pos;
    
    break;
    default:
        break;
    }
    S->lastdir=S->curdir;////


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
        S->body[S->rear].y_pos=Forehead_HEIGHT/Y_STEP;
    }
    if(S->body[S->rear].y_pos<(Forehead_HEIGHT/Y_STEP))
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

        uint8_t score_high=S->score/10;
        uint8_t score_low=S->score%10;
        if(score_high==0)
        {
            st7735_drawAscii(lcd_dev,(SCREEN_SIZE_X/2+20),1,'0'+score_low,24,WHITE,GREEN);
        }else{
            st7735_drawAscii(lcd_dev,(SCREEN_SIZE_X/2+20),1,'0'+score_high,24,WHITE,GREEN);
            st7735_drawAscii(lcd_dev,(SCREEN_SIZE_X/2+44),1,'0'+score_low,24,WHITE,GREEN);
        }
        
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

uint8_t SnakeAutoMove(struct device* lcd_dev,Snake* S,food* fd)
{
    solve_game(S,fd->position,&assist);
}











void CreatFood(struct device* lcd_dev,Snake* S,food* fd)
{
    struct pos food_pos={
        .x_pos=k_cycle_get_32()%(LCD_WIDTH/TEST_SIZE),
        .y_pos=k_cycle_get_32()%(LCD_HIGH/TEST_SIZE),
    };
    if(fd->exist==0)
    {
        while(is_overlap(&food_pos,S))
        {
            food_pos.x_pos=k_cycle_get_32()%(MAX_STEP_X-1);
            food_pos.y_pos=(k_cycle_get_32()%(MAX_STEP_Y-Forehead_BYTE))+Forehead_BYTE;
        }

        printk("ypos:%d\n",food_pos.y_pos);
        fd->exist=1;
        fd->position.x_pos=food_pos.x_pos;
        fd->position.y_pos=food_pos.y_pos;

        st7735_fill_area(lcd_dev,(fd->position.x_pos)*X_STEP,(fd->position.y_pos)*Y_STEP,
            (fd->position.x_pos+1)*X_STEP,(fd->position.y_pos+1)*Y_STEP,RED);
    }
    
    
}


// void CreatFood(struct device* lcd_dev,Snake* S,food* fd)
// {
//     struct pos food_pos={
//         .x_pos=k_cycle_get_32()%(MAX_STEP_X-1),
//         .y_pos=k_cycle_get_32()%(MAX_STEP_Y-Forehead_BYTE)+Forehead_BYTE,
//     };
//     if(fd->exist==0)
//     {
//         while(is_overlap(&food_pos,S))
//         {
//             food_pos.x_pos=k_cycle_get_32()%(MAX_STEP_X-1);
//             food_pos.y_pos=(k_cycle_get_32()%(MAX_STEP_Y-Forehead_BYTE))+Forehead_BYTE;
//         }

//         printk("ypos:%d\n",food_pos.y_pos);
//         fd->exist=1;
//         fd->position.x_pos=food_pos.x_pos;
//         fd->position.y_pos=food_pos.y_pos;

//         st7735_fill_area(lcd_dev,(fd->position.x_pos)*X_STEP,(fd->position.y_pos)*Y_STEP,
//             (fd->position.x_pos+1)*X_STEP,(fd->position.y_pos+1)*Y_STEP,RED);
//     }
    
    
// }













void updata_control_input(Snake* S,uint16_t key_value)
{
    uint8_t keycode=(key_value>>8)&0xff;//哪一个按键
    uint8_t keytype=key_value&0xff;//单双击

    if(keycode)
    {
        
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
        if(keytype==KEY_TYPE_SHORT_UP)
        {
            if(S->play_pause)
                S->play_pause=0;
            else
                S->play_pause=1;
        }

    }

}
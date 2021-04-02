/**
 * 解法
 * 按照16*16像素来做,屏幕分为10*8=80个节点
 * 目标产生后，要先初始化一遍图，然后用广度优先
*/


#include "snake.h"
#include "string.h"

#include "zephyr.h"


struct pos stack[QUEUE_SIZE];

struct k_stack my_stack;
/**
 * 初始化图结构
*/  

struct solve_queue assist={
    .head=0,
    .rear=0,
};

int8_t dirx[]={1,0,-1,0};
int8_t diry[]={0,1,0,-1};
uint8_t distance[LCD_HIGH/TEST_SIZE][LCD_WIDTH/TEST_SIZE];


// int8_t temp_map[LCD_HIGH/TEST_SIZE][LCD_WIDTH/TEST_SIZE];//用来记录身体障碍

struct step history_path[LCD_HIGH/TEST_SIZE][LCD_WIDTH/TEST_SIZE];//记录路径

void map_init()
{
    k_stack_init(&my_stack, stack, QUEUE_SIZE);
    memset(&distance,255,sizeof(distance));
    printk("distance:%d\n",distance[10][10]);
    // while(pos!=S.rear)
    // {
    //     map[S.body[pos].y_pos][S.body[pos].x_pos]=1;
    //     pos=(pos+1)%MAX_BODY;
    // }
}



void solve_game(Snake* S,struct pos food,struct solve_queue* queue)
{
    struct pos head=S->body[S->rear-1];
    uint8_t find;
    queue->assist_queue[queue->rear++].locat=head;
    distance[head.y_pos][head.x_pos]=0;
    printk("start_x:%d==y:%d||end_x:%d==end_y:%d\n",head.x_pos,head.y_pos,food.x_pos,food.y_pos);
    
    while(queue->rear!=queue->head)//队列非空
    {
        printk("%d**%d\n",queue->rear,queue->head);
        find=0;
        for(uint8_t i=0;i<4;i++)
        {
            struct pos temp;
            temp.x_pos=queue->assist_queue[queue->head].locat.x_pos+dirx[i];
            temp.y_pos=queue->assist_queue[queue->head].locat.y_pos+diry[i];

            
            if((0<=temp.x_pos&&temp.x_pos<(LCD_WIDTH/TEST_SIZE))&&(0<=temp.y_pos&&temp.y_pos<(LCD_HIGH/TEST_SIZE))&&(distance[temp.y_pos][temp.x_pos]==255)
            &&(!is_overlap(&temp,S)))
            {
                printk("smell_X:%d==smell_Y:%d==distance:%d\n",temp.x_pos,temp.y_pos,distance[temp.y_pos][temp.x_pos]);
                history_path[temp.y_pos][temp.x_pos]=queue->assist_queue[queue->head];
                queue->assist_queue[queue->rear].locat=temp;
                queue->rear=(++queue->rear)%QUEUE_SIZE;
                distance[temp.y_pos][temp.x_pos]=distance[queue->assist_queue[queue->head].locat.y_pos][queue->assist_queue[queue->head].locat.x_pos]+1;

                if(temp.x_pos==food.x_pos&&
                temp.y_pos==food.y_pos)
                {
                    printk("find path");
                    find=true;
                    break;
                }
                
            }
        }

        if(find)
        {
            struct pos temp;
            temp=food;
        
            while(1)
            {
                k_stack_push(&my_stack, (uint32_t)&temp);
                if(temp.x_pos==head.x_pos&&temp.y_pos==head.y_pos)
                {
                    break;
                }
                temp=history_path[temp.y_pos][temp.x_pos].locat;
            }


            struct pos *new_buffer;

            while (k_stack_pop(&my_stack, (uint32_t *)&new_buffer, K_FOREVER))
            {
                
                printk("path:%d##%d\n",new_buffer->x_pos,new_buffer->y_pos);
            }
            
            break;
        }
        queue->head=(++queue->head)%QUEUE_SIZE;//出队
    }

}
/**
 * 还要考虑这个队列的自身覆盖问题
*/


















/*第一种方法深度优先*/


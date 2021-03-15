/**
 * 按键功能说明：
 * 左键单击向左
 * 右键单击向右
 * 左键双击向上
 * 右键双击向下
*/

#include <stdlib.h>
#include <time.h>
#include <zephyr.h>
#include <device.h>
#include "drivers/lcd_display.h"

#define MAX_BODY    50

#define X_STEP      10
#define Y_STEP      10

#define Forehead_BYTE       3

#define Forehead_HEIGHT     (Y_STEP*Forehead_BYTE)

#define SCREEN_SIZE_X   130
#define SCREEN_SIZE_Y   160
#define MAX_STEP_X    (SCREEN_SIZE_X/X_STEP)
#define MAX_STEP_Y    (SCREEN_SIZE_Y/Y_STEP)
enum{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
};
struct pos
{
    int x_pos;
    int y_pos;
};

typedef struct
{
    struct pos position;
    uint8_t exist;
}food;

typedef struct
{
    struct pos body[MAX_BODY];
    int head,rear;
    int curdir,lastdir;
    int score;
    int play_pause;
}Snake;

void updata_control_input(Snake* S,uint16_t key_value);
void CreatFood(struct device* lcd_dev,Snake* S,food* fd);
uint8_t SnakeMove(struct device* lcd_dev,Snake* S,food* fd);
void SnakeInit(struct device* lcd_dev,Snake* S,food* fd);
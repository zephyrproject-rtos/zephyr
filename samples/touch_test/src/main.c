/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include"cst816s.h"
int main(void)
{
uint8_t data_arr[8];
data_struct data;
const struct device *dev =  DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device *dev1 =  DEVICE_DT_GET(DT_NODELABEL(gpio0));
cst816s_t touch_dev_handle;
CST816S_init(&touch_dev_handle,dev1,dev,5,6,data_arr);
CST816S_begin(&touch_dev_handle);

while(1){
 data = CST816S_read_touch(&touch_dev_handle);
 printf("gesture code:%d\nX:%d\nY:%d\n",data.gestureID,data.x,data.y);
 if(data.gestureID==3) printf("Swipe left\n");
 else if(data.gestureID==4) printf("Swipe right\n");
 else if(data.gestureID==1) printf("Swipe up\n");
 else if(data.gestureID==2) printf("Swipe down\n");
 
}
}

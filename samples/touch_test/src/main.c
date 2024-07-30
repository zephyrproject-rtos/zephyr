/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include"cst816s.h"

data_struct data;
uint8_t data_arr[8];
const struct device *dev =  DEVICE_DT_GET(DT_NODELABEL(i2c0));
const struct device *dev1 =  DEVICE_DT_GET(DT_NODELABEL(gpio0));
cst816s_t touch_dev_handle;

K_MSGQ_DEFINE(touch_msg_q, sizeof(data_struct), 10, 1);
data_struct data_send;
void touch_isr(void);
void thread1(void)
{

// /* Enabling intterupts*/
// IRQ_CONNECT(32, 1, touch_isr, NULL, NULL);
// irq_enable(32);
// plic_irq_enable(32);

data_struct data_recv;
// CST816S_init(&touch_dev_handle,dev1,dev,5,6,data_arr);
// CST816S_begin(&touch_dev_handle);
while (1)
{
		k_msgq_get(&touch_msg_q, &data_recv, K_FOREVER);
		printk("Gesture ID:%d\n",data_recv.gestureID);
	}
}

void touch_isr(void)
{
    printf("Touch detected\n");
    // data = CST816S_read_touch(&touch_dev_handle);
    // k_msgq_put(&touch_msg_q, &data_send, K_NO_WAIT);
}






// while(1){
 
// //  printf("gesture code:%d\nX:%d\nY:%d\n",data.gestureID,data.x,data.y);
// //  if(data.gestureID==3) printf("Swipe left\n");
// //  else if(data.gestureID==4) printf("Swipe right\n");
// //  else if(data.gestureID==1) printf("Swipe up\n");
// //  else if(data.gestureID==2) printf("Swipe down\n");
 
// }

K_THREAD_DEFINE(thread1_id, 1024, thread1, NULL, NULL, NULL,
		7, 0, 0);

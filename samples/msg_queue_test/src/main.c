

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>
#include <stdlib.h>
#define STACKSIZE 1024
#define THREAD0_PRIORITY 7
#define THREAD1_PRIORITY 7
#define GPIO_INPUT 0
typedef struct{
	char sender[15];
	char message[30];
}msg_data;

K_MSGQ_DEFINE(sms_msg_q, sizeof(msg_data), 10, 1);

void thread0(void)
{

	msg_data data_recv;
	while (1) {
		printk("Task 1 is running\n");
		k_msgq_get(&sms_msg_q, &data_recv, K_FOREVER);
		printk("Q_data \nName:%s\nMessage:%s\n",data_recv.sender,data_recv.message);
		printk("Thread 1 is running\n");
	}
}
void thread1(void)
{
	uint8_t i=0;
	msg_data data_send;
	for(int i =0;i<15;i++){
		printk("Task 2 is running\n");
		sprintf(data_send.sender,"hello%d",i);
		sprintf(data_send.message,"This is message for %d",i);
		printk("Putiing to Q status: %d Posting %dth\n",k_msgq_put(&sms_msg_q, &data_send, K_NO_WAIT),i);
	}
}

K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
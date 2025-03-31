

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
void thread0(void);
void thread1(void);
K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL,
		THREAD1_PRIORITY, 0, 0);
// extern const k_tid_t thread0_id;
// extern const k_tid_t thread1_id;
void main(){
	*((uint32_t*)(0x40200))|=((1<<1)|(1<<2));
}
void thread0(void)
{
	thread0_id->id=1;
	while (1) {
	printf("Task 1 is running\n");
	// k_msleep(100);
	}
}

void thread1(void)
{
	thread1_id->id=2;
	while (1) {
	printf("Task 2 is running\n");
	// k_msleep(200);
	}
}

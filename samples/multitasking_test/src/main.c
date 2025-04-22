

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdlib.h>
#define STACKSIZE 1024
#define THREAD0_PRIORITY 7
#define THREAD1_PRIORITY 7
void thread0(void)
{
	while (1) {
    printk("Hello, I am thread0\n");
    k_msleep(10);
	}
}

// void thread1(void)
// {
// 	while (1) {
//      printk("Hello, I am thread1\n");
// 	   	k_msleep(10);
// 	}
// }

K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL,
		THREAD0_PRIORITY, 0, 0);
// K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL,
// 		THREAD1_PRIORITY, 0, 0);
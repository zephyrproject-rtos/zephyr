/* main.c - Memory Domain APIs demo */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY CONFIG_MAIN_THREAD_PRIORITY

/* busy waiting time (in us) */
#define BUSY_WAITING_TIME 3000000


K_THREAD_STACK_ARRAY_DEFINE(app_stack, 3, STACKSIZE);

struct k_thread app_thread_id[3];

struct k_mem_domain app_domain[2];

/* the start address of the MPU region needs to align with its size */
#ifdef CONFIG_ARM
u8_t __aligned(32) app0_buf[32];
u8_t __aligned(32) app1_buf[32];
#else
u8_t __aligned(4096) app0_buf[4096];
u8_t __aligned(4096) app1_buf[4096];
#endif

K_MEM_PARTITION_DEFINE(app0_parts0, app0_buf, sizeof(app0_buf),
		       K_MEM_PARTITION_P_RW_U_RW);
#ifdef CONFIG_X86
K_MEM_PARTITION_DEFINE(app0_parts1, app1_buf, sizeof(app1_buf),
		       K_MEM_PARTITION_P_RO_U_RO);
#else
K_MEM_PARTITION_DEFINE(app0_parts1, app1_buf, sizeof(app1_buf),
		       K_MEM_PARTITION_P_RW_U_RO);
#endif

struct k_mem_partition *app0_parts[] = {
	&app0_parts0,
	&app0_parts1
};

K_MEM_PARTITION_DEFINE(app1_parts0, app1_buf, sizeof(app1_buf),
		       K_MEM_PARTITION_P_RW_U_RW);
#ifdef CONFIG_X86
K_MEM_PARTITION_DEFINE(app1_parts1, app0_buf, sizeof(app0_buf),
		       K_MEM_PARTITION_P_RO_U_RO);
#else
K_MEM_PARTITION_DEFINE(app1_parts1, app0_buf, sizeof(app0_buf),
		       K_MEM_PARTITION_P_RW_U_RO);
#endif

struct k_mem_partition *app1_parts[] = {
	&app1_parts0,
	&app1_parts1
};


/* application thread */
void app_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int id = (int) dummy1;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	printk("app thread %d (%p) starts.\n", id, &app_thread_id[id]);

	k_busy_wait(BUSY_WAITING_TIME);

	while (1) {
		printk("app thread %d (%p) yields.\n", id, &app_thread_id[id]);

		k_yield();

		printk("app thread %d (%p) is back.\n", id, &app_thread_id[id]);

		k_busy_wait(BUSY_WAITING_TIME);
	}
}

void main(void)
{
	int count = 0;

	printk("initialize memory domains\n");
	/* init memory domain w/ memory partition array */
	k_mem_domain_init(&app_domain[0], ARRAY_SIZE(app0_parts), app0_parts);
	/* init memory domain w/ empty memory partition */
	k_mem_domain_init(&app_domain[1], 0, NULL);
	/* add memory partitions one by one into a domain */
	k_mem_domain_add_partition(&app_domain[1], &app1_parts0);
	k_mem_domain_add_partition(&app_domain[1], &app1_parts1);

	/* create application threads */
	k_thread_create(&app_thread_id[0], app_stack[0],
			K_THREAD_STACK_SIZEOF(app_stack[0]), app_thread,
			(void *) 0, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	printk("add app thread 0 (%p) into app0_domain.\n", &app_thread_id[0]);
	k_mem_domain_add_thread(&app_domain[0], &app_thread_id[0]);

	k_thread_create(&app_thread_id[1], app_stack[1],
			K_THREAD_STACK_SIZEOF(app_stack[1]), app_thread,
			(void *) 1, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	printk("add app thread 1 (%p) into app1_domain.\n", &app_thread_id[1]);
	k_mem_domain_add_thread(&app_domain[1], &app_thread_id[1]);

	k_thread_create(&app_thread_id[2], app_stack[2],
			K_THREAD_STACK_SIZEOF(app_stack[2]), app_thread,
			(void *) 2, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	printk("add app thread 2 (%p) into app0_domain.\n", &app_thread_id[2]);
	k_mem_domain_add_thread(&app_domain[0], &app_thread_id[2]);

	while (1) {
		printk("main thread (%p) yields.\n", k_current_get());
		k_yield();
		printk("main thread (%p) is back.\n", k_current_get());

		if (count == 2) {
			printk("remove app0_parts0 from app0_domain.\n");
			k_mem_domain_remove_partition(&app_domain[0],
						      &app0_parts0);
		}

		if (count == 4) {
			printk("remove app thread 1 (%p) from app1_domain.\n",
				&app_thread_id[1]);
			k_mem_domain_remove_thread(&app_thread_id[1]);
		}

		if (count == 6) {
			printk("destroy app0_domain.\n");
			k_mem_domain_destroy(&app_domain[0]);
			break;
		}

		count++;
	}
}

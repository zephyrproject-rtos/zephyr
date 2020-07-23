/*
 * Copyright (c) 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdlib.h>
#include <kernel.h>
#include <logging/log.h>
#include <app_memory/app_memdomain.h>

LOG_MODULE_REGISTER(app_main);

#define STACK_SIZE 512
#define PRIORITY 0

uint8_t __aligned(32) buf0[32];
uint8_t __aligned(32) buf1[32];
uint8_t __aligned(32) buf2[32];
uint8_t __aligned(32) buf3[32];
uint8_t __aligned(32) buf4[32];

K_MEM_PARTITION_DEFINE(part0, buf0, sizeof(buf0), K_MEM_PARTITION_P_RW_U_RW);
K_MEM_PARTITION_DEFINE(part1, buf1, sizeof(buf1), K_MEM_PARTITION_P_RW_U_RW);
K_MEM_PARTITION_DEFINE(part2, buf2, sizeof(buf2), K_MEM_PARTITION_P_RW_U_RW);
K_MEM_PARTITION_DEFINE(part3, buf3, sizeof(buf3), K_MEM_PARTITION_P_RW_U_RW);
K_MEM_PARTITION_DEFINE(part4, buf4, sizeof(buf4), K_MEM_PARTITION_P_RW_U_RW);

struct k_mem_domain dom0, dom1, dom2;

struct k_thread thread0;
K_THREAD_STACK_DEFINE(stack0, STACK_SIZE);

struct k_thread thread1;
K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);

struct k_thread thread2;
K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

struct k_thread thread3;
K_THREAD_STACK_DEFINE(stack3, STACK_SIZE);

void hello(void)
{
	if (arch_is_user_context()) {
		LOG_INF("Hello from thread 0x%08lx running user context!",
				(long unsigned int)k_current_get());
	} else {
		LOG_INF("Hello from thread 0x%08lx running machine context!",
				(long unsigned int)k_current_get());
	}
}

#define TRY_WRITE_BUF(x, msg) \
	char * str##x = msg; \
	LOG_INF("Writing '%s' to buf%d.", str##x, x); \
	memcpy(buf##x, str##x, sizeof(str##x));

#define TRY_READ_BUF(x) LOG_INF("Contents of buf%d: '%s'", x, buf##x);

void entry0(void)
{
	hello();
	
	TRY_WRITE_BUF(0, "OK");
	TRY_WRITE_BUF(1, "OK");

	/* This thread does not have access to the following partitions. */
	TRY_WRITE_BUF(2, "FAIL");
	TRY_WRITE_BUF(3, "FAIL");
	TRY_WRITE_BUF(4, "FAIL");
}

void entry1(void)
{
	hello();

	TRY_READ_BUF(1);
	TRY_READ_BUF(2);
	TRY_READ_BUF(3);
	TRY_WRITE_BUF(1, "OK");
	TRY_WRITE_BUF(2, "OK");
	TRY_WRITE_BUF(3, "OK");

	/* This thread does not have access to the following partitions. */
	TRY_READ_BUF(4);
	TRY_READ_BUF(0);
}

void entry2(void)
{
	hello();
}

void entry3(void)
{
	hello();
}

void main(void)
{
	hello();

	struct k_mem_partition *dom0_parts[] = {&part0, &part1};
	struct k_mem_partition *dom1_parts[] = {&part2, &part1, &part3};
	struct k_mem_partition *dom2_parts[] = {&part4, &part3};

	k_mem_domain_init(&dom0, 2, dom0_parts);
	k_mem_domain_init(&dom1, 3, dom1_parts);
	k_mem_domain_init(&dom2, 2, dom2_parts);

	k_tid_t tid0 = k_thread_create(&thread0, stack0, STACK_SIZE,
			(k_thread_entry_t)entry0, NULL, NULL, NULL,
			-1, K_USER, K_FOREVER);

	k_tid_t tid1 = k_thread_create(&thread1, stack1, STACK_SIZE,
			(k_thread_entry_t)entry1, NULL, NULL, NULL,
			-1, K_USER, K_FOREVER);

	k_tid_t tid2 = k_thread_create(&thread2, stack2, STACK_SIZE,
			(k_thread_entry_t)entry2, NULL, NULL, NULL,
			-1, K_USER, K_FOREVER);

	k_tid_t tid3 = k_thread_create(&thread3, stack3, STACK_SIZE,
			(k_thread_entry_t)entry3, NULL, NULL, NULL,
			-1, K_USER, K_FOREVER);

	k_mem_domain_add_thread(&dom0, tid0);
	k_mem_domain_add_thread(&dom1, tid1);
	k_mem_domain_add_thread(&dom1, tid2);
	k_mem_domain_add_thread(&dom2, tid3);

	LOG_INF("Starting thread 0x%08lx.", (long unsigned int)tid0);
	k_thread_start(tid0);

	LOG_INF("Starting thread 0x%08lx.", (long unsigned int)tid1);
	k_thread_start(tid1);

	LOG_INF("Starting thread 0x%08lx.", (long unsigned int)tid2);
	k_thread_start(tid2);

	LOG_INF("Starting thread 0x%08lx.", (long unsigned int)tid3);
	k_thread_start(tid3);

	LOG_INF("SUCCESS");

	/* TODO: z_idle_threads breaks */
	while(1) {}
}

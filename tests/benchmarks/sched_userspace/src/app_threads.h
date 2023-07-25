/*
 * Copyright (c) 2022 Huawei Technologies SASU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/app_memory/app_memdomain.h>

#define MAX_NB_THREADS 50

#define APP_STACKSIZE 1024

struct k_app_thread {
	struct k_thread thread;
	struct k_mem_domain domain;
	void *stack;
	struct k_mem_partition *partition;
};

struct k_app_thread app_threads[MAX_NB_THREADS];

/* Generate threads data structures */
#define DEFINE_THREADS_HELPER(nb) \
	K_THREAD_STACK_DEFINE(app_##nb##_stack, APP_STACKSIZE); \
	K_APPMEM_PARTITION_DEFINE(app_##nb##_partition); \
	K_APP_DMEM(app_##nb##_partition) int dummy##nb; /* Need data in each partition */

#define DEFINE_THREADS_HELPER_10(nb0, nb1, nb2, nb3, nb4, nb5, nb6, nb7, nb8, nb9) \
	DEFINE_THREADS_HELPER(nb0) \
	DEFINE_THREADS_HELPER(nb1) \
	DEFINE_THREADS_HELPER(nb2) \
	DEFINE_THREADS_HELPER(nb3) \
	DEFINE_THREADS_HELPER(nb4) \
	DEFINE_THREADS_HELPER(nb5) \
	DEFINE_THREADS_HELPER(nb6) \
	DEFINE_THREADS_HELPER(nb7) \
	DEFINE_THREADS_HELPER(nb8) \
	DEFINE_THREADS_HELPER(nb9)

DEFINE_THREADS_HELPER_10(1,   2,  3,  4,  5,  6,  7,  8,  9, 10)
DEFINE_THREADS_HELPER_10(11, 12, 13, 14, 15, 16, 17, 18, 19, 20)
DEFINE_THREADS_HELPER_10(21, 22, 23, 24, 25, 26, 27, 28, 29, 30)
DEFINE_THREADS_HELPER_10(31, 32, 33, 34, 35, 36, 37, 38, 39, 40)
DEFINE_THREADS_HELPER_10(41, 42, 43, 44, 45, 46, 47, 48, 49, 50)

void *app_thread_stacks[MAX_NB_THREADS] = {
	app_1_stack, app_2_stack, app_3_stack, app_4_stack, app_5_stack,
	app_6_stack, app_7_stack, app_8_stack, app_9_stack, app_10_stack,
	app_11_stack, app_12_stack, app_13_stack, app_14_stack, app_15_stack,
	app_16_stack, app_17_stack, app_18_stack, app_19_stack, app_20_stack,
	app_21_stack, app_22_stack, app_23_stack, app_24_stack, app_25_stack,
	app_26_stack, app_27_stack, app_28_stack, app_29_stack, app_30_stack,
	app_31_stack, app_32_stack, app_33_stack, app_34_stack, app_35_stack,
	app_36_stack, app_37_stack, app_38_stack, app_39_stack, app_40_stack,
	app_41_stack, app_42_stack, app_43_stack, app_44_stack, app_45_stack,
	app_46_stack, app_47_stack, app_48_stack, app_49_stack, app_50_stack
};

struct k_mem_partition *app_partitions[MAX_NB_THREADS] = {
	&app_1_partition, &app_2_partition, &app_3_partition, &app_4_partition,
	&app_5_partition, &app_6_partition, &app_7_partition, &app_8_partition,
	&app_9_partition, &app_10_partition,
	&app_11_partition, &app_12_partition, &app_13_partition, &app_14_partition,
	&app_15_partition, &app_16_partition, &app_17_partition, &app_18_partition,
	&app_19_partition, &app_20_partition,
	&app_21_partition, &app_22_partition, &app_23_partition, &app_24_partition,
	&app_25_partition, &app_26_partition, &app_27_partition, &app_28_partition,
	&app_29_partition, &app_30_partition,
	&app_31_partition, &app_32_partition, &app_33_partition, &app_34_partition,
	&app_35_partition, &app_36_partition, &app_37_partition, &app_38_partition,
	&app_39_partition, &app_40_partition,
	&app_41_partition, &app_42_partition, &app_43_partition, &app_44_partition,
	&app_45_partition, &app_46_partition, &app_47_partition, &app_48_partition,
	&app_49_partition, &app_50_partition
};

struct k_mem_domain app_domains[MAX_NB_THREADS];

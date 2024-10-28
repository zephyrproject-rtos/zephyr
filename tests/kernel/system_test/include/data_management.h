/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DATA_MANAGEMENT_H
#define __DATA_MANAGEMENT_H

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdio.h>

#define DATA_LEN 20

struct fifo_item_t {
	void *fifo_reserved;
	int data;
};

struct lifo_item_t {
	void *lifo_reserved;
	int data;
};

extern uint8_t fifo_pushed_data[DATA_LEN];
extern uint8_t fifo_poped_data[DATA_LEN];
extern uint8_t lifo_pushed_data[DATA_LEN];
extern uint8_t lifo_poped_data[DATA_LEN];
extern uint8_t stack_pushed_data[DATA_LEN];
extern uint8_t stack_poped_data[DATA_LEN];
extern uint8_t ring_buffer_pushed_data[DATA_LEN];
extern uint8_t ring_buffer_poped_data[DATA_LEN];

extern void prepare_data(void);
extern void fifo_push(void);
extern void fifo_pop(void);
extern void lifo_push(void);
extern void lifo_pop(void);
extern void stack_push(void);
extern void stack_pop(void);
extern void ring_buf_push(void);
extern void ring_buf_pop(void);
extern void data_summary(void);
extern bool compare_arrays(uint8_t *arr1, uint8_t *arr2, int size, bool rev);

#endif

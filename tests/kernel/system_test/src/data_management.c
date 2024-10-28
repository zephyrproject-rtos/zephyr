/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_management.h"
#include "log_management.h"

K_MUTEX_DEFINE(data_mutex);

K_FIFO_DEFINE(fifo);
static struct fifo_item_t fifo_data[DATA_LEN];

K_LIFO_DEFINE(lifo);
static struct lifo_item_t lifo_data[DATA_LEN];

K_STACK_DEFINE(stack, DATA_LEN);

RING_BUF_DECLARE(ring_buf, DATA_LEN);

uint8_t fifo_pushed_data[DATA_LEN];
uint8_t fifo_poped_data[DATA_LEN];
uint8_t lifo_poped_data[DATA_LEN];
uint8_t lifo_pushed_data[DATA_LEN];
uint8_t stack_poped_data[DATA_LEN];
uint8_t stack_pushed_data[DATA_LEN];
uint8_t ring_buffer_pushed_data[DATA_LEN];
uint8_t ring_buffer_poped_data[DATA_LEN];

void prepare_data(void)
{
	for (uint8_t i = 0; i < DATA_LEN; i++) {
		fifo_pushed_data[i] = sys_rand8_get();
		lifo_pushed_data[i] = sys_rand8_get();
		stack_pushed_data[i] = sys_rand8_get();
		ring_buffer_pushed_data[i] = sys_rand8_get();
	}
}

void fifo_push(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		k_mutex_lock(&data_mutex, K_FOREVER);
		fifo_data[i].data = fifo_pushed_data[i];
		k_fifo_put(&fifo, &fifo_data[i]);
		k_mutex_unlock(&data_mutex);
	}
}

void fifo_pop(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		struct fifo_item_t *data = k_fifo_get(&fifo, K_NO_WAIT);

		if (data) {
			k_mutex_lock(&data_mutex, K_FOREVER);
			fifo_poped_data[i] = data->data;
			k_mutex_unlock(&data_mutex);
		}
	}
}

void lifo_push(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		k_mutex_lock(&data_mutex, K_FOREVER);
		lifo_data[i].data = lifo_pushed_data[i];
		k_lifo_put(&lifo, &lifo_data[i]);
		k_mutex_unlock(&data_mutex);
	}
}

void lifo_pop(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		struct lifo_item_t *data = k_lifo_get(&lifo, K_NO_WAIT);

		if (data) {
			k_mutex_lock(&data_mutex, K_FOREVER);
			lifo_poped_data[i] = data->data;
			k_mutex_unlock(&data_mutex);
		}
	}
}

void stack_push(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		k_mutex_lock(&data_mutex, K_FOREVER);
		k_stack_push(&stack, stack_pushed_data[i]);
		k_mutex_unlock(&data_mutex);
	}
}

void stack_pop(void)
{
	stack_data_t data;

	for (int i = 0; i < DATA_LEN; i++) {
		k_mutex_lock(&data_mutex, K_FOREVER);
		k_stack_pop(&stack, &data, K_NO_WAIT);
		stack_poped_data[i] = (uint8_t)data;
		k_mutex_unlock(&data_mutex);
	}
}

void ring_buf_push(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		k_mutex_lock(&data_mutex, K_FOREVER);
		const uint8_t data = ring_buffer_pushed_data[i];

		ring_buf_put(&ring_buf, &data, 1);
		k_mutex_unlock(&data_mutex);
	}
}

void ring_buf_pop(void)
{
	for (int i = 0; i < DATA_LEN; i++) {
		uint8_t data;

		ring_buf_get(&ring_buf, &data, 1);
		k_mutex_lock(&data_mutex, K_FOREVER);
		ring_buffer_poped_data[i] = data;
		k_mutex_unlock(&data_mutex);
	}
}

bool compare_arrays(uint8_t *arr1, uint8_t *arr2, int size, bool rev)
{
	if (rev) {
		for (int i = 0; i < size; i++) {
			if (arr1[i] != arr2[size - 1 - i]) {
				fprintf(output_file, "\nFAILED (%d not equal %d at index %d)\n",
					arr1[i], arr2[size - 1 - i], i);
				return false;
			}
		}
	} else {
		for (int i = 0; i < size; i++) {
			if (arr1[i] != arr2[i]) {
				fprintf(output_file, "\nFAILED (%d not equal %d at index %d)\n",
					arr1[i], arr2[size - 1 - i], i);
				return false;
			}
		}
	}
	return true;
}

void data_summary(void)
{
	fprintf(output_file, "\n==================================================================="
			     "=============");
	fprintf(output_file, "\nTEST: DATA SUMMARY");
	fprintf(output_file, "\n==================================================================="
			     "=============");
	fprintf(output_file, "\nTEST: Fifo");
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	log_print_data("\nPushed data to Fifo:\n", fifo_pushed_data, DATA_LEN);
	log_print_data("\nPopped data from Fifo:\n", fifo_poped_data, DATA_LEN);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	bool is_test_passed = compare_arrays(fifo_pushed_data, fifo_poped_data, DATA_LEN, false);
	const char *test_result_message =
		is_test_passed ? "\nFifo: " GREEN("PASSED") : "\nFifo: " RED("FAILED");

	fprintf(output_file, "%s", test_result_message);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	fprintf(output_file, "\nTEST: Lifo");
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	log_print_data("\nPushed data to Lifo:\n", lifo_pushed_data, DATA_LEN);
	log_print_data("\nPopped data from Lifo.\n", lifo_poped_data, DATA_LEN);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	is_test_passed = compare_arrays(lifo_pushed_data, lifo_poped_data, DATA_LEN, true);
	test_result_message =
		is_test_passed ? "\nLifo: " GREEN("PASSED") : "\nLifo: " RED("FAILED");
	fprintf(output_file, "%s", test_result_message);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	fprintf(output_file, "\nTEST: Stack");
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	log_print_data("\nPushed data to Stack:\n", stack_pushed_data, DATA_LEN);
	log_print_data("\nPopped data from Stack:\n", stack_poped_data, DATA_LEN);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	is_test_passed = compare_arrays(stack_pushed_data, stack_poped_data, DATA_LEN, true);
	test_result_message =
		is_test_passed ? "\nStack: " GREEN("PASSED") : "\nStack: " RED("FAILED");
	fprintf(output_file, "%s", test_result_message);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	fprintf(output_file, "\nTEST: Ring Buffer");
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	log_print_data("\nPushed data to Ring Buffer:\n", ring_buffer_pushed_data, DATA_LEN);
	log_print_data("\nPopped data from Ring Buffer:\n", ring_buffer_poped_data, DATA_LEN);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
	is_test_passed =
		compare_arrays(ring_buffer_pushed_data, ring_buffer_poped_data, DATA_LEN, false);
	test_result_message = is_test_passed ? "\nRing Buffer: " GREEN("PASSED")
					     : "\nRing Buffer: " RED("FAILED");
	fprintf(output_file, "%s", test_result_message);
	fprintf(output_file, "\n-------------------------------------------------------------------"
			     "-------------");
}

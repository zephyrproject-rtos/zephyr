/*
 * SPDX-File-CopyrightText: Copyright Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/portability/freertos.h>

#define TEST_VALUE 1234

struct thread_query {
	k_tid_t found;
	const char *name;
};

static void thread_search_cb(const struct k_thread *thread, void *user_data)
{
	struct thread_query *query = user_data;

	if (strcmp(k_thread_name_get((k_tid_t)thread), query->name) == 0) {
		query->found = (k_tid_t)thread;
	}
}

static k_tid_t thread_find_by_name(char const *name)
{
	struct thread_query query = {.name = name, .found = NULL};

	k_thread_foreach(thread_search_cb, &query);

	return query.found;
}

void freertos_task_fn(void *p)
{
	int *intp = p;

	zassert_equal(*intp, TEST_VALUE, "value passed as parameter should be matching");

	/* Task is to be interrupted from the outside */
	k_sleep(K_FOREVER);

	zassert_ok(-EINVAL, "should never reach the end of a FreeRTOS task");
}

ZTEST(freertos, test_freertos_task)
{
	int task_param = TEST_VALUE;
	TaskHandle_t task_handle;
	char const *task_name = "freertos_task";
	int task_prio = 1;
	k_tid_t thread;
	int32_t err;

	err = xTaskCreate(freertos_task_fn, task_name, 512, &task_param, task_prio, &task_handle);
	zassert_equal(err, pdPASS, "task creation with valid parameters should work");

	thread = thread_find_by_name("freertos_task");
	zassert_not_null(thread, "thread should be found under given name");

	zassert(k_thread_priority_get(thread) < CONFIG_NUM_PREEMPT_PRIORITIES,
		"thread priority should be below maximum");

	zassert(k_thread_priority_get(thread) > 0,
		"thread priority should be above minimum");

	vTaskDelete(task_handle);

	thread = thread_find_by_name(task_name);
	zassert_is_null(thread, "thread should not be found after deletion");
}

void freertos_queue_fn(void *p)
{
	QueueHandle_t queue_handle = p;
	uint32_t queue_item = TEST_VALUE;
	int32_t err;

	err = xQueueGenericSend(queue_handle, &queue_item, 100, 0);
	zassert_equal(err, pdPASS, "queueing one item should be working");

	err = xQueueReceive(queue_handle, &queue_item, 100);
	zassert_equal(err, pdPASS, "receiving a queue item from the test should work");
	zassert_equal(queue_item, TEST_VALUE,
		      "item received from the test should be %d, got %d", TEST_VALUE, queue_item);

	/* Task is to be interrupted from the outside */
	k_sleep(K_FOREVER);
	zassert_ok(-EINVAL, "should never reach the end of a FreeRTOS task");
}

ZTEST(freertos, test_freertos_queue)
{
	TaskHandle_t task_handle;
	QueueHandle_t queue_handle;
	int queue_length = 2;
	uint32_t queue_item = 0;
	int32_t err;

	queue_handle = xQueueGenericCreate(queue_length, sizeof(queue_item), 0 /* not supported */);
	zassert_not_null(queue_handle, "queue creation should succeed");

	err = xTaskCreate(freertos_queue_fn, "freertos_queue", 512, queue_handle, 1, &task_handle);
	zassert_equal(err, pdPASS, "task creation with valid parameters should work");

	err = xQueueReceive(queue_handle, &queue_item, 100);
	zassert_equal(err, pdPASS, "receiving a queue item from the task should work");
	zassert_equal(queue_item, TEST_VALUE,
		      "item received from the task should be %d, got %d", TEST_VALUE, queue_item);

	err = xQueueGenericSend(queue_handle, &queue_item, 100, 0);
	zassert_equal(err, pdPASS, "queueing one item to the task should be working");

	vTaskDelete(task_handle);
}

ZTEST_SUITE(freertos, NULL, NULL, NULL, NULL, NULL);

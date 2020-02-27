/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Workqueue Tests
 * @defgroup kernel_workqueue_tests Workqueue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>

#define TIMEOUT 100
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define NUM_OF_WORK 2
#define SYNC_SEM_INIT_VAL (0U)
#define COM_SEM_MAX_VAL (1U)
#define COM_SEM_INIT_VAL (0U)
#define MY_PRIORITY 5

struct k_delayed_work new_work_item_delayed;
struct k_sem sema_fifo_two;
K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);

static struct k_work_q workq;
static struct k_sem sync_sema;

void new_common_work_handler(struct k_work *unused)
{
	printk("\nsync sema given\n");
	k_sem_give(&sync_sema);
	k_sem_take(&sema_fifo_two, K_FOREVER);
	printk("fifo sema taken\n");
}

void test_cancel_processed_work_item(void)
{
    int ret;

    k_sem_reset(&sync_sema);
    k_sem_reset(&sema_fifo_two);
    s32_t delay_time = 100;

    /**TESTPOINT: init delayed work to be processed
     * only after specific period of time*/
    k_delayed_work_init(&new_work_item_delayed, new_common_work_handler);

    ret = k_delayed_work_cancel(&new_work_item_delayed);
    printk("\n%d\n", ret);
    zassert_true(ret == -EINVAL, NULL);
    k_delayed_work_submit_to_queue(&workq, &new_work_item_delayed, delay_time);
    k_sem_take(&sync_sema, K_FOREVER);
    printk("sync sema taken\n");

    printk("fifo sema given\n");
    k_sem_give(&sema_fifo_two);
    k_queue_remove(&(new_work_item_delayed.work_q->queue), &(new_work_item_delayed.work));
    ret = k_delayed_work_cancel(&new_work_item_delayed);
    printk("\n%d\n", ret); // debug printk
    zassert_true(ret == -EALREADY, NULL);
    k_sleep(100);
}

void test_main(void)
{
	k_work_q_start(&workq, my_stack_area,
		       K_THREAD_STACK_SIZEOF(my_stack_area), MY_PRIORITY);
	k_sem_init(&sync_sema, SYNC_SEM_INIT_VAL, NUM_OF_WORK);
	k_sem_init(&sema_fifo_two, COM_SEM_INIT_VAL, COM_SEM_MAX_VAL);

	ztest_test_suite(workqueue_api,
			 ztest_unit_test(test_cancel_processed_work_item));
	ztest_run_test_suite(workqueue_api);
}

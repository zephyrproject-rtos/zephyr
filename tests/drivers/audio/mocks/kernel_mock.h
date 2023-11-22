#ifndef KERNEL_MOCK_H
#define KERNEL_MOCK_H

#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

DECLARE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem *, k_timeout_t);
DECLARE_FAKE_VOID_FUNC(k_sem_give, struct k_sem *);
DECLARE_FAKE_VALUE_FUNC(int, k_sem_init, struct k_sem *, unsigned int, unsigned int);
DECLARE_FAKE_VALUE_FUNC(int32_t, k_sleep, k_timeout_t);
DECLARE_FAKE_VOID_FUNC(k_work_queue_start, struct k_work_q *, k_thread_stack_t *, size_t, int,
                       const struct k_work_queue_config *);
DECLARE_FAKE_VOID_FUNC(k_work_queue_init, struct k_work_q *);
DECLARE_FAKE_VALUE_FUNC(int, k_work_submit_to_queue, struct k_work_q *, struct k_work *);
DECLARE_FAKE_VOID_FUNC(k_work_init, struct k_work *, k_work_handler_t);

#endif /* KERNEL_MOCK_H */

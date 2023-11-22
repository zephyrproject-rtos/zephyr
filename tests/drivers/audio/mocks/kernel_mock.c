#include <zephyr/ztest.h>

#include <stdint.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include "kernel_mock.h"

DEFINE_FAKE_VALUE_FUNC(int, k_sem_take, struct k_sem *, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(k_sem_give, struct k_sem *);
DEFINE_FAKE_VALUE_FUNC(int, k_sem_init, struct k_sem *, unsigned int, unsigned int);
DEFINE_FAKE_VALUE_FUNC(int32_t, k_sleep, k_timeout_t);
DEFINE_FAKE_VOID_FUNC(k_work_queue_start, struct k_work_q *, k_thread_stack_t *, size_t, int,
                      const struct k_work_queue_config *);
DEFINE_FAKE_VOID_FUNC(k_work_queue_init, struct k_work_q *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_submit_to_queue, struct k_work_q *, struct k_work *);
DEFINE_FAKE_VOID_FUNC(k_work_init, struct k_work *, k_work_handler_t);

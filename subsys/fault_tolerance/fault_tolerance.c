#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/fault_tolerance.h>
#include <zephyr/logging/log.h>

#define RECOVERY_STACK_SIZE 1024
#define RECOVERY_PRIORITY 5 

void worker_recovery_thread(void);

K_THREAD_STACK_DEFINE(recovery_thread_stack, RECOVERY_STACK_SIZE);
struct k_thread recovery_thread;

LOG_MODULE_REGISTER(fault_tolerance, CONFIG_FT_LOG_LEVEL);

K_MSGQ_DEFINE(ft_event_queue, sizeof(ft_event_t), 8, 4);

static int fault_manager_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    k_tid_t recovery_tid = k_thread_create(&recovery_thread, recovery_thread_stack,
                                         RECOVERY_STACK_SIZE,
                                         (k_thread_entry_t)worker_recovery_thread,
                                         NULL, NULL, NULL,
                                         RECOVERY_PRIORITY, 0, K_NO_WAIT);

    printk("Thread created with tid: %p\n", recovery_tid);

    LOG_INF("Fault Tolerance Manager initialized");
    return 0;
}

SYS_INIT(fault_manager_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

int ft_recovery_submit_event(const ft_event_t *event)
{
    return k_msgq_put(&ft_event_queue, event, K_NO_WAIT);
}

int ft_recovery_consume_event(ft_event_t *event)
{
    return k_msgq_get(&ft_event_queue, event, K_NO_WAIT);
}

void worker_recovery_thread(void)
{
    ft_event_t event;

    while (1) {
        if (ft_recovery_consume_event(&event) == 0) {
            LOG_INF("Processing fault event: seq=%llu, domain=%d, severity=%d",
                    event.seq, event.domain, event.severity);
        } else {
            k_sleep(K_MSEC(100));
        }
    }
}
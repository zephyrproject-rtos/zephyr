/*
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/llext/symbol.h>



/* export symbol kernel */
EXPORT_SYMBOL(printk);
EXPORT_SYMBOL(sys_clock_cycle_get_32);
EXPORT_SYSCALL(k_mutex_lock);
EXPORT_SYSCALL(k_mutex_unlock);
EXPORT_SYSCALL(k_sleep);
EXPORT_SYSCALL(k_thread_create);
EXPORT_SYSCALL(k_thread_name_set);
EXPORT_SYSCALL(k_timer_start);
EXPORT_SYSCALL(k_timer_stop);
EXPORT_SYSCALL(k_uptime_ticks);
EXPORT_SYSCALL(z_log_msg_static_create);
EXPORT_SYSCALL(z_log_msg_simple_create_0);
EXPORT_SYSCALL(z_log_msg_simple_create_1);
EXPORT_SYSCALL(z_log_msg_simple_create_2);
EXPORT_SYMBOL(z_timer_expiration_handler);
EXPORT_SYSCALL(k_sched_current_thread_query);
EXPORT_SYMBOL(k_object_access_revoke);
EXPORT_SYSCALL(k_sem_init);
EXPORT_SYSCALL(k_sem_take);
EXPORT_SYSCALL(k_sem_give);

#ifdef CONFIG_USERSPACE
EXPORT_SYMBOL(z_arm_thread_is_in_user_mode);
EXPORT_SYSCALL(k_object_access_grant);
EXPORT_SYMBOL(k_thread_user_mode_enter);
#ifdef CONFIG_DYNAMIC_THREAD
EXPORT_SYSCALL(k_thread_stack_alloc);
#endif
#ifdef CONFIG_DYNAMIC_OBJECTS
EXPORT_SYSCALL(k_object_alloc);
#endif
#endif

#ifdef CONFIG_TRACING_USER
EXPORT_SYMBOL(sys_trace_poc_log);
#endif

#ifdef CONFIG_SEGGER_SYSTEMVIEW
EXPORT_SYMBOL(SEGGER_SYSVIEW_RecordEndCall);
EXPORT_SYMBOL(SEGGER_SYSVIEW_RecordString);
EXPORT_SYMBOL(SEGGER_SYSVIEW_MarkStart);
EXPORT_SYMBOL(SEGGER_SYSVIEW_MarkStop);
EXPORT_SYMBOL(SEGGER_SYSVIEW_PrintfHost);
#endif

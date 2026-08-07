/*
 * Copyright 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_OS_PORT_H
#define IPC_OS_PORT_H
#include <stdint.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define os_interrupt_disable()    irq_lock()
#define os_interrupt_enable(mask) irq_unlock(mask)
#define os_interrupt_enter()
#define os_interrupt_exit()

#define os_interrupt_start(irq_number, priority, sub_priority) irq_enable(irq_number)
#define os_interrupt_stop(irq_number)                          irq_disable(irq_number)

#ifdef __cplusplus
}
#endif

#endif

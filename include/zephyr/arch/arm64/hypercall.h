/*
 * Copyright (c) 2021-2022 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_HYPERCALL_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_HYPERCALL_H_

/* defined in hypercall.S by HYPERCALL(hypercall) */
int HYPERVISOR_console_io(int op, int cnt, char *str);
int HYPERVISOR_sched_op(int op, void *param);
int HYPERVISOR_event_channel_op(int op, void *param);
int HYPERVISOR_hvm_op(int op, void *param);
int HYPERVISOR_memory_op(int op, void *param);
int HYPERVISOR_grant_table_op(int op, void *uop, unsigned int count);

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_HYPERCALL_H_ */

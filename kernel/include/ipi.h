/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_KERNEL_INCLUDE_IPI_H_
#define ZEPHYR_KERNEL_INCLUDE_IPI_H_

/* defined in ipi.c when CONFIG_SMP=y */
#ifdef CONFIG_SMP
void flag_ipi(void);
void signal_pending_ipi(void);
#else
#define flag_ipi() do { } while (false)
#define signal_pending_ipi() do { } while (false)
#endif /* CONFIG_SMP */

#endif /* ZEPHYR_KERNEL_INCLUDE_IPI_H_ */

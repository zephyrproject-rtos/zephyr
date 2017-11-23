/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_POSIX_SOC_INF_CLOCK_H
#define _POSIX_POSIX_SOC_INF_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

void posix_soc_halt_cpu(void);
void posix_soc_interrupt_raised(void);
void posix_soc_boot_cpu(void);
void posix_soc_atomic_halt_cpu(unsigned int imask);
int posix_soc_is_cpu_running(void);

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_POSIX_SOC_INF_CLOCK_H */

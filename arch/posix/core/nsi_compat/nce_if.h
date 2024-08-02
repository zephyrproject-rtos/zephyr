/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NSI_COMMON_SRC_INCL_NCE_IF_H
#define NSI_COMMON_SRC_INCL_NCE_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Native simulator CPU start/stop emulation module interface */

void *nce_init(void);
void nce_terminate(void *this);
void nce_boot_cpu(void *this, void (*start_routine)(void));
void nce_halt_cpu(void *this);
void nce_wake_cpu(void *this);
int nce_is_cpu_running(void *this);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NCE_IF_H */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_CPUN_IF_H
#define NSI_COMMON_SRC_NSI_CPUN_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Equivalent interfaces to nsi_cpu<n>_* but for the native simulator internal use
 */

void nsif_cpun_pre_cmdline_hooks(int n);
void nsif_cpun_pre_hw_init_hooks(int n);
void nsif_cpun_boot(int n);
int nsif_cpun_cleanup(int n);
void nsif_cpun_irq_raised(int n);
void nsif_cpun_irq_raised_from_sw(int n);
void nsif_cpun_test_hook(int n, void *p);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_NSI_CPUN_IF_H */

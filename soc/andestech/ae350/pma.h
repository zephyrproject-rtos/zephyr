/*
 * Copyright (c) 2021 Andes Technology Corporation
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @brief Init PMA CSRs of each CPU core
 *
 * In SMP, each CPU has it's own PMA CSR and PMA CSR only affect one CPU.
 * We should configure CSRs of all CPUs to make memory attribute
 * (e.g. uncacheable) affects all CPUs.
 */
void pma_init_per_core(void);

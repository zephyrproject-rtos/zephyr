/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NSI_COMMON_SRC_INCL_NCT_IF_H
#define NSI_COMMON_SRC_INCL_NCT_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interface provided by the Native simulator CPU threading emulation
 *
 * A description of each function can be found in the C file
 *
 * In docs/NCT.md you can find more information
 */

void *nct_init(void (*fptr)(void *));
void nct_clean_up(void *this);
void nct_swap_threads(void *this, int next_allowed_thread_nbr);
void nct_first_thread_start(void *this, int next_allowed_thread_nbr);
int nct_new_thread(void *this, void *payload);
void nct_abort_thread(void *this, int thread_idx);
int nct_get_unique_thread_id(void *this, int thread_idx);
int nct_thread_name_set(void *this, int thread_idx, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NCT_IF_H */

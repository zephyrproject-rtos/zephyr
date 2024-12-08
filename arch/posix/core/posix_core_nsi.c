/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Interfacing between the POSIX arch and the Native Simulator (nsi) CPU thread emulator
 *
 * This posix architecture "bottom" will be used when building with the native simulator.
 */

#include "nct_if.h"

static void *te_state;

/*
 * Initialize the posix architecture
 */
void posix_arch_init(void)
{
	extern void posix_arch_thread_entry(void *pa_thread_status);
	te_state = nct_init(posix_arch_thread_entry);
}

/*
 * Clear the state of the POSIX architecture
 * free whatever memory it may have allocated, etc.
 */
void posix_arch_clean_up(void)
{
	nct_clean_up(te_state);
}

void posix_swap(int next_allowed_thread_nbr, int this_th_nbr)
{
	(void) this_th_nbr;
	nct_swap_threads(te_state, next_allowed_thread_nbr);
}

void posix_main_thread_start(int next_allowed_thread_nbr)
{
	nct_first_thread_start(te_state, next_allowed_thread_nbr);
}

int posix_new_thread(void *payload)
{
	return nct_new_thread(te_state, payload);
}

void posix_abort_thread(int thread_idx)
{
	nct_abort_thread(te_state, thread_idx);
}

int posix_arch_get_unique_thread_id(int thread_idx)
{
	return nct_get_unique_thread_id(te_state, thread_idx);
}

int posix_arch_thread_name_set(int thread_idx, const char *str)
{
	return nct_thread_name_set(te_state, thread_idx, str);
}

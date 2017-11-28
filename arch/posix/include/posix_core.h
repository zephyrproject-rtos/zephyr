/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _POSIX_CORE_H
#define _POSIX_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*_posix_core_thread_entry_t)(void *, void *, void *);
/* we redefine here the kernel.h _thread_entry_t to not need to include the
 * kernel.h and all its dependencies
 */

/*Type for unique thread identifiers*/
typedef unsigned long int p_thread_id_t;

typedef struct {
	_posix_core_thread_entry_t entry_point;
	void *arg1;
	void *arg2;
	void *arg3;

	p_thread_id_t thread_id;
} posix_thread_status_t;


void posix_core_new_thread(posix_thread_status_t *ptr);
void posix_core_swap(p_thread_id_t next_allowed_thread_nbr);
void posix_core_main_thread_start(p_thread_id_t next_allowed_thread_nbr);
void posix_core_init_multithreading(void);

/*The SOC shall define this function*/
void posix_soc_halt_cpu(void);


#ifdef __cplusplus
}
#endif

#endif /* _POSIX_CORE_H */

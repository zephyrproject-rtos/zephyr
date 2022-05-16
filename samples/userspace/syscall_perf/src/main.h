/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_H
#define MAIN_H

#include <zephyr/kernel.h>

#define csr_read(csr)						\
({								\
	register unsigned long __v;				\
	__asm__ volatile ("csrr %0, " #csr			\
				: "=r" (__v));			\
	__v;							\
})

#endif  /* MAIN_H */

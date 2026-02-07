/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for the boot vector table
 */

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <zephyr/toolchain.h>

GDATA(_vector_table)

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vector_table[];

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _VECTOR_TABLE__H_ */

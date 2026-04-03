/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for boot code
 */

#ifndef _BOOT_H_
#define _BOOT_H_

#ifndef _ASMLANGUAGE

extern void *_vector_table[];
extern void __start(void);

#endif /* _ASMLANGUAGE */

/* Offsets into the boot_params structure */
#define BOOT_PARAM_MPID_OFFSET		0
#define BOOT_PARAM_SP_OFFSET		8
#define BOOT_PARAM_VOTING_OFFSET	16

#endif /* _BOOT_H_ */

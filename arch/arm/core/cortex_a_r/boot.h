/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
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
#define BOOT_PARAM_IRQ_SP_OFFSET	4
#define BOOT_PARAM_FIQ_SP_OFFSET	8
#define BOOT_PARAM_ABT_SP_OFFSET	12
#define BOOT_PARAM_UDF_SP_OFFSET	16
#define BOOT_PARAM_SVC_SP_OFFSET	20
#define BOOT_PARAM_SYS_SP_OFFSET	24

#endif /* _BOOT_H_ */

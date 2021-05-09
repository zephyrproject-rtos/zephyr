/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
        /* Kept in RAM on non-XIP */
#ifdef CONFIG_XIP
	*(".kobject_data.rodata*")
#endif
#endif /* CONFIG_USERSPACE */


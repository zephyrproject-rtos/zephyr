/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
        /* Kept in RAM on non-XIP */
#ifdef CONFIG_XIP
	*(".priv_stacks.rodata*")
#endif
#endif /* CONFIG_USERSPACE */


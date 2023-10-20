/*
 * Copyright (c) 2024 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HPMICRO_CLOCKS_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HPMICRO_CLOCKS_COMMON_H_

#define CLOCK_SRC_GROUP_INVALID          (15U)
#define SYSCTL_RESOURCE_INVALID          (0xFFFFU)

#define MAKE_CLOCK_NAME(resource, src_group, node)			\
	(((SYSCTL_RESOURCE_ ## resource) << 16U) |			\
	 ((CLOCK_SRC_GROUP_ ## src_group) << 8U) |			\
	 (node))

#define MAKE_CLOCK_SRC(src_grp, index) (((CLOCK_SRC_GROUP_ ## src_grp) << 4U) | (index))

#endif

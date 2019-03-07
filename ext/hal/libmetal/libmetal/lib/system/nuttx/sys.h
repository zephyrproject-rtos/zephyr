/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/sys.h
 * @brief	NuttX system primitives for libmetal.
 */

#ifndef __METAL_SYS__H__
#error "Include metal/sys.h instead of metal/nuttx/sys.h"
#endif

#ifndef __METAL_NUTTX_SYS__H__
#define __METAL_NUTTX_SYS__H__

#ifdef __cplusplus
extern "C" {
#endif

#define METAL_INIT_DEFAULTS				\
{							\
	.log_handler	= (metal_log_handler)syslog,	\
	.log_level	= METAL_LOG_INFO,		\
}

/** Structure of nuttx libmetal runtime state. */
struct metal_state {

	/** Common (system independent) data. */
	struct metal_common_state common;
};

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_SYS__H__ */

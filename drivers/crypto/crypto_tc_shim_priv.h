/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief tinycrypt driver context info
 *
 * The file defines the structure which is used to store per session context
 * by the driver. Placed in common location so that crypto applications
 * can allocate memory for the required number of sessions, to free driver
 * from dynamic memory allocation.
 */

#ifndef __TC_SHIM_PRIV_H__
#define __TC_SHIM_PRIV_H__

#include <tinycrypt/aes.h>

struct tc_shim_drv_state {
	int in_use;
	struct tc_aes_key_sched_struct session_key;
};

#endif  /* __TC_SHIM_PRIV_H__ */

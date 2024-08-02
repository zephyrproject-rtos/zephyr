/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <system.h>

/* To simulate XIP on QEMU, we split RAM into two chunks, with the
 * higher-addressed chunk considered "ROM"
 */

#define _RESET_VECTOR		_ROM_ADDR
#define	_EXC_VECTOR		ALT_CPU_EXCEPTION_ADDR

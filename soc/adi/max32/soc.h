/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the MAX32xxx family processors.
 *
 */

#ifndef _MAX32_SOC_H_
#define _MAX32_SOC_H_

#include <wrap_max32xxx.h>

#if defined(CONFIG_SOC_FAMILY_MAX32_RV32)

/**
 * The RV32 core does not support the FENCE instruction, so redefine these intrinsics.
 */

#undef __sync_synchronize
#define __sync_synchronize() {}

#undef __atomic_thread_fence
#define __atomic_thread_fence(_arg) {}

#endif

#endif /* _MAX32_SOC_H_ */

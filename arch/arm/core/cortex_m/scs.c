/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M Series System Control Space
 *
 * Most of the SCS interface consists of simple bit-flipping methods, and is
 * implemented as inline functions in scs.h. This module thus contains only data
 * definitions and more complex routines, if needed.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>

/* the linker always puts this object at 0xe000e000 */
volatile struct __scs __scs_section __scs;

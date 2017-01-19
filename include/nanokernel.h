/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Public APIs for the nanokernel.
 */

#ifndef __NANOKERNEL_H__
#define __NANOKERNEL_H__

/* fundamental include files */

#include <stddef.h>
#include <stdint.h>
#include <toolchain.h>
#include <sections.h>

/* generic kernel public APIs */

#include <kernel_version.h>
#include <sys_clock.h>
#include <drivers/rand32.h>
#include <misc/slist.h>
#include <misc/dlist.h>

#include <kernel.h>

/* architecture-specific nanokernel public APIs */
#include <arch/cpu.h>

#endif /* __NANOKERNEL_H__ */

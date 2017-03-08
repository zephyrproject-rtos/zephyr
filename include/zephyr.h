/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR__H
#define _ZEPHYR__H

/*
 * Applications can identify whether they are built for Zephyr by
 * macro below. (It may be already defined by a makefile or toolchain.)
 */
#ifndef __ZEPHYR__
#define __ZEPHYR__
#endif

#include <kernel.h>

#ifdef CONFIG_MDEF
#include <sysgen.h>
#endif

#endif /* _ZEPHYR__H */

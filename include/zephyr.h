/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_H_
#define ZEPHYR_INCLUDE_ZEPHYR_H_

/*
 * Applications can identify whether they are built for Zephyr by
 * macro below. (It may be already defined by a makefile or toolchain.)
 */
#ifndef __ZEPHYR__
#define __ZEPHYR__
#endif

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Optional application main entry point
 *
 * A Zephyr application may optionally define a main() function which will
 * be called by the kernel after it completes the initialization.
 * Such a function must take no arguments and return no value.
 *
 * This function may return but does not have to.
 */
void main(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_H_ */

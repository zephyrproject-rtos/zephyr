/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __POSIX_UNISTD_H__
#define __POSIX_UNISTD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sys/types.h"

#ifndef __ZEPHYR__
#include_next <unistd.h>
#endif

unsigned sleep(unsigned int seconds);
int usleep(useconds_t useconds);

#endif
#ifdef __cplusplus
}
#endif	/* __POSIX_UNISTD_H__ */

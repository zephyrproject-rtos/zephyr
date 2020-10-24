/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_
#define ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_

#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XXX: Make this inline after sorting out dependency loops */
#ifdef CONFIG_ERRNO
int *z_errno(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ERRNO_PRIVATE_H_ */

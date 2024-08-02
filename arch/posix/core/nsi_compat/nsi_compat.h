/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ARCH_POSIX_CORE_NSI_COMPAT_H
#define ARCH_POSIX_CORE_NSI_COMPAT_H

#include "nsi_tracing.h"
#include "nsi_safe_call.h"

#ifdef __cplusplus
extern "C" {
#endif

void nsi_exit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif /* ARCH_POSIX_CORE_NSI_COMPAT_H */

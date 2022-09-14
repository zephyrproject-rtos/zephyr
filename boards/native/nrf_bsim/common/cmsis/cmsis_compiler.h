/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This header defines replacements for CMSIS compiler attributes.
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_CMSIS_COMPILER_H
#define BOARDS_POSIX_NRF52_BSIM_CMSIS_COMPILER_H

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PACKED
#define __PACKED __packed
#endif

#ifndef __PACKED_STRUCT
#define __PACKED_STRUCT struct __packed
#endif

#ifndef __PACKED_UNION
#define __PACKED_UNION union __packed
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_CMSIS_COMPILER_H */

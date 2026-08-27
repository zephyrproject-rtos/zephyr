/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H__
#define __SOC_H__

#include <am_mcu_apollo.h>

bool buf_in_nocache(uintptr_t buf, size_t len_bytes);

#endif /* __SOC_H__ */

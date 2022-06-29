/*
 * Copyright (c) 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_SECURE_H__
#define SOC_SECURE_H__

#include <stdint.h>

static inline void soc_secure_read_deviceid(uint32_t deviceid[2])
{
    memset(deviceid, 1, 2*sizeof(*deviceid));
}


#endif /* SOC_SECURE_H__ */

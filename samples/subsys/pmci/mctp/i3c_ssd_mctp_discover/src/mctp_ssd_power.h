/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MCTP_SSD_POWER_H__
#define __MCTP_SSD_POWER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Powers up and initializes the SSD endpoint(s) before MCTP/I3C bring-up. */
void mctp_ssd_power_init(void);

/* Initializes the I3C controller and enumerates the bus for MCTP. */
void mctp_i3c_initialization(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCTP_SSD_POWER_H__ */

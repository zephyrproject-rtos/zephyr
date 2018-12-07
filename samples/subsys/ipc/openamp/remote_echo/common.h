/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMOM_H__
#define COMMON_H__

#define RPMSG_CHAN_NAME         "rpmsg-openamp-demo-channel"

#if defined(CONFIG_SOC_SERIES_IMX7_M4) || defined(CONFIG_SOC_SERIES_IMX_6X_M4)
/* Values aligned with Linux Master side implementation */
#define SHM_START_ADDR		0x90040000
#define SHM_SIZE                0x40000
#define SHM_DEVICE_NAME         "ddr.shm"

#define VRING_COUNT             2
#define VRING_TX_ADDRESS        0x8fff0000
#define VRING_RX_ADDRESS        0x8fff8000
#define VRING_ALIGNMENT         0x1000
#define VRING_SIZE		256

#define IPM_LABEL		DT_IPM_IMX_MU_B_NAME
#define RPMSG_MU_CHANNEL	1

#else
#error "Current SoC not support"
#endif

#endif /* COMMON_H__ */

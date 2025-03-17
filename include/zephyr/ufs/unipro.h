/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_UFS_UNIPRO_H_
#define ZEPHYR_INCLUDE_UFS_UNIPRO_H_

/* PHY Adapter Attributes */
#define PA_ACTIVETXDATALANES	0x1560
#define PA_CONNECTEDTXDATALANES	0x1561
#define PA_TXGEAR		0x1568
#define PA_TXTERMINATION	0x1569
#define PA_HSSERIES		0x156A
#define PA_PWRMODE		0x1571
#define PA_ACTIVERXDATALANES	0x1580
#define PA_CONNECTEDRXDATALANES	0x1581
#define PA_RXGEAR		0x1583
#define PA_RXTERMINATION	0x1584
#define PA_TXHSADAPTTYPE	0x15D4

/* Transport Layer Attributes */
#define T_CONNECTIONSTATE	0x4020

/* Other Attributes */
#define VS_MPHYCFGUPDT		0xD085
#define VS_MPHYDISABLE		0xD0C1

#endif /* ZEPHYR_INCLUDE_UFS_UNIPRO_H_ */

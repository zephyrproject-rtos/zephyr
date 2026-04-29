/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UniPro attribute definitions for UFS.
 * @ingroup ufs_interface
 */

#ifndef ZEPHYR_INCLUDE_UFS_UNIPRO_H_
#define ZEPHYR_INCLUDE_UFS_UNIPRO_H_

/* PHY Adapter Attributes */
#define PA_ACTIVETXDATALANES	0x1560 /**< PHY Adapter active TX data lanes attribute. */
#define PA_CONNECTEDTXDATALANES	0x1561 /**< PHY Adapter connected TX data lanes attribute. */
#define PA_TXGEAR		0x1568 /**< PHY Adapter TX gear attribute. */
#define PA_TXTERMINATION	0x1569 /**< PHY Adapter TX termination attribute. */
#define PA_HSSERIES		0x156A /**< PHY Adapter HS series attribute. */
#define PA_PWRMODE		0x1571 /**< PHY Adapter power mode attribute. */
#define PA_ACTIVERXDATALANES	0x1580 /**< PHY Adapter active RX data lanes attribute. */
#define PA_CONNECTEDRXDATALANES	0x1581 /**< PHY Adapter connected RX data lanes attribute. */
#define PA_RXGEAR		0x1583 /**< PHY Adapter RX gear attribute. */
#define PA_RXTERMINATION	0x1584 /**< PHY Adapter RX termination attribute. */
#define PA_TXHSADAPTTYPE	0x15D4 /**< PHY Adapter TX HS adapt type attribute. */

/* Transport Layer Attributes */
#define T_CONNECTIONSTATE	0x4020 /**< Transport layer connection state attribute. */

/* Vendor Specific Attributes */
#define VS_MPHYCFGUPDT		0xD085 /**< Vendor-specific M-PHY configuration update attribute. */
#define VS_MPHYDISABLE		0xD0C1 /**< Vendor-specific M-PHY disable attribute. */

#endif /* ZEPHYR_INCLUDE_UFS_UNIPRO_H_ */

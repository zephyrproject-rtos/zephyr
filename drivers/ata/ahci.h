/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AHCI_H__
#define __AHCI_H__

#include <zephyr/sys/util.h>

#define AHCI_DEV_NULL   0
#define AHCI_DEV_SATA   1
#define AHCI_DEV_SEMB   2
#define AHCI_DEV_PM     3
#define AHCI_DEV_SATAPI 4

/*  Offset 00h: CAP – HBA Capabilities */
#define AHCI_CAP (0x0UL)

/*  Offset 0Ch: PI – Ports Implemented */
#define AHCI_PI (0x0CUL)

/* Offset 04h: GHC– Global HBA Control */
#define AHCI_GHC (0x04UL)
#define GHC_AE   BIT(31)
#define GHC_IE   BIT(1)
#define GHC_HR   BIT(0)

/*  Offset 00h: PxCLB – Port x Command List Base Address */
#define AHCI_P_CLB (0x0UL)

/*  Offset 04h: PxCLBU – Port x Command List Base Address Upper 32-bits */
#define AHCI_P_CLBU (0x04UL)

/*  Offset 08h: PxFB – Port x FIS Base Address */
#define AHCI_P_FB (0x08UL)

/*  Offset 0Ch: PxFBU – Port x FIS Base Address Upper 32-bits */
#define AHCI_P_FBU (0x0cUL)

/* Offset 10h: PxIS – Port x Interrupt Status */
#define AHCI_P_IS (0x10UL)
#define IS_TFES_B BIT(30)

/* Offset 14h: PxIE – Port x Interrupt Enable */
#define AHCI_P_IE (0x14UL)

/* Offset 18h: PxCMD – Port x Command and Status */
#define AHCI_P_CMD     (0x18UL)
#define CMD_ICC_ACTIVE BIT(28)
#define CMD_ST         BIT(0)
#define CMD_FRE        BIT(4)
#define CMD_FR         BIT(14)
#define CMD_CR         BIT(15)
#define CMD_SUD        BIT(1)

/* Offset 20h: PxTFD – Port x Task File Data */
#define AHCI_P_TFD  (0x20UL)
#define TFD_STS_BSY BIT(7)
#define TFD_STS_DRQ BIT(3)
#define TFD_STS_ERR BIT(0)

/* Offset: 0x28 PxSSTS - Port x Serial ATA Status */
#define AHCI_P_SSTS (0x28UL)
#define SSTS_IPM_M  GENMASK(11, 8)
#define IPM_ACTIVE  1
#define SSTS_DET_M  GENMASK(3, 0)
#define DET_PRESENT 3

/*  Offset 2Ch: PxSCTL – Port x Serial ATA Control (SCR2: SControl) */
#define AHCI_P_SCTL (0x2cUL)

/*  Offset 34h: PxSACT – Port x Serial ATA Active (SCR3: SActive) */
#define AHCI_P_SACT (0x34UL)

/* Offset 38h: PxCI – Port x Command Issue */
#define AHCI_P_CI (0x38UL)

typedef struct tagHBA_PORT {
	volatile uint32_t clb;      // 0x00, command list base address, 1K-byte aligned
	volatile int32_t clbu;      // 0x04, command list base address upper 32 bits
	volatile int32_t fb;        // 0x08, FIS base address, 256-byte aligned
	volatile int32_t fbu;       // 0x0C, FIS base address upper 32 bits
	volatile int32_t is;        // 0x10, interrupt status
	volatile int32_t ie;        // 0x14, interrupt enable
	volatile int32_t cmd;       // 0x18, command and status
	volatile int32_t rsv0;      // 0x1C, Reserved
	volatile int32_t tfd;       // 0x20, task file data
	volatile int32_t sig;       // 0x24, signature
	volatile int32_t ssts;      // 0x28, SATA status (SCR0:SStatus)
	volatile int32_t sctl;      // 0x2C, SATA control (SCR2:SControl)
	volatile int32_t serr;      // 0x30, SATA error (SCR1:SError)
	volatile int32_t sact;      // 0x34, SATA active (SCR3:SActive)
	volatile int32_t ci;        // 0x38, command issue
	volatile int32_t sntf;      // 0x3C, SATA notification (SCR4:SNotification)
	volatile int32_t fbs;       // 0x40, FIS-based switch control
	volatile int32_t devslp;    // 0x44, Device Sleep
	volatile int32_t rsv1[10];  // 0x48 ~ 0x6F, Reserved
	volatile int32_t vendor[4]; // 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef struct tagHBA_MEM {
	// 0x00 - 0x2B, Generic Host Control
	volatile int32_t cap;     // 0x00, Host capability
	volatile int32_t ghc;     // 0x04, Global host control
	volatile int32_t is;      // 0x08, Interrupt status
	volatile int32_t pi;      // 0x0C, Port implemented
	volatile int32_t vs;      // 0x10, Version
	volatile int32_t ccc_ctl; // 0x14, Command completion coalescing control
	volatile int32_t ccc_pts; // 0x18, Command completion coalescing ports
	volatile int32_t em_loc;  // 0x1C, Enclosure management location
	volatile int32_t em_ctl;  // 0x20, Enclosure management control
	volatile int32_t cap2;    // 0x24, Host capabilities extended
	volatile int32_t bohc;    // 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	volatile uint8_t rsv[0xA0 - 0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	volatile uint8_t vendor[0x100 - 0xA0];

	// 0x100 - 0x10FF, Port control registers
	HBA_PORT ports[32]; // 1 ~ 32
} HBA_MEM;

#endif /* end of include guard: __AHCI_H__ */

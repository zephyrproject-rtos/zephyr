/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: Add to include/sys/util.h */
#define BITFIELD(val, h, l)	(((val) & GENMASK(h, l)) >> l)

#define PCI_VENDOR_ID_INTEL	0x8086

#define PCI_DEVICE_ID_SKU7	0x452a
#define PCI_DEVICE_ID_SKU12	0x4518

/* TODO: Move to correct place NMI registers */

/* NMI Status and Control Register (NMI_STS_CNT) */
#define NMI_STS_CNT_REG		0x61
/* Set by any source of PCH SERR (SERR_NMI_STS) */
#define NMI_STS_SRC_SERR	BIT(7)
/* Mask for all source bits in the NMI_STS_CNT_REG */
#define NMI_STS_SRC_MASK	GENMASK(7, 6)

/**
 * Writing 1 SERR NMI are disabled and cleared, writing 0
 * SERR NMIs are enabled
 */
#define NMI_STS_SERR_EN		BIT(2)
/* Mask for all enable bits in the NMI_STS_CNT_REG */
#define NMI_STS_MASK_EN		GENMASK(3, 0)

/**
 * In-Band Error Correction Code (IBECC) protects data at a cache line
 * granularity (64 Bytes) with 16 bits SECDED code.
 * Reports following fields:
 *   - CMI (Converged Memory Interface) Address
 *   - Syndrome
 *   - Error Type (Correctable, Uncorrectable)
 */

/**
 * PCI Configuration space registers area
 */

/* Top of Upper Usable DRAM, offset 0xa8, 64 bit */
#define TOUUD_REG		0x2a
#define	TOUUD_MASK		GENMASK(38, 20)

/* Top of Low Usable DRAM, offset 0xbc, 32 bit */
#define TOLUD_REG		0x2f
#define TOLUD_MASK		GENMASK(31, 20)

/* Total amount of physical memory, offset 0xa0, 64 bit */
#define TOM_REG			0x28
#define TOM_MASK		GENMASK(38, 20)

/* Base address for the Host Memory Mapped Configuration space,
 * offset 0x48, 64 bit
 */
#define MCHBAR_REG		0x12
#define MCHBAR_MASK		GENMASK(38, 16)
#define MCHBAR_ENABLE		BIT(0)
/* Size of Host Memory Mapped Configuration space (64K) */
#define MCH_SIZE		0x10000

/* Capability register, offset 0xec, 32 bit */
#define CAPID0_C_REG		0x3b
#define CAPID0_C_IBECC_ENABLED	BIT(15)

/* Register controlling reporting error SERR, offset 0xc8, 16 bit */
#define ERRSTS_REG		0x32
#define ERRSTS_IBECC_COR	BIT(6) /* Correctable error */
#define ERRSTS_IBECC_UC		BIT(7) /* Uncorrectable error */

/* Register controlling Host Bridge responses to system errors,
 * offset 0xca, 16 bit
 *
 * TODO: Fix this after PCI access is fixed, now we have to access
 * ERRSTS_REG with 32 bit access and get this 16 bits
 */
#define ERRCMD_REG		0x32
#define ERRCMD_IBECC_COR	BIT(6)	/* Correctable error */
#define ERRCMD_IBECC_UC		BIT(7)	/* Uncorrectable error */

/**
 * Host Memory Mapped Configuration Space (MCHBAR) registers area
 */

#define CHANNEL_HASH		0x5024

/* ECC Injection Registers */

#define IBECC_INJ_ADDR_BASE	0xdd88
#define INJ_ADDR_BASE_MASK	GENMASK(38, 6)

#define IBECC_INJ_ADDR_MASK	0xdd80
#define INJ_ADDR_BASE_MASK_MASK	GENMASK(38, 6)

#define IBECC_INJ_ADDR_CTRL	0xdd98
#define INJ_CTRL_COR		0x1
#define INJ_CTRL_UC		0x5

/* Error Logging Registers */

/* ECC Error Log register, 64 bit (ECC_ERROR_LOG) */
#define IBECC_ECC_ERROR_LOG	0xdd70
/* Uncorrectable (Multiple-bit) Error Status (MERRSTS) */
#define ECC_ERROR_MERRSTS	BIT64(63)
/* Correctable Error Status (CERRSTS) */
#define ECC_ERROR_CERRSTS	BIT64(62)
#define ECC_ERROR_ERRTYPE(val)	BITFIELD(val, 63, 62)
/* CMI address of the address block of main memory where error happened */
#define ECC_ERROR_ERRADD(val)	((val) & GENMASK(38, 5))
/* ECC Error Syndrome (ERRSYND) */
#define ECC_ERROR_ERRSYND(val)	BITFIELD(val, 61, 46)

/* Parity Error Log (PARITY_ERR_LOG) */
#define IBECC_PARITY_ERROR_LOG	0xdd78
/* Error Status (ERRSTS) */
#define PARITY_ERROR_ERRSTS	BIT64(63)

/* Memory configuration registers */

#define DRAM_MAX_CHANNELS	2
#define DRAM_MAX_DIMMS		2

/* Memory channel decoding register, 32 bit */
#define MAD_INTER_CHAN		0x5000
#define INTER_CHAN_DDR_TYPE(v)	BITFIELD(v, 2, 0)
/* Enhanced channel mode for LPDDR4 */
#define INTER_CHAN_ECHM(v)	BITFIELD(v, 3, 3)
/* Channel L mapping to physical channel */
#define INTER_CHAN_CH_L_MAP(v)	BITFIELD(v, 4, 4)
/* Channel S size in multiples of 0.5GB */
#define INTER_CHAN_CH_S_SIZE	BITFIELD(v, 19, 12)

/* DRAM decode stage 2 registers, 32 bit */
#define MAD_INTRA_CH(index)	(0x5004 + index * sizeof(uint32_t))
/* Virtual DIMM L mapping to physical DIMM */
#define DIMM_L_MAP(v)		BITFIELD(v, 0, 0)

/* DIMM channel characteristic 2 registers, 32 bit */
#define MAD_DIMM_CH(index)	(0x500c + index * sizeof(uint32_t))
/* Size of DIMM L in 0.5GB multiples */
#define DIMM_L_SIZE(v)		(BITFIELD(v, 6, 0) << 29)
/* DIMM L width of DDR chips (DLW) */
#define DIMM_L_WIDTH(v)		BITFIELD(v, 8, 7)
/* Size of DIMM S in 0.5GB multiples */
#define DIMM_S_SIZE(v)		(BITFIELD(v, 22, 16) << 29)
/* DIMM S width of DDR chips (DSW) */
#define DIMM_S_WIDTH(v)		BITFIELD(v, 25, 24)


/* MC Channel Selection register, 32 bit */
#define CHANNEL_HASH		0x5024

/* MC Enhanced Channel Selection register, 32 bit */
#define CHANNEL_EHASH		0x5028

struct ibecc_error {
	uint32_t type;
	uint64_t address;
	uint16_t syndrome;
};

/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __XLNX_ADMA_H__
#define __XLNX_ADMA_H__

/* Interrupt registers bit field definitions */
#define XILINX_ADMA_DONE             BIT(10)
#define XILINX_ADMA_AXI_WR_DATA      BIT(9)
#define XILINX_ADMA_AXI_RD_DATA      BIT(8)
#define XILINX_ADMA_AXI_RD_DST_DSCR  BIT(7)
#define XILINX_ADMA_AXI_RD_SRC_DSCR  BIT(6)
#define XILINX_ADMA_IRQ_DST_ACCT_ERR BIT(5)
#define XILINX_ADMA_IRQ_SRC_ACCT_ERR BIT(4)
#define XILINX_ADMA_BYTE_CNT_OVRFL   BIT(3)
#define XILINX_ADMA_DST_DSCR_DONE    BIT(2)
#define XILINX_ADMA_INV_APB          BIT(0)

/* Control 0 register bit field definitions */
#define XILINX_ADMA_OVR_FETCH     BIT(7)
#define XILINX_ADMA_POINT_TYPE_SG BIT(6)
#define XILINX_ADMA_RATE_CTRL_EN  BIT(3)

/* Control 1 register bit field definitions */
#define XILINX_ADMA_SRC_ISSUE GENMASK(4, 0)

/* Data Attribute register bit field definitions */
#define XILINX_ADMA_ARBURST      GENMASK(27, 26)
#define XILINX_ADMA_ARCACHE      GENMASK(25, 22)
#define XILINX_ADMA_ARCACHE_OFST 22
#define XILINX_ADMA_ARQOS        GENMASK(21, 18)
#define XILINX_ADMA_ARQOS_OFST   18
#define XILINX_ADMA_ARLEN        GENMASK(17, 14)
#define XILINX_ADMA_ARLEN_OFST   14
#define XILINX_ADMA_AWBURST      GENMASK(13, 12)
#define XILINX_ADMA_AWCACHE      GENMASK(11, 8)
#define XILINX_ADMA_AWCACHE_OFST 8
#define XILINX_ADMA_AWQOS        GENMASK(7, 4)
#define XILINX_ADMA_AWQOS_OFST   4
#define XILINX_ADMA_AWLEN        GENMASK(3, 0)
#define XILINX_ADMA_AWLEN_OFST   0

/* Descriptor Attribute register bit field definitions */
#define XILINX_ADMA_AXCOHRNT     BIT(8)
#define XILINX_ADMA_AXCACHE      GENMASK(7, 4)
#define XILINX_ADMA_AXCACHE_OFST 4
#define XILINX_ADMA_AXQOS        GENMASK(3, 0)
#define XILINX_ADMA_AXQOS_OFST   0

/* Control register 2 bit field definitions */
#define XILINX_ADMA_ENABLE BIT(0)

/* Buffer Descriptor definitions */
#define XILINX_ADMA_DESC_CTRL_STOP     0x10
#define XILINX_ADMA_DESC_CTRL_COMP_INT 0x4
#define XILINX_ADMA_DESC_CTRL_SIZE_256 0x2
#define XILINX_ADMA_DESC_CTRL_COHRNT   0x1

#define XILINX_ADMA_START 0x1

/* Interrupt Mask specific definitions */
#define XILINX_ADMA_INT_ERR                                                                        \
	(XILINX_ADMA_AXI_RD_DATA | XILINX_ADMA_AXI_WR_DATA | XILINX_ADMA_AXI_RD_DST_DSCR |         \
	 XILINX_ADMA_AXI_RD_SRC_DSCR | XILINX_ADMA_INV_APB)
#define XILINX_ADMA_INT_OVRFL                                                                      \
	(XILINX_ADMA_BYTE_CNT_OVRFL | XILINX_ADMA_IRQ_SRC_ACCT_ERR | XILINX_ADMA_IRQ_DST_ACCT_ERR)
#define XILINX_ADMA_INT_DONE (XILINX_ADMA_DONE | XILINX_ADMA_DST_DSCR_DONE)
#define XILINX_ADMA_INT_EN_DEFAULT_MASK                                                            \
	(XILINX_ADMA_INT_DONE | XILINX_ADMA_INT_ERR | XILINX_ADMA_INT_OVRFL |                      \
	 XILINX_ADMA_DST_DSCR_DONE)

/* Max number of descriptors per channel */
#define XILINX_ADMA_NUM_DESCS 32

/* Max transfer size per descriptor */
#define XILINX_ADMA_MAX_TRANS_LEN 0x40000000

/* Max burst lengths */
#define XILINX_ADMA_MAX_DST_BURST_LEN 32768U
#define XILINX_ADMA_MAX_SRC_BURST_LEN 32768U

/* Reset values for data attributes */
#define XILINX_ADMA_AXCACHE_VAL 0xF

#define XILINX_ADMA_SRC_ISSUE_RST_VAL 0x1F

#define XILINX_ADMA_IDS_DEFAULT_MASK 0xFFF

#define XILINX_ADMA_DATA_ATTR_RST_VAL 0x0483D20F

/* Reset value for control reg attributes */
#define XILINX_ADMA_RESET_VAL  0x80
#define XILINX_ADMA_RESET_VAL1 0x3FF
#define XILINX_ADMA_RESET_VAL2 0x0

/* Bus width in bits */
#define XILINX_ADMA_BUS_WIDTH_64  64
#define XILINX_ADMA_BUS_WIDTH_128 128

#define XILINX_ADMA_WORD0_LSB_MASK (0xFFFFFFFFU)

#define POLL_TIMEOUT_COUNTER        1000000U
/* @name Channel Source/Destination Word1 register bit mask */
#define XILINX_ADMA_WORD1_MSB_MASK  (0x0001FFFFU) /**< MSB Address mask */
#define XILINX_ADMA_WORD1_MSB_SHIFT (32U)         /**< MSB Address shift */
#define XILINX_ADMA_WORD2_SIZE_MASK (0x3FFFFFFFU) /**< Size mask */

/* registers are same with different name */
struct __attribute__((__packed__)) dma_xilinx_adma_registers {
	uint32_t err_cr;         /* Error control register */
	uint32_t reserved_0[63]; /* Reserved space */
#if DT_HAS_COMPAT_STATUS_OKAY(xlnx_zynqmp_dma_1_0)
	uint32_t chan_isr; /* Interrupt Status Register */
	uint32_t chan_imr; /* Interrupt Mask Register */
	uint32_t chan_ien; /* Interrupt Enable Register */
	uint32_t chan_ids; /* Interrupt Disable Register */
#elif DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
	uint32_t chan_err_isr; /* Error Status Register */
	uint32_t chan_err_imr; /* Error Mask Register */
	uint32_t chan_err_ien; /* Error Enable Register */
	uint32_t chan_err_ids; /* Error Disable Register */
#endif
	uint32_t chan_cntrl0;       /* Channel Control Register 0 */
	uint32_t chan_cntrl1;       /* Channel Flow Control Register */
	uint32_t chan_fci;          /* Channel Control Register 1 */
	uint32_t chan_sts;          /* Channel Status Register */
	uint32_t chan_data_attr;    /* Channel Data Register */
	uint32_t chan_dscr_attr;    /* Channel DSCR Register */
	uint32_t chan_srcdscr_wrd0; /* SRC DSCR Word 0 */
	uint32_t chan_srcdscr_wrd1; /* SRC DSCR Word 1 */
	uint32_t chan_srcdscr_wrd2; /* SRC DSCR Word 2 */
	uint32_t chan_srcdscr_wrd3; /* SRC DSCR Word 3 */
	uint32_t chan_dstdscr_wrd0; /* DST DSCR Word 0 */
	uint32_t chan_dstdscr_wrd1; /* DST DSCR Word 1 */
	uint32_t chan_dstdscr_wrd2; /* DST DSCR Word 2 */
	uint32_t chan_dstdscr_wrd3; /* DST DSCR Word 3 */
	uint32_t chan_wronly_wrd0;  /* Write only Data Word 0 */
	uint32_t chan_wronly_wrd1;  /* Write only Data Word 1 */
	uint32_t chan_wronly_wrd2;  /* Write only Data Word 2 */
	uint32_t chan_wronly_wrd3;  /* Write only Data Word 3 */
	uint32_t chan_srcdesc;      /* SRC DSCR Start Address LSB Register */
	uint32_t chan_srcdesc_msb;  /* SRC DSCR Start Address MSB Register */
	uint32_t chan_dstdesc;      /* DST DSCR Start Address LSB Register */
	uint32_t chan_dstdesc_msb;  /* DST DSCR Start Address MSB Register */
	uint32_t reserved_1[9];     /* Reserved space */
	uint32_t chan_rate_cntrl;   /* Rate Control Count Register */
	uint32_t chan_irq_src_acct; /* SRC Interrupt Account Count Register */
	uint32_t chan_irq_dst_acct; /* DST Interrupt Account Count Register */
	uint32_t reserved_2[26];    /* Register Space */
	uint32_t chan_cntrl2;       /* Control Register */
	uint32_t reserved_3[129];   /* Register Space */
#if DT_HAS_COMPAT_STATUS_OKAY(amd_versal2_dma_1_0)
	uint32_t chan_isr; /* Interrupt Status Register */
	uint32_t chan_imr; /* Interrupt Mask Register */
	uint32_t chan_ien; /* Interrupt Enable Register */
	uint32_t chan_ids; /* Interrupt Disable Register */
	uint32_t chan_itr;
#endif
} __aligned(64);
#endif /* __XLNX_ADMA_H__ */

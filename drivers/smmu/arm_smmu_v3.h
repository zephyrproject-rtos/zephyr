/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_DRIVERS_ARM_SMMU_V3_H__
#define __ZEPHYR_DRIVERS_ARM_SMMU_V3_H__

#include "smmu_page_map.h"

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>
#include <zephyr/iommu/iommu.h>

#define SMMU_IDR0              0x0
#define IDR0_ST_LVL_2          BIT(27)
#define IDR0_STALL_MODEL_M     GENMASK(25, 24)
#define IDR0_STALL_MODEL_STALL 0x0
#define IDR0_STALL_MODEL_FORCE 0x2
#define IDR0_TTF_M             GENMASK(3, 2)
#define IDR0_TTF_AA32          0x1
#define IDR0_TTF_AA64          0x2
#define IDR0_TTF_ALL           0x3

#define SMMU_IDR1      0x4
#define IDR1_SIDSIZE_M GENMASK(5, 0)

#define SMMU_IDR5   0x014
#define IDR5_OAS_M  GENMASK(2, 0)
#define IDR5_OAS_32 0
#define IDR5_OAS_36 1
#define IDR5_OAS_40 2
#define IDR5_OAS_42 3
#define IDR5_OAS_44 4
#define IDR5_OAS_48 5
#define IDR5_OAS_52 6
#define IDR5_VAX_M  GENMASK(11, 10)
#define IDR5_VAX_48 0
#define IDR5_VAX_52 1

#define SMMU_CR0     (0x20)
#define CR0_VMW      GENMASK(8, 6)
#define CR0_ATSCHK   BIT(4)
#define CR0_CMDQEN   BIT(3)
#define CR0_EVENTQEN BIT(2)
#define CR0_PRIQEN   BIT(1)
#define CR0_SMMUEN   BIT(0)

#define SMMU_CR0ACK (0x24)

#define SMMU_CR1         (0x28)
#define CR1_TABLE_SH     GENMASK(11, 10)
#define CR1_TABLE_SH_NS  0x0
#define CR1_TABLE_SH_OS  0x2
#define CR1_TABLE_SH_IS  0x3
#define CR1_TABLE_OC     GENMASK(9, 8)
#define CR1_TABLE_OC_WBC 0x1
#define CR1_TABLE_IC     GENMASK(7, 6)
#define CR1_TABLE_IC_WBC 0x1
#define CR1_QUEUE_SH     GENMASK(5, 4)
#define CR1_QUEUE_SH_IS  0x3
#define CR1_QUEUE_OC     GENMASK(3, 2)
#define CR1_QUEUE_OC_WBC 0x1
#define CR1_QUEUE_IC     GENMASK(1, 0)
#define CR1_QUEUE_IC_WBC 0x1

#define SMMU_WB_CACHE (1)

#define SMMU_CR2      (0x2c)
#define CR2_PTM       (1UL << 2)
#define CR2_RECINVSID (1UL << 1)
#define CR2_E2H       (1UL << 0)

#define SMMU_STATUSR (0x40)
#define SMMU_GBPA    (0x44)
#define SMMU_AGBPA   (0x48)

#define SMMU_GERROR     (0x60)
#define GERROR_CMDQ_ERR (1UL << 0)

#define SMMU_STRTAB_BASE   (0x80)
#define STRTAB_BASE_RA     BIT64(62)
#define STRTAB_BASE_ADDR_M GENMASK(51, 6)

#define SMMU_STRTAB_BASE_CFG          (0x88)
#define STRTAB_BASE_CFG_LOG2SIZE_MASK GENMASK(5, 0)
#define STRTAB_BASE_CFG_SPLIT_MASK    GENMASK(10, 6)
#define STRTAB_BASE_CFG_FMT_MASK      GENMASK(17, 16)
#define STRTAB_BASE_CFG_FMT_2LVL      0x1
#define STRTAB_BASE_CFG_FMT_LINEAR    0x0

#define SMMU_CMDQ_BASE (0x90)
#define CMDQ_BASE_RA   (1UL << 62)
#define Q_BASE_ADDR_M  GENMASK(51, 5)
#define Q_LOG2SIZE_M   GENMASK(4, 0)

#define SMMU_CMDQ_PROD 0x98
#define CMDQ_PROD_WR_M GENMASK(19, 0)

#define SMMU_CMDQ_CONS  0x9c
#define CMDQ_CONS_RD_M  GENMASK(19, 0)
#define CMDQ_CONS_ERR_M GENMASK(30, 24)

#define SMMU_EVENTQ_BASE 0xa0
#define EVENTQ_BASE_WA   (1UL << 62)

#define SMMU_EVENTQ_PROD 0x100a8
#define SMMU_EVENTQ_CONS 0x100ac
#define EVENTQ_CONS_RD_M GENMASK(19, 0)

#define STRTAB_BASE_ALIGN (0x4000) // should be check again
#define STE_ALIGN         (0x4000)
#define CD_ALIGN          (0x4000)
#define SMMU_Q_ALIGN      (0x800)

#define STRTAB_L1_DESC_L2PTR_M GENMASK(51, 6)
#define STRTAB_L1_DESC_SPAM    GENMASK(4, 0)

#define STE0_VALID            BIT64(0)
#define STE0_CONFIG_M         GENMASK(3, 1)
#define STE0_CONFIG_ABORT     0x0
#define STE0_CONFIG_BYPASS    0x4
#define STE0_CONFIG_S1_TRANS  0x5
#define STE0_CONFIG_S2_TRANS  0x6
#define STE0_CONFIG_ALL_TRANS 0x7
#define STE0_S1CONTEXTPTR_S   6
#define STE0_S1CONTEXTPTR_M   GENMASK(51, 6)

#define STE1_S1CIR_M        GENMASK(3, 2)
#define STE1_S1CIR_NC       0x0
#define STE1_S1CIR_WBRA     0x1
#define STE1_S1CIR_WT       0x2
#define STE1_S1CIR_WB       0x3
#define STE1_S1COR_M        GENMASK(5, 4)
#define STE1_S1COR_NC       0x0
#define STE1_S1COR_WBRA     0x1
#define STE1_S1COR_WT       0x2
#define STE1_S1COR_WB       0x3
#define STE1_S1CSH_M        GENMASK(7, 6)
#define STE1_S1CSH_NS       0x0
#define STE1_S1CSH_OS       0x2
#define STE1_S1CSH_IS       0x3
#define STE1_S1STALLD       BIT64(27)
#define STE1_EATS_M         GENMASK(29, 28)
#define STE1_EATS_FULLATS   0x1
#define STE1_STRW_M         GENMASK(31, 30)
#define STE1_STRW_NS_EL1    0x0
#define STE1_STRW_NS_EL2    0x2
#define STE1_SHCFG_M        GENMASK(45, 44)
#define STE1_SHCFG_NS       0x0
#define STE1_SHCFG_INCOMING 0x1
#define STE1_SHCFG_OS       0x2
#define STE1_SHCFG_IS       0x3

#define CD0_T0SZ_M      GENMASK(5, 0)
#define CD0_TG0_M       GENMASK(7, 6)
#define CD0_TG0_4KB     0x0
#define CD0_TG0_64KB    0x1
#define CD0_TG0_16KB    0x2
#define CD0_IR0_M       GENMASK(9, 8)
#define CD0_IR0_NC      0x0
#define CD0_IR0_WBC_RWA 0x1
#define CD0_IR0_WTC_RA  0x2
#define CD0_IR0_WBC_RA  0x3
#define CD0_OR0_M       GENMASK(11, 10)
#define CD0_OR0_NC      0x0
#define CD0_OR0_WBC_RWA 0x1
#define CD0_OR0_WTC_RA  0x2
#define CD0_OR0_WBC_RA  0x3
#define CD0_SH0_S       12 /* Shareability for TT0 access */
#define CD0_SH0_M       GENMASK(13, 12)
#define CD0_SH0_NS      0x0
#define CD0_SH0_OS      0x2       /* Outer Shareable */
#define CD0_SH0_IS      0x3       /* Inner Shareable */
#define CD0_EPD1        BIT64(30) /* TT1 tt walk disable */
#define CD0_VALID       BIT64(31)
#define CD0_IPS_M       GENMASK(34, 32)
#define CD0_IPS_32BITS  0x0UL
#define CD0_IPS_36BITS  0x1UL
#define CD0_IPS_40BITS  0x2UL
#define CD0_IPS_42BITS  0x3UL
#define CD0_IPS_44BITS  0x4UL
#define CD0_IPS_48BITS  0x5UL
#define CD0_IPS_52BITS  0x6UL
#define CD0_AA64        BIT64(41)
#define CD0_R           BIT64(45)
#define CD0_A           BIT64(46)
#define CD0_ASET        BIT64(47)
#define CD0_ASID_M      GENMASK(63, 48)
#define CD1_TTB0_M      GENMASK(51, 4)

#define CMD_QUEUE_OPCODE_M GENMASK(7, 0)

#define CMD_PREFETCH_CONFIG 0x01
#define PREFETCH_0_SID_M    GENMASK(63, 32)

#define CMD_PREFETCH_ADDR 0x02

#define CMD_CFGI_STE     0x03
#define CFGI_0_STE_SID_M GENMASK(63, 32)
#define CFGI_1_LEAF      1

#define CMD_CFGI_STE_RANGE 0x04
#define CFGI_1_STE_RANGE_M GENMASK(4, 0)

#define CMD_CFGI_CD   0x05
#define CFGI_0_SSID_M GENMASK(31, 12)

#define CMD_CFGI_CD_ALL   0x06
#define CMD_CFGI_VMS_PIDM 0x07

#define CMD_TLBI_NH_ASID 0x11
#define TLBI_0_ASID_M    GENMASK(63, 48)

#define CMD_TLBI_NH_VA 0x12
#define TLBI_1_ADDR_M  GENMASK64(63, 12)
#define TLBI_1_LEAF    BIT(0)

#define CMD_TLBI_EL2_ALL 0x20

#define CMD_TLBI_NSNH_ALL 0x30

#define CMD_SYNC            0x46
#define SYNC_0_CS_M         GENMASK(13, 12)
#define SYNC_0_CS_SIG_NONE  0UL
#define SYNC_0_CS_SIG_IRQ   1U
#define SYNC_0_CS_SIG_SEV   2UL
#define SYNC_0_MSH_M        GENMASK(23, 22)
#define SYNC_0_MSH_NS       0UL
#define SYNC_0_MSH_OS       2UL
#define SYNC_0_MSH_IS       3UL
#define SYNC_0_MSIATTR_M    GENMASK(27, 24)
#define SYNC_0_MSIATTR_OIWB 0xfUL
#define SYNC_1_MSIADDRESS_M GENMASK(31, 2)

struct smmu_queue_local_copy {
	union {
		uint64_t val;
		struct {
			uint32_t prod;
			uint32_t cons;
		};
	};
};

struct smmu_queue {
	struct smmu_queue_local_copy lc;

	void *base;
	mem_addr_t base_dma;

	mm_reg_t prod_reg;
	mm_reg_t cons_reg;

	uint64_t q_base;
	int size_log2;
};

struct smmu_cmdq_entry {
	uint8_t opcode;
	union {
		struct {
			uint16_t asid;
			uint16_t vmid;
			mem_addr_t addr;
			bool leaf;
		} tlbi;
		struct {
			uint32_t sid;
			uint32_t ssid;
			bool leaf;
		} cfgi;
		struct {
			uint32_t sid;
		} prefetch;
		struct {
			uint64_t msiaddr;
		} sync;
	};
};

struct l1_desc {
	mem_addr_t l2pa;
	uint64_t *l2va;
	uint8_t span;
};

struct smmu_strtab {
	mem_addr_t vaddr;
	mem_addr_t paddr;
	uint64_t base;     // SMMU_STRTAB_BASE
	uint32_t base_cfg; // SMU_STRTAB_BASE_CFG

	struct l1_desc *l1; // allocated from kernel
	uint32_t num_l1_entries;
};

struct smmu_cd {
	mem_addr_t vaddr;
	mem_addr_t paddr;
	size_t size;
};

struct smmu_domain {
	struct iommu_domain iodom;
	sys_slist_t ctx_list;
	struct smmu_cd *cd;
	struct page_map pmap;
	uint16_t asid;
	struct k_mutex lock;
};

struct smmu_ctx {
	struct iommu_ctx ioctx;
	struct smmu_domain *domain;
	sys_snode_t next;
	const struct device *dev;
	uint32_t sid;
	bool bypass;
};

#endif /* __ZEPHYR_DRIVERS_ARM_SMMU_V3_H__ */

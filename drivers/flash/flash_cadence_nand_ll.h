/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CDNS_NAND_LL_H
#define CDNS_NAND_LL_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define NAND_INT_SEM_TAKE(param_ptr)						\
		COND_CODE_1(IS_ENABLED(CONFIG_CDNS_NAND_INTERRUPT_SUPPORT),     \
		(k_sem_take(&(param_ptr->interrupt_sem_t), K_FOREVER)), ())

#define CNF_GET_INIT_COMP(x)				(FIELD_GET(BIT(9), x))
#define CNF_GET_INIT_FAIL(x)				(FIELD_GET(BIT(10), x))
#define CNF_GET_CTRL_BUSY(x)				(FIELD_GET(BIT(8), x))
#define GET_PAGE_SIZE(x)				(FIELD_GET(GENMASK(15, 0), x))
#define GET_PAGES_PER_BLOCK(x)				(FIELD_GET(GENMASK(15, 0), x))
#define GET_SPARE_SIZE(x)				(FIELD_GET(GENMASK(31, 16), x))
#define ONFI_TIMING_MODE_SDR(x)				(FIELD_GET(GENMASK(15, 0), x))
#define ONFI_TIMING_MODE_NVDDR(x)			(FIELD_GET(GENMASK(31, 15), x))

/* Controller parameter registers */
#define CNF_GET_NLUNS(x)				(FIELD_GET(GENMASK(7, 0), x))
#define CNF_GET_DEV_TYPE(x)				(FIELD_GET(GENMASK(31, 30), x))

#define CNF_CTRLPARAM_VERSION				(0x800)
#define CNF_CTRLPARAM_FEATURE				(0x804)
#define CNF_CTRLPARAM_MFR_ID				(0x808)
#define CNF_CTRLPARAM_DEV_AREA				(0x80C)
#define CNF_CTRLPARAM_DEV_PARAMS0			(0x810)
#define CNF_CTRLPARAM_DEV_PARAMS1			(0x814)
#define CNF_CTRLPARAM_DEV_FEATUERS			(0x818)
#define CNF_CTRLPARAM_DEV_BLOCKS_PLUN		        (0x81C)
#define CNF_CTRLPARAM_ONFI_TIMING_0			(0x824)
#define CNF_CTRLPARAM(_base, _reg)			(_base + (CNF_CTRLPARAM_##_reg))

#define CNF_CMDREG_CTRL_STATUS				(0x118)
#define CNF_CMDREG(_base, _reg)				(_base + (CNF_CMDREG_##_reg))
#define PINSEL(_x)					(PINSEL##_x)
#define PIN(_x)						PINSEL(_x)##SEL

/*Hardware Features Support*/
#define CNF_HW_NF_16_SUPPORT(x)				(FIELD_GET(BIT(29), x))
#define CNF_HW_NVDDR_SS_SUPPORT(x)			(FIELD_GET(BIT(27), x))
#define CNF_HW_ASYNC_SUPPORT(x)				(FIELD_GET(BIT(26), x))
#define CNF_HW_DMA_DATA_WIDTH_SUPPORT(x)		(FIELD_GET(BIT(21), x))
#define CNF_HW_DMA_ADDR_WIDTH_SUPPORT(x)		(FIELD_GET(BIT(20), x))
#define CNF_HW_DI_PR_SUPPORT(x)				(FIELD_GET(BIT(14), x))
#define CNF_HW_ECC_SUPPORT(x)				(FIELD_GET(BIT(17), x))
#define CNF_HW_RMP_SUPPORT(x)				(FIELD_GET(BIT(12), x))
#define CNF_HW_DI_CRC_SUPPORT(x)			(FIELD_GET(BIT(8), x))
#define CNF_HW_WR_PT_SUPPORT(x)				(FIELD_GET(BIT(9), x))

/* Device types */
#define CNF_DT_UNKNOWN					(0x00)
#define CNF_DT_ONFI					(0x01)
#define CNF_DT_JEDEC					(0x02)
#define CNF_DT_LEGACY					(0x03)

/* Controller configuration registers */
#define CNF_CTRLCFG_TRANS_CFG0				(0x400)
#define CNF_CTRLCFG_TRANS_CFG1				(0x404)
#define CNF_CTRLCFG_LONG_POLL				(0x408)
#define CNF_CTRLCFG_SHORT_POLL				(0x40C)
#define CNF_CTRLCFG_DEV_STAT				(0x410)
#define CNF_CTRLCFG_DEV_LAYOUT				(0x424)
#define CNF_CTRLCFG_ECC_CFG0				(0x428)
#define CNF_CTRLCFG_ECC_CFG1				(0x42C)
#define CNF_CTRLCFG_MULTIPLANE_CFG			(0x434)
#define CNF_CTRLCFG_CACHE_CFG				(0x438)
#define CNF_CTRLCFG_DMA_SETTINGS			(0x43C)
#define CNF_CTRLCFG_FIFO_TLEVEL				(0x454)

#define CNF_CTRLCFG(_base, _reg)			(_base + (CNF_CTRLCFG_##_reg))

/* Data integrity registers */
#define CNF_DI_PAR_EN					(0)
#define CNF_DI_CRC_EN					(1)
#define CNF_DI_CONTROL					(0x700)
#define CNF_DI_INJECT0					(0x704)
#define CNF_DI_INJECT1					(0x708)
#define CNF_DI_ERR_REG_ADDR				(0x70C)
#define CNF_DI_INJECT2					(0x710)

#define CNF_DI(_base, _reg)				(_base + (CNF_DI_##_reg))

/* Thread idle timeout */
#define THREAD_IDLE_TIME_OUT				500U

/* Operation work modes */
#define CNF_OPR_WORK_MODE_SDR				(0)
#define CNF_OPR_WORK_MODE_NVDDR				(1)
#define CNF_OPR_WORK_MODE_SDR_MASK                      (GENMASK(1, 0))
#define CNF_OPR_WORK_MODE_NVDDR_MASK			(BIT(0))

#define ONFI_INTERFACE					(0x01)
#define NV_DDR_TIMING_READ				(16)

/* Interrupt register field offsets */
#define INTERRUPT_STATUS_REG				(0x0114)
#define THREAD_INTERRUPT_STATUS				(0x0138)

/* Mini controller DLL PHY controller register field offsets */
#define CNF_DLL_PHY_RST_N				(24)
#define CNF_DLL_PHY_EXT_WR_MODE				(17)
#define CNF_DLL_PHY_EXT_RD_MODE				(16)

#define CNF_MINICTRL_WP_SETTINGS			(0x1000)
#define CNF_MINICTRL_RBN_SETTINGS			(0x1004)
#define CNF_MINICTRL_CMN_SETTINGS			(0x1008)
#define CNF_MINICTRL_SKIP_BYTES_CFG			(0x100C)
#define CNF_MINICTRL_SKIP_BYTES_OFFSET			(0x1010)
#define CNF_MINICTRL_TOGGLE_TIMINGS0			(0x1014)
#define CNF_MINICTRL_TOGGLE_TIMINGS1			(0x1018)
#define CNF_MINICTRL_ASYNC_TOGGLE_TIMINGS		(0x101C)
#define CNF_MINICTRL_SYNC_TIMINGS			(0x1020)
#define CNF_MINICTRL_DLL_PHY_CTRL			(0x1034)

#define CNF_MINICTRL(_base, _reg)			(_base + (CNF_MINICTRL_##_reg))

/* Async mode register field offsets */
#define CNF_ASYNC_TIMINGS_TRH				FIELD_PREP(GENMASK(28, 24), 2)
#define CNF_ASYNC_TIMINGS_TRP				FIELD_PREP(GENMASK(20, 16), 4)
#define CNF_ASYNC_TIMINGS_TWH				FIELD_PREP(GENMASK(12, 8), 2)
#define CNF_ASYNC_TIMINGS_TWP				FIELD_PREP(GENMASK(4, 0), 4)

/* Mini controller common settings register field offsets */
#define CNF_CMN_SETTINGS_WR_WUP				(20)
#define CNF_CMN_SETTINGS_RD_WUP				(16)
#define CNF_CMN_SETTINGS_DEV16				(8)
#define CNF_CMN_SETTINGS_OPR				(0)

/* Interrupt status register. */
#define INTR_STATUS					(0x0110)
#define GINTR_ENABLE					(31)
#define INTERRUPT_DISABLE				(0)
#define INTERRUPT_ENABLE				(1)

/* CDMA Command type descriptor*/
/* CDMA Command type Erase*/
#define CNF_CMD_ERASE					(0x1000)
/* CDMA Program Page type */
#define CNF_CMD_WR					(0x2100)
/* CDMA Read Page type */
#define CNF_CMD_RD					(0x2200)
#define DMA_MS_SEL					(1)
#define VOL_ID						(0)
#define CDMA_CF_DMA_MASTER				(10)
#define CDMA_CF_DMA_MASTER_SET(x)			FIELD_PREP(BIT(CDMA_CF_DMA_MASTER), x)
#define F_CFLAGS_VOL_ID					(4)
#define F_CFLAGS_VOL_ID_SET(x)				FIELD_PREP(GENMASK(7, 4), x)
#define CDMA_CF_INT					(8)
#define CDMA_CF_INT_SET				        BIT(CDMA_CF_INT)
#define	COMMON_SET_DEVICE_16BIT				(8)
#define CDNS_READ					(0)
#define CDNS_WRITE					(1)
#define MAX_PAGES_IN_ONE_DSC				(8)
#define CFLAGS_MPTRPC					(0)
#define CFLAGS_MPTRPC_SET				FIELD_PREP(BIT(CFLAGS_MPTRPC), 1)
#define CFLAGS_FPTRPC					(1)
#define CFLAGS_FPTRPC_SET				FIELD_PREP(BIT(CFLAGS_FPTRPC), 1)
#define CFLAGS_CONT					(9)
#define CFLAGS_CONT_SET					FIELD_PREP(BIT(CFLAGS_CONT), 1)
#define CLEAR_ALL_INTERRUPT                             (0xFFFFFFFF)
#define ENABLE                                          (1)
#define DISABLE                                         (0)
#define DEV_STAT_DEF_VALUE                              (0x40400000)

/*Command Resister*/
#define CDNS_CMD_REG0					(0x00)
#define CDNS_CMD_REG1					(0x04)
#define CDNS_CMD_REG2					(0x08)
#define CDNS_CMD_REG3					(0x0C)
#define CMD_STATUS_PTR_ADDR				(0x10)
#define CMD_STAT_CMD_STATUS				(0x14)
#define CDNS_CMD_REG4					(0x20)

/* Cdns Nand Operation Modes*/
#define CT_CDMA_MODE					(0)
#define CT_PIO_MODE					(1)
#define CT_GENERIC_MODE					(3)
#define OPERATING_MODE_CDMA				(0)
#define OPERATING_MODE_PIO				(1)
#define OPERATING_MODE_GENERIC				(2)

#define THR_STATUS					(0x120)
#define CMD_0_THREAD_POS				(24)
#define CMD_0_THREAD_POS_SET(x)                         (FIELD_PREP(GENMASK(26, 24), x))
#define CMD_0_C_MODE					(30)
#define CMD_0_C_MODE_SET(x)                             (FIELD_PREP(GENMASK(31, 30), x))
#define CMD_0_VOL_ID_SET(x)                             (FIELD_PREP(GENMASK(19, 16), x))
#define PIO_SET_FEA_MODE				(0x0100)
#define SET_FEAT_TIMING_MODE_ADDRESS			(0x01)

 /* default thread number*/
#define NF_TDEF_TRD_NUM					(0)

/* NF device number */
#define NF_TDEF_DEV_NUM					(0)
#define F_OTE						(16)
#define F_BURST_SEL_SET(x)				(FIELD_PREP(GENMASK(7, 0), x))

/* DMA maximum burst size (0-127)*/
#define NF_TDEF_BURST_SEL				(127)
#define NF_DMA_SETTING					(0x043C)
#define NF_PRE_FETCH					(0x0454)
#define PRE_FETCH_VALUE					(1024/8)
#define NF_FIFO_TRIGG_LVL_SET(x)			(FIELD_PREP(GENMASK(15, 0), x))
#define NF_DMA_PACKAGE_SIZE_SET(x)			(FIELD_PREP(GENMASK(31, 16), x))
#define NF_FIFO_TRIGG_LVL				(0)

/* BCH correction strength */
#define NF_TDEF_CORR_STR				(0)
#define F_CSTAT_COMP					(15)
#define F_CSTAT_FAIL					(14)
#define HPNFC_STAT_INPR					(0)
#define HPNFC_STAT_FAIL					(2)
#define HPNFC_STAT_OK					(1)
#define NF_16_ENABLE					(1)
#define NF_16_DISABLE					(0)

/*PIO Mode*/
#define NF_CMD4_BANK_SET(x)				(FIELD_PREP(GENMASK(31, 24), x))
#define PIO_CMD0_CT_POS					(0)
#define PIO_CMD0_CT_SET(x)                              (FIELD_PREP(GENMASK(15, 0), x))
#define PIO_CF_INT					(20)
#define PIO_CF_INT_SET					(FIELD_PREP(BIT(PIO_CF_INT), 1))
#define PIO_CF_DMA_MASTER				(21)
#define PIO_CF_DMA_MASTER_SET(x)			(FIELD_PREP(BIT(PIO_CF_DMA_MASTER), x))

/* Phy registers*/
#define PHY_DQ_TIMING_REG_OFFSET			(0x00002000)
#define PHY_DQS_TIMING_REG_OFFSET			(0x00002004)
#define PHY_GATE_LPBK_OFFSET				(0x00002008)
#define PHY_DLL_MASTER_OFFSET				(0x0000200c)
#define PHY_CTRL_REG_OFFSET				(0x00002080)
#define PHY_TSEL_REG_OFFSET				(0x00002084)

#define PHY_CTRL_REG_SDR				(0x00004040)
#define PHY_TSEL_REG_SDR				(0x00000000)
#define PHY_DQ_TIMING_REG_SDR				(0x00000002)
#define PHY_DQS_TIMING_REG_SDR				(0x00100004)
#define PHY_GATE_LPBK_CTRL_REG_SDR			(0x00D80000)
#define PHY_DLL_MASTER_CTRL_REG_SDR			(0x00800000)
#define PHY_DLL_SLAVE_CTRL_REG_SDR			(0x00000000)

#define PHY_CTRL_REG_DDR				(0x00000000)
#define PHY_TSEL_REG_DDR				(0x00000000)
#define PHY_DQ_TIMING_REG_DDR				(0x00000002)
#define PHY_DQS_TIMING_REG_DDR				(0x00000004)
#define PHY_GATE_LPBK_CTRL_REG_DDR			(0x00380002)
#define PHY_DLL_MASTER_CTRL_REG_DDR			(0x001400fe)
#define PHY_DLL_SLAVE_CTRL_REG_DDR			(0x00003f3f)

/*SDMA*/
#define GCMD_TWB_VALUE					BIT64(6)
#define GCMCD_ADDR_SEQ					(1)
#define GCMCD_DATA_SEQ					(2)
#define ERASE_ADDR_SIZE					(FIELD_PREP(GENMASK64(13, 11), 3ULL))
#define GEN_SECTOR_COUNT				(1ULL)
#define GEN_SECTOR_COUNT_SET				(FIELD_PREP(GENMASK64(39, 32),\
								GEN_SECTOR_COUNT))
#define GEN_SECTOR_SIZE					(0x100ULL)
#define GEN_LAST_SECTOR_SIZE_SET(x)                     (FIELD_PREP(GENMASK64(55, 40), x))
#define SDMA_TRIGG					(21ULL)
#define SDMA_SIZE_ADDR					(0x0440)
#define SDMA_TRD_NUM_ADDR				(0x0444)
#define SDMA_ADDR0_ADDR					(0x044c)
#define SDMA_ADDR1_ADDR					(0x0450)
#define PAGE_READ_CMD					(0x3ULL)
#define PAGE_WRITE_CMD					(0x4ULL)
#define PAGE_ERASE_CMD					(0x6ULL)
#define PAGE_CMOD_CMD					(0x00)
#define PAGE_MAX_SIZE					(4)
#define PAGE_MAX_BYTES(x)				(FIELD_PREP(GENMASK64(13, 11), x))
#define GEN_CF_INT					(20)
#define GEN_CF_INT_SET(x)				(FIELD_PREP(BIT(GEN_CF_INT), x))
#define GEN_CF_INT_ENABLE				(1)
#define GEN_ADDR_POS					(16)
#define GEN_DIR_SET(x)					(FIELD_PREP(BIT64(11), x))
#define GEN_SECTOR_SET(x)			        (FIELD_PREP(GENMASK64(31, 16), x))
#define PAGE_WRITE_10H_CMD				(FIELD_PREP(GENMASK64(23, 16), 0x10ULL))
#define GEN_ADDR_WRITE_DATA(x)                          (FIELD_PREP(GENMASK64(63, 32), x))
#define NUM_ONE						(1)
#define U32_MASK_VAL					(0xFFFFFFFF)
#define BIT16_CHECK                                     (16)
#define IDLE_TIME_OUT                                   (5000U)
#define ROW_VAL_SET(x, y, z)                            (FIELD_PREP(GENMASK(x, y), z))
#define SET_FEAT_ADDR(x)				(FIELD_PREP(GENMASK(7, 0), x))
#define THREAD_VAL(x)					(FIELD_PREP(GENMASK(2, 0), x))
#define INCR_CMD_TYPE(x)				(x++)
#define DECR_CNT_ONE(x)					(--x)
#define GET_INIT_SET_CHECK(x, y)			(FIELD_GET(BIT(y), x))
struct nf_ctrl_version {
	uint32_t ctrl_rev:8;
	uint32_t ctrl_fix:8;
	uint32_t hpnfc_magic_number:16;
};

/* Cadence cdma command descriptor*/
struct cdns_cdma_command_descriptor {
	/* Next descriptor address*/
	uint64_t next_pointer;
	/* Flash address is a 32-bit address comprising of ROW ADDR. */
	uint32_t flash_pointer;
	uint16_t bank_number;
	uint16_t reserved_0;
	/*operation the controller needs to perform*/
	uint16_t command_type;
	uint16_t reserved_1;
	/* Flags for operation of this command. */
	uint16_t command_flags;
	uint16_t reserved_2;
	/* System/host memory address required for data DMA commands. */
	uint64_t memory_pointer;
	/* Status of operation. */
	uint64_t status;
	/* Address pointer to sync buffer location. */
	uint64_t sync_flag_pointer;
	/* Controls the buffer sync mechanism. */
	uint32_t sync_arguments;
	uint32_t reserved_4;
	/* Control data pointer. */
	uint64_t ctrl_data_ptr;

} __aligned(64);

/* Row Address */
union row_address {
	struct {
		uint32_t page_address:7;
		uint32_t block_address:10;
		uint32_t lun_address:3;
	} row_bit_reg;

	uint32_t row_address_raw;
};

/* device info structure */
struct cadence_nand_params {
	uintptr_t nand_base;
	uintptr_t sdma_base;
	uint8_t datarate_mode;
	uint8_t nluns;
	uint16_t page_size;
	uint16_t spare_size;
	uint16_t npages_per_block;
	uint32_t nblocks_per_lun;
	uint32_t block_size;
	uint8_t total_bit_row;
	uint8_t page_size_bit;
	uint8_t block_size_bit;
	uint8_t lun_size_bit;
	size_t page_count;
	unsigned long long device_size;
#ifdef CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	struct k_sem interrupt_sem_t;
#endif
} __aligned(32);

/* Global function Api */
int cdns_nand_init(struct cadence_nand_params *params);
int cdns_nand_read(struct cadence_nand_params *params, const void *buffer, uint32_t offset,
										uint32_t size);
int cdns_nand_write(struct cadence_nand_params *params, const void *buffer, uint32_t offset,
										uint32_t len);
int cdns_nand_erase(struct cadence_nand_params *params, uint32_t offset, uint32_t size);
#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
void cdns_nand_irq_handler_ll(struct cadence_nand_params *params);
#endif

#endif

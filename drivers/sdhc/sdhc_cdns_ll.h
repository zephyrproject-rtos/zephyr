/*
 * Copyright (C) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/drivers/sdhc.h>

/* HRS09 */
#define CDNS_HRS09_PHY_SW_RESET		BIT(0)
#define CDNS_HRS09_PHY_INIT_COMP	BIT(1)
#define CDNS_HRS09_EXT_WR_MODE		BIT(3)
#define CDNS_HRS09_RDCMD_EN_BIT		BIT(15)
#define CDNS_HRS09_RDDATA_EN_BIT	BIT(16)
#define CDNS_HRS09_EXT_RD_MODE(x)	((x) << 2)
#define CDNS_HRS09_EXTENDED_WR(x)	((x) << 3)
#define CDNS_HRS09_RDCMD_EN(x)		((x) << 15)
#define CDNS_HRS09_RDDATA_EN(x)		((x) << 16)

/* HRS00 */
#define CDNS_HRS00_SWR			BIT(0)

/* CMD_DATA_OUTPUT */
#define SDHC_CDNS_HRS16			0x40

/* SRS09 - Present State Register */
#define CDNS_SRS09_STAT_DAT_BUSY	BIT(2)
#define CDNS_SRS09_CI			BIT(16)

/* SRS10 - Host Control 1 (General / Power / Block-Gap / Wake-Up) */
#define LEDC     BIT(0)
#define DT_WIDTH BIT(1)
#define HS_EN    BIT(2)

#define CDNS_SRS10_DTW		1
#define CDNS_SRS10_EDTW		5
#define CDNS_SRS10_BP		BIT(8)

#define CDNS_SRS10_BVS		9
#define BUS_VOLTAGE_1_8_V	(5 << CDNS_SRS10_BVS)
#define BUS_VOLTAGE_3_0_V	(6 << CDNS_SRS10_BVS)
#define BUS_VOLTAGE_3_3_V	(7 << CDNS_SRS10_BVS)


/* data bus width */
#define WIDTH_BIT1		CDNS_SRS10_DTW
#define WIDTH_BIT4		CDNS_SRS10_DTW
#define WIDTH_BIT8		CDNS_SRS10_EDTW

/* SRS11 */
#define CDNS_SRS11_ICE		BIT(0)
#define CDNS_SRS11_ICS		BIT(1)
#define CDNS_SRS11_SDCE		BIT(2)
#define CDNS_SRS11_USDCLKFS	6
#define CDNS_SRS11_SDCLKFS	8
#define CDNS_SRS11_DTCV		16
#define CDNS_SRS11_SRFA		BIT(24)
#define CDNS_SRS11_SRCMD	BIT(25)
#define CDNS_SRS11_SRDAT	BIT(26)

/*
 * This value determines the interval by which DAT line timeouts are detected
 * The interval can be computed as below:
 * • 1111b - Reserved
 * • 1110b - t_sdmclk*2(27+2)
 * • 1101b - t_sdmclk*2(26+2)
 */
#define DTC_VAL				0xE
#define READ_CLK			(0xa << CDNS_SRS11_DTCV)
#define WRITE_CLK			(0xe << CDNS_SRS11_DTCV)


/* SRS12 */
#define CDNS_SRS12_CC			BIT(0)
#define CDNS_SRS12_TC			BIT(1)
#define CDNS_SRS12_EINT			BIT(15)

/* SDMA Buffer Boundary */
#define BUFFER_BOUNDARY_4K   0U
#define BUFFER_BOUNDARY_8K   1U
#define BUFFER_BOUNDARY_16K  2U
#define BUFFER_BOUNDARY_32K  3U
#define BUFFER_BOUNDARY_64K  4U
#define BUFFER_BOUNDARY_128K 5U
#define BUFFER_BOUNDARY_256K 6U
#define BUFFER_BOUNDARY_512K 7U

/* SRS01 */
#define CDNS_SRS01_BLK_SIZE		0U
#define CDNS_SRS01_SDMA_BUF		12
#define CDNS_SRS01_BLK_COUNT_CT		16

/* SRS15 Registers */
#define CDNS_SRS15_UMS			16
#define CDNS_SRS15_SDR12		(0 << CDNS_SRS15_UMS)
#define CDNS_SRS15_SDR25		(1 << CDNS_SRS15_UMS)
#define CDNS_SRS15_SDR50		(2 << CDNS_SRS15_UMS)
#define CDNS_SRS15_SDR104		(3 << CDNS_SRS15_UMS)
#define CDNS_SRS15_DDR50		(4 << CDNS_SRS15_UMS)
/* V18SE is 0 for DS and HS, 1 for UHS-I */
#define CDNS_SRS15_V18SE		BIT(19)
#define CDNS_SRS15_CMD23_EN		BIT(27)
/* HC4E is 0 means version 3.0 and 1 means v 4.0 */
#define CDNS_SRS15_HV4E			BIT(28)
#define CDNS_SRS15_BIT_AD_32		0U
#define CDNS_SRS15_BIT_AD_64		BIT(29)
#define CDNS_SRS15_PVE			BIT(31)

/* Combo PHY */
#define PHY_DQ_TIMING_REG		0x0
#define PHY_DQS_TIMING_REG		0x04
#define PHY_GATE_LPBK_CTRL_REG		0x08
#define PHY_DLL_MASTER_CTRL_REG		0x0C
#define PHY_DLL_SLAVE_CTRL_REG		0x10
#define PHY_CTRL_REG			0x80

#define PERIPHERAL_SDMMC_MASK		0x60
#define PERIPHERAL_SDMMC_OFFSET		6
#define DFI_INTF_MASK			0x1

/* PHY_DQS_TIMING_REG */
#define CP_USE_EXT_LPBK_DQS(x)		(x << 22)
#define CP_USE_LPBK_DQS(x)		(x << 21)
#define CP_USE_PHONY_DQS(x)		(x << 20)
#define CP_USE_PHONY_DQS_CMD(x)		(x << 19)

/* PHY_GATE_LPBK_CTRL_REG */
#define CP_SYNC_METHOD(x)		((x) << 31)
#define CP_SW_HALF_CYCLE_SHIFT(x)	((x) << 28)
#define CP_RD_DEL_SEL(x)		((x) << 19)
#define CP_UNDERRUN_SUPPRESS(x)		((x) << 18)
#define CP_GATE_CFG_ALWAYS_ON(x)	((x) << 6)

/* PHY_DLL_MASTER_CTRL_REG */
#define CP_DLL_BYPASS_MODE(x)		((x) << 23)
#define CP_DLL_START_POINT(x)		((x) << 0)

/* PHY_DLL_SLAVE_CTRL_REG */
#define CP_READ_DQS_CMD_DELAY(x)	((x) << 24)
#define CP_CLK_WRDQS_DELAY(x)		((x) << 16)
#define CP_CLK_WR_DELAY(x)		((x) << 8)
#define CP_READ_DQS_DELAY(x)		(x)

/* PHY_DQ_TIMING_REG */
#define CP_IO_MASK_ALWAYS_ON(x)		((x) << 31)
#define CP_IO_MASK_END(x)		((x) << 27)
#define CP_IO_MASK_START(x)		((x) << 24)
#define CP_DATA_SELECT_OE_END(x)	(x)

/* SW RESET REG */
#define SDHC_CDNS_HRS00			(0x00)
#define CDNS_HRS00_SWR			BIT(0)

/* PHY access port */
#define SDHC_CDNS_HRS04			0x10
#define CDNS_HRS04_ADDR			GENMASK(5, 0)

/* PHY data access port */
#define SDHC_CDNS_HRS05			0x14

/* eMMC control registers */
#define SDHC_CDNS_HRS06			0x18

/* PHY_CTRL_REG */
#define CP_PHONY_DQS_TIMING_MASK	0x3F
#define CP_PHONY_DQS_TIMING_SHIFT	4

/* SRS */
#define SDHC_CDNS_SRS00			0x200
#define SDHC_CDNS_SRS01			0x204
#define SDHC_CDNS_SRS02			0x208
#define SDHC_CDNS_SRS03			0x20c
#define SDHC_CDNS_SRS04			0x210
#define SDHC_CDNS_SRS05			0x214
#define SDHC_CDNS_SRS06			0x218
#define SDHC_CDNS_SRS07			0x21C
#define SDHC_CDNS_SRS08			0x220
#define SDHC_CDNS_SRS09			0x224
#define SDHC_CDNS_SRS10			0x228
#define SDHC_CDNS_SRS11			0x22C
#define SDHC_CDNS_SRS12			0x230
#define SDHC_CDNS_SRS13			0x234
#define SDHC_CDNS_SRS14			0x238
#define SDHC_CDNS_SRS15			0x23c
#define SDHC_CDNS_SRS21			0x254
#define SDHC_CDNS_SRS22			0x258
#define SDHC_CDNS_SRS23			0x25c

/* SRS00 */
#define CDNS_SRS00_SAAR			1

/* SRS03 */
#define CDNS_SRS03_CMD_START		BIT(31)
#define CDNS_SRS03_CMD_USE_HOLD_REG	BIT(29)
#define CDNS_SRS03_COM_IDX		24

/* Command type */
#define CDNS_SRS03_CMD_TYPE		22
#define CMD_STOP_ABORT_CMD		(3 << CDNS_SRS03_CMD_TYPE)
#define CMD_RESUME_CMD			(2 << CDNS_SRS03_CMD_TYPE)
#define CMD_SUSPEND_CMD			(1 << CDNS_SRS03_CMD_TYPE)

#define CDNS_SRS03_DATA_PRSNT		BIT(21)
#define CDNS_SRS03_CMD_IDX_CHK_EN	BIT(20)
#define CDNS_SRS03_RESP_CRCCE		BIT(19)
#define CDNS_SRS03_RESP_ERR		BIT(7)
#define CDNS_SRS03_MULTI_BLK_READ	BIT(5)
#define CDNS_SRS03_CMD_READ		BIT(4)

/* Response type select */
#define CDNS_SRS03_RES_TYPE_SEL		16
#define RES_TYPE_SEL_NO			(0 << CDNS_SRS03_RES_TYPE_SEL)
#define RES_TYPE_SEL_136		(1 << CDNS_SRS03_RES_TYPE_SEL)
#define RES_TYPE_SEL_48			(2 << CDNS_SRS03_RES_TYPE_SEL)
#define RES_TYPE_SEL_48_B		(3 << CDNS_SRS03_RES_TYPE_SEL)

/* Auto CMD Enable */
#define CDNS_SRS03_ACE		2
#define NO_AUTO_COMMAND		(0 << CDNS_SRS03_ACE)
#define AUTO_CMD12		(1 << CDNS_SRS03_ACE)
#define AUTO_CMD23		(2 << CDNS_SRS03_ACE)
#define AUTO_CMD_AUTO		(3 << CDNS_SRS03_ACE)

#define CDNS_SRS03_DMA_EN	BIT(0)
#define CDNS_SRS03_BLK_CNT_EN	BIT(1)

/* HRS07 - IO Delay Information Register */
#define SDHC_CDNS_HRS07			0x1c
#define CDNS_HRS07_IDELAY_VAL(x)	(x)
#define CDNS_HRS07_RW_COMPENSATE(x)	((x) << 16)

/* HRS09 - PHY Control and Status Register */
#define SDHC_CDNS_HRS09			0x24

/* HRS10 - Host Controller SDCLK start point adjustment */
#define SDHC_CDNS_HRS10			0x28

/* HCSDCLKADJ DATA; DDR Mode */
#define SDHC_HRS10_HCSDCLKADJ(x)	((x) << 16)

/* HRS16 */
#define CDNS_HRS16_WRCMD0_DLY(x)        (x)
#define CDNS_HRS16_WRCMD1_DLY(x)        ((x) << 4)
#define CDNS_HRS16_WRDATA0_DLY(x)       ((x) << 8)
#define CDNS_HRS16_WRDATA1_DLY(x)       ((x) << 12)
#define CDNS_HRS16_WRCMD0_SDCLK_DLY(x)  ((x) << 16)
#define CDNS_HRS16_WRCMD1_SDCLK_DLY(x)  ((x) << 20)
#define CDNS_HRS16_WRDATA0_SDCLK_DLY(x) ((x) << 24)
#define CDNS_HRS16_WRDATA1_SDCLK_DLY(x) ((x) << 28)

/* Shared Macros */
#define SDMMC_CDN(_reg)		(SDMMC_CDN_REG_BASE + \
				(SDMMC_CDN_##_reg))

/* MMC Peripheral Definition */
#define MMC_BLOCK_SIZE			512U
#define MMC_BLOCK_MASK			(MMC_BLOCK_SIZE - 1)
#define MMC_BOOT_CLK_RATE		(400 * 1000)

#define OCR_POWERUP			BIT(31)
#define OCR_HCS				BIT(30)

#define OCR_3_5_3_6			BIT(23)
#define OCR_3_4_3_5			BIT(22)
#define OCR_3_3_3_4			BIT(21)
#define OCR_3_2_3_3			BIT(20)
#define OCR_3_1_3_2			BIT(19)
#define OCR_3_0_3_1			BIT(18)
#define OCR_2_9_3_0			BIT(17)
#define OCR_2_8_2_9			BIT(16)
#define OCR_2_7_2_8			BIT(15)
#define OCR_VDD_MIN_2V7			GENMASK(23, 15)
#define OCR_VDD_MIN_2V0			GENMASK(14, 8)
#define OCR_VDD_MIN_1V7			BIT(7)

#define MMC_RSP_48			BIT(0)
#define MMC_RSP_136			BIT(1)	/* 136 bit response */
#define MMC_RSP_CRC			BIT(2)	/* expect valid crc */
#define MMC_RSP_CMD_IDX			BIT(3)	/* response contains cmd idx */
#define MMC_RSP_BUSY			BIT(4)	/* device may be busy */

/* JEDEC 4.51 chapter 6.12 */
#define MMC_RESPONSE_R1			(MMC_RSP_48 | MMC_RSP_CMD_IDX | MMC_RSP_CRC)
#define MMC_RESPONSE_R1B		(MMC_RESPONSE_R1 | MMC_RSP_BUSY)
#define MMC_RESPONSE_R2			(MMC_RSP_48 | MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RESPONSE_R3			(MMC_RSP_48)
#define MMC_RESPONSE_R4			(MMC_RSP_48)
#define MMC_RESPONSE_R5			(MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R6			(MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R7			(MMC_RSP_48 | MMC_RSP_CRC)
#define MMC_RESPONSE_NONE		0

/* Value randomly chosen for eMMC RCA, it should be > 1 */
#define MMC_FIX_RCA			6
#define RCA_SHIFT_OFFSET		16

#define CMD_EXTCSD_PARTITION_CONFIG	179
#define CMD_EXTCSD_BUS_WIDTH		183
#define CMD_EXTCSD_HS_TIMING		185
#define CMD_EXTCSD_SEC_CNT		212

#define PART_CFG_BOOT_PARTITION1_ENABLE	BIT(3)
#define PART_CFG_PARTITION1_ACCESS	1

/* Values in EXT CSD register */
#define MMC_BUS_WIDTH_1			0
#define MMC_BUS_WIDTH_4			1
#define MMC_BUS_WIDTH_8			2
#define MMC_BUS_WIDTH_DDR_4		5
#define MMC_BUS_WIDTH_DDR_8		6
#define MMC_BOOT_MODE_BACKWARD		0
#define MMC_BOOT_MODE_HS_TIMING		BIT(3)
#define MMC_BOOT_MODE_DDR		(2 << 3)

#define EXTCSD_SET_CMD			0
#define EXTCSD_SET_BITS			BIT(24)
#define EXTCSD_CLR_BITS			(2 << 24)
#define EXTCSD_WRITE_BYTES		(3 << 24)
#define EXTCSD_CMD(x)			(((x) & 0xff) << 16)
#define EXTCSD_VALUE(x)			(((x) & 0xff) << 8)
#define EXTCSD_CMD_SET_NORMAL		1

#define CSD_TRAN_SPEED_UNIT_MASK	GENMASK(2, 0)
#define CSD_TRAN_SPEED_MULT_MASK	GENMASK(6, 3)
#define CSD_TRAN_SPEED_MULT_SHIFT	3

#define STATUS_CURRENT_STATE(x)		(((x) & 0xf) << 9)
#define STATUS_READY_FOR_DATA		BIT(8)
#define STATUS_SWITCH_ERROR		BIT(7)
#define MMC_GET_STATE(x)		(((x) >> 9) & 0xf)
#define MMC_STATE_IDLE			0
#define MMC_STATE_READY			1
#define MMC_STATE_IDENT			2
#define MMC_STATE_STBY			3
#define MMC_STATE_TRAN			4
#define MMC_STATE_DATA			5
#define MMC_STATE_RCV			6
#define MMC_STATE_PRG			7
#define MMC_STATE_DIS			8
#define MMC_STATE_BTST			9
#define MMC_STATE_SLP			10

#define MMC_FLAG_CMD23			1

#define CMD8_CHECK_PATTERN		0xAA
#define VHS_2_7_3_6_V			BIT(8)

#define SD_SCR_BUS_WIDTH_1		BIT(8)
#define SD_SCR_BUS_WIDTH_4		BIT(10)

/* ADMA table component */
#define ADMA_DESC_ATTR_VALID		BIT(0)
#define ADMA_DESC_ATTR_END		BIT(1)
#define ADMA_DESC_ATTR_INT		BIT(2)
#define ADMA_DESC_ATTR_ACT1		BIT(4)
#define ADMA_DESC_ATTR_ACT2		BIT(5)
#define ADMA_DESC_TRANSFER_DATA		ADMA_DESC_ATTR_ACT2

/* Conf depends on SRS15.HV4E */
#define SDMA				0
#define ADMA2_32			(2 << 3)
#define ADMA2_64			(3 << 3)
/* here 0 defines the 64 Kb size */
#define MAX_64KB_PAGE			0

struct sdmmc_cmd {
	unsigned int	cmd_idx;
	unsigned int	cmd_arg;
	unsigned int	resp_type;
	unsigned int	resp_data[4];
};

struct sdhc_cdns_ops {
	/* init function for card */
	int (*init)(void);
	/* busy check function for card */
	int (*busy)(void);
	/* card_present function check for card */
	int (*card_present)(void);
	/* reset the card */
	int (*reset)(void);
	/* send command and respective argument */
	int (*send_cmd)(struct sdmmc_cmd *cmd, struct sdhc_data *data);
	/* io set up for card */
	int (*set_ios)(unsigned int clk, unsigned int width);
	/* prepare dma descriptors */
	int (*prepare)(uint32_t lba, uintptr_t buf, struct sdhc_data *data);
	/* cache invd api */
	int (*cache_invd)(int lba, uintptr_t buf, size_t size);
};

/* Combo Phy reg */
struct sdhc_cdns_combo_phy {
	uint32_t	cp_clk_wr_delay;
	uint32_t	cp_clk_wrdqs_delay;
	uint32_t	cp_data_select_oe_end;
	uint32_t	cp_dll_bypass_mode;
	uint32_t	cp_dll_locked_mode;
	uint32_t	cp_dll_start_point;
	uint32_t	cp_gate_cfg_always_on;
	uint32_t	cp_io_mask_always_on;
	uint32_t	cp_io_mask_end;
	uint32_t	cp_io_mask_start;
	uint32_t	cp_rd_del_sel;
	uint32_t	cp_read_dqs_cmd_delay;
	uint32_t	cp_read_dqs_delay;
	uint32_t	cp_sw_half_cycle_shift;
	uint32_t	cp_sync_method;
	uint32_t	cp_underrun_suppress;
	uint32_t	cp_use_ext_lpbk_dqs;
	uint32_t	cp_use_lpbk_dqs;
	uint32_t	cp_use_phony_dqs;
	uint32_t	cp_use_phony_dqs_cmd;
};

/* sdmmc reg */
struct sdhc_cdns_sdmmc {
	uint32_t	sdhc_extended_rd_mode;
	uint32_t	sdhc_extended_wr_mode;
	uint32_t	sdhc_hcsdclkadj;
	uint32_t	sdhc_idelay_val;
	uint32_t	sdhc_rdcmd_en;
	uint32_t	sdhc_rddata_en;
	uint32_t	sdhc_rw_compensate;
	uint32_t	sdhc_sdcfsh;
	uint32_t	sdhc_sdcfsl;
	uint32_t	sdhc_wrcmd0_dly;
	uint32_t	sdhc_wrcmd0_sdclk_dly;
	uint32_t	sdhc_wrcmd1_dly;
	uint32_t	sdhc_wrcmd1_sdclk_dly;
	uint32_t	sdhc_wrdata0_dly;
	uint32_t	sdhc_wrdata0_sdclk_dly;
	uint32_t	sdhc_wrdata1_dly;
	uint32_t	sdhc_wrdata1_sdclk_dly;
};

enum sdmmc_device_mode {
	/* Identification */
	SD_DS_ID,
	/* Default speed */
	SD_DS,
	/* High speed */
	SD_HS,
	/* Ultra high speed SDR12 */
	SD_UHS_SDR12,
	/* Ultra high speed SDR25 */
	SD_UHS_SDR25,
	/* Ultra high speed SDR`50 */
	SD_UHS_SDR50,
	/* Ultra high speed SDR104 */
	SD_UHS_SDR104,
	/* Ultra high speed DDR50 */
	SD_UHS_DDR50,
	/* SDR backward compatible */
	EMMC_SDR_BC,
	/* SDR */
	EMMC_SDR,
	/* DDR */
	EMMC_DDR,
	/* High speed 200Mhz in SDR */
	EMMC_HS200,
	/* High speed 200Mhz in DDR */
	EMMC_HS400,
	/* High speed 200Mhz in SDR with enhanced strobe */
	EMMC_HS400ES,
};

struct sdhc_cdns_params {
	uintptr_t	reg_base;
	uintptr_t	reg_phy;
	uintptr_t	desc_base;
	size_t		desc_size;
	int		clk_rate;
	int		bus_width;
	unsigned int	flags;
	enum sdmmc_device_mode	cdn_sdmmc_dev_type;
	uint32_t	combophy;
};

struct sdmmc_device_info {
	/* Size of device in bytes */
	unsigned long long	device_size;
	/* Block size in bytes */
	unsigned int		block_size;
	/* Max bus freq in Hz */
	unsigned int		max_bus_freq;
	/* OCR voltage */
	unsigned int		ocr_voltage;
	/* Type of MMC */
	enum sdmmc_device_mode	cdn_sdmmc_dev_type;
};

/*descriptor structure with 8 byte alignment*/
struct sdhc_cdns_desc {
	/* 8 bit attribute */
	uint8_t attr;
	/* reserved bits in desc */
	uint8_t reserved;
	/* page length for the descriptor */
	uint16_t len;
	/* lower 32 bits for buffer (64 bit addressing) */
	uint32_t addr_lo;
	/* higher 32 bits for buffer (64 bit addressing) */
	uint32_t addr_hi;
} __aligned(8);

void sdhc_cdns_sdmmc_init(struct sdhc_cdns_params *params, struct sdmmc_device_info *info,
	const struct sdhc_cdns_ops **cb_sdmmc_ops);

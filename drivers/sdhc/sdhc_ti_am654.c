/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am654_sdhci

#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

LOG_MODULE_REGISTER(ti_am654_sdhc, CONFIG_SDHC_LOG_LEVEL);

struct ti_am654_ss_regs {
	uint8_t RESERVED_1[0x14];
	volatile uint32_t CTL_CFG_2;
	volatile uint32_t CTL_CFG_3;
	uint8_t RESERVED_4[0xE4];
	volatile uint32_t PHY_CTRL_1;
	uint8_t RESERVED_2[0x08];
	volatile uint32_t PHY_CTRL_4;
	volatile uint32_t PHY_CTRL_5;
	uint8_t RESERVED_3[0x1C];
	volatile uint32_t PHY_STAT_1;
};

/* Controller Config 2 Register */
#define TI_AM654_CTL_CFG_2_SLOTTYPE GENMASK(31, 30)

/* PHY Control 1 Register */
#define TI_AM654_PHY_CTRL_1_IOMUX_ENABLE       BIT(31)
#define TI_AM654_PHY_CTRL_1_DR_TY              GENMASK(22, 20)
#define TI_AM654_PHY_CTRL_1_DR_TY_VAL_50_OHMS  (0x0)
#define TI_AM654_PHY_CTRL_1_DR_TY_VAL_33_OHMS  (0x1)
#define TI_AM654_PHY_CTRL_1_DR_TY_VAL_66_OHMS  (0x2)
#define TI_AM654_PHY_CTRL_1_DR_TY_VAL_100_OHMS (0x3)
#define TI_AM654_PHY_CTRL_1_DR_TY_VAL_40_OHMS  (0x4)
#define TI_AM654_PHY_CTRL_1_EN_RTRIM           BIT(16)
#define TI_AM654_PHY_CTRL_1_DLL_TRM_ICP        GENMASK(7, 4)
#define TI_AM654_PHY_CTRL_1_ENDLL              BIT(1)
#define TI_AM654_PHY_CTRL_1_PDB                BIT(0)

/* PHY Control 4 Register */
#define TI_AM654_PHY_CTRL_4_STRBSEL            GENMASK(31, 24)
#define TI_AM654_PHY_CTRL_4_STRBSEL_4BIT       GENMASK(27, 24)
#define TI_AM654_PHY_CTRL_4_OTAPDLYENA         BIT(20)
#define TI_AM654_PHY_CTRL_4_OTAPDLYSEL         GENMASK(15, 12)
#define TI_AM654_PHY_CTRL_4_ITAPCHGWIN         BIT(9)
#define TI_AM654_PHY_CTRL_4_ITAPDLYENA         BIT(8)
#define TI_AM654_PHY_CTRL_4_ITAPDLYSEL         GENMASK(4, 0)
#define TI_AM654_PHY_CTRL_4_ITAPDLYSEL_VAL_MAX (31)

/* PHY Control 5 Register */
#define TI_AM654_PHY_CTRL_5_SETDLYTXCLK            BIT(17)
#define TI_AM654_PHY_CTRL_5_SETDLYRXCLK            BIT(16)
#define TI_AM654_PHY_CTRL_5_FRQSEL                 GENMASK(10, 8)
#define TI_AM654_PHY_CTRL_5_FRQSEL_VAL_200_170_MHZ (0x0)
#define TI_AM654_PHY_CTRL_5_FRQSEL_VAL_170_140_MHZ (0x1)
#define TI_AM654_PHY_CTRL_5_FRQSEL_VAL_140_110_MHZ (0x2)
#define TI_AM654_PHY_CTRL_5_FRQSEL_VAL_110_80_MHZ  (0x3)
#define TI_AM654_PHY_CTRL_5_FRQSEL_VAL_80_50_MHZ   (0x4)
#define TI_AM654_PHY_CTRL_5_FRQSEL100              BIT(9)
#define TI_AM654_PHY_CTRL_5_FRQSEL50               BIT(8)
#define TI_AM654_PHY_CTRL_5_CLKBUFSEL              GENMASK(2, 0)

/* PHY Status 1 Register */
#define TI_AM654_PHY_STAT_1_CALDONE BIT(1)
#define TI_AM654_PHY_STAT_1_DLLRDY  BIT(0)

struct ti_am654_hc_regs {
	volatile uint32_t SYS_ADDR;
	volatile uint16_t BLOCK_SIZE;
	uint8_t RESERVED_1[0x2];
	volatile uint32_t ARGUMENT1;
	volatile uint16_t TRANSFER_MODE;
	volatile uint16_t COMMAND;
	volatile uint16_t RESPONSE_0;
	volatile uint16_t RESPONSE_1;
	volatile uint16_t RESPONSE_2;
	volatile uint16_t RESPONSE_3;
	volatile uint16_t RESPONSE_4;
	volatile uint16_t RESPONSE_5;
	volatile uint16_t RESPONSE_6;
	volatile uint16_t RESPONSE_7;
	volatile uint32_t DATA_PORT;
	volatile uint32_t PRESENTSTATE;
	volatile uint8_t HOST_CONTROL1;
	volatile uint8_t POWER_CONTROL;
	uint8_t RESERVED_2[0x2];
	volatile uint16_t CLOCK_CONTROL;
	uint8_t RESERVED_3[0x1];
	volatile uint8_t SOFTWARE_RESET;
	volatile uint16_t NORMAL_INTR_STS;
	volatile uint16_t ERROR_INTR_STS;
	volatile uint16_t NORMAL_INTR_STS_ENA;
	volatile uint16_t ERROR_INTR_STS_ENA;
	volatile uint16_t NORMAL_INTR_SIG_ENA;
	volatile uint16_t ERROR_INTR_SIG_ENA;
	uint8_t RESERVED_4[0x2];
	volatile uint16_t HOST_CONTROL2;
	volatile uint64_t CAPABILITIES;
	volatile uint64_t MAX_CURRENT_CAP;
	uint8_t RESERVED_5[0x8];
	volatile uint64_t ADMA_SYS_ADDRESS;
};

/* Block Size */
#define TI_AM654_BLOCK_SIZE_XFER_BLK_SIZE GENMASK(11, 0)

/* Transfer Mode */
#define TI_AM654_TRANSFER_MODE_MULTI_BLK_SEL          BIT(5)
#define TI_AM654_TRANSFER_MODE_DATA_XFER_DIR          BIT(4)
#define TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA           GENMASK(3, 2)
#define TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA_VAL_CMD12 (0x1)
#define TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA_VAL_CMD23 (0x2)
#define TI_AM654_TRANSFER_MODE_BLK_CNT_ENA            BIT(1)
#define TI_AM654_TRANSFER_MODE_DMA_ENA                BIT(0)

/* Command */
#define TI_AM654_COMMAND_CMD_INDEX                     GENMASK(13, 8)
#define TI_AM654_COMMAND_CMD_TYPE                      GENMASK(7, 6)
#define TI_AM654_COMMAND_CMD_TYPE_VAL_NORMAL           (0x0)
#define TI_AM654_COMMAND_DATA_PRESENT                  BIT(5)
#define TI_AM654_COMMAND_CMD_INDEX_CHK_ENA             BIT(4)
#define TI_AM654_COMMAND_CMD_CRC_CHK_ENA               BIT(3)
#define TI_AM654_COMMAND_RESP_TYPE_SEL                 GENMASK(1, 0)
#define TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_NONE        (0x0)
#define TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_136     (0x1)
#define TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48      (0x2)
#define TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48_BUSY (0x3)

/* Present State */
#define TI_AM654_PRESENTSTATE_SDIF_DAT3IN   BIT(23)
#define TI_AM654_PRESENTSTATE_SDIF_DAT2IN   BIT(22)
#define TI_AM654_PRESENTSTATE_SDIF_DAT1IN   BIT(21)
#define TI_AM654_PRESENTSTATE_SDIF_DAT0IN   BIT(20)
#define TI_AM654_PRESENTSTATE_CARD_INSERTED BIT(16)
#define TI_AM654_PRESENTSTATE_SDIF_DAT7IN   BIT(7)
#define TI_AM654_PRESENTSTATE_SDIF_DAT6IN   BIT(6)
#define TI_AM654_PRESENTSTATE_SDIF_DAT5IN   BIT(5)
#define TI_AM654_PRESENTSTATE_SDIF_DAT4IN   BIT(4)
#define TI_AM654_PRESENTSTATE_INHIBIT_DAT   BIT(1)
#define TI_AM654_PRESENTSTATE_INHIBIT_CMD   BIT(0)

/* Host Control 1 */
#define TI_AM654_HOST_CONTROL1_CD_SIG_SEL           BIT(7)
#define TI_AM654_HOST_CONTROL1_CD_TEST_LEVEL        BIT(6)
#define TI_AM654_HOST_CONTROL1_EXT_DATA_WIDTH       BIT(5)
#define TI_AM654_HOST_CONTROL1_HIGH_SPEED_ENA       BIT(2)
#define TI_AM654_HOST_CONTROL1_DATA_WIDTH           BIT(1)
#define TI_AM654_HOST_CONTROL1_DATA_WIDTH           BIT(1)
#define TI_AM654_HOST_CONTROL1_DMA_SELECT           GENMASK(4, 3)
#define TI_AM654_HOST_CONTROL1_DMA_SELECT_VAL_ADMA2 (0x2)

/* Power Control */
#define TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE          GENMASK(3, 1)
#define TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V3P3 (0x7)
#define TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V3P0 (0x6)
#define TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V1P8 (0x5)
#define TI_AM654_POWER_CONTROL_SD_BUS_POWER            BIT(0)

/* Clock Control */
#define TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL             GENMASK(15, 8)
#define TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_UPBITS      GENMASK(7, 6)
#define TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MAX     (0x3FF)
#define TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MASK_HI (0x300)
#define TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MASK_LO (0x0FF)
#define TI_AM654_CLOCK_CONTROL_CLKGEN_SEL               BIT(5)
#define TI_AM654_CLOCK_CONTROL_PLL_ENA                  BIT(3)
#define TI_AM654_CLOCK_CONTROL_SD_CLK_ENA               BIT(2)
#define TI_AM654_CLOCK_CONTROL_INT_CLK_STABLE           BIT(1)
#define TI_AM654_CLOCK_CONTROL_INT_CLK_ENA              BIT(0)

/* Software Reset */
#define TI_AM654_SOFTWARE_RESET_SWRST_FOR_DAT BIT(2)
#define TI_AM654_SOFTWARE_RESET_SWRST_FOR_CMD BIT(1)
#define TI_AM654_SOFTWARE_RESET_SWRST_FOR_ALL BIT(0)

/* Normal Interrupt Bits (common to several registers) */
#define TI_AM654_NORMAL_INTR_CARD_REMOVAL   BIT(7)
#define TI_AM654_NORMAL_INTR_CARD_INSERTION BIT(6)
#define TI_AM654_NORMAL_INTR_BUF_RD_READY   BIT(5)
#define TI_AM654_NORMAL_INTR_BUF_WR_READY   BIT(4)
#define TI_AM654_NORMAL_INTR_DMA_INTERRUPT  BIT(3)
#define TI_AM654_NORMAL_INTR_XFER_COMPLETE  BIT(1)
#define TI_AM654_NORMAL_INTR_CMD_COMPLETE   BIT(0)

/* Error interrupt bits */
#define TI_AM654_ERROR_INTR_ALL GENMASK(15, 0)

/* Host Control 2 */
#define TI_AM654_HOST_CONTROL2_BIT64_ADDRESSING           BIT(13)
#define TI_AM654_HOST_CONTROL2_HOST_VER40_ENA             BIT(12)
#define TI_AM654_HOST_CONTROL2_ADMA2_LEN_MODE             BIT(10)
#define TI_AM654_HOST_CONTROL2_SAMPLING_CLK_SELECT        BIT(7)
#define TI_AM654_HOST_CONTROL2_EXECUTE_TUNING             BIT(6)
#define TI_AM654_HOST_CONTROL2_V1P8_SIGNAL_ENA            BIT(3)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT            GENMASK(2, 0)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR12  (0x0)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR25  (0x1)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR50  (0x2)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR104 (0x3)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_DDR50  (0x4)
#define TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_HS400  (0x5)

/* Capabilities */
#define TI_AM654_CAPABILITIES_BUS_HS400_SUPPORT BIT64(63)

/* Max Current Capabilities */
#define TI_AM654_MAX_CURRENT_CAP_VDD2_1P8V GENMASK64(39, 32)
#define TI_AM654_MAX_CURRENT_CAP_VDD1_1P8V GENMASK64(23, 16)
#define TI_AM654_MAX_CURRENT_CAP_VDD1_3P0V GENMASK64(15, 8)
#define TI_AM654_MAX_CURRENT_CAP_VDD1_3P3V GENMASK64(7, 0)

static const uint8_t ti_am654_tuning_blk_8_bit[] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee,
	0xee, 0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff,
	0xff, 0xff, 0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff, 0x77, 0x77, 0xff, 0x77,
	0xbb, 0xdd, 0xee, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff, 0xff,
	0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static const uint8_t ti_am654_tuning_blk_4_bit[] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc, 0xc3, 0x3c, 0xcc, 0xff, 0xfe,
	0xff, 0xfe, 0xef, 0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb, 0xbf, 0xff,
	0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef, 0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc,
	0x3c, 0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee, 0xff, 0xfd, 0xff, 0xfd,
	0xdf, 0xff, 0xbf, 0xff, 0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

struct ti_am654_adma2_descriptor {
	bool valid: 1;
	bool end: 1;
	bool interrupt: 1;
	uint8_t action: 3;
	uint32_t length_hi: 10;
	uint32_t length_lo: 16;
	uint32_t addr_lo: 32;
	uint32_t addr_hi: 32;
	uint32_t: 32;

} __packed __aligned(4);

#define TI_AM654_ADMA2_DESC_ACTION_TRAN (0b100)
#define TI_AM654_ADMA2_DESC_LENGTH_MAX  (GENMASK(25, 0))

struct ti_am654_tap_delay_config {
	bool itap_delay_enable: 1;
	uint8_t itap_delay_value: 5;
	bool otap_delay_enable: 1;
	uint8_t otap_delay_value: 5;
};

enum ti_am654_reset_type {
	TI_AM654_RESET_TYPE_DAT = TI_AM654_SOFTWARE_RESET_SWRST_FOR_DAT,
	TI_AM654_RESET_TYPE_CMD = TI_AM654_SOFTWARE_RESET_SWRST_FOR_CMD,
	TI_AM654_RESET_TYPE_ALL = TI_AM654_SOFTWARE_RESET_SWRST_FOR_ALL,
};

#define TI_AM654_TIMING_MODE_NUM (SDHC_TIMING_HS400 + 1)

struct ti_am654_cmd_cfg {
	uint8_t resp_type: 2;
	bool crc_chk: 1;
	bool idx_chk: 1;
	bool data_present: 1;
};

struct ti_am654_tuning_window {
	uint8_t start;
	uint8_t end;
	uint8_t length;
};

/* SDHC configuration. */
struct ti_am654_config {
	DEVICE_MMIO_NAMED_ROM(host);
	DEVICE_MMIO_NAMED_ROM(subsys);
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_func)(const struct device *dev);
	const struct device *vmmc;
	const struct device *vqmmc;
	bool dll_present;
	bool is_embedded;
	bool fails_without_test_cd;
	uint8_t clkbuf_sel;
	bool strobe_sel_4_bit;
	uint8_t strobe_sel;
	bool dll_frqsel_2_bit;
	uint8_t drive_impedance;
	uint8_t current_trim;
};

struct ti_am654_data {
	DEVICE_MMIO_NAMED_RAM(host);
	DEVICE_MMIO_NAMED_RAM(subsys);
	struct ti_am654_tap_delay_config delay_config[TI_AM654_TIMING_MODE_NUM];
	struct sdhc_host_props props;
	struct sdhc_io ios;
	struct k_event irq_event;
	sdhc_interrupt_cb_t callback;
	void *user_data;

/* ADMA specific fields */
#ifdef CONFIG_SDHC_TI_AM654_ENABLE_ADMA
	/* ADMA descriptors */
	struct ti_am654_adma2_descriptor descs[MAX(CONFIG_SDHC_TI_AM654_ADMA_DESC_LEN, 1)];
#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	/* Cache-aligned residual buffers for DMA */
	uint8_t residual_start[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
	uint8_t residual_end[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
#endif /* CONFIG_CACHE_MANAGEMENT && CONFIG_DCACHE */
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_ADMA */
};

#define TI_AM654_K_EVENT_ERRORS(n)  (n << 16)
#define TI_AM654_K_EVENT_ALL_ERRORS TI_AM654_K_EVENT_ERRORS(TI_AM654_ERROR_INTR_ALL)

#define DEV_CFG(dev)     ((const struct ti_am654_config *)dev->config)
#define DEV_DATA(dev)    ((struct ti_am654_data *)dev->data)
#define DEV_HC_REGS(dev) ((struct ti_am654_hc_regs *)DEVICE_MMIO_NAMED_GET(dev, host))
#define DEV_SS_REGS(dev) ((struct ti_am654_ss_regs *)DEVICE_MMIO_NAMED_GET(dev, subsys))

#define TI_AM654_REG_POLL_RETRIES                 (100)
#define TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US (10)
#define TI_AM654_TUNING_RETRIES                   (5)

static int ti_am654_reset(const struct device *dev, enum ti_am654_reset_type type)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	int retries = TI_AM654_REG_POLL_RETRIES;

	/* do software reset */
	hc_regs->SOFTWARE_RESET |= type;

	/* wait for completion */
	while (hc_regs->SOFTWARE_RESET & type) {

		if (retries-- == 0) {
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static int ti_am654_reset_all(const struct device *dev)
{
	return ti_am654_reset(dev, TI_AM654_RESET_TYPE_ALL);
}

static ALWAYS_INLINE k_timeout_t ti_am654_timeout_from_msec(int timeout)
{
	return (timeout == SDHC_TIMEOUT_FOREVER ? K_FOREVER : K_MSEC(timeout));
}

static void ti_am654_read_cmd_resp(const struct device *dev, struct sdhc_command *cmd)
{
	enum sd_rsp_type type = cmd->response_type;
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);

	uint32_t r01 = (hc_regs->RESPONSE_1 << 16) | hc_regs->RESPONSE_0;
	uint32_t r23 = (hc_regs->RESPONSE_3 << 16) | hc_regs->RESPONSE_2;
	uint32_t r45 = (hc_regs->RESPONSE_5 << 16) | hc_regs->RESPONSE_4;
	uint32_t r67 = (hc_regs->RESPONSE_7 << 16) | hc_regs->RESPONSE_6;

	switch (type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		cmd->response[3] = 0;
		cmd->response[2] = 0;
		cmd->response[1] = 0;
		cmd->response[0] = 0;
		break;
	case SD_RSP_TYPE_R2:
		/* REP[119:0] */
		/* shift by 1 byte to make it [127:8] for parsing */
		cmd->response[3] = ((r67 & GENMASK(23, 0)) << 8) | (r45 >> 24);
		cmd->response[2] = (r45 << 8) | (r23 >> 24);
		cmd->response[1] = (r23 << 8) | (r01 >> 24);
		cmd->response[0] = (r01 << 8);
		break;
	default:
		/* REP[31:0] */
		cmd->response[3] = 0;
		cmd->response[2] = 0;
		cmd->response[1] = 0;
		cmd->response[0] = r01;
	}
}

static int ti_am654_request_cmd_send(const struct device *dev, struct sdhc_command *cmd,
				     struct ti_am654_cmd_cfg *cfg)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t tries = cmd->retries + 1;
	uint32_t events;

	while (tries--) {
		k_event_clear(&data->irq_event, TI_AM654_NORMAL_INTR_CMD_COMPLETE);

		hc_regs->ARGUMENT1 = cmd->arg;

		hc_regs->COMMAND = FIELD_PREP(TI_AM654_COMMAND_CMD_INDEX, cmd->opcode) |
				   FIELD_PREP(TI_AM654_COMMAND_CMD_TYPE,
					      TI_AM654_COMMAND_CMD_TYPE_VAL_NORMAL) |
				   FIELD_PREP(TI_AM654_COMMAND_RESP_TYPE_SEL, cfg->resp_type) |
				   FIELD_PREP(TI_AM654_COMMAND_CMD_INDEX_CHK_ENA, cfg->idx_chk) |
				   FIELD_PREP(TI_AM654_COMMAND_CMD_CRC_CHK_ENA, cfg->crc_chk) |
				   FIELD_PREP(TI_AM654_COMMAND_DATA_PRESENT, cfg->data_present);

		events = k_event_wait(&data->irq_event,
				      TI_AM654_NORMAL_INTR_CMD_COMPLETE |
					      TI_AM654_K_EVENT_ALL_ERRORS,
				      false, ti_am654_timeout_from_msec(cmd->timeout_ms));

		if (events & TI_AM654_K_EVENT_ALL_ERRORS) {
			/* any error */
			LOG_DBG("Command Error Status: 0x%x", events >> 16);

			ti_am654_reset(dev, TI_AM654_RESET_TYPE_CMD);

			if (cfg->data_present) {
				ti_am654_reset(dev, TI_AM654_RESET_TYPE_DAT);
			}

			return -EIO;

		} else if (events & TI_AM654_NORMAL_INTR_CMD_COMPLETE) {
			/* command transmission successful */
			ti_am654_read_cmd_resp(dev, cmd);
			return 0;
		}
	}

	return -ETIMEDOUT;
}

#ifdef CONFIG_SDHC_TI_AM654_ENABLE_ADMA

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
static void ti_am654_writeback_residuals(const struct device *dev, struct sdhc_data *dat)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	size_t length = dat->block_size * dat->blocks;
	uintptr_t address = (uintptr_t)dat->data;
	uintptr_t end_addr = address + length;
	size_t cache_line_size = sys_cache_data_line_size_get();
	size_t start_residual_bytes = (-address) & (cache_line_size - 1);
	size_t end_residual_bytes = end_addr & (cache_line_size - 1);

	if (start_residual_bytes != 0) {
		if (start_residual_bytes < length) {
			memcpy(dat->data, data->residual_start, start_residual_bytes);
		} else {
			memcpy(dat->data, data->residual_start, length);
			return;
		}
	}

	if (end_residual_bytes != 0) {
		memcpy((void *)(end_addr - end_residual_bytes), data->residual_end,
		       end_residual_bytes);
	}
}
#endif /* CONFIG_CACHE_MANAGEMENT	&& CONFIG_DCACHE */

static ALWAYS_INLINE struct ti_am654_adma2_descriptor
ti_am654_create_descriptor(uint64_t address, size_t length, bool end)
{
	return (struct ti_am654_adma2_descriptor){
		.valid = true,
		.end = end,
		.interrupt = false,
		.action = TI_AM654_ADMA2_DESC_ACTION_TRAN,
		.length_hi = FIELD_GET(GENMASK(25, 16), length),
		.length_lo = FIELD_GET(GENMASK(15, 0), length),
		.addr_lo = FIELD_GET(GENMASK64(31, 0), address),
		.addr_hi = FIELD_GET(GENMASK64(63, 32), address),
	};
}

static int ti_am654_request_data_setup_adma(const struct device *dev, struct sdhc_data *dat,
					    bool is_write)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	size_t length_left = dat->block_size * dat->blocks;
	uint64_t address = (uintptr_t)dat->data;
	uintptr_t end_addr = address + length_left;
	size_t cache_line_size = sys_cache_data_line_size_get();
	size_t start_residual_bytes = (-address) & (cache_line_size - 1);
	size_t end_residual_bytes = end_addr & (cache_line_size - 1);
	int i = 0;

	if (ARRAY_SIZE(data->descs) * TI_AM654_ADMA2_DESC_LENGTH_MAX < length_left) {
		LOG_ERR("number of descriptors %lu is less than required", ARRAY_SIZE(data->descs));
		return -EINVAL;
	}

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	/* if not cache aligned at the start, separate buffer, for rx */
	if ((!is_write) && start_residual_bytes != 0) {
		uintptr_t residual = (uintptr_t)data->residual_start;

		if (start_residual_bytes < length_left) {
			data->descs[i++] =
				ti_am654_create_descriptor(residual, start_residual_bytes, false);
		} else {
			/* exit early if this is the only descriptor */
			data->descs[i++] = ti_am654_create_descriptor(residual, length_left, true);
			goto start_adma;
		}

		/* invalidate if read */
		sys_cache_data_invd_range(data->residual_start, sizeof(data->residual_start));
		length_left -= start_residual_bytes;
		address += start_residual_bytes;
	}
#endif /* CONFIG_CACHE_MANAGEMENT	&& CONFIG_DCACHE */

	/* descriptors that require max descriptor length */
	while (length_left > TI_AM654_ADMA2_DESC_LENGTH_MAX) {
		data->descs[i++] =
			ti_am654_create_descriptor(address, TI_AM654_ADMA2_DESC_LENGTH_MAX, false);

		length_left -= TI_AM654_ADMA2_DESC_LENGTH_MAX;
		address += TI_AM654_ADMA2_DESC_LENGTH_MAX;
	}

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	/* if not cache aligned at the end, separate buffer after last aligned descriptor for rx */
	if ((!is_write) && end_residual_bytes != 0) {
		if (end_residual_bytes < length_left) {
			/* last descriptor with cache aligned data */
			data->descs[i++] = ti_am654_create_descriptor(
				address, length_left - end_residual_bytes, false);
		}

		/* invalidate if read */
		sys_cache_data_invd_range(data->residual_end, sizeof(data->residual_end));
		address = (uintptr_t)data->residual_end;
		length_left = end_residual_bytes;
	}
#endif /* CONFIG_CACHE_MANAGEMENT	&& CONFIG_DCACHE */

	/* last descriptor */
	data->descs[i++] = ti_am654_create_descriptor(address, length_left, true);

start_adma:
	/* flush the descriptors */
	sys_cache_data_flush_range(data->descs, sizeof(data->descs[0]) * i);

	if (is_write) {
		sys_cache_data_flush_range(dat->data, dat->blocks * dat->block_size);
	} else {
		uintptr_t aligned_region = (uintptr_t)dat->data + start_residual_bytes;
		size_t aligned_len =
			(dat->blocks * dat->block_size) - start_residual_bytes - end_residual_bytes;

		/* invalidate the aligned region, the residual buffers are already invalidated */
		sys_cache_data_invd_range((void *)aligned_region, aligned_len);
	}
	/* write descriptor address */
	hc_regs->ADMA_SYS_ADDRESS = (uintptr_t)data->descs;

	return 0;
}

#else  /* !CONFIG_SDHC_TI_AM654_ENABLE_ADMA */

static int ti_am654_request_data_write(const struct device *dev, struct sdhc_data *dat)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t *data_32;
	uint8_t *data_8 = dat->data;
	uint32_t block_cnt = dat->blocks;
	uint32_t events;

	dat->bytes_xfered = 0;

	while (block_cnt > 0) {
		data_32 = (uint32_t *)(data_8 + dat->bytes_xfered);

		events = k_event_wait(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_WR_READY, false,
				      ti_am654_timeout_from_msec(dat->timeout_ms));
		k_event_clear(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_WR_READY);

		if ((events & TI_AM654_NORMAL_INTR_BUF_WR_READY) == 0) {
			LOG_ERR("data port is not ready for writing");
			return -ETIMEDOUT;
		}

		for (int i = 0; i < DIV_ROUND_UP(dat->block_size, 4); i++) {
			hc_regs->DATA_PORT = *(data_32++);
		}

		dat->bytes_xfered += dat->block_size;
		block_cnt--;
	};

	return 0;
}

static int ti_am654_request_data_read(const struct device *dev, struct sdhc_data *dat)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t *data_32;
	uint8_t *data_8 = dat->data;
	uint32_t block_cnt = dat->blocks;
	uint32_t events;

	dat->bytes_xfered = 0;

	while (block_cnt > 0) {
		data_32 = (uint32_t *)(data_8 + dat->bytes_xfered);

		events = k_event_wait(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_RD_READY, false,
				      ti_am654_timeout_from_msec(dat->timeout_ms));
		k_event_clear(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_RD_READY);
		if ((events & TI_AM654_NORMAL_INTR_BUF_RD_READY) == 0) {
			LOG_ERR("data port is not ready for reading");
			return -ETIMEDOUT;
		}

		for (int i = 0; i < DIV_ROUND_UP(dat->block_size, 4); i++) {
			*(data_32++) = hc_regs->DATA_PORT;
		}

		dat->bytes_xfered += dat->block_size;
		block_cnt--;
	};

	return 0;
}
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_ADMA */

static int ti_am654_request_data_setup(const struct device *dev, struct sdhc_data *dat,
				       bool is_write)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	uint16_t transfer_mode = 0;

	hc_regs->BLOCK_SIZE = FIELD_PREP(TI_AM654_BLOCK_SIZE_XFER_BLK_SIZE, dat->block_size);

#ifdef CONFIG_SDHC_TI_AM654_ENABLE_ADMA
	transfer_mode |= TI_AM654_TRANSFER_MODE_DMA_ENA;
	int rv = ti_am654_request_data_setup_adma(dev, dat, is_write);

	if (rv != 0) {
		return rv;
	}
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_ADMA */

	if (!is_write) {
		transfer_mode |= TI_AM654_TRANSFER_MODE_DATA_XFER_DIR;
	}

	if (dat->blocks > 1) {
		transfer_mode |= TI_AM654_TRANSFER_MODE_BLK_CNT_ENA;
		transfer_mode |= TI_AM654_TRANSFER_MODE_MULTI_BLK_SEL;
		hc_regs->SYS_ADDR = dat->blocks; /* 32 bit block count in ver4 */

#ifdef CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP
		/* mandatory for sdr104 */
		if (DEV_DATA(dev)->ios.timing == SDHC_TIMING_SDR104 &&
		    IS_ENABLED(CONFIG_SDHC_TI_AM654_ENABLE_ADMA)) {
			transfer_mode |= FIELD_PREP(TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA,
						    TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA_VAL_CMD23);
		} else {
			transfer_mode |= FIELD_PREP(TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA,
						    TI_AM654_TRANSFER_MODE_AUTO_CMD_ENA_VAL_CMD12);
		}
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP */
	}

	hc_regs->TRANSFER_MODE = transfer_mode;

	return 0;
}

static int ti_am654_wait_for_dat0_high(const struct device *dev)
{
	int retries = TI_AM654_REG_POLL_RETRIES;

	while ((DEV_HC_REGS(dev)->PRESENTSTATE & TI_AM654_PRESENTSTATE_SDIF_DAT0IN) == 0) {
		if (retries-- == 0) {
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static void ti_am654_init_cmd_cfg(struct ti_am654_cmd_cfg *cfg, struct sdhc_command *cmd,
				  struct sdhc_data *dat)
{
	cfg->data_present = dat != NULL;

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		cfg->resp_type = TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_NONE;
		cfg->crc_chk = false;
		cfg->idx_chk = false;
		break;
	case SD_RSP_TYPE_R2:
		cfg->resp_type = TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_136;
		cfg->crc_chk = true;
		cfg->idx_chk = false;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		cfg->resp_type = TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48;
		cfg->crc_chk = false;
		cfg->idx_chk = false;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R7:
		cfg->resp_type = TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48;
		cfg->crc_chk = true;
		cfg->idx_chk = true;
		break;
	case SD_RSP_TYPE_R1b:
	case SD_RSP_TYPE_R5b:
		cfg->resp_type = TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48_BUSY;
		cfg->crc_chk = true;
		cfg->idx_chk = true;
		break;
	default:
		LOG_ERR("invalid response type");
	}
}

#ifndef CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP
static int ti_am654_request(const struct device *dev, struct sdhc_command *cmd,
			    struct sdhc_data *dat);
static int ti_am654_request_stop_transmission(const struct device *dev)
{
	int rv;
	struct sdhc_command stop_cmd = {
		.opcode = SD_STOP_TRANSMISSION,
		.arg = 0,
		.response_type = SD_RSP_TYPE_NONE,
		.retries = 0,
		.timeout_ms = 1000,
	};

	rv = ti_am654_request(dev, &stop_cmd, NULL);
	if (rv != 0) {
		LOG_ERR("failed to stop transmission");
	}

	return rv;
}
#endif /* !CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP */

static ALWAYS_INLINE bool ti_am654_is_cmd_write(uint32_t opcode)
{
	return opcode == SD_WRITE_SINGLE_BLOCK || opcode == SD_WRITE_MULTIPLE_BLOCK;
}

static int ti_am654_request(const struct device *dev, struct sdhc_command *cmd,
			    struct sdhc_data *dat)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	bool is_write = ti_am654_is_cmd_write(cmd->opcode);
	struct ti_am654_cmd_cfg cfg;
	int rv;

	data->irq_event.events = 0;
	ti_am654_init_cmd_cfg(&cfg, cmd, dat);

	if (hc_regs->PRESENTSTATE & TI_AM654_PRESENTSTATE_INHIBIT_CMD) {
		LOG_ERR("command line is already busy");
		return -EBUSY;
	}

	if (cfg.data_present) {
		rv = ti_am654_request_data_setup(dev, dat, is_write);
		if (rv != 0) {
			return rv;
		}

		if (hc_regs->PRESENTSTATE & TI_AM654_PRESENTSTATE_INHIBIT_DAT) {
			LOG_ERR("data line is already busy");
			return -EBUSY;
		}
	}

	rv = ti_am654_request_cmd_send(dev, cmd, &cfg);
	if (rv != 0) {
		return rv;
	}

	if (cfg.data_present) {
		uint32_t events;

#ifndef CONFIG_SDHC_TI_AM654_ENABLE_ADMA
		if (is_write) {
			rv = ti_am654_request_data_write(dev, dat);
		} else {
			rv = ti_am654_request_data_read(dev, dat);
		}

		if (rv != 0) {
			ti_am654_reset(dev, TI_AM654_RESET_TYPE_CMD | TI_AM654_RESET_TYPE_DAT);
			return rv;
		}
#endif /* !CONFIG_SDHC_TI_AM654_ENABLE_ADMA */

		events = k_event_wait(&data->irq_event,
				      TI_AM654_NORMAL_INTR_XFER_COMPLETE |
					      TI_AM654_K_EVENT_ALL_ERRORS,
				      false, ti_am654_timeout_from_msec(dat->timeout_ms));

		if (events & TI_AM654_K_EVENT_ALL_ERRORS) {
			/* any error */
			LOG_DBG("Xfer Error Status: 0x%x", events >> 16);
			ti_am654_reset(dev, TI_AM654_RESET_TYPE_CMD | TI_AM654_RESET_TYPE_DAT);
			return -EIO;

		} else if (events & TI_AM654_NORMAL_INTR_XFER_COMPLETE) {
			/* xfer completed successfully */
#ifndef CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP
			if (dat->blocks > 1) {
				rv = ti_am654_request_stop_transmission(dev);
				if (rv != 0) {
					ti_am654_reset(dev, TI_AM654_RESET_TYPE_CMD |
								    TI_AM654_RESET_TYPE_DAT);
					return rv;
				}
			}
#endif /* !CONFIG_SDHC_TI_AM654_ENABLE_AUTO_STOP */

#if defined(CONFIG_SDHC_TI_AM654_ENABLE_ADMA) && defined(CONFIG_CACHE_MANAGEMENT) &&               \
	defined(CONFIG_DCACHE)
			if (!is_write) {
				ti_am654_writeback_residuals(dev, dat);
			}
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_ADMA && CONFIG_CACHE_MANAGEMENT	&& CONFIG_DCACHE */

			if (cfg.resp_type == TI_AM654_COMMAND_RESP_TYPE_SEL_VAL_LEN_48_BUSY) {
				rv = ti_am654_wait_for_dat0_high(dev);

				if (rv == -ETIMEDOUT) {
					LOG_ERR("Timed out while waiting for DAT0 to go high");
					ti_am654_reset(dev, TI_AM654_RESET_TYPE_CMD |
								    TI_AM654_RESET_TYPE_DAT);
					return -EIO;
				}
			}

			return 0;

			/* event timed out */
		} else {
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int ti_am654_wait_for_internal_clock_stable(const struct device *dev)
{
	int retries = TI_AM654_REG_POLL_RETRIES;

	while ((DEV_HC_REGS(dev)->CLOCK_CONTROL & TI_AM654_CLOCK_CONTROL_INT_CLK_STABLE) == 0) {
		if (retries-- == 0) {
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static int ti_am654_configure_clock(const struct device *dev, enum sdhc_clock_speed clock)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t multiplier = data->props.host_caps.clk_multiplier;
	uint32_t base = MHZ(data->props.host_caps.sd_base_clk);
	bool prog_clk_mode = false;
	uint16_t frqsel = 0;
	uint16_t divisor = 0;
	int rv;

	/* disable DLL for now */
	DEV_SS_REGS(dev)->PHY_CTRL_1 &= ~TI_AM654_PHY_CTRL_1_ENDLL;

	hc_regs->CLOCK_CONTROL = 0;

	if (clock == 0) {
		return 0;
	}

	/* Programmable Clock Mode */
	if (multiplier != 0) {
		for (frqsel = 0; frqsel <= TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MAX; frqsel++) {
			divisor = frqsel + 1;
			if ((base * multiplier) / divisor <= clock) {
				prog_clk_mode = true;
				break;
			}
		}
		/* 10-bit Divided Clock Mode */
	} else if (base <= clock) {
		frqsel = 0;
		divisor = 1;
	} else {
		for (frqsel = 1; frqsel <= TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MAX; frqsel++) {
			divisor = frqsel << 1;
			if (base / divisor <= clock) {
				break;
			}
		}

		if (frqsel > TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MAX) {
			LOG_ERR("Configured clock speed %uHz is too low", clock);
			return -EINVAL;
		}
	}

	LOG_DBG("clock divisor: %u, frqsel: %u", divisor, frqsel);

	hc_regs->CLOCK_CONTROL =
		/* frqsel lo bits */
		FIELD_PREP(TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL,
			   FIELD_GET(TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MASK_LO, frqsel)) |
		/* freqsel hi bits */
		FIELD_PREP(TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_UPBITS,
			   FIELD_GET(TI_AM654_CLOCK_CONTROL_SDCLK_FRQSEL_VAL_MASK_HI, frqsel)) |
		/* programmable clock mode */
		FIELD_PREP(TI_AM654_CLOCK_CONTROL_CLKGEN_SEL, prog_clk_mode) |
		TI_AM654_CLOCK_CONTROL_INT_CLK_ENA;

	/* wait for internal clock to become stable */
	rv = ti_am654_wait_for_internal_clock_stable(dev);
	if (rv == -ETIMEDOUT) {
		LOG_ERR("timed out while waiting for internal clock to become stable");
		return -EIO;
	}

	/* enable PLL for SD */
	if (!DEV_CFG(dev)->is_embedded) {
		hc_regs->CLOCK_CONTROL |= TI_AM654_CLOCK_CONTROL_PLL_ENA;

		/* wait for internal clock to become stable */
		rv = ti_am654_wait_for_internal_clock_stable(dev);
		if (rv == -ETIMEDOUT) {
			LOG_ERR("timed out while waiting for internal clock to become "
				"stable");
			return -EIO;
		}
	}

	hc_regs->CLOCK_CONTROL |= TI_AM654_CLOCK_CONTROL_SD_CLK_ENA;

	return 0;
}

static int ti_am654_configure_delay_locked_loop(const struct device *dev,
						enum sdhc_timing_mode mode,
						enum sdhc_clock_speed clock)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	const struct ti_am654_config *config = DEV_CFG(dev);
	uint32_t phy_ctrl_5;
	uint32_t phy_ctrl_1;
	uint8_t impedance_val;
	int retries;

	/* read phy_ctrl5 */
	phy_ctrl_5 = ss_regs->PHY_CTRL_5;

	/* modify phy_ctrl5 */
	if (config->dll_frqsel_2_bit) {
		switch (clock) {
		case MHZ(200):
			phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_FRQSEL100;
			phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_FRQSEL50;
			break;
		case MHZ(100):
			phy_ctrl_5 |= TI_AM654_PHY_CTRL_5_FRQSEL100;
			phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_FRQSEL50;
			break;
		default:
			phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_FRQSEL100;
			phy_ctrl_5 |= TI_AM654_PHY_CTRL_5_FRQSEL50;
		}
	} else {
		uint8_t frqsel;

		if (clock <= MHZ(200) && clock > MHZ(170)) {
			frqsel = TI_AM654_PHY_CTRL_5_FRQSEL_VAL_200_170_MHZ;
		} else if (clock <= MHZ(170) && clock > MHZ(140)) {
			frqsel = TI_AM654_PHY_CTRL_5_FRQSEL_VAL_170_140_MHZ;
		} else if (clock <= MHZ(140) && clock > MHZ(110)) {
			frqsel = TI_AM654_PHY_CTRL_5_FRQSEL_VAL_140_110_MHZ;
		} else if (clock <= MHZ(110) && clock > MHZ(80)) {
			frqsel = TI_AM654_PHY_CTRL_5_FRQSEL_VAL_110_80_MHZ;
		} else {
			frqsel = TI_AM654_PHY_CTRL_5_FRQSEL_VAL_80_50_MHZ;
		}
		phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_FRQSEL;
		phy_ctrl_5 |= FIELD_PREP(TI_AM654_PHY_CTRL_5_FRQSEL, frqsel);
	}

	switch (config->drive_impedance) {
	case 33:
		impedance_val = TI_AM654_PHY_CTRL_1_DR_TY_VAL_33_OHMS;
		break;
	case 40:
		impedance_val = TI_AM654_PHY_CTRL_1_DR_TY_VAL_40_OHMS;
		break;
	case 50:
		impedance_val = TI_AM654_PHY_CTRL_1_DR_TY_VAL_50_OHMS;
		break;
	case 66:
		impedance_val = TI_AM654_PHY_CTRL_1_DR_TY_VAL_66_OHMS;
		break;
	case 100:
		impedance_val = TI_AM654_PHY_CTRL_1_DR_TY_VAL_100_OHMS;
		break;
	default:
		LOG_ERR("invalid impedance");
		return -EINVAL;
	}

	/* write phy_ctrl5 */
	ss_regs->PHY_CTRL_5 = phy_ctrl_5;

	/* read phy_ctrl1 */
	phy_ctrl_1 = ss_regs->PHY_CTRL_1;

	/* modify phy_ctrl1 */
	phy_ctrl_1 &= ~(TI_AM654_PHY_CTRL_1_DR_TY | TI_AM654_PHY_CTRL_1_DLL_TRM_ICP);
	phy_ctrl_1 |= FIELD_PREP(TI_AM654_PHY_CTRL_1_DR_TY, impedance_val) |
		      FIELD_PREP(TI_AM654_PHY_CTRL_1_DLL_TRM_ICP, config->current_trim) |
		      TI_AM654_PHY_CTRL_1_ENDLL;

	/* write phy_ctrl1 */
	ss_regs->PHY_CTRL_1 = phy_ctrl_1;

	/* poll ready state */
	retries = TI_AM654_REG_POLL_RETRIES;
	while ((ss_regs->PHY_STAT_1 & TI_AM654_PHY_STAT_1_DLLRDY) == 0) {
		if (retries-- == 0) {
			LOG_ERR("Timed out while waiting for DLL to be ready");
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static void ti_am654_configure_delay_chain(const struct device *dev, enum sdhc_timing_mode mode)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	struct ti_am654_tap_delay_config delay_config = data->delay_config[mode];
	uint32_t phy_ctrl_5;

	/* read */
	phy_ctrl_5 = ss_regs->PHY_CTRL_5;

	/* modify */
	if (delay_config.itap_delay_enable) {
		phy_ctrl_5 |= TI_AM654_PHY_CTRL_5_SETDLYRXCLK;
	} else {
		phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_SETDLYRXCLK;
	}

	if (delay_config.otap_delay_enable) {
		phy_ctrl_5 |= TI_AM654_PHY_CTRL_5_SETDLYTXCLK;
	} else {
		phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_SETDLYTXCLK;
	}

	/* write */
	ss_regs->PHY_CTRL_5 = phy_ctrl_5;
}

static void ti_am654_configure_tap_delays(const struct device *dev,
					  const struct ti_am654_tap_delay_config *delay_config)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	uint32_t phy_ctrl_4;

	/* read phy_ctrl4 */
	phy_ctrl_4 = ss_regs->PHY_CTRL_4;

	/* modify phy_ctrl4 */
	phy_ctrl_4 &= ~(TI_AM654_PHY_CTRL_4_ITAPDLYENA | TI_AM654_PHY_CTRL_4_ITAPDLYSEL |
			TI_AM654_PHY_CTRL_4_OTAPDLYENA | TI_AM654_PHY_CTRL_4_OTAPDLYSEL);
	phy_ctrl_4 |= FIELD_PREP(TI_AM654_PHY_CTRL_4_ITAPDLYENA, delay_config->itap_delay_enable) |
		      FIELD_PREP(TI_AM654_PHY_CTRL_4_ITAPDLYSEL, delay_config->itap_delay_value) |
		      FIELD_PREP(TI_AM654_PHY_CTRL_4_OTAPDLYENA, delay_config->otap_delay_enable) |
		      FIELD_PREP(TI_AM654_PHY_CTRL_4_OTAPDLYSEL, delay_config->otap_delay_value);

	/* write phy_ctrl4 */
	ss_regs->PHY_CTRL_4 |= TI_AM654_PHY_CTRL_4_ITAPCHGWIN;
	ss_regs->PHY_CTRL_4 = phy_ctrl_4;
	ss_regs->PHY_CTRL_4 &= ~TI_AM654_PHY_CTRL_4_ITAPCHGWIN;
}

static int ti_am654_configure_timing_has_dll(const struct device *dev, enum sdhc_timing_mode mode,
					     enum sdhc_clock_speed clock)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	const struct ti_am654_config *config = DEV_CFG(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t phy_ctrl_4;
	uint32_t phy_ctrl_5;
	int rv;

	/* configure itap and otap delay */
	ti_am654_configure_tap_delays(dev, &data->delay_config[mode]);

	/* read phy_ctrl4 and phy_ctrl5 */
	phy_ctrl_4 = ss_regs->PHY_CTRL_4;
	phy_ctrl_5 = ss_regs->PHY_CTRL_5;

	/* modify phy_ctrl4 */
	if (mode == SDHC_TIMING_HS400) {
		uint32_t strobe_sel_mask;

		if (config->strobe_sel_4_bit) {
			strobe_sel_mask = TI_AM654_PHY_CTRL_4_STRBSEL_4BIT;
		} else {
			strobe_sel_mask = TI_AM654_PHY_CTRL_4_STRBSEL;
		}

		phy_ctrl_4 &= ~strobe_sel_mask;
		phy_ctrl_4 |= FIELD_PREP(strobe_sel_mask, config->strobe_sel);
	}

	/* modify phy_ctrl5 */
	phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_CLKBUFSEL;
	phy_ctrl_5 |= FIELD_PREP(TI_AM654_PHY_CTRL_5_CLKBUFSEL, config->clkbuf_sel);

	/* write phy_ctrl4 and phy_ctrl5 */
	ss_regs->PHY_CTRL_4 = phy_ctrl_4;
	ss_regs->PHY_CTRL_5 = phy_ctrl_5;

	switch (mode) {
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_HS:
	case SDHC_TIMING_SDR12:
	case SDHC_TIMING_SDR25: {
		ti_am654_configure_delay_chain(dev, mode);
		break;
	}
	case SDHC_TIMING_SDR50:
	case SDHC_TIMING_SDR104:
	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
	case SDHC_TIMING_HS200:
	case SDHC_TIMING_HS400: {
		rv = ti_am654_configure_delay_locked_loop(dev, mode, clock);
		if (rv != 0) {
			return rv;
		}
		break;
	}
	default: {
		LOG_ERR("invalid tuning mode");
		return -EINVAL;
	}
	}

	return 0;
}

static int ti_am654_configure_timing_non_dll(const struct device *dev, enum sdhc_timing_mode mode)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	const struct ti_am654_config *config = DEV_CFG(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	uint32_t phy_ctrl_5;

	/* configure itap and otap delay */
	ti_am654_configure_tap_delays(dev, &data->delay_config[mode]);

	/* read phy_ctrl5 */
	phy_ctrl_5 = ss_regs->PHY_CTRL_5;

	/* modify phy_ctrl5 */
	phy_ctrl_5 &= ~TI_AM654_PHY_CTRL_5_CLKBUFSEL;
	phy_ctrl_5 |= FIELD_PREP(TI_AM654_PHY_CTRL_5_CLKBUFSEL, config->clkbuf_sel);

	/* write phy_ctrl5 */
	ss_regs->PHY_CTRL_5 = phy_ctrl_5;

	return 0;
}

static int ti_am654_configure_timing(const struct device *dev, enum sdhc_timing_mode mode,
				     enum sdhc_clock_speed clock)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	uint8_t uhs_mode = 0;

	if (clock == 0) {
		return 0;
	}

	switch (mode) {
	case SDHC_TIMING_LEGACY:
	case SDHC_TIMING_HS:
		break;
	case SDHC_TIMING_SDR12:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR12;
		break;
	case SDHC_TIMING_SDR25:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR25;
		break;
	case SDHC_TIMING_SDR50:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR50;
		break;
	case SDHC_TIMING_HS200:
	case SDHC_TIMING_SDR104:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_SDR104;
		break;
	case SDHC_TIMING_DDR50:
	case SDHC_TIMING_DDR52:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_DDR50;
		break;
	case SDHC_TIMING_HS400:
		uhs_mode = TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT_VAL_HS400;
		break;
	default:
		LOG_ERR("invalid tuning mode");
		return -EINVAL;
	}

	hc_regs->HOST_CONTROL2 &= ~TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT;
	if (mode >= SDHC_TIMING_SDR12) {
		hc_regs->HOST_CONTROL2 |=
			FIELD_PREP(TI_AM654_HOST_CONTROL2_UHS_MODE_SELECT, uhs_mode);
		hc_regs->HOST_CONTROL1 |= TI_AM654_HOST_CONTROL1_HIGH_SPEED_ENA;
	} else {
		hc_regs->HOST_CONTROL1 &= ~TI_AM654_HOST_CONTROL1_HIGH_SPEED_ENA;
	}

	/* configure timing mode */
	if (DEV_CFG(dev)->dll_present) {
		return ti_am654_configure_timing_has_dll(dev, mode, clock);
	}
	return ti_am654_configure_timing_non_dll(dev, mode);
}

static void ti_am654_configure_bus_width(const struct device *dev, enum sdhc_bus_width width)
{
	uint8_t host_control1 = DEV_HC_REGS(dev)->HOST_CONTROL1;

	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		host_control1 &= ~TI_AM654_HOST_CONTROL1_EXT_DATA_WIDTH;
		host_control1 &= ~TI_AM654_HOST_CONTROL1_DATA_WIDTH;
		break;
	case SDHC_BUS_WIDTH4BIT:
		host_control1 &= ~TI_AM654_HOST_CONTROL1_EXT_DATA_WIDTH;
		host_control1 |= TI_AM654_HOST_CONTROL1_DATA_WIDTH;
		break;
	case SDHC_BUS_WIDTH8BIT:
		host_control1 |= TI_AM654_HOST_CONTROL1_EXT_DATA_WIDTH;
		break;
	default:
		LOG_ERR("invalid bus width");
	}

	DEV_HC_REGS(dev)->HOST_CONTROL1 = host_control1;
}

static int ti_am654_configure_voltage(const struct device *dev, enum sd_voltage voltage,
				      enum sdhc_timing_mode mode)
{
	const struct ti_am654_config *config = DEV_CFG(dev);
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	uint8_t power_control = hc_regs->POWER_CONTROL;
	uint16_t host_control2 = hc_regs->HOST_CONTROL2;
	uint32_t uV = 0;
	int rv;

	power_control &= ~TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE;

	switch (voltage) {
	case SD_VOL_1_8_V: {
		uV = 1800000;

		power_control |= FIELD_PREP(TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE,
					    TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V1P8);
		host_control2 |= TI_AM654_HOST_CONTROL2_V1P8_SIGNAL_ENA;
		break;
	}
	case SD_VOL_3_0_V: {
		uV = 3000000;

		power_control |= FIELD_PREP(TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE,
					    TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V3P0);
		host_control2 &= ~TI_AM654_HOST_CONTROL2_V1P8_SIGNAL_ENA;
		break;
	}
	case SD_VOL_3_3_V: {
		uV = 3300000;

		power_control |= FIELD_PREP(TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE,
					    TI_AM654_POWER_CONTROL_SD_BUS_VOLTAGE_VAL_V3P3);
		host_control2 &= ~TI_AM654_HOST_CONTROL2_V1P8_SIGNAL_ENA;
		break;
	}
	case SD_VOL_1_2_V: {
		LOG_ERR("1.2V not supported");
		return -ENOTSUP;
	}
	default: {
		LOG_ERR("invalid bus voltage");
		return -EINVAL;
	}
	}

	if (config->vqmmc != NULL && regulator_is_supported_voltage(config->vqmmc, uV, uV)) {
		rv = regulator_set_voltage(config->vqmmc, uV, uV);
		if (rv != 0) {
			LOG_ERR("failed to change regulator voltage");
			return rv;
		}
	}

	if (!config->is_embedded || mode >= SDHC_TIMING_SDR12) {
		hc_regs->HOST_CONTROL2 = host_control2;
		/* wait for signal */
		k_msleep(5);
	}

	hc_regs->POWER_CONTROL = power_control;

	return 0;
}

static int ti_am654_configure_power(const struct device *dev, enum sdhc_power power_mode)
{
	const struct ti_am654_config *config = DEV_CFG(dev);
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	int rv;

	switch (power_mode) {
	case SDHC_POWER_ON: {
		if (config->vmmc != NULL) {
			rv = regulator_enable(config->vmmc);
			if (rv != 0) {
				LOG_ERR("Failed to enable regulator");
				return rv;
			}
		}

		/* enable power */
		hc_regs->POWER_CONTROL |= TI_AM654_POWER_CONTROL_SD_BUS_POWER;

		break;
	}

	case SDHC_POWER_OFF: {
		if (config->vmmc != NULL) {
			rv = regulator_disable(config->vmmc);
			if (rv != 0) {
				LOG_ERR("Failed to disable regulator");
				return rv;
			}
		}

		/* disable power */
		hc_regs->POWER_CONTROL &= ~TI_AM654_POWER_CONTROL_SD_BUS_POWER;

		break;
	}
	default: {
		LOG_ERR("invalid power mode");
		return -EINVAL;
	}
	}

	return 0;
}

static int ti_am654_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	int rv;

	LOG_DBG("SDHC I/O: bus width %u, clk %uHz, power %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	if (ios->clock < data->props.f_min || ios->clock > data->props.f_max) {
		LOG_ERR("Invalid clock frequency: %uHz", ios->clock);
		return -EINVAL;
	}

	if (ios->bus_width == SDHC_BUS_WIDTH8BIT &&
	    data->props.host_caps.bus_8_bit_support == false) {
		LOG_ERR("Bus width not supported");
		return -ENOTSUP;
	}

	if (ios->bus_mode == SDHC_BUSMODE_OPENDRAIN) {
		LOG_ERR("Open drain is not supported");
		return -ENOTSUP;
	}

	/* configure bus width */
	if (ios->bus_width != data->ios.bus_width) {
		ti_am654_configure_bus_width(dev, ios->bus_width);
	}

	/* configure voltage */
	if (ios->signal_voltage != data->ios.signal_voltage || ios->timing != data->ios.timing) {
		rv = ti_am654_configure_voltage(dev, ios->signal_voltage, ios->timing);
		if (rv != 0) {
			return rv;
		}
	}

	/* set clock */
	if (ios->clock != data->ios.clock || ios->clock == 0) {
		rv = ti_am654_configure_clock(dev, ios->clock);
		if (rv != 0) {
			return rv;
		}
	}

	/* configure timing */
	if (ios->timing != data->ios.timing || ios->clock != data->ios.clock) {
		rv = ti_am654_configure_timing(dev, ios->timing, ios->clock);
		if (rv != 0) {
			return rv;
		}
	}

	/* configure power */
	if (ios->power_mode != data->ios.power_mode) {
		rv = ti_am654_configure_power(dev, ios->power_mode);
		if (rv != 0) {
			return rv;
		}
	}

	/* save */
	data->ios = *ios;

	return 0;
}

static int ti_am654_send_tuning_data(const struct device *dev)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	const uint8_t *expected_buf = NULL;
	struct sdhc_command cmd;
	struct sdhc_data dat;
	uint8_t rd_buf[sizeof(ti_am654_tuning_blk_8_bit)] = {0};
	int rv = 0;

	if (DEV_CFG(dev)->is_embedded) {
		cmd.opcode = MMC_SEND_TUNING_BLOCK;
	} else {
		cmd.opcode = SD_SEND_TUNING_BLOCK;
	}
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	cmd.retries = 0;
	cmd.arg = 0;

	if (data->ios.bus_width == SDHC_BUS_WIDTH8BIT) {
		expected_buf = ti_am654_tuning_blk_8_bit;
		dat.block_size = sizeof(ti_am654_tuning_blk_8_bit);
	} else {
		expected_buf = ti_am654_tuning_blk_4_bit;
		dat.block_size = sizeof(ti_am654_tuning_blk_4_bit);
	}

	dat.data = rd_buf;
	dat.timeout_ms = CONFIG_SD_DATA_TIMEOUT;
	dat.blocks = 1;

	rv = ti_am654_request(dev, &cmd, &dat);

	if (rv != 0) {
		return rv;
	}

	return memcmp(expected_buf, dat.data, dat.block_size);
}

static int ti_am654_calculate_itap(struct ti_am654_tuning_window *fail_window, uint8_t num_fails)
{
	struct ti_am654_tuning_window pass_window = {0, 0, 0};
	uint8_t start_fail = 0;
	uint8_t end_fail = 0;
	uint8_t pass_length = 0;
	int prev_fail_end = -1;

	if (num_fails == 0) {
		LOG_ERR("no failing region found, retry tuning");
		return -EIO;
	}

	if (fail_window->length == TI_AM654_PHY_CTRL_4_ITAPDLYSEL_VAL_MAX + 1) {
		LOG_ERR("no passing itapdly found, retry tuning");
		return -EIO;
	}

	for (int i = 0; i < num_fails; i++) {
		start_fail = fail_window[i].start;
		end_fail = fail_window[i].end;
		pass_length = start_fail - (prev_fail_end + 1);

		if (pass_length > pass_window.length) {
			pass_window.start = prev_fail_end + 1;
			pass_window.length = pass_length;
		}
		prev_fail_end = end_fail;
	}

	return (pass_window.start + (pass_window.length >> 1)) %
	       (TI_AM654_PHY_CTRL_4_ITAPDLYSEL_VAL_MAX + 1);
}

static int ti_am654_execute_manual_tuning(const struct device *dev)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	enum sdhc_timing_mode timing = data->ios.timing;
	struct ti_am654_tap_delay_config delay_config = data->delay_config[timing];
	struct ti_am654_tuning_window fail_window[TI_AM654_PHY_CTRL_4_ITAPDLYSEL_VAL_MAX + 1];
	uint8_t fail_idx = 0;
	int currPass = 0;
	int prevPass = 1;

	memset(fail_window, 0, sizeof(fail_window));

	/* Try different itap values */
	delay_config.itap_delay_enable = true;
	for (int itap = 0; itap <= TI_AM654_PHY_CTRL_4_ITAPDLYSEL_VAL_MAX; itap++) {
		delay_config.itap_delay_value = itap;

		/* configure itap value */
		ti_am654_configure_tap_delays(dev, &delay_config);

		/* send tuning block */
		currPass = !ti_am654_send_tuning_data(dev);

		if (!currPass && prevPass) {
			fail_window[fail_idx].start = itap;
		}

		if (!currPass) {
			fail_window[fail_idx].end = itap;
			fail_window[fail_idx].length++;
		}

		if (currPass && !prevPass) {
			fail_idx++;
		}

		prevPass = currPass;
	}

	if (fail_window[fail_idx].length != 0U) {
		fail_idx++;
	}

	return ti_am654_calculate_itap(fail_window, fail_idx);
}

static int ti_am654_execute_tuning(const struct device *dev)
{
	struct ti_am654_data *data = DEV_DATA(dev);
	enum sdhc_timing_mode timing = data->ios.timing;
	struct ti_am654_tap_delay_config *delay_config = &data->delay_config[timing];
	int rv;

	switch (timing) {
	case SDHC_TIMING_SDR104:
	case SDHC_TIMING_HS200:
		break;
	case SDHC_TIMING_SDR50:
		if (data->props.host_caps.sdr50_needs_tuning) {
			break;
		}
	default:
		LOG_ERR("invalid timing mode for tuning");
		return -ENOTSUP;
	}

	for (int i = 0; i < TI_AM654_TUNING_RETRIES; i++) {
		rv = ti_am654_execute_manual_tuning(dev);

		if (rv >= 0) {
			delay_config->itap_delay_enable = true;
			delay_config->itap_delay_value = rv;

			ti_am654_configure_tap_delays(dev, delay_config);

			LOG_DBG("tuned with itap: %u", rv);

			return 0;
		}
	}

	return rv;
}

static int ti_am654_get_card_present(const struct device *dev)
{
	return !!(DEV_HC_REGS(dev)->PRESENTSTATE & TI_AM654_PRESENTSTATE_CARD_INSERTED);
}

static int ti_am654_card_busy(const struct device *dev)
{
	uint32_t presentstate = DEV_HC_REGS(dev)->PRESENTSTATE;
	uint32_t lines = TI_AM654_PRESENTSTATE_SDIF_DAT0IN | TI_AM654_PRESENTSTATE_SDIF_DAT1IN |
			 TI_AM654_PRESENTSTATE_SDIF_DAT2IN | TI_AM654_PRESENTSTATE_SDIF_DAT3IN;

	if (DEV_CFG(dev)->is_embedded) {
		lines |= TI_AM654_PRESENTSTATE_SDIF_DAT4IN | TI_AM654_PRESENTSTATE_SDIF_DAT5IN |
			 TI_AM654_PRESENTSTATE_SDIF_DAT6IN | TI_AM654_PRESENTSTATE_SDIF_DAT7IN;
	}

	return !(presentstate & lines);
}

static int ti_am654_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
				     int sources, void *user_data)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);

	data->callback = callback;
	data->user_data = user_data;

	if (sources & SDHC_INT_SDIO) {
		return -ENOTSUP;
	}

	if (sources & SDHC_INT_INSERTED) {
		hc_regs->NORMAL_INTR_SIG_ENA |= TI_AM654_NORMAL_INTR_CARD_INSERTION;
		hc_regs->NORMAL_INTR_STS_ENA |= TI_AM654_NORMAL_INTR_CARD_INSERTION;
	}

	if (sources & SDHC_INT_REMOVED) {
		hc_regs->NORMAL_INTR_SIG_ENA |= TI_AM654_NORMAL_INTR_CARD_REMOVAL;
		hc_regs->NORMAL_INTR_STS_ENA |= TI_AM654_NORMAL_INTR_CARD_REMOVAL;
	}

	return 0;
}

static int ti_am654_disable_interrupt(const struct device *dev, int sources)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);

	if (sources & SDHC_INT_SDIO) {
		return -ENOTSUP;
	}

	if (sources & SDHC_INT_INSERTED) {
		hc_regs->NORMAL_INTR_SIG_ENA &= ~TI_AM654_NORMAL_INTR_CARD_INSERTION;
		hc_regs->NORMAL_INTR_STS_ENA &= ~TI_AM654_NORMAL_INTR_CARD_INSERTION;
	}

	if (sources & SDHC_INT_REMOVED) {
		hc_regs->NORMAL_INTR_SIG_ENA &= ~TI_AM654_NORMAL_INTR_CARD_REMOVAL;
		hc_regs->NORMAL_INTR_STS_ENA &= ~TI_AM654_NORMAL_INTR_CARD_REMOVAL;
	}

	return 0;
}

static void ti_am654_init_host_props(const struct device *dev)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = dev->data;
	struct sdhc_host_props *props = &data->props;
	uint64_t max_current_caps = hc_regs->MAX_CURRENT_CAP;
	uint64_t caps = hc_regs->CAPABILITIES;

	/* max current */
	props->max_current_180 = FIELD_GET(TI_AM654_MAX_CURRENT_CAP_VDD1_1P8V, max_current_caps)
				 << 2;
	props->max_current_300 = FIELD_GET(TI_AM654_MAX_CURRENT_CAP_VDD1_3P0V, max_current_caps)
				 << 2;
	props->max_current_330 = FIELD_GET(TI_AM654_MAX_CURRENT_CAP_VDD1_3P3V, max_current_caps)
				 << 2;

	/* copy capabilities to bitfield struct */
	BUILD_ASSERT(sizeof(props->host_caps) == sizeof(caps),
		     "SDHCI host capabilities do not fit the register");
	props->host_caps = *(const struct sdhc_host_caps *)(&caps);

	/* extra capabilities */
	if (!(caps & TI_AM654_CAPABILITIES_BUS_HS400_SUPPORT)) {
		props->hs400_support = false;
	}
	props->bus_4_bit_support = true;
}

static int ti_am654_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct ti_am654_data *data = dev->data;

	*props = data->props;

	return 0;
}

static int ti_am654_phy_calib(const struct device *dev)
{
	struct ti_am654_ss_regs *ss_regs = DEV_SS_REGS(dev);
	int retries = TI_AM654_REG_POLL_RETRIES;

	ss_regs->PHY_CTRL_1 |= TI_AM654_PHY_CTRL_1_EN_RTRIM;

	while ((ss_regs->PHY_CTRL_1 & TI_AM654_PHY_CTRL_1_EN_RTRIM) == 0) {
		if (retries-- == 0) {
			LOG_ERR("Timed out while waiting for rtrim enable");
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	retries = TI_AM654_REG_POLL_RETRIES;
	ss_regs->PHY_CTRL_1 |= TI_AM654_PHY_CTRL_1_PDB;

	while ((ss_regs->PHY_STAT_1 & TI_AM654_PHY_STAT_1_CALDONE) == 0) {
		if (retries-- == 0) {
			LOG_ERR("Timed out while waiting for calibration");
			return -ETIMEDOUT;
		}
		k_usleep(TI_AM654_REG_POLL_TIME_BETWEEN_RETRIES_US);
	}

	return 0;
}

static int ti_am654_init(const struct device *dev)
{
	const struct ti_am654_config *config = DEV_CFG(dev);
	struct ti_am654_data *data = DEV_DATA(dev);
	struct ti_am654_hc_regs *hc_regs;
	struct ti_am654_ss_regs *ss_regs;
	uint16_t normal_intr;
	uint16_t error_intr;
	uint32_t ctl_cfg_2;
	int rv;

	DEVICE_MMIO_NAMED_MAP(dev, host, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, subsys, K_MEM_CACHE_NONE);

	hc_regs = DEV_HC_REGS(dev);
	ss_regs = DEV_SS_REGS(dev);

	config->irq_func(dev);
	k_event_init(&data->irq_event);

	rv = ti_am654_reset_all(dev);
	if (rv != 0) {
		LOG_ERR("failed to reset the controller");
		return rv;
	}

	ti_am654_init_host_props(dev);

	if (config->dll_present) {
		rv = ti_am654_phy_calib(dev);
		if (rv != 0) {
			LOG_ERR("failed to calibrate");
			return rv;
		}
	} else {
		ss_regs->PHY_CTRL_1 &= ~TI_AM654_PHY_CTRL_1_IOMUX_ENABLE;

		rv = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
		if (rv < 0) {
			LOG_ERR("failed to apply pinctrl");
			return rv;
		}
	}

	/* set slot type */
	ctl_cfg_2 = ss_regs->CTL_CFG_2;
	ctl_cfg_2 &= ~TI_AM654_CTL_CFG_2_SLOTTYPE;
	ctl_cfg_2 |= FIELD_PREP(TI_AM654_CTL_CFG_2_SLOTTYPE, data->props.host_caps.slot_type);
	ss_regs->CTL_CFG_2 = ctl_cfg_2;

	/* enable version 4 */
	hc_regs->HOST_CONTROL2 |= TI_AM654_HOST_CONTROL2_HOST_VER40_ENA;

	/* force card detect if required */
	if (config->fails_without_test_cd) {
		hc_regs->HOST_CONTROL1 |=
			(TI_AM654_HOST_CONTROL1_CD_TEST_LEVEL | TI_AM654_HOST_CONTROL1_CD_SIG_SEL);
	}

#ifdef CONFIG_SDHC_TI_AM654_ENABLE_ADMA
	hc_regs->HOST_CONTROL1 &= ~TI_AM654_HOST_CONTROL1_DMA_SELECT;
	hc_regs->HOST_CONTROL1 |= FIELD_PREP(TI_AM654_HOST_CONTROL1_DMA_SELECT,
					     TI_AM654_HOST_CONTROL1_DMA_SELECT_VAL_ADMA2);

	/* 64 bit addressing and 26 bit length mode */
	hc_regs->HOST_CONTROL2 |=
		(TI_AM654_HOST_CONTROL2_ADMA2_LEN_MODE | TI_AM654_HOST_CONTROL2_BIT64_ADDRESSING);
#endif /* CONFIG_SDHC_TI_AM654_ENABLE_ADMA */

	/* enable interrupts */
	normal_intr = TI_AM654_NORMAL_INTR_CMD_COMPLETE | TI_AM654_NORMAL_INTR_XFER_COMPLETE |
		      TI_AM654_NORMAL_INTR_BUF_RD_READY | TI_AM654_NORMAL_INTR_BUF_WR_READY;
	error_intr = TI_AM654_ERROR_INTR_ALL;

	hc_regs->NORMAL_INTR_SIG_ENA |= normal_intr;
	hc_regs->NORMAL_INTR_STS_ENA |= normal_intr;
	hc_regs->ERROR_INTR_SIG_ENA |= error_intr;
	hc_regs->ERROR_INTR_STS_ENA |= error_intr;

	return 0;
}

static void ti_am654_isr(const struct device *dev)
{
	struct ti_am654_hc_regs *hc_regs = DEV_HC_REGS(dev);
	struct ti_am654_data *data = DEV_DATA(dev);

	uint16_t nstatus = hc_regs->NORMAL_INTR_STS;
	uint16_t estatus = hc_regs->ERROR_INTR_STS;

	if (estatus) {
		hc_regs->ERROR_INTR_STS |= estatus;
		k_event_post(&data->irq_event, TI_AM654_K_EVENT_ERRORS(estatus));
	}

	if (nstatus & TI_AM654_NORMAL_INTR_CMD_COMPLETE) {
		hc_regs->NORMAL_INTR_STS = TI_AM654_NORMAL_INTR_CMD_COMPLETE;
		k_event_post(&data->irq_event, TI_AM654_NORMAL_INTR_CMD_COMPLETE);
	}

	if (nstatus & TI_AM654_NORMAL_INTR_XFER_COMPLETE) {
		hc_regs->NORMAL_INTR_STS = TI_AM654_NORMAL_INTR_XFER_COMPLETE;
		k_event_post(&data->irq_event, TI_AM654_NORMAL_INTR_XFER_COMPLETE);
	}

	if (nstatus & TI_AM654_NORMAL_INTR_BUF_WR_READY) {
		hc_regs->NORMAL_INTR_STS = TI_AM654_NORMAL_INTR_BUF_WR_READY;
		k_event_post(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_WR_READY);
	}

	if (nstatus & TI_AM654_NORMAL_INTR_BUF_RD_READY) {
		hc_regs->NORMAL_INTR_STS = TI_AM654_NORMAL_INTR_BUF_RD_READY;
		k_event_post(&data->irq_event, TI_AM654_NORMAL_INTR_BUF_RD_READY);
	}

	if (nstatus & TI_AM654_NORMAL_INTR_CARD_INSERTION) {
		hc_regs->NORMAL_INTR_STS |= TI_AM654_NORMAL_INTR_CARD_INSERTION;

		if (ti_am654_get_card_present(dev)) {
			data->callback(dev, SDHC_INT_INSERTED, data->user_data);
		}
	}

	if (nstatus & TI_AM654_NORMAL_INTR_CARD_REMOVAL) {
		hc_regs->NORMAL_INTR_STS |= TI_AM654_NORMAL_INTR_CARD_REMOVAL;

		if (!ti_am654_get_card_present(dev)) {
			data->callback(dev, SDHC_INT_REMOVED, data->user_data);
		}
	}
}

static DEVICE_API(sdhc, ti_am654_api) = {
	.reset = ti_am654_reset_all,
	.request = ti_am654_request,
	.set_io = ti_am654_set_io,
	.enable_interrupt = ti_am654_enable_interrupt,
	.disable_interrupt = ti_am654_disable_interrupt,
	.get_card_present = ti_am654_get_card_present,
	.execute_tuning = ti_am654_execute_tuning,
	.card_busy = ti_am654_card_busy,
	.get_host_props = ti_am654_get_host_props,
};

#define TI_AM654_TIMING_DELAY(n, timing)                                                           \
	{                                                                                          \
		.itap_delay_enable = DT_INST_NODE_HAS_PROP(n, ti_itap_del_sel_##timing),           \
		.itap_delay_value = DT_INST_PROP_OR(n, ti_itap_del_sel_##timing, 0),               \
		.otap_delay_enable = DT_INST_NODE_HAS_PROP(n, ti_otap_del_sel_##timing),           \
		.otap_delay_value = DT_INST_PROP_OR(n, ti_otap_del_sel_##timing, 0),               \
	}

#define TI_AM654_TIMING_DELAY_LIST(n)                                                              \
	{                                                                                          \
		[SDHC_TIMING_LEGACY] = TI_AM654_TIMING_DELAY(n, legacy),                           \
		[SDHC_TIMING_HS] = TI_AM654_TIMING_DELAY(n, hs),                                   \
		[SDHC_TIMING_SDR12] = TI_AM654_TIMING_DELAY(n, sdr12),                             \
		[SDHC_TIMING_SDR25] = TI_AM654_TIMING_DELAY(n, sdr25),                             \
		[SDHC_TIMING_SDR50] = TI_AM654_TIMING_DELAY(n, sdr50),                             \
		[SDHC_TIMING_SDR104] = TI_AM654_TIMING_DELAY(n, sdr104),                           \
		[SDHC_TIMING_DDR50] = TI_AM654_TIMING_DELAY(n, ddr50),                             \
		[SDHC_TIMING_DDR52] = TI_AM654_TIMING_DELAY(n, ddr52),                             \
		[SDHC_TIMING_HS200] = TI_AM654_TIMING_DELAY(n, hs200),                             \
		[SDHC_TIMING_HS400] = TI_AM654_TIMING_DELAY(n, hs400),                             \
	}

#define TI_AM654_INIT(n)                                                                           \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void ti_am654_##n##_irq_func(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ti_am654_isr,               \
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(0, flags));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ti_am654_config ti_am654_##n##_config = {                              \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(host, DT_DRV_INST(n)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(subsys, DT_DRV_INST(n)),                        \
		.is_embedded = DT_INST_PROP(n, ti_is_embedded),                                    \
		.dll_present = DT_INST_PROP(n, ti_dll_present),                                    \
		.fails_without_test_cd = DT_INST_PROP(n, ti_fails_without_test_cd),                \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.irq_func = ti_am654_##n##_irq_func,                                               \
		.clkbuf_sel = DT_INST_PROP(n, ti_clkbuf_sel),                                      \
		.strobe_sel = DT_INST_PROP_OR(n, ti_strobe_sel, 0),                                \
		.strobe_sel_4_bit = DT_INST_PROP(n, ti_strobe_sel_4_bit),                          \
		.dll_frqsel_2_bit = DT_INST_PROP(n, ti_dll_frqsel_2_bit),                          \
		.drive_impedance = DT_INST_PROP_OR(n, ti_driver_strength_ohm, 0),                  \
		.current_trim = DT_INST_PROP_OR(n, ti_trm_icp, 0),                                 \
		.vmmc = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(DT_DRV_INST(n), vmmc_supply)),            \
		.vqmmc = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(DT_DRV_INST(n), vqmmc_supply)),          \
	};                                                                                         \
                                                                                                   \
	static struct ti_am654_data ti_am654_##n##_data = {                                        \
		.delay_config = TI_AM654_TIMING_DELAY_LIST(n),                                     \
		.props =                                                                           \
			{                                                                          \
				.f_min = DT_INST_PROP(n, min_bus_freq),                            \
				.f_max = DT_INST_PROP(n, max_bus_freq),                            \
				.power_delay = DT_INST_PROP(n, power_delay_ms),                    \
				.hs200_support = DT_INST_PROP(n, mmc_hs200_1_8v),                  \
				.hs400_support = DT_INST_PROP(n, mmc_hs400_1_8v),                  \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ti_am654_init, NULL, &ti_am654_##n##_data,                       \
			      &ti_am654_##n##_config, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,      \
			      &ti_am654_api);

DT_INST_FOREACH_STATUS_OKAY(TI_AM654_INIT)

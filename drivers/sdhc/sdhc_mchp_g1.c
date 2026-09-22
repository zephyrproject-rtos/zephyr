/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sdhc_g1

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

LOG_MODULE_REGISTER(sdhc_mchp_g1, CONFIG_SDHC_LOG_LEVEL);

#define SDHC_REG(dev) ((sdhc_registers_t *)((const struct sdhc_mchp_cfg *)(dev)->config)->regs)

#define SDHC_DESC_TABLE_ATTR_XFER_DATA (0x02U << 4U)
#define SDHC_DESC_TABLE_ATTR_VALID     (1U << 0U)
#define SDHC_DESC_TABLE_ATTR_END       (1UL << 1U)
#define SDHC_DESC_TABLE_ATTR_INTR      (1U << 2U)

#define SDHC_RESET_TIMEOUT_US        1000
#define SDHC_CLOCK_STABLE_TIMEOUT_US 1000
#define SDHC_CMD_IDLE_TIMEOUT_US     1000
#define SDHC_XFER_DONE_TIMEOUT_US    100000
#define SDHC_CMD_DEFAULT_TIMEOUT_MS  200

#define SDHC_ADMA2_DESC_MAX_BYTES 65536U

#define SDHC_DATA_TIMEOUT_CNT_VAL 0xEU

#define SDHC_DMASEL_ADMA2 2U

#define SDHC_CCR_SDCLKFSEL_MASK   0xFFU
#define SDHC_CCR_USDCLKFSEL_SHIFT 8U

#define SDHC_XFER_STATUS_CMD_COMPLETED  (0x01U)
#define SDHC_XFER_STATUS_DATA_COMPLETED (0x02U)
#define SDHC_XFER_STATUS_CARD_INSERTED  (0x04U)
#define SDHC_XFER_STATUS_CARD_REMOVED   (0x08U)

#define CACHE_LINE_SIZE (16U)

#define SDHC1_DMA_NUM_DESCR_LINES  (8U)
#define SDHC1_BASE_CLOCK_FREQUENCY (120000000U)
#define SDHC1_MAX_BLOCK_SIZE       (0x200U)
#define SDHC1_DMA_DESC_TABLE_SIZE  (8U * SDHC1_DMA_NUM_DESCR_LINES)

#define SDHC_BASECLKF_DEFAULT_DIV (2U)
#define SDHC_BASECLKF_HZ_PER_MHZ  (1000000U)

#define SDHC1_DMA_DESC_TABLE_SIZE_CACHE_ALIGN                                                      \
	(SDHC1_DMA_DESC_TABLE_SIZE +                                                               \
	 ((SDHC1_DMA_DESC_TABLE_SIZE % CACHE_LINE_SIZE)                                            \
		  ? (CACHE_LINE_SIZE - (SDHC1_DMA_DESC_TABLE_SIZE % CACHE_LINE_SIZE))              \
		  : 0U))

enum sdhc_speed_mode {
	SDHC_SPEED_MODE_NORMAL = 0,
	SDHC_SPEED_MODE_HIGH
};

enum sdhc_read_response_reg {
	SDHC_READ_RESP_REG_0 = 0,
	SDHC_READ_RESP_REG_1,
	SDHC_READ_RESP_REG_2,
	SDHC_READ_RESP_REG_3,
	SDHC_READ_RESP_REG_ALL
};

enum sdhc_reset_type {
	SDHC_RESET_ALL = 0x01,
	SDHC_RESET_CMD = 0x02,
	SDHC_RESET_DAT = 0x04
};

enum sdhc_clk_mode {
	SDHC_DIVIDED_CLK_MODE = 0,
	SDHC_PROGRAMMABLE_CLK_MODE
};

enum sdhc_data_transfer_type {
	SDHC_DATA_TRANSFER_TYPE_SINGLE = 0,
	SDHC_DATA_TRANSFER_TYPE_MULTI,
	SDHC_DATA_TRANSFER_MMC_STREAM,
	SDHC_DATA_TRANSFER_SDIO_BYTE,
	SDHC_DATA_TRANSFER_SDIO_BLOCK,
};

enum sdhc_data_transfer_dir {
	SDHC_DATA_TRANSFER_DIR_WRITE = 0,
	SDHC_DATA_TRANSFER_DIR_READ
};

struct sdhc_data_transfer_flags {
	bool is_data_present;
	enum sdhc_data_transfer_dir transfer_dir;
	enum sdhc_data_transfer_type transfer_type;
};

struct sdhc_adma_descr {
	uint16_t attribute;
	uint16_t length;
	uint32_t address;
};

struct sdhc_object {
	bool is_cmd_in_progress;
	bool is_data_in_progress;
	uint16_t error_status;
};

struct sdhc_mchp_data {
	struct k_mutex bus_mutex;
	struct k_sem transfer_done;
	struct sdhc_host_props props;
	int sdhc_response[4];
	volatile struct sdhc_object sdhc_obj;
	struct sdhc_adma_descr *dma_desc_table;
};

struct mchp_sdhc_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t gclk_sys;
	clock_control_subsys_t gclk_slow_sys;
};

struct sdhc_mchp_cfg {
	const struct pinctrl_dev_config *pcfg;
	uintptr_t regs;

	struct mchp_sdhc_clock sdhc_clock;

	int power_delay_ms;
	uint32_t min_bus_freq;
	uint32_t max_bus_freq;
	void (*irq_config_func)(const struct device *dev);
};

static void sdhc_reset_error(const struct device *dev, enum sdhc_reset_type reset_type)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_SRR = (uint8_t)reset_type;

	if (!WAIT_FOR(((sdhc_regs->SDHC_SRR & (uint8_t)reset_type) == 0U), SDHC_RESET_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_WRN("SDHC reset timed out");
	}
}

static inline void sdhc_set_buswidth(const struct device *dev, enum sdhc_bus_width bus_width)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	if (bus_width == SDHC_BUS_WIDTH4BIT) {
		sdhc_regs->SDHC_HC1R |= SDHC_HC1R_DW_4BIT;
	} else {
		sdhc_regs->SDHC_HC1R &= (uint8_t)(~SDHC_HC1R_DW_4BIT);
	}
}

static inline void sdhc_set_speedmode(const struct device *dev, enum sdhc_speed_mode speed_mode)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	if (speed_mode == SDHC_SPEED_MODE_HIGH) {
		sdhc_regs->SDHC_HC1R |= SDHC_HC1R_HSEN_Msk;
	} else {
		sdhc_regs->SDHC_HC1R &= (uint8_t)(~SDHC_HC1R_HSEN_Msk);
	}
}

static inline bool sdhc_is_cmdline_busy(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return (((sdhc_regs->SDHC_PSR & SDHC_PSR_CMDINHC_Msk) == SDHC_PSR_CMDINHC_Msk) ? true
										       : false);
}

static inline bool sdhc_is_dataline_busy(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return (((sdhc_regs->SDHC_PSR & SDHC_PSR_CMDINHD_Msk) == SDHC_PSR_CMDINHD_Msk) ? true
										       : false);
}

static inline bool sdhc_is_card_attached(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	return ((sdhc_regs->SDHC_PSR & SDHC_PSR_CARDDPL_Msk) == SDHC_PSR_CARDDPL_Msk) ? true
										      : false;
}

static void sdhc_set_blocksize(const struct device *dev, uint16_t block_size)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	if (block_size == 0U) {
		block_size = 1U;
	} else if (block_size > SDHC1_MAX_BLOCK_SIZE) {
		block_size = SDHC1_MAX_BLOCK_SIZE;
	} else {
		/* block_size is within the valid range; use as-is */
	}

	sdhc_regs->SDHC_BSR = (SDHC_BSR_BOUNDARY_4K | SDHC_BSR_BLOCKSIZE(block_size));
}

static inline void sdhc_set_blockcount(const struct device *dev, uint16_t num_blocks)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_BCR = num_blocks;
}

static void sdhc_clock_enable(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_CCR |= SDHC_CCR_INTCLKEN_Msk;

	if (!WAIT_FOR(((sdhc_regs->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) != 0U),
		      SDHC_CLOCK_STABLE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_WRN("Internal clock stabilization timed out");
	}

	sdhc_regs->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;
}

static inline void sdhc_clock_disable(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_CCR &= (uint16_t)(~(SDHC_CCR_INTCLKEN_Msk | SDHC_CCR_SDCLKEN_Msk));
}

static void sdhc_set_transfermode(const struct device *dev, int opcode,
				  struct sdhc_data_transfer_flags transfer_flags)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t transfer_mode = 0U;

	switch (opcode) {
	case SD_APP_SEND_SCR:
	case SD_APP_SET_BUS_WIDTH:
	case SD_READ_SINGLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_DTDSEL_Msk);
		break;

	case SD_READ_MULTIPLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_DTDSEL_Msk | SDHC_TMR_MSBSEL_Msk |
				 SDHC_TMR_BCEN_Msk);
		break;

	case SD_WRITE_SINGLE_BLOCK:
		transfer_mode = SDHC_TMR_DMAEN_ENABLE;
		break;

	case SD_WRITE_MULTIPLE_BLOCK:
		transfer_mode = (SDHC_TMR_DMAEN_ENABLE | SDHC_TMR_MSBSEL_Msk | SDHC_TMR_BCEN_Msk);
		break;

	case SDIO_RW_EXTENDED:
		if (transfer_flags.transfer_type == SDHC_DATA_TRANSFER_SDIO_BLOCK) {
			transfer_mode = SDHC_TMR_MSBSEL_Msk | SDHC_TMR_BCEN_Msk;
		}

		if (transfer_flags.transfer_dir == SDHC_DATA_TRANSFER_DIR_READ) {
			transfer_mode |= SDHC_TMR_DTDSEL_Msk;
		}

		transfer_mode |= SDHC_TMR_DMAEN_ENABLE;
		break;

	default:
		break;
	}

	sdhc_regs->SDHC_TMR = transfer_mode;
}

static int sdhc_dma_setup(const struct device *dev, uint8_t *buffer, int num_bytes)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint32_t remaining;
	uint32_t addr;
	uint32_t i = 0U;

	if ((buffer == NULL) || (num_bytes <= 0)) {
		return -EINVAL;
	}

	if ((uint32_t)num_bytes > (SDHC1_DMA_NUM_DESCR_LINES * SDHC_ADMA2_DESC_MAX_BYTES)) {
		LOG_ERR("Transfer size %d exceeds chained ADMA2 capacity %u", num_bytes,
			(unsigned int)(SDHC1_DMA_NUM_DESCR_LINES * SDHC_ADMA2_DESC_MAX_BYTES));
		return -ENOMEM;
	}

	remaining = (uint32_t)num_bytes;
	addr = (uint32_t)(uintptr_t)buffer;

	while (remaining > 0U) {
		uint32_t chunk = (remaining > SDHC_ADMA2_DESC_MAX_BYTES) ? SDHC_ADMA2_DESC_MAX_BYTES
									 : remaining;
		uint16_t attr = SDHC_DESC_TABLE_ATTR_XFER_DATA | SDHC_DESC_TABLE_ATTR_VALID;

		dev_data->dma_desc_table[i].address = addr;
		/* ADMA2 encodes a length of 65536 as the value 0; the cast handles that. */
		dev_data->dma_desc_table[i].length = (uint16_t)chunk;

		remaining -= chunk;
		addr += chunk;

		if (remaining == 0U) {
			attr |= SDHC_DESC_TABLE_ATTR_INTR | SDHC_DESC_TABLE_ATTR_END;
		}
		dev_data->dma_desc_table[i].attribute = attr;
		i++;
	}

	sdhc_regs->SDHC_ASAR[0] = (uint32_t)(uintptr_t)&dev_data->dma_desc_table[0];
	return 0;
}

static bool sdhc_set_clock(const struct device *dev, int speed)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint32_t base_clk_frq = 0U;
	uint16_t divider = 0U;
	uint32_t clk_mul = 0U;

	if ((sdhc_regs->SDHC_CCR & SDHC_CCR_SDCLKEN_Msk) != 0U) {
		if (!WAIT_FOR(((sdhc_regs->SDHC_PSR &
				(SDHC_PSR_CMDINHC_Msk | SDHC_PSR_CMDINHD_Msk)) == 0U),
			      SDHC_CMD_IDLE_TIMEOUT_US, k_busy_wait(1))) {
			LOG_WRN("SDHC bus did not become idle before clock change");
		}

		sdhc_regs->SDHC_CCR &= (uint16_t)(~SDHC_CCR_SDCLKEN_Msk);
	}

	base_clk_frq = (sdhc_regs->SDHC_CA0R & (SDHC_CA0R_BASECLKF_Msk)) >> SDHC_CA0R_BASECLKF_Pos;
	if (base_clk_frq == 0U) {
		base_clk_frq = (uint32_t)(SDHC1_BASE_CLOCK_FREQUENCY / SDHC_BASECLKF_DEFAULT_DIV);
	} else {
		base_clk_frq *= SDHC_BASECLKF_HZ_PER_MHZ;
	}

	clk_mul = (sdhc_regs->SDHC_CA1R & (SDHC_CA1R_CLKMULT_Msk)) >> SDHC_CA1R_CLKMULT_Pos;
	if (clk_mul == 0U) {
		return false;
	}

	divider = (uint16_t)((base_clk_frq * (clk_mul + 1U)) / speed);
	if (divider > 0U) {
		divider = divider - 1U;
	}

	sdhc_regs->SDHC_CCR |= SDHC_CCR_CLKGSEL_Msk;

	if (speed > SD_CLOCK_25MHZ) {
		sdhc_regs->SDHC_HC1R |= SDHC_HC1R_HSEN_Msk;
	} else {
		sdhc_regs->SDHC_HC1R &= (uint8_t)(~SDHC_HC1R_HSEN_Msk);
	}

	if (((sdhc_regs->SDHC_HC1R & SDHC_HC1R_HSEN_Msk) != 0U) && (divider == 0U)) {
		divider = 1;
	}

	sdhc_regs->SDHC_CCR &= (uint16_t)(~(SDHC_CCR_SDCLKFSEL_Msk | SDHC_CCR_USDCLKFSEL_Msk));
	sdhc_regs->SDHC_CCR |= SDHC_CCR_SDCLKFSEL((divider & SDHC_CCR_SDCLKFSEL_MASK)) |
			       SDHC_CCR_USDCLKFSEL((divider >> SDHC_CCR_USDCLKFSEL_SHIFT));

	sdhc_regs->SDHC_CCR |= SDHC_CCR_INTCLKEN_Msk;

	if (!WAIT_FOR(((sdhc_regs->SDHC_CCR & SDHC_CCR_INTCLKS_Msk) != 0U),
		      SDHC_CLOCK_STABLE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_WRN("Internal clock stabilization timed out in set_clock");
	}

	sdhc_regs->SDHC_CCR |= SDHC_CCR_SDCLKEN_Msk;

	return true;
}

static void sdhc_read_response(const struct device *dev, enum sdhc_read_response_reg resp_reg,
			       int *response)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	switch (resp_reg) {
	case SDHC_READ_RESP_REG_1:
		*response = sdhc_regs->SDHC_RR[1];
		break;

	case SDHC_READ_RESP_REG_2:
		*response = sdhc_regs->SDHC_RR[2];
		break;

	case SDHC_READ_RESP_REG_3:
		*response = sdhc_regs->SDHC_RR[3];
		break;

	case SDHC_READ_RESP_REG_ALL:
		response[0] = sdhc_regs->SDHC_RR[0];
		response[1] = sdhc_regs->SDHC_RR[1];
		response[2] = sdhc_regs->SDHC_RR[2];
		response[3] = sdhc_regs->SDHC_RR[3];
		break;

	case SDHC_READ_RESP_REG_0:
	default:
		*response = sdhc_regs->SDHC_RR[0];
		break;
	}
}

static void sdhc_send_command(const struct device *dev, uint8_t op_code, int argument,
			      uint8_t resp_type, struct sdhc_data_transfer_flags transfer_flags)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t cmd = 0U;
	uint16_t normal_int_sig_enable = 0U;
	uint16_t flags = 0U;

	dev_data->sdhc_obj.is_cmd_in_progress = false;
	dev_data->sdhc_obj.is_data_in_progress = false;
	dev_data->sdhc_obj.error_status = 0U;

	normal_int_sig_enable = (SDHC_NISIER_CINS_Msk | SDHC_NISIER_CREM_Msk);

	switch (resp_type) {
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		flags = (SDHC_CR_RESPTYP_48_BIT_Val | SDHC_CR_CMDCCEN_Msk | SDHC_CR_CMDICEN_Msk);
		break;

	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		flags = SDHC_CR_RESPTYP_48_BIT_Val;
		break;

	case SD_RSP_TYPE_R1b:
		flags = (SDHC_CR_RESPTYP_48_BIT_BUSY_Val | SDHC_CR_CMDCCEN_Msk |
			 SDHC_CR_CMDICEN_Msk);
		normal_int_sig_enable |= SDHC_NISIER_TRFC_Msk;
		break;

	case SD_RSP_TYPE_R2:
		flags = (SDHC_CR_RESPTYP_136_BIT_Val | SDHC_CR_CMDCCEN_Msk);
		break;

	default:
		flags = SDHC_CR_RESPTYP_NONE_Val;
		break;
	}

	if (resp_type != (uint8_t)SD_RSP_TYPE_R1b) {
		normal_int_sig_enable |= SDHC_NISIER_CMDC_Msk;
	}

	if (transfer_flags.is_data_present == true) {
		dev_data->sdhc_obj.is_data_in_progress = true;
		sdhc_set_transfermode(dev, op_code, transfer_flags);
		normal_int_sig_enable |= (SDHC_NISIER_TRFC_Msk | SDHC_NISIER_DMAINT_Msk);
	} else {
		sdhc_regs->SDHC_TMR = 0U;
	}

	sdhc_regs->SDHC_NISIER = normal_int_sig_enable;
	sdhc_regs->SDHC_EISIER = SDHC_EISIER_Msk;

	sdhc_regs->SDHC_ARG1R = argument;

	dev_data->sdhc_obj.is_cmd_in_progress = true;

	cmd = (SDHC_CR_CMDIDX((uint16_t)op_code) |
	       (transfer_flags.is_data_present ? (1U << SDHC_CR_DPSEL_Pos) : 0U) | (uint16_t)flags);
	sdhc_regs->SDHC_CR = cmd;
}

static void sdhc_init_module(const struct device *dev)
{
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);

	sdhc_regs->SDHC_SRR |= SDHC_SRR_SWRSTALL_Msk;
	if (!WAIT_FOR(((sdhc_regs->SDHC_SRR & SDHC_SRR_SWRSTALL_Msk) == 0U), SDHC_RESET_TIMEOUT_US,
		      k_busy_wait(1))) {
		LOG_WRN("SDHC software reset timed out");
	}

	sdhc_regs->SDHC_EISTR = SDHC_EISTR_Msk;
	sdhc_regs->SDHC_NISTR = SDHC_NISTR_Msk;
	sdhc_regs->SDHC_NISTER = SDHC_NISTER_Msk;
	sdhc_regs->SDHC_EISTER = SDHC_EISTER_Msk;
	sdhc_regs->SDHC_TCR = SDHC_TCR_DTCVAL(SDHC_DATA_TIMEOUT_CNT_VAL);
	sdhc_regs->SDHC_HC1R |= SDHC_HC1R_DMASEL(SDHC_DMASEL_ADMA2);
	sdhc_regs->SDHC_PCR = (SDHC_PCR_SDBVSEL_3V3 | SDHC_PCR_SDBPWR_ON);

	(void)sdhc_set_clock(dev, SDMMC_CLOCK_400KHZ);

	sdhc_regs->SDHC_HC1R &= (uint8_t)(~(SDHC_HC1R_HSEN_Msk | SDHC_HC1R_DW_Msk));
	sdhc_regs->SDHC_NISIER = (SDHC_NISIER_CINS_Msk | SDHC_NISIER_CREM_Msk);
}

static int sdhc_wait_completion(struct sdhc_mchp_data *const dev_data, k_timeout_t timeout)
{
	if (k_sem_take(&dev_data->transfer_done, timeout) == 0) {
		return 0;
	}

	return -ETIMEDOUT;
}

static void sdhc_init_props(const struct device *dev)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	const struct sdhc_mchp_cfg *cfg = dev->config;

	memset(&dev_data->props, 0, sizeof(dev_data->props));
	dev_data->props.f_min = cfg->min_bus_freq;
	dev_data->props.f_max = cfg->max_bus_freq;
	dev_data->props.power_delay = cfg->power_delay_ms;
	dev_data->props.host_caps.vol_330_support = true;
	dev_data->props.host_caps.sdma_support = true;
	dev_data->props.host_caps.adma_2_support = true;
	dev_data->props.host_caps.high_spd_support = true;
	dev_data->props.host_caps.bus_8_bit_support = false;
}

static uint8_t sdhc_resp_type(struct sdhc_command *cmd)
{
	switch (cmd->opcode) {

	case SD_GO_IDLE_STATE:
		return SD_RSP_TYPE_NONE;

	case SD_SEND_IF_COND:
		return SD_RSP_TYPE_R7;

	case SD_APP_CMD:
		return SD_RSP_TYPE_R1;

	case SD_APP_SEND_OP_COND:
		return SD_RSP_TYPE_R3;

	case SD_ALL_SEND_CID:
	case SD_SEND_CID:
	case SD_SEND_CSD:
		return SD_RSP_TYPE_R2;

	case SD_SEND_RELATIVE_ADDR:
		return SD_RSP_TYPE_R6;

	case SD_SELECT_CARD:
	case SD_STOP_TRANSMISSION:
		return SD_RSP_TYPE_R1b;

	default:
		return SD_RSP_TYPE_R1;
	}
}

static void sdhc_mchp_isr(const struct device *dev)
{
	struct sdhc_mchp_data *const dev_data = dev->data;
	sdhc_registers_t *sdhc_regs = SDHC_REG(dev);
	uint16_t nistr = 0U;
	uint16_t eistr = 0U;
	uint32_t xfer_status = 0U;

	nistr = sdhc_regs->SDHC_NISTR;
	eistr = sdhc_regs->SDHC_EISTR;

	dev_data->sdhc_obj.error_status |= eistr;

	if ((nistr & SDHC_NISTR_TRFC_Msk) != 0U) {
		dev_data->sdhc_obj.error_status &= (uint16_t)(~SDHC_EISTR_DATTEO_Msk);
	}

	if ((nistr & SDHC_NISTR_CINS_Msk) != 0U) {
		xfer_status |= SDHC_XFER_STATUS_CARD_INSERTED;
	}

	if ((nistr & SDHC_NISTR_CREM_Msk) != 0U) {
		xfer_status |= SDHC_XFER_STATUS_CARD_REMOVED;
	}

	if (dev_data->sdhc_obj.is_cmd_in_progress == true) {
		if ((nistr & (SDHC_NISTR_CMDC_Msk | SDHC_NISTR_TRFC_Msk | SDHC_NISTR_ERRINT_Msk)) !=
		    0U) {
			if ((nistr & SDHC_NISTR_ERRINT_Msk) != 0U) {
				if (((eistr & (SDHC_EISTR_CMDTEO_Msk | SDHC_EISTR_CMDCRC_Msk |
					       SDHC_EISTR_CMDEND_Msk | SDHC_EISTR_CMDIDX_Msk))) !=
				    0U) {
					sdhc_reset_error(dev, SDHC_RESET_CMD);
				}
			}

			dev_data->sdhc_obj.is_cmd_in_progress = false;
			xfer_status |= SDHC_XFER_STATUS_CMD_COMPLETED;
		}
	}

	if (dev_data->sdhc_obj.is_data_in_progress == true) {
		if ((nistr &
		     (SDHC_NISTR_TRFC_Msk | SDHC_NISTR_DMAINT_Msk | SDHC_NISTR_ERRINT_Msk)) != 0U) {
			if ((nistr & SDHC_NISTR_ERRINT_Msk) != 0U) {
				if ((eistr & (SDHC_EISTR_DATTEO_Msk | SDHC_EISTR_DATCRC_Msk |
					      SDHC_EISTR_DATEND_Msk)) != 0U) {
					sdhc_reset_error(dev, SDHC_RESET_DAT);
				}
			}

			dev_data->sdhc_obj.is_data_in_progress = false;
			xfer_status |= SDHC_XFER_STATUS_DATA_COMPLETED;
		}
	}

	sdhc_regs->SDHC_NISTR = nistr;
	sdhc_regs->SDHC_EISTR = eistr;

	if (xfer_status != 0U) {
		k_sem_give(&dev_data->transfer_done);
	}
}

static int sdhc_mchp_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *data)
{
	int rc = 0;
	struct sdhc_mchp_data *const dev_data = dev->data;
	struct sdhc_data_transfer_flags transfer_flags = {0};
	uint8_t op = (uint8_t)cmd->opcode;
	k_timeout_t cmd_timeout;

	__ASSERT_NO_MSG(cmd != NULL);

	if (cmd->timeout_ms == SDHC_TIMEOUT_FOREVER) {
		cmd_timeout = K_FOREVER;
	} else if (cmd->timeout_ms == 0) {
		cmd_timeout = K_MSEC(SDHC_CMD_DEFAULT_TIMEOUT_MS);
	} else {
		cmd_timeout = K_MSEC(cmd->timeout_ms);
	}

	if (k_mutex_lock(&dev_data->bus_mutex, cmd_timeout)) {
		return -EBUSY;
	}

	if (data != NULL) {
		sdhc_set_blocksize(dev, (uint16_t)data->block_size);
		sdhc_set_blockcount(dev, (uint16_t)data->blocks);
		rc = sdhc_dma_setup(dev, (uint8_t *)data->data, data->blocks * data->block_size);
		if (rc != 0) {
			k_mutex_unlock(&dev_data->bus_mutex);
			return rc;
		}

		transfer_flags.is_data_present = true;
		if (cmd->opcode == SDIO_RW_EXTENDED) {
			transfer_flags.transfer_type = SDHC_DATA_TRANSFER_SDIO_BLOCK;
		} else {
			transfer_flags.transfer_type = SDHC_DATA_TRANSFER_TYPE_SINGLE;
		}
	}

	sdhc_reset_error(dev, SDHC_RESET_CMD | SDHC_RESET_DAT);

	/* Drain stale completions before issuing the command */
	while (k_sem_count_get(&dev_data->transfer_done) != 0U) {
		k_sem_take(&dev_data->transfer_done, K_NO_WAIT);
	}

	sdhc_send_command(dev, op, cmd->arg, sdhc_resp_type(cmd), transfer_flags);

	rc = sdhc_wait_completion(dev_data, cmd_timeout);
	if (rc != 0) {
		k_mutex_unlock(&dev_data->bus_mutex);
		return rc;
	}

	if (!WAIT_FOR(((dev_data->sdhc_obj.is_cmd_in_progress == false) &&
		       (dev_data->sdhc_obj.is_data_in_progress == false)),
		      SDHC_XFER_DONE_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Transfer completion flags did not clear");
		k_mutex_unlock(&dev_data->bus_mutex);
		return -ETIMEDOUT;
	}

	sdhc_read_response(dev, SDHC_READ_RESP_REG_ALL, dev_data->sdhc_response);

	/*
	 * R2: hardware stores the 128-bit payload right-shifted by 8 across
	 * RR[3:0] with RR[3] as MSW; sd_parse_csd() expects response[0] as
	 * LSW.  Reverse the order and undo the 8-bit shift.  Cast to uint32_t
	 * to force logical right-shift (sdhc_response[] is signed int).
	 */
	if (cmd->response_type == SD_RSP_TYPE_R2) {
		uint32_t w0 = (uint32_t)dev_data->sdhc_response[0];
		uint32_t w1 = (uint32_t)dev_data->sdhc_response[1];
		uint32_t w2 = (uint32_t)dev_data->sdhc_response[2];
		uint32_t w3 = (uint32_t)dev_data->sdhc_response[3];

		cmd->response[3] = (int)((w3 << 8) | (w2 >> 24));
		cmd->response[2] = (int)((w2 << 8) | (w1 >> 24));
		cmd->response[1] = (int)((w1 << 8) | (w0 >> 24));
		cmd->response[0] = (int)(w0 << 8);
	} else {
		cmd->response[0] = dev_data->sdhc_response[0];
		cmd->response[1] = dev_data->sdhc_response[1];
		cmd->response[2] = dev_data->sdhc_response[2];
		cmd->response[3] = dev_data->sdhc_response[3];
	}

	k_mutex_unlock(&dev_data->bus_mutex);

	return rc;
}

static int sdhc_mchp_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_mchp_data *const dev_data = dev->data;

	if (ios->clock != 0U) {
		if ((ios->clock < dev_data->props.f_min) || (ios->clock > dev_data->props.f_max)) {
			LOG_ERR("Requested clock %u out of range (%u..%u)", ios->clock,
				dev_data->props.f_min, dev_data->props.f_max);
			return -EINVAL;
		}

		if (!sdhc_set_clock(dev, ios->clock)) {
			LOG_ERR("Failed to set clock to %u Hz", ios->clock);
			return -EIO;
		}
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		sdhc_clock_disable(dev);
	} else {
		sdhc_clock_enable(dev);
	}

	if (ios->bus_width != 0) {
		if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			sdhc_set_buswidth(dev, SDHC_BUS_WIDTH4BIT);
		} else if (ios->bus_width == SDHC_BUS_WIDTH1BIT) {
			sdhc_set_buswidth(dev, SDHC_BUS_WIDTH1BIT);
		} else {
			/* Unsupported bus width */
			LOG_ERR("Failed to set bus width to %u", ios->bus_width);
			return -ENOTSUP;
		}
	}

	return 0;
}

static int sdhc_mchp_get_card_present(const struct device *dev)
{
	return sdhc_is_card_attached(dev);
}

static int sdhc_mchp_card_busy(const struct device *dev)
{
	return (sdhc_is_cmdline_busy(dev) || sdhc_is_dataline_busy(dev));
}

static int sdhc_mchp_reset(const struct device *dev)
{
	const struct sdhc_mchp_cfg *cfg = dev->config;
	struct sdhc_mchp_data *const dev_data = dev->data;

	dev_data->sdhc_obj.error_status = 0U;
	dev_data->sdhc_obj.is_cmd_in_progress = false;
	dev_data->sdhc_obj.is_data_in_progress = false;
	sdhc_init_module(dev);

	/*
	 * Allow time for the card to power up and respond to commands after reset
	 * before the host attempts initialization.
	 */
	k_msleep(cfg->power_delay_ms);

	return 0;
}

static int sdhc_mchp_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_mchp_data *const dev_data = dev->data;

	memcpy(props, &dev_data->props, sizeof(*props));

	return 0;
}

static int sdhc_mchp_init(const struct device *dev)
{
	int ret = 0;
	struct sdhc_mchp_data *const dev_data = dev->data;
	const struct sdhc_mchp_cfg *cfg = dev->config;

	if (device_is_ready(cfg->sdhc_clock.clock_dev) != true) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable MCLK: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable GCLK: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->sdhc_clock.clock_dev, cfg->sdhc_clock.gclk_slow_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable slow GCLK: %d", ret);
		return ret;
	}

	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("pinctrl apply failed: %d", ret);
			return ret;
		}
	}

	sdhc_init_module(dev);
	k_mutex_init(&dev_data->bus_mutex);
	k_sem_init(&dev_data->transfer_done, 0, 1);
	cfg->irq_config_func(dev);

	sdhc_init_props(dev);

	return 0;
}

static DEVICE_API(sdhc, sdhc_mchp_api) = {
	.request = sdhc_mchp_request,
	.set_io = sdhc_mchp_set_io,
	.get_host_props = sdhc_mchp_get_host_props,
	.get_card_present = sdhc_mchp_get_card_present,
	.card_busy = sdhc_mchp_card_busy,
	.reset = sdhc_mchp_reset,
};

/*
 * Per-instance ADMA2 descriptor table and runtime data.
 *
 * The descriptor table must be aligned to 32 bytes so it never shares a
 * cache line with other data, preventing DMA coherency faults on Cortex-M4.
 */
#define SDHC_MCHP_DATA_DEFN(n)                                                                     \
	static struct sdhc_adma_descr __aligned(32)                                                \
	sdhc_mchp_dma_desc_##n[(SDHC1_DMA_DESC_TABLE_SIZE_CACHE_ALIGN / 8U)];                      \
	static struct sdhc_mchp_data sdhc_mchp_data_##n = {                                        \
		.dma_desc_table = sdhc_mchp_dma_desc_##n,                                          \
		.sdhc_obj =                                                                        \
			{                                                                          \
				.error_status = 0U,                                                \
				.is_cmd_in_progress = false,                                       \
				.is_data_in_progress = false,                                      \
			},                                                                         \
	}

#define SDHC_MCHP_CONFIG_DEFN(n)                                                                   \
	static const struct sdhc_mchp_cfg sdhc_mchp_cfg_##n = {                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.sdhc_clock.clock_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),          \
		.sdhc_clock.mclk_sys =                                                             \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem),        \
		.sdhc_clock.gclk_sys =                                                             \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),        \
		.sdhc_clock.gclk_slow_sys =                                                        \
			(void *)(uintptr_t)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk_slow, subsystem),   \
		.power_delay_ms = DT_INST_PROP(n, power_delay_ms),                                 \
		.min_bus_freq = DT_INST_PROP(n, min_bus_freq),                                     \
		.max_bus_freq = DT_INST_PROP(n, max_bus_freq),                                     \
		.irq_config_func = sdhc_mchp_irq_config_##n,                                       \
	}

#define MCHP_SDHC_IRQ_CONNECT(n, m)                                                                \
	do {                                                                                       \
		((sdhc_registers_t *)(uintptr_t)DT_INST_REG_ADDR(n))->SDHC_NISTR = SDHC_NISTR_Msk; \
		((sdhc_registers_t *)(uintptr_t)DT_INST_REG_ADDR(n))->SDHC_EISTR = SDHC_EISTR_Msk; \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    sdhc_mchp_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#define SDHC_MCHP_IRQ_HANDLER(n)                                                                   \
	static void sdhc_mchp_irq_config_##n(const struct device *dev)                             \
	{                                                                                          \
		MCHP_SDHC_IRQ_CONNECT(n, 0);                                                       \
	}

#define SDHC_MCHP_DEVICE_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void sdhc_mchp_irq_config_##n(const struct device *dev);                            \
	SDHC_MCHP_DATA_DEFN(n);                                                                    \
	SDHC_MCHP_CONFIG_DEFN(n);                                                                  \
	DEVICE_DT_INST_DEFINE(n, sdhc_mchp_init, NULL, &sdhc_mchp_data_##n, &sdhc_mchp_cfg_##n,    \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdhc_mchp_api);             \
	SDHC_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SDHC_MCHP_DEVICE_INIT)

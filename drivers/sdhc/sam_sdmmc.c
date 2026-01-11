/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sama7g5_sdmmc

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sdhc.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sdmmc, CONFIG_SDHC_LOG_LEVEL);

#define CMD_ERROR_MASK	(SDMMC_EISTR_SD_SDIO_CMDIDX_Msk | \
			 SDMMC_EISTR_SD_SDIO_CMDEND_Msk | \
			 SDMMC_EISTR_SD_SDIO_CMDCRC_Msk | \
			 SDMMC_EISTR_SD_SDIO_CMDTEO_Msk)
#define DATA_ERROR_MASK	(SDMMC_EISTR_SD_SDIO_DATEND_Msk | \
			 SDMMC_EISTR_SD_SDIO_DATCRC_Msk | \
			 SDMMC_EISTR_SD_SDIO_DATTEO_Msk)

#define INT_CMD_MASK	(SDMMC_NISTR_SD_SDIO_TRFC_Msk | \
			 SDMMC_NISTR_SD_SDIO_CMDC_Msk)
#define INT_DATA_MASK	(SDMMC_NISTR_SD_SDIO_BRDRDY_Msk | \
			 SDMMC_NISTR_SD_SDIO_BWRRDY_Msk | \
			 SDMMC_NISTR_SD_SDIO_DMAINT_Msk)
#define INT_CMD_ERROR_MASK \
			(CMD_ERROR_MASK | \
			 SDMMC_EISTR_SD_SDIO_ACMD_Msk)
#define INT_DATA_ERROR_MASK \
			(DATA_ERROR_MASK | \
			 SDMMC_EISTR_SD_SDIO_ADMA_Msk)

#define INT_MASK		(INT_CMD_MASK | \
				 INT_DATA_MASK | \
				 SDMMC_NISTR_SD_SDIO_ERRINT_Msk | \
				 SDMMC_NISTR_SD_SDIO_CREM_Msk | \
				 SDMMC_NISTR_SD_SDIO_CINS_Msk)
#define INT_ERROR_MASK		(INT_CMD_ERROR_MASK | \
				 INT_DATA_ERROR_MASK | \
				 SDMMC_EISTR_SD_SDIO_CURLIM_Msk)

#define DEFAULT_INT_MASK	(INT_CMD_MASK | \
				 SDMMC_NISTR_SD_SDIO_CINS_Msk | \
				 SDMMC_NISTR_SD_SDIO_CREM_Msk)
#define DEFAULT_INT_ERROR_MASK	(CMD_ERROR_MASK | \
				 DATA_ERROR_MASK | \
				 SDMMC_EISTR_SD_SDIO_CURLIM_Msk)

#define IS_READ_CMD(cmd) \
((((cmd) == SD_READ_SINGLE_BLOCK) || ((cmd) == SD_READ_MULTIPLE_BLOCK)) ? 1 : 0)
#define IS_WRITE_CMD(cmd) \
((((cmd) == SD_WRITE_SINGLE_BLOCK) || ((cmd) == SD_WRITE_MULTIPLE_BLOCK)) ? 1 : 0)
#define IS_MULTI_BLOCK(cmd) \
((((cmd) == SD_READ_MULTIPLE_BLOCK) || ((cmd) == SD_WRITE_MULTIPLE_BLOCK)) ? 1 : 0)
#define IS_DATA_CMD(cmd) \
((IS_READ_CMD(cmd) || IS_WRITE_CMD(cmd)) ? 1 : 0)

#define INH_TO_RST(x)	(((x) & 0x3) << 1)

/* ADMA2 32-bit descriptor */
struct adma2_desc {
	uint16_t cmd;
	uint16_t len;
	uint32_t addr;
} __packed __aligned(4);

#define ADMA2_TRAN_VALID	0x21
#define ADMA2_INT		0x4
#define ADMA2_END		0x2

/* ADMA2 data alignment */
#define ADMA2_ALIGN		4
#define ADMA2_MASK		(ADMA2_ALIGN - 1)
#define ADMA2_ALIGNED(addr)	(!(((uint32_t)(addr)) & ADMA2_MASK))

#define ADMA2_NUM_DESC		64
#define ADMA2_MAX_LEN		65536
#define ADMA2_MAX_SIZE		(ADMA2_NUM_DESC * ADMA2_MAX_LEN)

#define MAX_DATA_TIMEOUT	0xE

enum req_flag {
	REQ_RSP_PRESENT	= BIT(0),
	REQ_RSP_136	= BIT(1),
	REQ_RSP_DATA	= BIT(2),
	REQ_RSP_TRFC	= BIT(3),
	REQ_USE_ADMA	= BIT(4),
	REQ_IS_READ	= BIT(5),
};

#define REQ_RSP_MASK	(REQ_RSP_PRESENT | \
			 REQ_RSP_136 | \
			 REQ_RSP_DATA | \
			 REQ_RSP_TRFC)

struct sd_request {
	enum req_flag flags;
	uint32_t reset_mask;
	uint32_t *response;
	uint32_t block_size;
	uint32_t blocks;
	uint8_t  *data;
	struct   k_sem completion;
};

struct sam_sdmmc_config {
	sdmmc_registers_t *base;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pincfg;
	uint32_t base_clk;
	uint32_t non_removable: 1;
	uint32_t bus_width: 4;
	uint32_t no_18v: 1;
	uint32_t rstn_power_en: 1;
	uint32_t auto_cmd12: 1;
	uint32_t auto_cmd23: 1;
	uint32_t mmc_hs200_18v: 1;
	uint32_t mmc_hs400_18v: 1;
	uint32_t max_bus_freq;
	uint32_t min_bus_freq;
	uint32_t power_delay_ms;
	uint32_t max_current_330;
	uint32_t max_current_180;
	void (*irq_config_func)(const struct device *dev);
};

struct sam_sdmmc_data {
	struct sdhc_host_props *props;
	struct sdhc_io    io_cfg;
	struct sd_request *req;
	struct k_mutex    mutex;
	struct adma2_desc desc[ADMA2_NUM_DESC];
};

static int sam_sdmmc_reset(const struct device *dev);

static void sdmmc_set_default_irqs(sdmmc_registers_t *sdmmc)
{
	sdmmc->SDMMC_NISTER = DEFAULT_INT_MASK;
	sdmmc->SDMMC_EISTER = DEFAULT_INT_ERROR_MASK;
	sdmmc->SDMMC_NISIER = DEFAULT_INT_MASK;
	sdmmc->SDMMC_EISIER = DEFAULT_INT_ERROR_MASK;
}

static void sdmmc_set_dma_irqs(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_NISTER |= SDMMC_NISTER_SD_SDIO_DMAINT_Msk;
		sdmmc->SDMMC_EISTER |= SDMMC_EISTER_SD_SDIO_ADMA_Msk;
		sdmmc->SDMMC_NISIER |= SDMMC_NISIER_SD_SDIO_DMAINT_Msk;
		sdmmc->SDMMC_EISIER |= SDMMC_EISIER_SD_SDIO_ADMA_Msk;
	} else {
		sdmmc->SDMMC_NISTER &= ~SDMMC_NISTER_SD_SDIO_DMAINT_Msk;
		sdmmc->SDMMC_EISTER &= ~SDMMC_EISTER_SD_SDIO_ADMA_Msk;
		sdmmc->SDMMC_NISIER &= ~SDMMC_NISIER_SD_SDIO_DMAINT_Msk;
		sdmmc->SDMMC_EISIER &= ~SDMMC_EISIER_SD_SDIO_ADMA_Msk;
	}
}

static void sdmmc_set_pio_irqs(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_NISTER |= SDMMC_NISTER_SD_SDIO_BRDRDY_Msk |
				       SDMMC_NISTER_SD_SDIO_BWRRDY_Msk;
		sdmmc->SDMMC_NISIER |= SDMMC_NISIER_SD_SDIO_BRDRDY_Msk |
				       SDMMC_NISIER_SD_SDIO_BWRRDY_Msk;
	} else {
		sdmmc->SDMMC_NISTER &= ~(SDMMC_NISTER_SD_SDIO_BRDRDY_Msk |
					 SDMMC_NISTER_SD_SDIO_BWRRDY_Msk);
		sdmmc->SDMMC_NISIER &= ~(SDMMC_NISIER_SD_SDIO_BRDRDY_Msk |
					 SDMMC_NISIER_SD_SDIO_BWRRDY_Msk);
	}
}

static void sdmmc_set_acmd_irqs(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_EISTER |= SDMMC_EISTER_SD_SDIO_ACMD_Msk;
		sdmmc->SDMMC_EISIER |= SDMMC_EISIER_SD_SDIO_ACMD_Msk;
	} else {
		sdmmc->SDMMC_EISTER &= ~SDMMC_EISTER_SD_SDIO_ACMD_Msk;
		sdmmc->SDMMC_EISIER &= ~SDMMC_EISIER_SD_SDIO_ACMD_Msk;
	}
}

static int sdmmc_reset(sdmmc_registers_t *sdmmc, uint8_t mask)
{
	uint16_t clk = sdmmc->SDMMC_CCR;
	uint32_t timeout = 100;

	/* SDCLK must be disabled while resetting the HW block */
	if (clk & SDMMC_CCR_SDCLKEN_Msk) {
		sdmmc->SDMMC_CCR &= ~SDMMC_CCR_SDCLKEN_Msk;
	}

	sdmmc->SDMMC_SRR = mask;

	while ((sdmmc->SDMMC_SRR & mask) && timeout--) {
		k_msleep(1);
	}

	/* Reenable SDCLK */
	if (clk & SDMMC_CCR_SDCLKEN_Msk) {
		sdmmc->SDMMC_CCR |= SDMMC_CCR_SDCLKEN_Msk;
	}

	if (sdmmc->SDMMC_SRR & mask) {
		LOG_ERR("%s: timeout!", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static void sdmmc_set_bus_power(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_PCR |= SDMMC_PCR_SDBPWR_Msk;
	} else {
		sdmmc->SDMMC_PCR &= ~SDMMC_PCR_SDBPWR_Msk;
	}
}

static void sdmmc_set_force_card_detect(sdmmc_registers_t *sdmmc)
{
	sdmmc->SDMMC_MC1R |= SDMMC_MC1R_FCD_ENABLED;
}

static int sdmmc_set_clock(sdmmc_registers_t *sdmmc, uint32_t base_clk, uint32_t clock)
{
	uint32_t mult_clk;
	uint32_t divider, div_mask;
	uint32_t timeout = 150;

	if (clock == 0) {
		sdmmc->SDMMC_CCR &= ~SDMMC_CCR_SDCLKEN_Msk;
		return 0;
	}

	/* Retain INTCLKEN, clear all other bits */
	sdmmc->SDMMC_CCR &= SDMMC_CCR_INTCLKEN_Msk;

	div_mask = (SDMMC_CCR_SDCLKFSEL_Msk | SDMMC_CCR_USDCLKFSEL_Msk) >>
		   SDMMC_CCR_USDCLKFSEL_Pos;

	/* Programmable Clock Mode */
	if (sdmmc->SDMMC_CA1R & SDMMC_CA1R_CLKMULT_Msk) {
		mult_clk = (((sdmmc->SDMMC_CA1R & SDMMC_CA1R_CLKMULT_Msk) >>
			   SDMMC_CA1R_CLKMULT_Pos) + 1) * base_clk;

		if (mult_clk <= clock) {
			divider = 0;
			sdmmc->SDMMC_CCR |= SDMMC_CCR_CLKGSEL_Msk;
			goto done;
		}

		divider = (mult_clk / clock) - 1;
		if ((mult_clk / (divider + 1)) > clock) {
			divider++;
		}

		if (divider <= div_mask) {
			sdmmc->SDMMC_CCR |= SDMMC_CCR_CLKGSEL_Msk;
			goto done;
		}
	}

	/* Divided Clock Mode */
	if (base_clk <= clock) {
		divider = 0;
		goto done;
	}

	divider = base_clk / clock / 2;
	if (divider == 0) {
		divider = 1;
		goto done;
	}

	if ((base_clk / (2 * divider)) > clock) {
		divider++;
	}

	if (divider > div_mask) {
		return -ENOTSUP;
	}

done:
	sdmmc->SDMMC_CCR |= SDMMC_CCR_SDCLKFSEL(divider) |
			    SDMMC_CCR_USDCLKFSEL(divider >> 8) |
			    SDMMC_CCR_INTCLKEN_Msk;

	while (!(sdmmc->SDMMC_CCR & SDMMC_CCR_INTCLKS_Msk) && timeout--) {
		k_msleep(1);
	}

	if (!(sdmmc->SDMMC_CCR & SDMMC_CCR_INTCLKS_Msk)) {
		LOG_ERR("%s: timeout!", __func__);
		return -ETIMEDOUT;
	}

	sdmmc->SDMMC_CCR |= SDMMC_CCR_SDCLKEN_Msk;

	return 0;
}

static void sdmmc_set_rstn(sdmmc_registers_t *sdmmc, uint32_t active)
{
	if (active) {
		sdmmc->SDMMC_MC1R |= SDMMC_MC1R_RSTN_Msk;
	} else {
		sdmmc->SDMMC_MC1R &= ~SDMMC_MC1R_RSTN_Msk;
	}
}

static void sdmmc_cmd_line_mode(sdmmc_registers_t *sdmmc, uint32_t open_drain)
{
	if (open_drain) {
		sdmmc->SDMMC_MC1R |= SDMMC_MC1R_OPD_Msk;
	} else {
		sdmmc->SDMMC_MC1R &= ~SDMMC_MC1R_OPD_Msk;
	}
}

static int sdmmc_bus_width(sdmmc_registers_t *sdmmc, uint32_t width)
{
	if (width == 1) {
		sdmmc->SDMMC_HC1R &= ~(SDMMC_HC1R_EMMC_EXTDW_Msk |
				       SDMMC_HC1R_SD_SDIO_DW_Msk);
	} else if (width == 4) {
		sdmmc->SDMMC_HC1R &= ~SDMMC_HC1R_EMMC_EXTDW_Msk;
		sdmmc->SDMMC_HC1R |= SDMMC_HC1R_SD_SDIO_DW_Msk;
	} else if (width == 8) {
		sdmmc->SDMMC_HC1R |= SDMMC_HC1R_EMMC_EXTDW_Msk;
	} else {
		return -EINVAL;
	}

	return 0;
}

static void sdmmc_high_speed(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_HC1R |= SDMMC_HC1R_SD_SDIO_HSEN_Msk;
	} else {
		sdmmc->SDMMC_HC1R &= ~SDMMC_HC1R_SD_SDIO_HSEN_Msk;
	}
}

static void sdmmc_dirver_type(sdmmc_registers_t *sdmmc, uint32_t type)
{
	sdmmc->SDMMC_HC2R &= ~SDMMC_HC2R_SD_SDIO_DRVSEL_Msk;
	sdmmc->SDMMC_HC2R |= type << SDMMC_HC2R_SD_SDIO_DRVSEL_Pos;
}

static void sdmmc_set_1v8(sdmmc_registers_t *sdmmc, uint32_t enable)
{
	if (enable) {
		sdmmc->SDMMC_HC2R |= SDMMC_HC2R_SD_SDIO_VS18EN_Msk;
	} else {
		sdmmc->SDMMC_HC2R &= ~SDMMC_HC2R_SD_SDIO_VS18EN_Msk;
	}
}

static int sdmmc_card_present(sdmmc_registers_t *sdmmc)
{
	uint32_t timeout = 100;

	while (!(sdmmc->SDMMC_PSR & SDMMC_PSR_CARDSS_Msk) && timeout--) {
		k_msleep(1);
	}

	if (!(sdmmc->SDMMC_PSR & SDMMC_PSR_CARDSS_Msk)) {
		LOG_ERR("%s: timeout!", __func__);
		return 0;
	}

	return !!(sdmmc->SDMMC_PSR & SDMMC_PSR_CARDINS_Msk);
}

static int sdmmc_card_busy(sdmmc_registers_t *sdmmc)
{
	return !(sdmmc->SDMMC_PSR & (1 << SDMMC_PSR_DATLL_Pos));
}

static void req_cmd_irq(sdmmc_registers_t *sdmmc, struct sd_request *req, uint32_t status)
{
	if (!req) {
		return;
	}

	if (status & SDMMC_NISTR_SD_SDIO_CMDC_Msk) {
		if (req->flags & REQ_RSP_PRESENT) {
			req->response[0] = sdmmc->SDMMC_RR[0];
			req->flags &= ~REQ_RSP_PRESENT;
		}
		if (req->flags & REQ_RSP_136) {
			req->response[1] = sdmmc->SDMMC_RR[1];
			req->response[2] = sdmmc->SDMMC_RR[2];
			req->response[3] = sdmmc->SDMMC_RR[3];

			/* For CID and CSD, CRC is stripped so we need to do some shifting */
			req->response[3] = req->response[3] << 8 | req->response[2] >> 24;
			req->response[2] = req->response[2] << 8 | req->response[1] >> 24;
			req->response[1] = req->response[1] << 8 | req->response[0] >> 24;
			req->response[0] = req->response[0] << 8;

			req->flags &= ~REQ_RSP_136;
		}
	}

	if (status & SDMMC_NISTR_SD_SDIO_TRFC_Msk) {
		req->flags &= ~REQ_RSP_TRFC;
	}
}

static void req_data_irq(sdmmc_registers_t *sdmmc, struct sd_request *req, uint32_t status)
{
	int32_t i;

	if (!req || !(req->flags & REQ_RSP_DATA)) {
		return;
	}

	if (status & SDMMC_NISTER_SD_SDIO_BRDRDY_Msk) {
		if ((req->flags & REQ_IS_READ) && (sdmmc->SDMMC_PSR & SDMMC_PSR_BUFRDEN_Msk)) {
			for (i = 0; i < (req->block_size / 4); i++) {
				((uint32_t *)req->data)[i] = sdmmc->SDMMC_BDPR;
			}

			req->data += req->block_size;
			req->blocks--;
		}
	} else if (status & SDMMC_NISTER_SD_SDIO_BWRRDY_Msk) {
		if ((!(req->flags & REQ_IS_READ)) && (sdmmc->SDMMC_PSR & SDMMC_PSR_BUFWREN_Msk)) {
			for (i = 0; i < (req->block_size / 4); i++) {
				sdmmc->SDMMC_BDPR = ((uint32_t *)req->data)[i];
			}

			req->data += req->block_size;
			req->blocks--;
		}
	} else if (status & SDMMC_NISTER_SD_SDIO_DMAINT_Msk) {
		req->blocks = 0;
	} else {
		/* Nothing to do */
	}

	if (!req->blocks) {
		req->flags &= ~REQ_RSP_DATA;
	}
}

static void req_error_irq(sdmmc_registers_t *sdmmc, struct sd_request *req, uint32_t error)
{
	if (!req) {
		return;
	}

	if ((error & SDMMC_EISTR_SD_SDIO_ADMA_Msk) ||
	    (error & SDMMC_EISTR_SD_SDIO_ACMD_Msk)) {
		req->reset_mask |= SDMMC_SRR_SWRSTDAT_Msk |
				   SDMMC_SRR_SWRSTCMD_Msk;
	} else {
		if (error & CMD_ERROR_MASK) {
			req->reset_mask |= SDMMC_SRR_SWRSTCMD_Msk;
		}
		if (error & DATA_ERROR_MASK) {
			req->reset_mask |= SDMMC_SRR_SWRSTDAT_Msk;
		}
	}
}

static int sam_sdmmc_isr(const struct device *dev)
{
	const struct sam_sdmmc_config *config = dev->config;
	struct sam_sdmmc_data *data = dev->data;
	sdmmc_registers_t *sdmmc = config->base;
	struct sd_request *req = data->req;
	uint32_t status, error;
	int32_t max_loops = 16;

	status = sdmmc->SDMMC_NISTR;
	if (!status) {
		return 0;
	}

	do {
		LOG_DBG("    isr status = 0x%04x", status);
		sdmmc->SDMMC_NISTR = status;

		if (status & SDMMC_NISTR_SD_SDIO_ERRINT_Msk) {
			error = sdmmc->SDMMC_EISTR;
			sdmmc->SDMMC_EISTR = error;
			LOG_DBG("    isr error = 0x%04x", error);
		} else {
			error = 0;
		}

		if (error) {
			if (error & SDMMC_EISTR_SD_SDIO_CURLIM_Msk) {
				LOG_ERR("%s: Card is consuming too much power!",
					__func__);
			}

			if ((error & INT_CMD_ERROR_MASK) ||
			    (error & INT_DATA_ERROR_MASK)) {
				req_error_irq(sdmmc, req, error);
			}

			error &= ~INT_ERROR_MASK;
			if (error) {
				LOG_ERR("%s: Unexpected error interrupt 0x%04x",
					__func__, error);
			}
		}

		if (status) {
			if (status & SDMMC_NISTR_SD_SDIO_CREM_Msk) {
				LOG_DBG("%s: Card removal.", __func__);
			}

			if (status & SDMMC_NISTR_SD_SDIO_CINS_Msk) {
				LOG_DBG("%s: Card insertion.", __func__);
			}

			if (status & INT_CMD_MASK) {
				req_cmd_irq(sdmmc, req, status);
			}

			if (status & INT_DATA_MASK) {
				req_data_irq(sdmmc, req, status);
			}

			status &= ~INT_MASK;
			if (status) {
				LOG_ERR("%s: Unexpected interrupt 0x%04x",
					__func__, status);
			}

		}

		status = sdmmc->SDMMC_NISTR;
	} while (status && --max_loops);

	if ((req) && (!(req->flags & REQ_RSP_MASK) || req->reset_mask)) {
		k_sem_give(&req->completion);
	}

	return 0;
}

static int sdmmc_send_command(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *sd_data)
{
	const struct sam_sdmmc_config *config = dev->config;
	struct sam_sdmmc_data *data = dev->data;
	struct sdhc_host_props *props = data->props;
	sdmmc_registers_t *sdmmc = config->base;
	struct sd_request req;
	uint32_t command, mode = 0;
	uint32_t timeout;
	int32_t ret = 0;

	LOG_DBG("  cmd %d arg:0x%08x rsp:%d re:%d ms:%d data:%s %d %d %p",
		cmd->opcode,
		cmd->arg,
		cmd->response_type & 0xF,
		cmd->retries,
		cmd->timeout_ms,
		sd_data ? !IS_WRITE_CMD(cmd->opcode) ? "read" : "write" : "",
		sd_data ? sd_data->blocks : 0,
		sd_data ? sd_data->block_size : 0,
		sd_data ? sd_data->data : 0);

	memset(&req, 0, sizeof(req));
	if (k_sem_init(&req.completion, 0, 1)) {
		return -EINVAL;
	}
	req.response = cmd->response;
	data->req = &req;

	timeout = 1000;
	while ((sdmmc->SDMMC_PSR & (SDMMC_PSR_CMDINHC_Msk | SDMMC_PSR_CMDINHD_Msk)) && timeout--) {
		k_msleep(1);
	}

	if (sdmmc->SDMMC_PSR & (SDMMC_PSR_CMDINHC_Msk | SDMMC_PSR_CMDINHD_Msk)) {
		LOG_ERR("%s: timeout waiting for CMD and DAT Inhibit bits", __func__);
		sdmmc_reset(sdmmc, INH_TO_RST(sdmmc->SDMMC_PSR));

		return -EBUSY;
	}

	command = SDMMC_CR_CMDIDX(cmd->opcode);

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		command |= SDMMC_CR_CMDICEN_Msk | SDMMC_CR_CMDCCEN_Msk |
			   SDMMC_CR_RESPTYP_RL48;
		req.flags |= REQ_RSP_PRESENT;
		break;
	case SD_RSP_TYPE_R1b:
		command |= SDMMC_CR_CMDICEN_Msk | SDMMC_CR_CMDCCEN_Msk |
			   SDMMC_CR_RESPTYP_RL48BUSY;
		req.flags |= REQ_RSP_PRESENT | REQ_RSP_TRFC;
		break;
	case SD_RSP_TYPE_R2:
		command |= SDMMC_CR_CMDCCEN_Msk | SDMMC_CR_RESPTYP_RL136;
		req.flags |= REQ_RSP_PRESENT | REQ_RSP_136;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		command |= SDMMC_CR_RESPTYP_RL48;
		req.flags |= REQ_RSP_PRESENT;
		break;
	default:
		command |= SDMMC_CR_RESPTYP_NORESP;
	}

	if (sd_data) {
		req.flags |= REQ_RSP_DATA | REQ_RSP_TRFC;
		req.block_size = sd_data->block_size;
		req.blocks = sd_data->blocks;
		req.data = sd_data->data;
		if (!IS_WRITE_CMD(cmd->opcode)) {
			req.flags |= REQ_IS_READ;
		}

		command |= SDMMC_CR_DPSEL_1;
		mode |= !IS_WRITE_CMD(cmd->opcode) ? SDMMC_TMR_DTDSEL_READ : 0;
		sdmmc->SDMMC_BSR = SDMMC_BSR_BLKSIZE(sd_data->block_size);
		if (IS_MULTI_BLOCK(cmd->opcode)) {
			mode |= SDMMC_TMR_MSBSEL_Msk | SDMMC_TMR_BCEN_ENABLED;
			sdmmc->SDMMC_BCR = SDMMC_BCR_BLKCNT(sd_data->blocks);

			if ((config->auto_cmd12) || (config->auto_cmd23)) {
				if (config->auto_cmd12) {
					mode |= SDMMC_TMR_ACMDEN_CMD12;
				} else {
					mode |= SDMMC_TMR_ACMDEN_CMD23;
					sdmmc->SDMMC_SSAR = sd_data->blocks;
				}

				sdmmc_set_acmd_irqs(sdmmc, 1);
			}
		}

		/* DMA transfer */
		if (props->host_caps.adma_2_support &&
		    IS_DATA_CMD(cmd->opcode) && ADMA2_ALIGNED(sd_data->data)) {
			struct adma2_desc *desc = data->desc;
			int32_t len = sd_data->blocks * sd_data->block_size;
			uint32_t buf = (uint32_t)sd_data->data;
			uint32_t chunk;

			if (len > ADMA2_MAX_SIZE) {
				LOG_ERR("%s: data length exceeds ADMA link list", __func__);
				return -EINVAL;
			}

			if (IS_WRITE_CMD(cmd->opcode)) {
				sys_cache_data_flush_range(sd_data->data, len);
			} else {
				sys_cache_data_invd_range(sd_data->data, len);
			}

			while (len) {
				chunk = (len > ADMA2_MAX_LEN ? ADMA2_MAX_LEN : len);

				desc->cmd = sys_cpu_to_le16(ADMA2_TRAN_VALID);
				desc->len = sys_cpu_to_le16(chunk == ADMA2_MAX_LEN ? 0 : chunk);
				desc->addr = sys_cpu_to_le32(buf);

				buf += chunk;
				len -= chunk;
				if (!len) {
					desc->cmd |= sys_cpu_to_le16(ADMA2_INT | ADMA2_END);
				}

				LOG_DBG("  desc %p: cmd=0x%04x len=%d addr=%p",
					desc, desc->cmd, desc->len, (void *)desc->addr);

				desc++;
			}

			sys_cache_data_flush_range((void *)data->desc,
						   (size_t)desc - (size_t)data->desc);

			req.flags |= REQ_USE_ADMA;
			mode |= SDMMC_TMR_DMAEN_ENABLED;
			sdmmc->SDMMC_HC1R |= SDMMC_HC1R_SD_SDIO_DMASEL_ADMA32;
			sdmmc->SDMMC_ASAR0 = (uint32_t)data->desc;

			sdmmc_set_dma_irqs(sdmmc, 1);
		} else { /* PIO transfer */
			sdmmc_set_pio_irqs(sdmmc, 1);
		}
	}

	sdmmc->SDMMC_TMR   = mode;
	sdmmc->SDMMC_ARG1R = cmd->arg;
	sdmmc->SDMMC_CR    = command;

	ret = k_sem_take(&req.completion, K_MSEC(cmd->timeout_ms));
	if (!ret) {
		if (req.reset_mask) {
			sdmmc_reset(sdmmc, req.reset_mask);
			ret = -EIO;
		} else if ((sd_data) && (req.blocks)) {
			ret = -EIO;
		} else {
			/* Nothing to do */
		}
	} else {
		LOG_ERR("%s: error waiting for completion, return %d", __func__, ret);
	}

	LOG_DBG("    rsp 0x%08x 0x%08x 0x%08x 0x%08x %s", cmd->response[0], cmd->response[1],
		cmd->response[2], cmd->response[3], ret ? "Error" : "Ok");

	if (sd_data) {
		if (IS_MULTI_BLOCK(cmd->opcode) &&
		    ((config->auto_cmd12) || (config->auto_cmd23))) {
			sdmmc_set_acmd_irqs(sdmmc, 0);
		}

		if (req.flags & REQ_USE_ADMA) {
			sdmmc_set_dma_irqs(sdmmc, 0);

			if (!ret && IS_READ_CMD(cmd->opcode)) {
				sys_cache_data_invd_range((void *)sd_data->data,
					(size_t)(sd_data->blocks * sd_data->block_size));
			}
		} else {
			sdmmc_set_pio_irqs(sdmmc, 0);
		}
	}

	data->req = NULL;

	return ret;
}

static int sam_sdmmc_init(const struct device *dev)
{
	const struct sam_sdmmc_config *config = dev->config;
	struct sam_sdmmc_data *data = dev->data;
	int ret;

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("%s: pinctrl_apply_state() => %d", __func__, ret);
		return ret;
	}
	/* Enable module's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	k_mutex_init(&data->mutex);

	config->irq_config_func(dev);

	return sam_sdmmc_reset(dev);
}

static int sam_sdmmc_reset(const struct device *dev)
{
	const struct sam_sdmmc_config *config = dev->config;
	sdmmc_registers_t *sdmmc = config->base;
	int32_t ret;

	ret = sdmmc_reset(sdmmc, SDMMC_SRR_SWRSTALL_Msk);

	sdmmc_set_bus_power(sdmmc, 1);
	sdmmc_set_default_irqs(sdmmc);
	if (config->non_removable) {
		sdmmc_set_force_card_detect(sdmmc);
	}
	/* Set max data timeout */
	sdmmc->SDMMC_TCR = SDMMC_TCR_DTCVAL(MAX_DATA_TIMEOUT);

	return ret;
}

static int sam_sdmmc_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *sd_data)
{
	struct sam_sdmmc_data *data = dev->data;
	uint32_t retries = cmd->retries + 1;
	int32_t ret;

	if (k_mutex_lock(&data->mutex, K_MSEC(cmd->timeout_ms))) {
		return -EBUSY;
	}

	do {
		ret = sdmmc_send_command(dev, cmd, sd_data);
	} while (ret && --retries);

	k_mutex_unlock(&data->mutex);

	return ret;
}

static int sam_sdmmc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sam_sdmmc_config *config = dev->config;
	struct sam_sdmmc_data *data = dev->data;
	struct sdhc_io *io_cfg = &data->io_cfg;
	sdmmc_registers_t *sdmmc = config->base;
	uint32_t ret;

	if (ios->clock != io_cfg->clock) {
		ret = sdmmc_set_clock(sdmmc, config->base_clk, ios->clock);
		if (ret) {
			return ret;
		}

		if (ios->timing == SDHC_TIMING_LEGACY) {
			sdmmc_high_speed(sdmmc, 0);
		} else {
			sdmmc_high_speed(sdmmc, 1);
		}
		io_cfg->timing = ios->timing;

		io_cfg->clock = ios->clock;
	}

	if (ios->bus_mode != io_cfg->bus_mode) {
		switch (ios->bus_mode) {
		case SDHC_BUSMODE_OPENDRAIN:
			sdmmc_cmd_line_mode(sdmmc, 1);
			break;
		case SDHC_BUSMODE_PUSHPULL:
			sdmmc_cmd_line_mode(sdmmc, 0);
			break;
		default:
			return -EINVAL;
		}

		io_cfg->bus_mode = ios->bus_mode;
	}

	if (ios->power_mode != io_cfg->power_mode) {
		switch (ios->power_mode) {
		case SDHC_POWER_OFF:
			sdmmc_set_bus_power(sdmmc, 0);

			if (config->rstn_power_en) {
				sdmmc_set_rstn(sdmmc, 1);
			}
			break;
		case SDHC_POWER_ON:
			if (config->rstn_power_en) {
				sdmmc_set_rstn(sdmmc, 0);
			}

			sdmmc_set_bus_power(sdmmc, 1);
			break;
		default:
			return -EINVAL;
		}

		io_cfg->power_mode = ios->power_mode;
	}

	if (ios->bus_width != io_cfg->bus_width) {
		if ((config->bus_width) &&
		    (ios->bus_width > config->bus_width)) {
			return -ENOTSUP;
		}

		ret = sdmmc_bus_width(sdmmc, ios->bus_width);
		if (ret) {
			return ret;
		}

		io_cfg->bus_width = ios->bus_width;
	}

	if (ios->timing != io_cfg->timing) {
		switch (ios->timing) {
		case SDHC_TIMING_LEGACY:
			sdmmc_high_speed(sdmmc, 0);
			break;
		case SDHC_TIMING_HS:
			sdmmc_high_speed(sdmmc, 1);
			break;
		default:
			/* UHS-I mode is TBD */
			return -ENOTSUP;
		}

		io_cfg->timing = ios->timing;
	}

	if (ios->driver_type != io_cfg->driver_type) {
		switch (ios->driver_type) {
		case SD_DRIVER_TYPE_B:
			sdmmc_dirver_type(sdmmc, SDMMC_HC2R_SD_SDIO_DRVSEL_TYPEB_Val);
			break;
		case SD_DRIVER_TYPE_A:
			sdmmc_dirver_type(sdmmc, SDMMC_HC2R_SD_SDIO_DRVSEL_TYPEA_Val);
			break;
		case SD_DRIVER_TYPE_C:
			sdmmc_dirver_type(sdmmc, SDMMC_HC2R_SD_SDIO_DRVSEL_TYPEC_Val);
			break;
		case SD_DRIVER_TYPE_D:
			sdmmc_dirver_type(sdmmc, SDMMC_HC2R_SD_SDIO_DRVSEL_TYPED_Val);
			break;
		default:
			return -ENOTSUP;
		}

		io_cfg->driver_type = ios->driver_type;
	}

	if (ios->signal_voltage != io_cfg->signal_voltage) {
		switch (ios->signal_voltage) {
		case SD_VOL_3_3_V:
			sdmmc_set_1v8(sdmmc, 0);
			break;
		case SD_VOL_1_8_V:
			sdmmc_set_1v8(sdmmc, 1);
			break;
		default:
			return -ENOTSUP;
		}

		io_cfg->signal_voltage = ios->signal_voltage;
	}

	return 0;
}

static int sam_sdmmc_get_card_present(const struct device *dev)
{
	const struct sam_sdmmc_config *config = dev->config;
	sdmmc_registers_t *sdmmc = config->base;

	if (config->non_removable) {
		return 1;
	}

	return sdmmc_card_present(sdmmc);
}

static int sam_sdmmc_card_busy(const struct device *dev)
{
	const struct sam_sdmmc_config *config = dev->config;
	sdmmc_registers_t *sdmmc = config->base;

	return sdmmc_card_busy(sdmmc);
}

static int sam_sdmmc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sam_sdmmc_config *config = dev->config;
	struct sam_sdmmc_data *data = dev->data;
	sdmmc_registers_t *sdmmc = config->base;
	uint32_t cap0 = sdmmc->SDMMC_CA0R;
	uint32_t cap1 = sdmmc->SDMMC_CA1R;

	memset(props, 0, sizeof(*props));
	/* Save properties structure pointer */
	data->props = props;

	props->f_max = config->max_bus_freq;
	props->f_min = config->min_bus_freq;
	props->power_delay = config->power_delay_ms;

	props->host_caps.timeout_clk_freq = (cap0 & SDMMC_CA0R_TEOCLKF_Msk) >>
					    SDMMC_CA0R_TEOCLKF_Pos;
	props->host_caps.timeout_clk_unit = (cap0 & SDMMC_CA0R_TEOCLKU_Msk) >>
					    SDMMC_CA0R_TEOCLKU_Pos;
	props->host_caps.sd_base_clk      = config->base_clk / 1000000;
	props->host_caps.max_blk_len      = (cap0 & SDMMC_CA0R_MAXBLKL_Msk) >>
					    SDMMC_CA0R_MAXBLKL_Pos;
	if ((config->bus_width == 8) && (cap0 & SDMMC_CA0R_ED8SUP_Msk)) {
		props->host_caps.bus_8_bit_support = true;
	}
	if (config->bus_width != 1) {
		props->host_caps.bus_4_bit_support = true;
	}
	props->host_caps.adma_2_support      = (bool)(cap0 & SDMMC_CA0R_ADMA2SUP_Msk);
	props->host_caps.high_spd_support    = (bool)(cap0 & SDMMC_CA0R_HSSUP_Msk);
	props->host_caps.sdma_support        = (bool)(cap0 & SDMMC_CA0R_SDMASUP_Msk);
	props->host_caps.suspend_res_support = (bool)(cap0 & SDMMC_CA0R_SRSUP_Msk);
	props->host_caps.vol_330_support     = (bool)(cap0 & SDMMC_CA0R_V33VSUP_Msk);
	props->host_caps.vol_300_support     = false;
	if (!config->no_18v) {
		props->host_caps.vol_180_support = (bool)(cap0 & SDMMC_CA0R_V18VSUP_Msk);
	}
	props->host_caps.address_64_bit_support_v4    = false;
	props->host_caps.address_64_bit_support_v3    = (bool)(cap0 & SDMMC_CA0R_SB64SUP_Msk);
	props->host_caps.sdio_async_interrupt_support = (bool)(cap0 & SDMMC_CA0R_ASINTSUP_Msk);
	props->host_caps.slot_type                    = (cap0 & SDMMC_CA0R_SLTYPE_Msk) >>
							SDMMC_CA0R_SLTYPE_Pos;
	if (!config->no_18v) {
		props->host_caps.sdr50_support      = (bool)(cap1 & SDMMC_CA1R_SDR50SUP_Msk);
		props->host_caps.sdr104_support     = (bool)(cap1 & SDMMC_CA1R_SDR104SUP_Msk);
		props->host_caps.ddr50_support      = (bool)(cap1 & SDMMC_CA1R_DDR50SUP_Msk);
		props->host_caps.uhs_2_support      = false;
		props->host_caps.drv_type_a_support = (bool)(cap1 & SDMMC_CA1R_DRVASUP_Msk);
		props->host_caps.drv_type_c_support = (bool)(cap1 & SDMMC_CA1R_DRVCSUP_Msk);
		props->host_caps.drv_type_d_support = (bool)(cap1 & SDMMC_CA1R_DRVDSUP_Msk);
		props->host_caps.retune_timer_count = (cap1 & SDMMC_CA1R_TCNTRT_Msk) >>
						      SDMMC_CA1R_TCNTRT_Pos;
		props->host_caps.sdr50_needs_tuning = (bool)(cap1 & SDMMC_CA1R_TSDR50_Msk);
		props->host_caps.retuning_mode      = (cap1 & SDMMC_CA1R_RTMOD_Msk) >>
						      SDMMC_CA1R_RTMOD_Pos;
	}
	props->host_caps.clk_multiplier   = (cap1 & SDMMC_CA1R_CLKMULT_Msk) >>
					    SDMMC_CA1R_CLKMULT_Pos;
	props->host_caps.adma3_support    = false;
	props->host_caps.vdd2_180_support = false;
	if (!config->no_18v) {
		if (config->mmc_hs400_18v || config->mmc_hs200_18v) {
			props->host_caps.hs200_support = true;
		}
		if (config->mmc_hs400_18v) {
			props->host_caps.hs400_support = true;
		}
	}

	if (config->max_current_330) {
		props->max_current_330 = config->max_current_330;
	} else {
		props->max_current_330 = 200;
	}
	if (config->max_current_180) {
		props->max_current_180 = config->max_current_180;
	} else {
		props->max_current_180 = 200;
	}

	props->is_spi = false;

	return 0;
}

static DEVICE_API(sdhc, sdmmc_api) = {
	.reset = sam_sdmmc_reset,
	.request = sam_sdmmc_request,
	.set_io = sam_sdmmc_set_io,
	.get_card_present = sam_sdmmc_get_card_present,
	.card_busy = sam_sdmmc_card_busy,
	.get_host_props = sam_sdmmc_get_host_props,
};

#define SAM_SDMMC_INIT(N)									\
	static void sdmmc_##N##_irq_config_func(const struct device *dev);			\
												\
	PINCTRL_DT_INST_DEFINE(N);								\
												\
	static const struct sam_sdmmc_config sdmmc_##N##_config = {				\
		.base            = (sdmmc_registers_t *)DT_INST_REG_ADDR(N),			\
		.pincfg          = PINCTRL_DT_INST_DEV_CONFIG_GET(N),				\
		.clock_cfg       = SAM_DT_INST_CLOCK_PMC_CFG(N),				\
		.base_clk        = DT_INST_PROP(N, assigned_clock_rates) / 2,			\
		.non_removable   = DT_INST_PROP(N, non_removable),				\
		.bus_width       = DT_INST_PROP(N, bus_width),					\
		.no_18v          = DT_INST_PROP(N, no_1_8_v),					\
		.rstn_power_en   = DT_INST_PROP(N, rstn_power_en),				\
		.auto_cmd12      = DT_INST_PROP(N, auto_cmd12),					\
		.auto_cmd23      = DT_INST_PROP(N, auto_cmd23),					\
		.mmc_hs200_18v   = DT_INST_PROP(N, mmc_hs200_1_8v),				\
		.mmc_hs400_18v   = DT_INST_PROP(N, mmc_hs400_1_8v),				\
		.max_bus_freq    = DT_INST_PROP(N, max_bus_freq),				\
		.min_bus_freq    = DT_INST_PROP(N, min_bus_freq),				\
		.power_delay_ms  = DT_INST_PROP(N, power_delay_ms),				\
		.max_current_330 = DT_INST_PROP(N, max_current_330),				\
		.max_current_180 = DT_INST_PROP(N, max_current_180),				\
		.irq_config_func = sdmmc_##N##_irq_config_func					\
	};											\
												\
	static struct sam_sdmmc_data sdmmc_##N##_data = {};					\
												\
	DEVICE_DT_INST_DEFINE(N, &sam_sdmmc_init, NULL, &sdmmc_##N##_data, &sdmmc_##N##_config,	\
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &sdmmc_api);		\
												\
	static void sdmmc_##N##_irq_config_func(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(N), DT_INST_IRQ(N, priority),				\
			sam_sdmmc_isr, DEVICE_DT_INST_GET(N), 0);				\
		irq_enable(DT_INST_IRQN(N));							\
	}											\

DT_INST_FOREACH_STATUS_OKAY(SAM_SDMMC_INIT)

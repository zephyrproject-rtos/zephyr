/*
 * Copyright 2023 Nikhef
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_hsmci

#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>

LOG_MODULE_REGISTER(hsmci, CONFIG_SDHC_LOG_LEVEL);

#ifdef HSMCI_MR_PDCMODE
#ifdef CONFIG_SAM_HSMCI_PDCMODE
#define _HSMCI_PDCMODE
#endif
#endif

#ifdef CONFIG_SAM_HSMCI_PWRSAVE
#if (CONFIG_SAM_HSMCI_PWRSAVE_DIV < 0) || (CONFIG_SAM_HSMCI_PWRSAVE_DIV > 7)
#error "CONFIG_SAM_HSMCI_PWRSAVE_DIV must be 0 to 7"
#endif
#endif

#define _HSMCI_DEFAULT_TIMEOUT	5000
#define _HSMCI_MAX_FREQ			(SOC_ATMEL_SAM_MCK_FREQ_HZ >> 1)
#define _HSMCI_MIN_FREQ			(_HSMCI_MAX_FREQ / 0x200)
#define _MSMCI_MAX_DIVISOR		0x1FF
#define _HSMCI_SR_ERR			 (HSMCI_SR_RINDE \
								| HSMCI_SR_RDIRE \
								| HSMCI_SR_RCRCE \
								| HSMCI_SR_RENDE \
								| HSMCI_SR_RTOE \
								| HSMCI_SR_DCRCE \
								| HSMCI_SR_DTOE \
								| HSMCI_SR_CSTOE \
								| HSMCI_SR_OVRE \
								| HSMCI_SR_UNRE)

static const uint8_t _resp2size[] = {
	[SD_RSP_TYPE_NONE] = HSMCI_CMDR_RSPTYP_NORESP,
	[SD_RSP_TYPE_R1]   = HSMCI_CMDR_RSPTYP_48_BIT,
	[SD_RSP_TYPE_R1b]  = HSMCI_CMDR_RSPTYP_R1B,
	[SD_RSP_TYPE_R2]   = HSMCI_CMDR_RSPTYP_136_BIT,
	[SD_RSP_TYPE_R3]   = HSMCI_CMDR_RSPTYP_48_BIT,
	[SD_RSP_TYPE_R4]   = HSMCI_CMDR_RSPTYP_48_BIT,
	[SD_RSP_TYPE_R5]   = 0 /* SDIO not supported */,
	[SD_RSP_TYPE_R5b]  = 0 /* SDIO not supported */,
	[SD_RSP_TYPE_R6]   = HSMCI_CMDR_RSPTYP_48_BIT,
	[SD_RSP_TYPE_R7]   = HSMCI_CMDR_RSPTYP_48_BIT,
};

/* timeout multiplier shift (actual value is 1 << _mul_shift[*]) */
static const uint8_t _mul_shift[] = {0, 4, 7, 8, 10, 12, 16, 20};
static const uint8_t _mul_shift_size = 8;

struct sam_hsmci_config {
	Hsmci *base;
	const struct atmel_sam_pmc_config clock_cfg;
	const struct pinctrl_dev_config *pincfg;
	struct gpio_dt_spec carrier_detect;
};

struct sam_hsmci_data {
	bool open_drain;
	uint8_t cmd_in_progress;
	struct k_mutex mtx;
};

static int sam_hsmci_reset(const struct device *dev)
{
	const struct sam_hsmci_config *config = dev->config;
	Hsmci *hsmci = config->base;

	uint32_t mr = hsmci->HSMCI_MR;
	uint32_t dtor = hsmci->HSMCI_DTOR;
	uint32_t sdcr = hsmci->HSMCI_SDCR;
	uint32_t cstor = hsmci->HSMCI_CSTOR;
	uint32_t cfg = hsmci->HSMCI_CFG;

	hsmci->HSMCI_CR = HSMCI_CR_SWRST;
	hsmci->HSMCI_MR = mr;
	hsmci->HSMCI_DTOR = dtor;
	hsmci->HSMCI_SDCR = sdcr;
	hsmci->HSMCI_CSTOR = cstor;
	hsmci->HSMCI_CFG = cfg;

	hsmci->HSMCI_CR = HSMCI_CR_PWSEN | HSMCI_CR_MCIEN;
	return 0;
}

static int sam_hsmci_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	memset(props, 0, sizeof(*props));

	props->f_max = _HSMCI_MAX_FREQ;
	props->f_min = _HSMCI_MIN_FREQ;
	/* high-speed not working yet due to limitations of the SDHC sm */
	props->host_caps.high_spd_support = false;
	props->power_delay = 500;
	props->is_spi = false;
	props->max_current_330 = 4;

	return 0;
}

static int sam_hsmci_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sam_hsmci_config *config = dev->config;
	struct sam_hsmci_data *data = dev->data;
	Hsmci *hsmci = config->base;
	uint32_t frequency;
	uint32_t div_val;
	int ret;

	LOG_DBG("%s(clock=%d, bus_width=%d, timing=%d, mode=%d)", __func__, ios->clock,
		ios->bus_width, ios->timing, ios->bus_mode);

	if (ios->clock > 0) {
		if (ios->clock > _HSMCI_MAX_FREQ) {
			return -ENOTSUP;
		}

		ret = clock_control_get_rate(SAM_DT_PMC_CONTROLLER,
					     (clock_control_subsys_t)&config->clock_cfg,
					     &frequency);

		if (ret < 0) {
			LOG_ERR("Failed to get clock rate, err=%d", ret);
			return ret;
		}

		div_val = frequency / ios->clock - 2;

		if (div_val < 0) {
			div_val = 0;
		}

		if (div_val > _MSMCI_MAX_DIVISOR) {
			div_val = _MSMCI_MAX_DIVISOR;
		}

		LOG_DBG("divider: %d (freq=%d)", div_val, frequency / (div_val + 2));

		hsmci->HSMCI_MR &= ~HSMCI_MR_CLKDIV_Msk;
		hsmci->HSMCI_MR |=
			((div_val & 1) ? HSMCI_MR_CLKODD : 0) | HSMCI_MR_CLKDIV(div_val >> 1);
	}

	if (ios->bus_width)	{
		hsmci->HSMCI_SDCR &= ~HSMCI_SDCR_SDCBUS_Msk;

		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			hsmci->HSMCI_SDCR = HSMCI_SDCR_SDCBUS_1;
			break;
		case SDHC_BUS_WIDTH4BIT:
			hsmci->HSMCI_SDCR = HSMCI_SDCR_SDCBUS_4;
			break;
		default:
			return -ENOTSUP;
		}
	}

	data->open_drain = (ios->bus_mode == SDHC_BUSMODE_OPENDRAIN);

	if (ios->timing) {
		switch (ios->timing) {
		case SDHC_TIMING_LEGACY:
			hsmci->HSMCI_CFG &= ~HSMCI_CFG_HSMODE;
			break;
		case SDHC_TIMING_HS:
			hsmci->HSMCI_CFG |= HSMCI_CFG_HSMODE;
			break;
		default:
			return -ENOTSUP;
		}
	}

	return 0;
}

static int sam_hsmci_init(const struct device *dev)
{
	const struct sam_hsmci_config *config = dev->config;
	int ret;

	/* Connect pins to the peripheral */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("pinctrl_apply_state() => %d", ret);
		return ret;
	}
	/* Enable module's clock */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER, (clock_control_subsys_t)&config->clock_cfg);

	/* init carrier detect (if set) */
	if (config->carrier_detect.port != NULL) {
		if (!gpio_is_ready_dt(&config->carrier_detect)) {
			LOG_ERR("GPIO port for carrier-detect pin is not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->carrier_detect, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Couldn't configure carrier-detect pin; (%d)", ret);
			return ret;
		}
	}

	Hsmci *hsmci = config->base;

	/* reset the device */
	hsmci->HSMCI_CR = HSMCI_CR_SWRST;
	hsmci->HSMCI_CR = HSMCI_CR_PWSDIS;
	hsmci->HSMCI_CR = HSMCI_CR_MCIEN;
#ifdef CONFIG_SAM_HSMCI_PWRSAVE
	hsmci->HSMCI_MR =
		HSMCI_MR_RDPROOF | HSMCI_MR_WRPROOF | HSMCI_MR_PWSDIV(CONFIG_SAM_HSMCI_PWRSAVE_DIV);
	hsmci->HSMCI_CR = HSMCI_CR_PWSEN;
#else
	hsmci->HSMCI_MR = HSMCI_MR_RDPROOF | HSMCI_MR_WRPROOF;
#endif

	return 0;
}

static int sam_hsmci_get_card_present(const struct device *dev)
{
	const struct sam_hsmci_config *config = dev->config;

	if (config->carrier_detect.port == NULL) {
		return 1;
	}

	return gpio_pin_get_dt(&config->carrier_detect);
}

static int sam_hsmci_card_busy(const struct device *dev)
{
	const struct sam_hsmci_config *config = dev->config;
	Hsmci *hsmci = config->base;

	return (hsmci->HSMCI_SR & HSMCI_SR_NOTBUSY) == 0;
}

static void sam_hsmci_send_clocks(Hsmci *hsmci)
{
	hsmci->HSMCI_MR &= ~(HSMCI_MR_WRPROOF | HSMCI_MR_RDPROOF | HSMCI_MR_FBYTE);
	hsmci->HSMCI_ARGR = 0;
	hsmci->HSMCI_CMDR =
		HSMCI_CMDR_RSPTYP_NORESP | HSMCI_CMDR_SPCMD_INIT | HSMCI_CMDR_OPDCMD_OPENDRAIN;
	while (!(hsmci->HSMCI_SR & HSMCI_SR_CMDRDY)) {
		;
	}
	hsmci->HSMCI_MR |= HSMCI_MR_WRPROOF | HSMCI_MR_RDPROOF;
}

static int sam_hsmci_send_cmd(Hsmci *hsmci, struct sdhc_command *cmd, uint32_t cmdr,
			      struct sam_hsmci_data *data)
{
	uint32_t sr;

	hsmci->HSMCI_ARGR = cmd->arg;

	cmdr |= HSMCI_CMDR_CMDNB(cmd->opcode) | HSMCI_CMDR_MAXLAT_64;
	if (data->open_drain) {
		cmdr |= HSMCI_CMDR_OPDCMD_OPENDRAIN;
	}

	uint8_t nrt = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;

	if (nrt > SD_RSP_TYPE_R7) {
		return -ENOTSUP;
	}

	cmdr |= _resp2size[nrt];
	hsmci->HSMCI_CMDR = cmdr;
	do {
		sr = hsmci->HSMCI_SR;

		/* special case ,ignore CRC status if response is R3 to clear it */
		if (nrt == SD_RSP_TYPE_R3 || nrt == SD_RSP_TYPE_NONE) {
			sr &= ~HSMCI_SR_RCRCE;
		}

		if ((sr & _HSMCI_SR_ERR) != 0) {
			LOG_DBG("Status register error bits: %08x", sr & _HSMCI_SR_ERR);
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_CMDRDY));

	if (nrt == SD_RSP_TYPE_R1b) {
		do {
			sr = hsmci->HSMCI_SR;
		} while (!((sr & HSMCI_SR_NOTBUSY) && ((sr & HSMCI_SR_DTIP) == 0)));
	}

	/* RSPR is just a FIFO, index is of no consequence */
	cmd->response[3] = hsmci->HSMCI_RSPR[0];
	cmd->response[2] = hsmci->HSMCI_RSPR[0];
	cmd->response[1] = hsmci->HSMCI_RSPR[0];
	cmd->response[0] = hsmci->HSMCI_RSPR[0];
	return 0;
}

static int sam_hsmci_wait_write_end(Hsmci *hsmci)
{
	uint32_t sr = 0;

#ifdef _HSMCI_PDCMODE
	/* Timeout is included in HSMCI, see DTOE bit, not required explicitly. */
	do {
		sr = hsmci->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			LOG_DBG("PDC sr 0x%08x error", sr);
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_TXBUFE));
#endif

	do {
		sr = hsmci->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			LOG_DBG("PDC sr 0x%08x last transfer error", sr);
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_NOTBUSY));

	if (!(hsmci->HSMCI_SR & HSMCI_SR_FIFOEMPTY)) {
		return -EIO;
	}
	return 0;
}

static int sam_hsmci_wait_read_end(Hsmci *hsmci)
{
	uint32_t sr;

#ifdef _HSMCI_PDCMODE
	do {
		sr = hsmci->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			LOG_DBG("PDC sr 0x%08x error", sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE |
							     HSMCI_SR_DTOE | HSMCI_SR_DCRCE));
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_RXBUFF));
#endif

	do {
		sr = hsmci->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_XFRDONE));
	return 0;
}

static int sam_hsmci_write_timeout(Hsmci *hsmci, int timeout_ms)
{
	/* convert to clocks (coarsely) */
	int clocks = ATMEL_SAM_DT_CPU_CLK_FREQ_HZ / 1000 * timeout_ms;
	int mul, max_clock;

	for (int i = 0; i < _mul_shift_size; i++) {
		mul = 1 << _mul_shift[i];
		max_clock = 15 * mul;
		if (max_clock > clocks) {
			hsmci->HSMCI_DTOR = ((i << HSMCI_DTOR_DTOMUL_Pos) & HSMCI_DTOR_DTOMUL_Msk) |
					    HSMCI_DTOR_DTOCYC((clocks + mul - 1) / mul);
			return 0;
		}
	}
	/*
	 * So, if it is > maximum timeout... we'll just put it on the maximum the driver supports
	 * its not nice.. but it should work.. what else is there to do?
	 */
	hsmci->HSMCI_DTOR = HSMCI_DTOR_DTOMUL_Msk | HSMCI_DTOR_DTOCYC_Msk;
	return 0;
}

static inline int wait_write_transfer_done(Hsmci *hsmci)
{
	int sr;

	do {
		sr = hsmci->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_TXRDY));
	return 0;
}

static inline int wait_read_transfer_done(Hsmci *hsmci)
{
	int sr;

	do {
		sr = HSMCI->HSMCI_SR;
		if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
			return -EIO;
		}
	} while (!(sr & HSMCI_SR_RXRDY));
	return 0;
}

#ifndef _HSMCI_PDCMODE

static int hsmci_do_manual_transfer(Hsmci *hsmci, bool byte_mode, bool is_write, void *data,
				    int transfer_count)
{
	int ret;

	if (is_write) {
		if (byte_mode) {
			const uint8_t *ptr = data;

			while (transfer_count-- > 0) {
				ret = wait_write_transfer_done(hsmci);
				if (ret != 0) {
					return ret;
				}
				hsmci->HSMCI_TDR = *ptr;
				ptr++;
			}
		} else {
			const uint32_t *ptr = data;

			while (transfer_count-- > 0) {
				ret = wait_write_transfer_done(hsmci);
				if (ret != 0) {
					return ret;
				}
				hsmci->HSMCI_TDR = *ptr;
				ptr++;
			}
		}
		ret = sam_hsmci_wait_write_end(hsmci);
	} else {
		if (byte_mode) {
			uint8_t *ptr = data;

			while (transfer_count-- > 0) {
				ret = wait_read_transfer_done(hsmci);
				if (ret != 0) {
					return ret;
				}
				*ptr = hsmci->HSMCI_RDR;
				ptr++;
			}
		} else {
			uint32_t *ptr = data;

			while (transfer_count-- > 0) {
				ret = wait_read_transfer_done(hsmci);
				if (ret != 0) {
					return ret;
				}
				*ptr = hsmci->HSMCI_RDR;
				ptr++;
			}
		}
		ret = sam_hsmci_wait_read_end(hsmci);
	}
	return ret;
}

#endif /* !_HSMCI_PDCMODE */

static int sam_hsmci_request_inner(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *sd_data)
{
	const struct sam_hsmci_config *config = dev->config;
	struct sam_hsmci_data *data = dev->data;
	Hsmci *hsmci = config->base;
	uint32_t sr;
	uint32_t size;
	uint32_t transfer_count;
	uint32_t cmdr = 0;
	int ret;
	bool is_write, byte_mode;

	LOG_DBG("%s(opcode=%d, arg=%08x, data=%08x, rsptype=%d)", __func__, cmd->opcode, cmd->arg,
		(uint32_t)sd_data, cmd->response_type & SDHC_NATIVE_RESPONSE_MASK);

	if (cmd->opcode == SD_GO_IDLE_STATE) {
		/* send 74 clocks, as required by SD spec */
		sam_hsmci_send_clocks(hsmci);
	}

	if (sd_data) {
		cmdr |= HSMCI_CMDR_TRCMD_START_DATA;

		ret = sam_hsmci_write_timeout(hsmci, cmd->timeout_ms);
		if (ret != 0) {
			return ret;
		}

		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
			cmdr |= HSMCI_CMDR_TRTYP_SINGLE;
			cmdr |= HSMCI_CMDR_TRDIR_WRITE;
			is_write = true;
			break;
		case SD_WRITE_MULTIPLE_BLOCK:
			is_write = true;
			cmdr |= HSMCI_CMDR_TRTYP_MULTIPLE;
			cmdr |= HSMCI_CMDR_TRDIR_WRITE;
			break;
		case SD_APP_SEND_SCR:
		case SD_SWITCH:
		case SD_READ_SINGLE_BLOCK:
			is_write = false;
			cmdr |= HSMCI_CMDR_TRTYP_SINGLE;
			cmdr |= HSMCI_CMDR_TRDIR_READ;
			break;
		case SD_READ_MULTIPLE_BLOCK:
			is_write = false;
			cmdr |= HSMCI_CMDR_TRTYP_MULTIPLE;
			cmdr |= HSMCI_CMDR_TRDIR_READ;
			break;
		case SD_APP_SEND_NUM_WRITTEN_BLK:
			is_write = false;
			break;
		default:
			return -ENOTSUP;
		}

		if ((sd_data->block_size & 0x3) == 0 && (((uint32_t)sd_data->data) & 0x3) == 0) {
			size = (sd_data->block_size + 3) >> 2;
			hsmci->HSMCI_MR &= ~HSMCI_MR_FBYTE;
			byte_mode = true;
		} else {
			size = sd_data->block_size;
			hsmci->HSMCI_MR |= HSMCI_MR_FBYTE;
			byte_mode = false;
		}

		hsmci->HSMCI_BLKR =
			HSMCI_BLKR_BLKLEN(sd_data->block_size) | HSMCI_BLKR_BCNT(sd_data->blocks);

		transfer_count = size * sd_data->blocks;

#ifdef _HSMCI_PDCMODE
		hsmci->HSMCI_MR |= HSMCI_MR_PDCMODE;

		hsmci->HSMCI_RNCR = 0;

		if (is_write) {
			hsmci->HSMCI_TCR = transfer_count;
			hsmci->HSMCI_TPR = (uint32_t)sd_data->data;
		} else {
			hsmci->HSMCI_RCR = transfer_count;
			hsmci->HSMCI_RPR = (uint32_t)sd_data->data;
			hsmci->HSMCI_PTCR = HSMCI_PTCR_RXTEN;
		}

	} else {
		hsmci->HSMCI_MR &= ~HSMCI_MR_PDCMODE;
#endif /* _HSMCI_PDCMODE */
	}

	ret = sam_hsmci_send_cmd(hsmci, cmd, cmdr, data);

	if (sd_data) {
#ifdef _HSMCI_PDCMODE
		if (ret == 0) {
			if (is_write) {
				hsmci->HSMCI_PTCR = HSMCI_PTCR_TXTEN;
				ret = sam_hsmci_wait_write_end(hsmci);
			} else {
				ret = sam_hsmci_wait_read_end(hsmci);
			}
		}
		hsmci->HSMCI_PTCR = HSMCI_PTCR_TXTDIS | HSMCI_PTCR_RXTDIS;
		hsmci->HSMCI_MR &= ~HSMCI_MR_PDCMODE;
#else  /* !_HSMCI_PDCMODE */
		if (ret == 0) {
			ret = hsmci_do_manual_transfer(hsmci, byte_mode, is_write, sd_data->data,
						       transfer_count);
		}
#endif /* _HSMCI_PDCMODE */
	}

	sr = hsmci->HSMCI_SR;

	LOG_DBG("RSP0=%08x, RPS1=%08x, RPS2=%08x,RSP3=%08x, SR=%08x", cmd->response[0],
		cmd->response[1], cmd->response[2], cmd->response[3], sr);

	return ret;
}

static void sam_hsmci_abort(const struct device *dev)
{
#ifdef _HSMCI_PDCMODE
	const struct sam_hsmci_config *config = dev->config;
	Hsmci *hsmci = config->base;

	hsmci->HSMCI_PTCR = HSMCI_PTCR_RXTDIS | HSMCI_PTCR_TXTDIS;
#endif /* _HSMCI_PDCMODE */

	struct sdhc_command cmd = {
		.opcode = SD_STOP_TRANSMISSION, .arg = 0, .response_type = SD_RSP_TYPE_NONE};
	sam_hsmci_request_inner(dev, &cmd, NULL);
}

static int sam_hsmci_request(const struct device *dev, struct sdhc_command *cmd,
			     struct sdhc_data *sd_data)
{
	struct sam_hsmci_data *dev_data = dev->data;
	int busy_timeout = _HSMCI_DEFAULT_TIMEOUT;
	int ret;

	ret = k_mutex_lock(&dev_data->mtx, K_MSEC(cmd->timeout_ms));
	if (ret) {
		LOG_ERR("Could not access card");
		return -EBUSY;
	}

#ifdef CONFIG_SAM_HSMCI_PWRSAVE
	const struct sam_hsmci_config *config = dev->config;
	Hsmci *hsmci = config->base;

	hsmci->HSMCI_CR = HSMCI_CR_PWSDIS;
#endif /* CONFIG_SAM_HSMCI_PWRSAVE */

	do {
		ret = sam_hsmci_request_inner(dev, cmd, sd_data);
		if (sd_data && (ret || sd_data->blocks > 1)) {
			sam_hsmci_abort(dev);
			while (busy_timeout > 0) {
				if (!sam_hsmci_card_busy(dev)) {
					break;
				}
				k_busy_wait(125);
				busy_timeout -= 125;
			}
			if (busy_timeout <= 0) {
				LOG_ERR("Card did not idle after CMD12");
				ret = -ETIMEDOUT;
			}
		}
	} while (ret != 0 && (cmd->retries-- > 0));

#ifdef CONFIG_SAM_HSMCI_PWRSAVE
	hsmci->HSMCI_CR = HSMCI_CR_PWSEN;
#endif /* CONFIG_SAM_HSMCI_PWRSAVE */

	k_mutex_unlock(&dev_data->mtx);

	return ret;
}

static const struct sdhc_driver_api hsmci_api = {
	.reset = sam_hsmci_reset,
	.get_host_props = sam_hsmci_get_host_props,
	.set_io = sam_hsmci_set_io,
	.get_card_present = sam_hsmci_get_card_present,
	.request = sam_hsmci_request,
	.card_busy = sam_hsmci_card_busy,
};

#define SAM_HSMCI_INIT(N)                                                                          \
	PINCTRL_DT_INST_DEFINE(N);                                                                 \
	static const struct sam_hsmci_config hsmci_##N##_config = {                                \
		.base = (Hsmci *)DT_INST_REG_ADDR(N),                                              \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(N),                                       \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(N),                                         \
		.carrier_detect = GPIO_DT_SPEC_INST_GET_OR(N, cd_gpios, {0})};                     \
	static struct sam_hsmci_data hsmci_##N##_data = {};                                        \
	DEVICE_DT_INST_DEFINE(N, &sam_hsmci_init, NULL, &hsmci_##N##_data, &hsmci_##N##_config,    \
			      POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, &hsmci_api);

DT_INST_FOREACH_STATUS_OKAY(SAM_HSMCI_INIT)

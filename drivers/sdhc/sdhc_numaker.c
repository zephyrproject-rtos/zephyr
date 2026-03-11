/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Nuvoton Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_sdhc

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#if defined(CONFIG_SOC_SERIES_M55M1X)
#include <zephyr/dt-bindings/clock/numaker_m55m1x_clock.h>
#endif

LOG_MODULE_REGISTER(sdhc_numaker, CONFIG_SDHC_LOG_LEVEL);

#define NUMAKER_SDHC_RESET_TIMEOUT_US     (10 * 1000)
#define NUMAKER_SDHC_8CLKS_TIMEOUT_US     (2 * 1000)
#define NUMAKER_SDHC_DEBOUNCE_DURATION_US (10 * 1000)

/* Max number of blocks transferred in one round */
#define NUMAKER_SDHC_BLOCKS_MAX (SDH_CTL_BLKCNT_Msk >> SDH_CTL_BLKCNT_Pos)

/* Data out CRC status OK */
#define SDH_INTSTS_CRCSTS_OK (2 << SDH_INTSTS_CRCSTS_Pos)

#define NUMAKER_SDHC_LOG_REGS_DBG(base)                                                            \
	do {                                                                                       \
		LOG_DBG("DMACTL: 0x%08x", base->DMACTL);                                           \
		LOG_DBG("DMASA: 0x%08x", base->DMASA);                                             \
		LOG_DBG("DMABCNT: 0x%08x", base->DMABCNT);                                         \
		LOG_DBG("DMAINTSTS: 0x%08x", base->DMAINTSTS);                                     \
		LOG_DBG("GCTL: 0x%08x", base->GCTL);                                               \
		LOG_DBG("GINTSTS: 0x%08x", base->GINTSTS);                                         \
		LOG_DBG("CTL: 0x%08x", base->CTL);                                                 \
		LOG_DBG("CMDARG: 0x%08x", base->CMDARG);                                           \
		LOG_DBG("INTSTS: 0x%08x", base->INTSTS);                                           \
		LOG_DBG("BLEN: 0x%08x", base->BLEN);                                               \
	} while (0)

#define NUMAKER_SDHC_LOG_COMMAND_ERR(cmd)                                                          \
	do {                                                                                       \
		LOG_ERR("opcode: %d", cmd->opcode);                                                \
		LOG_ERR("arg: 0x%08x", cmd->arg);                                                  \
		LOG_ERR("response_type: 0x%08x", cmd->response_type);                              \
	} while (0)

#define NUMAKER_SDHC_LOG_DATA_ERR(data)                                                            \
	do {                                                                                       \
		LOG_ERR("block_addr: 0x%08x", data->block_addr);                                   \
		LOG_ERR("block_size: %d", data->block_size);                                       \
		LOG_ERR("blocks: %d", data->blocks);                                               \
	} while (0)

struct sdhc_numaker_config {
	SDH_T *base;
	const struct reset_dt_spec reset;
	const struct device *clkctrl_dev;
	struct numaker_scc_subsys pcc_init;
	struct numaker_scc_subsys pcc_def;
	uint32_t irq_n;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;

	uint32_t max_current_330;
	uint32_t max_current_300;
	uint32_t max_current_180;
	uint32_t min_bus_freq;
	uint32_t max_bus_freq;
	uint32_t power_delay_ms;
	bool mmc_hs200_1_8v;
	bool mmc_hs400_1_8v;

	bool card_detect_from_gpio;
};

struct sdhc_numaker_data {
	const struct device *dev;

	struct sdhc_host_props props;

	sdhc_interrupt_cb_t cb;
	int cb_sources;
	void *cb_user_data;

	atomic_t errors;

	/* SD card present work */
	struct k_work_delayable card_present_work;

	/* Sync between APIs */
	struct k_mutex api_lock;

	/* Sync for data transfer */
	struct k_sem xfer_complete;
};

/* Initialize SD host controller properties */
static void numaker_sdhc_init_host_props(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	struct sdhc_host_props *props = &data->props;
	struct sdhc_host_caps *caps = &props->host_caps;

	/* Initialize to zero for clean */
	memset(props, 0, sizeof(*props));

	/* SD host controller properties */
	props->f_max = config->max_bus_freq;
	props->f_min = config->min_bus_freq;
	props->power_delay = config->power_delay_ms;
	props->max_current_330 = config->max_current_330;
	props->max_current_300 = config->max_current_300;
	props->max_current_180 = config->max_current_180;
	props->bus_4_bit_support = true;

	/* SD host controller capabilities */
	caps->high_spd_support = 1;
	caps->vol_330_support = 1;
}

/* Reset H/W internal states as possible, don't premature return on error */
static int numaker_sdhc_reset_hw_fsm(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	SDH_T *base = config->base;

	base->DMACTL = SDH_DMACTL_DMARST_Msk;
	if (!WAIT_FOR((base->DMACTL & SDH_DMACTL_DMARST_Msk) == 0, NUMAKER_SDHC_RESET_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("DMACTL.DMARST timeout");
	}

	base->GCTL = SDH_GCTL_GCTLRST_Msk | SDH_GCTL_SDEN_Msk;
	if (!WAIT_FOR((base->GCTL & SDH_GCTL_GCTLRST_Msk) == 0, NUMAKER_SDHC_RESET_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("GCTL.GCTLRST timeout");
	}

	base->CTL |= SDH_CTL_CTLRST_Msk;
	if (!WAIT_FOR((base->CTL & SDH_CTL_CTLRST_Msk) == 0, NUMAKER_SDHC_RESET_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("CTL.CTLRST timeout");
	}

	return 0;
}

static int numaker_sdhc_reset(const struct device *dev, bool keep_state)
{
	const struct sdhc_numaker_config *config = dev->config;
	SDH_T *base = config->base;

	/* Reset H/W FSM anyway, ignore error code */
	numaker_sdhc_reset_hw_fsm(dev);

	if (keep_state) {
		/* NOTE: Some enable bits e.g. DMACTL.DMAEN will get cleared
		 * with above reset bit toggle. Enable them back.
		 */
		base->DMACTL |= SDH_DMACTL_DMAEN_Msk;
		base->GCTL |= SDH_GCTL_SDEN_Msk;
		return 0;
	}

	/* DMA */
	base->DMACTL = SDH_DMACTL_DMAEN_Msk;
	base->DMAINTEN = SDH_DMAINTEN_ABORTIEN_Msk;

	/* Global */
	base->GCTL = SDH_GCTL_SDEN_Msk;
	base->GINTEN = SDH_GINTEN_DTAIEN_Msk;

	/* SD */
	base->INTEN = SDH_INTEN_WKIEN_Msk | SDH_INTEN_CRCIEN_Msk | SDH_INTEN_BLKDIEN_Msk;
	if (config->card_detect_from_gpio) {
		/* Card detect pin from GPIO */
		base->INTEN |= SDH_INTEN_CDSRC_Msk;
	} else {
		/* Card detect pin from DAT3 */
		base->INTEN &= ~SDH_INTEN_CDSRC_Msk;
	}

	base->CTL = 0;

	/* Keep SD clock?
	 *
	 * Following BSP SDHC driver, SD clock is not kept for both card
	 * detection pin from GPIO and DAT3 during idle. The former will
	 * still support card plug-in/unplug-out interrupts but the latter
	 * won't.
	 */
	base->CTL &= ~SDH_CTL_CLKKEEP_Msk;

	/* Set SDNWR to 9, per BSP SDHC driver */
	base->CTL &= ~SDH_CTL_SDNWR_Msk;
	base->CTL |= 9 << SDH_CTL_SDNWR_Pos;

	/* Set BLKCNT to 1 */
	base->CTL &= ~SDH_CTL_BLKCNT_Msk;
	base->CTL |= 1 << SDH_CTL_BLKCNT_Pos;

	/* Set SD 1-bit data bus */
	base->CTL &= ~SDH_CTL_DBW_Msk;

	return 0;
}

/* Set SD bus clock
 *
 * rate_actual: actual SD bus clock
 */
static int numaker_sdhc_set_bus_clock(const struct device *dev, uint32_t rate,
				      uint32_t *rate_actual)
{
	const struct sdhc_numaker_config *config = dev->config;
	const struct numaker_scc_subsys *pcc_arr[] = {&config->pcc_init, &config->pcc_def};
	const struct numaker_scc_subsys **pcc_pos = pcc_arr;
	const struct numaker_scc_subsys **pcc_end = pcc_arr + ARRAY_SIZE(pcc_arr);
	uint32_t rate_cand;
	int rc;

	/* Traverse over available clock configurations and find best fit which is
	 * lower than and most close to target rate.
	 */
	rate_cand = 0;
	for (; pcc_pos != pcc_end; pcc_pos++) {
		uint32_t rate_cand2;
		struct numaker_scc_subsys_rate scc_subsys_rate = {0};

		scc_subsys_rate.pcc.clk_mod_rate = rate;
		rc = clock_control_set_rate(config->clkctrl_dev, (clock_control_subsys_t)*pcc_pos,
					    (clock_control_subsys_rate_t)&scc_subsys_rate);
		if (rc < 0) {
			return rc;
		}

		rc = clock_control_get_rate(config->clkctrl_dev, (clock_control_subsys_t)*pcc_pos,
					    &rate_cand2);
		if (rc < 0) {
			return rc;
		}

		if ((rate_cand2 <= rate) && (rate_cand2 > rate_cand)) {
			rate_cand = rate_cand2;
		}

		LOG_DBG("Candidate clock: source clock %dHz, clock divider %d, bus clock %dHz",
			scc_subsys_rate.pcc.clk_src_rate, scc_subsys_rate.pcc.clk_div_value,
			rate_cand2);
	}

	if (rate_cand == 0) {
		LOG_ERR("No match clock configuration");
		return -ENOTSUP;
	}

	if (rate_actual) {
		*rate_actual = rate_cand;
	}

	return 0;
}

static int numaker_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;
	int rc;

	LOG_DBG("SDHC I/O: bus width %d, clock %dHz, card power %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	/* Note on power cycle
	 *
	 * The sdmmc subsystem will power cycle the card to reset it, but
	 * power cycle is not supported by the H/W.
	 */

	/* Note on CTL.CLK74OEN
	 *
	 * We must wait 74 clock cycles, per SD spec, to use card after power
	 * on. At 400000KHz, this is a 185us delay. Because sdmmc subsystem
	 * has done with the delay, the CTL.CLK74OEN function is not used here.
	 */

	/* Note on gated clock (clock set to zero)
	 *
	 * This is required during signal voltage switch. With CTL.CLKKEEP
	 * unset, SD bus clock is disabled on no traffic, so no extra work
	 * is needed to set clock to zero.
	 */
	__ASSERT_NO_MSG(!(base->CTL & SDH_CTL_CLKKEEP_Msk));
	if (ios->clock > 0) {
		uint32_t clock_actual;

		if (ios->clock > data->props.f_max || ios->clock < data->props.f_min) {
			LOG_ERR("Unsupported bus clock %dHz (min %dHz, max %dHz)", ios->clock,
				data->props.f_min, data->props.f_max);
			return -ENOTSUP;
		}

		rc = numaker_sdhc_set_bus_clock(dev, ios->clock, &clock_actual);
		if (rc < 0) {
			LOG_ERR("Set bus clock %dHz failed: %d", ios->clock, rc);
			return rc;
		}

		LOG_DBG("Clock expected %dHz, actual %dHz", ios->clock, clock_actual);
	}

	if (ios->bus_width > 0) {
		if (ios->bus_width == SDHC_BUS_WIDTH1BIT) {
			base->CTL &= ~SDH_CTL_DBW_Msk;
		} else if (ios->bus_width == SDHC_BUS_WIDTH4BIT) {
			base->CTL |= SDH_CTL_DBW_Msk;
		} else {
			LOG_ERR("Unsupported bus width %d", ios->bus_width);
			return -ENOTSUP;
		}
	}

	return 0;
}

static int numaker_sdhc_get_card_present(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	SDH_T *base = config->base;
	int rc;

	if ((base->INTEN & SDH_INTEN_CDSRC_Msk) == SDH_INTEN_CDSRC_Msk) {
		/* Card detect pin from GPIO */
		rc = (base->INTSTS & SDH_INTSTS_CDSTS_Msk) ? 0 : 1;
	} else {
		/* Card detect pin from DAT3 */
		base->CTL |= SDH_CTL_CLKKEEP_Msk;
		/* NOTE: This function can invoke from ISR. Don't invoke
		 * sleep-like function for delay.
		 */
		k_busy_wait(NUMAKER_SDHC_8CLKS_TIMEOUT_US);
		rc = (base->INTSTS & SDH_INTSTS_CDSTS_Msk) ? 1 : 0;
		base->CTL &= ~SDH_CTL_CLKKEEP_Msk;
	}

	return rc;
}

static void numaker_sdhc_submit_card_present(struct k_work *work)
{
	struct k_work_delayable *card_present_work = k_work_delayable_from_work(work);
	struct sdhc_numaker_data *data = CONTAINER_OF(card_present_work, struct sdhc_numaker_data,
						      card_present_work);
	const struct device *dev = data->dev;

	/* Submit SD card insert/remove event */
	if (numaker_sdhc_get_card_present(dev)) {
		if (data->cb && (data->cb_sources & SDHC_INT_INSERTED)) {
			data->cb(dev, SDHC_INT_INSERTED, data->cb_user_data);
		}
	} else {
		if (data->cb && (data->cb_sources & SDHC_INT_REMOVED)) {
			data->cb(dev, SDHC_INT_REMOVED, data->cb_user_data);
		}
	}
}

static int numaker_sdhc_card_busy(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	SDH_T *base = config->base;

	base->CTL |= SDH_CTL_CLK8OEN_Msk;
	if (!WAIT_FOR((base->CTL & SDH_CTL_CLK8OEN_Msk) == 0, NUMAKER_SDHC_8CLKS_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("CTL.CLK8OEN timeout");
		return -ETIMEDOUT;
	}

	return (base->INTSTS & SDH_INTSTS_DAT0STS_Msk) ? 0 : 1;
}

static int numaker_sdhc_request_command(const struct device *dev, struct sdhc_command *sd_cmd,
					bool log)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;
	uint32_t sdhc_ctl;
	uint32_t sdhc_resp0, sdhc_resp1;
	uint32_t i;
	uint32_t response_r2[5];

	sdhc_ctl = base->CTL;

	/* Command out */
	sdhc_ctl &= ~SDH_CTL_CMDCODE_Msk;
	sdhc_ctl |= (sd_cmd->opcode << SDH_CTL_CMDCODE_Pos) | SDH_CTL_COEN_Msk;

	/* Argument */
	base->CMDARG = sd_cmd->arg;

	/* Response in */
	switch (sd_cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		break;
	case SD_RSP_TYPE_R2:
		sdhc_ctl |= SDH_CTL_R2EN_Msk;
		for (i = 0; i < ARRAY_SIZE(response_r2); i++) {
			base->FB[i] = 0x0;
		}
		break;
	default:
		sdhc_ctl |= SDH_CTL_RIEN_Msk;
	}

	k_sem_reset(&data->xfer_complete);

	/* Trigger command/response */
	base->CTL = sdhc_ctl;

	if (!WAIT_FOR((base->CTL & (SDH_CTL_R2EN_Msk | SDH_CTL_RIEN_Msk | SDH_CTL_COEN_Msk)) == 0,
		      sd_cmd->timeout_ms * 1000, k_yield())) {
		LOG_ERR("Command/response timeout");
		NUMAKER_SDHC_LOG_REGS_DBG(base);
		NUMAKER_SDHC_LOG_COMMAND_ERR(sd_cmd);
		return -ETIMEDOUT;
	}

	/* Check CRC */
	switch (sd_cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		break;
	case SD_RSP_TYPE_R3:
		/* R3 doesn't have CRC7, skip it */
		atomic_clear(&data->errors);
		break;
	default:
		if ((base->INTSTS & SDH_INTSTS_CRC7_Msk) != SDH_INTSTS_CRC7_Msk) {
			LOG_ERR("Response CRC7 error");
			NUMAKER_SDHC_LOG_COMMAND_ERR(sd_cmd);
			return -EIO;
		}
	}

	switch (sd_cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		break;
	case SD_RSP_TYPE_R2:
		response_r2[0] = BSWAP_32(base->FB[0]);
		for (i = 1; i < ARRAY_SIZE(response_r2); i++) {
			response_r2[i] = BSWAP_32(base->FB[i]);
			sd_cmd->response[ARRAY_SIZE(response_r2) - i - 1] =
				((response_r2[i - 1] & 0xffffff) << 8) |
				((response_r2[i] & 0xff000000) >> 24);
		}
		break;
	default:
		sdhc_resp0 = base->RESP0;
		sdhc_resp1 = base->RESP1;
		sd_cmd->response[0] = ((sdhc_resp0 & 0xffffff) << 8) | (sdhc_resp1 & 0xff);
		sd_cmd->response[1] = 0x0;
		sd_cmd->response[2] = 0x0;
		sd_cmd->response[3] = 0x0;
	}

	return 0;
}

static int numaker_sdhc_request_cmd12(const struct device *dev, bool log)
{
	struct sdhc_command stop_cmd = {
		.opcode = SD_STOP_TRANSMISSION,
		.arg = 0,
		.response_type = SD_RSP_TYPE_R1b | SD_SPI_RSP_TYPE_R1b,
		.retries = CONFIG_SDHC_NUMAKER_CMD_RETRIES,
		.timeout_ms = CONFIG_SDHC_NUMAKER_CMD_TIMEOUT,
	};

	return numaker_sdhc_request_command(dev, &stop_cmd, log);
}

static int numaker_sdhc_request_data(const struct device *dev, struct sdhc_command *sd_cmd,
				     struct sdhc_data *sd_data, bool log)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;
	int rc = 0;
	bool is_write_cmd;
	bool is_multi_block_cmd;
	uint32_t sdhc_ctl;
	uint32_t blocks_rmn = sd_data->blocks;
	uint32_t data_pos_xfering;
	void *data_ptr_xfering;
	uint32_t blocks_xfering;
	uint32_t bytes_xfering;

	/* Determine data transfer direction */
	switch (sd_cmd->opcode) {
	case SD_WRITE_SINGLE_BLOCK:
		is_write_cmd = true;
		is_multi_block_cmd = false;
		break;
	case SD_WRITE_MULTIPLE_BLOCK:
		is_write_cmd = true;
		is_multi_block_cmd = true;
		break;
	case SD_READ_SINGLE_BLOCK:
		is_write_cmd = false;
		is_multi_block_cmd = false;
		break;
	case SD_READ_MULTIPLE_BLOCK:
		is_write_cmd = false;
		is_multi_block_cmd = true;
		break;
	case SD_APP_SEND_SCR:
	case SD_SWITCH:
	case SD_APP_SEND_NUM_WRITTEN_BLK:
		is_write_cmd = false;
		is_multi_block_cmd = false;
		break;
	default:
		if (log) {
			LOG_ERR("Unsupported command code (%d) for data transfer", sd_cmd->opcode);
		}
		return -ENOTSUP;
	}

	sd_data->bytes_xfered = 0;
	data_pos_xfering = (uint32_t)sd_data->data;
	data_ptr_xfering = (void *)(uintptr_t)data_pos_xfering;
	while (blocks_rmn > 0) {
		blocks_xfering = blocks_rmn;
		if (blocks_xfering > NUMAKER_SDHC_BLOCKS_MAX) {
			blocks_xfering = NUMAKER_SDHC_BLOCKS_MAX;
		}
		bytes_xfering = sd_data->block_size * blocks_xfering;

		sdhc_ctl = base->CTL;

		/* Prepare data transfer */
		sdhc_ctl &= ~SDH_CTL_BLKCNT_Msk;
		sdhc_ctl |= blocks_xfering << SDH_CTL_BLKCNT_Pos;
		base->BLEN = sd_data->block_size - 1;
		base->DMASA = data_pos_xfering;

		/* Data in/out */
		if (is_write_cmd) {
			sdhc_ctl |= SDH_CTL_DOEN_Msk;
		} else {
			sdhc_ctl |= SDH_CTL_DIEN_Msk;
		}

		/* Cache management before DMA */
		sys_cache_data_flush_range(data_ptr_xfering, bytes_xfering);

		/* Trigger command/response */
		base->CTL = sdhc_ctl;

		rc = k_sem_take(&data->xfer_complete, K_MSEC(sd_data->timeout_ms));
		if (rc < 0) {
			if (log) {
				LOG_ERR("Data transfer timeout");
				NUMAKER_SDHC_LOG_COMMAND_ERR(sd_cmd);
			}

			rc = -ETIMEDOUT;
			break;
		}

		/* Cache management after DMA */
		if (!is_write_cmd) {
			sys_cache_data_invd_range(data_ptr_xfering, bytes_xfering);
		}

		/* Check CRC */
		if (is_write_cmd) {
			if ((base->INTSTS & SDH_INTSTS_CRCSTS_Msk) != SDH_INTSTS_CRCSTS_OK) {
				LOG_ERR("Data out CRC status error");
				NUMAKER_SDHC_LOG_COMMAND_ERR(sd_cmd);
				rc = -EIO;
				break;
			}
		} else {
			if ((base->INTSTS & SDH_INTSTS_CRC16_Msk) != SDH_INTSTS_CRC16_Msk) {
				LOG_ERR("Data in CRC16 error");
				NUMAKER_SDHC_LOG_COMMAND_ERR(sd_cmd);
				rc = -EIO;
				break;
			}
		}

		/* Next round */
		sd_data->bytes_xfered += bytes_xfering;
		data_pos_xfering += bytes_xfering;
		data_ptr_xfering = (void *)(uintptr_t)data_pos_xfering;
		blocks_rmn -= blocks_xfering;
	}

	/* Auto CMD12
	 *
	 * Stop transmission on error or multi-block command
	 */
	if (rc < 0 || is_multi_block_cmd) {
		numaker_sdhc_request_cmd12(dev, log);
	}

	return rc;
}

static int numaker_sdhc_request(const struct device *dev, struct sdhc_command *sd_cmd,
				struct sdhc_data *sd_data, bool log)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	/* Recover from last errors */
	if (atomic_clear(&data->errors)) {
		numaker_sdhc_reset(dev, true);
	}

	/* Command/response */
	rc = numaker_sdhc_request_command(dev, sd_cmd, log);
	if (rc < 0) {
		return rc;
	}

	/* Has data transfer? */
	if (sd_data) {
		rc = numaker_sdhc_request_data(dev, sd_cmd, sd_data, log);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static int numaker_sdhc_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
					 int sources, void *user_data)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;

	if (sources & SDHC_INT_SDIO) {
		LOG_ERR("Unsupported interrupt source SDHC_INT_SDIO");
		return -ENOTSUP;
	}

	if ((sources & (SDHC_INT_INSERTED | SDHC_INT_REMOVED)) && !config->card_detect_from_gpio) {
		/* Card detect pin from DAT3 */
		/* Not support card detect interrupt due to SD bus clock
		 * not kept on for DAT3
		 */
		LOG_ERR("Unsupported interrupt source SDHC_INT_INSERTED/SDHC_INT_REMOVED for DAT3 "
			"as card detect pin");
		return -ENOTSUP;
	}

	/* Record callback parameters */
	data->cb = callback;
	data->cb_sources = sources;
	data->cb_user_data = user_data;

	if (data->cb_sources & (SDHC_INT_INSERTED | SDHC_INT_REMOVED)) {
		base->INTEN |= SDH_INTEN_CDIEN_Msk;
	}

	return 0;
}

static int numaker_sdhc_disable_interrupt(const struct device *dev, int sources)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;

	data->cb_sources &= ~sources;

	if ((data->cb_sources & (SDHC_INT_INSERTED | SDHC_INT_REMOVED)) == 0) {
		base->INTEN &= ~SDH_INTEN_CDIEN_Msk;
	}

	if (data->cb_sources == 0) {
		data->cb = NULL;
		data->cb_user_data = NULL;
	}

	return 0;
}

/* Zephyr SDHC HAL API */

/* Reset SDHC controller state */
static int sdhc_numaker_reset(const struct device *dev)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_reset(dev, false);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Send command to SDHC */
static int sdhc_numaker_request(const struct device *dev, struct sdhc_command *sd_cmd,
				struct sdhc_data *sd_data)
{
	struct sdhc_numaker_data *data = dev->data;
	uint32_t retries = sd_cmd->retries + 1;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	do {
		rc = numaker_sdhc_request(dev, sd_cmd, sd_data, retries == 1);
	} while ((rc < 0) && --retries);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Set I/O properties of SDHC */
static int sdhc_numaker_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_set_io(dev, ios);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Check for SDHC card presence */
static int sdhc_numaker_get_card_present(const struct device *dev)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_get_card_present(dev);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Check if SD card is busy */
static int sdhc_numaker_card_busy(const struct device *dev)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_card_busy(dev);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Get SD host controller properties */
static int sdhc_numaker_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_numaker_data *data = dev->data;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	memcpy(props, &data->props, sizeof(*props));

	k_mutex_unlock(&data->api_lock);

	return 0;
}

/* Enable SDHC interrupt sources */
static int sdhc_numaker_enable_interrupt(const struct device *dev, sdhc_interrupt_cb_t callback,
					 int sources, void *user_data)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_enable_interrupt(dev, callback, sources, user_data);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

/* Disable SDHC interrupt sources */
static int sdhc_numaker_disable_interrupt(const struct device *dev, int sources)
{
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	rc = numaker_sdhc_disable_interrupt(dev, sources);

	k_mutex_unlock(&data->api_lock);

	return rc;
}

static void sdhc_numaker_isr(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	SDH_T *base = config->base;
	uint32_t inten = base->INTEN;
	uint32_t intsts = base->INTSTS;

	/* SD card insert/remove */
	if ((intsts & SDH_INTSTS_CDIF_Msk) && (inten & SDH_INTEN_CDIEN_Msk)) {
		/* Clear the event */
		base->INTSTS = SDH_INTSTS_CDIF_Msk;

		/* Submit SD card insert/remove event
		 *
		 * Reschedule the work to implement debounce
		 */
		k_work_reschedule(&data->card_present_work,
				  K_USEC(NUMAKER_SDHC_DEBOUNCE_DURATION_US));
	}

	/* Data transfer done */
	if (intsts & SDH_INTSTS_BLKDIF_Msk) {
		/* Clear the event */
		base->INTSTS = SDH_INTSTS_BLKDIF_Msk;

		/* End of data transfer */
		k_sem_give(&data->xfer_complete);
	}

	/* DMA read/write target abort */
	if (base->GINTSTS & SDH_GINTSTS_DTAIF_Msk) {
		/* Clear the event */
		base->GINTSTS = SDH_GINTSTS_DTAIF_Msk;

		/* Increment error count */
		atomic_inc(&data->errors);

		/* End of data transfer */
		k_sem_give(&data->xfer_complete);
	}

	/* DMA read/write target abort */
	if (base->DMAINTSTS & SDH_DMAINTSTS_ABORTIF_Msk) {
		/* Clear the event */
		base->DMAINTSTS = SDH_DMAINTSTS_ABORTIF_Msk;

		/* Increment error count */
		atomic_inc(&data->errors);

		/* End of data transfer */
		k_sem_give(&data->xfer_complete);
	}

	/* CRC error */
	if (intsts & SDH_INTSTS_CRCIF_Msk) {
		/* Clear the event */
		base->INTSTS = SDH_INTSTS_CRCIF_Msk;

		/* Increment error count */
		atomic_inc(&data->errors);

		/* End of data transfer */
		k_sem_give(&data->xfer_complete);
	}
}

static int sdhc_numaker_init(const struct device *dev)
{
	const struct sdhc_numaker_config *config = dev->config;
	struct sdhc_numaker_data *data = dev->data;
	int rc;

	/* Validate this module's reset object */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Keep pointer to device
	 *
	 * Note we cannot get the device via CONTAINER_OF(data, struct device, data)
	 * because 'data' is a pointer to user-defined struct, not the address of
	 * the 'data' field inside the containing device.
	 */
	data->dev = dev;

	/* Initialize SD host controller properties */
	numaker_sdhc_init_host_props(dev);

	/* Initialize error count */
	data->errors = ATOMIC_INIT(0);

	/* Initialize card present work */
	k_work_init_delayable(&data->card_present_work, numaker_sdhc_submit_card_present);

	/* Initialize synchronization objects */
	k_mutex_init(&data->api_lock);
	k_sem_init(&data->xfer_complete, 0, 1);

	SYS_UnlockReg();

	/* Equivalent to CLK_EnableModuleClock() */
	rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc_init);
	if (rc < 0) {
		goto cleanup;
	}

	/* Equivalent to CLK_SetModuleClock() */
	rc = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t)&config->pcc_init,
				     NULL);
	if (rc < 0) {
		goto cleanup;
	}

	/* Configure pinmux (NuMaker's SYS MFP) */
	rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		goto cleanup;
	}

	/* Reset SDHC to default state, same as BSP's SYS_ResetModule(id_rst) */
	reset_line_toggle_dt(&config->reset);

	config->irq_config_func(dev);

	rc = numaker_sdhc_reset(dev, false);

cleanup:

	SYS_LockReg();
	return rc;
}

static DEVICE_API(sdhc, sdhc_numaker_api) = {
	.reset = sdhc_numaker_reset,
	.request = sdhc_numaker_request,
	.set_io = sdhc_numaker_set_io,
	.get_card_present = sdhc_numaker_get_card_present,
	.card_busy = sdhc_numaker_card_busy,
	.get_host_props = sdhc_numaker_get_host_props,
	.enable_interrupt = sdhc_numaker_enable_interrupt,
	.disable_interrupt = sdhc_numaker_disable_interrupt,
};

/* Peripheral Clock Control by name */
#define NUMAKER_PCC_INST_GET_BY_NAME(inst, name)                                                   \
	{                                                                                          \
		.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC,                                            \
		.pcc.clk_modidx = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_module_index),     \
		.pcc.clk_src = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_source),              \
		.pcc.clk_div = DT_INST_CLOCKS_CELL_BY_NAME(inst, name, clock_divider),             \
	}

#define SDHC_NUMAKER_INIT(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void sdhc_numaker_irq_config_func_##inst(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), sdhc_numaker_isr,     \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
                                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static const struct sdhc_numaker_config sdhc_numaker_config_##inst = {                     \
		.base = (SDH_T *)DT_INST_REG_ADDR(inst),                                           \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                \
		.pcc_init = NUMAKER_PCC_INST_GET_BY_NAME(inst, initial),                           \
		.pcc_def = NUMAKER_PCC_INST_GET_BY_NAME(inst, default),                            \
		.irq_n = DT_INST_IRQN(inst),                                                       \
		.irq_config_func = sdhc_numaker_irq_config_func_##inst,                            \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.max_current_330 = DT_INST_PROP(inst, max_current_330),                            \
		.max_current_300 = DT_INST_PROP(inst, max_current_300),                            \
		.max_current_180 = DT_INST_PROP(inst, max_current_180),                            \
		.min_bus_freq = DT_INST_PROP(inst, min_bus_freq),                                  \
		.max_bus_freq = DT_INST_PROP(inst, max_bus_freq),                                  \
		.power_delay_ms = DT_INST_PROP(inst, power_delay_ms),                              \
		.mmc_hs200_1_8v = DT_INST_PROP(inst, mmc_hs200_1_8v),                              \
		.mmc_hs400_1_8v = DT_INST_PROP(inst, mmc_hs400_1_8v),                              \
		.card_detect_from_gpio = DT_INST_PROP(inst, card_detect_from_gpio),                \
	};                                                                                         \
                                                                                                   \
	static struct sdhc_numaker_data sdhc_numaker_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sdhc_numaker_init, NULL, &sdhc_numaker_data_##inst,            \
			      &sdhc_numaker_config_##inst, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY, \
			      &sdhc_numaker_api);

DT_INST_FOREACH_STATUS_OKAY(SDHC_NUMAKER_INIT);

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esai.h"

/* TODO:
 *	1) Some pin functions can be inferred from software ctx. For instance,
 *	if you use more than 1 data line, it's obvious you're going
 *	to want to keep the pins of the data lines in ESAI mode.
 *
 *	2) Add function for handling underrun/overrun. Preferably
 *	we should do the same as we did for SAI to ease the testing
 *	process. This approach will do for now. In the future, this
 *	can be handled in a more sophisticated maner.
 *
 * notes:
 *	1) EXTAL clock is divided as follows:
 *		a) Initial EXTAL signal is automatically divided by 2.
 *		b) If prescaler is enabled the resulting EXTAL from a)
 *		is divided by 8.
 *		c) The resulting EXTAL signal from b) can be divided
 *		by 1 up to 256 (configured via xPM0-xPM7). The resulting
 *		signal is referred to as HCLK.
 *		d) HCLK obtained from c) can be further divided by 1
 *		up to 16 (configured via xFP0-xFP3). The resulting signal is
 *		referred to as BCLK.
 */

static int esai_get_clock_rate_config(uint32_t extal_rate, uint32_t hclk_rate,
				      uint32_t bclk_rate, bool variable_hclk,
				      bool allow_bclk_configuration,
				      struct esai_transceiver_config *cfg)
{
	uint32_t hclk_div_ratio, bclk_div_ratio;

	/* sanity checks */
	if (!cfg) {
		LOG_ERR("got NULL clock configuration");
		return -EINVAL;
	}

	if (!extal_rate || !hclk_rate || !bclk_rate) {
		LOG_ERR("got NULL clock rate");
		return -EINVAL;
	}

	if (hclk_rate > extal_rate) {
		LOG_ERR("HCLK rate cannot be higher than EXTAL rate");
		return -EINVAL;
	}

	if (bclk_rate > extal_rate) {
		LOG_ERR("BCLK rate cannot be higher than EXTAL rate");
		return -EINVAL;
	}

	if (DIV_ROUND_UP(extal_rate, bclk_rate) > 2 * 8 * 256 * 16) {
		LOG_ERR("BCLK rate %u cannot be obtained from EXTAL rate %u",
			bclk_rate, extal_rate);
		return -EINVAL;
	}

	/* TODO: add explanation */
	if (DIV_ROUND_UP(extal_rate / 2, bclk_rate) == 1) {
		LOG_ERR("HCLK prescaler bypass with divider bypass is not supported");
		return -EINVAL;
	}

	hclk_div_ratio = 1;
	bclk_div_ratio = 1;

	/* check if HCLK is in (EXTAL_RATE / 2, EXTAL_RATE). If so,
	 * return an error as any rates from this interval cannot be obtained.
	 */
	if (hclk_rate > extal_rate / 2 && hclk_rate < extal_rate) {
		LOG_ERR("HCLK rate cannot be higher than EXTAL's rate divided by 2");
		return -EINVAL;
	}

	/* compute HCLK configuration - only required if HCLK pad output is used */
	if (!variable_hclk) {
		if (extal_rate == hclk_rate) {
			/* HCLK rate from pad is the same as EXTAL rate */
			cfg->hclk_bypass = true;
		} else {
			/* EXTAL is automatically divided by 2 */
			extal_rate /= 2;

			/* compute prescaler divide ratio w/ prescaler bypass */
			hclk_div_ratio = DIV_ROUND_UP(extal_rate, hclk_rate);

			if (hclk_div_ratio > 256) {
				/* can't obtain HCLK w/o prescaler */
				cfg->hclk_prescaler_en = true;

				extal_rate /= 8;

				/* recompute ratio w/ prescaler */
				hclk_div_ratio = DIV_ROUND_UP(extal_rate, hclk_rate);

				if (hclk_div_ratio > 256) {
					LOG_ERR("cannot obtain HCLK rate %u from EXTAL rate %u",
						hclk_rate, extal_rate);
					return -EINVAL;
				}
			}
		}
	}

	cfg->hclk_div_ratio = hclk_div_ratio;

	if (!allow_bclk_configuration) {
		return 0;
	}

	extal_rate = DIV_ROUND_UP(extal_rate, hclk_div_ratio);

	/* compute BCLK configuration */
	if (variable_hclk || cfg->hclk_bypass) {
		/* attempt to find a configuration that satisfies BCLK's rate */
		extal_rate /= 2;

		hclk_div_ratio = DIV_ROUND_UP(extal_rate, bclk_rate);

		/* check if prescaler is required */
		if (hclk_div_ratio > 256 * 16) {
			extal_rate /= 8;
			cfg->hclk_prescaler_en = true;
			hclk_div_ratio = DIV_ROUND_UP(extal_rate, bclk_rate);
		}

		/* check if we really need to loop through TPM div ratios */
		if (hclk_div_ratio < 256) {
			cfg->bclk_div_ratio = 1;
			cfg->hclk_div_ratio = hclk_div_ratio;
			return 0;
		}

		for (int i = 1; i < 256; i++) {
			hclk_div_ratio = DIV_ROUND_UP(extal_rate / i, bclk_rate);
			bclk_div_ratio = DIV_ROUND_UP(extal_rate / hclk_div_ratio, bclk_rate);

			if (bclk_div_ratio <= 16) {
				/* found valid configuration, let caller know */
				cfg->bclk_div_ratio = bclk_div_ratio;
				cfg->hclk_div_ratio = hclk_div_ratio;

				return 0;
			}
		}

		/* no valid configuration found */
		LOG_ERR("no valid configuration for BCLK rate %u and EXTAL rate %u",
			bclk_rate, extal_rate);
		return -EINVAL;
	}

	/* can the BCLK rate be obtained w/o modifying divided EXTAL? */
	bclk_div_ratio = DIV_ROUND_UP(extal_rate, bclk_rate);

	if (bclk_div_ratio > 16) {
		LOG_ERR("cannot obtain BCLK rate %d from EXTAL rate %d",
			bclk_rate, extal_rate);
		return -EINVAL;
	}

	/* save ratios before returning */
	cfg->bclk_div_ratio = bclk_div_ratio;
	cfg->hclk_div_ratio = hclk_div_ratio;

	return 0;
}

static int esai_get_clk_provider_config(const struct dai_config *cfg,
					struct esai_transceiver_config *xceiver_cfg)
{
	switch (cfg->format & DAI_FORMAT_CLOCK_PROVIDER_MASK) {
	case DAI_CBC_CFC:
		/* default FSYNC and BCLK are OUTPUT */
		break;
	case DAI_CBP_CFP:
		xceiver_cfg->bclk_dir = kESAI_ClockInput;
		xceiver_cfg->fsync_dir = kESAI_ClockInput;
		break;
	default:
		LOG_ERR("invalid clock provider configuration: %d",
			cfg->format & DAI_FORMAT_CLOCK_PROVIDER_MASK);
		return -EINVAL;
	}

	return 0;
}

static int esai_get_clk_inversion_config(const struct dai_config *cfg,
					 struct esai_transceiver_config *xceiver_cfg)
{
	switch (cfg->format & DAI_FORMAT_CLOCK_INVERSION_MASK) {
	case DAI_INVERSION_IB_IF:
		ESAI_INVERT_POLARITY(xceiver_cfg->bclk_polarity);
		ESAI_INVERT_POLARITY(xceiver_cfg->fsync_polarity);
		break;
	case DAI_INVERSION_IB_NF:
		ESAI_INVERT_POLARITY(xceiver_cfg->bclk_polarity);
		break;
	case DAI_INVERSION_NB_IF:
		ESAI_INVERT_POLARITY(xceiver_cfg->fsync_polarity);
		break;
	case DAI_INVERSION_NB_NF:
		/* nothing to do here */
		break;
	default:
		LOG_ERR("invalid clock inversion configuration: %d",
			cfg->format & DAI_FORMAT_CLOCK_INVERSION_MASK);
		return -EINVAL;
	}

	return 0;
}

static int esai_get_proto_config(const struct dai_config *cfg,
				 struct esai_transceiver_config *xceiver_cfg)
{
	switch (cfg->format & DAI_FORMAT_PROTOCOL_MASK) {
	case DAI_PROTO_I2S:
		xceiver_cfg->bclk_polarity = kESAI_ClockActiveLow;
		xceiver_cfg->fsync_polarity = kESAI_ClockActiveLow;
		break;
	case DAI_PROTO_DSP_A:
		xceiver_cfg->bclk_polarity = kESAI_ClockActiveLow;
		xceiver_cfg->fsync_is_bit_wide = true;
		break;
	default:
		LOG_ERR("invalid DAI protocol: %d",
			cfg->format & DAI_FORMAT_PROTOCOL_MASK);
		return -EINVAL;
	}
	return 0;
}

static int esai_get_slot_format(uint32_t slot_width, uint32_t word_width,
				struct esai_transceiver_config *cfg)
{
	/* sanity check */
	if (!ESAI_SLOT_WORD_WIDTH_IS_VALID(slot_width, word_width)) {
		LOG_ERR("invalid slot %d word %d width configuration",
			slot_width, word_width);
		return -EINVAL;
	}

	cfg->slot_format = ESAI_SLOT_FORMAT(slot_width, word_width);

	return 0;
}

static void esai_get_xceiver_default_config(struct esai_transceiver_config *cfg)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->hclk_prescaler_en = false;
	cfg->hclk_div_ratio = 1;
	cfg->bclk_div_ratio = 1;
	cfg->hclk_bypass = false;

	cfg->hclk_src = kESAI_HckSourceExternal;
	cfg->hclk_dir = kESAI_ClockOutput;
	cfg->hclk_polarity = kESAI_ClockActiveHigh;

	cfg->bclk_dir = kESAI_ClockOutput;
	cfg->bclk_polarity = kESAI_ClockActiveHigh;

	cfg->fsync_dir = kESAI_ClockOutput;
	cfg->fsync_polarity = kESAI_ClockActiveHigh;

	cfg->fsync_is_bit_wide = false;
	cfg->zero_pad_en = true;
	cfg->fsync_early = true;

	cfg->mode = kESAI_NetworkMode;
	cfg->data_order = kESAI_ShifterMSB;
	cfg->data_left_aligned = true;
}

static void esai_commit_config(ESAI_Type *base,
			       enum dai_dir dir,
			       struct esai_transceiver_config *cfg)
{
	if (dir == DAI_DIR_TX) {
		base->TCCR &= ~(ESAI_TCCR_THCKD_MASK | ESAI_TCCR_TFSD_MASK |
				ESAI_TCCR_TCKD_MASK | ESAI_TCCR_THCKP_MASK |
				ESAI_TCCR_TFSP_MASK | ESAI_TCCR_TCKP_MASK |
				ESAI_TCCR_TFP_MASK | ESAI_TCCR_TDC_MASK |
				ESAI_TCCR_TPSR_MASK | ESAI_TCCR_TPM_MASK);

		base->TCCR |= ESAI_TCCR_THCKD(cfg->hclk_dir) |
			ESAI_TCCR_TFSD(cfg->fsync_dir) |
			ESAI_TCCR_TCKD(cfg->bclk_dir) |
			ESAI_TCCR_THCKP(cfg->hclk_polarity) |
			ESAI_TCCR_TFSP(cfg->fsync_polarity) |
			ESAI_TCCR_TCKP(cfg->bclk_polarity) |
			ESAI_TCCR_TFP(cfg->bclk_div_ratio - 1) |
			ESAI_TCCR_TDC(cfg->fsync_div - 1) |
			ESAI_TCCR_TPSR(!cfg->hclk_prescaler_en) |
			ESAI_TCCR_TPM(cfg->hclk_div_ratio - 1);

		base->TCR &= ~(ESAI_TCR_PADC_MASK | ESAI_TCR_TFSR_MASK |
			       ESAI_TCR_TFSL_MASK | ESAI_TCR_TMOD_MASK |
			       ESAI_TCR_TWA_MASK | ESAI_TCR_TSHFD_MASK);

		base->TCR |= ESAI_TCR_PADC(cfg->zero_pad_en) |
			ESAI_TCR_TFSR(cfg->fsync_early) |
			ESAI_TCR_TFSL(cfg->fsync_is_bit_wide) |
			ESAI_TCR_TSWS(cfg->slot_format) |
			ESAI_TCR_TMOD(cfg->mode) |
			ESAI_TCR_TWA(!cfg->data_left_aligned) |
			ESAI_TCR_TSHFD(cfg->data_order);

		base->ECR &= ~(ESAI_ECR_ETI_MASK |
			       ESAI_ECR_ETO_MASK);

		base->ECR |= ESAI_ECR_ETI(cfg->hclk_src) |
			ESAI_ECR_ETO(cfg->hclk_bypass);

		base->TFCR &= ~(ESAI_TFCR_TFWM_MASK | ESAI_TFCR_TWA_MASK);
		base->TFCR |= ESAI_TFCR_TFWM(cfg->watermark) |
			ESAI_TFCR_TWA(cfg->word_alignment);

		ESAI_TxSetSlotMask(base, cfg->slot_mask);
	} else {
		base->RCCR &= ~(ESAI_RCCR_RHCKD_MASK | ESAI_RCCR_RFSD_MASK |
				ESAI_RCCR_RCKD_MASK | ESAI_RCCR_RHCKP_MASK |
				ESAI_RCCR_RFSP_MASK | ESAI_RCCR_RCKP_MASK |
				ESAI_RCCR_RFP_MASK | ESAI_RCCR_RDC_MASK |
				ESAI_RCCR_RPSR_MASK | ESAI_RCCR_RPM_MASK);

		base->RCCR |= ESAI_RCCR_RHCKD(cfg->hclk_dir) |
			ESAI_RCCR_RFSD(cfg->fsync_dir) |
			ESAI_RCCR_RCKD(cfg->bclk_dir) |
			ESAI_RCCR_RHCKP(cfg->hclk_polarity) |
			ESAI_RCCR_RFSP(cfg->fsync_polarity) |
			ESAI_RCCR_RCKP(cfg->bclk_polarity) |
			ESAI_RCCR_RFP(cfg->bclk_div_ratio - 1) |
			ESAI_RCCR_RDC(cfg->fsync_div - 1) |
			ESAI_RCCR_RPSR(!cfg->hclk_prescaler_en) |
			ESAI_RCCR_RPM(cfg->hclk_div_ratio - 1);

		base->RCR &= ~(ESAI_RCR_RFSR_MASK | ESAI_RCR_RFSL_MASK |
			       ESAI_RCR_RMOD_MASK | ESAI_RCR_RWA_MASK |
			       ESAI_RCR_RSHFD_MASK);

		base->RCR |= ESAI_RCR_RFSR(cfg->fsync_early) |
			ESAI_RCR_RFSL(cfg->fsync_is_bit_wide) |
			ESAI_RCR_RSWS(cfg->slot_format) |
			ESAI_RCR_RMOD(cfg->mode) |
			ESAI_RCR_RWA(!cfg->data_left_aligned) |
			ESAI_RCR_RSHFD(cfg->data_order);

		base->ECR &= ~(ESAI_ECR_ERI_MASK |
			       ESAI_ECR_ERO_MASK);

		base->ECR |= ESAI_ECR_ERI(cfg->hclk_src) |
			ESAI_ECR_ERO(cfg->hclk_bypass);

		base->RFCR &= ~(ESAI_RFCR_RFWM_MASK | ESAI_RFCR_RWA_MASK);
		base->RFCR |= ESAI_RFCR_RFWM(cfg->watermark) |
			ESAI_RFCR_RWA(cfg->word_alignment);

		EASI_RxSetSlotMask(base, cfg->slot_mask);
	}
}

static int esai_config_set(const struct device *dev,
			   const struct dai_config *cfg,
			   const void *bespoke_data)
{
	const struct esai_bespoke_config *bespoke;
	struct esai_data *data;
	const struct esai_config *esai_cfg;
	struct esai_transceiver_config tx_config;
	struct esai_transceiver_config rx_config;
	ESAI_Type *base;
	int ret;

	if (!cfg || !bespoke_data) {
		return -EINVAL;
	}

	if (cfg->type != DAI_IMX_ESAI) {
		LOG_ERR("wrong DAI type: %d", cfg->type);
		return -EINVAL;
	}

	data = dev->data;
	esai_cfg = dev->config;
	bespoke = bespoke_data;
	base = UINT_TO_ESAI(data->regmap);

	/* config_set() configures both the transmitter and the receiver.
	 * As such, the following state transitions make sure that the
	 * directions are stopped. This means that they can be safely
	 * reset and re-configured.
	 */
	ret = esai_update_state(data, DAI_DIR_TX, DAI_STATE_READY);
	if (ret < 0) {
		LOG_ERR("failed to update TX state");
		return ret;
	}

	ret = esai_update_state(data, DAI_DIR_RX, DAI_STATE_READY);
	if (ret < 0) {
		LOG_ERR("failed to update RX state");
		return ret;
	}

	ESAI_Enable(base, true);

	/* disconnect all ESAI pins */
	base->PCRC &= ~ESAI_PCRC_PC_MASK;
	base->PRRC &= ~ESAI_PRRC_PDC_MASK;

	/* go back to known configuration through reset */
	ESAI_Reset(UINT_TO_ESAI(data->regmap));

	/* get default configuration */
	esai_get_xceiver_default_config(&tx_config);

	/* TODO: for now, only network mode is supported */
	tx_config.fsync_div = bespoke->tdm_slots;

	/* clock provider configuration */
	ret = esai_get_clk_provider_config(cfg, &tx_config);
	if (ret < 0) {
		return ret;
	}

	/* protocol configuration */
	ret = esai_get_proto_config(cfg, &tx_config);
	if (ret < 0) {
		return ret;
	}

	/* clock inversion configuration */
	ret = esai_get_clk_inversion_config(cfg, &tx_config);
	if (ret < 0) {
		return ret;
	}

	ret = esai_get_slot_format(bespoke->tdm_slot_width,
				   esai_cfg->word_width, &tx_config);
	if (ret < 0) {
		return ret;
	}

	tx_config.word_alignment = ESAI_WORD_ALIGNMENT(esai_cfg->word_width);

	/* duplicate TX configuration */
	memcpy(&rx_config, &tx_config, sizeof(tx_config));

	/* parse clock configuration from DTS. This will overwrite
	 * directions set in bespoke data.
	 */
	ret = esai_parse_clock_config(esai_cfg, &tx_config, &rx_config);
	if (ret < 0) {
		return ret;
	}

	/* compute TX clock configuration */
	ret = esai_get_clock_rate_config(bespoke->mclk_rate, bespoke->mclk_rate,
					 bespoke->bclk_rate,
					 !ESAI_PIN_IS_USED(data, ESAI_PIN_HCKT),
					 tx_config.bclk_dir,
					 &tx_config);
	if (ret < 0) {
		return ret;
	}

	/* compute RX clock configuration */
	ret = esai_get_clock_rate_config(bespoke->mclk_rate, bespoke->mclk_rate,
					 bespoke->bclk_rate,
					 !ESAI_PIN_IS_USED(data, ESAI_PIN_HCKR),
					 rx_config.bclk_dir,
					 &rx_config);
	if (ret < 0) {
		return ret;
	}


	tx_config.watermark = esai_cfg->tx_fifo_watermark;
	rx_config.watermark = esai_cfg->rx_fifo_watermark;

	tx_config.slot_mask = bespoke->tx_slots;
	rx_config.slot_mask = bespoke->rx_slots;

	LOG_DBG("dumping TX configuration");
	esai_dump_xceiver_config(&tx_config);

	LOG_DBG("dumping RX configuration");
	esai_dump_xceiver_config(&rx_config);

	/* enable ESAI to allow committing the configurations */
	ESAI_Enable(base, true);

	esai_dump_register_data(base);

	esai_commit_config(base, DAI_DIR_TX, &tx_config);
	esai_commit_config(base, DAI_DIR_RX, &rx_config);

	/* allow each TX data register to be initialized from TX FIFO */
	base->TFCR |= ESAI_TFCR_TIEN_MASK;

	/* enable FIFO usage
	 *
	 * TODO: for now, only 1 data line per direction is supported.
	 */
	esai_tx_rx_enable_disable_fifo_usage(base, DAI_DIR_TX, BIT(0), true);
	esai_tx_rx_enable_disable_fifo_usage(base, DAI_DIR_RX, BIT(0), true);

	/* re-connect pins based on DTS pin configuration */
	base->PCRC = data->pcrc;
	base->PRRC = data->prrc;

	data->cfg.rate = bespoke->fsync_rate;
	data->cfg.channels = bespoke->tdm_slots;

	esai_dump_register_data(base);

	return 0;
}

static int esai_config_get(const struct device *dev,
			   struct dai_config *cfg,
			   enum dai_dir dir)
{
	struct esai_data *data = dev->data;

	if (!cfg) {
		return -EINVAL;
	}

	memcpy(cfg, &data->cfg, sizeof(*cfg));

	return 0;
}

static int esai_trigger_start(const struct device *dev, enum dai_dir dir)
{
	struct esai_data *data;
	ESAI_Type *base;
	int ret, i;

	data = dev->data;
	base = UINT_TO_ESAI(data->regmap);

	ret = esai_update_state(data, dir, DAI_STATE_RUNNING);
	if (ret < 0) {
		LOG_ERR("failed to transition to RUNNING");
		return -EINVAL;
	}

	LOG_DBG("starting direction %d", dir);

	/* enable the FIFO */
	esai_tx_rx_enable_disable_fifo(base, dir, true);

	/* TODO: without this, the ESAI won't enter underrun
	 * but playing a song while doing pause-resume very
	 * fast seems to result in a degraded sound quality?
	 *
	 * TODO: for multiple channels, this needs to be changed.
	 */
	if (dir == DAI_DIR_TX) {
		for (i = 0; i < 1; i++) {
			ESAI_WriteData(base, 0x0);
		}
	}

	/* enable the transmitter/receiver */
	esai_tx_rx_enable_disable(base, dir, BIT(0), true);

	return 0;
}

static int esai_trigger_stop(const struct device *dev, enum dai_dir dir)
{
	struct esai_data *data;
	int ret;
	ESAI_Type *base;

	data = dev->data;
	base = UINT_TO_ESAI(data->regmap);

	ret = esai_update_state(data, dir, DAI_STATE_STOPPING);
	if (ret < 0) {
		LOG_ERR("failed to transition to STOPPING");
		return -EINVAL;
	}

	LOG_DBG("stopping direction %d", dir);

	/* disable transmitter/receiver */
	esai_tx_rx_enable_disable(base, dir, BIT(0), false);

	/* disable FIFO */
	esai_tx_rx_enable_disable_fifo(base, dir, false);

	return 0;
}

static int esai_trigger(const struct device *dev,
			enum dai_dir dir,
			enum dai_trigger_cmd cmd)
{
	/* TX/RX should be triggered individually */
	if (dir != DAI_DIR_RX && dir != DAI_DIR_TX) {
		LOG_ERR("invalid direction: %d", dir);
		return -EINVAL;
	}

	switch (cmd) {
	case DAI_TRIGGER_START:
		return esai_trigger_start(dev, dir);
	case DAI_TRIGGER_PAUSE:
	case DAI_TRIGGER_STOP:
		return esai_trigger_stop(dev, dir);
	case DAI_TRIGGER_PRE_START:
	case DAI_TRIGGER_COPY:
		/* nothing to do here, return success code */
		return 0;
	default:
		LOG_ERR("invalid trigger command: %d", cmd);
		return -EINVAL;
	}

	return 0;
}

static const struct dai_properties
	*esai_get_properties(const struct device *dev, enum dai_dir dir, int stream_id)
{
	const struct esai_config *cfg = dev->config;

	switch (dir) {
	case DAI_DIR_RX:
		return cfg->rx_props;
	case DAI_DIR_TX:
		return cfg->tx_props;
	default:
		LOG_ERR("invalid direction: %d", dir);
		return NULL;
	}
}

static int esai_probe(const struct device *dev)
{
	/* nothing to be done here but mandatory to implement */
	return 0;
}

static int esai_remove(const struct device *dev)
{
	/* nothing to be done here but mandatory to implement */
	return 0;
}

static const struct dai_driver_api esai_api = {
	.config_set = esai_config_set,
	.config_get = esai_config_get,
	.trigger = esai_trigger,
	.get_properties = esai_get_properties,
	.probe = esai_probe,
	.remove = esai_remove,
};

static int esai_init(const struct device *dev)
{
	const struct esai_config *cfg;
	struct esai_data *data;
	int ret;

	cfg = dev->config;
	data = dev->data;

	device_map(&data->regmap, cfg->regmap_phys, cfg->regmap_size, K_MEM_CACHE_NONE);

	ESAI_Reset(UINT_TO_ESAI(data->regmap));

	ret = esai_parse_pinmodes(cfg, data);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define ESAI_INIT(inst)							\
									\
BUILD_ASSERT(ESAI_TX_FIFO_WATERMARK(inst) >= 1 &&			\
	     ESAI_TX_FIFO_WATERMARK(inst) <= _ESAI_FIFO_DEPTH(inst),	\
	     "invalid TX watermark value");				\
									\
BUILD_ASSERT(ESAI_RX_FIFO_WATERMARK(inst) >= 1 &&			\
	     ESAI_RX_FIFO_WATERMARK(inst) <= _ESAI_FIFO_DEPTH(inst),	\
	     "invalid RX watermark value");				\
									\
BUILD_ASSERT(ESAI_FIFO_DEPTH(inst) >= 1 &&				\
	     ESAI_FIFO_DEPTH(inst) <= _ESAI_FIFO_DEPTH(inst),		\
	     "invalid FIFO depth value");				\
									\
BUILD_ASSERT(ESAI_WORD_WIDTH(inst) == 8 ||				\
	     ESAI_WORD_WIDTH(inst) == 12 ||				\
	     ESAI_WORD_WIDTH(inst) == 16 ||				\
	     ESAI_WORD_WIDTH(inst) == 20 ||				\
	     ESAI_WORD_WIDTH(inst) == 24,				\
	     "invalid word width value");				\
									\
static const struct dai_properties esai_tx_props_##inst = {		\
	.fifo_address = ESAI_TX_FIFO_BASE(inst),			\
	.fifo_depth = ESAI_FIFO_DEPTH(inst) * 4,			\
	.dma_hs_id = ESAI_TX_RX_DMA_HANDSHAKE(inst, tx),		\
};									\
									\
static const struct dai_properties esai_rx_props_##inst = {		\
	.fifo_address = ESAI_RX_FIFO_BASE(inst),			\
	.fifo_depth = ESAI_FIFO_DEPTH(inst) * 4,			\
	.dma_hs_id = ESAI_TX_RX_DMA_HANDSHAKE(inst, rx),		\
};									\
									\
static uint32_t	pinmodes_##inst[] =					\
	DT_INST_PROP_OR(inst, esai_pin_modes, {});			\
									\
BUILD_ASSERT(ARRAY_SIZE(pinmodes_##inst) % 2 == 0,			\
	     "bad pinmask array size");					\
									\
static uint32_t clock_cfg_##inst[] =					\
	DT_INST_PROP_OR(inst, esai_clock_configuration, {});		\
									\
BUILD_ASSERT(ARRAY_SIZE(clock_cfg_##inst) % 2 == 0,			\
	     "bad clock configuration array size");			\
									\
static struct esai_config esai_config_##inst = {			\
	.regmap_phys = DT_INST_REG_ADDR(inst),				\
	.regmap_size = DT_INST_REG_SIZE(inst),				\
	.tx_props = &esai_tx_props_##inst,				\
	.rx_props = &esai_rx_props_##inst,				\
	.tx_fifo_watermark = ESAI_TX_FIFO_WATERMARK(inst),		\
	.rx_fifo_watermark = ESAI_RX_FIFO_WATERMARK(inst),		\
	.word_width = ESAI_WORD_WIDTH(inst),				\
	.pinmodes = pinmodes_##inst,					\
	.pinmodes_size = ARRAY_SIZE(pinmodes_##inst),			\
	.clock_cfg = clock_cfg_##inst,					\
	.clock_cfg_size = ARRAY_SIZE(clock_cfg_##inst),			\
};									\
									\
static struct esai_data esai_data_##inst = {				\
	.cfg.type = DAI_IMX_ESAI,					\
	.cfg.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),		\
};									\
									\
DEVICE_DT_INST_DEFINE(inst, &esai_init, NULL,				\
		      &esai_data_##inst, &esai_config_##inst,		\
		      POST_KERNEL, CONFIG_DAI_INIT_PRIORITY,		\
		      &esai_api);					\

DT_INST_FOREACH_STATUS_OKAY(ESAI_INIT);

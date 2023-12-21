/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "sai.h"

/* used for binding the driver */
#define DT_DRV_COMPAT nxp_dai_sai

#define SAI_TX_RX_HW_DISABLE_TIMEOUT 50

/* TODO list:
 *
 * 1) No busy waiting should be performed in any of the operations.
 * In the case of STOP(), the operation should be split into TRIGGER_STOP
 * and TRIGGER_POST_STOP. (SOF)
 *
 * 2) The SAI ISR should stop the SAI whenever a FIFO error interrupt
 * is raised.
 *
 * 3) Transmitter/receiver may remain enabled after sai_tx_rx_disable().
 * Fix this.
 */

#ifdef CONFIG_SAI_HAS_MCLK_CONFIG_OPTION
/* note: i.MX8 boards don't seem to support the MICS field in the MCR
 * register. As such, the MCLK source field of sai_master_clock_t is
 * useless. I'm assuming the source is selected through xCR2's MSEL.
 *
 * TODO: for now, this function will set MCR's MSEL to the same value
 * as xCR2's MSEL or, rather, to the same MCLK as the one used for
 * generating BCLK. Is there a need to support different MCLKs in
 * xCR2 and MCR?
 */
static int sai_mclk_config(const struct device *dev,
			   sai_bclk_source_t bclk_source,
			   const struct sai_bespoke_config *bespoke)
{
	const struct sai_config *cfg;
	struct sai_data *data;
	sai_master_clock_t mclk_config;
	uint32_t msel, mclk_rate;
	int ret;

	cfg = dev->config;
	data = dev->data;

	mclk_config.mclkOutputEnable = cfg->mclk_is_output;

	ret = get_msel(bclk_source, &msel);
	if (ret < 0) {
		LOG_ERR("invalid MCLK source %d for MSEL", bclk_source);
		return ret;
	}

	/* get MCLK's rate */
	ret = get_mclk_rate(&cfg->clk_data, bclk_source, &mclk_rate);
	if (ret < 0) {
		LOG_ERR("failed to query MCLK's rate");
		return ret;
	}

	LOG_DBG("source MCLK is %u", mclk_rate);

	LOG_DBG("target MCLK is %u", bespoke->mclk_rate);

	/* source MCLK rate */
	mclk_config.mclkSourceClkHz = mclk_rate;

	/* target MCLK rate */
	mclk_config.mclkHz = bespoke->mclk_rate;

	/* commit configuration */
	SAI_SetMasterClockConfig(UINT_TO_I2S(data->regmap), &mclk_config);

	set_msel(data->regmap, msel);

	return 0;
}
#endif /* CONFIG_SAI_HAS_MCLK_CONFIG_OPTION */

void sai_isr(const void *parameter)
{
	const struct device *dev;
	struct sai_data *data;

	dev = parameter;
	data = dev->data;

	/* check for TX FIFO error */
	if (SAI_TX_RX_STATUS_IS_SET(DAI_DIR_TX, data->regmap, kSAI_FIFOErrorFlag)) {
		LOG_ERR("FIFO underrun detected");
		/* TODO: this will crash the program and should be addressed as
		 * mentioned in TODO list's 2).
		 */
		z_irq_spurious(NULL);
	}

	/* check for RX FIFO error */
	if (SAI_TX_RX_STATUS_IS_SET(DAI_DIR_RX, data->regmap, kSAI_FIFOErrorFlag)) {
		LOG_ERR("FIFO overrun detected");
		/* TODO: this will crash the program and should be addressed as
		 * mentioned in TODO list's 2).
		 */
		z_irq_spurious(NULL);
	}
}

static int sai_config_get(const struct device *dev,
			  struct dai_config *cfg,
			  enum dai_dir dir)
{
	struct sai_data *data = dev->data;

	/* dump content of the DAI configuration */
	memcpy(cfg, &data->cfg, sizeof(*cfg));

	return 0;
}

static const struct dai_properties
	*sai_get_properties(const struct device *dev, enum dai_dir dir, int stream_id)
{
	const struct sai_config *cfg = dev->config;

	switch (dir) {
	case DAI_DIR_RX:
		return cfg->rx_props;
	case DAI_DIR_TX:
		return cfg->tx_props;
	default:
		LOG_ERR("invalid direction: %d", dir);
		return NULL;
	}

	CODE_UNREACHABLE;
}

static int sai_config_set(const struct device *dev,
			  const struct dai_config *cfg,
			  const void *bespoke_data)
{
	const struct sai_bespoke_config *bespoke;
	sai_transceiver_t *rx_config, *tx_config;
	struct sai_data *data;
	const struct sai_config *sai_cfg;
	int ret;

	if (cfg->type != DAI_IMX_SAI) {
		LOG_ERR("wrong DAI type: %d", cfg->type);
		return -EINVAL;
	}

	bespoke = bespoke_data;
	data = dev->data;
	sai_cfg = dev->config;
	rx_config = &data->rx_config;
	tx_config = &data->tx_config;

	/* since this function configures the transmitter AND the receiver, that
	 * means both of them need to be stopped. As such, doing the state
	 * transition here will also result in a state check.
	 */
	ret = sai_update_state(DAI_DIR_TX, data, DAI_STATE_READY);
	if (ret < 0) {
		LOG_ERR("failed to update TX state. Reason: %d", ret);
		return ret;
	}

	ret = sai_update_state(DAI_DIR_RX, data, DAI_STATE_READY);
	if (ret < 0) {
		LOG_ERR("failed to update RX state. Reason: %d", ret);
		return ret;
	}

	/* condition: BCLK = FSYNC * TDM_SLOT_WIDTH * TDM_SLOTS */
	if (bespoke->bclk_rate !=
	    (bespoke->fsync_rate * bespoke->tdm_slot_width * bespoke->tdm_slots)) {
		LOG_ERR("bad BCLK value: %d", bespoke->bclk_rate);
		return -EINVAL;
	}

	/* TODO: this should be removed if we're to support sw channels != hw channels */
	if (count_leading_zeros(~bespoke->tx_slots) != bespoke->tdm_slots ||
	    count_leading_zeros(~bespoke->rx_slots) != bespoke->tdm_slots) {
		LOG_ERR("number of TX/RX slots doesn't match number of TDM slots");
		return -EINVAL;
	}

	/* get default configurations */
	get_bclk_default_config(&tx_config->bitClock);
	get_fsync_default_config(&tx_config->frameSync);
	get_serial_default_config(&tx_config->serialData);
	get_fifo_default_config(&tx_config->fifo);

	/* note1: this may be obvious but enabling multiple SAI
	 * channels (or data lines) may lead to FIFO starvation/
	 * overflow if data is not written/read from the respective
	 * TDR/RDR registers.
	 *
	 * note2: the SAI data line should be enabled based on
	 * the direction (TX/RX) we're enabling. Enabling the
	 * data line for the opposite direction will lead to FIFO
	 * overrun/underrun when working with a SYNC direction.
	 *
	 * note3: the TX/RX data line shall be enabled/disabled
	 * via the sai_trigger_() suite to avoid scenarios in
	 * which one configures both direction but only starts
	 * the SYNC direction which would lead to a FIFO underrun.
	 */
	tx_config->channelMask = 0x0;

	/* TODO: for now, only MCLK1 is supported */
	tx_config->bitClock.bclkSource = kSAI_BclkSourceMclkOption1;

	/* FSYNC is asserted for tdm_slot_width BCLKs */
	tx_config->frameSync.frameSyncWidth = bespoke->tdm_slot_width;

	/* serial data common configuration */
	tx_config->serialData.dataWord0Length = bespoke->tdm_slot_width;
	tx_config->serialData.dataWordNLength = bespoke->tdm_slot_width;
	tx_config->serialData.dataFirstBitShifted = bespoke->tdm_slot_width;
	tx_config->serialData.dataWordNum = bespoke->tdm_slots;

	/* clock provider configuration */
	switch (cfg->format & DAI_FORMAT_CLOCK_PROVIDER_MASK) {
	case DAI_CBP_CFP:
		tx_config->masterSlave = kSAI_Slave;
		break;
	case DAI_CBC_CFC:
		tx_config->masterSlave = kSAI_Master;
		break;
	case DAI_CBC_CFP:
	case DAI_CBP_CFC:
		LOG_ERR("unsupported provider configuration: %d",
			cfg->format & DAI_FORMAT_CLOCK_PROVIDER_MASK);
		return -ENOTSUP;
	default:
		LOG_ERR("invalid provider configuration: %d",
			cfg->format & DAI_FORMAT_CLOCK_PROVIDER_MASK);
		return -EINVAL;
	}

	LOG_DBG("SAI is in %d mode", tx_config->masterSlave);

	/* protocol configuration */
	switch (cfg->format & DAI_FORMAT_PROTOCOL_MASK) {
	case DAI_PROTO_I2S:
		/* BCLK is active LOW */
		tx_config->bitClock.bclkPolarity = kSAI_PolarityActiveLow;
		/* FSYNC is active LOW */
		tx_config->frameSync.frameSyncPolarity = kSAI_PolarityActiveLow;
		break;
	case DAI_PROTO_DSP_A:
		/* FSYNC is asserted for a single BCLK */
		tx_config->frameSync.frameSyncWidth = 1;
		/* BCLK is active LOW */
		tx_config->bitClock.bclkPolarity = kSAI_PolarityActiveLow;
		break;
	default:
		LOG_ERR("unsupported DAI protocol: %d",
			cfg->format & DAI_FORMAT_PROTOCOL_MASK);
		return -EINVAL;
	}

	LOG_DBG("SAI uses protocol: %d",
		cfg->format & DAI_FORMAT_PROTOCOL_MASK);

	/* clock inversion configuration */
	switch (cfg->format & DAI_FORMAT_CLOCK_INVERSION_MASK) {
	case DAI_INVERSION_IB_IF:
		SAI_INVERT_POLARITY(tx_config->bitClock.bclkPolarity);
		SAI_INVERT_POLARITY(tx_config->frameSync.frameSyncPolarity);
		break;
	case DAI_INVERSION_IB_NF:
		SAI_INVERT_POLARITY(tx_config->bitClock.bclkPolarity);
		break;
	case DAI_INVERSION_NB_IF:
		SAI_INVERT_POLARITY(tx_config->frameSync.frameSyncPolarity);
		break;
	case DAI_INVERSION_NB_NF:
		/* nothing to do here */
		break;
	default:
		LOG_ERR("invalid clock inversion configuration: %d",
			cfg->format & DAI_FORMAT_CLOCK_INVERSION_MASK);
		return -EINVAL;
	}

	LOG_DBG("FSYNC polarity: %d", tx_config->frameSync.frameSyncPolarity);
	LOG_DBG("BCLK polarity: %d", tx_config->bitClock.bclkPolarity);

	/* duplicate TX configuration */
	memcpy(rx_config, tx_config, sizeof(sai_transceiver_t));

	tx_config->serialData.dataMaskedWord = ~bespoke->tx_slots;
	rx_config->serialData.dataMaskedWord = ~bespoke->rx_slots;

	tx_config->fifo.fifoWatermark = sai_cfg->tx_fifo_watermark - 1;
	rx_config->fifo.fifoWatermark = sai_cfg->rx_fifo_watermark - 1;

	LOG_DBG("RX watermark: %d", sai_cfg->rx_fifo_watermark);
	LOG_DBG("TX watermark: %d", sai_cfg->tx_fifo_watermark);

	/* TODO: for now, the only supported operation mode is RX sync with TX.
	 * Is there a need to support other modes?
	 */
	tx_config->syncMode = kSAI_ModeAsync;
	rx_config->syncMode = kSAI_ModeSync;

	/* commit configuration */
	SAI_RxSetConfig(UINT_TO_I2S(data->regmap), rx_config);
	SAI_TxSetConfig(UINT_TO_I2S(data->regmap), tx_config);

	/* a few notes here:
	 *	1) TX and RX operate in the same mode: master or slave.
	 *	2) Setting BCLK's rate needs to be performed explicitly
	 *	since SetConfig() doesn't do it for us.
	 *	3) Setting BCLK's rate has to be performed after the
	 *	SetConfig() call as that resets the SAI registers.
	 */
	if (tx_config->masterSlave == kSAI_Master) {
		SAI_TxSetBitClockRate(UINT_TO_I2S(data->regmap), bespoke->mclk_rate,
				      bespoke->fsync_rate, bespoke->tdm_slot_width,
				      bespoke->tdm_slots);

		SAI_RxSetBitClockRate(UINT_TO_I2S(data->regmap), bespoke->mclk_rate,
				      bespoke->fsync_rate, bespoke->tdm_slot_width,
				      bespoke->tdm_slots);
	}

#ifdef CONFIG_SAI_HAS_MCLK_CONFIG_OPTION
	ret = sai_mclk_config(dev, tx_config->bitClock.bclkSource, bespoke);
	if (ret < 0) {
		LOG_ERR("failed to set MCLK configuration");
		return ret;
	}
#endif /* CONFIG_SAI_HAS_MCLK_CONFIG_OPTION */

	/* this is needed so that rates different from FSYNC_RATE
	 * will not be allowed.
	 *
	 * this is because the hardware is configured to match
	 * the topology rates so attempting to play a file using
	 * a different rate from the one configured in the hardware
	 * doesn't work properly.
	 *
	 * if != 0, SOF will raise an error if the PCM rate is
	 * different than the hardware rate (a.k.a this one).
	 */
	data->cfg.rate = bespoke->fsync_rate;
	/* SOF note: we don't support a variable number of channels
	 * at the moment so leaving the number of channels as 0 is
	 * unnecessary and leads to issues (e.g: the mixer buffers
	 * use this value to set the number of channels so having
	 * a 0 as this value leads to mixer buffers having 0 channels,
	 * which, in turn, leads to the DAI ending up with 0 channels,
	 * thus resulting in an error)
	 */
	data->cfg.channels = bespoke->tdm_slots;

	sai_dump_register_data(data->regmap);

	return 0;
}

/* SOF note: please be very careful with this function as it does
 * busy waiting and may mess up your timing in time critial applications
 * (especially with timer domain). If this becomes unusable, the busy
 * waiting should be removed altogether and the HW state check should
 * be performed in sai_trigger_start() or in sai_config_set().
 *
 * TODO: seems like the transmitter still remains active (even if 1ms
 * has passed after doing a sai_trigger_stop()!). Most likely this is
 * because sai_trigger_stop() immediately stops the data line w/o
 * checking the HW state of the transmitter/receiver. As such, to get
 * rid of the busy waiting, the STOP operation may have to be split into
 * 2 operations: TRIG_STOP and TRIG_POST_STOP.
 */
static int sai_tx_rx_disable(struct sai_data *data, enum dai_dir dir)
{
	bool ret;

	/* sai_disable() should never be called from ISR context
	 * as it does some busy waiting.
	 */
	if (k_is_in_isr()) {
		LOG_ERR("sai_disable() should never be called from ISR context");
		return -EINVAL;
	}

	if ((dir == DAI_DIR_TX && !data->rx_enabled) || dir == DAI_DIR_RX) {
		/* VERY IMPORTANT: DO NOT use SAI_TxEnable/SAI_RxEnable
		 * here as they do not disable the ASYNC direction.
		 * Since the software logic assures that the ASYNC direction
		 * is not disabled before the SYNC direction, we can force
		 * the disablement of the given direction.
		 */
		sai_tx_rx_force_disable(dir, data->regmap);

		/* please note the difference between the transmitter/receiver's
		 * hardware states and their software states. The software
		 * states can be obtained by reading data->tx/rx_enabled, while
		 * the hardware states can be obtained by reading TCSR/RCSR. The
		 * hadrware state can actually differ from the software state.
		 * Here, we're interested in reading the hardware state which
		 * indicates if the transmitter/receiver was actually disabled
		 * or not.
		 */
		ret = WAIT_FOR(!SAI_TX_RX_IS_HW_ENABLED(dir, data->regmap),
			       SAI_TX_RX_HW_DISABLE_TIMEOUT, k_busy_wait(1));
		if (!ret) {
			LOG_ERR("timed out while waiting for dir %d disable", dir);
			return -ETIMEDOUT;
		}
	}

	/* if TX wasn't explicitly enabled (via sai_trigger_start(TX))
	 * then that means it was enabled by a sai_trigger_start(RX). As
	 * such, data->tx_enabled will be false.
	 */
	if (dir == DAI_DIR_RX && !data->tx_enabled) {
		sai_tx_rx_force_disable(DAI_DIR_TX, data->regmap);

		ret = WAIT_FOR(!SAI_TX_RX_IS_HW_ENABLED(DAI_DIR_TX, data->regmap),
			       SAI_TX_RX_HW_DISABLE_TIMEOUT, k_busy_wait(1));
		if (!ret) {
			LOG_ERR("timed out while waiting for dir TX disable");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int sai_trigger_pause(const struct device *dev,
			     enum dai_dir dir)
{
	struct sai_data *data;
	int ret;

	data = dev->data;

	if (dir != DAI_DIR_RX && dir != DAI_DIR_TX) {
		LOG_ERR("invalid direction: %d", dir);
		return -EINVAL;
	}

	/* attempt to change state */
	ret = sai_update_state(dir, data, DAI_STATE_PAUSED);
	if (ret < 0) {
		LOG_ERR("failed to transition to PAUSED from %d. Reason: %d",
			sai_get_state(dir, data), ret);
		return ret;
	}

	LOG_DBG("pause on direction %d", dir);

	ret = sai_tx_rx_disable(data, dir);
	if (ret < 0) {
		return ret;
	}

	/* update the software state of TX/RX */
	sai_tx_rx_sw_enable_disable(dir, data, false);

	return 0;
}

static int sai_trigger_stop(const struct device *dev,
			    enum dai_dir dir)
{
	struct sai_data *data;
	int ret;
	uint32_t old_state;

	data = dev->data;
	old_state = sai_get_state(dir, data);

	if (dir != DAI_DIR_RX && dir != DAI_DIR_TX) {
		LOG_ERR("invalid direction: %d", dir);
		return -EINVAL;
	}

	/* attempt to change state */
	ret = sai_update_state(dir, data, DAI_STATE_STOPPING);
	if (ret < 0) {
		LOG_ERR("failed to transition to STOPPING from %d. Reason: %d",
			sai_get_state(dir, data), ret);
		return ret;
	}

	LOG_DBG("stop on direction %d", dir);

	if (old_state == DAI_STATE_PAUSED) {
		/* if SAI was previously paused then all that's
		 * left to do is disable the DMA requests and
		 * the data line.
		 */
		goto out_dline_disable;
	}

	ret = sai_tx_rx_disable(data, dir);
	if (ret < 0) {
		return ret;
	}

	/* update the software state of TX/RX */
	sai_tx_rx_sw_enable_disable(dir, data, false);

out_dline_disable:
	/* disable TX/RX data line */
	sai_tx_rx_set_dline_mask(dir, data->regmap, 0x0);

	/* disable DMA requests */
	SAI_TX_RX_DMA_ENABLE_DISABLE(dir, data->regmap, false);

	/* disable error interrupt */
	SAI_TX_RX_ENABLE_DISABLE_IRQ(dir, data->regmap,
				     kSAI_FIFOErrorInterruptEnable, false);

	return 0;
}

static int sai_trigger_start(const struct device *dev,
			     enum dai_dir dir)
{
	struct sai_data *data;
	uint32_t old_state;
	int ret;

	data = dev->data;
	old_state = sai_get_state(dir, data);

	/* TX and RX should be triggered independently */
	if (dir != DAI_DIR_RX && dir != DAI_DIR_TX) {
		LOG_ERR("invalid direction: %d", dir);
		return -EINVAL;
	}

	/* attempt to change state */
	ret = sai_update_state(dir, data, DAI_STATE_RUNNING);
	if (ret < 0) {
		LOG_ERR("failed to transition to RUNNING from %d. Reason: %d",
			sai_get_state(dir, data), ret);
		return ret;
	}

	if (old_state == DAI_STATE_PAUSED) {
		/* if the SAI has been paused then there's no
		 * point in issuing a software reset. As such,
		 * skip this part and go directly to the TX/RX
		 * enablement.
		 */
		goto out_enable_tx_rx;
	}

	LOG_DBG("start on direction %d", dir);

	if (dir == DAI_DIR_RX) {
		/* this is fine because TX is async so it won't be
		 * affected by an RX software reset.
		 */
		SAI_TX_RX_SW_RESET(dir, data->regmap);

		/* do a TX software reset only if not already enabled */
		if (!data->tx_enabled) {
			SAI_TX_RX_SW_RESET(DAI_DIR_TX, data->regmap);
		}
	} else {
		/* a software reset should be issued for TX
		 * only if RX was not already enabled.
		 */
		if (!data->rx_enabled) {
			SAI_TX_RX_SW_RESET(dir, data->regmap);
		}
	}

	/* enable error interrupt */
	SAI_TX_RX_ENABLE_DISABLE_IRQ(dir, data->regmap,
				     kSAI_FIFOErrorInterruptEnable, true);

	/* TODO: is there a need to write some words to the FIFO to avoid starvation? */

	/* TODO: for now, only DMA mode is supported */
	SAI_TX_RX_DMA_ENABLE_DISABLE(dir, data->regmap, true);

	/* enable TX/RX data line. This translates to TX_DLINE0/RX_DLINE0
	 * being enabled.
	 *
	 * TODO: for now we only support 1 data line per direction.
	 */
	sai_tx_rx_set_dline_mask(dir, data->regmap, 0x1);

out_enable_tx_rx:
	/* this will also enable the async side */
	SAI_TX_RX_ENABLE_DISABLE(dir, data->regmap, true);

	/* update the software state of TX/RX */
	sai_tx_rx_sw_enable_disable(dir, data, true);

	return 0;
}

static int sai_trigger(const struct device *dev,
		       enum dai_dir dir,
		       enum dai_trigger_cmd cmd)
{
	switch (cmd) {
	case DAI_TRIGGER_START:
		return sai_trigger_start(dev, dir);
	case DAI_TRIGGER_PAUSE:
		return sai_trigger_pause(dev, dir);
	case DAI_TRIGGER_STOP:
		return sai_trigger_stop(dev, dir);
	case DAI_TRIGGER_PRE_START:
	case DAI_TRIGGER_COPY:
		/* COPY and PRE_START don't require the SAI
		 * driver to do anything at the moment so
		 * mark them as successful via a NULL return
		 *
		 * note: although the rest of the unhandled
		 * trigger commands may be valid, return
		 * an error code for them as they aren't
		 * implemented ATM (since they're not
		 * mandatory for the SAI driver to work).
		 */
		return 0;
	default:
		LOG_ERR("invalid trigger command: %d", cmd);
		return -EINVAL;
	}

	CODE_UNREACHABLE;
}

static int sai_probe(const struct device *dev)
{
	/* nothing to be done here but sadly mandatory to implement */
	return 0;
}

static int sai_remove(const struct device *dev)
{
	/* nothing to be done here but sadly mandatory to implement */
	return 0;
}

static const struct dai_driver_api sai_api = {
	.config_set = sai_config_set,
	.config_get = sai_config_get,
	.trigger = sai_trigger,
	.get_properties = sai_get_properties,
	.probe = sai_probe,
	.remove = sai_remove,
};

static int sai_init(const struct device *dev)
{
	const struct sai_config *cfg;
	struct sai_data *data;
	int i, ret;

	cfg = dev->config;
	data = dev->data;

	device_map(&data->regmap, cfg->regmap_phys, cfg->regmap_size, K_MEM_CACHE_NONE);

	/* enable clocks if any */
	for (i = 0; i < cfg->clk_data.clock_num; i++) {
		ret = clock_control_on(cfg->clk_data.dev,
				       UINT_TO_POINTER(cfg->clk_data.clocks[i]));
		if (ret < 0) {
			return ret;
		}

		LOG_DBG("clock %s has been ungated", cfg->clk_data.clock_names[i]);
	}

	/* set TX/RX default states */
	data->tx_state = DAI_STATE_NOT_READY;
	data->rx_state = DAI_STATE_NOT_READY;

	/* register ISR and enable IRQ */
	cfg->irq_config();

	return 0;
}

#define SAI_INIT(inst)								\
										\
BUILD_ASSERT(SAI_FIFO_DEPTH(inst) > 0 &&					\
	     SAI_FIFO_DEPTH(inst) <= _SAI_FIFO_DEPTH(inst),			\
	     "invalid FIFO depth");						\
										\
BUILD_ASSERT(SAI_RX_FIFO_WATERMARK(inst) > 0 &&					\
	     SAI_RX_FIFO_WATERMARK(inst) <= _SAI_FIFO_DEPTH(inst),		\
	     "invalid RX FIFO watermark");					\
										\
BUILD_ASSERT(SAI_TX_FIFO_WATERMARK(inst) > 0 &&					\
	     SAI_TX_FIFO_WATERMARK(inst) <= _SAI_FIFO_DEPTH(inst),		\
	     "invalid TX FIFO watermark");					\
										\
BUILD_ASSERT(IS_ENABLED(CONFIG_SAI_HAS_MCLK_CONFIG_OPTION) ||			\
	     !DT_INST_PROP(inst, mclk_is_output),				\
	     "SAI doesn't support MCLK config but mclk_is_output is specified");\
										\
BUILD_ASSERT(SAI_TX_SYNC_MODE(inst) != SAI_RX_SYNC_MODE(inst) ||		\
	     SAI_TX_SYNC_MODE(inst) != kSAI_ModeSync,				\
	     "transmitter and receiver can't be both SYNC with each other");	\
										\
static const struct dai_properties sai_tx_props_##inst = {			\
	.fifo_address = SAI_TX_FIFO_BASE(inst),					\
	.fifo_depth = SAI_FIFO_DEPTH(inst) * CONFIG_SAI_FIFO_WORD_SIZE,		\
	.dma_hs_id = SAI_TX_DMA_MUX(inst),					\
};										\
										\
static const struct dai_properties sai_rx_props_##inst = {			\
	.fifo_address = SAI_RX_FIFO_BASE(inst),					\
	.fifo_depth = SAI_FIFO_DEPTH(inst) * CONFIG_SAI_FIFO_WORD_SIZE,		\
	.dma_hs_id = SAI_RX_DMA_MUX(inst),					\
};										\
										\
void irq_config_##inst(void)							\
{										\
	IRQ_CONNECT(DT_INST_IRQN(inst),						\
		    0,								\
		    sai_isr,							\
		    DEVICE_DT_INST_GET(inst),					\
		    0);								\
	irq_enable(DT_INST_IRQN(inst));						\
}										\
										\
static struct sai_config sai_config_##inst = {					\
	.regmap_phys = DT_INST_REG_ADDR(inst),					\
	.regmap_size = DT_INST_REG_SIZE(inst),					\
	.clk_data = SAI_CLOCK_DATA_DECLARE(inst),				\
	.rx_fifo_watermark = SAI_RX_FIFO_WATERMARK(inst),			\
	.tx_fifo_watermark = SAI_TX_FIFO_WATERMARK(inst),			\
	.mclk_is_output = DT_INST_PROP_OR(inst, mclk_is_output, false),		\
	.tx_props = &sai_tx_props_##inst,					\
	.rx_props = &sai_rx_props_##inst,					\
	.irq_config = irq_config_##inst,					\
	.tx_sync_mode = SAI_TX_SYNC_MODE(inst),					\
	.rx_sync_mode = SAI_RX_SYNC_MODE(inst),					\
};										\
										\
static struct sai_data sai_data_##inst = {					\
	.cfg.type = DAI_IMX_SAI,						\
	.cfg.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),			\
};										\
										\
DEVICE_DT_INST_DEFINE(inst, &sai_init, NULL,					\
		      &sai_data_##inst, &sai_config_##inst,			\
		      POST_KERNEL, CONFIG_DAI_INIT_PRIORITY,			\
		      &sai_api);						\

DT_INST_FOREACH_STATUS_OKAY(SAI_INIT);

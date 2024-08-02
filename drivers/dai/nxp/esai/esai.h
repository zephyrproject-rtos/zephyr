/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DAI_NXP_ESAI_H_
#define ZEPHYR_DRIVERS_DAI_NXP_ESAI_H_

#include <zephyr/logging/log.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/dai/esai.h>

#include <fsl_esai.h>

LOG_MODULE_REGISTER(nxp_dai_esai);

/* used for binding the driver */
#define DT_DRV_COMPAT nxp_dai_esai

/* workaround the fact that device_map() doesn't exist for SoCs with no MMU */
#ifndef DEVICE_MMIO_IS_IN_RAM
#define device_map(virt, phys, size, flags) *(virt) = (phys)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/* macros used for parsing DTS data */

#define _ESAI_FIFO_DEPTH(inst)\
	FSL_FEATURE_ESAI_FIFO_SIZEn(UINT_TO_ESAI(DT_INST_REG_ADDR(inst)))

/* used to fetch the depth of the FIFO. If the "fifo-depth" property is
 * not specified, the FIFO depth that will be reported to the upper layers
 * will be 128 * 4 (which is the maximum value, or, well, the actual FIFO
 * depth)
 */
#define ESAI_FIFO_DEPTH(inst)\
	DT_INST_PROP_OR(inst, fifo_depth, _ESAI_FIFO_DEPTH(inst))

/* used to fetch the TX FIFO watermark value. If the "tx-fifo-watermark"
 * property is not specified, this will be set to half of the FIFO depth.
 */
#define ESAI_TX_FIFO_WATERMARK(inst)\
	DT_INST_PROP_OR(inst, tx_fifo_watermark, (_ESAI_FIFO_DEPTH(inst) / 2))

/* used to fetch the RX FIFO watermark value. If the "rx-fifo-watermark"
 * property is not specified, this will be set to half of the FIFO depth.
 */
#define ESAI_RX_FIFO_WATERMARK(inst)\
	DT_INST_PROP_OR(inst, rx_fifo_watermark, (_ESAI_FIFO_DEPTH(inst) / 2))

/* use to fetch the handshake value for a given direction. The handshake
 * is computed as follows:
 *	handshake = CHANNEL_ID | (MUX_VALUE << 8)
 * The channel ID and MUX value are each encoded in 8 bits.
 */
#define ESAI_TX_RX_DMA_HANDSHAKE(inst, dir)\
	((DT_INST_DMAS_CELL_BY_NAME(inst, dir, channel) & GENMASK(7, 0)) |\
	((DT_INST_DMAS_CELL_BY_NAME(inst, dir, mux) << 8) & GENMASK(15, 8)))

/* used to fetch the word width. If the "word-width" property is not specified,
 * this will default to 24.
 */
#define ESAI_WORD_WIDTH(inst) DT_INST_PROP_OR(inst, word_width, 24)

/* utility macros */

/* convert uint to ESAI_Type * */
#define UINT_TO_ESAI(x) ((ESAI_Type *)(uintptr_t)(x))

/* invert a clock's polarity. This works because a clock's polarity
 * is expressed as a 0 or as a 1.
 */
#define ESAI_INVERT_POLARITY(polarity) (polarity) = !(polarity)

#define _ESAI_SLOT_WORD_WIDTH_IS_VALID(width) (!(((width) - 8) % 4))

/* used to check if a slot/word width combination is valid */
#define ESAI_SLOT_WORD_WIDTH_IS_VALID(slot_width, word_width)\
	(_ESAI_SLOT_WORD_WIDTH_IS_VALID(slot_width) && \
	_ESAI_SLOT_WORD_WIDTH_IS_VALID(word_width) && \
	((word_width) < 32) && ((word_width) <= (slot_width)))

/* used to convert slot/word width combination to a value that can be written
 * to TCR's TSWS or RCR's RSWS.
 */
#define ESAI_SLOT_FORMAT(s, w)\
	((w) < 24 ? ((s) - (w) + (((w) - 8) / 4)) : ((s) < 32 ? 0x1e : 0x1f))

/* used to compute the word alignment based on the word width value.
 * This returns a value that can be written to TFCR's TWA or RFCR's
 * RWA.
 */
#define ESAI_WORD_ALIGNMENT(word_width) ((32 - (word_width)) / 4)

#define _ESAI_RX_FIFO_USAGE_EN(mask)\
	(((mask) << ESAI_RFCR_RE0_SHIFT) &\
	(ESAI_RFCR_RE0_MASK | ESAI_RFCR_RE1_MASK |\
	 ESAI_RFCR_RE2_MASK | ESAI_RFCR_RE3_MASK))

#define _ESAI_TX_FIFO_USAGE_EN(mask)\
	(((mask) << ESAI_TFCR_TE0_SHIFT) &\
	(ESAI_TFCR_TE0_MASK | ESAI_TFCR_TE1_MASK | ESAI_TFCR_TE2_MASK |\
	 ESAI_TFCR_TE3_MASK | ESAI_TFCR_TE4_MASK | ESAI_TFCR_TE5_MASK))

/* used to fetch the mask for setting TX/RX FIFO usage. By FIFO usage
 * we mean we allow receivers/transmitters to use the "global" TX/RX
 * FIFO (i.e: the FIFO that's common to all transmitters/receivers).
 * More specifically, this macro returns the mask required for setting
 * TFCR's TEx fields or RFCR's REx fields.
 */
#define ESAI_TX_RX_FIFO_USAGE_EN(dir, mask)\
	((dir) == DAI_DIR_TX ? _ESAI_TX_FIFO_USAGE_EN(mask) :\
	 _ESAI_RX_FIFO_USAGE_EN(mask))

#define _ESAI_TX_EN(mask)\
	(((mask) << ESAI_TCR_TE0_SHIFT) &\
	(ESAI_TCR_TE0_MASK | ESAI_TCR_TE1_MASK | ESAI_TCR_TE2_MASK |\
	 ESAI_TCR_TE3_MASK | ESAI_TCR_TE4_MASK | ESAI_TCR_TE5_MASK))

#define _ESAI_RX_EN(mask)\
	(((mask) << ESAI_RCR_RE0_SHIFT) &\
	(ESAI_RCR_RE0_MASK | ESAI_RCR_RE1_MASK | ESAI_RCR_RE2_MASK |\
	 ESAI_RCR_RE3_MASK))

/* used to fetch the mask for enabling transmitters/receivers.
 * More specifically, this refers to TCR's TEx bits or RCR's REx
 * bits.
 */
#define ESAI_TX_RX_EN(dir, mask)\
	((dir) == DAI_DIR_TX ? _ESAI_TX_EN(mask) : _ESAI_RX_EN(mask))

/* used to fetch the base address of the TX FIFO */
#define ESAI_TX_FIFO_BASE(inst)\
	POINTER_TO_UINT(&(UINT_TO_ESAI(DT_INST_REG_ADDR(inst))->ETDR))

/* used to fetch the base address of the RX FIFO */
#define ESAI_RX_FIFO_BASE(inst)\
	POINTER_TO_UINT(&(UINT_TO_ESAI(DT_INST_REG_ADDR(inst))->ERDR))

/* used to check if an ESAI pin is used. An ESAI pin is considered to
 * be used if PDC and PC bits for that pin are set (i.e: pin is in ESAI
 * mode).
 *
 * The ESAI pins support 4 functionalities which can be configured
 * via PCRC and PRRC:
 *	1) Disconnected
 *	2) GPIO input
 *	3) GPIO output
 *	4) ESAI
 */
#define ESAI_PIN_IS_USED(data, which)\
	(((data)->pcrc & BIT(which)) && ((data->prrc) & BIT(which)))

struct esai_data {
	mm_reg_t regmap;
	struct dai_config cfg;
	/* transmitter state */
	enum dai_state tx_state;
	/* receiver state */
	enum dai_state rx_state;
	/* value to be committed to PRRC. This is computed
	 * during esai_init() and committed during config_set()
	 * stage.
	 */
	uint32_t prrc;
	/* value to be committed to PCRC. Computed and committed
	 * during the same stages as PRRC.
	 */
	uint32_t pcrc;
};

struct esai_config {
	uint32_t regmap_phys;
	uint32_t regmap_size;
	const struct dai_properties *tx_props;
	const struct dai_properties *rx_props;
	uint32_t rx_fifo_watermark;
	uint32_t tx_fifo_watermark;
	uint32_t word_width;
	uint32_t *pinmodes;
	uint32_t pinmodes_size;
	uint32_t *clock_cfg;
	uint32_t clock_cfg_size;
};

/* this needs to perfectly match SOF's struct sof_ipc_dai_esai_params */
struct esai_bespoke_config {
	uint32_t reserved0;

	uint16_t reserved1;
	uint16_t mclk_id;
	uint32_t mclk_direction;

	/* clock-related data */
	uint32_t mclk_rate;
	uint32_t fsync_rate;
	uint32_t bclk_rate;

	/* TDM-related data */
	uint32_t tdm_slots;
	uint32_t rx_slots;
	uint32_t tx_slots;
	uint16_t tdm_slot_width;

	uint16_t reserved2;
};

struct esai_transceiver_config {
	/* enable/disable the HCLK prescaler */
	bool hclk_prescaler_en;
	/* controls the divison value of HCLK (i.e: TPM0-TPM7) */
	uint32_t hclk_div_ratio;
	/* controls the division value of HCLK before reaching
	 * BCLK consumers (i.e: TFP0-TFP3)
	 */
	uint32_t bclk_div_ratio;
	/* should the HCLK divison be bypassed or not?
	 * If in bypass, HCLK pad will be the same as EXTAL
	 */
	bool hclk_bypass;

	/* HCLK direction - input or output */
	esai_clock_direction_t hclk_dir;
	/* HCLK source - EXTAL or IPG clock */
	esai_hclk_source_t hclk_src;
	/* HCLK polarity - LOW or HIGH */
	esai_clock_polarity_t hclk_polarity;

	/* BCLK direction - input or output */
	esai_clock_direction_t bclk_dir;
	/* BCLK polarity - LOW or HIGH */
	esai_clock_polarity_t bclk_polarity;

	/* FSYNC direction - input or output */
	esai_clock_direction_t fsync_dir;
	/* FSYNC polarity - LOW or HIGH */
	esai_clock_polarity_t fsync_polarity;

	/* should FSYNC be bit-wide or word-wide? */
	bool fsync_is_bit_wide;
	/* enable/disable padding word with zeros. If
	 * disabled, pad will be done using last/first
	 * bit - see TCR's PADC bit for more info.
	 */
	bool zero_pad_en;
	/* should FSYNC be asserted before MSB transmission
	 * or alongside it?
	 */
	bool fsync_early;

	/* FSYNC divison value - for network mode this is
	 * the same as the number of slots - 1.
	 */
	uint32_t fsync_div;

	/* slot format - see TCR's TSWS or RCR's RSWS */
	esai_slot_format_t slot_format;
	/* mode - network or normal
	 * TODO: at the moment, only network mode is supported.
	 */
	esai_mode_t mode;

	/* controls whether MSB or LSB is transmitted first */
	esai_shift_direction_t data_order;

	/* controls the word alignment inside a slot. If enabled
	 * word is left-aligned, otherwise it will be right-aligned.
	 * For details, see TCR/RCR's TWA/RWA.
	 */
	bool data_left_aligned;
	/* TX/RX watermark value */
	uint32_t watermark;

	/* concatenation of TSMA+TSMB/RSMA+RSMB. Controls which
	 * slots should be High-Z or data.
	 */
	uint32_t slot_mask;
	/* controls the alignment of data written to FIFO.
	 * See TFCR's TWA or RFCR's RWA for more details.
	 */
	uint32_t word_alignment;
};

static int esai_parse_clock_config(const struct esai_config *cfg,
				   struct esai_transceiver_config *tx_cfg,
				   struct esai_transceiver_config *rx_cfg)
{
	int i;
	uint32_t crt_clock, crt_dir;

	for (i = 0; i < cfg->clock_cfg_size; i += 2) {
		crt_clock = cfg->clock_cfg[i];
		crt_dir = cfg->clock_cfg[i + 1];

		/* sanity checks */
		if (crt_clock > ESAI_CLOCK_FST) {
			LOG_ERR("invalid clock configuration ID: %d", crt_clock);
			return -EINVAL;
		}

		if (crt_dir > ESAI_CLOCK_OUTPUT) {
			LOG_ERR("invalid clock configuration direction: %d", crt_dir);
			return -EINVAL;
		}

		switch (crt_clock) {
		case ESAI_CLOCK_HCKT:
			tx_cfg->hclk_dir = crt_dir;
			break;
		case ESAI_CLOCK_HCKR:
			rx_cfg->hclk_dir = crt_dir;
			break;
		case ESAI_CLOCK_SCKT:
			tx_cfg->bclk_dir = crt_dir;
			break;
		case ESAI_CLOCK_SCKR:
			rx_cfg->bclk_dir = crt_dir;
			break;
		case ESAI_CLOCK_FST:
			tx_cfg->fsync_dir = crt_dir;
			break;
		case ESAI_CLOCK_FSR:
			rx_cfg->fsync_dir = crt_dir;
			break;
		}
	}

	return 0;
}

static int esai_parse_pinmodes(const struct esai_config *cfg,
			      struct esai_data *data)
{
	int i;
	uint32_t pin, pin_mode;

	/* initially, the assumption is that all pins are in ESAI mode */
	data->pcrc = ESAI_PCRC_PC_MASK;
	data->prrc = ESAI_PRRC_PDC_MASK;

	for (i = 0; i < cfg->pinmodes_size; i += 2) {
		pin = cfg->pinmodes[i];
		pin_mode = cfg->pinmodes[i + 1];

		if (pin > ESAI_PIN_SDO0 || pin_mode > ESAI_PIN_ESAI) {
			return -EINVAL;
		}

		switch (pin_mode) {
		case ESAI_PIN_DISCONNECTED:
			data->pcrc &= ~BIT(pin);
			data->prrc &= ~BIT(pin);
			break;
		case ESAI_PIN_GPIO_INPUT:
			data->pcrc &= ~BIT(pin);
			break;
		case ESAI_PIN_GPIO_OUTPUT:
			data->prrc &= ~BIT(pin);
			break;
		case ESAI_PIN_ESAI:
			/* nothing to be done here, this is the default */
			break;
		}
	}

	return 0;
}

static inline uint32_t esai_get_state(struct esai_data *data,
				      enum dai_dir dir)
{
	if (dir == DAI_DIR_RX) {
		return data->rx_state;
	} else {
		return data->tx_state;
	}
}

static inline int esai_update_state(struct esai_data *data,
				    enum dai_dir dir, enum dai_state new_state)
{
	enum dai_state old_state = esai_get_state(data, dir);

	LOG_DBG("attempting state transition from %d to %d", old_state, new_state);

	switch (new_state) {
	case DAI_STATE_NOT_READY:
		/* initial state, transition is not possible */
		return -EPERM;
	case DAI_STATE_READY:
		if (old_state != DAI_STATE_NOT_READY &&
		    old_state != DAI_STATE_READY &&
		    old_state != DAI_STATE_STOPPING) {
			return -EPERM;
		}
		break;
	case DAI_STATE_RUNNING:
		if (old_state != DAI_STATE_STOPPING &&
		    old_state != DAI_STATE_READY) {
			return -EPERM;
		}
		break;
	case DAI_STATE_STOPPING:
		if (old_state != DAI_STATE_RUNNING) {
			return -EPERM;
		}
		break;
	default:
		LOG_ERR("invalid new state: %d", new_state);
		return -EINVAL;
	}

	if (dir == DAI_DIR_RX) {
		data->rx_state = new_state;
	} else {
		data->tx_state = new_state;
	}

	return 0;
}

static inline void esai_tx_rx_enable_disable_fifo(ESAI_Type *base,
						  enum dai_dir dir,
						  bool enable)
{
	if (enable) {
		if (dir == DAI_DIR_RX) {
			base->RFCR |= ESAI_RFCR_RFE_MASK;
		} else {
			base->TFCR |= ESAI_TFCR_TFE_MASK;
		}
	} else {
		if (dir == DAI_DIR_RX) {
			base->RFCR &= ~ESAI_RFCR_RFE_MASK;
		} else {
			base->TFCR &= ~ESAI_TFCR_TFE_MASK;
		}
	}
}

static inline void esai_tx_rx_enable_disable(ESAI_Type *base,
					     enum dai_dir dir,
					     uint32_t which, bool enable)
{
	uint32_t val = ESAI_TX_RX_EN(dir, which);

	if (enable) {
		if (dir == DAI_DIR_RX) {
			base->RCR |= val;
		} else {
			base->TCR |= val;
		}
	} else {
		if (dir == DAI_DIR_RX) {
			base->RCR &= ~val;
		} else {
			base->TCR &= ~val;
		}
	}
}

static inline void esai_tx_rx_enable_disable_fifo_usage(ESAI_Type *base,
							enum dai_dir dir,
							uint32_t which, bool enable)
{
	uint32_t val = ESAI_TX_RX_FIFO_USAGE_EN(dir, which);

	if (enable) {
		if (dir == DAI_DIR_RX) {
			base->RFCR |= val;
		} else {
			base->TFCR |= val;
		}
	} else {
		if (dir == DAI_DIR_RX) {
			base->RFCR &= ~val;
		} else {
			base->TFCR &= ~val;
		}
	}
}

static inline void esai_dump_xceiver_config(struct esai_transceiver_config *cfg)
{
	LOG_DBG("HCLK prescaler enable: %d", cfg->hclk_prescaler_en);
	LOG_DBG("HCLK divider ratio: %d", cfg->hclk_div_ratio);
	LOG_DBG("BCLK divider ratio: %d", cfg->bclk_div_ratio);
	LOG_DBG("HCLK bypass: %d", cfg->hclk_bypass);

	LOG_DBG("HCLK direction: %d", cfg->hclk_dir);
	LOG_DBG("HCLK source: %d", cfg->hclk_src);
	LOG_DBG("HCLK polarity: %d", cfg->hclk_polarity);

	LOG_DBG("BCLK direction: %d", cfg->bclk_dir);
	LOG_DBG("BCLK polarity: %d", cfg->bclk_polarity);

	LOG_DBG("FSYNC direction: %d", cfg->fsync_dir);
	LOG_DBG("FSYNC polarity: %d", cfg->fsync_polarity);

	LOG_DBG("FSYNC is bit wide: %d", cfg->fsync_is_bit_wide);
	LOG_DBG("zero pad enable: %d", cfg->zero_pad_en);
	LOG_DBG("FSYNC asserted early: %d", cfg->fsync_early);

	LOG_DBG("watermark: %d", cfg->watermark);
	LOG_DBG("slot mask: 0x%x", cfg->slot_mask);
	LOG_DBG("word alignment: 0x%x", cfg->word_alignment);
}

static inline void esai_dump_register_data(ESAI_Type *base)
{
	LOG_DBG("ECR: 0x%x", base->ECR);
	LOG_DBG("ESR: 0x%x", base->ESR);
	LOG_DBG("TFCR: 0x%x", base->TFCR);
	LOG_DBG("TFSR: 0x%x", base->TFSR);
	LOG_DBG("RFCR: 0x%x", base->RFCR);
	LOG_DBG("RFSR: 0x%x", base->RFSR);
	LOG_DBG("TSR: 0x%x", base->TSR);
	LOG_DBG("SAISR: 0x%x", base->SAISR);
	LOG_DBG("SAICR: 0x%x", base->SAICR);
	LOG_DBG("TCR: 0x%x", base->TCR);
	LOG_DBG("TCCR: 0x%x", base->TCCR);
	LOG_DBG("RCR: 0x%x", base->RCR);
	LOG_DBG("RCCR: 0x%x", base->RCCR);
	LOG_DBG("TSMA: 0x%x", base->TSMA);
	LOG_DBG("TSMB: 0x%x", base->TSMB);
	LOG_DBG("RSMA: 0x%x", base->RSMA);
	LOG_DBG("RSMB: 0x%x", base->RSMB);
	LOG_DBG("PRRC: 0x%x", base->PRRC);
	LOG_DBG("PCRC: 0x%x", base->PCRC);
}

#endif /* ZEPHYR_DRIVERS_DAI_NXP_ESAI_H_ */

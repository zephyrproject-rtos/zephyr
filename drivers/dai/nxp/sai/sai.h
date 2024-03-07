/* Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DAI_NXP_SAI_H_
#define ZEPHYR_DRIVERS_DAI_NXP_SAI_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <fsl_sai.h>

LOG_MODULE_REGISTER(nxp_dai_sai);

#ifdef CONFIG_SAI_HAS_MCLK_CONFIG_OPTION
#define SAI_MCLK_MCR_MSEL_SHIFT 24
#define SAI_MCLK_MCR_MSEL_MASK GENMASK(24, 25)
#endif /* CONFIG_SAI_HAS_MCLK_CONFIG_OPTION */
/* workaround the fact that device_map() doesn't exist for SoCs with no MMU */
#ifndef DEVICE_MMIO_IS_IN_RAM
#define device_map(virt, phys, size, flags) *(virt) = (phys)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/* used to convert an uint to I2S_Type * */
#define UINT_TO_I2S(x) ((I2S_Type *)(uintptr_t)(x))

/* macros used for parsing DTS data */

/* used instead of IDENTITY because LISTIFY expects the used macro function
 * to also take a variable number of arguments.
 */
#define IDENTITY_VARGS(V, ...) IDENTITY(V)

/* used to generate the list of clock indexes */
#define _SAI_CLOCK_INDEX_ARRAY(inst)\
	LISTIFY(DT_INST_PROP_LEN_OR(inst, clocks, 0), IDENTITY_VARGS, (,))

/* used to retrieve a clock's ID using its index generated via _SAI_CLOCK_INDEX_ARRAY */
#define _SAI_GET_CLOCK_ID(clock_idx, inst)\
	DT_INST_CLOCKS_CELL_BY_IDX(inst, clock_idx, name)

/* used to retrieve a clock's name using its index generated via _SAI_CLOCK_INDEX_ARRAY */
#define _SAI_GET_CLOCK_NAME(clock_idx, inst)\
	DT_INST_PROP_BY_IDX(inst, clock_names, clock_idx)

/* used to convert the clocks property into an array of clock IDs */
#define _SAI_CLOCK_ID_ARRAY(inst)\
	FOR_EACH_FIXED_ARG(_SAI_GET_CLOCK_ID, (,), inst, _SAI_CLOCK_INDEX_ARRAY(inst))

/* used to convert the clock-names property into an array of clock names */
#define _SAI_CLOCK_NAME_ARRAY(inst)\
	FOR_EACH_FIXED_ARG(_SAI_GET_CLOCK_NAME, (,), inst, _SAI_CLOCK_INDEX_ARRAY(inst))

/* used to convert a clocks property into an array of clock IDs. If the property
 * is not specified then this macro will return {}.
 */
#define _SAI_GET_CLOCK_ARRAY(inst)\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(inst, nxp_dai_sai), clocks),\
		    ({ _SAI_CLOCK_ID_ARRAY(inst) }),\
		    ({ }))

/* used to retrieve a const struct device *dev pointing to the clock controller.
 * It is assumed that all SAI clocks come from a single clock provider.
 * This macro returns a NULL if the clocks property doesn't exist.
 */
#define _SAI_GET_CLOCK_CONTROLLER(inst)\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(inst, nxp_dai_sai), clocks),\
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))),\
		    (NULL))

/* used to convert a clock-names property into an array of clock names. If the
 * property is not specified then this macro will return {}.
 */
#define _SAI_GET_CLOCK_NAMES(inst)\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(inst, nxp_dai_sai), clocks),\
		    ({ _SAI_CLOCK_NAME_ARRAY(inst) }),\
		    ({ }))

/* used to declare a struct clock_data */
#define SAI_CLOCK_DATA_DECLARE(inst)					\
{									\
	.clocks = (uint32_t [])_SAI_GET_CLOCK_ARRAY(inst),		\
	.clock_num = DT_INST_PROP_LEN_OR(inst, clocks, 0),		\
	.dev = _SAI_GET_CLOCK_CONTROLLER(inst),				\
	.clock_names = (const char *[])_SAI_GET_CLOCK_NAMES(inst),	\
}

/* used to parse the tx-fifo-watermark property. If said property is not
 * specified then this macro will return half of the number of words in the
 * FIFO.
 */
#define SAI_TX_FIFO_WATERMARK(inst)\
	DT_INST_PROP_OR(inst, tx_fifo_watermark,\
			FSL_FEATURE_SAI_FIFO_COUNTn(UINT_TO_I2S(DT_INST_REG_ADDR(inst))) / 2)

/* used to parse the rx-fifo-watermark property. If said property is not
 * specified then this macro will return half of the number of words in the
 * FIFO.
 */
#define SAI_RX_FIFO_WATERMARK(inst)\
	DT_INST_PROP_OR(inst, rx_fifo_watermark,\
			FSL_FEATURE_SAI_FIFO_COUNTn(UINT_TO_I2S(DT_INST_REG_ADDR(inst))) / 2)

/* used to retrieve TDR0's address based on SAI's physical address */
#define SAI_TX_FIFO_BASE(inst)\
	POINTER_TO_UINT(&(UINT_TO_I2S(DT_INST_REG_ADDR(inst))->TDR[0]))

/* used to retrieve RDR0's address based on SAI's physical address */
#define SAI_RX_FIFO_BASE(inst)\
	POINTER_TO_UINT(&(UINT_TO_I2S(DT_INST_REG_ADDR(inst))->RDR[0]))

/* internal macro used to retrieve the default TX/RX FIFO's size (in FIFO words) */
#define _SAI_FIFO_DEPTH(inst)\
	FSL_FEATURE_SAI_FIFO_COUNTn(UINT_TO_I2S(DT_INST_REG_ADDR(inst)))

/* used to retrieve the TX/RX FIFO's size (in FIFO words) */
#define SAI_FIFO_DEPTH(inst)\
	DT_INST_PROP_OR(inst, fifo_depth, _SAI_FIFO_DEPTH(inst))

/* used to retrieve the DMA MUX for transmitter */
#define SAI_TX_DMA_MUX(inst)\
	FSL_FEATURE_SAI_TX_DMA_MUXn(UINT_TO_I2S(DT_INST_REG_ADDR(inst)))

/* used to retrieve the DMA MUX for receiver */
#define SAI_RX_DMA_MUX(inst)\
	FSL_FEATURE_SAI_RX_DMA_MUXn(UINT_TO_I2S(DT_INST_REG_ADDR(inst)))

/* used to retrieve the synchronization mode of the transmitter. If this
 * property is not specified, ASYNC mode will be used.
 */
#define SAI_TX_SYNC_MODE(inst)\
	DT_INST_PROP_OR(inst, tx_sync_mode, kSAI_ModeAsync)

/* used to retrieve the synchronization mode of the receiver. If this property
 * is not specified, ASYNC mode will be used.
 */
#define SAI_RX_SYNC_MODE(inst)\
	DT_INST_PROP_OR(inst, rx_sync_mode, kSAI_ModeAsync)

/* utility macros */

/* invert a clock's polarity. This works because a clock's polarity is expressed
 * as a 0 or as a 1.
 */
#define SAI_INVERT_POLARITY(polarity) (polarity) = !(polarity)

/* used to issue a software reset of the transmitter/receiver */
#define SAI_TX_RX_SW_RESET(dir, regmap)\
	((dir) == DAI_DIR_RX ? SAI_RxSoftwareReset(UINT_TO_I2S(regmap), kSAI_ResetTypeSoftware) :\
	 SAI_TxSoftwareReset(UINT_TO_I2S(regmap), kSAI_ResetTypeSoftware))

/* used to enable/disable the transmitter/receiver.
 * When enabling the SYNC component, the ASYNC component will also be enabled.
 * Attempting to disable the SYNC component will fail unless the SYNC bit is
 * cleared. It is recommended to use sai_tx_rx_force_disable() instead of this
 * macro when disabling transmitter/receiver.
 */
#define SAI_TX_RX_ENABLE_DISABLE(dir, regmap, enable)\
	((dir) == DAI_DIR_RX ? SAI_RxEnable(UINT_TO_I2S(regmap), enable) :\
	 SAI_TxEnable(UINT_TO_I2S(regmap), enable))

/* used to enable/disable the DMA requests for transmitter/receiver */
#define SAI_TX_RX_DMA_ENABLE_DISABLE(dir, regmap, enable)\
	((dir) == DAI_DIR_RX ? SAI_RxEnableDMA(UINT_TO_I2S(regmap),\
					       kSAI_FIFORequestDMAEnable, enable) :\
	SAI_TxEnableDMA(UINT_TO_I2S(regmap), kSAI_FIFORequestDMAEnable, enable))

/* used to check if the hardware transmitter/receiver is enabled */
#define SAI_TX_RX_IS_HW_ENABLED(dir, regmap)\
	((dir) == DAI_DIR_RX ? (UINT_TO_I2S(regmap)->RCSR & I2S_RCSR_RE_MASK) : \
	 (UINT_TO_I2S(regmap)->TCSR & I2S_TCSR_TE_MASK))

/* used to enable various transmitter/receiver interrupts */
#define _SAI_TX_RX_ENABLE_IRQ(dir, regmap, which)\
	((dir) == DAI_DIR_RX ? SAI_RxEnableInterrupts(UINT_TO_I2S(regmap), which) : \
	 SAI_TxEnableInterrupts(UINT_TO_I2S(regmap), which))

/* used to disable various transmitter/receiver interrupts */
#define _SAI_TX_RX_DISABLE_IRQ(dir, regmap, which)\
	((dir) == DAI_DIR_RX ? SAI_RxDisableInterrupts(UINT_TO_I2S(regmap), which) : \
	 SAI_TxDisableInterrupts(UINT_TO_I2S(regmap), which))

/* used to enable/disable various transmitter/receiver interrupts */
#define SAI_TX_RX_ENABLE_DISABLE_IRQ(dir, regmap, which, enable)\
	((enable == true) ? _SAI_TX_RX_ENABLE_IRQ(dir, regmap, which) :\
	 _SAI_TX_RX_DISABLE_IRQ(dir, regmap, which))

/* used to check if a status flag is set */
#define SAI_TX_RX_STATUS_IS_SET(dir, regmap, which)\
	((dir) == DAI_DIR_RX ? ((UINT_TO_I2S(regmap))->RCSR & (which)) : \
	 ((UINT_TO_I2S(regmap))->TCSR & (which)))

/* used to retrieve the SYNC direction. Use this macro when you know for sure
 * you have 1 SYNC direction with 1 ASYNC direction.
 */
#define SAI_TX_RX_GET_SYNC_DIR(cfg)\
	((cfg)->tx_sync_mode == kSAI_ModeSync ? DAI_DIR_TX : DAI_DIR_RX)

/* used to retrieve the ASYNC direction. Use this macro when you know for sure
 * you have 1 SYNC direction with 1 ASYNC direction.
 */
#define SAI_TX_RX_GET_ASYNC_DIR(cfg)\
	((cfg)->tx_sync_mode == kSAI_ModeAsync ? DAI_DIR_TX : DAI_DIR_RX)

/* used to check if transmitter/receiver is SW enabled */
#define SAI_TX_RX_DIR_IS_SW_ENABLED(dir, data)\
	((dir) == DAI_DIR_TX ? data->tx_enabled : data->rx_enabled)

struct sai_clock_data {
	uint32_t *clocks;
	uint32_t clock_num;
	/* assumption: all clocks belong to the same producer */
	const struct device *dev;
	const char **clock_names;
};

struct sai_data {
	mm_reg_t regmap;
	sai_transceiver_t rx_config;
	sai_transceiver_t tx_config;
	bool tx_enabled;
	bool rx_enabled;
	enum dai_state tx_state;
	enum dai_state rx_state;
	struct dai_config cfg;
};

struct sai_config {
	uint32_t regmap_phys;
	uint32_t regmap_size;
	struct sai_clock_data clk_data;
	bool mclk_is_output;
	/* if the tx/rx-fifo-watermark properties are not specified, it's going
	 * to be assumed that the watermark should be set to half of the FIFO
	 * size.
	 */
	uint32_t rx_fifo_watermark;
	uint32_t tx_fifo_watermark;
	const struct dai_properties *tx_props;
	const struct dai_properties *rx_props;
	uint32_t dai_index;
	/* RX synchronization mode - may be SYNC or ASYNC */
	sai_sync_mode_t rx_sync_mode;
	/* TX synchronization mode - may be SYNC or ASYNC */
	sai_sync_mode_t tx_sync_mode;
	void (*irq_config)(void);
};

/* this needs to perfectly match SOF's struct sof_ipc_dai_sai_params */
struct sai_bespoke_config {
	uint32_t reserved0;

	uint16_t reserved1;
	uint16_t mclk_id;
	uint32_t mclk_direction;

	/* CLOCK-related data */
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

#ifdef CONFIG_SAI_HAS_MCLK_CONFIG_OPTION
static int get_msel(sai_bclk_source_t bclk_source, uint32_t *msel)
{
	switch (bclk_source) {
	case kSAI_BclkSourceMclkOption1:
		*msel = 0;
		break;
	case kSAI_BclkSourceMclkOption2:
		*msel = (0x2 << SAI_MCLK_MCR_MSEL_SHIFT);
		break;
	case kSAI_BclkSourceMclkOption3:
		*msel = (0x3 << SAI_MCLK_MCR_MSEL_SHIFT);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void set_msel(uint32_t regmap, int msel)
{
	UINT_TO_I2S(regmap)->MCR &= ~SAI_MCLK_MCR_MSEL_MASK;
	UINT_TO_I2S(regmap)->MCR |= msel;
}

static int clk_lookup_by_name(const struct sai_clock_data *clk_data, char *name)
{
	int i;

	for (i = 0; i < clk_data->clock_num; i++) {
		if (!strcmp(name, clk_data->clock_names[i])) {
			return i;
		}
	}

	return -EINVAL;
}

static int get_mclk_rate(const struct sai_clock_data *clk_data,
			 sai_bclk_source_t bclk_source,
			 uint32_t *rate)
{
	int clk_idx;
	char *clk_name;

	switch (bclk_source) {
	case kSAI_BclkSourceMclkOption1:
		clk_name = "mclk1";
		break;
	case kSAI_BclkSourceMclkOption2:
		clk_name = "mclk2";
		break;
	case kSAI_BclkSourceMclkOption3:
		clk_name = "mclk3";
		break;
	default:
		LOG_ERR("invalid bitclock source: %d", bclk_source);
		return -EINVAL;
	}

	clk_idx = clk_lookup_by_name(clk_data, clk_name);
	if (clk_idx < 0) {
		LOG_ERR("failed to get clock index for %s", clk_name);
		return clk_idx;
	}

	return clock_control_get_rate(clk_data->dev,
				      UINT_TO_POINTER(clk_data->clocks[clk_idx]),
				      rate);
}
#endif /* CONFIG_SAI_HAS_MCLK_CONFIG_OPTION */

static inline void get_bclk_default_config(sai_bit_clock_t *cfg)
{
	memset(cfg, 0, sizeof(sai_bit_clock_t));

	/* by default, BCLK has the following properties:
	 *
	 *	1) BCLK is active HIGH.
	 *	2) BCLK uses MCLK1 source. (only applicable to master mode)
	 *	3) No source swap.
	 *	4) No input delay.
	 */
	cfg->bclkPolarity = kSAI_PolarityActiveHigh;
	cfg->bclkSource = kSAI_BclkSourceMclkOption1;
}

static inline void get_fsync_default_config(sai_frame_sync_t *cfg)
{
	memset(cfg, 0, sizeof(sai_frame_sync_t));

	/* by default, FSYNC has the following properties:
	 *
	 *	1) FSYNC is asserted one bit early with respect to the next
	 *	frame.
	 *	2) FSYNC is active HIGH.
	 */
	cfg->frameSyncEarly = true;
	cfg->frameSyncPolarity = kSAI_PolarityActiveHigh;
}

static inline void get_serial_default_config(sai_serial_data_t *cfg)
{
	memset(cfg, 0, sizeof(sai_serial_data_t));

	/* by default, the serial configuration has the following quirks:
	 *
	 * 1) Data pin is not tri-stated.
	 * 2) MSB is first.
	 */
	/* note: this is equivalent to checking if the SAI has xCR4's CHMOD bit */
#if FSL_FEATURE_SAI_HAS_CHANNEL_MODE
	cfg->dataMode = kSAI_DataPinStateOutputZero;
#endif /* FSL_FEATURE_SAI_HAS_CHANNEL_MODE */
	cfg->dataOrder = kSAI_DataMSB;
}

static inline void get_fifo_default_config(sai_fifo_t *cfg)
{
	memset(cfg, 0, sizeof(sai_fifo_t));
}

static inline uint32_t sai_get_state(enum dai_dir dir,
				     struct sai_data *data)
{
	if (dir == DAI_DIR_RX) {
		return data->rx_state;
	} else {
		return data->tx_state;
	}
}

static int sai_update_state(enum dai_dir dir,
			    struct sai_data *data,
			    enum dai_state new_state)
{
	enum dai_state old_state = sai_get_state(dir, data);

	LOG_DBG("attempting to transition from %d to %d", old_state, new_state);

	/* check if transition is possible */
	switch (new_state) {
	case DAI_STATE_NOT_READY:
		/* this shouldn't be possible */
		return -EPERM;
	case DAI_STATE_READY:
		if (old_state != DAI_STATE_NOT_READY &&
		    old_state != DAI_STATE_READY &&
		    old_state != DAI_STATE_STOPPING) {
			return -EPERM;
		}
		break;
	case DAI_STATE_RUNNING:
		if (old_state != DAI_STATE_PAUSED &&
		    old_state != DAI_STATE_STOPPING &&
		    old_state != DAI_STATE_READY) {
			return -EPERM;
		}
		break;
	case DAI_STATE_PAUSED:
		if (old_state != DAI_STATE_RUNNING) {
			return -EPERM;
		}
		break;
	case DAI_STATE_STOPPING:
		if (old_state != DAI_STATE_READY &&
		    old_state != DAI_STATE_RUNNING &&
		    old_state != DAI_STATE_PAUSED) {
			return -EPERM;
		}
		break;
	case DAI_STATE_ERROR:
	case DAI_STATE_PRE_RUNNING:
		/* these states are not used so transitioning to them
		 * is considered invalid.
		 */
	default:
		return -EINVAL;
	}

	if (dir == DAI_DIR_RX) {
		data->rx_state = new_state;
	} else {
		data->tx_state = new_state;
	}

	return 0;
}

static inline void sai_tx_rx_force_disable(enum dai_dir dir,
					   uint32_t regmap)
{
	I2S_Type *base = UINT_TO_I2S(regmap);

	if (dir == DAI_DIR_RX) {
		base->RCSR = ((base->RCSR & 0xFFE3FFFFU) & (~I2S_RCSR_RE_MASK));
	} else {
		base->TCSR = ((base->TCSR & 0xFFE3FFFFU) & (~I2S_TCSR_TE_MASK));
	}
}

static inline void sai_tx_rx_sw_enable_disable(enum dai_dir dir,
					       struct sai_data *data,
					       bool enable)
{
	if (dir == DAI_DIR_RX) {
		data->rx_enabled = enable;
	} else {
		data->tx_enabled = enable;
	}
}

static inline int count_leading_zeros(uint32_t word)
{
	int num = 0;

	while (word) {
		if (!(word & 0x1)) {
			num++;
		} else {
			break;
		}

		word = word >> 1;
	}

	return num;
}

static inline void sai_tx_rx_set_dline_mask(enum dai_dir dir, uint32_t regmap, uint32_t mask)
{
	I2S_Type *base = UINT_TO_I2S(regmap);

	if (dir == DAI_DIR_RX) {
		base->RCR3 &= ~I2S_RCR3_RCE_MASK;
		base->RCR3 |= I2S_RCR3_RCE(mask);
	} else {
		base->TCR3 &= ~I2S_TCR3_TCE_MASK;
		base->TCR3 |= I2S_TCR3_TCE(mask);
	}
}

static inline void sai_dump_register_data(uint32_t regmap)
{
	I2S_Type *base = UINT_TO_I2S(regmap);

	LOG_DBG("TCSR: 0x%x", base->TCSR);
	LOG_DBG("RCSR: 0x%x", base->RCSR);

	LOG_DBG("TCR1: 0x%x", base->TCR1);
	LOG_DBG("RCR1: 0x%x", base->RCR1);

	LOG_DBG("TCR2: 0x%x", base->TCR2);
	LOG_DBG("RCR2: 0x%x", base->RCR2);

	LOG_DBG("TCR3: 0x%x", base->TCR3);
	LOG_DBG("RCR3: 0x%x", base->RCR3);

	LOG_DBG("TCR4: 0x%x", base->TCR4);
	LOG_DBG("RCR4: 0x%x", base->RCR4);

	LOG_DBG("TCR5: 0x%x", base->TCR5);
	LOG_DBG("RCR5: 0x%x", base->RCR5);

	LOG_DBG("TMR: 0x%x", base->TMR);
	LOG_DBG("RMR: 0x%x", base->RMR);

#ifdef CONFIG_SAI_HAS_MCLK_CONFIG_OPTION
	LOG_DBG("MCR: 0x%x", base->MCR);
#endif /* CONFIG_SAI_HAS_MCLK_CONFIG_OPTION */
}

#endif /* ZEPHYR_DRIVERS_DAI_NXP_SAI_H_ */

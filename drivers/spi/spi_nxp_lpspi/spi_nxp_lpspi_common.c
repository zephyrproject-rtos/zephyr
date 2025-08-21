/*
 * Copyright 2018, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is a collection of functions that would be useful for any driver for LPSPI.
 * This function/file has no knowledge of lpspi usage model by the driver
 * beyond basic configuration and should avoid making any assumptions about how
 * the driver is going to achieve the zephyr API.
 */

 /*
  * The other spi_nxp_lpspi driver source files also use DT_DRV_COMPAT and it is used by the
  * spi_context.h file to determine if the gpio cs code can be removed.
  * If DT_DRV_COMPAT is not defined in this file, the gpio cs code may not be removed for this file
  * but in the other spi_nxp_lpspi driver source files it may be removed and result in breakage of
  * this driver. Do not remove DT_DRV_COMPAT from this file.
  */
#define DT_DRV_COMPAT nxp_lpspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_lpspi, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

/* simple macro for readability of the equations used in the clock configuring */
#define TWO_EXP(power) BIT(power)

#if defined(LPSPI_RSTS) || defined(LPSPI_CLOCKS)
static LPSPI_Type *const lpspi_bases[] = LPSPI_BASE_PTRS;
#endif

#ifdef LPSPI_RSTS
static const reset_ip_name_t lpspi_resets[] = LPSPI_RSTS;

static inline reset_ip_name_t lpspi_get_reset(LPSPI_Type *const base)
{
	reset_ip_name_t rst = -1; /* invalid initial value */

	ARRAY_FOR_EACH(lpspi_bases, idx) {
		if (lpspi_bases[idx] == base) {
			rst = lpspi_resets[idx];
			break;
		}
	}

	__ASSERT_NO_MSG(rst != -1);
	return rst;

}
#endif

#ifdef LPSPI_CLOCKS
static const clock_ip_name_t lpspi_clocks[] = LPSPI_CLOCKS;

static inline clock_ip_name_t lpspi_get_clock(LPSPI_Type *const base)
{
	clock_ip_name_t clk = -1; /* invalid initial value */

	ARRAY_FOR_EACH(lpspi_bases, idx) {
		if (lpspi_bases[idx] == base) {
			clk = lpspi_clocks[idx];
			break;
		}
	}

	__ASSERT_NO_MSG(clk != -1);
	return clk;
}
#endif

int lpspi_wait_tx_fifo_empty(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	int arbitrary_cycle_limit = CONFIG_SPI_NXP_LPSPI_TXFIFO_WAIT_CYCLES;
	bool limit_wait = arbitrary_cycle_limit > 0;

	while (FIELD_GET(LPSPI_FSR_TXCOUNT_MASK, base->FSR) != 0) {
		if (!limit_wait) {
			continue;
		}

		if (arbitrary_cycle_limit-- < 0) {
			LOG_WRN("Failed waiting for TX fifo empty");
			return -EIO;
		}
	}

	return 0;
}

int spi_lpspi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct lpspi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static inline int lpspi_validate_xfer_args(const struct spi_config *spi_cfg)
{
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	uint32_t pcs = spi_cfg->slave;

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		/* the IP DOES support half duplex, need to implement driver support */
		LOG_WRN("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (word_size < 2 || (word_size % 32 == 1)) {
		/* Zephyr word size == hardware FRAME size (not word size)
		 * Max frame size: 4096 bits
		 *   (zephyr field is 6 bit wide for max 64 bit size, no need to check)
		 * Min frame size: 8 bits.
		 * Minimum hardware word size is 2. Since this driver is intended to work
		 * for 32 bit platforms, and 64 bits is max size, then only 33 and 1 are invalid.
		 */
		LOG_WRN("Word size %d not allowed", word_size);
		return -EINVAL;
	}

	if (pcs > LPSPI_CHIP_SELECT_COUNT - 1) {
		LOG_WRN("Peripheral %d select exceeds max %d", pcs, LPSPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	return 0;
}

static uint8_t lpspi_calc_delay_scaler(uint32_t desired_delay_ns,
				       uint32_t prescaled_clock,
				       uint32_t min_cycles)
{
	uint64_t delay_cycles;

	/* calculates the number of functional clock cycles needed to achieve delay */
	delay_cycles = (uint64_t)prescaled_clock * desired_delay_ns;
	delay_cycles = DIV_ROUND_UP(delay_cycles, NSEC_PER_SEC);

	/* clamp to minimally possible cycles to avoid underflow */
	delay_cycles = MAX(delay_cycles, min_cycles);

	/* what the min_cycles parameter is about is that
	* PCSSCK and SCKPSC are +1 cycles of the programmed value,
	* while DBT is +2 cycles of the programmed value.
	* So this calculates the value to program to the register.
	*/
	delay_cycles -= min_cycles;

	/* Don't overflow */
	delay_cycles = MIN(delay_cycles, UINT8_MAX);

	return (uint8_t)delay_cycles;
}

/* returns CCR mask of the bits 8-31 */
static inline uint32_t lpspi_set_delays(const struct device *dev, uint32_t prescaled_clock)
{
	const struct lpspi_config *config = dev->config;

	return LPSPI_CCR_PCSSCK(lpspi_calc_delay_scaler(config->pcs_sck_delay,
							prescaled_clock, 1)) |
	       LPSPI_CCR_SCKPCS(lpspi_calc_delay_scaler(config->sck_pcs_delay,
							prescaled_clock, 1)) |
	       LPSPI_CCR_DBT(lpspi_calc_delay_scaler(config->transfer_delay,
							prescaled_clock, 2));
}

/* This is the equation for the sck frequency given a div and prescaler. */
static uint32_t lpspi_calc_sck_freq(uint32_t src_clk_hz, uint16_t sckdiv, uint8_t prescaler)
{
	return (uint32_t)(src_clk_hz / (TWO_EXP(prescaler) * (sckdiv + 2)));
}

static inline uint8_t lpspi_calc_best_div_for_prescaler(uint32_t src_clk_hz,
							uint8_t prescaler,
							uint32_t req_freq)
{
	uint64_t prescaled_req_freq = TWO_EXP(prescaler) * req_freq;
	uint64_t ratio;

	if (prescaled_req_freq == 0) {
		ratio = UINT8_MAX + 2;
	} else {
		ratio = DIV_ROUND_UP(src_clk_hz, prescaled_req_freq);
	}

	ratio = MAX(ratio, 2);
	ratio -= 2;
	ratio = MIN(ratio, UINT8_MAX);

	return (uint8_t)ratio;
}

/* This function configures the clock control register (CCR) for the desired frequency
 * It does a binary search for the optimal CCR divider and TCR prescaler.
 * The prescale_value parameter is changed to the best value of the prescaler,
 * for use in setting the TCR outside this function.
 * The return value is the mask of the CCR (bits 0-7) required to set SCKDIV for best result.
 */
static inline uint32_t lpspi_set_sckdiv(uint32_t desired_freq,
					uint32_t clock_freq, uint8_t *prescale_value)
{
	uint8_t best_prescaler = 0, best_div = 0;
	uint32_t best_freq = 0;

	for (int8_t prescaler = 7U; prescaler >= 0; prescaler--) {
		/* if maximum freq (div = 0) won't get better than what we got with
		 * previous prescaler, then we can fast path exit this loop.
		 */
		if (lpspi_calc_sck_freq(clock_freq, 0, prescaler) < best_freq) {
			break;
		}

		/* the algorithm approaches the desired freq from below intentionally,
		 * therefore the min is our previous best and the max is the desired.
		 */
		uint8_t new_div = lpspi_calc_best_div_for_prescaler(clock_freq, prescaler,
								    desired_freq);
		uint32_t new_freq = lpspi_calc_sck_freq(clock_freq, new_div, prescaler);

		if (new_freq >= best_freq && new_freq <= desired_freq) {
			best_div = new_div;
			best_freq = new_freq;
			best_prescaler = prescaler;
		}
	}

	*prescale_value = best_prescaler;

	return LPSPI_CCR_SCKDIV(best_div);
}

/* This function configures everything except the TCR and the clock scaler */
static void lpspi_basic_config(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct lpspi_config *config = dev->config;
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t pcs_control_bit = 1 << (LPSPI_CFGR1_PCSPOL_SHIFT + spi_cfg->slave);
	uint32_t cfgr1_val = 0;

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		cfgr1_val |= pcs_control_bit;
	} else {
		cfgr1_val &= ~pcs_control_bit;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		cfgr1_val |= LPSPI_CFGR1_MASTER_MASK;
	}

	if (config->tristate_output) {
		cfgr1_val |= LPSPI_CFGR1_OUTCFG_MASK;
	}

	cfgr1_val |= config->data_pin_config << LPSPI_CFGR1_PINCFG_SHIFT;

	base->CFGR1 = cfgr1_val;

	if (IS_ENABLED(CONFIG_DEBUG)) {
		/* DEBUG mode makes it so the lpspi does not keep
		 * running while debugger has halted the chip.
		 * This makes debugging spi transfers easier.
		 */
		base->CR |= LPSPI_CR_DBGEN_MASK;
	}
}

int lpspi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct lpspi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool already_configured = spi_context_configured(ctx, spi_cfg);
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint32_t word_size = SPI_WORD_SIZE_GET(spi_cfg->operation);
	uint32_t clock_freq = 0;
	uint8_t prescaler = 0;
	int ret = 0;

	/* fast path to avoid reconfigure */
	/* TODO: S32K3 errata ERR050456 requiring module reset before every transfer,
	 * investigate alternative workaround so we don't have this latency for S32.
	 */
	if (already_configured && !IS_ENABLED(CONFIG_SOC_FAMILY_NXP_S32)) {
		return 0;
	}

	ret = lpspi_validate_xfer_args(spi_cfg);
	if (ret) {
		return ret;
	}

	/* For the purpose of configuring the LPSPI, 8 is the minimum frame size for the hardware */
	word_size = MAX(word_size, 8);

	/* specific driver implementation should set up watermarks and interrupts.
	 * we reset them here to avoid any unexpected events during configuring.
	 */
	base->FCR = 0;
	base->IER = 0;

	/* this is workaround for ERR050456 */
	base->CR |= LPSPI_CR_RST_MASK;
	base->CR |= LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;

	/* Setting the baud rate requires module to be disabled. */
	base->CR = 0;
	while ((base->CR & LPSPI_CR_MEN_MASK) != 0) {
		/* According to datasheet, should wait for this MEN bit to clear once idle */
	}

	data->ctx.config = spi_cfg;

	lpspi_basic_config(dev, spi_cfg);

	clock_freq = data->clock_freq;

	if (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_MASTER) {
		uint32_t ccr = 0;

		/* sckdiv algorithm must run *before* delays are set in order to know prescaler */
		ccr |= lpspi_set_sckdiv(spi_cfg->frequency, clock_freq, &prescaler);
		ccr |= lpspi_set_delays(dev, clock_freq / TWO_EXP(prescaler));

		/* note that not all bits of the register are readable on some platform,
		 * that's why we update it on one write
		 */
		base->CCR = ccr;
	}

	base->CR |= LPSPI_CR_MEN_MASK;

	base->TCR = LPSPI_TCR_CPOL(!!(spi_cfg->operation & SPI_MODE_CPOL)) |
		    LPSPI_TCR_CPHA(!!(spi_cfg->operation & SPI_MODE_CPHA)) |
		    LPSPI_TCR_LSBF(!!(spi_cfg->operation & SPI_TRANSFER_LSB)) |
		    LPSPI_TCR_FRAMESZ(word_size - 1) |
		    LPSPI_TCR_PRESCALE(prescaler) | LPSPI_TCR_PCS(spi_cfg->slave);

	return lpspi_wait_tx_fifo_empty(dev);
}

static void lpspi_module_system_init(LPSPI_Type *base)
{
#ifdef LPSPI_CLOCKS
	CLOCK_EnableClock(lpspi_get_clock(base));
#endif

#ifdef LPSPI_RSTS
	RESET_ReleasePeripheralReset(lpspi_get_reset(base));
#endif
}

int spi_nxp_init_common(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct lpspi_config *config = dev->config;
	struct lpspi_data *data = dev->data;
	int err = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	lpspi_module_system_init(base);

	data->major_version = (base->VERID & LPSPI_VERID_MAJOR_MASK) >> LPSPI_VERID_MAJOR_SHIFT;

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &data->clock_freq);
	if (err) {
		return err;
	}

	/* Full software reset */
	base->CR |= LPSPI_CR_RST_MASK;
	base->CR |= LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;
	base->CR = 0x00U;

	config->irq_config_func(dev);

	return err;
}

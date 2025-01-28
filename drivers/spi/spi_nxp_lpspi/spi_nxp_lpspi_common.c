/*
 * Copyright 2018, 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_nxp_lpspi_common, CONFIG_SPI_LOG_LEVEL);

#include "spi_nxp_lpspi_priv.h"

#define TWO_EXP(power) BIT(power)

static LPSPI_Type *const lpspi_bases[] = LPSPI_BASE_PTRS;
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

void lpspi_wait_tx_fifo_empty(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);

	while (FIELD_GET(LPSPI_FSR_TXCOUNT_MASK, base->FSR) != 0) {
	}
}

int spi_nxp_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_nxp_data *data = dev->data;

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

	if (word_size < 8 || (word_size % 32 == 1)) {
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

	if (pcs > LPSPI_CHIP_SELECT_COUNT) {
		LOG_WRN("Peripheral %d select exceeds max %d", pcs, LPSPI_CHIP_SELECT_COUNT - 1);
		return -EINVAL;
	}

	return 0;
}

static inline void lpspi_reset_state(LPSPI_Type *base)
{
	base->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK; /* flush fifos */
	base->IER = 0;                                     /* disable all interrupts */
	base->CR = 0;                                      /* reset control register */
	base->FCR = 0;                                     /* set watermarks to 0 */
}

static inline void lpspi_config_setup(LPSPI_Type *base, const struct spi_nxp_config *config,
				      const struct spi_config *spi_cfg)
{
	uint32_t cfgr1_val = base->CFGR1 & LPSPI_CFGR1_PCSCFG_MASK;
	uint32_t pcs_control_bit = 1 << (LPSPI_CFGR1_PCSPOL_SHIFT + spi_cfg->slave);

	if (spi_cfg->operation & SPI_CS_ACTIVE_HIGH) {
		cfgr1_val |= pcs_control_bit;
	} else {
		cfgr1_val &= ~pcs_control_bit;
	}

	cfgr1_val |= LPSPI_CFGR1_MASTER_MASK;
	cfgr1_val |= config->tristate_output ? LPSPI_CFGR1_OUTCFG_MASK : 0;
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

static inline uint32_t lpspi_set_sckdiv(LPSPI_Type *base, uint32_t desired_freq,
					uint32_t clock_freq, uint8_t *prescale_value)
{
	uint8_t best_prescaler = 0, best_div = 0;
	uint32_t best_freq = 0;

	for (uint8_t prescaler = 0U; prescaler < 8U; prescaler++) {
		uint8_t high = 255, low = 0, div = 255;
		uint32_t real_freq = 0;

		/* maximum freq won't get better than what we got with previous prescaler */
		if (clock_freq / (TWO_EXP(prescaler) * 2) < best_freq) {
			goto done;
		}

		while (div > 0) {
			div = low + (high - low) / 2;
			real_freq = (clock_freq / (TWO_EXP(prescaler) * (div + 2)));

			/* ensure that we do not exceed desired freq */
			if (real_freq > desired_freq) {
				low = div + 1;
				continue;
			} else {
				high = div - 1;
			}

			/* check if we are closer to the desired */
			if (real_freq >= best_freq) {
				best_prescaler = prescaler;
				best_div = div;
				best_freq = real_freq;
			}

			/* if our best found is a match, we're done */
			if (best_freq == desired_freq) {
				goto done;
			}
		}
	}

done:
	uint32_t ccr_val = base->CCR & ~LPSPI_CCR_SCKDIV_MASK;

	ccr_val |= LPSPI_CCR_SCKDIV(best_div);

	base->CCR = ccr_val;

	*prescale_value = best_prescaler;

	return best_freq;
}

static uint8_t lpspi_calc_delay(uint32_t desired_delay_ns, uint32_t best_delay_ns,
				uint32_t prescaled_clock, uint32_t additional_scaler)
{
	uint64_t real_delay = NSEC_PER_SEC / prescaled_clock;
	uint8_t best_scaler = 0, scaler = 0;
	uint32_t diff, min_diff = 0xFFFFFFFF;

	while (scaler < 256 && min_diff != 0) {
		real_delay *= ((uint64_t)scaler + 1 + (uint64_t)additional_scaler);

		/* Delay must not be less than desired */
		if (real_delay >= desired_delay_ns) {
			diff = (uint32_t)(real_delay - (uint64_t)desired_delay_ns);
			if (min_diff > diff) {
				/* a better match found */
				min_diff = diff;
				best_scaler = scaler;
			}
		}

		scaler++;
	}

	return best_scaler;
}

static inline void lpspi_set_delays(const struct device *dev, uint32_t prescaled_clock)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint64_t minimum_delay_ns = NSEC_PER_SEC / prescaled_clock;
	const struct spi_nxp_config *config = dev->config;
	uint32_t ccr = base->CCR;
	uint8_t scaler;

	ccr &= ~(LPSPI_CCR_PCSSCK_MASK | LPSPI_CCR_SCKPCS_MASK | LPSPI_CCR_DBT_MASK);

	scaler = (config->pcs_sck_delay <= minimum_delay_ns)
			 ? lpspi_calc_delay(config->pcs_sck_delay, minimum_delay_ns * 256,
					    prescaled_clock, 0)
			 : 0;
	ccr |= LPSPI_CCR_PCSSCK(scaler);

	scaler = (config->sck_pcs_delay <= minimum_delay_ns)
			 ? lpspi_calc_delay(config->sck_pcs_delay, minimum_delay_ns * 256,
					    prescaled_clock, 0)
			 : 0;
	ccr |= LPSPI_CCR_SCKPCS(scaler);

	scaler = (config->transfer_delay <= (2 * minimum_delay_ns))
			 ? lpspi_calc_delay(config->transfer_delay, minimum_delay_ns * 257,
					    prescaled_clock, 1)
			 : 0;
	ccr |= LPSPI_CCR_DBT(scaler);

	base->CCR = ccr;
}

static inline uint8_t lpspi_sck_config(const struct device *dev, uint32_t spi_freq,
				       uint32_t clock_freq)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	uint8_t prescaler = 0;

	lpspi_set_sckdiv(base, spi_freq, clock_freq, &prescaler);

	lpspi_set_delays(dev, clock_freq / TWO_EXP(prescaler));

	return prescaler;
}

int spi_nxp_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct spi_nxp_config *config = dev->config;
	struct spi_nxp_data *data = dev->data;
	uint32_t clock_freq = 0;
	int ret = 0;

	ret = lpspi_validate_xfer_args(spi_cfg);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq);
	if (ret) {
		return ret;
	}

	lpspi_reset_state(base);

	if (spi_cfg != data->ctx.config) {
		data->ctx.config = spi_cfg;

		lpspi_config_setup(base, config, spi_cfg);

		data->prescaler = lpspi_sck_config(dev, spi_cfg->frequency, clock_freq);
	}

	base->CR |= LPSPI_CR_MEN_MASK; /* enable the lpspi module */

	base->TCR = LPSPI_TCR_CPOL(!!(spi_cfg->operation & SPI_MODE_CPOL)) |
		    LPSPI_TCR_CPHA(!!(spi_cfg->operation & SPI_MODE_CPHA)) |
		    LPSPI_TCR_LSBF(!!(spi_cfg->operation & SPI_TRANSFER_LSB)) |
		    LPSPI_TCR_FRAMESZ(SPI_WORD_SIZE_GET(spi_cfg->operation)) |
		    LPSPI_TCR_PRESCALE(data->prescaler) | LPSPI_TCR_PCS(spi_cfg->slave);

	lpspi_wait_tx_fifo_empty(dev);

	return 0;
}

int spi_nxp_init_common(const struct device *dev)
{
	LPSPI_Type *base = (LPSPI_Type *)DEVICE_MMIO_NAMED_GET(dev, reg_base);
	const struct spi_nxp_config *config = dev->config;
	struct spi_nxp_data *data = dev->data;
	int err = 0;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	data->dev = dev;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	CLOCK_EnableClock(lpspi_clocks[lpspi_get_clock(base)]);

	err = spi_context_cs_configure_all(&data->ctx);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	config->irq_config_func(dev);

	return err;
}

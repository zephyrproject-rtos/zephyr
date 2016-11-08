/* spi_k64.c - Driver implementation for K64 SPI controller */

/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Portions of this file are derived from material that is
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>

#include <nanokernel.h>
#include <arch/cpu.h>

#include <misc/__assert.h>
#define SYS_LOG_LEVEL SYS_LOG_SPI_LEVEL
#include <misc/sys_log.h>
#include <board.h>
#include <init.h>

#include <sys_io.h>
#include <limits.h>
#include <power.h>

#include <spi.h>
#include <spi/spi_k64.h>
#include "spi_k64_priv.h"

/* SPI protocol frequency = K64 bus clock frequency, in hz */

#define SPI_K64_PROTOCOL_FREQ (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / \
							CONFIG_K64_BUS_CLOCK_DIVIDER)

/* SPI protocol period, in ns */

#define SPI_K64_PROTOCOL_PERIOD_NS	(NSEC_PER_SEC/SPI_K64_PROTOCOL_FREQ)

/* # of possible SPI baud rate and delay prescaler and scaler values */

#define SPI_K64_NUM_PRESCALERS	4
#define SPI_K64_NUM_SCALERS		16

/*
 * SPI baud rate prescaler and scaler values, indexed by the clocking and timing
 * attribute register (CTAR) parameters CTAR[PBR] and CTAR[BR], respectively.
 */

static const uint32_t baud_rate_prescaler[] = { 2, 3, 5, 7 };
static const uint32_t baud_rate_scaler[] = {
	2, 4, 6, 8, 16, 32, 64, 128, 256, 512,
	1024, 2048, 4096, 8192, 16384, 32768
};
/*
 * SPI delay prescaler and scaler values, indexed by clocking and timing
 * attribute register (CTAR) parameter pairs:
 * CTAR[PCSSCK]/CTAR[CSSCK] for the PCS to SCK delay,
 * CTAR[PASC]/CTAR[ASC] for the after SCK delay, and
 * CTAR[PDT]/CTAR[DT] for the after transfer delay.
 */

static const uint32_t delay_prescaler[] = { 1, 3, 5, 7 };
static const uint32_t delay_scaler[] = {
	2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	2048, 4096, 8192, 16384, 32768, 65536
};


/**
 * @brief Halt SPI module operation.
 * @param dev Pointer to the device structure for the driver instance
 * @return None.
 */
static inline void spi_k64_halt(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;

	/* Ensure module operation is stopped */

	sys_set_bit((info->regs + SPI_K64_REG_MCR), SPI_K64_MCR_HALT_BIT);

	while (sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_TXRXS) {
		SYS_LOG_DBG("SPI Controller dev %p is running.  Waiting for "
			    "Halt.\n", dev);
	}

}

/**
 * @brief Enable SPI module operation.
 * @param dev Pointer to the device structure for the driver instance
 * @return None.
 */
static inline void spi_k64_start(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;

	/* Allow module operation */

	sys_clear_bit((info->regs + SPI_K64_REG_MCR), SPI_K64_MCR_HALT_BIT);

}

/**
 * @brief Set a SPI baud rate nearest to the desired rate, without exceeding it.
 * @param baud_rate The desired baud rate.
 * @param ctar_ptr Pointer to clocking and timing attribute storage.
 * @return The calculated baud rate or 0 if an error occurred.
 */
static uint32_t spi_k64_set_baud_rate(uint32_t baud_rate, uint32_t *ctar_ptr)
{
	/*
	 * The 'volatile' attribute is added to some of the variables in this
	 * function to prevent bad code generation by gcc toolchains for ARM
	 * when an optimization setting above -O0 is used.
	 *
	 * Specifically, a register is loaded with the constant 0 and is used as
	 * the divisor in a following divide instruction, resulting in a
	 * divide-by-zero exception.
	 * This issue has been seen with gcc versions 4.8.1 and 5.2.0.
	 */

	/* prescaler values,CTAR[PBR] */
	uint32_t prescaler;
	volatile uint32_t best_prescaler;
	/* scaler values, CTAR[BR] */
	uint32_t scaler;
	volatile uint32_t best_scaler;
	/* doubler values,CTAR[DBR]-1 */
	uint32_t dbr, best_dbr;
	/* baud rate */
	uint32_t calc_baud_rate;
	volatile uint32_t best_baud_rate;
	/* calculated differences */
	uint32_t diff, min_diff;

	min_diff = 0xFFFFFFFFU;
	best_dbr = 1;

	/*
	 * Master mode is assumed.
	 *
	 * Find the combination of prescaler, scaler and doubler factors that
	 * results in the baud rate closest to the requested value, without
	 * exceeding it.
	 */

	SYS_LOG_DBG("spi_k64_set_baud_rate - ");

	/*
	 * Initialize the prescaler and scaler to their maximum values to calculate
	 * the minimum baud rate and check if it is greater than the desired rate.
	 */

	best_prescaler = SPI_K64_NUM_PRESCALERS - 1;
	best_scaler = SPI_K64_NUM_SCALERS - 1;
	best_baud_rate = SPI_K64_PROTOCOL_PERIOD_NS /
						(baud_rate_prescaler[best_prescaler] *
							baud_rate_prescaler[best_scaler]);

	if (best_baud_rate > baud_rate) {
		SYS_LOG_DBG("ERROR : Minimum baud rate %d is greater than "
			    "desired rate %d\n", best_baud_rate, baud_rate);

		return 0;
	}


	/*
	 * Note that no further combinations are checked if the calculated baud rate
	 * equals the requested baud rate.
	 */

	for (prescaler = 0;
		(prescaler < SPI_K64_NUM_PRESCALERS) && min_diff;
		prescaler++) {

		for (scaler = 0; (scaler < SPI_K64_NUM_SCALERS) && min_diff; scaler++) {

			for (dbr = 1; (dbr < 3) && min_diff; dbr++) {

				calc_baud_rate = ((SPI_K64_PROTOCOL_FREQ * dbr) /
									(baud_rate_prescaler[prescaler] *
										baud_rate_scaler[scaler]));

				/* ensure the rate will not exceed the one requested */

				if (baud_rate >= calc_baud_rate) {

					diff = baud_rate - calc_baud_rate;

					if (min_diff > diff) {

						/* a better match was found */

						min_diff = diff;
						best_prescaler = prescaler;
						best_scaler = scaler;
						best_baud_rate = calc_baud_rate;
						best_dbr = dbr;
					}

				}

			}

		}

	}

	/* save the best baud rate dbr, prescaler and scaler */

	*ctar_ptr = *ctar_ptr | SPI_K64_CTAR_DBR_SET(best_dbr - 1) |
				SPI_K64_CTAR_PBR_SET(best_prescaler) | best_scaler;

	/* return the actual baud rate */

	SYS_LOG_DBG("%d bps desired, %d bps set\n", baud_rate, best_baud_rate);

	return best_baud_rate;
}

/**
 * @brief Set the specified delay nearest to the desired value, but not lower.
 * @param delay_id The delay identifier.
 * @param delay_ns The desired delay value, in ns.
 * @param ctar_ptr Pointer to clocking and timing attribute storage.
 * @return The calculated delay or 0 if an error occurred.
 */
static uint32_t spi_k64_set_delay(enum spi_k64_delay_id delay_id,
									uint32_t delay_ns,
									uint32_t *ctar_ptr)
{
	/*
	 * The 'volatile' attribute is added to some of the variables in this
	 * function to prevent bad code generation by gcc toolchains for ARM
	 * when an optimization setting above -O0 is used.
	 *
	 * Specifically, a register is loaded with the constant 0 and is used as
	 * the divisor in a following divide instruction, resulting in a
	 * divide-by-zero exception.
	 * This issue has been seen with gcc versions 4.8.1 and 5.2.0.
	 */

	uint32_t prescaler;							/* prescaler values */
	volatile uint32_t best_prescaler;
	uint32_t scaler;							/* scaler values */
	volatile uint32_t best_scaler;
	uint32_t calc_delay;						/* delay values */
	volatile uint32_t best_delay;
	uint32_t diff, min_diff;					/* difference values */

	SYS_LOG_DBG("spi_k64_set_delay - ");

	/*
	 * This function can calculate the clocking and timing attribute register
	 * (CTAR) values for:
	 * - PCS to SCK delay prescaler (PCSSCK) and scaler (CSSCK),
	 * - After SCK delay prescaler (PASC) and scaler (ASC), or
	 * - Delay after transfer prescaler (PDT) and scaler (DT).
	 */

	if ((delay_id != DELAY_PCS_TO_SCK) && (delay_id != DELAY_AFTER_SCK) &&
		(delay_id != DELAY_AFTER_XFER)) {

		SYS_LOG_DBG("ERROR : Unknown delay type %d\n", delay_id);

		return 0;
	}

	/*
	 * Initialize the prescaler and scaler to their maximum values to calculate
	 * the maximum delay and check if it is less than the desired delay.
	 */

	best_prescaler = SPI_K64_NUM_PRESCALERS - 1;
	best_scaler = SPI_K64_NUM_SCALERS - 1;
	best_delay = SPI_K64_PROTOCOL_PERIOD_NS * delay_prescaler[best_prescaler] *
				 delay_scaler[best_scaler];

	if (best_delay < delay_ns) {
		SYS_LOG_DBG("ERROR : Maximum delay %d does meet desired minimum"
			    " of %d\n", best_delay, delay_ns);

		return 0;
	}

	min_diff = 0xFFFFFFFFU;

	/*
	 * Check if the minimum delay (prescaler value = 1, scaler value = 2) is
	 * greater than the desired delay.  If so, set the prescaler and scaler to
	 * their associated minimum in the CTAR (0).
	 */

	calc_delay = SPI_K64_PROTOCOL_PERIOD_NS * 2;

	if (calc_delay >= delay_ns) {
		best_prescaler = 0;
		best_scaler = 0;
		min_diff = 0;	/* skip remaining calculations */
	}


	/*
	 * Note that no further combinations are checked if the calculated delay
	 * equals the requested delay.
	 */

	for (prescaler = 0;
		(prescaler < SPI_K64_NUM_PRESCALERS) && min_diff;
		prescaler++) {

		for (scaler = 0; (scaler < SPI_K64_NUM_SCALERS) && min_diff; scaler++) {

			calc_delay = SPI_K64_PROTOCOL_PERIOD_NS *
						delay_prescaler[prescaler] * delay_scaler[scaler];

			/* ensure the delay is at least as long as the one requested */

			if (calc_delay >= delay_ns) {

				diff = calc_delay - delay_ns;

				if (min_diff > diff) {

					/* a better match was found */

					min_diff = diff;
					best_prescaler = prescaler;
					best_scaler = scaler;
					best_delay = calc_delay;
				}

			}

		}

	}

	/* save the best delay prescaler and scaler */

	switch (delay_id) {

	case DELAY_PCS_TO_SCK:
		*ctar_ptr = *ctar_ptr | SPI_K64_CTAR_PCSSCK_SET(best_prescaler) |
					SPI_K64_CTAR_CSSCK_SET(best_scaler);
		SYS_LOG_DBG("DELAY_PCS_TO_SCK: ");
		break;

	case DELAY_AFTER_SCK:
		*ctar_ptr = *ctar_ptr | SPI_K64_CTAR_PASC_SET(best_prescaler) |
					SPI_K64_CTAR_ASC_SET(best_scaler);
		SYS_LOG_DBG("DELAY_AFTER_SCK: ");
		break;

	case DELAY_AFTER_XFER:
		*ctar_ptr = *ctar_ptr | SPI_K64_CTAR_PDT_SET(best_prescaler) |
					SPI_K64_CTAR_DT_SET(best_scaler);
		SYS_LOG_DBG("DELAY_AFTER_XFER: ");
		break;

	default:
		break;

	}

	/* return the actual delay */

	SYS_LOG_DBG("%d delay desired, %d delay set\n", delay_ns, best_delay);

	return best_delay;
}


/**
 * @brief Configure the SPI host controller for operating against slaves
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to the application provided configuration
 *
 * @return 0 if successful, another DEV_* code otherwise.
 */
static int spi_k64_configure(struct device *dev, struct spi_config *config)
{
	const struct spi_k64_config *info = dev->config->config_info;
	struct spi_k64_data *spi_data = dev->driver_data;
	uint32_t flags = config->config;
	uint32_t mcr;		/* mode configuration attributes, for MCR */
	uint32_t ctar = 0;	/* clocking and timing attributes, for CTAR */
	uint32_t frame_sz;	/* frame size, in bits */

	SYS_LOG_DBG("spi_k64_configure: dev %p (regs @ 0x%x), ", dev,
		    info->regs);
	SYS_LOG_DBG("config 0x%x, freq 0x%x",
		    config->config, config->max_sys_freq);

	 /* Disable transfer operations during configuration */

	spi_k64_halt(dev);

	/*
	 * Set the common configuration:
	 * Master mode, normal SPI transfers, PCS strobe disabled,
	 * Rx overflow data ignored, PCSx inactive low signal, Doze disabled,
	 * Rx/Tx FIFOs enabled.
	 *
	 * Also, keep transfers disabled.
	 */

	mcr = SPI_K64_MCR_MSTR | SPI_K64_MCR_HALT;

	/* Set PCSx signal polarities and continuous SCK, as requested */

	mcr |= (SPI_K64_MCR_PCSIS_SET(SPI_PCS_POL_GET(flags)) |
			SPI_K64_MCR_CONT_SCKE_SET(SPI_CONT_SCK_GET(flags)));

	sys_write32(mcr, (info->regs + SPI_K64_REG_MCR));


	/* Set clocking and timing parameters */


	/* SCK polarity and phase, and bit order of data */

	if (flags & SPI_MODE_CPOL) {
		ctar |= SPI_K64_CTAR_CPOL;
	}

	if (flags & SPI_MODE_CPHA) {
		ctar |= SPI_K64_CTAR_CPHA;
	}

	if (flags & SPI_TRANSFER_MASK) {
		ctar |= SPI_K64_CTAR_LSBFE;
	}

	/*
	 * Frame size is limited to 16 bits (vs. 8 bit value in struct spi_config),
	 * programmed as: (frame_size - 1)
	 */

	frame_sz = SPI_WORD_SIZE_GET(flags);
	if (frame_sz > SPI_K64_WORD_SIZE_MAX) {
		return -ENOTSUP;
	}

	spi_data->frame_sz = frame_sz;

	ctar |= (SPI_K64_CTAR_FRMSZ_SET(frame_sz - 1));

	/* Set baud rate and signal timing parameters (delays) */

	if (spi_k64_set_baud_rate(config->max_sys_freq, &ctar) == 0) {
		return -ENOTSUP;
	}

	/*
	 * Set signal timing parameters (delays):
	 * - PCS to SCK delay is set to the minimum, CTAR[PCSSCK] = CTAR[CSSCK] = 0;
	 * - After SCK delay is set to at least half of the baud rate period,
	 *   (using the combination of CTAR[PASC] and CTAR[ASC]); and
	 * - Delay after transfer is set to the minimum, CTAR[PDT] = CTAR[DT] = 0.
	 */

	if (spi_k64_set_delay(DELAY_AFTER_SCK,
							(NSEC_PER_SEC / 2) / config->max_sys_freq,
							&ctar) == 0) {
		return -ENOTSUP;
	}


	SYS_LOG_DBG("spi_k64_configure: MCR: 0x%x CTAR0: 0x%x\n", mcr, ctar);

	sys_write32(ctar, (info->regs + SPI_K64_REG_CTAR0));

	/* Initialize Tx/Rx parameters */

	spi_data->tx_buf = spi_data->rx_buf = NULL;
	spi_data->tx_buf_len = spi_data->rx_buf_len = 0;

	/* Store continuous slave/PCS signal selection mode */

	spi_data->cont_pcs_sel = SPI_CONT_PCS_GET(flags);

	return 0;
}

/**
 * @brief Select a slave to transmit data to.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param slave An integer identifying the slave, where the bit values denote:
 *				0 - Negate the associated PCS signal
 *				1 - Assert the associated PCS signal
 *
 * Note: The polarity of each PCS signal is defined by the Peripheral Chip
 * Select inactive state setting, MCR[PCSIS], determined by the configuration
 * data parameter to spi_configure()/spi_k64_configure().
 *
 * @return 0 if successful, another DEV_* code otherwise.
 */
static int spi_k64_slave_select(struct device *dev, uint32_t slave)
{
	struct spi_k64_data *spi_data = dev->driver_data;

	/*
	 * Note that the number of valid PCS signals differs for each
	 * K64 SPI module:
	 * - SPI0 uses PCS0-5;
	 * - SPI1 uses PCS0-3;
	 * - SPI2 uses PCS0-1;
	 */

	SYS_LOG_DBG("spi_k64_slave_select: slave 0x%x selected for dev %p\n",
		(uint8_t)slave, dev);

	spi_data->pcs = (uint8_t)slave;

	return 0;
}

/**
 * @brief Read and/or write a defined amount of data through an SPI driver
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param tx_buf Memory buffer that data should be transferred from
 * @param tx_buf_len Size of the memory buffer available for reading from
 * @param rx_buf Memory buffer that data should be transferred to
 * @param rx_buf_len Size of the memory buffer available for writing to
 *
 * @return 0 if successful, another DEV_* code otherwise.
 */
static int spi_k64_transceive(struct device *dev,
				const void *tx_buf, uint32_t tx_buf_len,
				void *rx_buf, uint32_t rx_buf_len)
{
	const struct spi_k64_config *info = dev->config->config_info;
	struct spi_k64_data *spi_data = dev->driver_data;
	uint32_t int_config;	/* interrupt configuration */

	SYS_LOG_DBG("spi_k64_transceive: dev %p, Tx buf %p, ", dev, tx_buf);
	SYS_LOG_DBG("Tx len %u, Rx buf %p, Rx len %u\n", tx_buf_len, rx_buf,
		    rx_buf_len);

#ifdef CONFIG_SPI_DEBUG
	__ASSERT(!((tx_buf_len && (tx_buf == NULL)) ||
				(rx_buf_len && (rx_buf == NULL))),
			"spi_k64_transceive: ERROR - NULL buffer");
#endif

	/* Check Tx FIFO status */

	if (tx_buf_len &&
		((sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_TFFF) == 0)) {

		SYS_LOG_DBG("spi_k64_transceive: Tx FIFO is already full\n");
		return -EBUSY;
	}

	/* Set buffers info */
	spi_data->tx_buf = tx_buf;
	spi_data->tx_buf_len = tx_buf_len;
	spi_data->rx_buf = rx_buf;
	spi_data->rx_buf_len = rx_buf_len;

	/* enable transfer operations - must be done before enabling interrupts */

	spi_k64_start(dev);

	/*
	 * Enable interrupts:
	 * - Transmit FIFO Fill (Tx FIFO not full); and/or
	 * - Receive FIFO Drain (Rx FIFO not empty);
	 *
	 * Note: DMA requests are not supported.
	 */

	int_config = sys_read32(info->regs + SPI_K64_REG_RSER);

	if (tx_buf_len) {

		int_config |= SPI_K64_RSER_TFFF_RE;
	}

	if (rx_buf_len) {

		int_config |= SPI_K64_RSER_RFDF_RE;
	}

	sys_write32(int_config, (info->regs + SPI_K64_REG_RSER));

    /* wait for transfer to complete */

	device_sync_call_wait(&spi_data->sync_info);

    /* check completion status */

	if (spi_data->error) {
		spi_data->error = 0;
		return -EIO;
	}

	return 0;
}

/**
 * @brief SPI module data push (write) operation.
 * @param dev Pointer to the device structure for the driver instance
 * @return None.
 */
static void spi_k64_push_data(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;
	struct spi_k64_data *spi_data = dev->driver_data;
	uint32_t data;
#ifdef CONFIG_SPI_DEBUG
	uint32_t cnt = 0;		/* # of bytes pushed */
#endif

	SYS_LOG_DBG("spi_k64_push_data - ");

	do {	/* initial status already checked by spi_k64_isr() */

		if (spi_data->tx_buf && (spi_data->tx_buf_len > 0)) {

			if (spi_data->frame_sz > CHAR_BIT) {

				/* get 2nd byte with frame sizes larger than 8 bits  */

				data = (uint32_t)(*(uint16_t *)(spi_data->tx_buf));

				spi_data->tx_buf += 2;
				spi_data->tx_buf_len -= 2;

#ifdef CONFIG_SPI_DEBUG
				cnt += 2;
#endif
			} else {

				data = (uint32_t)(*(spi_data->tx_buf));

				spi_data->tx_buf++;
				spi_data->tx_buf_len--;

#ifdef CONFIG_SPI_DEBUG
				cnt++;
#endif
			}

			/* Write data to the selected slave */

			if (spi_data->cont_pcs_sel && (spi_data->tx_buf_len == 0)) {

				/* clear continuous PCS enabling in the last frame */

				sys_write32((data | SPI_K64_PUSHR_PCS_SET(spi_data->pcs)),
							(info->regs + SPI_K64_REG_PUSHR));

			} else {

				sys_write32((data | SPI_K64_PUSHR_PCS_SET(spi_data->pcs) |
								SPI_K64_PUSHR_CONT_SET(spi_data->cont_pcs_sel)),
							(info->regs + SPI_K64_REG_PUSHR));
			}

			/* Clear interrupt */

			sys_write32(SPI_K64_SR_TFFF, (info->regs + SPI_K64_REG_SR));

		} else {

			/* Nothing more to push */

			break;
		}

	} while (sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_TFFF);

	SYS_LOG_DBG("pushed: %d\n", cnt);

}

/**
 * @brief SPI module data pull (read) operation.
 * @param dev Pointer to the device structure for the driver instance
 * @return None.
 */
static void spi_k64_pull_data(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;
	struct spi_k64_data *spi_data = dev->driver_data;
	uint16_t data;
#ifdef CONFIG_SPI_DEBUG
	uint32_t cnt = 0;		/* # of bytes pulled */
#endif

	SYS_LOG_DBG("spi_k64_pull_data - ");

	do {	/* initial status already checked by spi_k64_isr() */

		if (spi_data->rx_buf && spi_data->rx_buf_len > 0) {

			data = (uint16_t)sys_read32(info->regs + SPI_K64_REG_POPR);

			if (spi_data->frame_sz > CHAR_BIT) {

				/* store 2nd byte with frame sizes larger than 8 bits  */

				*((uint16_t *)(spi_data->rx_buf)) = data;
				spi_data->rx_buf += 2;
				spi_data->rx_buf_len -= 2;

#ifdef CONFIG_SPI_DEBUG
				cnt += 2;
#endif
			} else {

				*(spi_data->rx_buf) = (uint8_t)data;
				spi_data->rx_buf++;
				spi_data->rx_buf_len--;

#ifdef CONFIG_SPI_DEBUG
				cnt++;
#endif
			}

			/* Clear interrupt */

			sys_write32(SPI_K64_SR_RFDF, (info->regs + SPI_K64_REG_SR));

		} else {

			/* No buffer to store data to */

			break;
		}

	} while (sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_RFDF);


	SYS_LOG_DBG("pulled: %d\n", cnt);
}

/**
 * @brief Complete SPI module data transfer operations.
 * @param dev Pointer to the device structure for the driver instance
 * @param error Error condition (0 = no error, otherwise an error occurred)
 * @return None.
 */
static void spi_k64_complete(struct device *dev, uint32_t error)
{
	struct spi_k64_data *spi_data = dev->driver_data;
	const struct spi_k64_config *info = dev->config->config_info;
	uint32_t int_config;	/* interrupt configuration */

	if (error) {

		SYS_LOG_DBG("spi_k64_complete - ERROR condition\n");

		goto complete;
	}

	/* Check for a completed transfer */

	if (spi_data->tx_buf && (spi_data->tx_buf_len == 0) && !spi_data->rx_buf) {

		/* disable Tx interrupts */

		int_config = sys_read32(info->regs + SPI_K64_REG_RSER);

		int_config &= ~SPI_K64_RSER_TFFF_RE;

		sys_write32(int_config, (info->regs + SPI_K64_REG_RSER));

	} else if (spi_data->rx_buf && (spi_data->rx_buf_len == 0) &&
				!spi_data->tx_buf) {

		/* disable Rx interrupts */

		int_config = sys_read32(info->regs + SPI_K64_REG_RSER);

		int_config &= ~SPI_K64_RSER_RFDF_RE;

		sys_write32(int_config, (info->regs + SPI_K64_REG_RSER));

	} else if (spi_data->tx_buf && spi_data->tx_buf_len == 0 &&
			spi_data->rx_buf && spi_data->rx_buf_len == 0) {

		/* disable Tx, Rx interrupts */

		int_config = sys_read32(info->regs + SPI_K64_REG_RSER);

		int_config &= ~(SPI_K64_RSER_TFFF_RE | SPI_K64_RSER_RFDF_RE);

		sys_write32(int_config, (info->regs + SPI_K64_REG_RSER));

	} else {

		return;
	}

complete:

	spi_data->tx_buf = spi_data->rx_buf = NULL;
	spi_data->tx_buf_len = spi_data->rx_buf_len = 0;

	/* Disable transfer operations */

	spi_k64_halt(dev);

    /* Save status */

	spi_data->error = error;

    /* Signal completion */

	device_sync_call_complete(&spi_data->sync_info);
}

/**
 * @brief SPI module interrupt handler.
 * @param arg Pointer to the device structure for the driver instance
 * @return None.
 */
void spi_k64_isr(void *arg)
{
	struct device *dev = arg;
	const struct spi_k64_config *info = dev->config->config_info;
	uint32_t error = 0;
	uint32_t status;

	status = sys_read32(info->regs + SPI_K64_REG_SR);

	SYS_LOG_DBG("spi_k64_isr: dev %p, status 0x%x\n", dev, status);

	if (status & (SPI_K64_SR_RFOF | SPI_K64_SR_TFUF)) {

		/* Unrecoverable error: Rx overflow, Tx underflow */

		error = 1;

	} else {

		if (status & SPI_K64_SR_TFFF) {
			spi_k64_push_data(dev);
		}

		if (status & SPI_K64_SR_RFDF) {
			spi_k64_pull_data(dev);
		}

	}

	/* finish processing, if data transfer is complete */

	spi_k64_complete(dev, error);
}

static const struct spi_driver_api k64_spi_api = {
	.configure = spi_k64_configure,
	.slave_select = spi_k64_slave_select,
	.transceive = spi_k64_transceive,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/**
 * @brief API to set device power state
 *
 * This function simply sets the device power state in driver_data
 *
 * @param dev Device struct
 * @param power_state device power state to be saved
 * @return N/A
 */
static void spi_k64_set_power_state(struct device *dev, uint32_t power_state)
{
	struct spi_k64_data *context = dev->driver_data;

	context->device_power_state = power_state;
}
#else
#define spi_k64_set_power_state(...)
#endif

int spi_k64_init(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;
	struct spi_k64_data *data = dev->driver_data;
	uint32_t mcr;

	/* Enable module clocking */

	sys_set_bit(info->clk_gate_reg, info->clk_gate_bit);

	/*
	 * Ensure module operation is stopped and enabled before writing anything
	 * more to the registers.
	 * (Clear MCR[MDIS] and set MCR[HALT].)
	 */

	SYS_LOG_DBG("halt\n");
	mcr = SPI_K64_MCR_HALT;
	sys_write32(mcr, (info->regs + SPI_K64_REG_MCR));

	while (sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_TXRXS) {
		SYS_LOG_DBG("SPI Controller dev %p is running.  Waiting for "
			    "Halt.\n", dev);
	}

	/* Clear Tx and Rx FIFOs */

	mcr |= (SPI_K64_MCR_CLR_RXF | SPI_K64_MCR_CLR_TXF);

	SYS_LOG_DBG("fifo clr\n");
	sys_write32(mcr, (info->regs + SPI_K64_REG_MCR));

	/* Set master mode */

	mcr = SPI_K64_MCR_MSTR | SPI_K64_MCR_HALT;
	SYS_LOG_DBG("master mode\n");
	sys_write32(mcr, (info->regs + SPI_K64_REG_MCR));

	/* Disable SPI module interrupt generation */

	SYS_LOG_DBG("irq disable\n");
	sys_write32(0, (info->regs + SPI_K64_REG_RSER));

	/* Clear status */

	SYS_LOG_DBG("status clr\n");
	sys_write32((SPI_K64_SR_RFDF | SPI_K64_SR_RFOF | SPI_K64_SR_TFUF |
				SPI_K64_SR_EOQF	| SPI_K64_SR_TCF),
				(info->regs + SPI_K64_REG_SR));

    /* Set up the synchronous call mechanism */

	device_sync_call_init(&data->sync_info);

	/* Configure and enable SPI module IRQs */

	info->config_func();

	spi_k64_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	irq_enable(info->irq);

	/*
	 * Enable Rx overflow interrupt generation.
	 * Note that Tx underflow is only generated when in slave mode.
	 */

	SYS_LOG_DBG("rxfifo overflow enable\n");
	sys_write32(SPI_K64_RSER_RFOF_RE, (info->regs + SPI_K64_REG_RSER));

	SYS_LOG_DBG("K64 SPI Driver initialized on device: %p\n", dev);

	/* operation remains disabled (MCR[HALT] = 1)*/

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/**
 * @brief API to get device power state
 *
 * This function simply returns the device power state
 * from driver_data
 *
 * @param dev Device struct
 * @return device power state
 */
static uint32_t spi_k64_get_power_state(struct device *dev)
{
	struct spi_k64_data *context = dev->driver_data;

	return context->device_power_state;
}
/**
 * @brief Suspend SPI host controller operations.
 * @param dev Pointer to the device structure for the driver instance
 * @return 0 if successful, a negative errno value otherwise.
 */
static int spi_k64_suspend(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;

	SYS_LOG_DBG("spi_k64_suspend: %p\n", dev);

	if (sys_read32(info->regs + SPI_K64_REG_SR) & SPI_K64_SR_TXRXS)
		return -EBUSY;

	/* disable module */

	sys_set_bit((info->regs + SPI_K64_REG_MCR), SPI_K64_MCR_MDIS_BIT);

	spi_k64_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	irq_disable(info->irq);

	return 0;
}

/**
 * @brief Resume SPI host controller operations.
 * @param dev Pointer to the device structure for the driver instance
 * @return 0 if successful, a negative errno value otherwise.
 */
static int spi_k64_resume_from_suspend(struct device *dev)
{
	const struct spi_k64_config *info = dev->config->config_info;

	SYS_LOG_DBG("spi_k64_resume: %p\n", dev);

	/* enable module */

	sys_clear_bit((info->regs + SPI_K64_REG_MCR), SPI_K64_MCR_MDIS_BIT);

	spi_k64_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	irq_enable(info->irq);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int spi_k64_device_ctrl(struct device *dev, uint32_t ctrl_command,
			       void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return spi_k64_suspend(dev);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return spi_k64_resume_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = spi_k64_get_power_state(dev);
		return 0;
	}

	return 0;
}
#else
#define spi_k64_set_power_state(...)
#endif

/* system bindings */
#ifdef CONFIG_SPI_0

void spi_config_0_irq(void);

struct spi_k64_data spi_k64_data_port_0;

static const struct spi_k64_config spi_k64_config_0 = {
	.regs = SPI_K64_0_BASE_ADDR,
	.clk_gate_reg = SPI_K64_0_CLK_GATE_REG_ADDR,
	.clk_gate_bit = SPI_K64_0_CLK_GATE_REG_BIT,
	.irq = SPI_K64_0_IRQ,
	.config_func = spi_config_0_irq
};

DEVICE_DEFINE(spi_k64_port_0, CONFIG_SPI_0_NAME, spi_k64_init,
	      spi_k64_device_ctrl, &spi_k64_data_port_0,
	      &spi_k64_config_0, PRE_KERNEL_1,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &k64_spi_api);


void spi_config_0_irq(void)
{
	IRQ_CONNECT(SPI_K64_0_IRQ, CONFIG_SPI_0_IRQ_PRI,
		    spi_k64_isr, DEVICE_GET(spi_k64_port_0), 0);
}

#endif /* CONFIG_SPI_0 */


#ifdef CONFIG_SPI_1

void spi_config_1_irq(void);

struct spi_k64_data spi_k64_data_port_1;

static const struct spi_k64_config spi_k64_config_1 = {
	.regs = SPI_K64_1_BASE_ADDR,
	.clk_gate_reg = SPI_K64_1_CLK_GATE_REG_ADDR,
	.clk_gate_bit = SPI_K64_1_CLK_GATE_REG_BIT,
	.irq = SPI_K64_1_IRQ,
	.config_func = spi_config_1_irq
};

DEVICE_DEFINE(spi_k64_port_1, CONFIG_SPI_1_NAME, spi_k64_init,
	      spi_k64_device_ctrl, &spi_k64_data_port_1,
	      &spi_k64_config_1, PRE_KERNEL_1,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &k64_spi_api);


void spi_config_1_irq(void)
{
	IRQ_CONNECT(SPI_K64_1_IRQ, CONFIG_SPI_1_IRQ_PRI,
		    spi_k64_isr, DEVICE_GET(spi_k64_port_1), 0);
}

#endif /* CONFIG_SPI_1 */


#ifdef CONFIG_SPI_2

void spi_config_2_irq(void);

struct spi_k64_data spi_k64_data_port_2;

static const struct spi_k64_config spi_k64_config_2 = {
	.regs = SPI_K64_2_BASE_ADDR,
	.clk_gate_reg = SPI_K64_2_CLK_GATE_REG_ADDR,
	.clk_gate_bit = SPI_K64_2_CLK_GATE_REG_BIT,
	.irq = SPI_K64_2_IRQ,
	.config_func = spi_config_2_irq
};

DEVICE_DEFINE(spi_k64_port_2, CONFIG_SPI_2_NAME, spi_k64_init,
	      spi_k64_device_ctrl, &spi_k64_data_port_2,
	      &spi_k64_config_2, PRE_KERNEL_1,
	      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &k64_spi_api);


void spi_config_2_irq(void)
{
	IRQ_CONNECT(SPI_K64_2_IRQ, CONFIG_SPI_2_IRQ_PRI,
		    spi_k64_isr, DEVICE_GET(spi_k64_port_2), 0);
}

#endif /* CONFIG_SPI_2 */

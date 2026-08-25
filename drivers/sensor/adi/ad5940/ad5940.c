/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 Analog Devices, Inc.
 */

#include "ad5940.h"

#include <math.h>
#include <zephyr/drivers/sensor/ad5940.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(ad5940, CONFIG_SENSOR_LOG_LEVEL);

static int ad5940_fifo_config(const struct device *dev, uint8_t src);

#ifdef CONFIG_AD5940_SEQGEN
static int ad5940_seqgen_write(const struct device *dev, uint16_t addr, uint32_t val);
static int ad5940_seqgen_read(const struct device *dev, uint16_t addr, uint32_t *val);
#endif

static const uint16_t seq_info_regs[4] = {
	AD5940_REG_SEQ0INFO, AD5940_REG_SEQ1INFO,
	AD5940_REG_SEQ2INFO, AD5940_REG_SEQ3INFO,
};
static const uint16_t seq_wupl_regs[4] = {
	AD5940_REG_SEQ0WUPL, AD5940_REG_SEQ1WUPL,
	AD5940_REG_SEQ2WUPL, AD5940_REG_SEQ3WUPL,
};
static const uint16_t seq_wuph_regs[4] = {
	AD5940_REG_SEQ0WUPH, AD5940_REG_SEQ1WUPH,
	AD5940_REG_SEQ2WUPH, AD5940_REG_SEQ3WUPH,
};
static const uint16_t seq_sleepl_regs[4] = {
	AD5940_REG_SEQ0SLEEPL, AD5940_REG_SEQ1SLEEPL,
	AD5940_REG_SEQ2SLEEPL, AD5940_REG_SEQ3SLEEPL,
};
static const uint16_t seq_sleeph_regs[4] = {
	AD5940_REG_SEQ0SLEEPH, AD5940_REG_SEQ1SLEEPH,
	AD5940_REG_SEQ2SLEEPH, AD5940_REG_SEQ3SLEEPH,
};

static const uint16_t ad5940_sin2osr_table[] = {
	22, 44, 89, 178, 267, 533, 640, 667, 800, 889, 1067, 1333,
};

static const uint8_t ad5940_sinc3osr_table[] = {5, 4, 2};

/**
 * @brief Write a register via SPI.
 *
 * AD5940 protocol requires CS to toggle between the SETADDR and WRITEREG
 * commands — two separate SPI transactions, each with its own CS assertion.
 *
 * Registers 0x1000–0x3014 are 32-bit wide (4 data bytes after WRITEREG).
 * Registers below 0x1000 are 16-bit wide (2 data bytes after WRITEREG).
 * Sending the wrong number of data bytes desyncs the chip's SPI state machine.
 *
 * @param dev Device pointer
 * @param addr Register address
 * @param val Value to write
 * @return 0 on success
 */
int ad5940_reg_write(const struct device *dev, uint16_t addr, uint32_t val)
{
	const struct ad5940_config *cfg = dev->config;
	bool is32 = (addr >= AD5940_REG32_ADDR_MIN) && (addr <= AD5940_REG32_ADDR_MAX);
	uint8_t addr_buf[AD5940_SPI_ADDR_FRAME_LEN] = {
		SPICMD_SETADDR,
		(uint8_t)(addr >> AD5940_BYTE_BITS),
		(uint8_t)(addr & AD5940_BYTE_MSK),
	};
	uint8_t wr_buf[AD5940_SPI_WR32_LEN];
	const struct spi_buf addr_tx = {.buf = addr_buf, .len = sizeof(addr_buf)};
	const struct spi_buf_set addr_tx_set = {.buffers = &addr_tx, .count = 1};
	struct spi_buf wr_tx = {.buf = wr_buf, .len = 0};
	const struct spi_buf_set wr_tx_set = {.buffers = &wr_tx, .count = 1};
	int ret;

#ifdef CONFIG_AD5940_SEQGEN
	struct ad5940_data *data = dev->data;

	if (data->seqgen.recording) {
		return ad5940_seqgen_write(dev, addr, val);
	}
#endif

	ret = spi_transceive_dt(&cfg->spi, &addr_tx_set, NULL);
	if (ret) {
		return ret;
	}

	wr_buf[0] = SPICMD_WRITEREG;
	if (is32) {
		wr_buf[1] = (uint8_t)((val >> 24u) & AD5940_BYTE_MSK);
		wr_buf[2] = (uint8_t)((val >> 16u) & AD5940_BYTE_MSK);
		wr_buf[3] = (uint8_t)((val >> 8u) & AD5940_BYTE_MSK);
		wr_buf[4] = (uint8_t)(val & AD5940_BYTE_MSK);
		wr_tx.len = AD5940_SPI_WR32_LEN;
	} else {
		wr_buf[1] = (uint8_t)((val >> 8u) & AD5940_BYTE_MSK);
		wr_buf[2] = (uint8_t)(val & AD5940_BYTE_MSK);
		wr_tx.len = AD5940_SPI_WR16_LEN;
	}

	return spi_transceive_dt(&cfg->spi, &wr_tx_set, NULL);
}

/**
 * @brief Read a register via SPI.
 *
 * Per the AD5940 datasheet (Rev. G, p.104), register access requires two
 * separate SPI transactions (CS toggled between them):
 *
 * Transaction 1 (set address):
 *   TX: [SETADDR][addr_hi][addr_lo]   CS↑
 *
 * Transaction 2 (read data):
 *   CS↓  TX: [READREG][dummy][0x00][0x00][0x00][0x00]
 *         RX: [x      ][x    ][d3  ][d2  ][d1  ][d0  ]
 *
 * Registers 0x1000–0x3014 are 32-bit wide; registers below 0x1000 are 16-bit.
 * The read transaction always uses 6 bytes to capture all 4 data bytes for
 * 32-bit registers in a single CS assertion, matching the no-OS reference driver.
 *
 * @param dev Device pointer
 * @param addr Register address
 * @param val Output register value
 * @return 0 on success
 */
int ad5940_reg_read(const struct device *dev, uint16_t addr, uint32_t *val)
{
	const struct ad5940_config *cfg = dev->config;
	bool is32 = (addr >= AD5940_REG32_ADDR_MIN) && (addr <= AD5940_REG32_ADDR_MAX);
	uint8_t addr_buf[AD5940_SPI_ADDR_FRAME_LEN] = {
		SPICMD_SETADDR,
		(uint8_t)(addr >> AD5940_BYTE_BITS),
		(uint8_t)(addr & AD5940_BYTE_MSK),
	};
	uint8_t tx_buf[AD5940_SPI_REG_RD_LEN] = {SPICMD_READREG};
	uint8_t rx_buf[AD5940_SPI_REG_RD_LEN] = {0};
	const struct spi_buf addr_tx = {.buf = addr_buf, .len = sizeof(addr_buf)};
	const struct spi_buf_set addr_tx_set = {.buffers = &addr_tx, .count = 1};
	const struct spi_buf rd_tx = {.buf = tx_buf, .len = sizeof(tx_buf)};
	const struct spi_buf rd_rx = {.buf = rx_buf, .len = sizeof(rx_buf)};
	const struct spi_buf_set rd_tx_set = {.buffers = &rd_tx, .count = 1};
	const struct spi_buf_set rd_rx_set = {.buffers = &rd_rx, .count = 1};
	int ret;

#ifdef CONFIG_AD5940_SEQGEN
	struct ad5940_data *data = dev->data;

	if (data->seqgen.recording) {
		return ad5940_seqgen_read(dev, addr, val);
	}
#endif

	ret = spi_transceive_dt(&cfg->spi, &addr_tx_set, NULL);
	if (ret) {
		return ret;
	}

	ret = spi_transceive_dt(&cfg->spi, &rd_tx_set, &rd_rx_set);
	if (ret) {
		return ret;
	}

	if (is32) {
		*val = ((uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET] << 24u) |
		       ((uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET + 1u] << 16u) |
		       ((uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET + 2u] << 8u) |
		       (uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET + 3u];
	} else {
		*val = ((uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET] << 8u) |
		       (uint32_t)rx_buf[AD5940_SPI_RD_DATA_OFFSET + 1u];
	}

	return 0;
}

/**
 * @brief Pop N 32-bit words from the AD5940 data FIFO using SPICMD_READFIFO.
 *
 * Protocol: [SPICMD_READFIFO][6 dummy bytes][N*4 data bytes with 0x44 on TX]
 * The last two words must have 0x44 on the TX line to signal end-of-burst.
 *
 * @param dev   Driver device pointer.
 * @param words Output buffer.
 * @param count Number of 32-bit words to read (must be >= 1).
 * @return 0 on success, negative errno on failure.
 */
int ad5940_fifo_read_words(const struct device *dev, uint32_t *words, uint16_t count)
{
	const struct ad5940_config *cfg = dev->config;
	uint8_t addr_buf[AD5940_SPI_ADDR_FRAME_LEN] = {
		SPICMD_SETADDR,
		(uint8_t)(AD5940_REG_DATAFIFORD >> AD5940_BYTE_BITS),
		(uint8_t)(AD5940_REG_DATAFIFORD & AD5940_BYTE_MSK),
	};
	uint8_t iobuf_tx[AD5940_FIFO_BURST_HDR_LEN +
			 AD5940_FIFO_BURST_MAX_WORDS * AD5940_FIFO_WORD_BYTES];
	uint8_t iobuf_rx[AD5940_FIFO_BURST_HDR_LEN +
			 AD5940_FIFO_BURST_MAX_WORDS * AD5940_FIFO_WORD_BYTES];
	const struct spi_buf addr_tx = {.buf = addr_buf, .len = sizeof(addr_buf)};
	const struct spi_buf_set addr_tx_set = {.buffers = &addr_tx, .count = 1};
	struct spi_buf tx_buf = {.buf = iobuf_tx, .len = 0};
	struct spi_buf rx_buf = {.buf = iobuf_rx, .len = 0};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	uint32_t io_sz;
	uint32_t off;
	uint16_t i;
	int ret;

	if (count == 0u) {
		return 0;
	}

	if (count > AD5940_FIFO_BURST_MAX_WORDS) {
		return -EINVAL;
	}

	ret = spi_transceive_dt(&cfg->spi, &addr_tx_set, NULL);
	if (ret) {
		return ret;
	}

	if (count < AD5940_FIFO_BURST_MIN_WORDS) {
		for (i = 0u; i < count; i++) {
			memset(iobuf_tx, 0, AD5940_SPI_REG_RD_LEN);
			memset(iobuf_rx, 0, AD5940_SPI_REG_RD_LEN);
			iobuf_tx[0] = SPICMD_READREG;
			tx_buf.len = AD5940_SPI_REG_RD_LEN;
			rx_buf.len = AD5940_SPI_REG_RD_LEN;

			ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
			if (ret) {
				return ret;
			}
			words[i] = ((uint32_t)iobuf_rx[AD5940_SPI_RD_DATA_OFFSET] << 24u) |
				   ((uint32_t)iobuf_rx[AD5940_SPI_RD_DATA_OFFSET + 1u] << 16u) |
				   ((uint32_t)iobuf_rx[AD5940_SPI_RD_DATA_OFFSET + 2u] << 8u) |
				   (uint32_t)iobuf_rx[AD5940_SPI_RD_DATA_OFFSET + 3u];
		}
		return 0;
	}

	io_sz = AD5940_FIFO_BURST_HDR_LEN + (uint32_t)count * AD5940_FIFO_WORD_BYTES;

	memset(iobuf_tx, 0, io_sz);
	iobuf_tx[0] = SPICMD_READFIFO;
	memset(&iobuf_tx[io_sz - AD5940_FIFO_BURST_END_LEN], AD5940_FIFO_BURST_END,
	       AD5940_FIFO_BURST_END_LEN);

	tx_buf.len = io_sz;
	rx_buf.len = io_sz;

	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret) {
		return ret;
	}

	for (i = 0u; i < count; i++) {
		off = AD5940_FIFO_BURST_HDR_LEN + (uint32_t)i * AD5940_FIFO_WORD_BYTES;

		words[i] = ((uint32_t)iobuf_rx[off] << 24u) |
			   ((uint32_t)iobuf_rx[off + 1u] << 16u) |
			   ((uint32_t)iobuf_rx[off + 2u] << 8u) |
			   (uint32_t)iobuf_rx[off + 3u];
	}

	return 0;
}

/**
 * @brief Send the SPI unlock word (0xDEADBEEF) after reset.
 *
 * @param dev Device pointer
 * @return 0 on success
 */
int ad5940_spi_unlock(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;
	uint8_t unlock_buf[AD5940_SPI_UNLOCK_LEN] = {
		(uint8_t)((AD5940_SPI_UNLOCK_KEY >> 24u) & AD5940_BYTE_MSK),
		(uint8_t)((AD5940_SPI_UNLOCK_KEY >> 16u) & AD5940_BYTE_MSK),
		(uint8_t)((AD5940_SPI_UNLOCK_KEY >> 8u) & AD5940_BYTE_MSK),
		(uint8_t)(AD5940_SPI_UNLOCK_KEY & AD5940_BYTE_MSK),
	};
	const struct spi_buf tx = {.buf = unlock_buf, .len = sizeof(unlock_buf)};
	const struct spi_buf_set tx_set = {.buffers = &tx, .count = 1};

	return spi_transceive_dt(&cfg->spi, &tx_set, NULL);
}

/**
 * @brief Wake the AD5940 from sleep mode.
 *
 * @param dev Device pointer
 * @return 0 on success
 */
int ad5940_wakeup(const struct device *dev)
{
	uint32_t adiid;
	int ret;
	int i;

	for (i = 0; i < AD5940_WAKEUP_RETRIES; i++) {
		ret = ad5940_reg_read(dev, AD5940_REG_ADIID, &adiid);
		if (ret == 0 && (adiid & AD5940_REG16_MSK) == AD5940_ADI_ID) {
			return 0;
		}
		k_busy_wait(AD5940_WAKEUP_DELAY_US);
	}

	LOG_ERR("AD5940 wakeup timed out after %d retries", AD5940_WAKEUP_RETRIES);
	return -ETIMEDOUT;
}

/**
 * @brief Put the AD5940 into sleep mode.
 *
 * @param dev Device pointer
 * @return 0 on success
 */
int ad5940_enter_sleep(const struct device *dev)
{
	uint32_t wuptmr;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_WUPTMRCON, &wuptmr);
	if (ret) {
		return ret;
	}
	wuptmr &= ~AD5940_TMRCON_WUPTEN_MSK;
	ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, wuptmr);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_PWRMOD, AD5940_SLEEP_KEY);
}

/**
 * @brief Configure an AD5940 AGPIO pin function, direction, pull, and output level.
 * @param dev       Device pointer
 * @param pin       Pin number (0–AD5940_GPIO_PIN_MAX)
 * @param func      Pin function selector (0–AD5940_GPIO_FUNC_MAX)
 * @param output_en Enable output driver
 * @param input_en  Enable input buffer
 * @param pull_en   Enable pull resistor
 * @param value     Output level when output_en is true
 * @return 0 on success, -EINVAL for out-of-range pin/func, negative errno on SPI error
 */
int ad5940_gpio_cfg(const struct device *dev, uint8_t pin, uint8_t func,
		    bool output_en, bool input_en, bool pull_en, bool value)
{
	uint32_t gp0con, gp0oen, gp0ien, gp0pe, gp0out;
	uint32_t shift;
	uint32_t pinbit;
	int ret;

	if (pin > AD5940_GPIO_PIN_MAX || func > AD5940_GPIO_FUNC_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_GP0CON, &gp0con);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_GP0OEN, &gp0oen);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_GP0IEN, &gp0ien);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_GP0PE, &gp0pe);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_GP0OUT, &gp0out);
	if (ret) {
		return ret;
	}

	shift = (uint32_t)pin * AD5940_GPIO_FUNC_BITS;
	gp0con = (gp0con & ~(AD5940_GPIO_FUNC_MSK << shift)) |
		 ((uint32_t)func << shift);

	pinbit = BIT(pin);

	if (output_en) {
		gp0oen |= pinbit;
	} else {
		gp0oen &= ~pinbit;
	}
	if (input_en) {
		gp0ien |= pinbit;
	} else {
		gp0ien &= ~pinbit;
	}
	if (pull_en) {
		gp0pe |= pinbit;
	} else {
		gp0pe &= ~pinbit;
	}
	if (value) {
		gp0out |= pinbit;
	} else {
		gp0out &= ~pinbit;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_GP0CON, gp0con);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_GP0OEN, gp0oen);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_GP0IEN, gp0ien);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_GP0PE, gp0pe);
	if (ret) {
		return ret;
	}
	return ad5940_reg_write(dev, AD5940_REG_GP0OUT, gp0out);
}

/**
 * @brief Set an AD5940 AGPIO output pin high or low.
 * @param dev   Device pointer
 * @param pin   Pin number (0–7)
 * @param value true = drive high, false = drive low
 * @return 0 on success, -EINVAL for pin > 7, negative errno on SPI error
 */
int ad5940_gpio_set(const struct device *dev, uint8_t pin, bool value)
{
	uint32_t gp0out;
	int ret;

	if (pin > 7u) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_GP0OUT, &gp0out);

	if (ret) {
		return ret;
	}

	if (value) {
		gp0out |= BIT(pin);
	} else {
		gp0out &= ~BIT(pin);
	}
	return ad5940_reg_write(dev, AD5940_REG_GP0OUT, gp0out);
}

/**
 * @brief Load an opcode block into the AD5940 sequencer SRAM slot.
 * @param dev Device pointer
 * @param id  Sequencer slot (AD5940_SEQ_0 .. AD5940_SEQ_3)
 * @param cfg Opcode blocks, lengths, timer ticks, and FIFO source
 * @return 0 on success, -EINVAL if SRAM is full or cfg is invalid, negative errno on SPI error
 */
int ad5940_seq_load(const struct device *dev, enum ad5940_seq_id id,
		    const struct ad5940_seq_cfg *cfg)
{
	struct ad5940_data *data = dev->data;
	const uint32_t *blocks[2];
	uint16_t lens[2];
	uint16_t total_words;
	uint32_t slot_base;
	uint32_t word_addr;
	uint32_t meas_start;
	uint16_t i;
	int b;
	int ret;

	if (cfg == NULL || cfg->meas_opcodes == NULL || cfg->meas_len == 0u) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	total_words = cfg->meas_len + (cfg->init_opcodes != NULL ? cfg->init_len : 0u);

	if (data->seq_sram_next + total_words > AD5940_SEQ_SRAM_WORDS) {
		LOG_ERR("seq_load: slot %d needs %u words but only %u remain",
			(int)id,
			(unsigned int)total_words,
			(unsigned int)(AD5940_SEQ_SRAM_WORDS - data->seq_sram_next));
		ret = -EINVAL;
		goto out;
	}

	slot_base = data->seq_sram_next;

	blocks[0] = cfg->init_opcodes;
	blocks[1] = cfg->meas_opcodes;
	lens[0]   = (cfg->init_opcodes != NULL) ? cfg->init_len : 0u;
	lens[1]   = cfg->meas_len;
	word_addr = slot_base;

	for (b = 0; b < 2; b++) {
		if (lens[b] == 0u || blocks[b] == NULL) {
			continue;
		}
		for (i = 0u; i < lens[b]; i++) {
			ret = ad5940_reg_write(dev, AD5940_REG_CMDFIFOWADDR, word_addr);
			if (ret) {
				goto out;
			}
			ret = ad5940_reg_write(dev, AD5940_REG_CMDFIFOWRITE, blocks[b][i]);
			if (ret) {
				goto out;
			}
			word_addr++;
		}
	}

	meas_start = slot_base + (cfg->init_opcodes ? cfg->init_len : 0u);

	ret = ad5940_reg_write(dev, seq_info_regs[id],
			       FIELD_PREP(AD5940_SEQINFO_INSTNUM_MSK,
					  (uint32_t)cfg->meas_len) |
			       FIELD_PREP(AD5940_SEQINFO_STARTADDR_MSK, meas_start));
	if (ret) {
		goto out;
	}

	data->seq_slot_addr[id]      = (uint16_t)meas_start;
	data->seq_slot_len[id]       = cfg->meas_len;
	data->seq_slot_init_addr[id] = (uint16_t)slot_base;
	data->seq_slot_init_len[id]  = (cfg->init_opcodes ? cfg->init_len : 0u);
	data->seq_slot_fifo[id]      = cfg->fifo_src;
	data->seq_slot_loaded[id]    = true;
	data->seq_sram_next          = slot_base + total_words;

	ret = ad5940_reg_write(dev, seq_wupl_regs[id],
			       cfg->wakeup_ticks & AD5940_REG16_MSK);
	if (ret) {
		goto out;
	}
	ret = ad5940_reg_write(dev, seq_wuph_regs[id],
			       (cfg->wakeup_ticks >> AD5940_TICK_HI_SHIFT) &
			       AD5940_TICK_HI_MSK);
	if (ret) {
		goto out;
	}
	ret = ad5940_reg_write(dev, seq_sleepl_regs[id],
			       cfg->sleep_ticks & AD5940_REG16_MSK);
	if (ret) {
		goto out;
	}
	ret = ad5940_reg_write(dev, seq_sleeph_regs[id],
			       (cfg->sleep_ticks >> AD5940_TICK_HI_SHIFT) &
			       AD5940_TICK_HI_MSK);
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * @brief Start the AD5940 wakeup timer to run a sequencer slot continuously.
 * @param dev Device pointer
 * @param id  Sequencer slot to run (must be loaded via ad5940_seq_load())
 * @return 0 on success, -ENOENT if the slot is not loaded, negative errno on SPI error
 */
int ad5940_seq_start(const struct device *dev, enum ad5940_seq_id id)
{
	struct ad5940_data *data = dev->data;
	uint32_t wuptmr;
	int ret;

	if (!data->seq_slot_loaded[id]) {
		return -ENOENT;
	}

	ret = ad5940_fifo_config(dev, data->seq_slot_fifo[id]);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQORDER, (uint32_t)id);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQCON,
			       AD5940_SEQCON_SEQEN_MSK |
			       AD5940_SEQCON_SEQHALTFIFOEMPTY_MSK);
	if (ret) {
		return ret;
	}

	wuptmr = AD5940_TMRCON_WUPTEN_MSK |
		 FIELD_PREP(AD5940_TMRCON_ENDSEQ_MSK, AD5940_TMRCON_ENDSEQ_A);

	ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, wuptmr);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON,
				AD5940_ALLON_TMRCON_TMRINTEN_MSK);
}

/**
 * @brief Stop the AD5940 wakeup timer and ALLON_TMRCON.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
int ad5940_seq_stop(const struct device *dev)
{
	int ret;

	ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, 0x0u);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON, 0x0u);
}

/**
 * @brief Trigger a single one-shot execution of a sequencer slot via TRIGSEQ.
 * @param dev Device pointer
 * @param id  Sequencer slot to trigger (must be loaded via ad5940_seq_load())
 * @return 0 on success, -ENOENT if the slot is not loaded, negative errno on SPI error
 */
int ad5940_seq_trigger(const struct device *dev, enum ad5940_seq_id id)
{
	struct ad5940_data *data = dev->data;
	int ret;

	if (!data->seq_slot_loaded[id]) {
		return -ENOENT;
	}

	ret = ad5940_fifo_config(dev, data->seq_slot_fifo[id]);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_TRIGSEQ,
				FIELD_PREP(AD5940_TRIGSEQ_SEQID_MSK, (uint32_t)id));
}

/**
 * @brief Run the init opcode block of a sequencer slot via SEQ1/TRIGSEQ.
 * @param dev Device pointer
 * @param id  Sequencer slot whose init block to run
 * @return 0 on success, -ENOENT if the slot is not loaded, negative errno on SPI error
 */
int ad5940_init_sequence(const struct device *dev, enum ad5940_seq_id id)
{
	struct ad5940_data *data = dev->data;
	int ret;

	if (!data->seq_slot_loaded[id]) {
		return -ENOENT;
	}

	if (data->seq_slot_init_len[id] == 0u) {
		return 0;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ1INFO,
			       FIELD_PREP(AD5940_SEQINFO_INSTNUM_MSK,
					  (uint32_t)data->seq_slot_init_len[id]) |
			       FIELD_PREP(AD5940_SEQINFO_STARTADDR_MSK,
					  (uint32_t)data->seq_slot_init_addr[id]));
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_TRIGSEQ,
				FIELD_PREP(AD5940_TRIGSEQ_SEQID_MSK, 1u));
}

/**
 * @brief Run the measurement opcode block of a sequencer slot via SEQ0/TRIGSEQ.
 * @param dev Device pointer
 * @param id  Sequencer slot whose meas block to run
 * @return 0 on success, -ENOENT if the slot is not loaded, negative errno on SPI error
 */
int ad5940_measure_sequence(const struct device *dev, enum ad5940_seq_id id)
{
	struct ad5940_data *data = dev->data;
	int ret;

	if (!data->seq_slot_loaded[id]) {
		return -ENOENT;
	}

	ret = ad5940_fifo_config(dev, data->seq_slot_fifo[id]);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0INFO,
			       FIELD_PREP(AD5940_SEQINFO_INSTNUM_MSK,
					  (uint32_t)data->seq_slot_len[id]) |
			       FIELD_PREP(AD5940_SEQINFO_STARTADDR_MSK,
					  (uint32_t)data->seq_slot_addr[id]));
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_TRIGSEQ,
				FIELD_PREP(AD5940_TRIGSEQ_SEQID_MSK, 0u));
}

#ifdef CONFIG_AD5940_STREAM
static void ad5940_stream_warmup(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	uint32_t discard[AD5940_EIS_WORDS_PER_FRAME];
	uint32_t intflag;
	uint32_t fifo_cnt;
	uint32_t words;
	uint32_t wuptmr;
	int i;
	int p;

	if (data->mode != AD5940_MODE_EIS ||
	    !data->seq_slot_loaded[AD5940_SEQ_0]) {
		return;
	}

	for (i = 0; i < (int)AD5940_STREAM_WARMUP_TICKS; i++) {
		if (ad5940_wakeup(dev) != 0) {
			return;
		}
		if (ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, 0x0u) != 0) {
			return;
		}
		if (ad5940_reg_write(dev, AD5940_REG_INTCCLR,
				     AFEINTSRC_ALLINT) != 0) {
			return;
		}

		if (ad5940_reg_write(dev, AD5940_REG_SEQCON,
				     AD5940_SEQCON_SEQEN_MSK) != 0) {
			return;
		}
		if (ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON,
				     AD5940_ALLON_TMRCON_TMRINTEN_MSK) != 0) {
			return;
		}
		wuptmr = AD5940_TMRCON_WUPTEN_MSK |
			 FIELD_PREP(AD5940_TMRCON_ENDSEQ_MSK,
				    AD5940_TMRCON_ENDSEQ_A);
		if (ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, wuptmr) != 0) {
			return;
		}

		for (p = 0; p < AD5940_FIFO_POLL_COUNT; p++) {
			if (ad5940_reg_read(dev, AD5940_REG_INTCFLAG0,
					    &intflag) != 0) {
				break;
			}
			if (intflag & AFEINTSRC_DATAFIFOTHRESH) {
				break;
			}
			k_busy_wait(AD5940_POLL_STEP_US);
		}

		ad5940_reg_write(dev, AD5940_REG_WUPTMRCON, 0x0u);
		ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON, 0x0u);

		if (ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt) == 0) {
			words = FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK,
					  fifo_cnt);
			words = MIN(words, (uint32_t)ARRAY_SIZE(discard));
			if (words > 0u) {
				(void)ad5940_fifo_read_words(dev, discard,
							     (uint16_t)words);
			}
		}
	}

	ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);
	(void)ad5940_fifo_flush(dev);
}

/**
 * @brief Prime the AD5940 streaming path by running warmup ticks.
 * @param dev Device pointer
 */
void ad5940_stream_prime(const struct device *dev)
{
	ad5940_stream_warmup(dev);
}
#endif /* CONFIG_AD5940_STREAM */

/**
 * @brief Assert the AD5940 hardware reset pin.
 * @param dev Device pointer
 * @return 0 on success, -ENOTSUP if no reset GPIO is configured, negative errno on GPIO error
 */
int ad5940_hw_reset(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;
	int ret;

	if (cfg->reset_gpio.port == NULL) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);

	if (ret) {
		return ret;
	}

	k_busy_wait(AD5940_RESET_PULSE_US);
	ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (ret) {
		return ret;
	}

	k_msleep(1);
	return 0;
}

/**
 * @brief Change the AD5940 FIFO data source at runtime.
 * @param dev Device pointer
 * @param src FIFO source selector (AD5940_FIFOSRC_ADC .. AD5940_FIFOSRC_MEAN)
 * @return 0 on success, -EINVAL for an out-of-range source, negative errno on SPI error
 */
int ad5940_fifo_source_set(const struct device *dev, uint8_t src)
{
	if (src < AD5940_FIFOSRC_ADC || src > AD5940_FIFOSRC_MEAN) {
		return -EINVAL;
	}
	return ad5940_fifo_config(dev, src);
}

/**
 * @brief Put the AD5940 into hibernate mode.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_enter_hibernate(const struct device *dev)
{
	int ret;

	ret = ad5940_enter_sleep(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_LPREFBUFCON, 0x00u);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_PWRMOD, AD5940_HIBERNATE_KEY);
}

/**
 * @brief Perform a software reset of the AD5940.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sw_reset(const struct device *dev)
{
	int ret;

	ret = ad5940_reg_write(dev, AD5940_REG_RSTCONKEY, AD5940_RSTCONKEY_UNLOCK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SWRSTCON, AD5940_SWRST_KEY);
	if (ret) {
		return ret;
	}

	k_busy_wait(AD5940_RESET_SETTLE_US);
	return 0;
}

/**
 * @brief Write the mandatory system initialization register sequence.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sys_init_sequence(const struct device *dev)
{
	static const struct {
		uint16_t addr;
		uint32_t val;
	} init_seq[] = {
		{AD5940_REG_SILICON_0908,   AD5940_INIT_SILICON_0908},
		{AD5940_REG_SILICON_0C08,   AD5940_INIT_SILICON_0C08},
		{AD5940_REG_REPEATADCCNV,   AD5940_INIT_REPEATADCCNV},
		{AD5940_REG_CLKEN1,         AD5940_INIT_CLKEN1},
		{AD5940_REG_EI2CON,         AD5940_INIT_EI2CON},
		{AD5940_REG_ADCBUFCON,      AD5940_INIT_ADCBUFCON},
		{AD5940_REG_PWRKEY,         AD5940_PWRKEY1},
		{AD5940_REG_PWRKEY,         AD5940_PWRKEY2},
		{AD5940_REG_PWRMOD,         AD5940_INIT_PWRMOD},
		{AD5940_REG_PMBW,           AD5940_INIT_PMBW},
	};

	int ret;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(init_seq); i++) {
		ret = ad5940_reg_write(dev, init_seq[i].addr, init_seq[i].val);
		if (ret) {
			LOG_ERR("Init seq step %zu failed: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

/**
 * @brief Configure the AD5940 clock system.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_clock_config(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;
	uint32_t clkcon0;
	uint32_t clksel;
	uint32_t osccon;
	int ret;
	int i;

	ret = ad5940_reg_read(dev, AD5940_REG_OSCCON, &osccon);
	if (ret) {
		return ret;
	}
	osccon |= AD5940_OSCCON_LFOSCEN_MSK | AD5940_OSCCON_HFOSCEN_MSK;
	ret = ad5940_reg_write(dev, AD5940_REG_OSCKEY, AD5940_OSCKEY_UNLOCK);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_OSCCON, osccon);
	if (ret) {
		return ret;
	}

	for (i = 0; i < AD5940_OSC_POLL_COUNT; i++) {
		ret = ad5940_reg_read(dev, AD5940_REG_OSCCON, &osccon);
		if (ret) {
			return ret;
		}
		if (osccon & AD5940_OSCCON_LFOSCOK_MSK) {
			break;
		}
		k_busy_wait(AD5940_POLL_STEP_US);
	}
	if (!(osccon & AD5940_OSCCON_LFOSCOK_MSK)) {
		LOG_WRN("AD5940: LFOSC did not stabilise");
	}

	ret = ad5940_reg_read(dev, AD5940_REG_CLKSEL, &clksel);
	if (ret) {
		return ret;
	}
	clksel &= ~AD5940_CLKSEL_SYSCLKSEL_MSK;
	clksel |= FIELD_PREP(AD5940_CLKSEL_SYSCLKSEL_MSK, cfg->clock_source);
	ret = ad5940_reg_write(dev, AD5940_REG_CLKSEL, clksel);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_CLKCON0, &clkcon0);
	if (ret) {
		return ret;
	}
	clkcon0 &= ~AD5940_CLKCON0_SYSCLKDIV_MSK;
	if (cfg->hfosc_32mhz) {
		clkcon0 |= FIELD_PREP(AD5940_CLKCON0_SYSCLKDIV_MSK, 2u);
	} else {
		clkcon0 |= FIELD_PREP(AD5940_CLKCON0_SYSCLKDIV_MSK, 1u);
	}
	return ad5940_reg_write(dev, AD5940_REG_CLKCON0, clkcon0);
}

/**
 * @brief Flush the AD5940 data FIFO by toggling DATAFIFOEN.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
int ad5940_fifo_flush(const struct device *dev)
{
	uint32_t fifocon = 0u;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_FIFOCON, &fifocon);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_FIFOCON,
			       fifocon & ~AD5940_FIFOCON_DATAFIFOEN_MSK);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_FIFOCON, fifocon);
}

/**
 * @brief Configure the AD5940 data FIFO source, size, mode, and threshold.
 * @param dev Device pointer
 * @param src FIFO data source (AD5940_FIFOSRC_DFT, SINC2, ADC, etc.)
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_fifo_config(const struct device *dev, uint8_t src)
{
	struct ad5940_data *data = dev->data;
	uint32_t fifocon;
	int ret;

	fifocon = ((uint32_t)AD5940_FIFOMODE_FIFO << AD5940_AFE_FIFOCON_DATAFIFOMODE_LSB) |
		  ((uint32_t)AD5940_FIFOSIZE_4KB << AD5940_AFE_FIFOCON_DATAFIFOSIZE_LSB)  |
		  ((uint32_t)src << AD5940_AFE_FIFOCON_DATAFIFOSRCSEL_LSB)                |
		  AD5940_FIFOCON_DATAFIFOEN_MSK;

	ret = ad5940_reg_write(dev, AD5940_REG_FIFOCON, fifocon);
	if (ret) {
		return ret;
	}
	data->fifo_src = src;

	return ad5940_reg_write(dev, AD5940_REG_DATAFIFOTHRES,
				FIELD_PREP(AD5940_DATAFIFOTHRES_HIGHTHRES_MSK,
					   AD5940_EIS_WORDS_PER_FRAME));
}

/**
 * @brief Compute the waveform generator frequency control word.
 *
 * FCW = freq_hz * 2^30 / SYS_CLK_HZ
 *
 * @param freq_hz Desired excitation frequency in Hz.
 * @return 30-bit FCW value.
 */
static uint32_t ad5940_freq_to_fcw(float freq_hz)
{
	return (uint32_t)((freq_hz * (float)(1u << AD5940_FCW_SHIFT)) /
			  (float)AD5940_SYS_CLK_HZ + AD5940_ROUND_F);
}

static float ad5940_fcw_to_freq(uint32_t fcw)
{
	return ((float)(fcw & AD5940_WGFCW_SINEFCW_MSK) *
		(float)AD5940_SYS_CLK_HZ) / (float)(1u << AD5940_FCW_SHIFT);
}

/**
 * @brief Convert a raw ADC code to volts.
 * @param code      Raw 16-bit ADC code (0..65535).
 * @param pga_gain  AD5940_ADCPGA_* selector configured on the ADC.
 * @param volts     Output voltage.
 * @return 0 on success, -EINVAL on an unknown PGA selector.
 */
static int ad5940_adc_code_to_volt(uint32_t code, uint8_t pga_gain, float *volts)
{
	float tmp = (float)((int32_t)code - (int32_t)AD5940_ADC_MIDSCALE_CODE);
	float gain;

	switch (pga_gain) {
	case AD5940_ADCPGA_1:
		gain = AD5940_ADCPGA_GAIN_1;
		break;
	case AD5940_ADCPGA_1P5:
		gain = AD5940_ADCPGA_GAIN_1P5;
		break;
	case AD5940_ADCPGA_2:
		gain = AD5940_ADCPGA_GAIN_2;
		break;
	case AD5940_ADCPGA_4:
		gain = AD5940_ADCPGA_GAIN_4;
		break;
	case AD5940_ADCPGA_9:
		gain = AD5940_ADCPGA_GAIN_9;
		break;
	default:
		return -EINVAL;
	}

	*volts = (tmp / gain) * AD5940_ADC_VREF_V / AD5940_ADC_HALF_SCALE;
	return 0;
}

/**
 * @brief Compute the number of system clocks a measurement takes.
 * @param info    Data-flow description (see struct ad5940_clks_cal_info).
 * @param clocks  Output clock count.
 * @return 0 on success, -EINVAL on an out-of-range OSR/avg selector.
 */
static int ad5940_clks_calculate(const struct ad5940_clks_cal_info *info,
				 uint32_t *clocks)
{
	struct ad5940_clks_cal_info tmp = *info;
	float ratio = info->ratio_sys2adc;

	if (info->sinc2_osr >= ARRAY_SIZE(ad5940_sin2osr_table) ||
	    info->sinc3_osr >= ARRAY_SIZE(ad5940_sinc3osr_table) ||
	    info->avg_num > AD5940_AVGNUM_MAX) {
		return -EINVAL;
	}

	switch (info->data_type) {
	case AD5940_DATATYPE_ADCRAW:
		*clocks = (uint32_t)(info->data_count * ratio + AD5940_ROUND_F);
		break;
	case AD5940_DATATYPE_SINC3:
		*clocks = (uint32_t)(((info->data_count + 2u) *
				      ad5940_sinc3osr_table[info->sinc3_osr] + 1u) *
				     ratio + AD5940_ROUND_F);
		break;
	case AD5940_DATATYPE_SINC2: {
		tmp.data_count = (info->data_count + 1u) *
					 ad5940_sin2osr_table[info->sinc2_osr] + 1u;
		tmp.data_type = AD5940_DATATYPE_SINC3;
		return ad5940_clks_calculate(&tmp, clocks);
	}
	case AD5940_DATATYPE_DFT:
		switch (info->dft_src) {
		case AD5940_DFTSRC_ADCRAW:
			tmp.data_type = AD5940_DATATYPE_ADCRAW;
			break;
		case AD5940_DFTSRC_SINC3:
			tmp.data_type = AD5940_DATATYPE_SINC3;
			break;
		case AD5940_DFTSRC_SINC2NOTCH:
			tmp.data_type = AD5940_DATATYPE_SINC2;
			break;
		case AD5940_DFTSRC_AVG:
			tmp.data_count = info->data_count << (info->avg_num + 1u);
			tmp.data_type = AD5940_DATATYPE_SINC3;
			break;
		default:
			return -EINVAL;
		}
		if (ad5940_clks_calculate(&tmp, clocks) != 0) {
			return -EINVAL;
		}
		*clocks += AD5940_DFT_OVERHEAD_CLKS;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ad5940_afe_pwr_bw_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;

	return ad5940_reg_write(dev, AD5940_REG_PMBW,
				FIELD_PREP(AD5940_PMBW_SYSHS_MSK,
					   data->eis.power_mode != 0u ? 1u : 0u) |
				FIELD_PREP(AD5940_PMBW_SYSBW_MSK,
					   AD5940_PMBW_SYSBW_250KHZ));
}

static int ad5940_afe_ref_cfg(const struct device *dev)
{
	uint32_t bufsencon = 0u;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_BUFSENCON, &bufsencon);
	if (ret) {
		return ret;
	}
	bufsencon |= AD5940_BUFSENCON_V1P8HPADCEN_MSK |
		     AD5940_BUFSENCON_V1P1HPADCEN_MSK;
	return ad5940_reg_write(dev, AD5940_REG_BUFSENCON, bufsencon);
}

static int ad5940_afe_wg_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;
	uint32_t fcw;
	uint32_t amplitude_word;
	int ret;

	ret = ad5940_reg_write(dev, AD5940_REG_WGCON,
			       FIELD_PREP(AD5940_WGCON_TYPESEL_MSK, AD5940_WGTYPE_SINE));
	if (ret) {
		return ret;
	}

	fcw = ad5940_freq_to_fcw(data->eis.freq_hz);
	ret = ad5940_reg_write(dev, AD5940_REG_WGFCW, fcw);
	if (ret) {
		return ret;
	}

	amplitude_word = (data->eis.amplitude_mvpp * AD5940_WG_AMP_FULL_SCALE) /
			 AD5940_WG_FULL_SCALE_MVPP;
	ret = ad5940_reg_write(dev, AD5940_REG_WGAMPLITUDE, amplitude_word);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_WGOFFSET, 0x0u);
}

static int ad5940_afe_hstia_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;
	uint32_t rtiacon = FIELD_PREP(AD5940_HSRTIACON_RTIACON_MSK,
				      (uint32_t)data->eis.rtia_sel & AD5940_RTIA_SEL_MSK) |
			   FIELD_PREP(AD5940_HSRTIACON_CTIACON_MSK,
				      (uint32_t)data->eis.ctia_sel & AD5940_CTIA_SEL_MSK);

	return ad5940_reg_write(dev, AD5940_REG_HSRTIACON, rtiacon);
}

static int ad5940_afe_hsdac_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;
	uint32_t hsdaccon = FIELD_PREP(AD5940_HSDACCON_Rate_MSK,
				       (uint32_t)data->eis.hsdac_update_rate);

	if (data->eis.excit_buf_gain != 0u) {
		hsdaccon |= AD5940_HSDACCON_ATTENEN_MSK;
	}
	if (data->eis.hsdac_gain != 0u) {
		hsdaccon |= AD5940_HSDACCON_GAINX5_MSK;
	}
	return ad5940_reg_write(dev, AD5940_REG_HSDACCON, hsdaccon);
}

static int ad5940_afe_con_eis_idle_cfg(const struct device *dev)
{
	return ad5940_reg_write(dev, AD5940_REG_AFECON,
				AD5940_AFECON_DACREFEN_MSK  |
				AD5940_AFECON_SINC2EN_MSK   |
				AD5940_AFECON_DFTEN_MSK     |
				AD5940_AFECON_WAVEGENEN_MSK |
				AD5940_AFECON_TIAEN_MSK     |
				AD5940_AFECON_INAMPEN_MSK   |
				AD5940_AFECON_EXBUFEN_MSK   |
				AD5940_AFECON_DACEN_MSK);
}

static int ad5940_afe_adc_mux_current_cfg(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;

	return ad5940_reg_write(dev, AD5940_REG_ADCCON,
				AD5940_ADCMUX_WORD(cfg->adc_mux_i_p, cfg->adc_mux_i_n));
}

static int ad5940_afe_dft_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;

	return ad5940_reg_write(dev, AD5940_REG_DFTCON,
				FIELD_PREP(AD5940_DFTCON_DFTINSEL_MSK,
					   (uint32_t)data->eis.dft_src) |
				FIELD_PREP(AD5940_DFTCON_DFTNUM_MSK,
					   (uint32_t)data->eis.dft_num) |
				(data->eis.hanning_win ? AD5940_DFTCON_HANNINGEN_MSK : 0u));
}

static int ad5940_afe_adc_filter_cfg(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;

	return ad5940_reg_write(dev, AD5940_REG_ADCFILTERCON,
				FIELD_PREP(AD5940_ADCFILTERCON_SINC3OSR_MSK,
					   (uint32_t)data->eis.sinc3_osr) |
				FIELD_PREP(AD5940_ADCFILTERCON_SINC2OSR_MSK, 0u) |
				AD5940_ADCFILTERCON_LPFBYPEN_MSK |
				AD5940_ADCFILTERCON_ADCSAMPLERATE_MSK);
}

static int ad5940_afe_switch_matrix_cfg(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;
	int ret;

	ret = ad5940_reg_write(dev, AD5940_REG_DSWFULLCON, cfg->sw_dsw);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_NSWFULLCON, cfg->sw_nsw);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_PSWFULLCON, cfg->sw_psw);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_TSWFULLCON, cfg->sw_tsw);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SWCON, AD5940_SWCON_SWSOURCESEL_MSK);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_SWMUX, cfg->sw_swmux);
}

static int ad5940_afe_stats_cfg(const struct device *dev,
				uint32_t sample_num, bool enable)
{
	uint32_t statscon = FIELD_PREP(AD5940_STATSCON_SAMPLENUM_MSK, sample_num);

	if (enable) {
		statscon |= AD5940_STATSCON_STATSEN_MSK;
	}
	return ad5940_reg_write(dev, AD5940_REG_STATSCON, statscon);
}

static int ad5940_afe_int0_eis_cfg(const struct device *dev)
{
	uint32_t intcsel0 = 0u;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_INTCSEL0, &intcsel0);
	if (ret) {
		return ret;
	}
	return ad5940_reg_write(dev, AD5940_REG_INTCSEL0,
				intcsel0 | AFEINTSRC_DFTRDY | AFEINTSRC_DATAFIFOTHRESH);
}

static int ad5940_eis_mode_config(const struct device *dev)
{
	int ret;

	ret = ad5940_afe_pwr_bw_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_ref_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_wg_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_hstia_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_fifo_config(dev, AD5940_FIFOSRC_DFT);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_hsdac_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_con_eis_idle_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_adc_mux_current_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_dft_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_adc_filter_cfg(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_afe_switch_matrix_cfg(dev);
	if (ret) {
		return ret;
	}

	return ad5940_afe_int0_eis_cfg(dev);
}

static int ad5940_amper_mode_config(const struct device *dev)
{
	const struct ad5940_data *data = dev->data;
	uint32_t dac_code;
	int ret;

	ret = ad5940_reg_write(dev, AD5940_REG_PMBW, 0x0u);
	if (ret) {
		return ret;
	}

	if (data->amper.bias_mv < AD5940_LPDAC_MIN_MV ||
	    data->amper.bias_mv > AD5940_LPDAC_MAX_MV) {
		return -EINVAL;
	}
	dac_code = ((data->amper.bias_mv - AD5940_LPDAC_MIN_MV) *
		    AD5940_LPDAC_FULL_SCALE) / AD5940_LPDAC_SPAN_MV;
	ret = ad5940_reg_write(dev, AD5940_REG_LPDACDAT0, dac_code);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_LPDACCON0, 0x1u);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_LPDACSW0,
			       AD5940_LPDACSW0_LPMODEDIS_MSK |
			       AD5940_LPDACSW0_SW4_MSK |
			       AD5940_LPDACSW0_SW3_MSK |
			       AD5940_LPDACSW0_SW2_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_LPTIACON0,
			       FIELD_PREP(AD5940_LPTIACON0_TIAGAIN_MSK,
					  (uint32_t)data->amper.lpamp_rtia));
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_ADCFILTERCON,
			       FIELD_PREP(AD5940_ADCFILTERCON_SINC2OSR_MSK,
					  (uint32_t)data->amper.sinc2_osr) |
			       AD5940_ADCFILTERCON_ADCSAMPLERATE_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_LPTIASW0, AD5940_LPTIASW0_AMPER_NORMAL);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_ADCEN_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_ADCCON,
			       FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK,
					  AD5940_ADCCON_MUXSELP_LPTIA_P) |
			       FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK,
					  AD5940_ADCCON_MUXSELN_VZERO0));
	if (ret) {
		return ret;
	}

	return ad5940_fifo_config(dev, AD5940_FIFOSRC_SINC2);
}

#ifdef CONFIG_AD5940_SEQGEN
static void ad5940_seqgen_init(const struct device *dev)
{
	struct ad5940_data *data = dev->data;

	data->seqgen.buf        = data->seqgen_buf;
	data->seqgen.buf_sz     = (uint16_t)ARRAY_SIZE(data->seqgen_buf);
	data->seqgen.seq_len    = 0u;
	data->seqgen.reg_count  = 0u;
	data->seqgen.last_error = 0;
	data->seqgen.recording  = false;
}

static void ad5940_seqgen_ctrl(const struct device *dev, bool on)
{
	struct ad5940_data *data = dev->data;

	if (on) {
		data->seqgen.seq_len    = 0u;
		data->seqgen.last_error = 0;
	}
	data->seqgen.recording = on;
}

static void ad5940_seqgen_insert(const struct device *dev, uint32_t opcode)
{
	struct ad5940_data *data = dev->data;
	struct ad5940_seqgen *sg = &data->seqgen;

	if ((uint32_t)sg->seq_len + (uint32_t)sg->reg_count >= sg->buf_sz) {
		sg->last_error = -ENOSPC;
		return;
	}
	sg->buf[sg->seq_len] = opcode;
	sg->seq_len++;
}

static struct ad5940_seq_reg_info *ad5940_seqgen_search_reg(const struct device *dev,
							   uint16_t addr)
{
	struct ad5940_data *data = dev->data;
	struct ad5940_seqgen *sg = &data->seqgen;
	struct ad5940_seq_reg_info *table =
		(struct ad5940_seq_reg_info *)&sg->buf[sg->buf_sz];
	uint16_t i;

	for (i = 1u; i <= sg->reg_count; i++) {
		if (table[-(int)i].reg_addr == addr) {
			return &table[-(int)i];
		}
	}
	return NULL;
}

static int ad5940_seqgen_write(const struct device *dev, uint16_t addr, uint32_t val)
{
	struct ad5940_data *data = dev->data;
	struct ad5940_seqgen *sg = &data->seqgen;
	struct ad5940_seq_reg_info *info = ad5940_seqgen_search_reg(dev, addr);
	struct ad5940_seq_reg_info *table;
	uint16_t words_per_entry;

	if (info == NULL) {
		words_per_entry = (uint16_t)((sizeof(struct ad5940_seq_reg_info) + 3u) / 4u);

		if ((uint32_t)sg->seq_len +
		    (uint32_t)(sg->reg_count + 1u) * words_per_entry >= sg->buf_sz) {
			sg->last_error = -ENOSPC;
			return 0;
		}

		table = (struct ad5940_seq_reg_info *)&sg->buf[sg->buf_sz];

		sg->reg_count++;
		info = &table[-(int)sg->reg_count];
		info->reg_addr = addr;
	}
	info->reg_data = val;

	ad5940_seqgen_insert(dev, AD5940_SEQ_WR_REG(addr, val));
	return 0;
}

static int ad5940_seqgen_read(const struct device *dev, uint16_t addr, uint32_t *val)
{
	struct ad5940_seq_reg_info *info = ad5940_seqgen_search_reg(dev, addr);

	*val = (info != NULL) ? info->reg_data : 0u;
	return 0;
}

static int ad5940_eis_seq_build(const struct device *dev, struct ad5940_seq_cfg *out)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	struct ad5940_clks_cal_info clk_info;
	uint32_t dft_wait;
	uint16_t i;
	int ret;

	clk_info.data_type = AD5940_DATATYPE_DFT;
	clk_info.dft_src = data->eis.dft_src;
	clk_info.data_count = AD5940_DFT_NUM_TO_POINTS(data->eis.dft_num);
	clk_info.sinc2_osr = AD5940_SINC2OSR_22;
	clk_info.sinc3_osr = data->eis.sinc3_osr;
	clk_info.avg_num = 0u;
	clk_info.ratio_sys2adc = AD5940_RATIO_SYS2ADC;

	ret = ad5940_clks_calculate(&clk_info, &dft_wait);
	if (ret) {
		return ret;
	}

	ad5940_seqgen_init(dev);
	ad5940_seqgen_ctrl(dev, true);

	ad5940_reg_write(dev, AD5940_REG_DSWFULLCON, cfg->sw_dsw);
	ad5940_reg_write(dev, AD5940_REG_PSWFULLCON, cfg->sw_psw);
	ad5940_reg_write(dev, AD5940_REG_NSWFULLCON, cfg->sw_nsw);
	ad5940_reg_write(dev, AD5940_REG_TSWFULLCON, cfg->sw_tsw);
	ad5940_reg_write(dev, AD5940_REG_SWCON, AD5940_SWCON_SWSOURCESEL_MSK);

	ad5940_reg_write(dev, AD5940_REG_ADCCON,
			 AD5940_ADCMUX_WORD(cfg->adc_mux_v_p, cfg->adc_mux_v_n));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_ANALOG);
	ad5940_seqgen_insert(dev, AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_CONV);
	ad5940_seqgen_insert(dev, AD5940_SEQ_WAIT_CLKS(dft_wait));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_OFF);

	ad5940_reg_write(dev, AD5940_REG_ADCCON,
			 AD5940_ADCMUX_WORD(cfg->adc_mux_i_p, cfg->adc_mux_i_n));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_ANALOG);
	ad5940_seqgen_insert(dev, AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_CONV);
	ad5940_seqgen_insert(dev, AD5940_SEQ_WAIT_CLKS(dft_wait));
	ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_OFF);

	ad5940_seqgen_insert(dev, AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS));
	ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);
	ad5940_reg_write(dev, AD5940_REG_SEQTRGSLP, AD5940_SEQTRGSLP_EN);

	ad5940_seqgen_ctrl(dev, false);

	if (data->seqgen.last_error != 0) {
		LOG_ERR("SEQGen overflow building EIS sequence (buf=%u words)",
			data->seqgen.buf_sz);
		return data->seqgen.last_error;
	}

	out->init_opcodes = NULL;
	out->init_len     = 0u;
	out->meas_opcodes = data->seqgen.buf;
	out->meas_len     = data->seqgen.seq_len;
	out->wakeup_ticks = 1u;
	out->sleep_ticks  = AD5940_SEQ_SLEEP_TICKS;
	out->fifo_src     = AD5940_FIFOSRC_DFT;

	return 0;
}
#endif /* CONFIG_AD5940_SEQGEN */

static const uint32_t ad5940_eis_meas_seq[] = {
	/* Switch-matrix routing + sense mux come from DT (swmat-* + adccon-muxsel*),
	 * resolved at compile time for the single AD5940 instance.
	 */
	AD5940_SEQ_WR_REG(AD5940_REG_DSWFULLCON, DT_INST_PROP(0, swmat_dsw)),
	AD5940_SEQ_WR_REG(AD5940_REG_PSWFULLCON, DT_INST_PROP(0, swmat_psw)),
	AD5940_SEQ_WR_REG(AD5940_REG_NSWFULLCON, DT_INST_PROP(0, swmat_nsw)),
	AD5940_SEQ_WR_REG(AD5940_REG_TSWFULLCON, DT_INST_PROP(0, swmat_tsw)),
	/* SWSOURCESEL must be re-asserted every tick: SWCON resets to MUX mode
	 * across hibernate, which would ignore the FULLCON writes.
	 */
	AD5940_SEQ_WR_REG(AD5940_REG_SWCON, AD5940_SWCON_SWSOURCESEL_MSK),

	/* Capture 1: voltage channel (Kelvin sense, DT adc-mux-v-*) */
	AD5940_SEQ_WR_REG(AD5940_REG_ADCCON, AD5940_DT_SEQ_ADCMUX_VOLT),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_ANALOG),
	AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_CONV),
	AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_DFT_WAIT_CLKS),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_OFF),

	/* Capture 2: current channel (across HSTIA, DT adc-mux-i-*) */
	AD5940_SEQ_WR_REG(AD5940_REG_ADCCON, AD5940_DT_SEQ_ADCMUX_RTIA),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_ANALOG),
	AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_CONV),
	AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_DFT_WAIT_CLKS),
	AD5940_SEQ_WR_REG(AD5940_REG_AFECON, AD5940_AFECON_OFF),

	/* Let the DFT engine flush both words of pair 2 into the FIFO before
	 * hibernating: the AFE clock stops on sleep and would drop them.
	 */
	AD5940_SEQ_WAIT_CLKS(AD5940_SEQ_SETTLE_CLKS),

	/* Re-hibernate: AFE sleeps until the next WUPT tick. Sequencer stays
	 * enabled — SEQTRGSLP triggers the sleep, SEQCON is left untouched.
	 */
	AD5940_SEQ_WR_REG(AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY),
	AD5940_SEQ_WR_REG(AD5940_REG_SEQTRGSLP, AD5940_SEQTRGSLP_EN),
};

static int ad5940_sequencer_config(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	struct ad5940_clks_cal_info clk_info;
	const uint32_t *seq_ptr;
	uint32_t seq_words;
	uint32_t dft_clks = 0u;
	size_t i;
	int ret;

	clk_info.data_type = AD5940_DATATYPE_DFT;
	clk_info.dft_src = data->eis.dft_src;
	clk_info.data_count = AD5940_DFT_NUM_TO_POINTS(data->eis.dft_num);
	clk_info.sinc2_osr = AD5940_SINC2OSR_22;
	clk_info.sinc3_osr = data->eis.sinc3_osr;
	clk_info.avg_num = 0u;
	clk_info.ratio_sys2adc = AD5940_RATIO_SYS2ADC;

	if (ad5940_clks_calculate(&clk_info, &dft_clks) == 0) {
		LOG_DBG("ClksCalculate: DFT wait=%u clks (static=%u)",
			dft_clks, AD5940_SEQ_DFT_WAIT_CLKS);
	} else {
		LOG_WRN("ClksCalculate failed for EIS DFT config");
	}

	/* Disable sequencer before programming */
	ret = ad5940_reg_write(dev, AD5940_REG_SEQCON, 0x0u);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_FIFOCON, 0x0u);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_CMDDATACON, AD5940_CMDDATACON_DEFAULT);
	if (ret) {
		return ret;
	}

	/* Measurement sequence starts at word 0 */
	data->seq_meas_addr = 0u;

	seq_ptr = ad5940_eis_meas_seq;
	seq_words = (uint32_t)ARRAY_SIZE(ad5940_eis_meas_seq);

#ifdef CONFIG_AD5940_SEQGEN
	struct ad5940_seq_cfg gen_cfg = {0};

	ret = ad5940_eis_seq_build(dev, &gen_cfg);
	if (ret) {
		return ret;
	}
	seq_ptr   = gen_cfg.meas_opcodes;
	seq_words = gen_cfg.meas_len;
	LOG_DBG("sequencer: using SEQGen-generated stream (%u words, static=%u)",
		(unsigned int)seq_words, (unsigned int)ARRAY_SIZE(ad5940_eis_meas_seq));
#endif

	/* Write each opcode with an explicit address update per word — no auto-increment */
	for (i = 0; i < seq_words; i++) {
		ret = ad5940_reg_write(dev, AD5940_REG_CMDFIFOWADDR,
				       data->seq_meas_addr + (uint32_t)i);
		if (ret) {
			return ret;
		}
		ret = ad5940_reg_write(dev, AD5940_REG_CMDFIFOWRITE, seq_ptr[i]);
		if (ret) {
			return ret;
		}
	}

	/* Advance the free-SRAM pointer past the EIS measurement sequence so that
	 * ad5940_seq_load() calls do not overwrite the built-in SEQ0 content.
	 */
	data->seq_sram_next = data->seq_meas_addr + seq_words;
	data->seq_slot_addr[AD5940_SEQ_0]   = (uint16_t)data->seq_meas_addr;
	data->seq_slot_len[AD5940_SEQ_0]    = (uint16_t)seq_words;
	data->seq_slot_fifo[AD5940_SEQ_0]   = AD5940_FIFOSRC_DFT;
	data->seq_slot_loaded[AD5940_SEQ_0] = true;

	ret = ad5940_reg_write(dev, AD5940_REG_FIFOCON,
			       AD5940_FIFOCON_DATAFIFOEN_MSK |
			       FIELD_PREP(AD5940_FIFOCON_DATAFIFOSRCSEL_MSK,
					  AD5940_FIFOSRC_DFT) |
			       (AD5940_FIFOSIZE_4KB << AD5940_AFE_FIFOCON_DATAFIFOSIZE_LSB));
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0INFO,
			       FIELD_PREP(AD5940_SEQINFO_INSTNUM_MSK, seq_words) |
			       FIELD_PREP(AD5940_SEQINFO_STARTADDR_MSK,
					  data->seq_meas_addr));
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQORDER, 0x0u); /* SEQ0 at A */
	if (ret) {
		return ret;
	}

	/* Wakeup period: low 16 bits */
	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0WUPL,
			       AD5940_SEQ_WAKEUP_COUNTS & AD5940_REG16_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0WUPH,
			       (AD5940_SEQ_WAKEUP_COUNTS >> AD5940_TICK_HI_SHIFT) &
			       AD5940_TICK_HI_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0SLEEPL,
			       AD5940_SEQ_SLEEP_COUNTS & AD5940_REG16_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQ0SLEEPH,
			       (AD5940_SEQ_SLEEP_COUNTS >> AD5940_TICK_HI_SHIFT) &
			       AD5940_TICK_HI_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_PWRMOD, AD5940_INIT_PWRMOD);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SEQSLPLOCK, AD5940_SEQSLPLOCK_KEY);
	if (ret) {
		return ret;
	}

	return ad5940_reg_write(dev, AD5940_REG_ALLON_TMRCON, BIT(0));
}

static int32_t ad5940_dft_sign_extend(uint32_t raw)
{
	int32_t v = (int32_t)(raw & AD5940_DFTREAL_DATA_MSK);

	if (v & BIT(AD5940_DFT_DATA_BITS - 1u)) {
		v |= (int32_t)~AD5940_DFTREAL_DATA_MSK;
	}
	return v;
}

static int ad5940_cal_capture_dft(const struct device *dev, uint32_t adc_mux,
				  int32_t *out_real, int32_t *out_imag)
{
	uint32_t dft_real_raw, dft_imag_raw;
	uint32_t intflag;
	int idle_ret;
	int ret;
	int i;

	ret = ad5940_reg_write(dev, AD5940_REG_ADCCON, adc_mux);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_DFTRDY);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_CAL_SETTLE);
	if (ret) {
		return ret;
	}
	k_busy_wait(AD5940_ANALOG_SETTLE_US);

	ret = ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_CAL_RUN);
	if (ret) {
		return ret;
	}

	for (i = 0; i < AD5940_DFTRDY_POLL_COUNT; i++) {
		ret = ad5940_reg_read(dev, AD5940_REG_INTCFLAG0, &intflag);
		if (ret) {
			break;
		}
		if (intflag & AFEINTSRC_DFTRDY) {
			break;
		}
		k_busy_wait(AD5940_POLL_STEP_US);
	}

	idle_ret = ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_CAL_IDLE);
	k_busy_wait(AD5940_PIPELINE_DRAIN_US);

	if (ret) {
		return ret;
	}
	if (idle_ret) {
		return idle_ret;
	}

	if (i >= AD5940_DFTRDY_POLL_COUNT) {
		LOG_WRN("AD5940: DFTRDY timeout mux=0x%06X", adc_mux);
		return -ETIMEDOUT;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_DFTREAL, &dft_real_raw);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_DFTIMAG, &dft_imag_raw);
	if (ret) {
		return ret;
	}


	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_DFTRDY);
	if (ret) {
		return ret;
	}

	LOG_DBG("AD5940: CAL mux=0x%06X DFTREAL=0x%08X DFTIMAG=0x%08X",
		adc_mux, dft_real_raw, dft_imag_raw);

	*out_real = ad5940_dft_sign_extend(dft_real_raw);
	*out_imag = ad5940_dft_sign_extend(dft_imag_raw);

	return 0;
}

static int ad5940_adc_sample_raw(const struct device *dev, uint32_t adc_mux,
				 uint16_t *code)
{
	uint32_t adcdat = 0u;
	int stop_ret;
	int ret;
	int i;

	ret = ad5940_reg_write(dev, AD5940_REG_ADCCON, adc_mux);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_AFECON,
			       AD5940_AFECON_ADCEN_MSK);
	if (ret) {
		return ret;
	}
	k_busy_wait(AD5940_ANALOG_SETTLE_US);

	ret = ad5940_reg_write(dev, AD5940_REG_AFECON,
			       AD5940_AFECON_ADCEN_MSK | AD5940_AFECON_ADCCONVEN_MSK);
	if (ret) {
		return ret;
	}

	for (i = 0; i < AD5940_ADC_SETTLE_ITERS; i++) {
		k_busy_wait(AD5940_ADC_SETTLE_STEP_US);
	}

	ret = ad5940_reg_read(dev, AD5940_REG_ADCDAT, &adcdat);

	stop_ret = ad5940_reg_write(dev, AD5940_REG_AFECON, AD5940_AFECON_ADCEN_MSK);
	if (ret) {
		return ret;
	}
	if (stop_ret) {
		return stop_ret;
	}

	*code = (uint16_t)(adcdat & AD5940_REG16_MSK);
	return 0;
}

#ifdef CONFIG_AD5940_ADC_SELFCAL
static int ad5940_adc_pga_calibrate(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	const uint16_t midcode = AD5940_ADC_MIDSCALE_CODE;
	uint32_t short_mux;
	uint32_t acc = 0u;
	uint16_t code;
	uint16_t measured;
	uint16_t offset_reg;
	int32_t offset;
	uint8_t pga = data->adc_cfg.pga_gain;
	int ret;
	int n;

	LOG_INF("AD5940: starting ADC offset self-cal (PGA gain code %u)", pga);

	ret = ad5940_wakeup(dev);
	if (ret) {
		LOG_WRN("AD5940: wakeup failed before ADC self-cal: %d", ret);
	}

	ret = ad5940_reg_write(dev, AD5940_REG_ADCCON,
			       FIELD_PREP(AD5940_ADCCON_GNPGA_MSK, (uint32_t)pga));
	if (ret) {
		return ret;
	}

	short_mux = FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK, AD5940_ADCCON_MUXSELP_LPTIA_P) |
		    FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK, AD5940_ADCCON_MUXSELN_VZERO0);

	for (n = 0; n < 8; n++) {
		code = 0u;

		ret = ad5940_adc_sample_raw(dev, short_mux, &code);
		if (ret) {
			LOG_WRN("AD5940: ADC self-cal sample %d failed: %d", n, ret);
			return ret;
		}
		acc += code;
	}

	measured = (uint16_t)(acc / 8u);
	offset = (int32_t)midcode - (int32_t)measured;

	if (offset > AD5940_ADC_SELFCAL_MAX_OFFSET ||
	    offset < -AD5940_ADC_SELFCAL_MAX_OFFSET) {
		data->adc_cal.valid = false;
		LOG_WRN("AD5940: ADC self-cal rejected: measured=0x%04X offset=%d out of "
			"range (+/-%d), offset register left unchanged",
			measured, offset, AD5940_ADC_SELFCAL_MAX_OFFSET);
		return 0;
	}

	data->adc_cal.valid    = true;
	data->adc_cal.pga_gain = pga;
	data->adc_cal.offset   = offset;
	data->adc_cal.gain     = 1.0f;

	offset_reg = AD5940_REG_ADCOFFSETGN1;
	ret = ad5940_reg_write(dev, offset_reg, (uint32_t)(offset & AD5940_REG16_MSK));
	if (ret) {
		return ret;
	}

	LOG_INF("AD5940: ADC self-cal done: measured=0x%04X offset=%d (applied to 0x%04X)",
		measured, offset, offset_reg);
	return 0;
}
#endif /* CONFIG_AD5940_ADC_SELFCAL */

/**
 * @brief Perform HSTIA RTIA calibration using the on-chip RCAL resistor.
 *
 * Routes the RCAL into the signal path and takes two DFT captures (voltage
 * across RCAL, then current through RTIA). Computes the polar RTIA impedance
 * (magnitude in ohms, phase in radians) and stores it in data->rtia_cal[].
 * Restores the normal EIS switch matrix and excitation amplitude when done.
 *
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_rtia_calibrate(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	const struct ad5940_config *cfg = dev->config;
	int32_t rcal_real, rcal_imag;
	int32_t rtia_real, rtia_imag;
	float rcal_mag, rtia_mag;
	uint32_t mux_rcal, mux_rtia;
	uint32_t rtia_idx;
	uint32_t rtia_ohms;
	float excit_mvpp;
	uint32_t wg_amp;
	int ret;
	static const uint32_t rtia_ohms_tbl[] = {
		200u, 1000u, 5000u, 10000u, 20000u, 40000u, 80000u, 160000u,
	};

	LOG_DBG("AD5940: starting RTIA calibration, RCAL=%u Ohm", cfg->rcal_ohms);
	ret = ad5940_wakeup(dev);
	if (ret) {
		LOG_WRN("AD5940: wakeup failed before calibration: %d", ret);
	}

	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_DSWFULLCON, AD5940_DSWFULLCON_DR0_MSK);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_PSWFULLCON, AD5940_PSWFULLCON_PR0_MSK);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_NSWFULLCON, AD5940_NSWFULLCON_NR1_MSK);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_TSWFULLCON,
			       AD5940_TSWFULLCON_TR1_MSK | AD5940_TSWFULLCON_T9_MSK);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_SWCON, AD5940_SWCON_SWSOURCESEL_MSK);
	if (ret) {
		return ret;
	}

	rtia_idx = (uint32_t)data->eis.rtia_sel & 0xFu;
	rtia_ohms = (rtia_idx < ARRAY_SIZE(rtia_ohms_tbl)) ?
			     rtia_ohms_tbl[rtia_idx] : AD5940_RTIA_DEFAULT_OHMS;

	excit_mvpp = AD5940_CAL_EXCIT_MVPP_BASE * (float)cfg->rcal_ohms / (float)rtia_ohms;
	wg_amp = (uint32_t)((excit_mvpp / AD5940_EXCIT_BUF_FS_MVPP) *
			    (float)AD5940_WG_AMP_FULL_SCALE + AD5940_ROUND_F);
	if (wg_amp > AD5940_WG_AMP_MAX_WORD) {
		LOG_WRN("AD5940: cal wg_amp 0x%03X clamped to full scale 0x%03X"
			" — check RCAL/RTIA ratio",
			wg_amp, AD5940_WG_AMP_MAX_WORD);
		wg_amp = AD5940_WG_AMP_MAX_WORD;
	}
	if (wg_amp < AD5940_CAL_MIN_AMP_WORD) {
		wg_amp = AD5940_CAL_MIN_AMP_WORD;
	}

	LOG_DBG("AD5940: CAL excit: RTIA=%u Ohm RCAL=%u Ohm -> %d mVpp word=0x%03X",
		rtia_ohms, cfg->rcal_ohms, (int)excit_mvpp, wg_amp);

	ret = ad5940_reg_write(dev, AD5940_REG_WGAMPLITUDE, wg_amp);
	if (ret) {
		return ret;
	}

	mux_rcal = FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK, AD5940_ADCCON_MUXSELP_P_NODE) |
		   FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK, AD5940_ADCCON_MUXSELN_N_NODE);
	mux_rtia = FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK, AD5940_ADCCON_MUXSELP_HSTIA_P) |
		   FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK, AD5940_ADCCON_MUXSELN_HSTIA_N);

	/* DFT #1: voltage across RCAL */
	ret = ad5940_cal_capture_dft(dev, mux_rcal, &rcal_real, &rcal_imag);
	if (ret == -ETIMEDOUT) {
		LOG_WRN("AD5940: calibration DFT#1 (RCAL) timeout, using defaults");
		data->rtia_cal[0] = (float)cfg->rcal_ohms;
		data->rtia_cal[1] = 0.0f;
		goto restore;
	} else if (ret) {
		return ret;
	}

	/* DFT #2: current through RTIA */
	ret = ad5940_cal_capture_dft(dev, mux_rtia, &rtia_real, &rtia_imag);
	if (ret == -ETIMEDOUT) {
		LOG_WRN("AD5940: calibration DFT#2 (RTIA) timeout, using defaults");
		data->rtia_cal[0] = (float)cfg->rcal_ohms;
		data->rtia_cal[1] = 0.0f;
		goto restore;
	} else if (ret) {
		return ret;
	}

	rtia_real = -rtia_real;
	rtia_imag = -rtia_imag;   /* current direction inversion */

	rtia_imag = -rtia_imag;   /* DFT engine returns negated imag */
	rcal_imag = -rcal_imag;   /* DFT engine returns negated imag */

	rcal_mag = hypotf((float)rcal_real, (float)rcal_imag);
	rtia_mag = hypotf((float)rtia_real, (float)rtia_imag);

	if (rcal_mag < 1.0f) {
		/* No signal across RCAL — excitation path broken. */
		LOG_WRN("AD5940: CAL: DftRcal magnitude ~0, check RCAL wiring");
		data->rtia_cal[0] = (float)cfg->rcal_ohms;
		data->rtia_cal[1] = 0.0f;
		goto restore;
	}

	/*
	 * RTIA = RCAL * DftRtia / DftRcal (complex division). We store the polar
	 * form: rtia_cal[0] = |RTIA| in ohms, rtia_cal[1] = phase in radians.
	 */
	data->rtia_cal[0] = (rtia_mag / rcal_mag) * (float)cfg->rcal_ohms;
	data->rtia_cal[1] = atan2f((float)rtia_imag, (float)rtia_real) -
			    atan2f((float)rcal_imag, (float)rcal_real);

	LOG_DBG("AD5940: RTIA cal: mag=%.2f Ohm, phase=%.4f rad",
		(double)data->rtia_cal[0], (double)data->rtia_cal[1]);

	data->calibrated = true;

restore:
	ret = ad5940_reg_write(dev, AD5940_REG_WGAMPLITUDE,
			       (data->eis.amplitude_mvpp * AD5940_WG_AMP_FULL_SCALE) /
				       AD5940_WG_FULL_SCALE_MVPP);
	if (ret) {
		LOG_ERR("AD5940: cal restore WGAMPLITUDE failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_DSWFULLCON, cfg->sw_dsw);
	if (ret) {
		LOG_ERR("AD5940: cal restore DSWFULLCON failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_NSWFULLCON, cfg->sw_nsw);
	if (ret) {
		LOG_ERR("AD5940: cal restore NSWFULLCON failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_PSWFULLCON, cfg->sw_psw);
	if (ret) {
		LOG_ERR("AD5940: cal restore PSWFULLCON failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_TSWFULLCON, cfg->sw_tsw);
	if (ret) {
		LOG_ERR("AD5940: cal restore TSWFULLCON failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_SWCON, AD5940_SWCON_SWSOURCESEL_MSK);
	if (ret) {
		LOG_ERR("AD5940: cal restore SWCON failed: %d", ret);
	}
	ret = ad5940_reg_write(dev, AD5940_REG_ADCCON,
			       AD5940_ADCMUX_WORD(cfg->adc_mux_i_p, cfg->adc_mux_i_n));
	if (ret) {
		LOG_ERR("AD5940: cal restore ADCCON failed: %d", ret);
	}
	if (ret) {
		data->calibrated = false;
	}

	return ret;
}

/**
 * @brief Initialize the AD5940/AD5941 sensor driver.
 *
 * Sequences: HW reset (if GPIO present) → SPI unlock → SW reset → ADIID/CHIPID
 * check → sys init → clock config → FIFO config → trigger init → EIS mode config
 * → sequencer config → RTIA calibration → ADC self-cal (if enabled) → FIFO flush.
 *
 * @param dev Device pointer
 * @return 0 on success, negative errno on failure
 */
static int ad5940_init(const struct device *dev)
{
	const struct ad5940_config *cfg = dev->config;
	struct ad5940_data *data = dev->data;
	uint32_t id_val;
	int ret;

	k_mutex_init(&data->lock);

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("AD5940: SPI bus not ready");
		return -ENODEV;
	}

	data->dev = dev;

	data->mode = AD5940_MODE_EIS;
	data->eis.freq_hz        = (float)cfg->def_eis_freq_hz;
	data->eis.amplitude_mvpp = cfg->def_eis_amplitude_mvpp;
	data->eis.rtia_sel       = cfg->def_eis_rtia_sel;
	data->eis.settling_cycles = cfg->def_eis_settling_cycles;

	data->eis.dft_num           = cfg->def_eis_dft_num;
	data->eis.dft_src           = cfg->def_eis_dft_src;
	data->eis.hanning_win       = cfg->def_eis_hanning_win;
	data->eis.sinc3_osr         = cfg->def_eis_sinc3_osr;
	data->eis.excit_buf_gain    = cfg->def_eis_excit_buf_gain;
	data->eis.hsdac_gain        = cfg->def_eis_hsdac_gain;
	data->eis.hsdac_update_rate = cfg->def_eis_hsdac_update_rate;
	data->eis.ctia_sel          = cfg->def_eis_ctia_sel;
	data->eis.power_mode        = cfg->def_eis_power_mode;
	data->amper.bias_mv      = cfg->def_bias_mv;
	data->amper.lpamp_rtia   = cfg->def_lpamp_rtia;
	data->amper.sinc2_osr    = 0u;
	data->sweep.start_freq_hz = (float)cfg->def_sweep_start_hz;
	data->sweep.stop_freq_hz  = (float)cfg->def_sweep_stop_hz;
	data->sweep.points        = cfg->def_sweep_points;
	data->sweep.log_scale     = cfg->def_sweep_log_scale;
	data->sweep.needs_recal   = true;
	if (cfg->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("AD5940: failed to configure reset GPIO: %d", ret);
			return ret;
		}
		k_busy_wait(AD5940_RESET_PULSE_US);
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret) {
			LOG_ERR("AD5940: failed to deassert reset GPIO: %d", ret);
			return ret;
		}
		k_msleep(1);
	}

	ret = ad5940_spi_unlock(dev);
	if (ret) {
		LOG_ERR("AD5940: SPI unlock failed: %d", ret);
		return ret;
	}

	ret = ad5940_sw_reset(dev);
	if (ret) {
		LOG_ERR("AD5940: software reset failed: %d", ret);
		return ret;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_ADIID, &id_val);
	if (ret) {
		LOG_ERR("AD5940: failed to read ADIID: %d", ret);
		return -EIO;
	}

	if ((id_val & AD5940_REG16_MSK) != AD5940_ADI_ID) {
		LOG_ERR("AD5940: unexpected ADIID=0x%04X (expected 0x%04X)",
			(unsigned int)(id_val & AD5940_REG16_MSK), AD5940_ADI_ID);
		return -EIO;
	}
	data->adi_id = (uint16_t)(id_val & AD5940_REG16_MSK);

	ret = ad5940_reg_read(dev, AD5940_REG_CHIPID, &id_val);
	if (ret) {
		LOG_ERR("AD5940: failed to read CHIPID: %d", ret);
		return -EIO;
	}
	data->chip_id = (uint16_t)(id_val & AD5940_REG16_MSK);
	LOG_INF("AD5940: CHIPID=0x%04X", data->chip_id);

	if (data->chip_id != AD5940_CHIP_ID_S1 &&
	    data->chip_id != AD5940_CHIP_ID_S2 &&
	    data->chip_id != AD5940_CHIP_ID_S3) {
		LOG_WRN("AD5940: unknown CHIPID=0x%04X, continuing", data->chip_id);
	}

	ret = ad5940_sys_init_sequence(dev);
	if (ret) {
		LOG_ERR("AD5940: system init sequence failed: %d", ret);
		return ret;
	}

	ret = ad5940_clock_config(dev);
	if (ret) {
		LOG_ERR("AD5940: clock config failed: %d", ret);
		return ret;
	}

	ret = ad5940_fifo_config(dev, AD5940_FIFOSRC_DFT);
	if (ret) {
		LOG_ERR("AD5940: FIFO config failed: %d", ret);
		return ret;
	}

#ifdef CONFIG_AD5940_TRIGGER
	ret = ad5940_trigger_init(dev);
	if (ret) {
		LOG_ERR("AD5940: trigger init failed: %d", ret);
		return ret;
	}
#endif

	ret = ad5940_eis_mode_config(dev);
	if (ret) {
		LOG_ERR("AD5940: EIS mode config failed: %d", ret);
		return ret;
	}

	ret = ad5940_sequencer_config(dev);
	if (ret) {
		LOG_WRN("AD5940: sequencer config failed: %d (non-fatal)", ret);
	}

	ret = ad5940_rtia_calibrate(dev);
	if (ret) {
		LOG_WRN("AD5940: RTIA calibration failed: %d, using defaults", ret);
		data->rtia_cal[0] = (float)cfg->rcal_ohms;
		data->rtia_cal[1] = 0.0f;
	}

#ifdef CONFIG_AD5940_ADC_SELFCAL
	ret = ad5940_adc_pga_calibrate(dev);
	if (ret) {
		LOG_WRN("AD5940: ADC self-cal failed: %d (non-fatal)", ret);
	}
#endif

	ret = ad5940_fifo_flush(dev);
	if (ret) {
		LOG_WRN("AD5940: FIFO flush failed: %d (non-fatal)", ret);
	}

	data->initialized = true;
	LOG_INF("AD5940: initialization complete");

	return 0;
}

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
/**
 * @brief Advance the EIS frequency sweep to the next point and update FCW.
 *
 * Called from both the synchronous sample fetch path and the RTIO streaming
 * completion callback so sweep state stays consistent in both paths.
 */
void ad5940_advance_sweep_freq(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	float next_freq;
	float log_start;
	float log_stop;
	float step;

	data->sweep_current_idx++;
	if (data->sweep_current_idx >= data->sweep.points) {
		data->sweep_current_idx = 0u;
	}

	if (data->sweep.log_scale) {
		log_start = log10f(data->sweep.start_freq_hz);
		log_stop  = log10f(data->sweep.stop_freq_hz);
		step = (log_stop - log_start) / (float)(data->sweep.points - 1u);

		next_freq = powf(AD5940_DECADE_BASE_F,
				 log_start + step * (float)data->sweep_current_idx);
	} else {
		step = (data->sweep.stop_freq_hz - data->sweep.start_freq_hz) /
		       (float)(data->sweep.points - 1u);

		next_freq = data->sweep.start_freq_hz +
			    step * (float)data->sweep_current_idx;
	}

	data->eis.freq_hz = next_freq;
	ad5940_reg_write(dev, AD5940_REG_WGFCW, ad5940_freq_to_fcw(next_freq));

	LOG_DBG("sweep: point %u/%u -> %d Hz", data->sweep_current_idx,
		data->sweep.points, (int)next_freq);
}
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

/**
 * @brief Read one EIS impedance sample (4 DFT words) synchronously.
 *
 * If the FIFO already has ≥4 words (e.g. called from trigger callback after
 * free-running DFT has filled it), reads immediately without starting the
 * wakeup timer. Otherwise wakes the AFE, starts the timer, and polls.
 */
static int ad5940_sample_fetch_eis(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	uint32_t raw_words[AD5940_EIS_WORDS_PER_FRAME];
	uint32_t fifo_cnt;
	uint32_t fifo_avail;
	uint32_t wuptmr;
	uint32_t intflag;
	int32_t v_real, v_imag, i_real, i_imag;
	float denom, z_real, z_imag;
	float mag, phase;
	bool frame_valid;
	uint8_t w;
	int ret;
	int i;
	bool timer_started = false;

	ret = ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt);
	if (ret) {
		return ret;
	}

	if (FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, fifo_cnt) <
	    AD5940_EIS_WORDS_PER_FRAME) {
		ret = ad5940_wakeup(dev);
		if (ret) {
			return ret;
		}

		ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
		if (ret) {
			return ret;
		}

		ret = ad5940_reg_read(dev, AD5940_REG_WUPTMRCON, &wuptmr);
		if (ret) {
			return ret;
		}

		ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON,
				       wuptmr | AD5940_TMRCON_WUPTEN_MSK);
		if (ret) {
			return ret;
		}
		timer_started = true;

		for (i = 0; i < AD5940_FIFO_POLL_COUNT; i++) {
			ret = ad5940_reg_read(dev, AD5940_REG_INTCFLAG0, &intflag);
			if (ret) {
				return ret;
			}

			if (intflag & AFEINTSRC_DATAFIFOTHRESH) {
				break;
			}

			k_busy_wait(AD5940_POLL_STEP_US);
		}

		if (i >= AD5940_FIFO_POLL_COUNT) {
			LOG_ERR("AD5940: sample_fetch timed out");
			return -ETIMEDOUT;
		}
	}

	ret = ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt);
	if (ret) {
		return ret;
	}

	fifo_avail = FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, fifo_cnt);

	if (fifo_avail < AD5940_EIS_WORDS_PER_FRAME) {
		LOG_ERR("AD5940: FIFO holds %u words, need %u", fifo_avail,
			AD5940_EIS_WORDS_PER_FRAME);
		return -EIO;
	}

	ret = ad5940_fifo_read_words(dev, raw_words, AD5940_EIS_WORDS_PER_FRAME);
	if (ret) {
		return ret;
	}

	frame_valid = true;
	for (w = 0u; w < AD5940_EIS_WORDS_PER_FRAME; w++) {
		if (FIELD_GET(AD5940_DFT_CHID_TOP_MSK, raw_words[w]) !=
		    AD5940_DFT_CHID_TOP_DFT) {
			frame_valid = false;
			break;
		}
	}

	v_real = frame_valid ?  ad5940_dft_sign_extend(raw_words[0]) : 0;
	v_imag = frame_valid ? -ad5940_dft_sign_extend(raw_words[1]) : 0;
	i_real = frame_valid ? -ad5940_dft_sign_extend(raw_words[2]) : 0;
	i_imag = frame_valid ?  ad5940_dft_sign_extend(raw_words[3]) : 0;

	data->result.dft_real = v_real;
	data->result.dft_imag = v_imag;

	denom = (float)((int64_t)i_real * i_real + (int64_t)i_imag * i_imag);

	if (denom < 1.0f) {
		data->result.magnitude_ohm = 0.0f;
		data->result.phase_rad = 0.0f;
	} else {
		z_real = ((float)(v_real * i_real + v_imag * i_imag)) / denom;
		z_imag = ((float)(v_imag * i_real - v_real * i_imag)) / denom;

		mag = sqrtf(z_real * z_real + z_imag * z_imag);
		phase = atan2f(z_imag, z_real);

		if (data->rtia_cal[0] > 0.0f) {
			mag   *= data->rtia_cal[0];
			phase += data->rtia_cal[1];
		}

		data->result.magnitude_ohm = mag;
		data->result.phase_rad = phase;
	}

	ret = ad5940_reg_write(dev, AD5940_REG_INTCCLR, AFEINTSRC_ALLINT);
	if (ret) {
		return ret;
	}

	if (timer_started) {
		ret = ad5940_reg_read(dev, AD5940_REG_WUPTMRCON, &wuptmr);
		if (ret) {
			return ret;
		}
		ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON,
				       wuptmr & ~AD5940_TMRCON_WUPTEN_MSK);
		if (ret) {
			return ret;
		}
	}

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
	if (data->sweep_enabled) {
		ad5940_advance_sweep_freq(dev);
	}
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

	return 0;
}

/**
 * @brief Read one amperometry sample synchronously.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sample_fetch_amper(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	uint32_t fifo_cnt;
	uint32_t fifo_word;
	uint16_t adc_code;
	int ret;
	int i;

	ret = ad5940_wakeup(dev);
	if (ret) {
		return ret;
	}

	for (i = 0; i < AD5940_FIFO_POLL_COUNT; i++) {
		ret = ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt);
		if (ret) {
			return ret;
		}
		if (FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, fifo_cnt) >= 1u) {
			break;
		}
		k_busy_wait(AD5940_POLL_STEP_US);
	}

	if (i >= AD5940_FIFO_POLL_COUNT) {
		return -ETIMEDOUT;
	}

	ret = ad5940_fifo_read_words(dev, &fifo_word, 1u);
	if (ret) {
		return ret;
	}

	adc_code = (uint16_t)(fifo_word & AD5940_ADCDAT_DATA_MSK);
	data->result.current_na = (int32_t)adc_code;

	return 0;
}

/**
 * @brief Read one raw ADC sample synchronously.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sample_fetch_adc(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	uint16_t code = 0u;
	uint32_t adc_mux;
	int ret;

	ret = ad5940_wakeup(dev);
	if (ret) {
		return ret;
	}

	adc_mux = FIELD_PREP(AD5940_ADCCON_MUXSELP_MSK, AD5940_ADCCON_MUXSELP_AIN2) |
		  FIELD_PREP(AD5940_ADCCON_MUXSELN_MSK, AD5940_ADCCON_MUXSELN_AIN3) |
		  FIELD_PREP(AD5940_ADCCON_GNPGA_MSK, (uint32_t)data->adc_cfg.pga_gain);

	ret = ad5940_adc_sample_raw(dev, adc_mux, &code);
	if (ret) {
		return ret;
	}

	data->result.adc_raw = (int32_t)code;

	return 0;
}

/**
 * @brief Read one ADC potential sample and convert the code to millivolts.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sample_fetch_potential(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	float volts = 0.0f;
	int ret;

	ret = ad5940_sample_fetch_adc(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_adc_code_to_volt((uint32_t)data->result.adc_raw,
				      data->adc_cfg.pga_gain, &volts);
	if (ret) {
		return ret;
	}

	data->result.potential_mv = volts * AD5940_MV_PER_V;
	return 0;
}

/**
 * @brief Read one SINC2+Notch filtered ADC sample.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sample_fetch_sinc2(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	int ret;

	ret = ad5940_sample_fetch_adc(dev);
	if (ret) {
		return ret;
	}

	data->result.sinc2 = data->result.adc_raw & (int32_t)AD5940_ADCDAT_DATA_MSK;
	return 0;
}

/**
 * @brief Read the statistics-engine mean and variance registers.
 * @param dev Device pointer
 * @return 0 on success, negative errno on SPI error
 */
static int ad5940_sample_fetch_stats(const struct device *dev)
{
	struct ad5940_data *data = dev->data;
	uint32_t mean = 0u;
	uint32_t var = 0u;
	int ret;

	ret = ad5940_wakeup(dev);
	if (ret) {
		return ret;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_STATSMEAN, &mean);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_read(dev, AD5940_REG_STATSVAR, &var);
	if (ret) {
		return ret;
	}

	data->result.mean     = (int32_t)mean;
	data->result.variance = (int32_t)var;
	return 0;
}

/**
 * @brief Zephyr sensor_sample_fetch() implementation.
 * @param dev  Device pointer
 * @param chan Sensor channel to fetch, or SENSOR_CHAN_ALL
 * @return 0 on success, -ENOTSUP for unsupported channel/mode combinations
 */
static int ad5940_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ad5940_data *data = dev->data;
	int ret;

#ifdef CONFIG_AD5940_STREAM
	if (data->active_sqe != NULL) {
		return -ENOTSUP;
	}
#endif

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (data->mode) {
	case AD5940_MODE_EIS:
		if (chan == SENSOR_CHAN_ALL ||
		    chan == (enum sensor_channel)SENSOR_CHAN_AD5940_DFT ||
		    chan == (enum sensor_channel)SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE) {
			ret = ad5940_sample_fetch_eis(dev);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case AD5940_MODE_AMPEROMETRY:
		if (chan == SENSOR_CHAN_ALL ||
		    chan == (enum sensor_channel)SENSOR_CHAN_AD5940_CURRENT) {
			ret = ad5940_sample_fetch_amper(dev);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case AD5940_MODE_ADC:
		if (chan == SENSOR_CHAN_ALL ||
		    chan == (enum sensor_channel)SENSOR_CHAN_AD5940_ADC_RAW) {
			ret = ad5940_sample_fetch_adc(dev);
		} else if (chan == (enum sensor_channel)SENSOR_CHAN_AD5940_POTENTIAL) {
			ret = ad5940_sample_fetch_potential(dev);
		} else if (chan == (enum sensor_channel)SENSOR_CHAN_AD5940_SINC2) {
			ret = ad5940_sample_fetch_sinc2(dev);
		} else if (chan == (enum sensor_channel)SENSOR_CHAN_AD5940_MEAN ||
			   chan == (enum sensor_channel)SENSOR_CHAN_AD5940_VARIANCE) {
			ret = ad5940_sample_fetch_stats(dev);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case AD5940_MODE_CYCLIC_VOLTAMMETRY:
		ret = -ENOTSUP;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Zephyr sensor_channel_get() implementation.
 *
 * Returns cached values from data->result without hardware access (FR-SYNC-005).
 */
static int ad5940_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_AD5940_IMPEDANCE_MAGNITUDE:
		/* val1 = integer Ohms, val2 = fractional micro-Ohms */
		val->val1 = (int32_t)data->result.magnitude_ohm;
		val->val2 = (int32_t)((data->result.magnitude_ohm -
				       (float)val->val1) * AD5940_MICRO_PER_UNIT);
		break;

	case SENSOR_CHAN_AD5940_IMPEDANCE_PHASE:
		/* val1 = integer radians, val2 = fractional micro-radians */
		val->val1 = (int32_t)data->result.phase_rad;
		val->val2 = (int32_t)((data->result.phase_rad -
				       (float)val->val1) * AD5940_MICRO_PER_UNIT);
		break;

	case SENSOR_CHAN_AD5940_IMPEDANCE_REAL:
		val->val1 = data->result.dft_real;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_IMPEDANCE_IMAG:
		val->val1 = data->result.dft_imag;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_CURRENT:
		val->val1 = data->result.current_na;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_ADC_RAW:
		val->val1 = data->result.adc_raw;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_DFT:
		/* Return raw DFT real in val1 (debug/calibration use) */
		val->val1 = data->result.dft_real;
		val->val2 = data->result.dft_imag;
		break;

	case SENSOR_CHAN_AD5940_POTENTIAL:
		/* val1 = integer mV, val2 = fractional micro-mV */
		val->val1 = (int32_t)data->result.potential_mv;
		val->val2 = (int32_t)((data->result.potential_mv -
				       (float)val->val1) * AD5940_MICRO_PER_UNIT);
		break;

	case SENSOR_CHAN_AD5940_SINC2:
		val->val1 = data->result.sinc2;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_VARIANCE:
		val->val1 = data->result.variance;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AD5940_MEAN:
		val->val1 = data->result.mean;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ad5940_set_eis_freq_hz(const struct device *dev,
				  const struct sensor_value *val)
{
	float freq_hz = (float)val->val1 + (float)val->val2 * 1e-6f;

	if (freq_hz < AD5940_EXCIT_MIN_FREQ_HZ || freq_hz > AD5940_EXCIT_MAX_FREQ_HZ) {
		return -EINVAL;
	}
	return ad5940_reg_write(dev, AD5940_REG_WGFCW, ad5940_freq_to_fcw(freq_hz));
}

static int ad5940_set_eis_amplitude_mvpp(const struct device *dev,
					 const struct sensor_value *val)
{
	uint32_t amp_word;

	if (val->val1 < 1 || val->val1 > AD5940_EXCIT_MAX_MVPP) {
		return -EINVAL;
	}

	amp_word = ((uint32_t)val->val1 * AD5940_WG_AMP_FULL_SCALE) /
		   AD5940_WG_FULL_SCALE_MVPP;

	return ad5940_reg_write(dev, AD5940_REG_WGAMPLITUDE, amp_word);
}

static int ad5940_set_eis_settling_cycles(const struct device *dev,
					  const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < 0 || val->val1 > AD5940_U8_MAX) {
		return -EINVAL;
	}
	data->eis.settling_cycles = (uint8_t)val->val1;
	return 0;
}

static int ad5940_set_rtia_sel(const struct device *dev,
			       const struct sensor_value *val)
{
	uint32_t rtiacon_val;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_RTIA_SEL_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_HSRTIACON, &rtiacon_val);
	if (ret) {
		return ret;
	}
	rtiacon_val &= ~AD5940_HSRTIACON_RTIACON_MSK;
	rtiacon_val |= FIELD_PREP(AD5940_HSRTIACON_RTIACON_MSK,
				  (uint32_t)val->val1 & AD5940_RTIA_SEL_MSK);
	return ad5940_reg_write(dev, AD5940_REG_HSRTIACON, rtiacon_val);
}

static int ad5940_set_bias_mv(const struct device *dev,
			      const struct sensor_value *val)
{
	uint32_t dac_code;

	if (val->val1 < (int32_t)AD5940_LPDAC_MIN_MV ||
	    val->val1 > (int32_t)AD5940_LPDAC_MAX_MV) {
		return -EINVAL;
	}

	dac_code = ((uint32_t)(val->val1 - (int32_t)AD5940_LPDAC_MIN_MV) *
		    AD5940_LPDAC_FULL_SCALE) / AD5940_LPDAC_SPAN_MV;

	return ad5940_reg_write(dev, AD5940_REG_LPDACDAT0, dac_code);
}

static int ad5940_set_lpamp_rtia(const struct device *dev,
				 const struct sensor_value *val)
{
	uint32_t lptiacon;
	int ret;

	if (val->val1 < 0 || val->val1 > 7) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_LPTIACON0, &lptiacon);
	if (ret) {
		return ret;
	}

	lptiacon &= ~AD5940_LPTIACON0_TIAGAIN_MSK;
	lptiacon |= FIELD_PREP(AD5940_LPTIACON0_TIAGAIN_MSK, (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_LPTIACON0, lptiacon);
}

static int ad5940_set_adc_sinc2_osr(const struct device *dev,
				    const struct sensor_value *val)
{
	uint32_t filtercon;
	int ret;

	if (val->val1 < 0 || val->val1 > 6) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_ADCFILTERCON, &filtercon);
	if (ret) {
		return ret;
	}
	filtercon &= ~AD5940_ADCFILTERCON_SINC2OSR_MSK;
	filtercon |= FIELD_PREP(AD5940_ADCFILTERCON_SINC2OSR_MSK,
				(uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_ADCFILTERCON, filtercon);
}

static int ad5940_set_adc_pga_gain(const struct device *dev,
				   const struct sensor_value *val)
{
	uint32_t adccon;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_ADCCON, &adccon);
	if (ret) {
		return ret;
	}
	adccon &= ~AD5940_ADCCON_GNPGA_MSK;
	adccon |= FIELD_PREP(AD5940_ADCCON_GNPGA_MSK, (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_ADCCON, adccon);
}

static int ad5940_set_odr_hz(const struct device *dev,
			     const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < AD5940_ODR_MIN_HZ) {
		return -EINVAL;
	}
	data->odr_hz = (uint32_t)val->val1;
	return 0;
}

static int ad5940_set_stats_enable(const struct device *dev,
				   const struct sensor_value *val)
{
	uint32_t sample_num;
	bool enable;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_STATSSAMPLE_128) {
		return -EINVAL;
	}
	sample_num = (uint32_t)val->val1;
	enable = (sample_num != 0u);

	ret = ad5940_afe_stats_cfg(dev, sample_num, enable);
	if (ret) {
		return ret;
	}

	return ad5940_fifo_config(dev, enable ? AD5940_FIFOSRC_MEAN
					      : AD5940_FIFOSRC_ADC);
}

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
static void ad5940_sweep_rearm(const struct device *dev)
{
	struct ad5940_data *data = dev->data;

	data->sweep_enabled     = (data->sweep.points > 1u);
	data->sweep_current_idx = 0u;
	data->eis.freq_hz       = data->sweep.start_freq_hz;

	if (data->initialized && data->mode == AD5940_MODE_EIS) {
		ad5940_reg_write(dev, AD5940_REG_WGFCW,
				 ad5940_freq_to_fcw(data->eis.freq_hz));
	}
}

static int ad5940_set_eis_freq_start_hz(const struct device *dev,
					const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	data->sweep.start_freq_hz = (float)val->val1 +
				    (float)val->val2 * AD5940_MICRO_SCALE_F;
	data->sweep.needs_recal = true;
	ad5940_sweep_rearm(dev);
	return 0;
}

static int ad5940_set_eis_freq_stop_hz(const struct device *dev,
				       const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	data->sweep.stop_freq_hz = (float)val->val1 +
				   (float)val->val2 * AD5940_MICRO_SCALE_F;
	data->sweep.needs_recal = true;
	ad5940_sweep_rearm(dev);
	return 0;
}

static int ad5940_set_eis_freq_points(const struct device *dev,
				      const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < AD5940_SWEEP_POINTS_MIN ||
	    val->val1 > CONFIG_AD5940_MAX_SWEEP_POINTS) {
		return -EINVAL;
	}
	data->sweep.points = (uint16_t)val->val1;
	data->sweep.needs_recal = true;
	ad5940_sweep_rearm(dev);
	return 0;
}

static int ad5940_set_eis_freq_log_scale(const struct device *dev,
					 const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	data->sweep.log_scale = (val->val1 != 0);
	return 0;
}
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

static int ad5940_set_cv_vertex1_mv(const struct device *dev,
				    const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < (int32_t)AD5940_LPDAC_MIN_MV ||
	    val->val1 > (int32_t)AD5940_LPDAC_MAX_MV) {
		return -EINVAL;
	}
	data->cv.vertex1_mv = val->val1;
	return 0;
}

static int ad5940_set_cv_vertex2_mv(const struct device *dev,
				    const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < (int32_t)AD5940_LPDAC_MIN_MV ||
	    val->val1 > (int32_t)AD5940_LPDAC_MAX_MV) {
		return -EINVAL;
	}
	if (val->val1 == data->cv.vertex1_mv) {
		return -EINVAL;
	}
	data->cv.vertex2_mv = val->val1;
	return 0;
}

static int ad5940_set_cv_init_mv(const struct device *dev,
				 const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < (int32_t)AD5940_LPDAC_MIN_MV ||
	    val->val1 > (int32_t)AD5940_LPDAC_MAX_MV) {
		return -EINVAL;
	}
	data->cv.init_mv = val->val1;
	return 0;
}

static int ad5940_set_cv_scan_rate_mv_per_s(const struct device *dev,
					    const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < 1 || val->val1 > AD5940_CV_SCAN_RATE_MAX) {
		return -EINVAL;
	}
	data->cv.scan_rate_mv_s = val->val1;
	return 0;
}

static int ad5940_set_cv_cycles(const struct device *dev,
				const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	if (val->val1 < 1 || val->val1 > AD5940_U8_MAX) {
		return -EINVAL;
	}
	data->cv.cycles = (uint8_t)val->val1;
	return 0;
}

static int ad5940_set_dft_num(const struct device *dev,
			      const struct sensor_value *val)
{
	uint32_t dftcon;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_DFT_NUM_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);
	if (ret) {
		return ret;
	}
	dftcon &= ~AD5940_DFTCON_DFTNUM_MSK;
	dftcon |= FIELD_PREP(AD5940_DFTCON_DFTNUM_MSK, (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_DFTCON, dftcon);
}

static int ad5940_set_dft_src(const struct device *dev,
			      const struct sensor_value *val)
{
	uint32_t dftcon;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_DFT_SRC_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);
	if (ret) {
		return ret;
	}
	dftcon &= ~AD5940_DFTCON_DFTINSEL_MSK;
	dftcon |= FIELD_PREP(AD5940_DFTCON_DFTINSEL_MSK, (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_DFTCON, dftcon);
}

static int ad5940_set_hanning_win(const struct device *dev,
				  const struct sensor_value *val)
{
	uint32_t dftcon;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);
	if (ret) {
		return ret;
	}
	dftcon &= ~AD5940_DFTCON_HANNINGEN_MSK;
	dftcon |= FIELD_PREP(AD5940_DFTCON_HANNINGEN_MSK,
			     val->val1 != 0 ? 1u : 0u);
	return ad5940_reg_write(dev, AD5940_REG_DFTCON, dftcon);
}

static int ad5940_set_adc_sinc3_osr(const struct device *dev,
				    const struct sensor_value *val)
{
	uint32_t filtercon;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_SINC3_OSR_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_ADCFILTERCON, &filtercon);
	if (ret) {
		return ret;
	}
	filtercon &= ~AD5940_ADCFILTERCON_SINC3OSR_MSK;
	filtercon |= FIELD_PREP(AD5940_ADCFILTERCON_SINC3OSR_MSK,
				(uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_ADCFILTERCON, filtercon);
}

static int ad5940_set_excit_buf_gain(const struct device *dev,
				     const struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);
	if (ret) {
		return ret;
	}
	hsdaccon &= ~AD5940_HSDACCON_ATTENEN_MSK;
	hsdaccon |= FIELD_PREP(AD5940_HSDACCON_ATTENEN_MSK,
			       val->val1 != 0 ? 1u : 0u);
	return ad5940_reg_write(dev, AD5940_REG_HSDACCON, hsdaccon);
}

static int ad5940_set_hsdac_gain(const struct device *dev,
				 const struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret;

	ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);
	if (ret) {
		return ret;
	}
	hsdaccon &= ~AD5940_HSDACCON_GAINX5_MSK;
	hsdaccon |= FIELD_PREP(AD5940_HSDACCON_GAINX5_MSK,
			       val->val1 != 0 ? 1u : 0u);
	return ad5940_reg_write(dev, AD5940_REG_HSDACCON, hsdaccon);
}

static int ad5940_set_hsdac_update_rate(const struct device *dev,
					const struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret;

	if (val->val1 < AD5940_HSDAC_RATE_MIN || val->val1 > AD5940_U8_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);
	if (ret) {
		return ret;
	}
	hsdaccon &= ~AD5940_HSDACCON_Rate_MSK;
	hsdaccon |= FIELD_PREP(AD5940_HSDACCON_Rate_MSK, (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_HSDACCON, hsdaccon);
}

static int ad5940_set_ctia_sel(const struct device *dev,
			       const struct sensor_value *val)
{
	uint32_t rtiacon;
	int ret;

	if (val->val1 < 0 || val->val1 > AD5940_CTIA_SEL_MAX) {
		return -EINVAL;
	}

	ret = ad5940_reg_read(dev, AD5940_REG_HSRTIACON, &rtiacon);
	if (ret) {
		return ret;
	}
	rtiacon &= ~AD5940_HSRTIACON_CTIACON_MSK;
	rtiacon |= FIELD_PREP(AD5940_HSRTIACON_CTIACON_MSK,
			      (uint32_t)val->val1);
	return ad5940_reg_write(dev, AD5940_REG_HSRTIACON, rtiacon);
}

static int ad5940_set_power_mode(const struct device *dev,
				 const struct sensor_value *val)
{
	uint32_t pmbw;
	int ret = ad5940_reg_read(dev, AD5940_REG_PMBW, &pmbw);

	if (ret) {
		return ret;
	}
	pmbw &= ~AD5940_PMBW_SYSHS_MSK;
	pmbw |= FIELD_PREP(AD5940_PMBW_SYSHS_MSK,
			   val->val1 != 0 ? 1u : 0u);
	return ad5940_reg_write(dev, AD5940_REG_PMBW, pmbw);
}

static int ad5940_get_chip_id(const struct device *dev, struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->chip_id;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_adi_id(const struct device *dev, struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->adi_id;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_rtia_cal_magnitude(const struct device *dev,
					 struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	sensor_value_from_double(val, (double)data->rtia_cal[0]);
	return 0;
}

static int ad5940_get_rtia_cal_phase(const struct device *dev,
				     struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	sensor_value_from_double(val, (double)data->rtia_cal[1]);
	return 0;
}

static int ad5940_get_fifo_count(const struct device *dev,
				 struct sensor_value *val)
{
	uint32_t fifo_cnt;
	int ret = ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &fifo_cnt);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK, fifo_cnt);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_eis_last_freq_hz(const struct device *dev,
				       struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	sensor_value_from_double(val, (double)data->last_stream_freq_hz);
	return 0;
}

static int ad5940_get_mode(const struct device *dev, struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->mode;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_eis_freq_hz(const struct device *dev,
				  struct sensor_value *val)
{
	uint32_t fcw;
	int ret = ad5940_reg_read(dev, AD5940_REG_WGFCW, &fcw);

	if (ret) {
		return ret;
	}
	sensor_value_from_double(val, (double)ad5940_fcw_to_freq(fcw));
	return 0;
}

static int ad5940_get_eis_amplitude_mvpp(const struct device *dev,
					 struct sensor_value *val)
{
	uint32_t amp_word;
	int ret = ad5940_reg_read(dev, AD5940_REG_WGAMPLITUDE, &amp_word);

	if (ret) {
		return ret;
	}

	val->val1 = (int32_t)((amp_word * AD5940_WG_FULL_SCALE_MVPP +
			       (AD5940_WG_AMP_FULL_SCALE / 2u)) /
			      AD5940_WG_AMP_FULL_SCALE);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_eis_settling_cycles(const struct device *dev,
					  struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->eis.settling_cycles;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_rtia_sel(const struct device *dev,
			       struct sensor_value *val)
{
	uint32_t rtiacon;
	int ret = ad5940_reg_read(dev, AD5940_REG_HSRTIACON, &rtiacon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_HSRTIACON_RTIACON_MSK, rtiacon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_bias_mv(const struct device *dev,
			      struct sensor_value *val)
{
	uint32_t dac_code;
	int ret = ad5940_reg_read(dev, AD5940_REG_LPDACDAT0, &dac_code);

	if (ret) {
		return ret;
	}

	val->val1 = (int32_t)((dac_code * AD5940_LPDAC_SPAN_MV +
			       (AD5940_LPDAC_FULL_SCALE / 2u)) /
			      AD5940_LPDAC_FULL_SCALE) + (int32_t)AD5940_LPDAC_MIN_MV;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_lpamp_rtia(const struct device *dev,
				 struct sensor_value *val)
{
	uint32_t lptiacon;
	int ret = ad5940_reg_read(dev, AD5940_REG_LPTIACON0, &lptiacon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_LPTIACON0_TIAGAIN_MSK, lptiacon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_adc_sinc2_osr(const struct device *dev,
				    struct sensor_value *val)
{
	uint32_t filtercon;
	int ret = ad5940_reg_read(dev, AD5940_REG_ADCFILTERCON, &filtercon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_ADCFILTERCON_SINC2OSR_MSK, filtercon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_adc_pga_gain(const struct device *dev,
				   struct sensor_value *val)
{
	uint32_t adccon;
	int ret = ad5940_reg_read(dev, AD5940_REG_ADCCON, &adccon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_ADCCON_GNPGA_MSK, adccon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_odr_hz(const struct device *dev,
			     struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->odr_hz;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_stats_enable(const struct device *dev,
				   struct sensor_value *val)
{
	uint32_t statscon;
	int ret = ad5940_reg_read(dev, AD5940_REG_STATSCON, &statscon);

	if (ret) {
		return ret;
	}
	if ((statscon & AD5940_STATSCON_STATSEN_MSK) == 0u) {
		val->val1 = 0;
	} else {
		val->val1 = (int32_t)FIELD_GET(AD5940_STATSCON_SAMPLENUM_MSK,
					       statscon);
	}
	val->val2 = 0;
	return 0;
}

static int ad5940_get_cv_vertex1_mv(const struct device *dev,
				    struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = data->cv.vertex1_mv;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_cv_vertex2_mv(const struct device *dev,
				    struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = data->cv.vertex2_mv;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_cv_init_mv(const struct device *dev,
				 struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = data->cv.init_mv;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_cv_scan_rate_mv_per_s(const struct device *dev,
					    struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = data->cv.scan_rate_mv_s;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_cv_cycles(const struct device *dev,
				struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->cv.cycles;
	val->val2 = 0;
	return 0;
}

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
static int ad5940_get_eis_freq_start_hz(const struct device *dev,
					struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	sensor_value_from_double(val, (double)data->sweep.start_freq_hz);
	return 0;
}

static int ad5940_get_eis_freq_stop_hz(const struct device *dev,
				       struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	sensor_value_from_double(val, (double)data->sweep.stop_freq_hz);
	return 0;
}

static int ad5940_get_eis_freq_points(const struct device *dev,
				      struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = (int32_t)data->sweep.points;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_eis_freq_log_scale(const struct device *dev,
					 struct sensor_value *val)
{
	const struct ad5940_data *data = dev->data;

	val->val1 = data->sweep.log_scale ? 1 : 0;
	val->val2 = 0;
	return 0;
}
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

static int ad5940_get_dft_num(const struct device *dev,
			      struct sensor_value *val)
{
	uint32_t dftcon;
	int ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_DFTCON_DFTNUM_MSK, dftcon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_dft_src(const struct device *dev,
			      struct sensor_value *val)
{
	uint32_t dftcon;
	int ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_DFTCON_DFTINSEL_MSK, dftcon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_hanning_win(const struct device *dev,
				  struct sensor_value *val)
{
	uint32_t dftcon;
	int ret = ad5940_reg_read(dev, AD5940_REG_DFTCON, &dftcon);

	if (ret) {
		return ret;
	}
	val->val1 = (dftcon & AD5940_DFTCON_HANNINGEN_MSK) ? 1 : 0;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_adc_sinc3_osr(const struct device *dev,
				    struct sensor_value *val)
{
	uint32_t filtercon;
	int ret = ad5940_reg_read(dev, AD5940_REG_ADCFILTERCON, &filtercon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_ADCFILTERCON_SINC3OSR_MSK, filtercon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_excit_buf_gain(const struct device *dev,
				     struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);

	if (ret) {
		return ret;
	}
	val->val1 = (hsdaccon & AD5940_HSDACCON_ATTENEN_MSK) ? 1 : 0;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_hsdac_gain(const struct device *dev,
				 struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);

	if (ret) {
		return ret;
	}
	val->val1 = (hsdaccon & AD5940_HSDACCON_GAINX5_MSK) ? 1 : 0;
	val->val2 = 0;
	return 0;
}

static int ad5940_get_hsdac_update_rate(const struct device *dev,
					struct sensor_value *val)
{
	uint32_t hsdaccon;
	int ret = ad5940_reg_read(dev, AD5940_REG_HSDACCON, &hsdaccon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_HSDACCON_Rate_MSK, hsdaccon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_ctia_sel(const struct device *dev,
			       struct sensor_value *val)
{
	uint32_t rtiacon;
	int ret = ad5940_reg_read(dev, AD5940_REG_HSRTIACON, &rtiacon);

	if (ret) {
		return ret;
	}
	val->val1 = (int32_t)FIELD_GET(AD5940_HSRTIACON_CTIACON_MSK, rtiacon);
	val->val2 = 0;
	return 0;
}

static int ad5940_get_power_mode(const struct device *dev,
				 struct sensor_value *val)
{
	uint32_t pmbw;
	int ret = ad5940_reg_read(dev, AD5940_REG_PMBW, &pmbw);

	if (ret) {
		return ret;
	}

	val->val1 = (pmbw & AD5940_PMBW_SYSHS_MSK) ? 1 : 0;
	val->val2 = 0;
	return 0;
}

static int ad5940_set_sw_reset(const struct device *dev,
			       const struct sensor_value *val)
{
	ARG_UNUSED(val);
	int ret = ad5940_sw_reset(dev);

	if (ret) {
		return ret;
	}

	return ad5940_sys_init_sequence(dev);
}

static int ad5940_set_mode(const struct device *dev,
			   const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;
	uint32_t wuptmr;
	uint32_t cnt;
	int ret;

	if (val->val1 < 0 || val->val1 > (int32_t)AD5940_MODE_ADC) {
		return -EINVAL;
	}
	data->mode = (enum ad5940_mode)val->val1;

	ret = ad5940_reg_read(dev, AD5940_REG_WUPTMRCON, &wuptmr);
	if (ret) {
		return ret;
	}
	ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON,
			       wuptmr & ~AD5940_TMRCON_WUPTEN_MSK);
	if (ret) {
		return ret;
	}

	switch (data->mode) {
	case AD5940_MODE_EIS:
#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
		data->sweep_enabled = (data->sweep.points > 1u);
		if (data->sweep_enabled) {
			data->sweep_current_idx = 0u;
			data->eis.freq_hz = data->sweep.start_freq_hz;
		}
#endif
		ret = ad5940_eis_mode_config(dev);
		if (ret) {
			return ret;
		}
		ret = ad5940_sequencer_config(dev);
		if (ret) {
			return ret;
		}
		ad5940_fifo_flush(dev);
		cnt = 0u;
		ad5940_reg_read(dev, AD5940_REG_FIFOCNTSTA, &cnt);
		LOG_DBG("set_mode EIS done: FIFOCNT=%u",
			(unsigned int)FIELD_GET(
				AD5940_FIFOCNTSTA_DATAFIFOCNTSTA_MSK,
				cnt));
		break;
	case AD5940_MODE_AMPEROMETRY:
		ret = ad5940_amper_mode_config(dev);
		break;
	case AD5940_MODE_ADC:
		ret = ad5940_fifo_config(dev, AD5940_FIFOSRC_ADC);
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

/**
 * @brief Zephyr sensor_attr_set() implementation.
 *
 * Dispatches SENSOR_ATTR_AD5940_* attributes to their setter functions.
 * Holds data->lock for the duration of the call.
 *
 * @param dev  Device pointer
 * @param chan Sensor channel (unused; attrs are device-wide)
 * @param attr Attribute to set (SENSOR_ATTR_AD5940_*)
 * @param val  Value to set
 * @return 0 on success, -ENOTSUP for unknown attributes, negative errno otherwise
 */
static int ad5940_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct ad5940_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_AD5940_SW_RESET:
		ret = ad5940_set_sw_reset(dev, val);
		break;

	case SENSOR_ATTR_AD5940_MODE:
		ret = ad5940_set_mode(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_FREQ_HZ:
		ret = ad5940_set_eis_freq_hz(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_AMPLITUDE_MVPP:
		ret = ad5940_set_eis_amplitude_mvpp(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_SETTLING_CYCLES:
		ret = ad5940_set_eis_settling_cycles(dev, val);
		break;

	case SENSOR_ATTR_AD5940_RTIA_SEL:
		ret = ad5940_set_rtia_sel(dev, val);
		break;

	case SENSOR_ATTR_AD5940_BIAS_MV:
		ret = ad5940_set_bias_mv(dev, val);
		break;

	case SENSOR_ATTR_AD5940_LPAMP_RTIA:
		ret = ad5940_set_lpamp_rtia(dev, val);
		break;

	case SENSOR_ATTR_AD5940_ADC_SINC2_OSR:
		ret = ad5940_set_adc_sinc2_osr(dev, val);
		break;

	case SENSOR_ATTR_AD5940_ADC_PGA_GAIN:
		ret = ad5940_set_adc_pga_gain(dev, val);
		break;

	case SENSOR_ATTR_AD5940_ODR_HZ:
		ret = ad5940_set_odr_hz(dev, val);
		break;

	case SENSOR_ATTR_AD5940_STATS_ENABLE:
		ret = ad5940_set_stats_enable(dev, val);
		break;

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
	case SENSOR_ATTR_AD5940_EIS_FREQ_START_HZ:
		ret = ad5940_set_eis_freq_start_hz(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_FREQ_STOP_HZ:
		ret = ad5940_set_eis_freq_stop_hz(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_FREQ_POINTS:
		ret = ad5940_set_eis_freq_points(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EIS_FREQ_LOG_SCALE:
		ret = ad5940_set_eis_freq_log_scale(dev, val);
		break;
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

	case SENSOR_ATTR_AD5940_CV_VERTEX1_MV:
		ret = ad5940_set_cv_vertex1_mv(dev, val);
		break;

	case SENSOR_ATTR_AD5940_CV_VERTEX2_MV:
		ret = ad5940_set_cv_vertex2_mv(dev, val);
		break;

	case SENSOR_ATTR_AD5940_CV_INIT_MV:
		ret = ad5940_set_cv_init_mv(dev, val);
		break;

	case SENSOR_ATTR_AD5940_CV_SCAN_RATE_MV_PER_S:
		ret = ad5940_set_cv_scan_rate_mv_per_s(dev, val);
		break;

	case SENSOR_ATTR_AD5940_CV_CYCLES:
		ret = ad5940_set_cv_cycles(dev, val);
		break;

	case SENSOR_ATTR_AD5940_DFT_NUM:
		ret = ad5940_set_dft_num(dev, val);
		break;

	case SENSOR_ATTR_AD5940_DFT_SRC:
		ret = ad5940_set_dft_src(dev, val);
		break;

	case SENSOR_ATTR_AD5940_HANNING_WIN:
		ret = ad5940_set_hanning_win(dev, val);
		break;

	case SENSOR_ATTR_AD5940_ADC_SINC3_OSR:
		ret = ad5940_set_adc_sinc3_osr(dev, val);
		break;

	case SENSOR_ATTR_AD5940_EXCIT_BUF_GAIN:
		ret = ad5940_set_excit_buf_gain(dev, val);
		break;

	case SENSOR_ATTR_AD5940_HSDAC_GAIN:
		ret = ad5940_set_hsdac_gain(dev, val);
		break;

	case SENSOR_ATTR_AD5940_HSDAC_UPDATE_RATE:
		ret = ad5940_set_hsdac_update_rate(dev, val);
		break;

	case SENSOR_ATTR_AD5940_CTIA_SEL:
		ret = ad5940_set_ctia_sel(dev, val);
		break;

	case SENSOR_ATTR_AD5940_POWER_MODE:
		ret = ad5940_set_power_mode(dev, val);
		break;

	case SENSOR_ATTR_AD5940_RECAL:
		ret = ad5940_rtia_calibrate(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

/**
 * @brief Zephyr sensor_attr_get() implementation.
 *
 * Dispatches SENSOR_ATTR_AD5940_* attributes to their getter functions.
 * Read-only attributes read from hardware; others return cached driver state.
 *
 * @param dev  Device pointer
 * @param chan Sensor channel (unused; attrs are device-wide)
 * @param attr Attribute to get (SENSOR_ATTR_AD5940_*)
 * @param val  Output value
 * @return 0 on success, -ENOTSUP for unknown attributes, negative errno otherwise
 */
static int ad5940_attr_get(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   struct sensor_value *val)
{
	switch ((int)attr) {
	case SENSOR_ATTR_AD5940_CHIP_ID:
		return ad5940_get_chip_id(dev, val);

	case SENSOR_ATTR_AD5940_ADI_ID:
		return ad5940_get_adi_id(dev, val);

	case SENSOR_ATTR_AD5940_RTIA_CAL_MAGNITUDE:
		return ad5940_get_rtia_cal_magnitude(dev, val);

	case SENSOR_ATTR_AD5940_RTIA_CAL_PHASE:
		return ad5940_get_rtia_cal_phase(dev, val);

	case SENSOR_ATTR_AD5940_FIFO_COUNT:
		return ad5940_get_fifo_count(dev, val);

	case SENSOR_ATTR_AD5940_EIS_LAST_FREQ_HZ:
		return ad5940_get_eis_last_freq_hz(dev, val);

	case SENSOR_ATTR_AD5940_MODE:
		return ad5940_get_mode(dev, val);

	case SENSOR_ATTR_AD5940_EIS_FREQ_HZ:
		return ad5940_get_eis_freq_hz(dev, val);

	case SENSOR_ATTR_AD5940_EIS_AMPLITUDE_MVPP:
		return ad5940_get_eis_amplitude_mvpp(dev, val);

	case SENSOR_ATTR_AD5940_EIS_SETTLING_CYCLES:
		return ad5940_get_eis_settling_cycles(dev, val);

	case SENSOR_ATTR_AD5940_RTIA_SEL:
		return ad5940_get_rtia_sel(dev, val);

	case SENSOR_ATTR_AD5940_BIAS_MV:
		return ad5940_get_bias_mv(dev, val);

	case SENSOR_ATTR_AD5940_LPAMP_RTIA:
		return ad5940_get_lpamp_rtia(dev, val);

	case SENSOR_ATTR_AD5940_ADC_SINC2_OSR:
		return ad5940_get_adc_sinc2_osr(dev, val);

	case SENSOR_ATTR_AD5940_ADC_PGA_GAIN:
		return ad5940_get_adc_pga_gain(dev, val);

	case SENSOR_ATTR_AD5940_ODR_HZ:
		return ad5940_get_odr_hz(dev, val);

	case SENSOR_ATTR_AD5940_STATS_ENABLE:
		return ad5940_get_stats_enable(dev, val);

	case SENSOR_ATTR_AD5940_CV_VERTEX1_MV:
		return ad5940_get_cv_vertex1_mv(dev, val);

	case SENSOR_ATTR_AD5940_CV_VERTEX2_MV:
		return ad5940_get_cv_vertex2_mv(dev, val);

	case SENSOR_ATTR_AD5940_CV_INIT_MV:
		return ad5940_get_cv_init_mv(dev, val);

	case SENSOR_ATTR_AD5940_CV_SCAN_RATE_MV_PER_S:
		return ad5940_get_cv_scan_rate_mv_per_s(dev, val);

	case SENSOR_ATTR_AD5940_CV_CYCLES:
		return ad5940_get_cv_cycles(dev, val);

#ifdef CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE
	case SENSOR_ATTR_AD5940_EIS_FREQ_START_HZ:
		return ad5940_get_eis_freq_start_hz(dev, val);

	case SENSOR_ATTR_AD5940_EIS_FREQ_STOP_HZ:
		return ad5940_get_eis_freq_stop_hz(dev, val);

	case SENSOR_ATTR_AD5940_EIS_FREQ_POINTS:
		return ad5940_get_eis_freq_points(dev, val);

	case SENSOR_ATTR_AD5940_EIS_FREQ_LOG_SCALE:
		return ad5940_get_eis_freq_log_scale(dev, val);
#endif /* CONFIG_AD5940_FREQUENCY_SWEEP_ENABLE */

	case SENSOR_ATTR_AD5940_DFT_NUM:
		return ad5940_get_dft_num(dev, val);

	case SENSOR_ATTR_AD5940_DFT_SRC:
		return ad5940_get_dft_src(dev, val);

	case SENSOR_ATTR_AD5940_HANNING_WIN:
		return ad5940_get_hanning_win(dev, val);

	case SENSOR_ATTR_AD5940_ADC_SINC3_OSR:
		return ad5940_get_adc_sinc3_osr(dev, val);

	case SENSOR_ATTR_AD5940_EXCIT_BUF_GAIN:
		return ad5940_get_excit_buf_gain(dev, val);

	case SENSOR_ATTR_AD5940_HSDAC_GAIN:
		return ad5940_get_hsdac_gain(dev, val);

	case SENSOR_ATTR_AD5940_HSDAC_UPDATE_RATE:
		return ad5940_get_hsdac_update_rate(dev, val);

	case SENSOR_ATTR_AD5940_CTIA_SEL:
		return ad5940_get_ctia_sel(dev, val);

	case SENSOR_ATTR_AD5940_POWER_MODE:
		return ad5940_get_power_mode(dev, val);

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Zephyr PM device action callback.
 *
 * Handles SUSPEND (enter sleep), RESUME (wakeup + re-enable timer), and
 * TURN_OFF (hibernate). Other actions return -ENOTSUP.
 *
 * @param dev    Device pointer
 * @param action PM action to perform
 * @return 0 on success, -ENOTSUP for unsupported actions, negative errno otherwise
 */
static int __maybe_unused ad5940_pm_action(const struct device *dev,
					   enum pm_device_action action)
{
	int ret;
	uint32_t wuptmr;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ad5940_enter_sleep(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = ad5940_wakeup(dev);
		if (ret) {
			break;
		}
		ret = ad5940_reg_read(dev, AD5940_REG_WUPTMRCON, &wuptmr);
		if (ret) {
			break;
		}
		ret = ad5940_reg_write(dev, AD5940_REG_WUPTMRCON,
				       wuptmr | AD5940_TMRCON_WUPTEN_MSK);
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		ret = ad5940_enter_hibernate(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static DEVICE_API(sensor, ad5940_driver_api) = {
	.sample_fetch = ad5940_sample_fetch,
	.channel_get  = ad5940_channel_get,
	.attr_set     = ad5940_attr_set,
	.attr_get     = ad5940_attr_get,
#ifdef CONFIG_AD5940_TRIGGER
	.trigger_set  = ad5940_trigger_set,
#endif
#ifdef CONFIG_AD5940_STREAM
	.get_decoder = ad5940_get_decoder,
#endif
#ifdef CONFIG_AD5940_STREAM
	.submit      = ad5940_submit,
#endif
};

#define AD5940_INIT(inst)								\
	static struct ad5940_data ad5940_data_##inst;					\
											\
	IF_ENABLED(CONFIG_AD5940_STREAM,						\
		(SPI_DT_IODEV_DEFINE(ad5940_spi_iodev_##inst,				\
			DT_DRV_INST(inst),						\
			SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0U);))			\
											\
	static const struct ad5940_config ad5940_config_##inst = {			\
		.spi        = SPI_DT_SPEC_INST_GET(inst,				\
				SPI_WORD_SET(AD5940_SPI_WORD_BITS) |			\
				SPI_TRANSFER_MSB),					\
		IF_ENABLED(CONFIG_AD5940_STREAM,					\
			(.spi_iodev = &ad5940_spi_iodev_##inst,))			\
		.int_gpio   = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),		\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),		\
		.rcal_ohms          = DT_INST_PROP_OR(inst, rcal_ohms,			\
						      AD5940_DEF_RCAL_OHMS),		\
		.clock_source       = DT_INST_PROP_OR(inst, clksel_sysclksel,	\
						      AD5940_CLKSEL_HFOSC),		\
		.hfosc_32mhz        = DT_INST_PROP_OR(inst, hfosc_32mhz_mode, false),		\
		.int_ad5940_pin     = DT_INST_PROP_OR(inst, int_ad5940_pin,		\
						      AD5940_DEF_INT_PIN),		\
		.int_ad5940_pull_en = DT_INST_PROP(inst, int_ad5940_pull_en),			\
		.def_eis_freq_hz         = DT_INST_PROP_OR(inst, eis_frequency_hz,	\
							   AD5940_DEF_EIS_FREQ_HZ),	\
		.def_eis_amplitude_mvpp  = DT_INST_PROP_OR(inst, eis_amplitude_mvpp,	\
							   AD5940_DEF_EIS_AMPLITUDE_MVPP), \
		.def_eis_rtia_sel        = DT_INST_PROP_OR(inst, hsrtiacon_rtiacon,	\
							   AD5940_DEF_EIS_RTIA_SEL),	\
		.def_eis_settling_cycles = DT_INST_PROP_OR(inst, eis_settling_cycles,	\
							   AD5940_DEF_EIS_SETTLING_CYCLES), \
		.def_eis_dft_num         = DT_INST_PROP_OR(inst, dftcon_dftnum,		\
							   AD5940_DEF_EIS_DFT_NUM),	\
		.def_eis_dft_src         = DT_INST_PROP_OR(inst, dftcon_dftinsel,	\
							   AD5940_DEF_EIS_DFT_SRC),	\
		.def_eis_hanning_win     = DT_INST_PROP(inst, dftcon_hanningen),		\
		.def_eis_sinc3_osr       = DT_INST_PROP_OR(inst, adcfiltercon_sinc3osr,	\
							   AD5940_DEF_EIS_SINC3_OSR),	\
		.def_eis_excit_buf_gain  = DT_INST_PROP_OR(inst, hsdaccon_inampgnmde,	\
							   AD5940_DEF_EIS_EXCIT_BUF_GAIN), \
		.def_eis_hsdac_gain      = DT_INST_PROP_OR(inst, hsdaccon_attenen,	\
							   AD5940_DEF_EIS_HSDAC_GAIN),	\
		.def_eis_hsdac_update_rate = DT_INST_PROP_OR(inst, hsdaccon_rate,	\
							     AD5940_DEF_EIS_HSDAC_RATE), \
		.def_eis_ctia_sel        = DT_INST_PROP_OR(inst, hsrtiacon_ctiacon,	\
							   AD5940_DEF_EIS_CTIA_SEL),	\
		.def_eis_power_mode      = DT_INST_PROP_OR(inst, eis_power_mode,	\
							   AD5940_DEF_EIS_POWER_MODE),	\
		.def_bias_mv            = DT_INST_PROP_OR(inst, amper_bias_mv,		\
							  AD5940_DEF_AMPER_BIAS_MV),	\
		.def_lpamp_rtia         = DT_INST_PROP_OR(inst, amper_lpamp_rtia,	\
							  AD5940_DEF_AMPER_LPAMP_RTIA),	\
		.def_sweep_start_hz     = DT_INST_PROP_OR(inst, sweep_start_hz,		\
							  AD5940_DEF_SWEEP_START_HZ),	\
		.def_sweep_stop_hz      = DT_INST_PROP_OR(inst, sweep_stop_hz,		\
							  AD5940_DEF_SWEEP_STOP_HZ),	\
		.def_sweep_points       = DT_INST_PROP_OR(inst, sweep_points,		\
							  AD5940_DEF_SWEEP_POINTS),	\
		.def_sweep_log_scale    = DT_INST_PROP(inst, sweep_log_scale),			\
		.sw_dsw       = DT_INST_PROP_OR(inst, swmat_dsw, 0x10),			\
		.sw_psw       = DT_INST_PROP_OR(inst, swmat_psw, 0x400),			\
		.sw_nsw       = DT_INST_PROP_OR(inst, swmat_nsw, 0x2),			\
		.sw_tsw       = DT_INST_PROP_OR(inst, swmat_tsw, 0x102),			\
		.sw_swmux     = DT_INST_PROP_OR(inst, swmat_swmux, 0x8),			\
		.adc_mux_v_p  = DT_INST_PROP_OR(inst, adccon_muxselp_v, 0x6),		\
		.adc_mux_v_n  = DT_INST_PROP_OR(inst, adccon_muxseln_v, 0x7),		\
		.adc_mux_i_p  = DT_INST_PROP_OR(inst, adccon_muxselp_i, 0x1),		\
		.adc_mux_i_n  = DT_INST_PROP_OR(inst, adccon_muxseln_i, 0x1),		\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, ad5940_pm_action);				\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,						\
				     ad5940_init,					\
				     PM_DEVICE_DT_INST_GET(inst),			\
				     &ad5940_data_##inst,				\
				     &ad5940_config_##inst,				\
				     POST_KERNEL,					\
				     CONFIG_SENSOR_INIT_PRIORITY,			\
				     &ad5940_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AD5940_INIT)

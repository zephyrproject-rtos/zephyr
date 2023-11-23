/* Copyright 2013 EVOS Energy Pty Ltd
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#define DT_DRV_COMPAT microchip_mcp3914

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
/* Park the driver code in a fast zone on the ESP SOCs */
#ifdef CONFIG_SOC_FAMILY_ESP32
	#include <esp_attr.h>
#else
	#define IRAM_ATTR
#endif

LOG_MODULE_REGISTER(adc_mcp3914, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#define ADC_CONTEXT_ENABLE_ON_COMPLETE
#include <../drivers/adc/adc_context.h>

/*
 * Driver private defines for device registers
 */

/*
 * SPI Control byte - sent at start of each transaction
 */
#define MCP3914_ADDRESS                      (0x40)
#define MCP3914_READ                         (0x01)
#define MCP3914_WRITE                        (0x00)
#define _SPI_CONTROL_BYTE_READ               (MCP3914_ADDRESS | MCP3914_READ)
#define _SPI_CONTROL_BYTE_WRITE              (MCP3914_ADDRESS | MCP3914_WRITE)
#define _SPI_REG_ADDR_BIT_SHR(addr)          (addr << 1)
#define MCP3914_READ_START_REG_CONTROL(addr) (_SPI_CONTROL_BYTE_READ | _SPI_REG_ADDR_BIT_SHR(addr))
#define MCP3914_WRITE_START_REG_CONTROL(addr)                                                      \
	(_SPI_CONTROL_BYTE_WRITE | _SPI_REG_ADDR_BIT_SHR(addr))

#define MCP3914_REGISTER_SIZE              (3U) /* all registers excpept MODS register */
#define MCP3914_NUMBER_OF_CHANNELS         (8U)
#define MCP3914_INTERNAL_VOLTAGE_REFERENCE 1200 /* mV */
#define MCP3914_UNLOCK_PASSWORD            (0xA5)
/*
 * MCP3914 register address defines
 * Driver converts registers to/from cpu native 32 bit value
 * on reads and writes. Bit field setters/masks can
 * assume a uint32 value.
 */

/* Data registers - read only*/
#define CH0_DATA_REG_DEV_ADDR (0x00)
#define CH1_DATA_REG_DEV_ADDR (0x01)
#define CH2_DATA_REG_DEV_ADDR (0x02)
#define CH3_DATA_REG_DEV_ADDR (0x03)
#define CH4_DATA_REG_DEV_ADDR (0x04)
#define CH5_DATA_REG_DEV_ADDR (0x05)
#define CH6_DATA_REG_DEV_ADDR (0x06)
#define CH7_DATA_REG_DEV_ADDR (0x07)

#define ALL_CHANNELS_BUFFER_LENGTH (MCP3914_REGISTER_SIZE * MCP3914_NUMBER_OF_CHANNELS)

/* Weird only 32 bit register....
 * also probably useless, can't see why you would either read
 * or write it, but that's just me....
 * SD Modulator values
 */
#define DS_MODS_OP_REG_DEV_ADDR (0x08)

/* Config registers */
#define PH_DLY_C0_REG_DEV_ADDR (0x09)
#define PH_DLY_C1_REG_DEV_ADDR (0x0A)
#define PGA_GAIN_REG_DEV_ADDR  (0x0B)
#define STATUSCOM_REG_DEV_ADDR (0x0C)
#define CONFIG0_REG_DEV_ADDR   (0x0D)
#define CONFIG1_REG_DEV_ADDR   (0x0E)

/* Offset/Gain cal registers */
/* CH0 */
#define CH0_OFFCAL_REG_DEV_ADDR  (0x0F)
#define CH0_GAINCAL_REG_DEV_ADDR (0x10)
/* CH1 */
#define CH1_OFFCAL_REG_DEV_ADDR  (0x11)
#define CH1_GAINCAL_REG_DEV_ADDR (0x12)
/* CH2 */
#define CH2_OFFCAL_REG_DEV_ADDR  (0x13)
#define CH2_GAINCAL_REG_DEV_ADDR (0x14)
/* CH3 */
#define CH3_OFFCAL_REG_DEV_ADDR  (0x15)
#define CH3_GAINCAL_REG_DEV_ADDR (0x16)
/* CH4 */
#define CH4_OFFCAL_REG_DEV_ADDR  (0x17)
#define CH4_GAINCAL_REG_DEV_ADDR (0x18)
/* CH5 */
#define CH5_OFFCAL_REG_DEV_ADDR  (0x19)
#define CH5_GAINCAL_REG_DEV_ADDR (0x1A)
/* CH6 */
#define CH6_OFFCAL_REG_DEV_ADDR  (0x1B)
#define CH6_GAINCAL_REG_DEV_ADDR (0x1C)
/* CH7 */
#define CH7_OFFCAL_REG_DEV_ADDR  (0x1D)
#define CH7_GAINCAL_REG_DEV_ADDR (0x1E)

/* LOCK BYTE + CRC Register*/
#define LOCK_CRC_REG_DEV_ADDR (0x1F)

#define MCP3914_FIRST_REG_ADDR CH0_DATA_REG_DEV_ADDR
#define MCP3914_LAST_REG_ADDR  LOCK_CRC_REG_DEV_ADDR

/* Register fields values */

/***************
 * The only registers defined here are ones that
 * we are currently using.
 * TODO: Define all register fields available
 ****************/

/******** PGA_GAIN Register ************/
/* Gain register values */
#define PGA_CH_GAIN_1  0b000
#define PGA_CH_GAIN_2  0b001
#define PGA_CH_GAIN_4  0b010
#define PGA_CH_GAIN_8  0b011
#define PGA_CH_GAIN_16 0b100
#define PGA_CH_GAIN_32 0b101

#define PGA_CH_GAIN_CLR(reg, ch)     (reg & ~GENMASK(((ch * 3) + 3), (ch * 3)))
#define PGA_CH_GAIN_FOR_CH(gain, ch) FIELD_PREP(GENMASK((ch * 3) + 3), (ch * 3), gain)

/******** STATUSCOM Register ************/
/* READ control setting */
#define MCP3914_READ_CONT_CLR(reg) (reg & ~GENMASK(23, 22))

#define MCP3914_READ_CONT_ONE   (0)
#define MCP3914_READ_CONT_GROUP FIELD_PREP(GENMASK(23, 22), 0x1)
#define MCP3914_READ_CONT_TYPE  FIELD_PREP(GENMASK(23, 22), 0x2)
#define MCP3914_READ_CONT_ALL   FIELD_PREP(GENMASK(23, 22), 0x3)

/* WRITE control setting */
#define MCP3914_WRITE_CONT_CLR(reg) (reg & ~BIT(21))

#define MCP3914_WRITE_CONT_ONE  (0)
#define MCP3914_WRITE_CONT_LOOP BIT(21)

/* DREADY Idle setting */
#define MCP3914_DR_IDLE_CLR(reg) (reg & ~BIT(20))

#define MCP3914_DR_IDLE_HIZ  (0)
#define MCP3914_DR_IDLE_HIGH BIT(20)

/******** CONFIG0 Register ************/
/* AMCLK/MCLK Prescale setting */
#define MCP3914_AMCLK_PRESCALE_CLR(reg) (reg & ~GENMASK(17, 16))

#define MCP3914_AMCLK_PRESCALE_NONE FIELD_PREP(GENMASK(17, 16), 0x0)
#define MCP3914_AMCLK_PRESCALE_DIV2 FIELD_PREP(GENMASK(17, 16), 0x1)
#define MCP3914_AMCLK_PRESCALE_DIV4 FIELD_PREP(GENMASK(17, 16), 0x2)
#define MCP3914_AMCLK_PRESCALE_DIV8 FIELD_PREP(GENMASK(17, 16), 0x3)

/* Oversampling Ratio values */
#define MCP3914_OSR_CLR(reg) (reg & ~GENMASK(15, 13))

#define MCP3914_OSR_32   (0x0)
#define MCP3914_OSR_64   FIELD_PREP(GENMASK(15, 13), 0b001)
#define MCP3914_OSR_128  FIELD_PREP(GENMASK(15, 13), 0b010)
#define MCP3914_OSR_256  FIELD_PREP(GENMASK(15, 13), 0b011)
#define MCP3914_OSR_512  FIELD_PREP(GENMASK(15, 13), 0b100)
#define MCP3914_OSR_1024 FIELD_PREP(GENMASK(15, 13), 0b101)
#define MCP3914_OSR_2048 FIELD_PREP(GENMASK(15, 13), 0b110)
#define MCP3914_OSR_4096 FIELD_PREP(GENMASK(15, 13), 0b111)

/******** CONFIG1 Register ************/
/* VREF select setting */
#define MCP3914_VREF_SEL_CLR(reg) (reg & ~BIT(7))

#define MCP3914_VREF_SEL_INT (0)
#define MCP3914_VREF_SEL_EXT BIT(7)

/* CLK source select setting */
#define MCP3914_CLK_SRC_CLR(reg) (reg & ~BIT(6))

#define MCP3914_CLK_SRC_XTAL (0)
#define MCP3914_CLK_SRC_EXT  BIT(7)

/* Channel RESET */
#define MCP3914_CH_RESET_ALL(reg) (reg | GENMASK(23, 16))
#define MCP3914_CH_RESET_CLR(reg) (reg & ~GENMASK(23, 16))

#define MCP3914_CH_RESET_CH_MSK(ch_msk) FIELD_PREP(GENMASK(23, 16), ch_msk)


struct mcp3914_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec interrupt;
	uint8_t channels;
	uint32_t presc;
	uint32_t osr;
	uint32_t clksrc;
};

struct mcp3914_data {
	struct gpio_callback callback_data_ready;
	struct adc_context ctx;
	const struct device *dev;
	int32_t *buffer;
	uint32_t *ts_buffer;
	struct k_thread thread;
	struct k_sem read_data_signal;
	struct k_sem data_ready_signal;
	int32_t timestamp;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MCP3914_ACQUISITION_THREAD_STACK_SIZE);

	/* If adc debugging add a counter for interrupts */
#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
	uint16_t drdy_counter;
	uint32_t last_reset_tick;
#endif
};

#define SIZE_OF_CH_DATA (sizeof(*(((struct mcp3914_data *)0)->buffer)))

#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
#define DREADY_ROLLOVER 10000
#endif

IRAM_ATTR void mcp3914_data_ready_handler(const struct device *dev, struct gpio_callback *gpio_cb,
					  uint32_t pins)
{

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);
	struct mcp3914_data *data = CONTAINER_OF(gpio_cb, struct mcp3914_data, callback_data_ready);

	/* Signal whoever needs to know */
	data->timestamp = sys_clock_tick_get_32();
	k_sem_give(&data->data_ready_signal);

#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
	if (data->drdy_counter >= DREADY_ROLLOVER) {
		uint32_t t = data->timestamp;

		if (data->last_reset_tick < t) {
			t -= data->last_reset_tick;
		} else {
			t += UINT32_MAX - data->last_reset_tick;
		}
		LOG_DBG("int_h %s (%u)", data->dev->name, k_ticks_to_us_ceil32(t));
		data->drdy_counter = 0;
		data->last_reset_tick = sys_clock_tick_get_32();
	}
	data->drdy_counter++;
#endif
}

static int mcp3914_spi_transceive(const struct device *dev, const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	struct mcp3914_config *cfg = (struct mcp3914_config *)dev->config;

	return spi_transceive_dt(&cfg->bus, tx_bufs, rx_bufs);
}

/*
 * Read from the device registers from the start address to
 * the end address.
 * Allocates buf_len ints and stores result in buf.
 * buf_len pointer can be NULL
 * It is up to the caller to free the buffer when done.
 * Returns ENOMEM if allocation fails, error from SPI
 * transaction or 0 if success.
 */
static int mcp3914_read_regs(const struct device *dev, const uint8_t start_reg_addr,
			     const uint8_t end_reg_addr, uint32_t **buf, uint8_t *buf_len)
{
	/* See if we can malloc space for the returned data before we
	 * waste time anywhere else
	 */
	uint8_t buf_cnt = (1 + end_reg_addr - start_reg_addr);

	*buf = k_calloc(buf_cnt, sizeof(uint32_t));
	if (*buf == NULL) {
		LOG_DBG("%s alloc buffer failed", dev->name);
		return ENOMEM;
	}

	uint8_t byte_len = buf_cnt * MCP3914_REGISTER_SIZE + 1; /* One more for control byte */

	/* Add one byte if we are including the ONLY 32 bit register.... */
	if (start_reg_addr <= DS_MODS_OP_REG_DEV_ADDR && end_reg_addr >= DS_MODS_OP_REG_DEV_ADDR) {
		byte_len += 1;
	}

	/* The byte array for returned data, align to int32 boundaries */
	uint8_t *buffer;

	size_t align_len = byte_len + (sizeof(uint32_t) - (byte_len % sizeof(uint32_t)));

	buffer = k_calloc(align_len, sizeof(uint8_t));
	if (buffer == NULL) {
		LOG_DBG("%s (rd) alloc bytes failed", dev->name);
		return ENOMEM;
	}

	/* We have space for the read, so setup SPI buffers
	 * The control byte goes first, it is all the device
	 * will listen to
	 */
	buffer[0] = MCP3914_READ_START_REG_CONTROL(start_reg_addr);
	struct spi_buf tx_buf[1] = {{.buf = buffer, .len = byte_len}};
	struct spi_buf rx_buf[1] = {{.buf = buffer, .len = byte_len}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	int err = mcp3914_spi_transceive(dev, &tx, &rx);

	if (err) {
		LOG_DBG("%s spi read failed %s", dev->name, strerror(err));
		k_free(buffer);
		k_free(*buf);
		*buf = NULL;
		return err;
	}

	/* We have a decent read, convert read bytes into return buffer */
	uint32_t *pval = *buf;

	for (uint8_t i = 1, j = start_reg_addr; i < byte_len; i += MCP3914_REGISTER_SIZE, j++) {
		if (j == DS_MODS_OP_REG_DEV_ADDR) { /* This reg occupies 4 bytes!! */
			*pval = sys_get_be32(&buffer[i++]);
		} else {
			*pval = sys_get_be24(&buffer[i]);
		}
		pval++;
	}

	k_free(buffer);
	if (buf_len != NULL) {
		*buf_len = buf_cnt;
	}
	/* Return no errors */
	return 0;
}

/*
 * Write to the device registers from the start address to
 * the end address.
 */
static int mcp3914_write_regs(const struct device *dev, const uint8_t start_reg_addr,
			      const uint8_t end_reg_addr, uint32_t *data)
{
	/* Most (except one) are 24bit */
	uint8_t byte_len = (1 + end_reg_addr - start_reg_addr) * MCP3914_REGISTER_SIZE;

	/* Add one byte if we are including the ONLY 32 bit register.... */
	if (start_reg_addr <= DS_MODS_OP_REG_DEV_ADDR && end_reg_addr >= DS_MODS_OP_REG_DEV_ADDR) {
		byte_len += 1;
	}

	/* Bytes out buffer allowing for control byte */
	byte_len++;
	size_t align_len = (byte_len) + (sizeof(uint32_t) - (byte_len % sizeof(uint32_t)));

	uint8_t *buf = k_calloc(align_len, sizeof(uint8_t));

	if (buf == NULL) {
		LOG_DBG("%s (wr) alloc bytes failed", dev->name);
		return ENOMEM;
	}
	buf[0] = MCP3914_WRITE_START_REG_CONTROL(start_reg_addr);

	/* fill the buffer */
	for (uint8_t i = 1, j = start_reg_addr; i < byte_len; i = i + MCP3914_REGISTER_SIZE, j++) {
		if (j == DS_MODS_OP_REG_DEV_ADDR) { /* This reg is 32 bit */
			sys_put_be32(data[j - start_reg_addr], &buf[i++]);
		} else {
			sys_put_be24(data[j - start_reg_addr], &buf[i]);
		}
	}

	struct spi_buf tx_buf[1] = {{.buf = buf, .len = byte_len}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	int err = mcp3914_spi_transceive(dev, &tx, NULL);

	if (err) {
		LOG_DBG("%s spi read failed %s", dev->name, strerror(err));
		k_free(buf);
		return err;
	}

	k_free(buf);
	/* Return no errors */
	return 0;
}

static int mcp3914_set_osr_presc(const struct device *dev)
{
	const struct mcp3914_config *config = dev->config;

	uint32_t *C0_reg;
	uint8_t reg_len;

	if (mcp3914_read_regs(dev, CONFIG0_REG_DEV_ADDR, CONFIG0_REG_DEV_ADDR, &C0_reg, &reg_len)) {
		return ENODATA;
	}

	/* Clear and set prescale bits */
	*C0_reg = MCP3914_AMCLK_PRESCALE_CLR(*C0_reg);
	*C0_reg |= config->presc;

	/* Clear and set hw osr bits */
	*C0_reg = MCP3914_OSR_CLR(*C0_reg);
	*C0_reg |= config->osr;

	/* Write out */
	if (mcp3914_write_regs(dev, CONFIG0_REG_DEV_ADDR, CONFIG0_REG_DEV_ADDR, C0_reg)) {
		k_free(C0_reg);
		return EBADMSG;
	}

	/* Happy days! */
	k_free(C0_reg);
	return 0;
}

/* Sets the channels with bits set in ch_mask to reset mode */
static int mcp3914_set_ch_reset(const struct device *dev, uint8_t ch_mask)
{
	uint32_t *C1_reg;

	if (mcp3914_read_regs(dev, CONFIG1_REG_DEV_ADDR, CONFIG1_REG_DEV_ADDR, &C1_reg, NULL)) {
		return ENODATA;
	}

	/* Clear the current reset bits and set the new ones */
	*C1_reg = MCP3914_CH_RESET_CLR(*C1_reg);
	*C1_reg |= MCP3914_CH_RESET_CH_MSK(ch_mask);

	if (mcp3914_write_regs(dev, CONFIG1_REG_DEV_ADDR, CONFIG1_REG_DEV_ADDR, C1_reg)) {
		k_free(C1_reg);
		return EBADMSG;
	}

	k_free(C1_reg);
	return 0;
}

static int mcp3914_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct mcp3914_config *config = dev->config;

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	/*
	 * #TODO: Support the rest of the PGA gains the chip supports...
	 * && channel_cfg->gain != ADC_GAIN_2 &&
	 * channel_cfg->gain != ADC_GAIN_4 && channel_cfg->gain != ADC_GAIN_8 &&
	 * channel_cfg->gain != ADC_GAIN_16 && channel_cfg->gain != ADC_GAIN_32
	 */

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}
	return 0;
}

static int mcp3914_validate_buffer_size(const struct device *dev,
					const struct adc_sequence *sequence)

{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(sequence->channels);

	needed = channels * SIZE_OF_CH_DATA;

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int mcp3914_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct mcp3914_config *config = dev->config;
	struct mcp3914_data *data = dev->data;
	int err;

	if (find_msb_set(sequence->channels) > config->channels) {
		LOG_ERR("Dev %s too many ch: 0x%08x > %u", dev->name, sequence->channels,
			config->channels);
		return -ENOTSUP;
	}

	err = mcp3914_validate_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("Dev %s buffer %u too small", dev->name, sequence->buffer_size);
		return err;
	}

	data->buffer = sequence->buffer;
	data->ts_buffer = sequence->options->user_data;

	/* Throw away any pending ints */
	if (k_sem_count_get(&data->data_ready_signal)) {
		LOG_DBG("%s: dready reset", data->dev->name);
		k_sem_reset(&data->data_ready_signal);
	}

	/* Get the channels we are using out of reset */
	err = mcp3914_set_ch_reset(dev, ~sequence->channels);
	if (err) {
		LOG_ERR("Dev %s ch clear reset failed %d", dev->name, err);
		return err;
	}

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcp3914_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct mcp3914_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = mcp3914_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int mcp3914_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return mcp3914_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{

	struct mcp3914_data *data = CONTAINER_OF(ctx, struct mcp3914_data, ctx);

	k_sem_give(&data->read_data_signal);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{

	struct mcp3914_data *data = CONTAINER_OF(ctx, struct mcp3914_data, ctx);

	/* If a repeat has been asked for, reset these sampling instance vars */
	if (repeat_sampling) {
		data->buffer = ctx->sequence.buffer;
		data->ts_buffer = ctx->sequence.options->user_data;
		ctx->sampling_index = 0;
#ifdef CONFIG_ADC_ASYNC /* Signal async users, it is up to them to deal with races */
		if (ctx->asynchronous) {
			if (ctx->signal) {
				k_poll_signal_raise(ctx->signal, 0);
			}
		}
#endif
	}
}

static void adc_context_on_complete(struct adc_context *ctx, int status)
{
	struct mcp3914_data *data = CONTAINER_OF(ctx, struct mcp3914_data, ctx);

	/* Put all the chanels back into reset, we are finished sampling */
	mcp3914_set_ch_reset(data->dev, UINT8_MAX);

	/* Make sure that there isn't an interrupt left to service... */
	k_sem_reset(&data->data_ready_signal);
}

IRAM_ATTR void mcp3914_acquisition_thread(struct mcp3914_data *data)
{

	const struct spi_dt_spec *spi_dev = &((struct mcp3914_config *)(data->dev->config))->bus;

	while (true) {
		/* wait for the adc context to signal, ready for read */
		int res = k_sem_take(&data->read_data_signal, K_FOREVER);

		if (res < 0) {
			LOG_DBG("%s: read %s", data->dev->name, strerror(-res));
			break;
		}

		/* now wait for next interrupt data ready */
		res = k_sem_take(&data->data_ready_signal, K_FOREVER);
		if (res < 0) {
			LOG_DBG("%s: dready %s", data->dev->name, strerror(-res));
			break;
		}

		int32_t *readings;
		uint8_t len;
		uint32_t top_cn;

		top_cn = find_msb_set((uint32_t)data->ctx.sequence.channels) - 1;

		if (mcp3914_read_regs(data->dev, CH0_DATA_REG_DEV_ADDR,
				      (uint8_t)(CH0_DATA_REG_DEV_ADDR + top_cn),
				      (uint32_t **)&readings, &len)) {
			LOG_DBG("Acq dev %s failed", data->dev->name);
			k_free(readings);
		}

		for (uint8_t i = 0, j = BIT(i); i < len; i++, j = j << 1) {
			if ((data->ctx.sequence.channels & j)) {
				*data->buffer =
					((readings[i] ^ BIT(23)) - BIT(23)); /* Sign extend */
				data->buffer++;
			}
		}

		/* This driver assumes that if set the options user data stores
		 * a timestamp for each sampling and it is (1 + extra_samplings) long
		 */
		if (data->ts_buffer) {
			*data->ts_buffer = data->timestamp;
			data->ts_buffer++;
		}

		k_free(readings);
		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int mcp3914_set_addr_loop(const struct device *dev, uint32_t loop_type)
{
	uint32_t *regs;

	if (mcp3914_read_regs(dev, STATUSCOM_REG_DEV_ADDR, STATUSCOM_REG_DEV_ADDR, &regs, NULL)) {
		return ENODATA;
	}

	regs[0] = MCP3914_READ_CONT_CLR(regs[0]);
	regs[0] |= loop_type;

	if (mcp3914_write_regs(dev, STATUSCOM_REG_DEV_ADDR, STATUSCOM_REG_DEV_ADDR, regs)) {
		k_free(regs);
		return EBADMSG;
	}

	k_free(regs);

	return 0;
}

#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
/* Debug level dump the ADC register set for fun! */
static int mcp3914_dump_registers(const struct device *dev)
{
	uint32_t *regs;
	uint8_t reg_len;

	if (mcp3914_read_regs(dev, MCP3914_FIRST_REG_ADDR, MCP3914_LAST_REG_ADDR, &regs,
			      &reg_len)) {
		return ENODATA;
	}

	LOG_DBG("ADC %s reg set %u long", dev->name, reg_len);
	LOG_HEXDUMP_DBG(regs, reg_len * sizeof(regs[0]), "Register set");

#ifdef CONFIG_LOG_MODE_IMMEDIATE
	for (uint8_t i = 0; i < reg_len; i++) {
		LOG_DBG("Reg %02x = %08x", i, regs[i]);
	}
#endif

	k_free(regs);
	return 0;
}
#endif

static int mcp3914_init(const struct device *dev)
{

	LOG_DBG("Init %s", dev->name);

	const struct mcp3914_config *config = dev->config;
	struct mcp3914_data *data = dev->data;

	data->dev = dev;
	int result;

	adc_context_init(&data->ctx);

	k_sem_init(&data->read_data_signal, 0, 1);
	k_sem_init(&data->data_ready_signal, 0, 1);

#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
	data->drdy_counter = DREADY_ROLLOVER;
#endif

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (mcp3914_set_addr_loop(dev, MCP3914_READ_CONT_ALL)) {
		LOG_ERR("Set loop to all failed for %s", dev->name);
		return -ENODEV;
	}

	if (mcp3914_set_osr_presc(dev)) {
		LOG_ERR("Set ocr_presc failed for %s", dev->name);
		return -ENODEV;
	}

	/* Set all channels to RESET */
	if (mcp3914_set_ch_reset(dev, UINT8_MAX)) {
		LOG_ERR("Reset all ch failed for %s", dev->name);
		return -ENODEV;
	}

#if CONFIG_ADC_LOG_LEVEL > LOG_LEVEL_INF
	if (mcp3914_dump_registers(dev)) {
		LOG_ERR("%s register dump failed", dev->name);
		return -ENODATA;
	}
#endif

	/* Initialize GPIO */
	if (!gpio_is_ready_dt(&config->interrupt)) {
		LOG_ERR("%s: GPIO port %s not ready", dev->name, config->interrupt.port->name);
		return -EINVAL;
	}

	if (gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT)) {
		LOG_ERR("%s: Unable to configure GPIO pin %u", dev->name, config->interrupt.pin);
		return -EINVAL;
	}

	gpio_init_callback(&(data->callback_data_ready), mcp3914_data_ready_handler,
			   BIT(config->interrupt.pin));

	if (gpio_add_callback(config->interrupt.port, &(data->callback_data_ready))) {
		LOG_ERR("%s: Failed to add data ready callback", dev->name);
		return -EINVAL;
	}

	gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_INACTIVE);

	k_tid_t tid = k_thread_create(
		&data->thread, data->stack, CONFIG_ADC_MCP3914_ACQUISITION_THREAD_STACK_SIZE,
		(k_thread_entry_t)mcp3914_acquisition_thread, data, NULL, NULL,
		CONFIG_ADC_MCP3914_ACQUISITION_THREAD_PRIO, 0, K_FOREVER);

	if (!tid) {
		LOG_ERR("%s cannot create acq thread", dev->name);
		return -ENOSYS;
	}

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(tid, dev->name);
	}

	k_thread_start(tid);

	adc_context_unlock_unconditionally(&data->ctx);

	LOG_INF("MCP3941 device (%s) initialised", dev->name);

	return 0;
}

static const struct adc_driver_api mcp3914_adc_api = {
	.channel_setup = mcp3914_channel_setup,
	.read = mcp3914_read,
	.ref_internal = (uint16_t)MCP3914_INTERNAL_VOLTAGE_REFERENCE,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp3914_read_async,
#endif
};

#define MCP3491_CONF_IRQ(inst) GPIO_DT_SPEC_INST_GET(inst, dready_gpios)

#define MCP3491_ADC_INIT(inst)                                                                     \
                                                                                                   \
	static const struct mcp3914_config mcp3914_config_##inst = {                               \
		.bus = SPI_DT_SPEC_GET(DT_DRV_INST(inst),                                          \
				       SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8),    \
				       0),                                                         \
		.interrupt = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, dready_gpios),                \
					 (MCP3491_CONF_IRQ(inst)), ({0})),                         \
		.channels = DT_PROP(DT_DRV_INST(inst), channel_count),                             \
		.presc = DT_STRING_UNQUOTED(DT_DRV_INST(inst), amclk_prescale),                    \
		.osr = DT_STRING_UNQUOTED(DT_DRV_INST(inst), hware_osr),                           \
		.clksrc = DT_STRING_UNQUOTED(DT_DRV_INST(inst), mclk_src)};                        \
                                                                                                   \
	static struct mcp3914_data mcp3914_data_##inst = {                                         \
		ADC_CONTEXT_INIT_LOCK(mcp3914_data_##inst, ctx),                                   \
		ADC_CONTEXT_INIT_SYNC(mcp3914_data_##inst, ctx),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &mcp3914_init, NULL, &mcp3914_data_##inst,                     \
			      &mcp3914_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,       \
			      &mcp3914_adc_api);

DT_INST_FOREACH_STATUS_OKAY(MCP3491_ADC_INIT)

/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT semtech_sx1276

#include <drivers/counter.h>
#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <drivers/spi.h>
#include <zephyr.h>

#include <sx1276/sx1276.h>

#define LOG_LEVEL CONFIG_LORA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(sx1276);

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_RESET_FLAGS	DT_INST_GPIO_FLAGS(0, reset_gpios)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)

#define SX1276_REG_PA_CONFIG			0x09
#define SX1276_REG_PA_DAC			0x4d
#define SX1276_REG_VERSION			0x42

extern DioIrqHandler *DioIrq[];

struct sx1276_dio {
	const char *port;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define SX1276_DIO_GPIO_ELEM(idx, inst) \
	{ \
		DT_INST_GPIO_LABEL_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_PIN_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_FLAGS_BY_IDX(inst, dio_gpios, idx), \
	},

#define SX1276_DIO_GPIO_INIT(n) \
	UTIL_LISTIFY(DT_INST_PROP_LEN(n, dio_gpios), SX1276_DIO_GPIO_ELEM, n)

static struct sx1276_dio sx1276_dios[] = { SX1276_DIO_GPIO_INIT(0) };

#define SX1276_MAX_DIO ARRAY_SIZE(sx1276_dios)

struct sx1276_data {
	struct device *counter;
	struct device *reset;
	struct device *spi;
	struct spi_config spi_cfg;
	struct device *dio_dev[SX1276_MAX_DIO];
	struct k_sem data_sem;
	RadioEvents_t sx1276_event;
	u8_t *rx_buf;
	u8_t rx_len;
	s8_t snr;
	s16_t rssi;
} dev_data;

bool SX1276CheckRfFrequency(uint32_t frequency)
{
	/* TODO */
	return true;
}

void RtcStopAlarm(void)
{
	counter_stop(dev_data.counter);
}

void SX1276SetAntSwLowPower(bool status)
{
	/* TODO */
}

void SX1276SetBoardTcxo(u8_t state)
{
	/* TODO */
}

void SX1276SetAntSw(u8_t opMode)
{
	/* TODO */
}

void SX1276Reset(void)
{
	gpio_pin_configure(dev_data.reset, GPIO_RESET_PIN,
			   GPIO_OUTPUT_ACTIVE | GPIO_RESET_FLAGS);

	k_sleep(K_MSEC(1));

	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);

	k_sleep(K_MSEC(6));
}

void BoardCriticalSectionBegin(uint32_t *mask)
{
	*mask = irq_lock();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
	irq_unlock(*mask);
}

uint32_t RtcGetTimerElapsedTime(void)
{
	u32_t ticks;
	int err;

	err = counter_get_value(dev_data.counter, &ticks);
	if (err) {
		LOG_ERR("Failed to read counter value (err %d)", err);
		return 0;
	}

	return ticks;
}

u32_t RtcGetMinimumTimeout(void)
{
	/* TODO: Get this value from counter driver */
	return 3;
}

void RtcSetAlarm(uint32_t timeout)
{
	struct counter_alarm_cfg alarm_cfg;

	alarm_cfg.flags = 0;
	alarm_cfg.ticks = timeout;

	counter_set_channel_alarm(dev_data.counter, 0, &alarm_cfg);
}

uint32_t RtcSetTimerContext(void)
{
	return 0;
}

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
	return counter_us_to_ticks(dev_data.counter, (milliseconds / 1000));
}

void DelayMsMcu(uint32_t ms)
{
	k_sleep(ms);
}

static void sx1276_irq_callback(struct device *dev,
				struct gpio_callback *cb, u32_t pins)
{
	unsigned int i, pin;

	pin = find_lsb_set(pins) - 1;

	for (i = 0; i < SX1276_MAX_DIO; i++) {
		if (pin == sx1276_dios[i].pin) {
			(*DioIrq[i])(NULL);
		}
	}
}

void SX1276IoIrqInit(DioIrqHandler **irqHandlers)
{
	unsigned int i;
	static struct gpio_callback callbacks[SX1276_MAX_DIO];

	/* Setup DIO gpios */
	for (i = 0; i < SX1276_MAX_DIO; i++) {
		dev_data.dio_dev[i] = device_get_binding(sx1276_dios[i].port);
		if (dev_data.dio_dev[i] == NULL) {
			LOG_ERR("Cannot get pointer to %s device",
				sx1276_dios[i].port);
			return;
		}

		gpio_pin_configure(dev_data.dio_dev[i], sx1276_dios[i].pin,
				   GPIO_INPUT | GPIO_INT_DEBOUNCE
				   | sx1276_dios[i].flags);

		gpio_init_callback(&callbacks[i],
				   sx1276_irq_callback,
				   BIT(sx1276_dios[i].pin));

		if (gpio_add_callback(dev_data.dio_dev[i], &callbacks[i]) < 0) {
			LOG_ERR("Could not set gpio callback.");
			return;
		}
		gpio_pin_interrupt_configure(dev_data.dio_dev[i],
					     sx1276_dios[i].pin,
					     GPIO_INT_EDGE_TO_ACTIVE);
	}

}

static int sx1276_transceive(u8_t reg, bool write, void *data, size_t length)
{
	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = sizeof(reg)
		},
		{
			.buf = data,
			.len = length
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	if (!write) {
		const struct spi_buf_set rx = {
			.buffers = buf,
			.count = ARRAY_SIZE(buf)
		};

		return spi_transceive(dev_data.spi, &dev_data.spi_cfg,
				&tx, &rx);
	}

	return spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
}

int sx1276_read(u8_t reg_addr, u8_t *data, u8_t len)
{
	return sx1276_transceive(reg_addr, false, data, len);
}

int sx1276_write(u8_t reg_addr, u8_t *data, u8_t len)
{
	return sx1276_transceive(reg_addr | BIT(7), true, data, len);
}

void SX1276WriteBuffer(u16_t addr, u8_t *buffer, u8_t size)
{
	int ret;

	ret = sx1276_write(addr, buffer, size);
	if (ret < 0) {
		LOG_ERR("Unable to write address: 0x%x", addr);
	}
}

void SX1276ReadBuffer(u16_t addr, u8_t *buffer, u8_t size)
{
	int ret;

	ret = sx1276_read(addr, buffer, size);
	if (ret < 0) {
		LOG_ERR("Unable to read address: 0x%x", addr);
	}
}

void SX1276SetRfTxPower(int8_t power)
{
	int ret;
	u8_t pa_config = 0;
	u8_t pa_dac = 0;

	ret = sx1276_read(SX1276_REG_PA_CONFIG, &pa_config, 1);
	if (ret < 0) {
		LOG_ERR("Unable to read PA config");
		return;
	}

	ret = sx1276_read(SX1276_REG_PA_DAC, &pa_dac, 1);
	if (ret < 0) {
		LOG_ERR("Unable to read PA dac");
		return;
	}

	pa_config = (pa_config & RF_PACONFIG_MAX_POWER_MASK) | 0x70;
	pa_config &= RF_PACONFIG_PASELECT_MASK;

#if defined CONFIG_PA_BOOST_PIN
	pa_config |= RF_PACONFIG_PASELECT_PABOOST;

	if (power > 17) {
		pa_dac = (pa_dac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
	} else {
		pa_dac = (pa_dac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
	}

	if ((pa_dac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
		if (power < 5) {
			power = 5;
		} else if (power > 20) {
			power = 20;
		}

		pa_config = (pa_config & RF_PACONFIG_OUTPUTPOWER_MASK) |
			     ((power - 5) & 0x0F);
	} else {
		if (power < 2) {
			power = 2;
		} else if (power > 17) {
			power = 17;
		}

		pa_config = (pa_config & RF_PACONFIG_OUTPUTPOWER_MASK) |
			     ((power - 2) & 0x0F);
	}
#elif CONFIG_PA_RFO_PIN
	if (power < -1) {
		power = -1;
	} else if (power > 14) {
		power = 14;
	}

	pa_config = (pa_config & RF_PACONFIG_OUTPUTPOWER_MASK) |
			     ((power + 1) & 0x0F);
#endif
	ret = sx1276_write(SX1276_REG_PA_CONFIG, &pa_config, 1);
	if (ret < 0) {
		LOG_ERR("Unable to write PA config");
		return;
	}

	ret = sx1276_write(SX1276_REG_PA_DAC, &pa_dac, 1);
	if (ret < 0) {
		LOG_ERR("Unable to write PA dac");
		return;
	}
}

static int sx1276_lora_send(struct device *dev, u8_t *data, u32_t data_len)
{
	Radio.SetMaxPayloadLength(MODEM_LORA, data_len);

	Radio.Send(data, data_len);

	return 0;
}

static void sx1276_tx_done(void)
{
	Radio.Sleep();
}

static void sx1276_rx_done(u8_t *payload, u16_t size, int16_t rssi, int8_t snr)
{
	Radio.Sleep();

	dev_data.rx_buf = payload;
	dev_data.rx_len = size;
	dev_data.rssi = rssi;
	dev_data.snr = snr;

	k_sem_give(&dev_data.data_sem);
}

static int sx1276_lora_recv(struct device *dev, u8_t *data, u8_t size,
			    s32_t timeout, s16_t *rssi, s8_t *snr)
{
	int ret;

	Radio.SetMaxPayloadLength(MODEM_LORA, 255);
	Radio.Rx(0);

	/*
	 * As per the API requirement, timeout value can be in ms/K_FOREVER/
	 * K_NO_WAIT. So, let's handle all cases.
	 */
	ret = k_sem_take(&dev_data.data_sem, timeout == K_FOREVER ? K_FOREVER :
			 timeout == K_NO_WAIT ? K_NO_WAIT : K_MSEC(timeout));
	if (ret < 0) {
		LOG_ERR("Receive timeout!");
		return ret;
	}

	/* Only copy the bytes that can fit the buffer, drop the rest */
	if (dev_data.rx_len > size)
		dev_data.rx_len = size;

	/*
	 * FIXME: We are copying the global buffer here, so it might get
	 * overwritten inbetween when a new packet comes in. Use some
	 * wise method to fix this!
	 */
	memcpy(data, dev_data.rx_buf, dev_data.rx_len);

	if (rssi != NULL) {
		*rssi = dev_data.rssi;
	}

	if (snr != NULL) {
		*snr = dev_data.snr;
	}

	return dev_data.rx_len;
}

static int sx1276_lora_config(struct device *dev,
			      struct lora_modem_config *config)
{

	Radio.SetChannel(config->frequency);

	if (config->tx) {
		Radio.SetTxConfig(MODEM_LORA, config->tx_power, 0,
				  config->bandwidth, config->datarate,
				  config->coding_rate, config->preamble_len,
				  false, true, 0, 0, false, 4000);
	} else {
		/* TODO: Get symbol timeout value from config parameters */
		Radio.SetRxConfig(MODEM_LORA, config->bandwidth,
				  config->datarate, config->coding_rate,
				  0, config->preamble_len, 10, false, 0,
				  false, 0, 0, false, true);
	}

	return 0;
}

/* Initialize Radio driver callbacks */
const struct Radio_s Radio = {
	.Init = SX1276Init,
	.GetStatus = SX1276GetStatus,
	.SetModem = SX1276SetModem,
	.SetChannel = SX1276SetChannel,
	.IsChannelFree = SX1276IsChannelFree,
	.Random = SX1276Random,
	.SetRxConfig = SX1276SetRxConfig,
	.SetTxConfig = SX1276SetTxConfig,
	.Send = SX1276Send,
	.Sleep = SX1276SetSleep,
	.Standby = SX1276SetStby,
	.Rx = SX1276SetRx,
	.Write = SX1276Write,
	.Read = SX1276Read,
	.WriteBuffer = SX1276WriteBuffer,
	.ReadBuffer = SX1276ReadBuffer,
	.SetMaxPayloadLength = SX1276SetMaxPayloadLength,
	.IrqProcess = NULL,
	.RxBoosted = NULL,
	.SetRxDutyCycle = NULL,
};

static int sx1276_lora_init(struct device *dev)
{
	static struct spi_cs_control spi_cs;
	int ret;
	u8_t regval;

	dev_data.spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!dev_data.spi) {
		LOG_ERR("Cannot get pointer to %s device",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	dev_data.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	dev_data.spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	dev_data.spi_cfg.slave = DT_INST_REG_ADDR(0);

	spi_cs.gpio_pin = GPIO_CS_PIN,
	spi_cs.gpio_dev = device_get_binding(
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!spi_cs.gpio_dev) {
		LOG_ERR("Cannot get pointer to %s device",
		       DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
		return -EIO;
	}

	dev_data.spi_cfg.cs = &spi_cs;

	/* Setup Reset gpio */
	dev_data.reset = device_get_binding(
			DT_INST_GPIO_LABEL(0, reset_gpios));
	if (!dev_data.reset) {
		LOG_ERR("Cannot get pointer to %s device",
		       DT_INST_GPIO_LABEL(0, reset_gpios));
		return -EIO;
	}

	/* Perform soft reset */
	ret = gpio_pin_configure(dev_data.reset, GPIO_RESET_PIN,
				 GPIO_OUTPUT_ACTIVE | GPIO_RESET_FLAGS);

	k_sleep(K_MSEC(100));
	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);
	k_sleep(K_MSEC(100));

	ret = sx1276_read(SX1276_REG_VERSION, &regval, 1);
	if (ret < 0) {
		LOG_ERR("Unable to read version info");
		return -EIO;
	}

	dev_data.counter = device_get_binding(DT_RTC_0_NAME);
	if (!dev_data.counter) {
		LOG_ERR("Cannot get pointer to %s device", DT_RTC_0_NAME);
		return -EIO;
	}

	k_sem_init(&dev_data.data_sem, 0, UINT_MAX);

	dev_data.sx1276_event.TxDone = sx1276_tx_done;
	dev_data.sx1276_event.RxDone = sx1276_rx_done;
	Radio.Init(&dev_data.sx1276_event);

	LOG_INF("SX1276 Version:%02x found", regval);

	return 0;
}

static const struct lora_driver_api sx1276_lora_api = {
	.config = sx1276_lora_config,
	.send = sx1276_lora_send,
	.recv = sx1276_lora_recv,
};

DEVICE_AND_API_INIT(sx1276_lora, DT_INST_LABEL(0),
		    &sx1276_lora_init, NULL,
		    NULL, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		    &sx1276_lora_api);

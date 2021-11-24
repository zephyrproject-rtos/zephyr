/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/lora.h>
#include <drivers/spi.h>
#include <zephyr.h>

#include "sx12xx_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(sx127x, CONFIG_LORA_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1272)

#include <sx1272/sx1272.h>

#define DT_DRV_COMPAT semtech_sx1272
#define SX127X_FUNC(func) SX1272##func

#elif DT_HAS_COMPAT_STATUS_OKAY(semtech_sx1276)

#include <sx1276/sx1276.h>

#define DT_DRV_COMPAT semtech_sx1276
#define SX127X_FUNC(func) SX1276##func

#else
#error No SX127x instance in device tree.
#endif

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(semtech_sx1272) +
	     DT_NUM_INST_STATUS_OKAY(semtech_sx1276) <= 1,
	     "Multiple SX127x instances in DT");

#define GPIO_RESET_PIN		DT_INST_GPIO_PIN(0, reset_gpios)
#define GPIO_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define GPIO_CS_FLAGS		DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0)

#define GPIO_ANTENNA_ENABLE_PIN				\
	DT_INST_GPIO_PIN(0, antenna_enable_gpios)
#define GPIO_RFI_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, rfi_enable_gpios)
#define GPIO_RFO_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, rfo_enable_gpios)
#define GPIO_PA_BOOST_ENABLE_PIN			\
	DT_INST_GPIO_PIN(0, pa_boost_enable_gpios)

#define GPIO_TCXO_POWER_PIN	DT_INST_GPIO_PIN(0, tcxo_power_gpios)

/*
 * Those macros must be in sync with 'power-amplifier-output' dts property.
 */
#define SX127X_PA_RFO				0
#define SX127X_PA_BOOST				1

#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios) &&	\
	DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
#define SX127X_PA_OUTPUT(power)				\
	((power) > 14 ? SX127X_PA_BOOST : SX127X_PA_RFO)
#elif DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
#define SX127X_PA_OUTPUT(power)		SX127X_PA_RFO
#elif DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
#define SX127X_PA_OUTPUT(power)		SX127X_PA_BOOST
#elif DT_INST_NODE_HAS_PROP(0, power_amplifier_output)
#define SX127X_PA_OUTPUT(power)				\
	DT_INST_ENUM_IDX(0, power_amplifier_output)
#else
BUILD_ASSERT(0, "None of rfo-enable-gpios, pa-boost-enable-gpios and "
	     "power-amplifier-output has been specified. "
	     "Look at semtech,sx127x-base.yaml to fix that.");
#endif

extern DioIrqHandler *DioIrq[];

struct sx127x_dio {
	const char *port;
	gpio_pin_t pin;
	gpio_dt_flags_t flags;
};

/* Helper macro that UTIL_LISTIFY can use and produces an element with comma */
#define SX127X_DIO_GPIO_LEN(inst) \
	DT_INST_PROP_LEN(inst, dio_gpios)

#define SX127X_DIO_GPIO_ELEM(idx, inst) \
	{ \
		DT_INST_GPIO_LABEL_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_PIN_BY_IDX(inst, dio_gpios, idx), \
		DT_INST_GPIO_FLAGS_BY_IDX(inst, dio_gpios, idx), \
	},

#define SX127X_DIO_GPIO_INIT(n) \
	UTIL_LISTIFY(SX127X_DIO_GPIO_LEN(n), SX127X_DIO_GPIO_ELEM, n)

static const struct sx127x_dio sx127x_dios[] = { SX127X_DIO_GPIO_INIT(0) };

#define SX127X_MAX_DIO ARRAY_SIZE(sx127x_dios)

static struct sx127x_data {
	const struct device *reset;
#if DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
	const struct device *antenna_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
	const struct device *rfi_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
	const struct device *rfo_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	const struct device *pa_boost_enable;
#endif
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios) &&	\
	DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	uint8_t tx_power;
#endif
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
	const struct device *tcxo_power;
	bool tcxo_power_enabled;
#endif
	const struct device *spi;
	struct spi_config spi_cfg;
	const struct device *dio_dev[SX127X_MAX_DIO];
	struct k_work dio_work[SX127X_MAX_DIO];
} dev_data;

static int8_t clamp_int8(int8_t x, int8_t min, int8_t max)
{
	if (x < min) {
		return min;
	} else if (x > max) {
		return max;
	} else {
		return x;
	}
}

bool SX127X_FUNC(CheckRfFrequency)(uint32_t frequency)
{
	/* TODO */
	return true;
}

uint32_t SX127X_FUNC(GetBoardTcxoWakeupTime)(void)
{
	return DT_INST_PROP_OR(0, tcxo_power_startup_delay_ms, 0);
}

static inline void sx127x_antenna_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, antenna_enable_gpios)
	gpio_pin_set(dev_data.antenna_enable, GPIO_ANTENNA_ENABLE_PIN, val);
#endif
}

static inline void sx127x_rfi_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
	gpio_pin_set(dev_data.rfi_enable, GPIO_RFI_ENABLE_PIN, val);
#endif
}

static inline void sx127x_rfo_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
	gpio_pin_set(dev_data.rfo_enable, GPIO_RFO_ENABLE_PIN, val);
#endif
}

static inline void sx127x_pa_boost_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	gpio_pin_set(dev_data.pa_boost_enable, GPIO_PA_BOOST_ENABLE_PIN, val);
#endif
}

void SX127X_FUNC(SetAntSwLowPower)(bool low_power)
{
	if (low_power) {
		/* force inactive (low power) state of all antenna paths */
		sx127x_rfi_enable(0);
		sx127x_rfo_enable(0);
		sx127x_pa_boost_enable(0);

		sx127x_antenna_enable(0);
	} else {
		sx127x_antenna_enable(1);

		/* rely on SX127xSetAntSw() to configure proper antenna path */
	}
}

void SX127X_FUNC(SetBoardTcxo)(uint8_t state)
{
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
	bool enable = state;

	if (enable == dev_data.tcxo_power_enabled) {
		return;
	}

	if (enable) {
		gpio_pin_set(dev_data.tcxo_power, GPIO_TCXO_POWER_PIN, 1);

		if (TCXO_POWER_STARTUP_DELAY_MS > 0) {
			k_sleep(K_MSEC(TCXO_POWER_STARTUP_DELAY_MS));
		}
	} else {
		gpio_pin_set(dev_data.tcxo_power, GPIO_TCXO_POWER_PIN, 0);
	}

	dev_data.tcxo_power_enabled = enable;
#endif
}

void SX127X_FUNC(SetAntSw)(uint8_t opMode)
{
	switch (opMode) {
	case RFLR_OPMODE_TRANSMITTER:
		sx127x_rfi_enable(0);

		if (SX127X_PA_OUTPUT(dev_data.tx_power) == SX127X_PA_BOOST) {
			sx127x_rfo_enable(0);
			sx127x_pa_boost_enable(1);
		} else {
			sx127x_pa_boost_enable(0);
			sx127x_rfo_enable(1);
		}
		break;
	default:
		sx127x_rfo_enable(0);
		sx127x_pa_boost_enable(0);
		sx127x_rfi_enable(1);
		break;
	}
}

void SX127X_FUNC(Reset)(void)
{
	SX127X_FUNC(SetBoardTcxo)(true);

	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 1);

	k_sleep(K_MSEC(1));

	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);

	k_sleep(K_MSEC(6));
}

static void sx127x_dio_work_handle(struct k_work *work)
{
	int dio = work - dev_data.dio_work;

	(*DioIrq[dio])(NULL);
}

static void sx127x_irq_callback(const struct device *dev,
				struct gpio_callback *cb, uint32_t pins)
{
	unsigned int i, pin;

	pin = find_lsb_set(pins) - 1;

	for (i = 0; i < SX127X_MAX_DIO; i++) {
		if (dev == dev_data.dio_dev[i] &&
		    pin == sx127x_dios[i].pin) {
			k_work_submit(&dev_data.dio_work[i]);
		}
	}
}

void SX127X_FUNC(IoIrqInit)(DioIrqHandler **irqHandlers)
{
	unsigned int i;
	static struct gpio_callback callbacks[SX127X_MAX_DIO];

	/* Setup DIO gpios */
	for (i = 0; i < SX127X_MAX_DIO; i++) {
		if (!irqHandlers[i]) {
			continue;
		}

		dev_data.dio_dev[i] = device_get_binding(sx127x_dios[i].port);
		if (dev_data.dio_dev[i] == NULL) {
			LOG_ERR("Cannot get pointer to %s device",
				sx127x_dios[i].port);
			return;
		}

		k_work_init(&dev_data.dio_work[i], sx127x_dio_work_handle);

		gpio_pin_configure(dev_data.dio_dev[i], sx127x_dios[i].pin,
				   GPIO_INPUT | GPIO_INT_DEBOUNCE
				   | sx127x_dios[i].flags);

		gpio_init_callback(&callbacks[i],
				   sx127x_irq_callback,
				   BIT(sx127x_dios[i].pin));

		if (gpio_add_callback(dev_data.dio_dev[i], &callbacks[i]) < 0) {
			LOG_ERR("Could not set gpio callback.");
			return;
		}
		gpio_pin_interrupt_configure(dev_data.dio_dev[i],
					     sx127x_dios[i].pin,
					     GPIO_INT_EDGE_TO_ACTIVE);
	}

}

static int sx127x_transceive(uint8_t reg, bool write, void *data, size_t length)
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

		return spi_transceive(dev_data.spi, &dev_data.spi_cfg, &tx, &rx);
	}

	return spi_write(dev_data.spi, &dev_data.spi_cfg, &tx);
}

int sx127x_read(uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	return sx127x_transceive(reg_addr, false, data, len);
}

int sx127x_write(uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	return sx127x_transceive(reg_addr | BIT(7), true, data, len);
}

void SX127X_FUNC(WriteBuffer)(uint32_t addr, uint8_t *buffer, uint8_t size)
{
	int ret;

	ret = sx127x_write(addr, buffer, size);
	if (ret < 0) {
		LOG_ERR("Unable to write address: 0x%x", addr);
	}
}

void SX127X_FUNC(ReadBuffer)(uint32_t addr, uint8_t *buffer, uint8_t size)
{
	int ret;

	ret = sx127x_read(addr, buffer, size);
	if (ret < 0) {
		LOG_ERR("Unable to read address: 0x%x", addr);
	}
}

void SX127X_FUNC(SetRfTxPower)(int8_t power)
{
	int ret;
	uint8_t pa_config = 0;
	uint8_t pa_dac = 0;

	ret = sx127x_read(REG_PADAC, &pa_dac, 1);
	if (ret < 0) {
		LOG_ERR("Unable to read PA dac");
		return;
	}

	pa_dac &= RF_PADAC_20DBM_MASK;

	if (SX127X_PA_OUTPUT(power) == SX127X_PA_BOOST) {
		power = clamp_int8(power, 2, 20);

		pa_config |= RF_PACONFIG_PASELECT_PABOOST;
		if (power > 17) {
			pa_dac |= RF_PADAC_20DBM_ON;
			pa_config |= (power - 5) & ~RF_PACONFIG_OUTPUTPOWER_MASK;
		} else {
			pa_dac |= RF_PADAC_20DBM_OFF;
			pa_config |= (power - 2) & ~RF_PACONFIG_OUTPUTPOWER_MASK;
		}
	} else {
#ifdef RF_PACONFIG_MAX_POWER_MASK
		power = clamp_int8(power, -4, 15);

		pa_dac |= RF_PADAC_20DBM_OFF;
		if (power > 0) {
			/* Set the power range to 0 -- 10.8+0.6*7 dBm */
			pa_config |= 7 << 4;
			pa_config |= power & ~RF_PACONFIG_OUTPUTPOWER_MASK;
		} else {
			/* Set the power range to -4.2 -- 10.8+0.6*0 dBm */
			pa_config |= (power + 4) & ~RF_PACONFIG_OUTPUTPOWER_MASK;
		}
#else
		power = clamp_int8(power, -1, 14);

		pa_dac |= RF_PADAC_20DBM_OFF;
		pa_config |= (power + 1) & ~RF_PACONFIG_OUTPUTPOWER_MASK;
#endif
	}

#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios) &&	\
	DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	dev_data.tx_power = power;
#endif

	ret = sx127x_write(REG_PACONFIG, &pa_config, 1);
	if (ret < 0) {
		LOG_ERR("Unable to write PA config");
		return;
	}

	ret = sx127x_write(REG_PADAC, &pa_dac, 1);
	if (ret < 0) {
		LOG_ERR("Unable to write PA dac");
		return;
	}
}

uint32_t SX127X_FUNC(GetDio1PinState)(void)
{
#if SX127X_DIO_GPIO_LEN(0) >= 2
	if (gpio_pin_get(dev_data.dio_dev[1], sx127x_dios[1].pin) > 0) {
		return 1U;
	}
#endif

	return 0U;
}

/* Initialize Radio driver callbacks */
const struct Radio_s Radio = {
	.Init = SX127X_FUNC(Init),
	.GetStatus = SX127X_FUNC(GetStatus),
	.SetModem = SX127X_FUNC(SetModem),
	.SetChannel = SX127X_FUNC(SetChannel),
	.IsChannelFree = SX127X_FUNC(IsChannelFree),
	.Random = SX127X_FUNC(Random),
	.SetRxConfig = SX127X_FUNC(SetRxConfig),
	.SetTxConfig = SX127X_FUNC(SetTxConfig),
	.CheckRfFrequency = SX127X_FUNC(CheckRfFrequency),
	.TimeOnAir = SX127X_FUNC(GetTimeOnAir),
	.Send = SX127X_FUNC(Send),
	.Sleep = SX127X_FUNC(SetSleep),
	.Standby = SX127X_FUNC(SetStby),
	.Rx = SX127X_FUNC(SetRx),
	.Write = SX127X_FUNC(Write),
	.Read = SX127X_FUNC(Read),
	.WriteBuffer = SX127X_FUNC(WriteBuffer),
	.ReadBuffer = SX127X_FUNC(ReadBuffer),
	.SetMaxPayloadLength = SX127X_FUNC(SetMaxPayloadLength),
	.SetPublicNetwork = SX127X_FUNC(SetPublicNetwork),
	.GetWakeupTime = SX127X_FUNC(GetWakeupTime),
	.IrqProcess = NULL,
	.RxBoosted = NULL,
	.SetRxDutyCycle = NULL,
	.SetTxContinuousWave = SX127X_FUNC(SetTxContinuousWave),
};

static int sx127x_antenna_configure(void)
{
	int ret;

	ret = sx12xx_configure_pin(&dev_data, antenna_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&dev_data, rfi_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&dev_data, rfo_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&dev_data, pa_boost_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	return 0;
}

static int sx127x_lora_init(const struct device *dev)
{
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	static struct spi_cs_control spi_cs;
#endif
	int ret;
	uint8_t regval;

	dev_data.spi = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!dev_data.spi) {
		LOG_ERR("Cannot get pointer to %s device",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	dev_data.spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
	dev_data.spi_cfg.frequency = DT_INST_PROP(0, spi_max_frequency);
	dev_data.spi_cfg.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	spi_cs.gpio_dev = device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	if (!spi_cs.gpio_dev) {
		LOG_ERR("Cannot get pointer to %s device",
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
		return -EIO;
	}

	spi_cs.gpio_pin = GPIO_CS_PIN;
	spi_cs.gpio_dt_flags = GPIO_CS_FLAGS;
	spi_cs.delay = 0U;

	dev_data.spi_cfg.cs = &spi_cs;
#endif

	ret = sx12xx_configure_pin(&dev_data, tcxo_power, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	/* Setup Reset gpio and perform soft reset */
	ret = sx12xx_configure_pin(&dev_data, reset, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(100));
	gpio_pin_set(dev_data.reset, GPIO_RESET_PIN, 0);
	k_sleep(K_MSEC(100));

	ret = sx127x_read(REG_VERSION, &regval, 1);
	if (ret < 0) {
		LOG_ERR("Unable to read version info");
		return -EIO;
	}

	LOG_INF("SX127x version 0x%02x found", regval);

	ret = sx127x_antenna_configure();
	if (ret < 0) {
		LOG_ERR("Unable to configure antenna");
		return -EIO;
	}

	ret = sx12xx_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize SX12xx common");
		return ret;
	}

	return 0;
}

static const struct lora_driver_api sx127x_lora_api = {
	.config = sx12xx_lora_config,
	.send = sx12xx_lora_send,
	.send_async = sx12xx_lora_send_async,
	.recv = sx12xx_lora_recv,
	.recv_async = sx12xx_lora_recv_async,
	.test_cw = sx12xx_lora_test_cw,
};

DEVICE_DT_INST_DEFINE(0, &sx127x_lora_init, NULL, NULL,
		      NULL, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &sx127x_lora_api);

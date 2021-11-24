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

#define SX127X_MAX_DIO DT_PROP_LEN(DT_DRV_INST(0), dio_gpios)

struct sx127x_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec antenna_enable;
	struct gpio_dt_spec rfi_enable;
	struct gpio_dt_spec rfo_enable;
	struct gpio_dt_spec pa_boost_enable;
	struct gpio_dt_spec tcxo_power;
	struct gpio_dt_spec dios[SX127X_MAX_DIO];
};

struct sx127x_data {
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios) &&	\
	DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	uint8_t tx_power;
#endif
#if DT_INST_NODE_HAS_PROP(0, tcxo_power_gpios)
	bool tcxo_power_enabled;
#endif
	struct k_work dio_work[SX127X_MAX_DIO];
};

static const struct device *const sx127x = DEVICE_DT_GET(DT_DRV_INST(0));

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
	const struct sx127x_config *cfg = sx127x->config;

	gpio_pin_set_dt(&cfg->antenna_enable, val);
#endif
}

static inline void sx127x_rfi_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfi_enable_gpios)
	const struct sx127x_config *cfg = sx127x->config;

	gpio_pin_set_dt(&cfg->rfi_enable, val);
#endif
}

static inline void sx127x_rfo_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, rfo_enable_gpios)
	const struct sx127x_config *cfg = sx127x->config;

	gpio_pin_set_dt(&cfg->rfo_enable, val);
#endif
}

static inline void sx127x_pa_boost_enable(int val)
{
#if DT_INST_NODE_HAS_PROP(0, pa_boost_enable_gpios)
	const struct sx127x_config *cfg = sx127x->config;

	gpio_pin_set_dt(&cfg->pa_boost_enable, val);
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
	const struct sx127x_config *cfg = sx127x->config;
	struct sx127x_data *data = sx127x->data;
	bool enable = state;

	if (enable == data->tcxo_power_enabled) {
		return;
	}

	if (enable) {
		gpio_pin_set_dt(&cfg->tcxo_power, 1);

		if (TCXO_POWER_STARTUP_DELAY_MS > 0) {
			k_sleep(K_MSEC(TCXO_POWER_STARTUP_DELAY_MS));
		}
	} else {
		gpio_pin_set_dt(&cfg->tcxo_power, 0);
	}

	data->tcxo_power_enabled = enable;
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
	const struct sx127x_config *cfg = sx127x->config;

	SX127X_FUNC(SetBoardTcxo)(true);

	gpio_pin_set_dt(&cfg->reset, 1);

	k_sleep(K_MSEC(1));

	gpio_pin_set_dt(&cfg->reset, 0);

	k_sleep(K_MSEC(6));
}

static void sx127x_dio_work_handle(struct k_work *work)
{
	struct sx127x_data *data = sx127x->data;
	int dio = work - data->dio_work;

	(*DioIrq[dio])(NULL);
}

static void sx127x_irq_callback(const struct device *dev,
				struct gpio_callback *cb, uint32_t pins)
{
	const struct sx127x_config *cfg = sx127x->config;
	struct sx127x_data *data = sx127x->data;
	unsigned int i, pin;

	pin = find_lsb_set(pins) - 1;

	for (i = 0; i < ARRAY_SIZE(cfg->dios); i++) {
		if (dev == cfg->dios[i].port &&
		    pin == cfg->dios[i].pin) {
			k_work_submit(&data->dio_work[i]);
		}
	}
}

void SX127X_FUNC(IoIrqInit)(DioIrqHandler **irqHandlers)
{
	const struct sx127x_config *cfg = sx127x->config;
	struct sx127x_data *data = sx127x->data;
	static struct gpio_callback callbacks[SX127X_MAX_DIO];
	unsigned int i;

	/* Setup DIO gpios */
	for (i = 0; i < ARRAY_SIZE(cfg->dios); i++) {
		if (!irqHandlers[i]) {
			continue;
		}

		/* Configure pin */
		gpio_pin_configure_dt(&cfg->dios[i], GPIO_INPUT | GPIO_INT_DEBOUNCE);

		k_work_init(&data->dio_work[i], sx127x_dio_work_handle);

		gpio_init_callback(&callbacks[i],
				   sx127x_irq_callback,
				   BIT(cfg->dios[i].pin));

		if (gpio_add_callback(cfg->dios[i].port, &callbacks[i]) < 0) {
			LOG_ERR("Could not set gpio callback.");
			return;
		}
		gpio_pin_interrupt_configure_dt(&cfg->dios[i], GPIO_INT_EDGE_TO_ACTIVE);
	}

}

static int sx127x_transceive(uint8_t reg, bool write, void *data, size_t length)
{
	const struct sx127x_config *cfg = sx127x->config;
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

		return spi_transceive_dt(&cfg->bus, &tx, &rx);
	}

	return spi_write_dt(&cfg->bus, &tx);
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
		power = CLAMP(power, 2, 20);

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
		power = CLAMP(power, -4, 15);

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
		power = CLAMP(power, -1, 14);

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
#if SX127X_MAX_DIO >= 2
	const struct sx127x_config *cfg = sx127x->config;

	return gpio_pin_get_dt(&cfg->dios[1]);
#else
	return 0U;
#endif
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
	const struct sx127x_config *cfg = sx127x->config;
	int ret;

	ret = sx12xx_configure_pin(&cfg->antenna_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&cfg->rfi_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&cfg->rfo_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	ret = sx12xx_configure_pin(&cfg->pa_boost_enable, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	return 0;
}

static int sx127x_lora_init(const struct device *dev)
{
	const struct sx127x_config *cfg = dev->config;
	uint8_t regval;
	int ret;

	if (!spi_is_ready(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = sx12xx_configure_pin(&cfg->tcxo_power, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	/* Setup Reset gpio and perform soft reset */
	ret = sx12xx_configure_pin(&cfg->reset, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(100));
	gpio_pin_set_dt(&cfg->reset, 0);
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

#define DIO_INIT(node_id, prop, idx)   GPIO_DT_SPEC_GET_BY_IDX_OR(node_id, dio_gpios, idx, {}),

static const struct sx127x_config sx127x_0_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0),
	.reset = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {0}),
	.antenna_enable = GPIO_DT_SPEC_INST_GET_OR(0, antenna_enable_gpios, {0}),
	.rfi_enable = GPIO_DT_SPEC_INST_GET_OR(0, rfi_enable_gpios, {0}),
	.rfo_enable = GPIO_DT_SPEC_INST_GET_OR(0, rfo_enable_gpios, {0}),
	.rfo_enable = GPIO_DT_SPEC_INST_GET_OR(0, pa_boost_enable_gpios, {0}),
	.rfo_enable = GPIO_DT_SPEC_INST_GET_OR(0, tcxo_power_gpios, {0}),
	.dios = { DT_INST_FOREACH_PROP_ELEM(0, dio_gpios, DIO_INIT) }
};

static struct sx127x_data sx127x_0_data;

DEVICE_DT_INST_DEFINE(0, &sx127x_lora_init, NULL, &sx127x_0_data,
		      &sx127x_0_config, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
		      &sx127x_lora_api);

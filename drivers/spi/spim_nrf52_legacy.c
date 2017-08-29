/*
 * Copyright (c) 2017 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * #define CONFIG_ASSERT
 * #define __ASSERT_ON 1
 */

#include <errno.h>
#include <spi.h>
#include <soc.h>
#include <nrf.h>
#include <misc/util.h>
#include <gpio.h>

#define SYS_LOG_DOMAIN "spim"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL
#include <logging/sys_log.h>

/* @todo
 *
 * Add support for 52840 spim2.
 */

struct spim_nrf52_config {
	volatile NRF_SPIM_Type *base;
	void (*irq_config_func)(struct device *dev);
	struct spi_config default_cfg;
	struct {
		u8_t sck;
		u8_t mosi;
		u8_t miso;
#define SS_UNUSED              255
		/* Pin number of up to 4 slave devices */
		u8_t ss[4];
	} psel;
	u8_t orc;
};

struct spim_nrf52_data {
	struct k_sem sem;
	struct device *gpio_port;
	u8_t slave;
	u8_t stopped:1;
	u8_t txd:1;
	u8_t rxd:1;
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
	u32_t tx_cnt;
	u32_t rx_cnt;
#endif
};

#define NRF52_SPIM_INT_END SPIM_INTENSET_END_Msk
#define NRF52_SPIM_INT_ENDRX SPIM_INTENSET_ENDRX_Msk
#define NRF52_SPIM_INT_ENDTX SPIM_INTENSET_ENDTX_Msk
#define NRF52_SPIM_ENABLE SPIM_ENABLE_ENABLE_Enabled
#define NRF52_SPIM_DISABLE SPIM_ENABLE_ENABLE_Disabled

static void spim_nrf52_print_cfg_registers(struct device *dev)
{
	const struct spim_nrf52_config *config = dev->config->config_info;
	volatile NRF_SPIM_Type *spim = config->base;

	SYS_LOG_DBG("\n"
		"SHORTS=0x%x INT=0x%x FREQUENCY=0x%x CONFIG=0x%x\n"
		"ENABLE=0x%x SCKPIN=%d MISOPIN=%d MOSIPIN=%d\n"
		"RXD.(PTR=0x%x MAXCNT=0x%x AMOUNT=0x%x)\n"
		"TXD.(PTR=0x%x MAXCNT=0x%x AMOUNT=0x%x)\n",
		spim->SHORTS, spim->INTENSET, spim->FREQUENCY, spim->CONFIG,
		spim->ENABLE, spim->PSEL.SCK, spim->PSEL.MISO, spim->PSEL.MOSI,
		spim->RXD.PTR, spim->RXD.MAXCNT, spim->RXD.AMOUNT,
		spim->TXD.PTR, spim->TXD.MAXCNT, spim->TXD.AMOUNT);

	/* maybe unused */
	(void)spim;
}

static int spim_nrf52_configure(struct device *dev,
				struct spi_config *spi_config)
{
	const struct spim_nrf52_config *config = dev->config->config_info;
	struct spim_nrf52_data *data = dev->driver_data;
	volatile NRF_SPIM_Type *spim = config->base;
	u32_t flags;

	SYS_LOG_DBG("config=0x%x max_sys_freq=%d", spi_config->config,
		    spi_config->max_sys_freq);

	/* make sure SPIM block is off */
	spim->ENABLE = NRF52_SPIM_DISABLE;

	spim->INTENCLR = 0xffffffffUL;

	spim->SHORTS = 0;

	spim->ORC = config->orc;

	spim->TXD.LIST = 0;
	spim->RXD.LIST = 0;

	spim->TXD.MAXCNT = 0;
	spim->RXD.MAXCNT = 0;

	spim->EVENTS_END = 0;
	spim->EVENTS_ENDTX = 0;
	spim->EVENTS_ENDRX = 0;
	spim->EVENTS_STOPPED = 0;
	spim->EVENTS_STARTED = 0;

	data->stopped = 1;
	data->txd = 0;
	data->rxd = 0;
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
	data->tx_cnt = 0;
	data->rx_cnt = 0;
#endif

	switch (spi_config->max_sys_freq) {
	case 125000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K125;
		break;
	case 250000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K250;
		break;
	case 500000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_K500;
		break;
	case 1000000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M1;
		break;
	case 2000000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M2;
		break;
	case 4000000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M4;
		break;
	case 8000000:
		spim->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8;
		break;
	default:
		SYS_LOG_ERR("unsupported frequency sck=%d\n",
			    spi_config->max_sys_freq);
		return -EINVAL;
	}

	flags = spi_config->config;

	/* nrf5 supports only 8 bit word size */
	if (SPI_WORD_SIZE_GET(flags) != 8) {
		SYS_LOG_ERR("unsupported word size\n");
		return -EINVAL;
	}

	if (flags & SPI_MODE_LOOP) {
		SYS_LOG_ERR("loopback unsupported\n");
		return -EINVAL;
	}

	spim->CONFIG = (flags & SPI_TRANSFER_LSB) ?
		       (SPIM_CONFIG_ORDER_LsbFirst << SPIM_CONFIG_ORDER_Pos) :
		       (SPIM_CONFIG_ORDER_MsbFirst << SPIM_CONFIG_ORDER_Pos);
	spim->CONFIG |= (flags & SPI_MODE_CPOL) ?
			(SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) :
			(SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos);
	spim->CONFIG |= (flags & SPI_MODE_CPHA) ?
			(SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos) :
			(SPIM_CONFIG_CPHA_Leading << SPIM_CONFIG_CPHA_Pos);

	spim->INTENSET = NRF52_SPIM_INT_END;

	spim_nrf52_print_cfg_registers(dev);

	return 0;
}

static int spim_nrf52_slave_select(struct device *dev, u32_t slave)
{
	struct spim_nrf52_data *data = dev->driver_data;
	const struct spim_nrf52_config *config = dev->config->config_info;

	__ASSERT((slave > 0) && (slave <= 4), "slave=%d", slave);

	slave--;

	if (config->psel.ss[slave] == SS_UNUSED) {
		SYS_LOG_ERR("Slave %d is not configured\n", slave);
		return -EINVAL;
	}

	data->slave = slave;

	return 0;
}

static inline void spim_nrf52_csn(struct device *gpio_port, u32_t pin,
				  bool select)
{
	int status;

	status = gpio_pin_write(gpio_port, pin, select ? 0x0 : 0x1);
	__ASSERT_NO_MSG(status == 0);
}

static int spim_nrf52_transceive(struct device *dev, const void *tx_buf,
				 u32_t tx_buf_len, void *rx_buf,
				 u32_t rx_buf_len)
{
	const struct spim_nrf52_config *config = dev->config->config_info;
	struct spim_nrf52_data *data = dev->driver_data;
	volatile NRF_SPIM_Type *spim = config->base;

	SYS_LOG_DBG("transceive tx_buf=0x%p rx_buf=0x%p tx_len=0x%x "
		    "rx_len=0x%x\n", tx_buf, rx_buf, tx_buf_len, rx_buf_len);

	if (spim->ENABLE) {
		return -EALREADY;
	}
	spim->ENABLE = NRF52_SPIM_ENABLE;

	__ASSERT_NO_MSG(data->stopped);
	data->stopped = 0;

	if (tx_buf_len) {
		__ASSERT_NO_MSG(tx_buf);
		spim->TXD.MAXCNT = tx_buf_len;
		spim->TXD.PTR = (u32_t)tx_buf;
		data->txd = 0;
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
		data->tx_cnt = 0;
#endif
	} else {
		spim->TXD.MAXCNT = 0;
	}

	if (rx_buf_len) {
		__ASSERT_NO_MSG(rx_buf);
		spim->RXD.MAXCNT = rx_buf_len;
		spim->RXD.PTR = (u32_t)rx_buf;
		data->rxd = 0;
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
		data->rx_cnt = 0;
#endif
	} else {
		spim->RXD.MAXCNT = 0;
	}

	if (data->slave != SS_UNUSED) {
		spim_nrf52_csn(data->gpio_port, config->psel.ss[data->slave],
			       true);
	}

	spim->INTENSET = NRF52_SPIM_INT_END;

	SYS_LOG_DBG("spi_xfer %s/%s CS%d\n", rx_buf_len ? "R" : "-",
		    tx_buf_len ? "W" : "-", data->slave);

	/* start SPI transfer transaction */
	spim->TASKS_START = 1;

	/* Wait for the transfer to complete */
	k_sem_take(&data->sem, K_FOREVER);

	if (data->slave != SS_UNUSED) {
		spim_nrf52_csn(data->gpio_port, config->psel.ss[data->slave],
			       false);
	}

	/* Disable SPIM block for power saving */
	spim->INTENCLR = 0xffffffffUL;
	spim->ENABLE = NRF52_SPIM_DISABLE;

#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
	SYS_LOG_DBG("xfer complete rx_cnt=0x%x tx_cnt=0x%x rxd=%d txd=%d "
		    "stopped=%d\n", data->rx_cnt, data->tx_cnt, data->rxd,
		    data->txd, data->stopped);
#endif

	return 0;
}

static void spim_nrf52_isr(void *arg)
{
	struct device *dev = arg;
	const struct spim_nrf52_config *config = dev->config->config_info;
	struct spim_nrf52_data *data = dev->driver_data;
	volatile NRF_SPIM_Type *spim = config->base;

	if (spim->EVENTS_END) {
		data->rxd = 1;
		data->txd = 1;

		/* assume spi transaction has stopped */
		data->stopped = 1;

		/* Cortex M4 specific EVENTS register clearing requires 4 cycles
		 * delayto avoid re-triggering of interrupt. Call to
		 * k_sem_give() will ensure this limit.
		 */
		spim->EVENTS_END = 0;

#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_INFO)
		data->rx_cnt = spim->RXD.AMOUNT;
		data->tx_cnt = spim->TXD.AMOUNT;
		SYS_LOG_DBG("endrxtx rx_cnt=%d tx_cnt=%d", data->rx_cnt,
			    data->tx_cnt);
#endif
		k_sem_give(&data->sem);
	}
}

static int spim_nrf52_init(struct device *dev)
{
	const struct spim_nrf52_config *config = dev->config->config_info;
	struct spim_nrf52_data *data = dev->driver_data;
	volatile NRF_SPIM_Type *spim = config->base;
	int status;
	int i;

	SYS_LOG_DBG("%s", dev->config->name);

	data->gpio_port = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	k_sem_init(&data->sem, 0, UINT_MAX);

	for (i = 0; i < sizeof(config->psel.ss); i++) {
		if (config->psel.ss[i] != SS_UNUSED) {
			status = gpio_pin_configure(data->gpio_port,
						    config->psel.ss[i],
						    GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
			__ASSERT_NO_MSG(status == 0);

			spim_nrf52_csn(data->gpio_port, config->psel.ss[i],
				       false);
			SYS_LOG_DBG("CS%d=%d\n", i, config->psel.ss[i]);
		}
	}

	data->slave = SS_UNUSED;

	status = gpio_pin_configure(data->gpio_port, config->psel.sck,
				    GPIO_DIR_OUT);
	__ASSERT_NO_MSG(status == 0);

	status = gpio_pin_configure(data->gpio_port, config->psel.mosi,
				    GPIO_DIR_OUT);
	__ASSERT_NO_MSG(status == 0);

	status = gpio_pin_configure(data->gpio_port, config->psel.miso,
				    GPIO_DIR_IN);
	__ASSERT_NO_MSG(status == 0);

	spim->PSEL.SCK = config->psel.sck;
	spim->PSEL.MOSI = config->psel.mosi;
	spim->PSEL.MISO = config->psel.miso;

	status = spim_nrf52_configure(dev, (void *)&config->default_cfg);
	if (status) {
		return status;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct spi_driver_api spim_nrf52_driver_api = {
	.configure = spim_nrf52_configure,
	.slave_select = spim_nrf52_slave_select,
	.transceive = spim_nrf52_transceive,
};

/* i2c & spi (SPIM, SPIS, SPI) instance with the same id (e.g. I2C_0 and SPI_0)
 * can NOT be used at the same time on nRF5x chip family.
 */
#if defined(CONFIG_SPIM0_NRF52) && !defined(CONFIG_I2C_0)
static void spim_nrf52_config_func_0(struct device *dev);

static const struct spim_nrf52_config spim_nrf52_config_0 = {
	.base = NRF_SPIM0,
	.irq_config_func = spim_nrf52_config_func_0,
	.default_cfg = {
		.config = CONFIG_SPI_0_DEFAULT_CFG,
		.max_sys_freq = CONFIG_SPI_0_DEFAULT_BAUD_RATE,
	},
	.psel = {
		.sck = CONFIG_SPIM0_NRF52_GPIO_SCK_PIN,
		.mosi = CONFIG_SPIM0_NRF52_GPIO_MOSI_PIN,
		.miso = CONFIG_SPIM0_NRF52_GPIO_MISO_PIN,
		.ss = { CONFIG_SPIM0_NRF52_GPIO_SS_PIN_0,
			CONFIG_SPIM0_NRF52_GPIO_SS_PIN_1,
			CONFIG_SPIM0_NRF52_GPIO_SS_PIN_2,
			CONFIG_SPIM0_NRF52_GPIO_SS_PIN_3 },
	},
	.orc = CONFIG_SPIM0_NRF52_ORC,
};

static struct spim_nrf52_data spim_nrf52_data_0;

DEVICE_AND_API_INIT(spim_nrf52_0, CONFIG_SPI_0_NAME, spim_nrf52_init,
		    &spim_nrf52_data_0, &spim_nrf52_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spim_nrf52_driver_api);

static void spim_nrf52_config_func_0(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI0_TWI0_IRQn, CONFIG_SPI_0_IRQ_PRI,
		    spim_nrf52_isr, DEVICE_GET(spim_nrf52_0), 0);

	irq_enable(NRF5_IRQ_SPI0_TWI0_IRQn);
}
#endif /* CONFIG_SPIM0_NRF52 && !CONFIG_I2C_0 */

#if defined(CONFIG_SPIM1_NRF52) && !defined(CONFIG_I2C_1)
static void spim_nrf52_config_func_1(struct device *dev);

static const struct spim_nrf52_config spim_nrf52_config_1 = {
	.base = NRF_SPIM1,
	.irq_config_func = spim_nrf52_config_func_1,
	.default_cfg = {
		.config = CONFIG_SPI_1_DEFAULT_CFG,
		.max_sys_freq = CONFIG_SPI_1_DEFAULT_BAUD_RATE,
	},
	.psel = {
		.sck = CONFIG_SPIM1_NRF52_GPIO_SCK_PIN,
		.mosi = CONFIG_SPIM1_NRF52_GPIO_MOSI_PIN,
		.miso = CONFIG_SPIM1_NRF52_GPIO_MISO_PIN,
		.ss = { CONFIG_SPIM1_NRF52_GPIO_SS_PIN_0,
			CONFIG_SPIM1_NRF52_GPIO_SS_PIN_1,
			CONFIG_SPIM1_NRF52_GPIO_SS_PIN_2,
			CONFIG_SPIM1_NRF52_GPIO_SS_PIN_3 },
	},
	.orc = CONFIG_SPIM1_NRF52_ORC,
};

static struct spim_nrf52_data spim_nrf52_data_1;

DEVICE_AND_API_INIT(spim_nrf52_1, CONFIG_SPI_1_NAME, spim_nrf52_init,
		    &spim_nrf52_data_1, &spim_nrf52_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &spim_nrf52_driver_api);

static void spim_nrf52_config_func_1(struct device *dev)
{
	IRQ_CONNECT(NRF5_IRQ_SPI1_TWI1_IRQn, CONFIG_SPI_1_IRQ_PRI,
		    spim_nrf52_isr, DEVICE_GET(spim_nrf52_1), 0);

	irq_enable(NRF5_IRQ_SPI1_TWI1_IRQn);
}
#endif /* CONFIG_SPIM1_NRF52 && !CONFIG_I2C_1 */

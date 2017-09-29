/* spis_nrf5.c - SPI slave driver for Nordic nRF5x SoCs */
/*
 * Copyright (c) 2016, 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SPI_LEVEL

#include <device.h>
#include <errno.h>

#include <logging/sys_log.h>
#include <sys_io.h>
#include <board.h>
#include <init.h>
#include <gpio.h>
#include <spi.h>
#include <nrf.h>
#include <toolchain.h>

typedef void (*spis_nrf5_config_t)(void);

struct spis_nrf5_config {
	NRF_SPIS_Type *regs;                    /* Registers */
	spis_nrf5_config_t config_func;         /* IRQ config func pointer */
	u8_t sck_pin;                           /* SCK GPIO pin number */
	u8_t mosi_pin;                          /* MOSI GPIO pin number */
	u8_t miso_pin;                          /* MISO GPIO pin number */
	u8_t csn_pin;                           /* CSN GPIO pin number */
	u8_t def;                               /* Default character */
};

struct spis_nrf5_data {
	u8_t error;
	struct k_sem device_sync_sem;           /* synchronisation semaphore */
};

#define DEV_CFG(dev)	\
	((const struct spis_nrf5_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)	\
	((struct spis_nrf5_data * const)(dev)->driver_data)
#define SPI_REGS(dev)	\
	((DEV_CFG(dev))->regs)

/* Register fields */

#define NRF5_SPIS_SHORTCUT_END_ACQUIRE \
	(SPIS_SHORTS_END_ACQUIRE_Enabled << SPIS_SHORTS_END_ACQUIRE_Pos)

#define NRF5_SPIS_ORDER_MSB \
	(SPIS_CONFIG_ORDER_MsbFirst << SPIS_CONFIG_ORDER_Pos)
#define NRF5_SPIS_ORDER_LSB \
	(SPIS_CONFIG_ORDER_LsbFirst << SPIS_CONFIG_ORDER_Pos)

#define NRF5_SPIS_CPHA_LEADING \
	(SPIS_CONFIG_CPHA_Leading << SPIS_CONFIG_CPHA_Pos)
#define NRF5_SPIS_CPHA_TRAILING \
	(SPIS_CONFIG_CPHA_Trailing << SPIS_CONFIG_CPHA_Pos)

#define NRF5_SPIS_CPOL_HIGH \
	(SPIS_CONFIG_CPOL_ActiveHigh << SPIS_CONFIG_CPOL_Pos)
#define NRF5_SPIS_CPOL_LOW \
	(SPIS_CONFIG_CPOL_ActiveLow << SPIS_CONFIG_CPOL_Pos)

#define NRF5_SPIS_ENABLED \
	(SPIS_ENABLE_ENABLE_Enabled << SPIS_ENABLE_ENABLE_Pos)

#define NRF5_SPIS_CSN_DISABLED_CFG 0xff   /* CS disabled value from Kconfig */
#if defined(CONFIG_SOC_SERIES_NRF51X)
#define NRF5_SPIS_CSN_DISABLED (~0U)      /* CS disabled register value */
#elif defined(CONFIG_SOC_SERIES_NRF52X)
#define NRF5_SPIS_CSN_DISABLED (1U << 31) /* CS disabled register value */
#endif

static inline bool is_buf_in_ram(const void *buf)
{
	return ((((uintptr_t) buf) & 0xE0000000) == 0x20000000);
}

static void spis_nrf5_print_cfg_registers(struct device *dev)
{
	__unused NRF_SPIS_Type *regs = SPI_REGS(dev);
	__unused u32_t sck, miso, mosi, csn;
	__unused u32_t rxd_ptr, rxd_max, rxd_amount;
	__unused u32_t txd_ptr, txd_max, txd_amount;

#if defined(CONFIG_SOC_SERIES_NRF51X)
	sck = regs->PSELSCK;
	miso = regs->PSELMISO;
	mosi = regs->PSELMOSI;
	csn = regs->PSELCSN;
	rxd_ptr = regs->RXDPTR;
	rxd_max = regs->MAXRX;
	rxd_amount = regs->AMOUNTRX;
	txd_ptr = regs->TXDPTR;
	txd_max = regs->MAXTX;
	txd_amount = regs->AMOUNTTX;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
	sck = regs->PSEL.SCK;
	miso = regs->PSEL.MISO;
	mosi = regs->PSEL.MOSI;
	csn = regs->PSEL.CSN;
	rxd_ptr = regs->RXD.PTR;
	rxd_max = regs->RXD.MAXCNT;
	rxd_amount = regs->RXD.AMOUNT;
	txd_ptr = regs->TXD.PTR;
	txd_max = regs->TXD.MAXCNT;
	txd_amount = regs->TXD.AMOUNT;
#endif	/* defined(CONFIG_SOC_SERIES_NRF51X) */

	SYS_LOG_DBG("\n"
		    "SHORTS: %x, IRQ: %x, SEMSTAT: %x\n"
		    "CONFIG: %x, STATUS: %x, ENABLE: %x\n"
		    "SCKPIN: %x, MISOPIN: %x, MOSIPIN: %x, CSNPIN: %x\n"
		    "RXD (PTR: %x, MAX: %x, AMOUNT: %x)\n"
		    "TXD (PTR: %x, MAX: %x, AMOUNT: %x)",
		    regs->SHORTS, regs->INTENSET, regs->SEMSTAT,
		    regs->CONFIG, regs->STATUS, regs->ENABLE,
		    sck, miso, mosi, csn,
		    rxd_ptr, rxd_max, rxd_amount,
		    txd_ptr, txd_max, txd_amount);
}

/**
 * @brief Configure the SPI host controller for operating against slaves
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to the application provided configuration
 *
 * @return 0 if successful, another DEV_* code otherwise.
 */
static int spis_nrf5_configure(struct device *dev, struct spi_config *config)
{
	NRF_SPIS_Type *spi_regs = SPI_REGS(dev);
	u32_t flags;

	/* make sure module is disabled */
	spi_regs->ENABLE = 0;

	/* TODO: Muck with IRQ priority if needed */

	/* Clear any pending events */
	spi_regs->EVENTS_ACQUIRED = 0;
	spi_regs->EVENTS_ENDRX = 0;
	spi_regs->EVENTS_END = 0;
	spi_regs->INTENCLR = 0xFFFFFFFF; /* do we need to clear INT ?*/
	spi_regs->SHORTS = NRF5_SPIS_SHORTCUT_END_ACQUIRE;
	spi_regs->INTENSET |= (SPIS_INTENSET_ACQUIRED_Msk |
			       SPIS_INTENSET_END_Msk);

	/* default transmit and over-read characters */
	spi_regs->DEF = DEV_CFG(dev)->def;
	spi_regs->ORC = 0x000000AA;

	/* user configuration */
	flags = config->config;
	if (flags & SPI_TRANSFER_LSB) {
		spi_regs->CONFIG = NRF5_SPIS_ORDER_LSB;
	} else {
		spi_regs->CONFIG = NRF5_SPIS_ORDER_MSB;
	}
	if (flags & SPI_MODE_CPHA) {
		spi_regs->CONFIG |= NRF5_SPIS_CPHA_TRAILING;
	} else {
		spi_regs->CONFIG |= NRF5_SPIS_CPHA_LEADING;
	}
	if (flags & SPI_MODE_CPOL) {
		spi_regs->CONFIG |= NRF5_SPIS_CPOL_LOW;
	} else {
		spi_regs->CONFIG |= NRF5_SPIS_CPOL_HIGH;
	}

	/* Enable the SPIS - peripherals sharing same ID will be disabled */
	spi_regs->ENABLE = NRF5_SPIS_ENABLED;

	spis_nrf5_print_cfg_registers(dev);

	SYS_LOG_DBG("SPI Slave Driver configured");

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
static int spis_nrf5_transceive(struct device *dev, const void *tx_buf,
			 u32_t tx_buf_len, void *rx_buf, u32_t rx_buf_len)
{
	NRF_SPIS_Type *spi_regs = SPI_REGS(dev);
	struct spis_nrf5_data *priv_data = DEV_DATA(dev);

	__ASSERT(!((tx_buf_len && !tx_buf) || (rx_buf_len && !rx_buf)),
		"spis_nrf5_transceive: ERROR - NULL buffers");

	/* Buffer needs to be in RAM for EasyDMA to work */
	if (!tx_buf || !is_buf_in_ram(tx_buf)) {
		SYS_LOG_ERR("Invalid TX buf %p", tx_buf);
		return -EINVAL;
	}
	if (!rx_buf || !is_buf_in_ram(rx_buf)) {
		SYS_LOG_ERR("Invalid RX buf %p", rx_buf);
		return -EINVAL;
	}

	priv_data->error = 0;

	if (spi_regs->SEMSTAT == 1) {
#if defined(CONFIG_SOC_SERIES_NRF51X)
		spi_regs->TXDPTR = (u32_t) tx_buf;
		spi_regs->RXDPTR = (u32_t) rx_buf;
		spi_regs->MAXTX = tx_buf_len;
		spi_regs->MAXRX = rx_buf_len;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
		spi_regs->TXD.PTR = (u32_t) tx_buf;
		spi_regs->RXD.PTR = (u32_t) rx_buf;
		spi_regs->TXD.MAXCNT = tx_buf_len;
		spi_regs->RXD.MAXCNT = rx_buf_len;
#endif
		spi_regs->TASKS_RELEASE = 1;
	} else {
		SYS_LOG_ERR("Can't get SEM; unfinished transfer?");
		return -EIO;
	}

	/* wait for transfer to complete */
	k_sem_take(&priv_data->device_sync_sem, K_FOREVER);

	if (priv_data->error) {
		priv_data->error = 0;
		return -EIO;
	}

	return 0;
}

/**
 * @brief Complete SPI module data transfer operations.
 * @param dev Pointer to the device structure for the driver instance
 * @param error Error condition (0 = no error, otherwise an error occurred)
 * @return None.
 */
static void spis_nrf5_complete(struct device *dev, u32_t error)
{
	__unused NRF_SPIS_Type *spi_regs = SPI_REGS(dev);
	__unused u32_t txd_amount, rxd_amount;
	struct spis_nrf5_data *priv_data = DEV_DATA(dev);

#if defined(CONFIG_SOC_SERIES_NRF51X)
	txd_amount = spi_regs->AMOUNTTX;
	rxd_amount = spi_regs->AMOUNTRX;
#elif defined(CONFIG_SOC_SERIES_NRF52X)
	txd_amount = spi_regs->RXD.AMOUNT;
	rxd_amount = spi_regs->TXD.AMOUNT;
#endif

	SYS_LOG_DBG("bytes transferred: TX: %u, RX: %u [err %u (%s)]",
		    txd_amount, rxd_amount,
		    error, error == 0 ? "OK" : "ERR");

	priv_data->error = error ? 1 : 0;

	k_sem_give(&priv_data->device_sync_sem);
}

/**
 * @brief SPI module interrupt handler.
 * @param arg Pointer to the device structure for the driver instance
 * @return None.
 */
static void spis_nrf5_isr(void *arg)
{
	struct device *dev = arg;
	NRF_SPIS_Type *spi_regs = SPI_REGS(dev);
	u32_t error;
	u32_t tmp;

	/* We get an interrupt for the following reasons:
	 * 1. Semaphore ACQUIRED:
	 *      Semaphore is assigned to the CPU again (always happens
	 *      after END if the END_ACQUIRE SHORTS is set)
	 * 2. End of Granted SPI transaction:
	 *      Used to unblocked the caller, finishing the transaction
	 */

	/* NOTE:
	 * Section 15.8.1 of nrf52 manual suggests reading back the register
	 * to cause a 4-cycle delay to prevent the interrupt from
	 * re-occurring
	 */

	if (spi_regs->EVENTS_END) {
		spi_regs->EVENTS_END = 0;
		/* force register flush (per spec) */
		tmp = spi_regs->EVENTS_END;

		/* Read and clear error flags. */
		error = spi_regs->STATUS;
		spi_regs->STATUS = error;

		spis_nrf5_complete(dev, error);
	}
	if (spi_regs->EVENTS_ACQUIRED) {
		spi_regs->EVENTS_ACQUIRED = 0;
		/* force registesr flush (per spec) */
		tmp = spi_regs->EVENTS_ACQUIRED;
	}
}

static const struct spi_driver_api nrf5_spis_api = {
	.transceive = spis_nrf5_transceive,
	.configure = spis_nrf5_configure,
	.slave_select = NULL,
};

#if defined(CONFIG_SOC_SERIES_NRF51X)
static void spis_configure_psel(NRF_SPIS_Type *spi_regs,
				const struct spis_nrf5_config *cfg)
{
	spi_regs->PSELMOSI = cfg->mosi_pin;
	spi_regs->PSELMISO = cfg->miso_pin;
	spi_regs->PSELSCK  = cfg->sck_pin;
	if (cfg->csn_pin == NRF5_SPIS_CSN_DISABLED_CFG) {
		spi_regs->PSELCSN = NRF5_SPIS_CSN_DISABLED;
	} else {
		spi_regs->PSELCSN = cfg->csn_pin;
	}
}
#elif defined(CONFIG_SOC_SERIES_NRF52X)
static void spis_configure_psel(NRF_SPIS_Type *spi_regs,
				const struct spis_nrf5_config *cfg)
{
	spi_regs->PSEL.MOSI = cfg->mosi_pin;
	spi_regs->PSEL.MISO = cfg->miso_pin;
	spi_regs->PSEL.SCK = cfg->sck_pin;
	if (cfg->csn_pin == NRF5_SPIS_CSN_DISABLED_CFG) {
		spi_regs->PSEL.CSN = NRF5_SPIS_CSN_DISABLED;
	} else {
		spi_regs->PSEL.CSN = cfg->csn_pin;
	}
}
#else
#error "Unsupported NRF5 SoC"
#endif

static int spis_nrf5_init(struct device *dev)
{
	NRF_SPIS_Type *spi_regs = SPI_REGS(dev);
	struct spis_nrf5_data *priv_data = DEV_DATA(dev);
	const struct spis_nrf5_config *cfg = DEV_CFG(dev);
	struct device *gpio_dev;
	int ret;

	SYS_LOG_DBG("SPI Slave driver init: %p", dev);

	/* Enable constant latency for faster SPIS response */
	NRF_POWER->TASKS_CONSTLAT = 1;

	spi_regs->ENABLE = 0;

	gpio_dev = device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	ret = gpio_pin_configure(gpio_dev, cfg->miso_pin,
				  GPIO_DIR_IN | GPIO_PUD_NORMAL);
	__ASSERT_NO_MSG(!ret);

	ret = gpio_pin_configure(gpio_dev, cfg->mosi_pin,
				  GPIO_DIR_IN | GPIO_PUD_NORMAL);
	__ASSERT_NO_MSG(!ret);

	ret = gpio_pin_configure(gpio_dev, cfg->sck_pin,
				  GPIO_DIR_IN | GPIO_PUD_NORMAL);
	__ASSERT_NO_MSG(!ret);

	if (cfg->csn_pin != 0xff) {
		ret = gpio_pin_configure(gpio_dev, cfg->csn_pin,
					 GPIO_DIR_IN | GPIO_PUD_PULL_UP);
		__ASSERT_NO_MSG(!ret);
	}

	spis_configure_psel(spi_regs, cfg);

	cfg->config_func();

	k_sem_init(&priv_data->device_sync_sem, 0, 1);

	SYS_LOG_DBG("SPI Slave driver initialized on device: %p", dev);

	return 0;
}

/* system bindings */
#ifdef CONFIG_SPIS0_NRF52

static void spis_config_irq_0(void);

static struct spis_nrf5_data spis_nrf5_data_0;

static const struct spis_nrf5_config spis_nrf5_config_0 = {
	.regs = NRF_SPIS0,
	.config_func = spis_config_irq_0,
	.sck_pin = CONFIG_SPIS0_NRF52_GPIO_SCK_PIN,
	.mosi_pin = CONFIG_SPIS0_NRF52_GPIO_MOSI_PIN,
	.miso_pin = CONFIG_SPIS0_NRF52_GPIO_MISO_PIN,
	.csn_pin = CONFIG_SPIS0_NRF52_GPIO_CSN_PIN,
	.def = CONFIG_SPIS0_NRF52_DEF,
};

DEVICE_AND_API_INIT(spis_nrf5_port_0, CONFIG_SPI_0_NAME, spis_nrf5_init,
		    &spis_nrf5_data_0, &spis_nrf5_config_0, PRE_KERNEL_1,
		    CONFIG_SPI_INIT_PRIORITY, &nrf5_spis_api);

static void spis_config_irq_0(void)
{
	IRQ_CONNECT(NRF5_IRQ_SPI0_TWI0_IRQn, CONFIG_SPI_0_IRQ_PRI,
		    spis_nrf5_isr, DEVICE_GET(spis_nrf5_port_0), 0);
	irq_enable(NRF5_IRQ_SPI0_TWI0_IRQn);
}

#endif /* CONFIG_SPIS0_NRF52 */

#ifdef CONFIG_SPIS1_NRF5

static void spis_config_irq_1(void);

static struct spis_nrf5_data spis_nrf5_data_1;

static const struct spis_nrf5_config spis_nrf5_config_1 = {
	.regs = NRF_SPIS1,
	.config_func = spis_config_irq_1,
	.sck_pin = CONFIG_SPIS1_NRF5_GPIO_SCK_PIN,
	.mosi_pin = CONFIG_SPIS1_NRF5_GPIO_MOSI_PIN,
	.miso_pin = CONFIG_SPIS1_NRF5_GPIO_MISO_PIN,
	.csn_pin = CONFIG_SPIS1_NRF5_GPIO_CSN_PIN,
	.def = CONFIG_SPIS1_NRF5_DEF,
};

DEVICE_AND_API_INIT(spis_nrf5_port_1, CONFIG_SPI_1_NAME, spis_nrf5_init,
		    &spis_nrf5_data_1, &spis_nrf5_config_1, PRE_KERNEL_1,
		    CONFIG_SPI_INIT_PRIORITY, &nrf5_spis_api);

static void spis_config_irq_1(void)
{
	IRQ_CONNECT(NRF5_IRQ_SPI1_TWI1_IRQn, CONFIG_SPI_1_IRQ_PRI,
		    spis_nrf5_isr, DEVICE_GET(spis_nrf5_port_1), 0);
	irq_enable(NRF5_IRQ_SPI1_TWI1_IRQn);
}

#endif /* CONFIG_SPIS1_NRF5 */

/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_cc27xx_spi

#if !defined(CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN)
#error "spi_cc23x0_cc27xx driver requires CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN to be enabled"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_cc23x0_cc27xx, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <driverlib/clkctl.h>
#include <driverlib/gpio.h>
#include <driverlib/spi.h>
#include <driverlib/udma.h>

#include <inc/hw_memmap.h>

#include "spi_context.h"

/**
 * This define must match the value in the pinctrl-names for cs_sw_controlled,
 * and the value must be greater than or equal to PINCTRL_STATE_PRIV_START.
 */
#define PINCTRL_STATE_CS_SW_CONTROLLED PINCTRL_STATE_PRIV_START + 0

/*
 * SPI bit rate = (SPI functional clock frequency) / ((SCR + 1) * 2)
 * Serial clock divider value (SCR) can be from 0 to 1023.
 */
#define SPI_CC23X0_CC27XX_MIN_FREQ DIV_ROUND_UP(TI_CC23X0_CC27XX_DT_CPU_CLK_FREQ_HZ, 2048)
#define SPI_CC23X0_CC27XX_MAX_FREQ (TI_CC23X0_CC27XX_DT_CPU_CLK_FREQ_HZ >> 1)

#define SPI_CC23X0_CC27XX_DATA_WIDTH 8
#define SPI_CC23X0_CC27XX_DFS        (SPI_CC23X0_CC27XX_DATA_WIDTH >> 3)

#define SPI_CC23X0_CC27XX_DUMMY_DATA 0xFFFFFFFF

#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
#define SPI_CC23X0_CC27XX_REG_GET(base, offset) ((base) + (offset))
#define SPI_CC23X0_CC27XX_INT_MASK              (SPI_DMA_DONE_RX | SPI_RXFIFO_OVF | SPI_DMA_DONE_TX)
#endif /* CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN */

#define SPI_CC23X0_CC27XX_STATUS_SUCCESS                         0
#define SPI_CC23X0_CC27XX_STATUS_ERROR                           -1
#define SPI_CC23X0_CC27XX_STATUS_ERROR_RX_FIFO_OVERFLOW          -2
#define SPI_CC23X0_CC27XX_STATUS_ERROR_MAX_TRANSFER_AMT_EXCEEDED -3
#define SPI_CC23X0_CC27XX_STATUS_ERROR_DMA_NOT_READY             -4

#define SPI_CC23X0_CC27XX_DMA_RX_DONE BIT(0)
#define SPI_CC23X0_CC27XX_DMA_TX_DONE BIT(1)

#define SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT 1024

#define SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE(operation)                                               \
	(IS_ENABLED(CONFIG_SPI_SLAVE) && (SPI_OP_MODE_GET(operation) == SPI_OP_MODE_SLAVE))

enum transfer_direction {
	TRANSFER_DIR_NONE,
	TRANSFER_DIR_TX,
	TRANSFER_DIR_RX,
	TRANSFER_DIR_BOTH
};

static int spi_cc23xo_prime_transceive(const struct device *dev);
static void spi_cc23x0_cc27xx_dma_xfer_config(const struct device *dev);
static int spi_cc23x0_cc27xx_dma_load_tx(const struct device *dev, size_t buf_size);
static int spi_cc23x0_cc27xx_dma_load_rx(const struct device *dev, size_t buf_size);
static int spi_cc23x0_cc27xx_dma_start(const struct device *dev, enum transfer_direction dir);
static void spi_cc23x0_cc27xx_dma_stop(const struct device *dev);

struct spi_cc23x0_cc27xx_config {
	uint32_t base;
	uint32_t instance;
	uint32_t clock_id;
	const struct device *gpio_dev;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(void);
#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
	const struct device *dma_dev;
	uint8_t dma_channel_tx;
	uint8_t dma_trigsrc_tx;
	uint8_t dma_channel_rx;
	uint8_t dma_trigsrc_rx;
#endif
};

struct spi_cc23x0_cc27xx_data {
	struct spi_context ctx;
	struct spi_buf_set *tx_bufs;
	struct spi_buf_set *rx_bufs;
#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
	struct dma_block_config block_cfg_tx;
	struct dma_config dma_cfg_tx;
	struct dma_block_config block_cfg_rx;
	struct dma_config dma_cfg_rx;
	struct k_sem xfer_sync_sem;
	volatile uint32_t dma_tx_xfer_len;
	volatile uint32_t dma_rx_xfer_len;
	volatile uint8_t dma_status_flags;
#endif /* CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN */
	struct gpio_dt_spec sck_gpio;
	struct gpio_dt_spec mosi_gpio;
	struct gpio_dt_spec miso_gpio;
	struct gpio_dt_spec *cs_gpio;
	bool is_cs_sw_controlled;
#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_SPI_SLAVE)
	bool slave_pm_lock_held;
#endif
};

static uint32_t dummy_tx_data = SPI_CC23X0_CC27XX_DUMMY_DATA;
static uint32_t dummy_rx_data;

/**
 * @brief Acquire the PM policy state locks for SPI CC23x0.
 */
static inline void spi_cc23x0_cc27xx_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

/**
 * @brief Release the PM policy state locks for SPI CC23x0.
 */
static inline void spi_cc23x0_cc27xx_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Apply the sleep state for SPI CC23x0 pins.
 *
 * This function configures the SPI pins to their sleep state, which is
 * necessary to ensure that the pins are in a known state during low power
 * modes. It sets the pins to GPIO mode and applies the pinctrl state
 * SLEEP, which is defined in the board's pinctrl configuration.
 *
 * @note In CC23x0, the SPI pins must be set to known states using the GPIO
 * module because the platform's AON domain does not support GPIO latching.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 */
static int spi_cc23x0_cc27xx_pinctrl_apply_sleep_state(const struct device *dev,
						       const struct spi_config *config)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	int ret = 0;

	/*
	 * In cc23x0, the SPI pins must be set to known states using the GPIO module
	 * because the platform's AON domain does not support GPIO latching. This
	 * means that the SPI pins must be configured temporarily as GPIOs before
	 * standby until the core wakes up again.
	 */
	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		gpio_pin_configure_dt(&data->miso_gpio, GPIO_INPUT | GPIO_OUTPUT_LOW);
	} else {
		gpio_pin_configure_dt(&data->sck_gpio, GPIO_INPUT | GPIO_OUTPUT_LOW);
		gpio_pin_configure_dt(&data->mosi_gpio, GPIO_INPUT);

		if (data->cs_gpio != NULL) {
			gpio_pin_configure_dt(data->cs_gpio, GPIO_INPUT | GPIO_OUTPUT_HIGH);
		}
	}

	/*
	 * Configure the pin in IOC (with pin function as regular GPIO). The pinmux
	 * values come from `<board>-pinctrl.dtsi`.
	 */
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_SLEEP);
	if (ret) {
		return ret;
	}

	return ret;
}

#endif /* CONFIG_PM_DEVICE */

/**
 * @brief Apply the active state for SPI CC23x0 pins.
 *
 * This function configures the SPI pins to their active state, allowing
 * the SPI module to drive the pins correctly during operation.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_pinctrl_apply_active_state(const struct device *dev,
							const struct spi_config *config)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	int ret = 0;

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	/**
	 * Setting pin as GPIO_INPUT disables `Output Enable` for the
	 * GPIO pin. The purpose is to disable GPIO latching on that pin.
	 * The SPI module still drives these pins.
	 */
	gpio_pin_configure_dt(&data->sck_gpio, GPIO_INPUT);

	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		gpio_pin_configure_dt(&data->mosi_gpio, GPIO_INPUT);
		gpio_pin_configure_dt(&data->miso_gpio, GPIO_DISCONNECTED);
		if (data->cs_gpio != NULL) {
			gpio_pin_configure_dt(data->cs_gpio, GPIO_INPUT);
		}
	} else {
		gpio_pin_configure_dt(&data->mosi_gpio, GPIO_DISCONNECTED);
		gpio_pin_configure_dt(&data->miso_gpio, GPIO_INPUT);
		if (data->cs_gpio != NULL) {
			gpio_pin_configure_dt(data->cs_gpio, GPIO_INPUT | GPIO_OUTPUT_HIGH);
		}
	}

	return ret;
}

static void spi_cc23x0_cc27xx_reset_txrx_fifos(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;

	/* Reset TX/RX FIFOs */
	HWREG(cfg->base + SPI_O_CTL0) |= SPI_CTL0_FIFORST_RST_TRIG;
}

static void spi_cc23x0_cc27xx_isr(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t status;

	__disable_irq();

	status = SPIIntStatus(cfg->base, true);

	LOG_DBG("status = %08x", status);
	/*
	 * Disabling the interrupts in this ISR when SPI has completed
	 * the transfer triggers a subsequent spurious interrupt, with
	 * a null status. Let's ignore this event.
	 */
	if (!status) {
		__enable_irq();
		return;
	}

	if (status & SPI_RXFIFO_OVF) {
#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
		spi_cc23x0_cc27xx_dma_stop(dev);
		SPIClearInt(cfg->base, SPI_DMA_DONE_RX);
		data->dma_status_flags = 0;
#endif
		SPIClearInt(cfg->base, SPI_RXFIFO_OVF);
		spi_cc23x0_cc27xx_reset_txrx_fifos(dev);
		spi_context_complete(ctx, dev, SPI_CC23X0_CC27XX_STATUS_ERROR_RX_FIFO_OVERFLOW);
	}

#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
	if (status & SPI_DMA_DONE_RX) {
		SPIClearInt(cfg->base, SPI_DMA_DONE_RX);
		data->dma_status_flags |= SPI_CC23X0_CC27XX_DMA_RX_DONE;
	}

	if (status & SPI_DMA_DONE_TX) {
		SPIClearInt(cfg->base, SPI_DMA_DONE_TX);
		data->dma_status_flags |= SPI_CC23X0_CC27XX_DMA_TX_DONE;
	}

#ifdef CONFIG_SPI_SLAVE
	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((data->ctx.config->operation))) {
		bool rx_done = data->dma_status_flags & SPI_CC23X0_CC27XX_DMA_RX_DONE;
		bool tx_done = data->dma_status_flags & SPI_CC23X0_CC27XX_DMA_TX_DONE;

		if (rx_done) {
			spi_context_update_rx(ctx, SPI_CC23X0_CC27XX_DFS,
					      ctx->rx_len ? data->dma_rx_xfer_len : 0);
		}

		if (tx_done) {
			spi_context_update_tx(ctx, SPI_CC23X0_CC27XX_DFS,
					      ctx->tx_len ? data->dma_tx_xfer_len : 0);
		}

		if (!spi_context_rx_on(ctx) && !spi_context_tx_on(ctx)) {
			/* All data transferred - complete the transaction */
			spi_cc23x0_cc27xx_dma_stop(dev);
			SPIDisableInt(cfg->base, SPI_CC23X0_CC27XX_INT_MASK);
			data->dma_status_flags = 0;
			spi_context_release(ctx, SPI_CC23X0_CC27XX_STATUS_SUCCESS);
			spi_cc23x0_cc27xx_pm_policy_state_lock_put();
			spi_context_complete(ctx, dev, SPI_CC23X0_CC27XX_STATUS_SUCCESS);
			__enable_irq();
			return;
		}

		/*
		 * Process stops before re-arms. If a direction is done and has no
		 * more data, its DMA trigger must be disabled before the other
		 * direction is re-armed. This prevents the SPI from stalling when
		 * one DMA trigger is active but its channel has no more data.
		 */
		if (rx_done && !spi_context_rx_buf_on(ctx)) {
			SPIDisableDMA(cfg->base, SPI_DMA_RX);
			dma_stop(cfg->dma_dev, cfg->dma_channel_rx);
			SPIDisableInt(cfg->base, SPI_DMA_DONE_RX);
			data->dma_status_flags &= ~SPI_CC23X0_CC27XX_DMA_RX_DONE;
		}

		if (tx_done && !spi_context_tx_buf_on(ctx)) {
			SPIDisableDMA(cfg->base, SPI_DMA_TX);
			dma_stop(cfg->dma_dev, cfg->dma_channel_tx);
			SPIDisableInt(cfg->base, SPI_DMA_DONE_TX);
			data->dma_status_flags &= ~SPI_CC23X0_CC27XX_DMA_TX_DONE;
		}

		/* Re-arm directions that have more data, now that stale triggers are cleared */
		if (rx_done && spi_context_rx_buf_on(ctx)) {
			data->dma_rx_xfer_len =
				MIN(ctx->rx_len, SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT);
			data->dma_status_flags &= ~SPI_CC23X0_CC27XX_DMA_RX_DONE;
			spi_cc23x0_cc27xx_dma_load_rx(dev, data->dma_rx_xfer_len);
			spi_cc23x0_cc27xx_dma_start(dev, TRANSFER_DIR_RX);
		}

		if (tx_done && spi_context_tx_buf_on(ctx)) {
			data->dma_tx_xfer_len =
				MIN(ctx->tx_len, SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT);
			data->dma_status_flags &= ~SPI_CC23X0_CC27XX_DMA_TX_DONE;
			spi_cc23x0_cc27xx_dma_load_tx(dev, data->dma_tx_xfer_len);
			spi_cc23x0_cc27xx_dma_start(dev, TRANSFER_DIR_TX);
		}
	} else {
#endif /* CONFIG_SPI_SLAVE */
		if (data->dma_status_flags ==
		    (SPI_CC23X0_CC27XX_DMA_RX_DONE | SPI_CC23X0_CC27XX_DMA_TX_DONE)) {
			/* Master mode: proceed only when both TX and RX are done */
			spi_context_update_tx(ctx, SPI_CC23X0_CC27XX_DFS, data->dma_tx_xfer_len);
			spi_context_update_rx(ctx, SPI_CC23X0_CC27XX_DFS, data->dma_rx_xfer_len);

			if (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
				spi_cc23xo_prime_transceive(dev);
			} else {
				spi_cc23x0_cc27xx_dma_stop(dev);
				SPIDisableInt(cfg->base, SPI_CC23X0_CC27XX_INT_MASK);
				spi_context_cs_control(ctx, false);
				spi_context_release(ctx, SPI_CC23X0_CC27XX_STATUS_SUCCESS);
				spi_cc23x0_cc27xx_pm_policy_state_lock_put();
				spi_context_complete(ctx, dev, SPI_CC23X0_CC27XX_STATUS_SUCCESS);
			}
		}
#ifdef CONFIG_SPI_SLAVE
	}
#endif /* CONFIG_SPI_SLAVE */
#endif /* CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN */
	__enable_irq();
}

static int spi_cc23x0_cc27xx_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	uint32_t protocol;
	uint32_t spi_op_mode;
	uint32_t frequency;
	int ret;

	if (spi_context_configured(&data->ctx, config)) {
		/* Nothing to do */
		return 0;
	}

	ret = spi_cc23x0_cc27xx_pinctrl_apply_active_state(dev, config);

	if (ret) {
		return ret;
	}

	frequency = config->frequency;

	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		frequency = SPI_CC23X0_CC27XX_MAX_FREQ;
	}

	if (config->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex is not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		spi_op_mode = SPI_MODE_CONTROLLER;
	} else {
		spi_op_mode = SPI_MODE_PERIPHERAL;
	}

	/* Word sizes other than 8 bits has not been implemented */
	if (SPI_WORD_SIZE_GET(config->operation) != SPI_CC23X0_CC27XX_DATA_WIDTH) {
		LOG_ERR("Word sizes other than %d bits are not supported",
			SPI_CC23X0_CC27XX_DATA_WIDTH);
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Multiple lines are not supported");
		return -EINVAL;
	}

	if (config->operation & SPI_CS_ACTIVE_HIGH && !spi_cs_is_gpio(config)) {
		LOG_ERR("Active high CS requires emulation through a GPIO line");
		return -EINVAL;
	}

	if (frequency < SPI_CC23X0_CC27XX_MIN_FREQ) {
		LOG_ERR("Frequencies lower than %d Hz are not supported",
			SPI_CC23X0_CC27XX_MIN_FREQ);
		return -EINVAL;
	}

	if (frequency > SPI_CC23X0_CC27XX_MAX_FREQ) {
		LOG_ERR("Frequency greater than %d Hz are not supported",
			SPI_CC23X0_CC27XX_MAX_FREQ);
		return -EINVAL;
	}

	/**
	 * If the application provides a GPIO, we configure SPI in 3-wire mode
	 * and use the provided GPIO as chip select.
	 */
	if (!spi_cs_is_gpio(config)) {
		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
			if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
				protocol = SPI_FRF_MOTO_MODE_7;
			} else {
				protocol = SPI_FRF_MOTO_MODE_6;
			}
		} else {
			if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
				protocol = SPI_FRF_MOTO_MODE_5;
			} else {
				protocol = SPI_FRF_MOTO_MODE_4;
			}
		}
	} else {

		if (SPI_MODE_GET(config->operation) & SPI_MODE_CPOL) {
			if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
				protocol = SPI_FRF_MOTO_MODE_3;
			} else {
				protocol = SPI_FRF_MOTO_MODE_2;
			}
		} else {
			if (SPI_MODE_GET(config->operation) & SPI_MODE_CPHA) {
				protocol = SPI_FRF_MOTO_MODE_1;
			} else {
				protocol = SPI_FRF_MOTO_MODE_0;
			}
		}
	}

	/* Enable clock for SPI IP. */
	CLKCTLEnable(CLKCTL_BASE, cfg->clock_id);

	/* Disable SPI before making configuration changes */
	SPIDisable(cfg->base);

	/* Configure SPI */
	SPIConfigSetExpClk(cfg->base, TI_CC23X0_CC27XX_DT_CPU_CLK_FREQ_HZ, protocol, spi_op_mode,
			   frequency, SPI_CC23X0_CC27XX_DATA_WIDTH);

	if (SPI_MODE_GET(config->operation) & SPI_MODE_LOOP) {
		HWREG(cfg->base + SPI_O_CTL1) |= SPI_CTL1_LBM;
	} else {
		HWREG(cfg->base + SPI_O_CTL1) &= ~SPI_CTL1_LBM;
	}

	if (config->operation & SPI_TRANSFER_LSB) {
		HWREG(cfg->base + SPI_O_CTL1) &= ~SPI_CTL1_MSB_MSB;
		HWREG(cfg->base + SPI_O_CTL1) |= SPI_CTL1_MSB_LSB;
	}

	data->ctx.config = config;

	/* Configure TX/RX FIFO level */
#ifdef CONFIG_SOC_SERIES_CC27XX
	/*
	 * Set TX FIFO <= 1/4 empty, and RX FIFO >= 1/2 full (default).
	 * This is a workaround for a DMA errata UDMA_01.
	 */
	HWREG(cfg->base + SPI_O_IFLS) = SPI_IFLS_TXSEL_LVL_1_4 | SPI_IFLS_RXSEL_LVL_1_2;
#endif

	/* Re-enable SPI after making configuration changes */
	SPIEnable(cfg->base);

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_SPI_SLAVE)
	/*
	 * In slave mode the SPI IP must remain clocked for the entire period
	 * the device is configured, not just during an active transfer. The
	 * master can assert CS and send a clock edge at any time, so standby
	 * is not permitted while the slave is active. Acquire the policy lock
	 * once here; it is released in spi_cc23x0_cc27xx_release().
	 */
	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE(config->operation) && !data->slave_pm_lock_held) {
		pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
		pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		data->slave_pm_lock_held = true;
	}
#endif

	return 0;
}

static void spi_cc23x0_cc27xx_initialize_data(const struct device *dev)
{
#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
	spi_cc23x0_cc27xx_dma_xfer_config(dev);
#endif /* CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN */
}

/**
 * @brief Start DMA-driven SPI transfer
 *
 * This function updates the DMA configuration for both TX and RX channels with
 * the SPI TX and RX buffers, and then starts the DMA transfer.
 *
 * DMA TX Channel: Source is the SPI TX buffer, destination is the SPI TX data
 * register.
 * DMA RX Channel: Source is the SPI RX data register, destination is
 * the SPI RX buffer.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 */
static int spi_cc23xo_prime_transceive(const struct device *dev)
{
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;
	size_t xfer_len;

	if (data->ctx.rx_len > 0 || data->ctx.tx_len > 0) {
		if (data->ctx.rx_len == 0) {
			xfer_len = MIN(data->ctx.tx_len,
				       SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT);
		} else if (data->ctx.tx_len == 0) {
			xfer_len = MIN(data->ctx.rx_len,
				       SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT);
		} else {
			xfer_len = MIN(MIN(data->ctx.tx_len, data->ctx.rx_len),
				       SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT);
		}

		data->dma_tx_xfer_len = xfer_len;
		data->dma_rx_xfer_len = xfer_len;

		ret = spi_cc23x0_cc27xx_dma_load_tx(dev, xfer_len);
		if (ret) {
			LOG_ERR("Failed to configure DMA TX channel");
			return ret;
		}

		ret = spi_cc23x0_cc27xx_dma_load_rx(dev, xfer_len);
		if (ret) {
			LOG_ERR("Failed to configure DMA RX channel");
			return ret;
		}

		data->dma_status_flags = 0;
		ret = spi_cc23x0_cc27xx_dma_start(dev, TRANSFER_DIR_BOTH);
	} else {
		spi_context_release(ctx, ret);
	}

	return ret;
}

#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
/**
 * @brief Initialize DMA configuration structs for SPI transfer
 *
 * This helper function initializes the DMA blocks and the DMA configuration
 * structures.
 *
 * @params dev Pointer to the SPI device structure.
 */
static void spi_cc23x0_cc27xx_dma_xfer_config(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;

	data->block_cfg_tx.dest_address = SPI_CC23X0_CC27XX_REG_GET(cfg->base, SPI_O_TXDATA);
	data->block_cfg_tx.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->block_cfg_tx.block_size = 1;
	data->dma_cfg_tx.dma_slot = cfg->dma_trigsrc_tx;
	data->dma_cfg_tx.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma_cfg_tx.block_count = 1;
	data->dma_cfg_tx.head_block = &data->block_cfg_tx;
	data->dma_cfg_tx.source_data_size = SPI_CC23X0_CC27XX_DFS;
	data->dma_cfg_tx.dest_data_size = SPI_CC23X0_CC27XX_DFS;
#ifdef CONFIG_SOC_SERIES_CC27XX
	/*
	 * Set burst length to 2. With this, the dma_config() API, will set
	 * the DMA arbitration size to 2.
	 * This is a workaround for a DMA errata UDMA_01.
	 */
	data->dma_cfg_tx.source_burst_length = SPI_CC23X0_CC27XX_DFS * 2;
#else
	data->dma_cfg_tx.source_burst_length = SPI_CC23X0_CC27XX_DFS;
#endif
	data->dma_cfg_tx.dest_burst_length = data->dma_cfg_tx.source_burst_length;
	data->dma_cfg_tx.dma_callback = NULL;
	data->dma_cfg_tx.user_data = NULL;

	data->block_cfg_rx.source_address = SPI_CC23X0_CC27XX_REG_GET(cfg->base, SPI_O_RXDATA);
	data->block_cfg_rx.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->block_cfg_rx.block_size = 1;
	data->dma_cfg_rx.dma_slot = cfg->dma_trigsrc_rx;
	data->dma_cfg_rx.channel_direction = PERIPHERAL_TO_MEMORY;
	data->dma_cfg_rx.block_count = 1;
	data->dma_cfg_rx.head_block = &data->block_cfg_rx;
	data->dma_cfg_rx.source_data_size = SPI_CC23X0_CC27XX_DFS;
	data->dma_cfg_rx.dest_data_size = SPI_CC23X0_CC27XX_DFS;
	data->dma_cfg_rx.source_burst_length = SPI_CC23X0_CC27XX_DFS;
	data->dma_cfg_rx.dest_burst_length = SPI_CC23X0_CC27XX_DFS;
	data->dma_cfg_rx.dma_callback = NULL;
	data->dma_cfg_rx.user_data = NULL;
}

/**
 * @brief Load and configure the TX DMA channel
 *
 * Sets up the TX DMA block config for the current TX context buffer and
 * calls dma_config. Falls back to dummy TX data if no TX buffer is available.
 *
 * @param dev Pointer to the SPI device structure.
 * @param buf_size Number of frames to transfer.
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_dma_load_tx(const struct device *dev, size_t buf_size)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	data->block_cfg_tx.source_address = (uint32_t)ctx->tx_buf;
	data->block_cfg_tx.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->block_cfg_tx.block_size = SPI_CC23X0_CC27XX_DFS * buf_size;

	/* No TX data - send dummy bytes */
	if (!ctx->tx_buf || (ctx->tx_count == 0 && ctx->tx_len == 0)) {
		data->block_cfg_tx.source_address = (uint32_t)&dummy_tx_data;
		data->block_cfg_tx.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	return dma_config(cfg->dma_dev, cfg->dma_channel_tx, &data->dma_cfg_tx);
}

/**
 * @brief Load and configure the RX DMA channel
 *
 * Sets up the RX DMA block config for the current RX context buffer and
 * calls dma_config. Falls back to dummy RX sink if no RX buffer is available.
 *
 * @param dev Pointer to the SPI device structure.
 * @param buf_size Number of frames to transfer.
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_dma_load_rx(const struct device *dev, size_t buf_size)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;

	data->block_cfg_rx.dest_address = (uint32_t)ctx->rx_buf;
	data->block_cfg_rx.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->block_cfg_rx.block_size = SPI_CC23X0_CC27XX_DFS * buf_size;

	/* No RX buffer - discard received data */
	if (!ctx->rx_buf || (ctx->rx_count == 0 && ctx->rx_len == 0)) {
		data->block_cfg_rx.dest_address = (uint32_t)&dummy_rx_data;
		data->block_cfg_rx.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	}

	return dma_config(cfg->dma_dev, cfg->dma_channel_rx, &data->dma_cfg_rx);
}

/**
 * @brief Stop both TX and RX DMA channels and disable DMA triggers
 *
 * @param dev Pointer to the SPI device structure.
 */
static void spi_cc23x0_cc27xx_dma_stop(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;

	SPIDisableDMA(cfg->base, SPI_DMA_TX | SPI_DMA_RX);
	dma_stop(cfg->dma_dev, cfg->dma_channel_tx);
	dma_stop(cfg->dma_dev, cfg->dma_channel_rx);
}

/**
 * @brief Start DMA transfer in the given direction
 *
 * Starts the DMA channel(s) for the specified direction and enables the
 * corresponding SPI DMA triggers. Interrupts are enabled for whichever
 * channels are started.
 *
 * @param dev Pointer to the SPI device structure.
 * @param dir Direction of transfer (TX, RX, or BOTH).
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_dma_start(const struct device *dev, enum transfer_direction dir)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	int ret;

	/*
	 * For each direction being started:
	 *   1. Disable the DMA trigger and stop the channel.
	 *   2. Clear any pending interrupt, then re-enable it. Clearing before
	 *      re-enabling prevents a stale interrupt left from the previous
	 *      transfer from firing spuriously and corrupting the new transfer.
	 *   3. Start the DMA channel.
	 *
	 * DMA triggers (SPIEnableDMA) are enabled only after both channels are
	 * started, with RX triggered last so it is already consuming data before
	 * the TX trigger can produce a clock edge (master) or accept data (slave).
	 */
	if ((dir == TRANSFER_DIR_TX) || (dir == TRANSFER_DIR_BOTH)) {
		SPIDisableDMA(cfg->base, SPI_DMA_TX);
		dma_stop(cfg->dma_dev, cfg->dma_channel_tx);
		SPIClearInt(cfg->base, SPI_DMA_DONE_TX);
		SPIEnableInt(cfg->base, SPI_DMA_DONE_TX);
#ifdef CONFIG_SOC_SERIES_CC27XX
		/*
		 * This is a workaround for a DMA errata UDMA_01.
		 */
		uDMAEnableChannelAttribute(BIT(cfg->dma_channel_tx), UDMA_ATTR_USEBURST);
#endif
		ret = dma_start(cfg->dma_dev, cfg->dma_channel_tx);
		if (ret) {
			LOG_ERR("Failed to start DMA TX channel");
			return ret;
		}
	}

	if ((dir == TRANSFER_DIR_RX) || (dir == TRANSFER_DIR_BOTH)) {
		SPIDisableDMA(cfg->base, SPI_DMA_RX);
		dma_stop(cfg->dma_dev, cfg->dma_channel_rx);
		SPIClearInt(cfg->base, SPI_DMA_DONE_RX);
		SPIEnableInt(cfg->base, SPI_DMA_DONE_RX | SPI_RXFIFO_OVF);
		ret = dma_start(cfg->dma_dev, cfg->dma_channel_rx);
		if (ret) {
			LOG_ERR("Failed to start DMA RX channel");
			return ret;
		}
	}

	/* Enable DMA triggers after both channels are started. TX first, then
	 * RX last so the RX path is active before the clock starts.
	 */
	if ((dir == TRANSFER_DIR_TX) || (dir == TRANSFER_DIR_BOTH)) {
		SPIEnableDMA(cfg->base, SPI_DMA_TX);
	}

	if ((dir == TRANSFER_DIR_RX) || (dir == TRANSFER_DIR_BOTH)) {
		SPIEnableDMA(cfg->base, SPI_DMA_RX);
	}

	return 0;
}
#endif /* CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN */

/**
 * @brief Internal function to handle both sync and async transceive calls
 *
 * This function is used to handle both synchronous and asynchronous
 * SPI transceive calls. It sets up the SPI context, configures the SPI device,
 * and initiates the first data transfer.
 *
 * @params dev Pointer to the SPI device structure.
 * @params config Pointer to the SPI configuration structure.
 * @params tx_bufs Pointer to the transmit buffer set.
 * @params rx_bufs Pointer to the receive buffer set.
 * @params cb Callback function for asynchronous transfers (NULL for sync).
 * @params userdata User data to be passed to the callback function.
 *
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_transceive_internal(const struct device *dev,
						 const struct spi_config *config,
						 const struct spi_buf_set *tx_bufs,
						 const struct spi_buf_set *rx_bufs,
						 spi_callback_t cb, void *userdata)
{
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, (cb == NULL ? false : true), cb, userdata, config);

	/**
	 * If the user provides a chip select gpio at the application, use that as a
	 * software-based chip select
	 *
	 * @note If cs gpio is not provided from either the devicetree or at the
	 * application, then the data->cs_gpio remains NULL. The application must
	 * handle the asserting and deasserting of their CS line (master).
	 */
	if (spi_cs_is_gpio(config) && !SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		data->cs_gpio = (struct gpio_dt_spec *)&(config->cs.gpio);
		data->is_cs_sw_controlled = true;
	}
	/**
	 * Ensure there is enough cs gpio in the device tree for the slave index
	 * that the caller wants to use.
	 *
	 * NOTE: the +1 is for adjusting the index
	 * to gpio count, i.e., if the caller wants to use cs for slave index 2,
	 * there must be 3 total count of slaves available.
	 * NOTE: "data->cs_gpio == NULL" check is required here to avoid overwriting
	 * the CS pin provided from the pinctrl-0. It takes precedence over the
	 * cs_gpios node (see spi-controller.yaml)
	 */
#ifndef DT_SPI_CTX_HAS_NO_CS_GPIOS
	else if (data->cs_gpio == NULL && data->ctx.num_cs_gpios >= (config->slave + 1)) {
		data->cs_gpio = (struct gpio_dt_spec *)&(data->ctx.cs_gpios[config->slave]);
		data->is_cs_sw_controlled = true;
	}
#endif /* !DT_SPI_CTX_HAS_NO_CS_GPIOS */

	if (data->cs_gpio == NULL && SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		/* NOTE: Removing this else-statement will remove the restriction of
		 * 4-wire mode only for the slave. 3-wire would be supported. However,
		 * note that the slave would be susceptible to errors due to glitches in
		 * any of the SPI lines.
		 */
		LOG_ERR("Slave only supports 4-wire mode. A SPI Chip Select "
			"capable GPIO must be selected");
		return -EINVAL;
	}

	ret = spi_cc23x0_cc27xx_configure(dev, config);
	if (ret) {
		spi_context_release(ctx, ret);
		return ret;
	}

	/**
	 * In slave mode, the maximum transfer is currently limited to the maximum
	 * single DMA transfer because rearming and starting the DMA is not
	 * guaranteed to be fast enough for the slave to keep up with the master.
	 * This is currently a limitation on the DMA driver implementation, until
	 * the DMA ping-pong feature is implemented.
	 */
	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		if (spi_context_total_tx_len(&data->ctx) >
			    SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT ||
		    spi_context_total_rx_len(&data->ctx) >
			    SPI_CC23X0_CC27XX_MAX_DMA_FRAME_TRANSFER_AMOUNT) {
			ret = SPI_CC23X0_CC27XX_STATUS_ERROR_MAX_TRANSFER_AMT_EXCEEDED;
			spi_context_release(ctx, ret);
			LOG_ERR("Total TX or RX length exceeds maximum transfer amount.");
			return ret;
		}
	}

	data->tx_bufs = (struct spi_buf_set *)tx_bufs;
	data->rx_bufs = (struct spi_buf_set *)rx_bufs;
	spi_context_buffers_setup(ctx, data->tx_bufs, data->rx_bufs, SPI_CC23X0_CC27XX_DFS);

	spi_cc23x0_cc27xx_initialize_data(dev);
	spi_cc23x0_cc27xx_pm_policy_state_lock_get();

	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE((config->operation))) {
		/* Clear stale DMA status flags and flush FIFOs before arming the
		 * slave DMA. Stale flags can cause a pending ISR from the previous
		 * transaction to fire spuriously during the new setup, advancing
		 * the RX context before any real data arrives and misaligning
		 * subsequent chunks.
		 */
		data->dma_status_flags = 0;
		spi_cc23x0_cc27xx_reset_txrx_fifos(dev);
	} else {
		spi_context_cs_control(ctx, true);
	}

	ret = spi_cc23xo_prime_transceive(dev);
	if (ret) {
		/**
		 * Make sure to release the lock on SPI if transceive fails so that
		 * system is allowed to try SPI again and/or go to standby
		 */
		spi_context_release(ctx, ret);
		spi_cc23x0_cc27xx_pm_policy_state_lock_put();
	}

	return ret;
}

/**
 * @brief Synchronous SPI transceive function
 *
 * This function performs a synchronous SPI transceive operation.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 * @param tx_bufs Pointer to the transmit buffer set.
 * @param rx_bufs Pointer to the receive buffer set.
 *
 * @return 0 on success, negative error code on failure
 * @note in slave mode, non-zero positive return value indicates the number of
 * received frames.
 */
static int spi_cc23x0_cc27xx_transceive(const struct device *dev, const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	int ret;

	ret = spi_cc23x0_cc27xx_transceive_internal(dev, config, tx_bufs, rx_bufs, NULL, NULL);
	if (ret) {
		LOG_ERR("Failed to initiate SPI transceive: %d", ret);
		return ret;
	}
	/**
	 * Wait for the transfer to complete. This is necessary for synchronous
	 * transceive calls to ensure that the transfer is finished before
	 * returning. This semaphore gets given in the ISR (using
	 * spi_context_complete) when all data has been transferred/received.
	 */
	ret = spi_context_wait_for_completion(&data->ctx);

	return ret;
}

#ifdef CONFIG_SPI_ASYNC
/**
 * @brief Asynchronous SPI transceive function
 *
 * This function performs an asynchronous SPI transceive operation.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 * @param tx_bufs Pointer to the transmit buffer set.
 * @param rx_bufs Pointer to the receive buffer set.
 *
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_transceive_async(const struct device *dev,
					      const struct spi_config *config,
					      const struct spi_buf_set *tx_bufs,
					      const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					      void *userdata)
{
	int ret;

	if (cb == NULL) {
		LOG_ERR("Asynchronous transfers require a callback function");
		return -EINVAL;
	}

	ret = spi_cc23x0_cc27xx_transceive_internal(dev, config, tx_bufs, rx_bufs, cb, userdata);
	if (ret) {
		LOG_ERR("Failed to initiate SPI transceive: %d", ret);
		return ret;
	}

	return ret;
}
#endif

/**
 * @brief Release the SPI device after a transfer
 *
 * This function releases the SPI device after a transfer has been completed.
 *
 * @param dev Pointer to the SPI device structure.
 * @param config Pointer to the SPI configuration structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_release(const struct device *dev, const struct spi_config *config)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;

	if (!spi_context_configured(&data->ctx, config)) {
		return -EINVAL;
	}

	if (SPIBusy(cfg->base)) {
		return -EBUSY;
	}

#if defined(CONFIG_PM_DEVICE) && defined(CONFIG_SPI_SLAVE)
	/*
	 * The application is done with the slave. Release the policy lock
	 * acquired in spi_cc23x0_cc27xx_configure() so the system may enter
	 * standby again.
	 */
	if (SPI_CC23X0_CC27XX_CONFIG_IS_SLAVE(data->ctx.config->operation) &&
	    data->slave_pm_lock_held) {
		pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
		pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
		data->slave_pm_lock_held = false;
	}
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_cc23x0_cc27xx_driver_api) = {
	.transceive = spi_cc23x0_cc27xx_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_cc23x0_cc27xx_transceive_async,
#endif
	.release = spi_cc23x0_cc27xx_release,
};

static struct gpio_dt_spec hw_cs_gpio[2] = {0};

static void spi_cc23xo_cc27xx_init_spi_gpios(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	/**
	 * Initialize SPI GPIO pins.
	 * NOTE: The pinctrl-0 node SPI pins must be in the correct order for
	 * this to work: SCK, MOSI, MISO, CS
	 */
	data->sck_gpio =
		(struct gpio_dt_spec){.port = cfg->gpio_dev,
				      .pin = cfg->pincfg->states[PINCTRL_STATE_DEFAULT].pins[0].pin,
				      .dt_flags = 0};

	data->mosi_gpio =
		(struct gpio_dt_spec){.port = cfg->gpio_dev,
				      .pin = cfg->pincfg->states[PINCTRL_STATE_DEFAULT].pins[1].pin,
				      .dt_flags = 0};

	data->miso_gpio =
		(struct gpio_dt_spec){.port = cfg->gpio_dev,
				      .pin = cfg->pincfg->states[PINCTRL_STATE_DEFAULT].pins[2].pin,
				      .dt_flags = 0};

	/**
	 * Only attempt to initialize the CS pin if there is a 4th pin, which must
	 * be the CS pin
	 */
	if (cfg->pincfg->states[PINCTRL_STATE_DEFAULT].pin_cnt == 4) {
		hw_cs_gpio[cfg->instance] = (struct gpio_dt_spec){
			.port = cfg->gpio_dev,
			.pin = cfg->pincfg->states[PINCTRL_STATE_DEFAULT].pins[3].pin,
			.dt_flags = 0};

		data->cs_gpio = &hw_cs_gpio[cfg->instance];
	}
}

static int spi_cc23x0_cc27xx_init(const struct device *dev)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;
	int ret;

	cfg->irq_config_func();

#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
	if (!device_is_ready(cfg->dma_dev)) {
		LOG_ERR("DMA not ready");
		return -ENODEV;
	}
#endif

	if (!device_is_ready(cfg->gpio_dev)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	spi_cc23xo_cc27xx_init_spi_gpios(dev);

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret) {
		return ret;
	}

	/**
	 * If no CS pin is provided in pinctrl-0, use the first cs_gpio at init as
	 * software controlled chip select.
	 */
#ifndef DT_SPI_CTX_HAS_NO_CS_GPIOS
	if (data->cs_gpio == NULL && data->ctx.num_cs_gpios > 0) {
		data->cs_gpio = (struct gpio_dt_spec *)&(data->ctx.cs_gpios[0]);
		data->is_cs_sw_controlled = true;
	}
#endif /* !DT_SPI_CTX_HAS_NO_CS_GPIOS */

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_SPI_CC23X0_CC27XX_DMA_DRIVEN
#define SPI_CC23X0_CC27XX_DMA_INIT(n)                                                              \
	.dma_dev = DEVICE_DT_GET(TI_CC23X0_CC27XX_DT_INST_DMA_CTLR(n, tx)),                        \
	.dma_channel_tx = TI_CC23X0_CC27XX_DT_INST_DMA_CHANNEL(n, tx),                             \
	.dma_trigsrc_tx = TI_CC23X0_CC27XX_DT_INST_DMA_TRIGSRC(n, tx),                             \
	.dma_channel_rx = TI_CC23X0_CC27XX_DT_INST_DMA_CHANNEL(n, rx),                             \
	.dma_trigsrc_rx = TI_CC23X0_CC27XX_DT_INST_DMA_TRIGSRC(n, rx),
#else
#define SPI_CC23X0_CC27XX_DMA_INIT(n)
#endif

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Handle power management actions for the SPI device
 *
 * This function handles the power management actions for the SPI device.
 *
 * @param dev Pointer to the SPI device structure.
 * @param action The power management action to perform (suspend or resume).
 *
 * @return 0 on success, negative error code on failure.
 */
static int spi_cc23x0_cc27xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct spi_cc23x0_cc27xx_config *cfg = dev->config;
	struct spi_cc23x0_cc27xx_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
#if defined(CONFIG_SPI_SLAVE)
		/*
		 * The pm_policy_state_lock held by the slave configure path
		 * should prevent the PM subsystem from ever requesting SUSPEND
		 * while a slave is active. Refuse defensively in case the lock
		 * was somehow bypassed.
		 */
		if (data->slave_pm_lock_held) {
			return -EBUSY;
		}
#endif
		SPIDisable(cfg->base);
		CLKCTLDisable(CLKCTL_BASE, cfg->clock_id);
		if (data->ctx.config != NULL) {
			spi_cc23x0_cc27xx_pinctrl_apply_sleep_state(dev, data->ctx.config);
		}
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		CLKCTLEnable(CLKCTL_BASE, cfg->clock_id);
		if (data->ctx.config != NULL) {
			spi_cc23x0_cc27xx_pinctrl_apply_active_state(dev, data->ctx.config);
			/* Disable SPI to clear any stale hardware state before next configure */
			SPIDisable(cfg->base);
		}
		/* Force SPI to be reconfigured at next transfer */
		data->ctx.config = NULL;
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

#define SPI_CC23X0_CC27XX_SPI_PINS_INIT(n)                                                         \
	.sck_gpio = NULL, .mosi_gpio = NULL, .miso_gpio = NULL, .cs_gpio = NULL

#define SPI_CC23X0_CC27XX_INIT(n)                                                                  \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, spi_cc23x0_cc27xx_pm_action);                                  \
                                                                                                   \
	static void spi_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), spi_cc23x0_cc27xx_isr,      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
                                                                                                   \
	static const struct spi_cc23x0_cc27xx_config spi_cc23x0_cc27xx_config_##n = {              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.instance = n,                                                                     \
		.clock_id = DT_INST_PROP(n, clock_id),                                             \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.irq_config_func = spi_irq_config_func_##n,                                        \
		.gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)),                                    \
		SPI_CC23X0_CC27XX_DMA_INIT(n)};                                                    \
                                                                                                   \
	static struct spi_cc23x0_cc27xx_data spi_cc23x0_cc27xx_data_##n = {                        \
		.xfer_sync_sem =                                                                   \
			Z_SEM_INITIALIZER(spi_cc23x0_cc27xx_data_##n.xfer_sync_sem, 0, 1),         \
		.is_cs_sw_controlled = false,                                                      \
		SPI_CONTEXT_INIT_LOCK(spi_cc23x0_cc27xx_data_##n, ctx),                            \
		SPI_CONTEXT_INIT_SYNC(spi_cc23x0_cc27xx_data_##n, ctx),                            \
		SPI_CC23X0_CC27XX_SPI_PINS_INIT(n),                                                \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)};                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, spi_cc23x0_cc27xx_init, PM_DEVICE_DT_INST_GET(n),                 \
			      &spi_cc23x0_cc27xx_data_##n, &spi_cc23x0_cc27xx_config_##n,          \
			      POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,                               \
			      &spi_cc23x0_cc27xx_driver_api)

DT_INST_FOREACH_STATUS_OKAY(SPI_CC23X0_CC27XX_INIT)

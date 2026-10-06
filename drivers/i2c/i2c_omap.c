/* Copyright (C) 2024 BeagleBoard.org Foundation
 * Copyright (C) 2024 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_omap_i2c
#include <errno.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2c.h>
#ifdef CONFIG_I2C_RTIO
#include <zephyr/drivers/i2c/rtio.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>

#include "i2c-priv.h"

#ifdef CONFIG_CLOCK_CONTROL_TISCI
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#endif

#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
#include "i2c_bitbang.h"
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */

LOG_MODULE_REGISTER(omap_i2c, CONFIG_I2C_LOG_LEVEL);

/* Short waits for soft-reset done and bus-busy (not the per-message xfer timeout) */
#define I2C_OMAP_HW_TIMEOUT_MS  100U
/* Per-message completion wait (poll loop or IRQ semaphore) */
#define I2C_OMAP_MSG_TIMEOUT_MS 1000U
/* OCP_SYSSTATUS bit definitions */
#define SYSS_RESETDONE_MASK  BIT(0)
#define I2C_OMAP_SYSC_SRST   BIT(1)
#define RETRY                -1
/* OMAP I2C internal clock targets for PSC (TRM), not I2C_BITRATE_* bus rates */
#define I2C_OMAP_DEFAULT_FCLK_HZ  96000000U
#define I2C_OMAP_ICLK_STANDARD_HZ 4000000U
#define I2C_OMAP_ICLK_FAST_HZ     9600000U
#define I2C_OMAP_SCLL_TRIM        7U
#define I2C_OMAP_SCLH_TRIM        5U
/* Write to IRQENABLE_CLR to mask all interrupt sources */
#define I2C_OMAP_IRQENABLE_ALL    0xffffffffU
#define I2C_BUFSTAT_RX_MASK  GENMASK(13, 8)
#define I2C_BUFSTAT_TX_MASK  GENMASK(5, 0)
#define I2C_BUFSTAT_FIFODEPTH_MASK GENMASK(15, 14)
#define I2C_BUF_RXTRSH_MASK  GENMASK(13, 8)
#define I2C_BUF_TXTRSH_MASK  GENMASK(5, 0)

/* I2C Registers (K3 / IP v2 layout) */
typedef struct {
	uint8_t RESERVED_0[0x10]; /**< Reserved, offset: 0x0 */

	uint32_t SYSC;          /**< System Configuration, offset: 0x10 */
	uint8_t RESERVED_1[0xC];     /**< Reserved, offset: 0x14 - 0x1F */
	uint32_t EOI;           /**< End Of Interrupt, offset: 0x20 */
	uint32_t IRQSTATUS_RAW; /**< Interrupt Status Raw, offset: 0x24 */
	uint32_t IRQSTATUS;     /**< Interrupt Status (W1C), offset: 0x28 */
	uint32_t IRQENABLE_SET; /**< Interrupt Enable Set, offset: 0x2C */
	uint32_t IRQENABLE_CLR; /**< Interrupt Enable Clear, offset: 0x30 */
	uint32_t WE;            /**< Wakeup Enable, offset: 0x34 */
	uint8_t RESERVED_2[0x58];    /**< Reserved, offset: 0x38 - 0x90 */
	uint32_t SYSS;          /**< System Status, offset: 0x90 */
	uint32_t BUF;           /**< Buffer, offset: 0x94 */
	uint32_t CNT;           /**< Data Count, offset: 0x98 */
	uint32_t DATA;          /**< Data Access, offset: 0x9C */
	uint8_t RESERVED_5[0x4];     /**< Reserved, offset: 0xA0 - 0xA4 */
	uint32_t CON;           /**< Configuration, offset: 0xA4 */
	uint32_t OA;            /**< Own Address, offset: 0xA8 */
	uint32_t SA;            /**< Target Address, offset: 0xAC */
	uint32_t PSC;           /**< Clock Prescaler, offset: 0xB0 */
	uint32_t SCLL;          /**< SCL Low Time, offset: 0xB4 */
	uint32_t SCLH;          /**< SCL High Time, offset: 0xB8 */
	uint32_t SYSTEST;       /**< System Test, offset: 0xBC */
	uint32_t BUFSTAT;       /**< Buffer Status, offset: 0xC0 */
} i2c_omap_regs_t;

BUILD_ASSERT(offsetof(i2c_omap_regs_t, SYSC) == 0x10);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, EOI) == 0x20);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, IRQSTATUS_RAW) == 0x24);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, IRQSTATUS) == 0x28);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, SYSS) == 0x90);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, CON) == 0xa4);
BUILD_ASSERT(offsetof(i2c_omap_regs_t, BUFSTAT) == 0xc0);

/* I2C Configuration Register (I2C_OMAP_CON) */
#define I2C_OMAP_CON_EN        BIT(15) /* I2C module enable */
#define I2C_OMAP_CON_OPMODE_HS BIT(12) /* High Speed support */
#define I2C_OMAP_CON_MST       BIT(10) /* Controller/target mode */
#define I2C_OMAP_CON_TRX       BIT(9)  /* TX/RX mode (controller only) */
#define I2C_OMAP_CON_STP       BIT(1)  /* Stop condition (controller only) */
#define I2C_OMAP_CON_STT       BIT(0)  /* Start condition (controller) */

/* I2C Buffer Configuration Register (I2C_OMAP_BUF): */
#define I2C_OMAP_BUF_RXFIF_CLR BIT(14) /* RX FIFO Clear */
#define I2C_OMAP_BUF_TXFIF_CLR BIT(6)  /* TX FIFO Clear */

/* I2C Status Register (I2C_OMAP_STAT): */
#define I2C_OMAP_STAT_XDR  BIT(14) /* TX Buffer draining */
#define I2C_OMAP_STAT_RDR  BIT(13) /* RX Buffer draining */
#define I2C_OMAP_STAT_BB   BIT(12) /* Bus busy */
#define I2C_OMAP_STAT_ROVR BIT(11) /* Receive overrun */
#define I2C_OMAP_STAT_XUDF BIT(10) /* Transmit underflow */
#define I2C_OMAP_STAT_AAS  BIT(9)  /* Address as target */
#define I2C_OMAP_STAT_BF   BIT(8)  /* Bus free (STOP detected) */
#define I2C_OMAP_STAT_XRDY BIT(4)  /* Transmit data ready */
#define I2C_OMAP_STAT_RRDY BIT(3)  /* Receive data ready */
#define I2C_OMAP_STAT_ARDY BIT(2)  /* Register access ready */
#define I2C_OMAP_STAT_NACK BIT(1)  /* No ack interrupt enable */
#define I2C_OMAP_STAT_AL   BIT(0)  /* Arbitration lost */

/* IRQSTATUS is write-1-to-clear; writing 0 leaves a bit unchanged */
#define I2C_OMAP_STAT_CLR_MASK                                                                     \
	(I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_AAS | I2C_OMAP_STAT_BF |            \
	 I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_ARDY | I2C_OMAP_STAT_NACK |       \
	 I2C_OMAP_STAT_AL)

/* I2C System Test Register (I2C_OMAP_SYSTEST): */
#define I2C_OMAP_SYSTEST_ST_EN       BIT(15)   /* System test enable */
#define I2C_OMAP_SYSTEST_FREE        BIT(14)   /* Free running mode */
#define I2C_OMAP_SYSTEST_TMODE_MASK  (3 << 12) /* Test mode select mask */
#define I2C_OMAP_SYSTEST_TMODE_SHIFT (12)      /* Test mode select shift */

/* Functional mode */
#define I2C_OMAP_SYSTEST_SCL_I_FUNC BIT(8) /* SCL line input value */
#define I2C_OMAP_SYSTEST_SDA_I_FUNC BIT(6) /* SDA line input value */

/* SDA/SCL IO mode */
#define I2C_OMAP_SYSTEST_SCL_I BIT(3) /* SCL line sense in */
#define I2C_OMAP_SYSTEST_SCL_O BIT(2) /* SCL line drive out */
#define I2C_OMAP_SYSTEST_SDA_I BIT(1) /* SDA line sense in */
#define I2C_OMAP_SYSTEST_SDA_O BIT(0) /* SDA line drive out */

typedef void (*init_func_t)(const struct device *dev);
#define DEV_CFG(dev)      ((const struct i2c_omap_cfg *)(dev)->config)
#define DEV_DATA(dev)     ((struct i2c_omap_data *)(dev)->data)
#define DEV_I2C_BASE(dev) ((volatile i2c_omap_regs_t *)DEVICE_MMIO_GET(dev))

struct i2c_omap_cfg {
	DEVICE_MMIO_ROM;
	uint32_t irq;
	uint32_t speed;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
};

struct i2c_omap_speed_config {
	uint32_t pscstate;
	uint32_t scllstate;
	uint32_t sclhstate;
};

struct i2c_omap_data {
	DEVICE_MMIO_RAM;
	uint32_t speed;
	uint32_t dev_config;
	struct i2c_omap_speed_config speed_config;
	struct i2c_msg current_msg;
	struct k_sem lock;
#ifdef CONFIG_I2C_OMAP_INTERRUPT
	struct k_sem xfer_done;
	volatile bool xfer_active;
	uint32_t xfer_ll_result;
	uint32_t irq_enable;
#endif
#if defined(CONFIG_I2C_TARGET)
	struct i2c_target_config *target_cfg;
	bool target_first;
	bool target_read_active;
#endif
#ifdef CONFIG_I2C_RTIO
	struct i2c_rtio *ctx;
#endif
	uint8_t discard;
	uint8_t fifo_size;
	bool receiver;
	bool bb_valid;
};

/**
 * @brief Initializes the OMAP I2C driver.
 *
 * This function is responsible for initializing the OMAP I2C driver.
 *
 * @param dev Pointer to the device structure for the I2C driver instance.
 */
static void i2c_omap_init_ll(const struct device *dev)
{

	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	i2c_base_addr->CON = 0;
	i2c_base_addr->PSC = data->speed_config.pscstate;
	i2c_base_addr->SCLL = data->speed_config.scllstate;
	i2c_base_addr->SCLH = data->speed_config.sclhstate;
	i2c_base_addr->CON = I2C_OMAP_CON_EN;
	i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_CLR_MASK;
#ifdef CONFIG_I2C_OMAP_INTERRUPT
	if (data->irq_enable != 0U) {
		i2c_base_addr->IRQENABLE_SET = data->irq_enable;
	}
#endif
}

static int i2c_omap_transfer_message_ll(const struct device *dev);
static int i2c_omap_ll_result_to_err(const struct device *dev, int result);
#ifdef CONFIG_I2C_RTIO
static void i2c_omap_rtio_complete(const struct device *dev, int status);
#endif

#ifdef CONFIG_I2C_OMAP_INTERRUPT
static void i2c_omap_update_irq_enable(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	uint32_t en = I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_ARDY |
		    I2C_OMAP_STAT_NACK | I2C_OMAP_STAT_AL;

	if (data->fifo_size != 0U) {
		en |= I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_RDR;
	}
	data->irq_enable = en;
}

#if defined(CONFIG_I2C_TARGET)
static void i2c_omap_target_update_irq_enable(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);

	/* Byte-oriented target: AAS/BF framing + RRDY/XRDY data path */
	data->irq_enable = I2C_OMAP_STAT_AAS | I2C_OMAP_STAT_BF | I2C_OMAP_STAT_RRDY |
			   I2C_OMAP_STAT_XRDY;
}

static void i2c_omap_target_isr(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	const struct i2c_target_callbacks *cb;
	uint32_t status;
	uint8_t byte;
	int ret;

	if (data->target_cfg == NULL || data->target_cfg->callbacks == NULL) {
		i2c_base_addr->IRQSTATUS = i2c_base_addr->IRQSTATUS;
		return;
	}

	cb = data->target_cfg->callbacks;
	status = i2c_base_addr->IRQSTATUS & data->irq_enable;

	if (status & I2C_OMAP_STAT_AAS) {
		LOG_DBG("target ISR AAS (addr-as-target) irqstat=0x%x", status);
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_AAS;
		data->target_first = true;
		data->target_read_active = false;
		status &= ~I2C_OMAP_STAT_AAS;
	}

	if (status & I2C_OMAP_STAT_RRDY) {
		byte = (uint8_t)i2c_base_addr->DATA;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_RRDY;
		LOG_DBG("target ISR RRDY byte=0x%02x first=%d", byte, data->target_first);

		if (data->target_first) {
			data->target_first = false;
			if (cb->write_requested != NULL) {
				ret = cb->write_requested(data->target_cfg);
				if (ret < 0) {
					/* OMAP target cannot NACK mid-byte; drop payload */
					status &= ~I2C_OMAP_STAT_RRDY;
					goto check_bf;
				}
			}
		}
		if (cb->write_received != NULL) {
			(void)cb->write_received(data->target_cfg, byte);
		}
		status &= ~I2C_OMAP_STAT_RRDY;
	}

	if (status & I2C_OMAP_STAT_XRDY) {
		byte = 0xff;

		if (data->target_first) {
			data->target_first = false;
			data->target_read_active = true;
			if (cb->read_requested != NULL) {
				ret = cb->read_requested(data->target_cfg, &byte);
				if (ret < 0) {
					data->target_read_active = false;
				}
			}
		} else if (data->target_read_active && cb->read_processed != NULL) {
			ret = cb->read_processed(data->target_cfg, &byte);
			if (ret < 0) {
				data->target_read_active = false;
				byte = 0xff;
			}
		}

		LOG_DBG("target ISR XRDY byte=0x%02x", byte);
		i2c_base_addr->DATA = byte;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_XRDY;
		status &= ~I2C_OMAP_STAT_XRDY;
	}

check_bf:
	if (status & I2C_OMAP_STAT_BF) {
		LOG_DBG("target ISR BF (stop)");
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_BF;
		data->target_first = false;
		data->target_read_active = false;
		if (cb->stop != NULL) {
			(void)cb->stop(data->target_cfg);
		}
	}
}

static int i2c_omap_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	if (cfg == NULL || cfg->callbacks == NULL) {
		return -EINVAL;
	}
	if ((cfg->flags & I2C_TARGET_FLAGS_ADDR_10_BITS) != 0U) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (data->target_cfg != NULL || data->xfer_active) {
		k_sem_give(&data->lock);
		return -EBUSY;
	}

	data->target_cfg = cfg;
	data->target_first = false;
	data->target_read_active = false;

	i2c_base_addr->CON = 0;
	i2c_base_addr->OA = cfg->address & 0x7fU;
	/* Threshold of 1 byte (TXTRSH/RXTRSH = 0) for Zephyr per-byte callbacks */
	i2c_base_addr->BUF = I2C_OMAP_BUF_RXFIF_CLR | I2C_OMAP_BUF_TXFIF_CLR;

	i2c_omap_target_update_irq_enable(dev);
	i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_CLR_MASK;
	i2c_base_addr->IRQENABLE_CLR = I2C_OMAP_IRQENABLE_ALL;
	i2c_base_addr->IRQENABLE_SET = data->irq_enable;
	/* Target mode: EN without MST */
	i2c_base_addr->CON = I2C_OMAP_CON_EN;

	k_sem_give(&data->lock);
	return 0;
}

static int i2c_omap_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	k_sem_take(&data->lock, K_FOREVER);

	if (data->target_cfg == NULL || data->target_cfg != cfg) {
		k_sem_give(&data->lock);
		return -EINVAL;
	}

	data->target_cfg = NULL;
	data->target_first = false;
	data->target_read_active = false;

	i2c_base_addr->CON = 0;
	i2c_base_addr->OA = 0;
	i2c_base_addr->IRQENABLE_CLR = I2C_OMAP_IRQENABLE_ALL;
	i2c_omap_update_irq_enable(dev);
	i2c_omap_init_ll(dev);

	k_sem_give(&data->lock);
	return 0;
}
#endif /* CONFIG_I2C_TARGET */

static void i2c_omap_isr(const void *arg)
{
	const struct device *dev = arg;
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	int result = RETRY;
	int count = 0;

#if defined(CONFIG_I2C_TARGET)
	if (data->target_cfg != NULL) {
		i2c_omap_target_isr(dev);
		i2c_base_addr->EOI = 0;
		return;
	}
#endif

	if (!data->xfer_active) {
		i2c_base_addr->IRQSTATUS = i2c_base_addr->IRQSTATUS;
		i2c_base_addr->EOI = 0;
		return;
	}

	/*
	 * Drain pending enabled events. RETRY means "more data later" (wait for
	 * the next IRQ) — never busy-loop here when IRQSTATUS is empty.
	 */
	do {
		if ((i2c_base_addr->IRQSTATUS & data->irq_enable) == 0U) {
			break;
		}
		result = i2c_omap_transfer_message_ll(dev);
		count++;
	} while (result == RETRY && count < 100);

	if (result != RETRY) {
		data->xfer_active = false;
#ifdef CONFIG_I2C_RTIO
		i2c_omap_rtio_complete(dev, i2c_omap_ll_result_to_err(dev, result));
#else
		data->xfer_ll_result = (uint32_t)result;
		k_sem_give(&data->xfer_done);
#endif
	}

	i2c_base_addr->EOI = 0;
}
#endif /* CONFIG_I2C_OMAP_INTERRUPT */

/**
 * @brief Reset the OMAP I2C controller.
 *
 * This function resets the OMAP I2C controller specified by the device pointer.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @return 0 on success, negative errno code on failure.
 */
static int i2c_omap_reset(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint64_t timeout;
	uint32_t sysc;

	sysc = i2c_base_addr->SYSC;
	i2c_base_addr->CON &= ~I2C_OMAP_CON_EN;
	i2c_base_addr->SYSC = sysc | I2C_OMAP_SYSC_SRST;
	timeout = k_uptime_get() + I2C_OMAP_HW_TIMEOUT_MS;
	i2c_base_addr->CON = I2C_OMAP_CON_EN;
	while (!(i2c_base_addr->SYSS & SYSS_RESETDONE_MASK)) {
		if (k_uptime_get() > timeout) {
			LOG_WRN("timeout waiting for controller reset");
			return -ETIMEDOUT;
		}
		k_busy_wait(100);
	}
	i2c_base_addr->SYSC = sysc;
	data->bb_valid = 0;
	return 0;
}

static int i2c_omap_get_fclk(const struct device *dev, uint32_t *fclk_hz)
{
	const struct i2c_omap_cfg *cfg = DEV_CFG(dev);
	int ret;

	if (cfg->clock_dev == NULL) {
		*fclk_hz = I2C_OMAP_DEFAULT_FCLK_HZ;
		LOG_WRN("no clocks property; using default fclk %u Hz", I2C_OMAP_DEFAULT_FCLK_HZ);
		return 0;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock controller not ready");
		return -ENODEV;
	}

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, fclk_hz);
	if (ret < 0) {
		LOG_ERR("clock_control_get_rate failed (%d)", ret);
		return ret;
	}

	if (*fclk_hz == 0U) {
		LOG_ERR("functional clock rate is zero");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Set the speed of the OMAP I2C controller.
 *
 * This function sets the speed of the OMAP I2C controller based on the
 * specified speed parameter. The speed can be set to either Fast mode or
 * Standard mode.
 *
 * @param dev The pointer to the device structure.
 * @param speed The desired speed for the I2C controller.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_omap_set_speed(const struct device *dev, uint32_t speed)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	uint32_t internal_clk;
	uint32_t psc;
	uint32_t scl;
	uint32_t scll;
	uint32_t sclh;
	uint32_t fclk_hz;
	int ret;

	ret = i2c_omap_get_fclk(dev, &fclk_hz);
	if (ret < 0) {
		return ret;
	}

	if (speed > I2C_BITRATE_STANDARD) {
		internal_clk = I2C_OMAP_ICLK_FAST_HZ;
	} else {
		internal_clk = I2C_OMAP_ICLK_STANDARD_HZ;
	}

	psc = fclk_hz / internal_clk;
	if (psc == 0U) {
		return -EINVAL;
	}
	psc -= 1U;

	internal_clk = fclk_hz / (psc + 1U);
	if (internal_clk < (speed * 2U)) {
		return -ERANGE;
	}

	scl = internal_clk / speed;
	if (scl <= (I2C_OMAP_SCLL_TRIM + I2C_OMAP_SCLH_TRIM)) {
		return -ERANGE;
	}

	if (speed > I2C_BITRATE_STANDARD) {
		scll = scl - (scl / 3U) - I2C_OMAP_SCLL_TRIM;
		sclh = (scl / 3U) - I2C_OMAP_SCLH_TRIM;
	} else {
		scll = (scl / 2U) - I2C_OMAP_SCLL_TRIM;
		sclh = (scl / 2U) - I2C_OMAP_SCLH_TRIM;
	}

	if ((scll > 0xffU) || (sclh > 0xffU)) {
		return -ERANGE;
	}

	data->speed_config.pscstate = psc;
	data->speed_config.scllstate = scll;
	data->speed_config.sclhstate = sclh;
	data->speed = speed;

	return 0;
}

/**
 * @brief Apply controller bitrate / mode (no bus lock).
 *
 * Used directly from the RTIO CONFIGURE SQE path and under @ref data->lock
 * from the classic configure API.
 */
static int i2c_omap_do_configure(const struct device *dev, uint32_t dev_config)
{
	uint32_t speed_cfg = I2C_BITRATE_STANDARD;
	struct i2c_omap_data *data = DEV_DATA(dev);
	int ret;

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed_cfg = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		speed_cfg = I2C_BITRATE_FAST;
		break;
	default:
		return -ENOTSUP;
	}
	if ((dev_config & I2C_MODE_CONTROLLER) != I2C_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	ret = i2c_omap_set_speed(dev, speed_cfg);
	if (ret == 0) {
		i2c_omap_init_ll(dev);
		data->dev_config = dev_config;
	}
	return ret;
}

/**
 * @brief Configure the OMAP I2C controller with the specified device configuration.
 *
 * This function configures the OMAP I2C controller with the specified device configuration.
 *
 * @param dev The pointer to the device structure.
 * @param dev_config The device configuration to be applied.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_omap_configure(const struct device *dev, uint32_t dev_config)
{
#ifdef CONFIG_I2C_RTIO
	struct i2c_omap_data *data = DEV_DATA(dev);

	return i2c_rtio_configure(data->ctx, dev_config);
#else
	struct i2c_omap_data *data = DEV_DATA(dev);
	int ret;

	k_sem_take(&data->lock, K_FOREVER);
	ret = i2c_omap_do_configure(dev, dev_config);
	k_sem_give(&data->lock);
	return ret;
#endif
}

static int i2c_omap_get_config(const struct device *dev, uint32_t *config)
{
	struct i2c_omap_data *data = DEV_DATA(dev);

	*config = data->dev_config;
	return 0;
}

/**
 * @brief Transmit or receive data over I2C bus
 *
 * This function transmits or receives data over the I2C bus using the OMAP I2C controller.
 *
 * @param dev Pointer to the I2C device structure
 * @param num_bytes Number of bytes to transmit or receive
 */
static void i2c_omap_transmit_receive_data(const struct device *dev, uint8_t num_bytes)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint8_t *buf_ptr = data->current_msg.buf;

	while (num_bytes--) {
		if (data->receiver) {
			*buf_ptr++ = i2c_base_addr->DATA;
		} else {
			i2c_base_addr->DATA = *(buf_ptr++);
		}
		data->current_msg.len--;
		data->current_msg.buf = buf_ptr;
	}
}

/**
 * @brief Resize the FIFO buffer for the OMAP I2C controller.
 *
 * This function resizes the FIFO buffer for the OMAP I2C controller based on the specified size.
 * It clears the RX threshold and sets the new size for the receiver, or clears the TX threshold
 * and sets the new size for the transmitter.
 *
 * @param dev Pointer to the device structure.
 * @param size The new size of the FIFO buffer.
 */
static void i2c_omap_resize_fifo(const struct device *dev, uint8_t size)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint32_t buf;
	uint8_t threshold;

	if (data->fifo_size == 0U) {
		return;
	}

	threshold = CLAMP(size, 1U, data->fifo_size);

	buf = i2c_base_addr->BUF;
	if (data->receiver) {
		buf &= ~I2C_BUF_RXTRSH_MASK;
		buf |= FIELD_PREP(I2C_BUF_RXTRSH_MASK, threshold - 1U) | I2C_OMAP_BUF_RXFIF_CLR;
	} else {
		buf &= ~I2C_BUF_TXTRSH_MASK;
		buf |= FIELD_PREP(I2C_BUF_TXTRSH_MASK, threshold - 1U) | I2C_OMAP_BUF_TXFIF_CLR;
	}
	i2c_base_addr->BUF = buf;
}

#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
/**
 * @brief Get the state of the SDA line.
 *
 * This function retrieves the state of the SDA (data) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @return The state of the SDA line.
 */
static int i2c_omap_get_sda(void *io_context)
{
	const struct device *dev = io_context;
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	return (i2c_base_addr->SYSTEST & I2C_OMAP_SYSTEST_SDA_I_FUNC) ? 1 : 0;
}

/**
 * @brief Set the state of the SDA line.
 *
 * This function sets the state of the SDA (data) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @param state The state to set (0 for low, 1 for high).
 */
static void i2c_omap_set_sda(void *io_context, int state)
{
	const struct device *dev = io_context;
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	if (state) {
		i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_SDA_O;
	} else {
		i2c_base_addr->SYSTEST &= ~I2C_OMAP_SYSTEST_SDA_O;
	}
}

/**
 * @brief Set the state of the SCL line.
 *
 * This function sets the state of the SCL (clock) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @param state The state to set (0 for low, 1 for high).
 */
static void i2c_omap_set_scl(void *io_context, int state)
{
	const struct device *dev = io_context;
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	if (state) {
		i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_SCL_O;
	} else {
		i2c_base_addr->SYSTEST &= ~I2C_OMAP_SYSTEST_SCL_O;
	}
}
/**
 * @brief Recovers the I2C bus using the OMAP I2C controller.
 *
 * This function attempts to recover the I2C bus by performing a bus recovery
 * sequence using the OMAP I2C controller. It uses the provided device
 * configuration and bit-banging operations to recover the bus.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, negative error code on failure.
 */

static int i2c_omap_do_recover_bus(const struct device *dev)
{
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	struct i2c_bitbang bitbang_omap;
	struct i2c_bitbang_io bitbang_omap_io = {
		.get_sda = i2c_omap_get_sda,
		.set_scl = i2c_omap_set_scl,
		.set_sda = i2c_omap_set_sda,
	};
	int error;

	i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_ST_EN | (3 << I2C_OMAP_SYSTEST_TMODE_SHIFT) |
				  I2C_OMAP_SYSTEST_SCL_O | I2C_OMAP_SYSTEST_SDA_O;
	i2c_bitbang_init(&bitbang_omap, &bitbang_omap_io, (void *)dev);
	error = i2c_bitbang_recover_bus(&bitbang_omap);
	if (error != 0) {
		LOG_ERR("failed to recover bus (err %d)", error);
	}

	i2c_base_addr->SYSTEST &= ~(I2C_OMAP_SYSTEST_ST_EN | I2C_OMAP_SYSTEST_TMODE_MASK |
				    I2C_OMAP_SYSTEST_SCL_O | I2C_OMAP_SYSTEST_SDA_O);
	i2c_omap_reset(dev);
	i2c_omap_init_ll(dev);
	return error;
}

static int i2c_omap_recover_bus(const struct device *dev)
{
#ifdef CONFIG_I2C_RTIO
	struct i2c_omap_data *data = DEV_DATA(dev);

	return i2c_rtio_recover(data->ctx);
#else
	struct i2c_omap_data *data = DEV_DATA(dev);
	int error;

	k_sem_take(&data->lock, K_FOREVER);
	error = i2c_omap_do_recover_bus(dev);
	k_sem_give(&data->lock);
	return error;
#endif
}
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */

/**
 * @brief Wait for the bus to become free (no longer busy).
 *
 * This function waits for the bus to become free by continuously checking the
 * status register of the OMAP I2C controller. If the bus remains busy for a
 * certain timeout period, the function will return attempts to recover the bus by calling
 * i2c_omap_recover_bus().
 *
 * @param dev The I2C device structure.
 * @return 0 if the bus becomes free, or a negative error code if the bus cannot
 * be recovered.
 */
static int i2c_omap_wait_for_bb(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint32_t timeout = k_uptime_get_32() + I2C_OMAP_HW_TIMEOUT_MS;

	if (!data->bb_valid) {
		return 0;
	}

	while (i2c_base_addr->IRQSTATUS & I2C_OMAP_STAT_BB) {
		if (k_uptime_get_32() > timeout) {
			LOG_ERR("Bus busy timeout");
#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
			return i2c_omap_do_recover_bus(dev);
#else
			(void)i2c_omap_reset(dev);
			i2c_omap_init_ll(dev);
			return -ETIMEDOUT;
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */
		}
		k_busy_wait(100);
	}
	return 0;
}

/**
 * @brief Performs data transfer for the OMAP I2C driver.
 *
 * This function is responsible for handling the data transfer logic for the OMAP I2C driver.
 * It reads the status register and performs the necessary actions based on the status flags.
 * It handles both receive and transmit logic, and also handles error conditions such as NACK,
 * arbitration lost, receive overrun, and transmit underflow.
 *
 * @param dev Pointer to the device structure.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int i2c_omap_transfer_message_ll(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	/*
	 * IP v2: poll IRQSTATUS_RAW; ack via write-1-to-clear on IRQSTATUS
	 * (Linux: read RAW, write STAT_REG mapped to IRQSTATUS).
	 */
	uint32_t irq_raw = i2c_base_addr->IRQSTATUS_RAW;
	uint32_t result = 0;
	uint8_t num_bytes;

	if (data->receiver) {
		irq_raw &= ~(I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_XRDY);
	} else {
		irq_raw &= ~(I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_RRDY);
	}
	if (irq_raw & I2C_OMAP_STAT_NACK) {
		result |= I2C_OMAP_STAT_NACK;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_NACK;
	}
	if (irq_raw & I2C_OMAP_STAT_AL) {
		result |= I2C_OMAP_STAT_AL;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_AL;
	}
	if (irq_raw & I2C_OMAP_STAT_ARDY) {
		/* ProDB0017052: clear ARDY twice */
		/* Mannnn, this took a long time to figure it out. */
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_ARDY;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_ARDY;
	}
	if (irq_raw & (I2C_OMAP_STAT_ARDY | I2C_OMAP_STAT_NACK | I2C_OMAP_STAT_AL)) {

		i2c_base_addr->IRQSTATUS =
			(I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_XRDY |
			 I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_ARDY);
		return result;
	}

	/* Handle receive logic */
	if (irq_raw & (I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR)) {
		num_bytes = FIELD_GET(I2C_BUFSTAT_RX_MASK, i2c_base_addr->BUFSTAT);
		if (num_bytes > data->current_msg.len) {
			num_bytes = data->current_msg.len;
		}
		if (num_bytes > 0) {
			i2c_omap_transmit_receive_data(dev, num_bytes);
		}
		i2c_base_addr->IRQSTATUS =
			(irq_raw & I2C_OMAP_STAT_RRDY) ? I2C_OMAP_STAT_RRDY : I2C_OMAP_STAT_RDR;
		return RETRY;
	}

	/* Handle transmit logic */
	if (irq_raw & (I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_XDR)) {
		num_bytes = FIELD_GET(I2C_BUFSTAT_TX_MASK, i2c_base_addr->BUFSTAT);
		if (num_bytes > data->current_msg.len) {
			num_bytes = data->current_msg.len;
		}
		if (num_bytes > 0) {
			i2c_omap_transmit_receive_data(dev, num_bytes);
		}
		i2c_base_addr->IRQSTATUS =
			(irq_raw & I2C_OMAP_STAT_XRDY) ? I2C_OMAP_STAT_XRDY : I2C_OMAP_STAT_XDR;
		return RETRY;
	}

	if (irq_raw & I2C_OMAP_STAT_ROVR) {
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_ROVR;
		return I2C_OMAP_STAT_ROVR;
	}
	if (irq_raw & I2C_OMAP_STAT_XUDF) {
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_XUDF;
		return I2C_OMAP_STAT_XUDF;
	}
	return RETRY;
}

/**
 * @brief Map HW status bits from transfer_message_ll to a Zephyr errno.
 *
 * May issue STOP / soft-reset as a side effect of error recovery.
 */
static int i2c_omap_ll_result_to_err(const struct device *dev, int result)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	if (!result) {
		return 0;
	}

	if (result & (I2C_OMAP_STAT_ROVR | I2C_OMAP_STAT_XUDF)) {
		i2c_omap_reset(dev);
		i2c_omap_init_ll(dev);
		return -EIO;
	}
	if (result & I2C_OMAP_STAT_AL) {
		return -EAGAIN;
	}
	if (result & I2C_OMAP_STAT_NACK) {
		i2c_base_addr->CON |= I2C_OMAP_CON_STP;
		data->bb_valid = true;
		i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_CLR_MASK;
		return -ENOMSG;
	}

	return -EIO;
}

/**
 * @brief Arm the controller for a single message (interrupt or poll completion).
 *
 * Does not wait for completion. On success with interrupt mode, @c xfer_active
 * is set and the ISR finishes the transfer.
 */
static int i2c_omap_msg_start(const struct device *dev, struct i2c_msg *msg, uint16_t addr)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	volatile i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint16_t control_reg;
	struct i2c_msg probe_msg;

	/*
	 * OMAP I2C does not allow CNT=0 (Linux I2C_AQ_NO_ZERO_LEN). Emulate
	 * Zephyr `i2c scan` zero-length writes as a 1-byte read (i2cdetect -r).
	 */
	if (msg->len == 0U) {
		data->discard = 0U;
		probe_msg.buf = &data->discard;
		probe_msg.len = 1U;
		probe_msg.flags = I2C_MSG_READ | (msg->flags & I2C_MSG_STOP);
		msg = &probe_msg;
	}

	/* Determine message direction (read or write) and update the receiver flag */
	data->receiver = msg->flags & I2C_MSG_READ;
	/* Adjust the FIFO size according to the message length */
	i2c_omap_resize_fifo(dev, (uint8_t)MIN(msg->len, 255U));
	/* Set the target I2C address for the transfer */
	i2c_base_addr->SA = addr;
	/* Store the message in the data structure */
	data->current_msg = *msg;
	/* Set the message length in the I2C controller */
	i2c_base_addr->CNT = msg->len;
	/* Clear FIFO buffers */
	control_reg = i2c_base_addr->BUF;
	control_reg |= I2C_OMAP_BUF_RXFIF_CLR | I2C_OMAP_BUF_TXFIF_CLR;
	i2c_base_addr->BUF = control_reg;
	/* Prepare the control register for the I2C operation */
	control_reg = I2C_OMAP_CON_EN | I2C_OMAP_CON_MST | I2C_OMAP_CON_STT;
	/* Enable high-speed mode if required by the transfer speed */
	if (data->speed > I2C_BITRATE_FAST) {
		control_reg |= I2C_OMAP_CON_OPMODE_HS;
	}
	/* Set the STOP condition if it's specified in the message flags */
	if (msg->flags & I2C_MSG_STOP) {
		control_reg |= I2C_OMAP_CON_STP;
	}
	/* Set the transmission mode based on whether it's a read or write operation */
	if (!(msg->flags & I2C_MSG_READ)) {
		control_reg |= I2C_OMAP_CON_TRX;
	}
	/* Drop stale ARDY/NACK from a previous STP before arming STT */
	i2c_base_addr->IRQSTATUS = I2C_OMAP_STAT_CLR_MASK;

#ifdef CONFIG_I2C_OMAP_INTERRUPT
	data->xfer_active = true;
#endif
	i2c_base_addr->CON = control_reg;
	return 0;
}

#ifndef CONFIG_I2C_RTIO
/**
 * @brief Performs an I2C transfer of a single message.
 *
 * This function is responsible for performing an I2C transfer of a single message.
 * It sets up the necessary configurations, writes the target device address,
 * sets the buffer and buffer length, and handles various error conditions.
 *
 * @param dev The I2C device structure.
 * @param msg Pointer to the I2C message structure.
 * @param polling Flag indicating whether to use polling mode or not.
 * @param addr The target device address.
 *
 * @return 0 on success, negative error code on failure.
 *         Possible error codes include:
 *         - ETIMEDOUT: Timeout occurred during the transfer.
 *         - EIO: I/O error due to receiver overrun or transmit underflow.
 *         - EAGAIN: Arbitration lost error, try again.
 *         - ENOMSG: Message error due to NACK.
 */
static int i2c_omap_transfer_message(const struct device *dev, struct i2c_msg *msg, bool polling,
				     uint16_t addr)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	k_timepoint_t end;
	int result = 0;

	end = sys_timepoint_calc(K_MSEC(I2C_OMAP_MSG_TIMEOUT_MS));

#ifdef CONFIG_I2C_OMAP_INTERRUPT
	if (!polling) {
		k_sem_reset(&data->xfer_done);
		(void)i2c_omap_msg_start(dev, msg, addr);
		if (k_sem_take(&data->xfer_done, sys_timepoint_timeout(end)) != 0) {
			data->xfer_active = false;
			(void)i2c_omap_reset(dev);
			i2c_omap_init_ll(dev);
			return -ETIMEDOUT;
		}
		result = (int)data->xfer_ll_result;
	} else {
#endif /* CONFIG_I2C_OMAP_INTERRUPT */
		(void)i2c_omap_msg_start(dev, msg, addr);
		do {
			result = i2c_omap_transfer_message_ll(dev);
		} while (result == RETRY && !sys_timepoint_expired(end));

		if (result == RETRY) {
			(void)i2c_omap_reset(dev);
			i2c_omap_init_ll(dev);
			return -ETIMEDOUT;
		}
#ifdef CONFIG_I2C_OMAP_INTERRUPT
	}
#endif /* CONFIG_I2C_OMAP_INTERRUPT */

	return i2c_omap_ll_result_to_err(dev, result);
}

/**
 * @brief Performs a common transfer operation for OMAP I2C devices.
 *
 * This function is responsible for transferring multiple I2C messages in a common way
 * for OMAP I2C devices. It waits for the bus to be idle, then iterates through each
 * message in the provided array and transfers them one by one using the i2c_omap_transfer_message()
 * function. After all messages have been transferred, it waits for the bus to be idle again
 * before returning.
 *
 * @param dev The pointer to the I2C device structure.
 * @param msg An array of I2C messages to be transferred.
 * @param num The number of messages in the array.
 * @param polling Specifies whether to use polling or interrupt-based transfer.
 * @param addr The I2C target address.
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_omap_transfer_main(const struct device *dev, struct i2c_msg msg[], int num,
				  bool polling, uint16_t addr)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

#if defined(CONFIG_I2C_TARGET)
	if (data->target_cfg != NULL) {
		k_sem_give(&data->lock);
		return -EBUSY;
	}
#endif

	ret = i2c_omap_wait_for_bb(dev);
	if (ret < 0) {
		k_sem_give(&data->lock);
		return ret;
	}
	for (int msg_idx = 0; msg_idx < num; msg_idx++) {
		ret = i2c_omap_transfer_message(dev, &msg[msg_idx], polling, addr);
		if (ret < 0) {
			break;
		}
	}
	data->bb_valid = true;
	(void)i2c_omap_wait_for_bb(dev);
	k_sem_give(&data->lock);
	return ret;
}
#endif /* !CONFIG_I2C_RTIO */

/**
 * @brief OMAP I2C transfer function using polling.
 *
 * This function performs the I2C transfer using the OMAP I2C controller
 * in polling mode. It calls the common transfer function with the
 * specified messages, number of messages, and target address.
 *
 * @param dev Pointer to the I2C device structure.
 * @param msgs Array of I2C messages to be transferred.
 * @param num_msgs Number of I2C messages in the array.
 * @param addr Target address.
 * @return 0 on success, negative error code on failure.
 */
#ifdef CONFIG_I2C_RTIO
static bool i2c_omap_rtio_start(const struct device *dev, int *status)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	struct i2c_rtio *ctx = data->ctx;
	struct rtio_sqe *sqe = &ctx->txn_curr->sqe;
	struct i2c_dt_spec *dt_spec = sqe->iodev->data;
	struct i2c_msg msg;
	int ret;

	switch (sqe->op) {
	case RTIO_OP_RX:
		msg.buf = sqe->rx.buf;
		msg.len = sqe->rx.buf_len;
		msg.flags = I2C_MSG_READ | sqe->iodev_flags;
		break;
	case RTIO_OP_TINY_TX:
		msg.buf = (uint8_t *)sqe->tiny_tx.buf;
		msg.len = sqe->tiny_tx.buf_len;
		msg.flags = I2C_MSG_WRITE | sqe->iodev_flags;
		break;
	case RTIO_OP_TX:
		msg.buf = (uint8_t *)sqe->tx.buf;
		msg.len = sqe->tx.buf_len;
		msg.flags = I2C_MSG_WRITE | sqe->iodev_flags;
		break;
	case RTIO_OP_I2C_CONFIGURE:
		*status = i2c_omap_do_configure(dev, sqe->i2c_config);
		return false;
#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
	case RTIO_OP_I2C_RECOVER:
		*status = i2c_omap_do_recover_bus(dev);
		return false;
#endif
	default:
		LOG_ERR("Invalid op code %d for submission %p", sqe->op, (void *)sqe);
		*status = -EINVAL;
		return false;
	}

#if defined(CONFIG_I2C_TARGET)
	if (data->target_cfg != NULL) {
		*status = -EBUSY;
		return false;
	}
#endif

	if (ctx->txn_curr == ctx->txn_head) {
		ret = i2c_omap_wait_for_bb(dev);
		if (ret < 0) {
			*status = ret;
			return false;
		}
	}

	ret = i2c_omap_msg_start(dev, &msg, dt_spec->addr);
	if (ret < 0) {
		*status = ret;
		return false;
	}

	return true;
}

static void i2c_omap_rtio_complete(const struct device *dev, int status)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_complete(ctx, status)) {
		(void)i2c_rtio_run_sync_start_async(dev, ctx, i2c_omap_rtio_start);
	} else {
		/* Defer BB wait to the next transaction start (ISR context). */
		data->bb_valid = true;
	}
}

static void i2c_omap_iodev_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	struct i2c_rtio *const ctx = data->ctx;

	if (i2c_rtio_submit(ctx, iodev_sqe)) {
		(void)i2c_rtio_run_sync_start_async(dev, ctx, i2c_omap_rtio_start);
	}
}

static int i2c_omap_transfer(const struct device *dev, struct i2c_msg msgs[], uint8_t num_msgs,
			     uint16_t addr)
{
	struct i2c_omap_data *data = DEV_DATA(dev);

	return i2c_rtio_transfer(data->ctx, msgs, num_msgs, addr);
}
#elif defined(CONFIG_I2C_OMAP_INTERRUPT)
static int i2c_omap_transfer(const struct device *dev, struct i2c_msg msgs[], uint8_t num_msgs,
			     uint16_t addr)
{
	return i2c_omap_transfer_main(dev, msgs, num_msgs, false, addr);
}
#else
static int i2c_omap_transfer_polling(const struct device *dev, struct i2c_msg msgs[],
						    uint8_t num_msgs, uint16_t addr)
{
	return i2c_omap_transfer_main(dev, msgs, num_msgs, true, addr);
}
#endif

static DEVICE_API(i2c, i2c_omap_api) = {
#if defined(CONFIG_I2C_RTIO) || defined(CONFIG_I2C_OMAP_INTERRUPT)
	.transfer = i2c_omap_transfer,
#else
	.transfer = i2c_omap_transfer_polling,
#endif
	.configure = i2c_omap_configure,
	.get_config = i2c_omap_get_config,
#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
	.recover_bus = i2c_omap_recover_bus,
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */
#if defined(CONFIG_I2C_TARGET)
	.target_register = i2c_omap_target_register,
	.target_unregister = i2c_omap_target_unregister,
#endif
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_omap_iodev_submit,
#endif
};

/**
 * @brief Initialize the OMAP I2C controller.
 *
 * This function initializes the OMAP I2C controller by setting the speed and
 * performing any necessary initialization steps.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @return 0 if successful, negative error code otherwise.
 */
static int i2c_omap_init(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	const struct i2c_omap_cfg *cfg = DEV_CFG(dev);
	uint32_t fifo_depth_code;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}

#ifdef CONFIG_CLOCK_CONTROL_TISCI
	if (cfg->clock_dev != NULL && cfg->clock_subsys != NULL) {
		const struct tisci_clock_config *clk_cfg = cfg->clock_subsys;
		const struct device *dmsc = DEVICE_DT_GET(DT_NODELABEL(dmsc));

		ret = tisci_cmd_get_device(dmsc, clk_cfg->dev_id);
		if (ret < 0) {
			LOG_ERR("tisci_cmd_get_device(%u) failed (%d)", clk_cfg->dev_id, ret);
			return ret;
		}
	}
#endif

	k_sem_init(&data->lock, 1, 1);
	/* Set the speed for I2C */
	if (i2c_omap_set_speed(dev, cfg->speed)) {
		LOG_ERR("Failed to set speed");
		return -ENOTSUP;
	}

	ret = i2c_omap_reset(dev);
	if (ret < 0) {
		return ret;
	}

	fifo_depth_code = FIELD_GET(I2C_BUFSTAT_FIFODEPTH_MASK, DEV_I2C_BASE(dev)->BUFSTAT);
	data->fifo_size = (uint8_t)((8U << fifo_depth_code) / 2U);
	if (data->fifo_size == 0U) {
		data->fifo_size = 8U;
	}

#ifdef CONFIG_I2C_OMAP_INTERRUPT
	k_sem_init(&data->xfer_done, 0, 1);
	data->xfer_active = false;
	i2c_omap_update_irq_enable(dev);
#endif

	i2c_omap_init_ll(dev);

	data->dev_config = I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(cfg->speed);

#ifdef CONFIG_I2C_RTIO
	i2c_rtio_init(data->ctx, dev);
#endif

	return 0;
}

#define I2C_OMAP_CLOCK_DEV(inst)                                                                   \
	COND_CODE_1(DT_INST_CLOCKS_HAS_IDX(inst, 0),                                               \
		    (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL))

#define I2C_OMAP_DEFINE_CLK_SUBSYS(inst)                                                           \
	COND_CODE_1(DT_INST_CLOCKS_HAS_IDX(inst, 0),                                               \
	(                                                                                          \
		COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (                                          \
			static struct tisci_clock_config i2c_omap_tisci_clk_##inst =               \
				TISCI_GET_CLOCK_DETAILS_BY_INST(inst);                             \
			static const clock_control_subsys_t i2c_omap_clk_subsys_##inst =           \
				&i2c_omap_tisci_clk_##inst;                                        \
		), (COND_CODE_1(CONFIG_CLOCK_CONTROL_ARM_SCMI, (                                   \
			static const clock_control_subsys_t i2c_omap_clk_subsys_##inst =           \
				(clock_control_subsys_t)DT_INST_PHA(inst, clocks, name);           \
		), (                                                                               \
			BUILD_ASSERT(0, "Unsupported clock controller for ti,omap-i2c");           \
			static const clock_control_subsys_t i2c_omap_clk_subsys_##inst;     \
		))))                                                                               \
	), (                                                                                       \
		static const clock_control_subsys_t i2c_omap_clk_subsys_##inst;             \
	))

#define I2C_OMAP_IRQ_CONFIG_DEFINE(inst)                                                           \
	static void i2c_omap_irq_config_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), i2c_omap_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define I2C_OMAP_INIT(inst)                                                                        \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	I2C_OMAP_DEFINE_CLK_SUBSYS(inst);                                                          \
	LOG_INSTANCE_REGISTER(omap_i2c, inst, CONFIG_I2C_LOG_LEVEL);                               \
	IF_ENABLED(CONFIG_I2C_OMAP_INTERRUPT, (I2C_OMAP_IRQ_CONFIG_DEFINE(inst);))                 \
	IF_ENABLED(CONFIG_I2C_RTIO,                                                                \
		   (I2C_RTIO_DEFINE(_i2c##inst##_omap_rtio,                                        \
				    DT_INST_PROP_OR(inst, sq_size, CONFIG_I2C_RTIO_SQ_SIZE),      \
				    DT_INST_PROP_OR(inst, cq_size, CONFIG_I2C_RTIO_CQ_SIZE));))   \
	static const struct i2c_omap_cfg i2c_omap_cfg_##inst = {                                   \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.irq = DT_INST_IRQN(inst),                                                         \
		.speed = DT_INST_PROP(inst, clock_frequency),                                      \
		.clock_dev = I2C_OMAP_CLOCK_DEV(inst),                                             \
		.clock_subsys = i2c_omap_clk_subsys_##inst,                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	static struct i2c_omap_data i2c_omap_data_##inst = {                                       \
		IF_ENABLED(CONFIG_I2C_RTIO, (.ctx = &_i2c##inst##_omap_rtio,))                     \
	};                                                                                         \
                                                                                                   \
	static int i2c_omap_init_##inst(const struct device *dev)                                  \
	{                                                                                          \
		int ret = i2c_omap_init(dev);                                                      \
                                                                                                   \
		if (ret == 0) {                                                                    \
			IF_ENABLED(CONFIG_I2C_OMAP_INTERRUPT,                                      \
				   (i2c_omap_irq_config_##inst();))                                \
		}                                                                                  \
		return ret;                                                                        \
	}                                                                                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst,                                                            \
		i2c_omap_init_##inst,                                                              \
		NULL,                                                                              \
		&i2c_omap_data_##inst,                                                             \
		&i2c_omap_cfg_##inst,                                                              \
		POST_KERNEL,                                                                       \
		CONFIG_I2C_INIT_PRIORITY,                                                          \
		&i2c_omap_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_OMAP_INIT)

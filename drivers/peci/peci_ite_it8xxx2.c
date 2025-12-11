/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_peci

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/peci.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(peci_ite_it8xxx2, CONFIG_PECI_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_PECI_INTERRUPT_DRIVEN),
	     "Please enable the option CONFIG_PECI_INTERRUPT_DRIVEN");

/*
 * This driver is single-instance. If the devicetree contains multiple
 * instances, this will fail and the driver needs to be revisited.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Unsupported PECI Instance");

/* The following constants describes the bitrate of it8xxx2 PECI,
 * for the frequency are 2000KHz, 1000KHz, and 1600KHz. (Unit: KHz)
 */
#define	PECI_IT8XXX2_BITRATE_2MHZ		2000
#define	PECI_IT8XXX2_BITRATE_1MHZ		1000
#define	PECI_IT8XXX2_BITRATE_1P6MHZ		1600

/* The following masks are designed for the PECI bitrate settings,
 * for the bits[7:3] are not related to this features.
 */
#define	PECI_IT8XXX2_BITRATE_BITS_MASK		0x07
#define	PECI_IT8XXX2_BITRATE_2MHZ_BITS		0x00
#define	PECI_IT8XXX2_BITRATE_1MHZ_BITS		0x01
#define	PECI_IT8XXX2_BITRATE_1P6MHZ_BITS	0x04

/* The Transaction Timeout */
#define PECI_TIMEOUT_MS		30

/* PECI interface 0 */
#define PECI0			0

/* HOSTAR (F02C00h) */
#define HOBY			BIT(0)
#define FINISH			BIT(1)
#define RD_FCS_ERR		BIT(2)
#define WR_FCS_ERR		BIT(3)
#define EXTERR			BIT(5)
#define BUS_ER			BIT(6)
#define TEMPERR			BIT(7)
#define HOSTAR_RST_ANYBIT \
		(TEMPERR|BUS_ER|EXTERR|WR_FCS_ERR|RD_FCS_ERR|FINISH)

/* HOCTLR (F02C01h) */
#define START			BIT(0)
#define AWFCS_EN		BIT(1)
#define CONTROL			BIT(2)
#define PECIHEN			BIT(3)
#define FCSERR_ABT		BIT(4)
#define FIFOCLR			BIT(5)

/*
 * TODO: The Voltage Configuration
 *       Related DTSi and registers settings should be fulfilled
 *       in the future.
 */
/* PADCTLR (F02C0Eh) */
#define PECI_DVIE		0x04

enum peci_vtts {
	HOVTTS0P85V = 0x00,
	HOVTTS0P90V = 0x01,
	HOVTTS0P95V = 0x02,
	HOVTTS1P00V = 0x03,
	HOVTTS1P05V = 0x08,
	HOVTTS1P10V = 0x09,
	HOVTTS1P15V = 0x0A,
	HOVTTS1P20V = 0x0B,
	HOVTTS1P25V = 0x10,
};

struct peci_it8xxx2_config {
	uintptr_t base_addr;
	uint8_t irq_no;
	const struct pinctrl_dev_config *pcfg;
};

struct peci_it8xxx2_data {
	struct peci_msg *msgs;
	struct k_sem device_sync_sem;
	uint32_t bitrate;
};

PINCTRL_DT_INST_DEFINE(0);

static const struct peci_it8xxx2_config peci_it8xxx2_config0 = {
	.base_addr = DT_INST_REG_ADDR(0),
	.irq_no = DT_INST_IRQN(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct peci_it8xxx2_data peci_it8xxx2_data0;

/* ITE IT8XXX2 PECI Functions */

static void peci_it8xxx2_init_vtts(struct peci_it8xxx2_regs *reg_base,
					enum peci_vtts vol_opt)
{
	reg_base->PADCTLR = (reg_base->PADCTLR & PECI_DVIE) | vol_opt;
}

static void peci_it8xxx2_rst_status(struct peci_it8xxx2_regs *reg_base)
{
	reg_base->HOSTAR = HOSTAR_RST_ANYBIT;
}

static int peci_it8xxx2_check_host_busy(struct peci_it8xxx2_regs *reg_base)
{
	return (reg_base->HOSTAR & HOBY) ? (-EBUSY) : 0;
}

static int peci_it8xxx2_check_host_finish(const struct device *dev)
{
	struct peci_it8xxx2_data *data = dev->data;
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;

	int ret = k_sem_take(&data->device_sync_sem, K_MSEC(PECI_TIMEOUT_MS));

	if (ret == -EAGAIN) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (peci_regs->HOSTAR != FINISH) {
		LOG_ERR("[PECI] Error: HOSTAR=0x%02X\r\n", peci_regs->HOSTAR);
			return -EIO;
	}

	return 0;
}

static int peci_it8xxx2_configure(const struct device *dev, uint32_t bitrate)
{
	struct peci_it8xxx2_data *data = dev->data;
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;

	uint8_t hoctl2r_to_write;

	data->bitrate =  bitrate;

	hoctl2r_to_write =
		(peci_regs->HOCTL2R) & (~(PECI_IT8XXX2_BITRATE_BITS_MASK));

	switch (bitrate) {
	case PECI_IT8XXX2_BITRATE_2MHZ:
		break;

	case PECI_IT8XXX2_BITRATE_1MHZ:
		hoctl2r_to_write |= PECI_IT8XXX2_BITRATE_1MHZ_BITS;
		break;

	case PECI_IT8XXX2_BITRATE_1P6MHZ:
		hoctl2r_to_write |= PECI_IT8XXX2_BITRATE_1P6MHZ_BITS;
		break;

	default:
		LOG_ERR("[PECI] Error: Specified Bitrate Not Supported\r\n");
		hoctl2r_to_write |= PECI_IT8XXX2_BITRATE_1MHZ_BITS;
		data->bitrate =  PECI_IT8XXX2_BITRATE_1MHZ;
		peci_regs->HOCTL2R = hoctl2r_to_write;
		return -ENOTSUP;
	}

	peci_regs->HOCTL2R = hoctl2r_to_write;

	return 0;
}

static int peci_it8xxx2_enable(const struct device *dev)
{
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;

	peci_regs->HOCTLR |= (FIFOCLR|FCSERR_ABT|PECIHEN|CONTROL);

	return 0;
}

static int peci_it8xxx2_disable(const struct device *dev)
{
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;

	peci_regs->HOCTLR &= ~(PECIHEN);
	return 0;
}

static void peci_it8xxx2_rst_module(const struct device *dev)
{
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;
	struct gctrl_it8xxx2_regs *const gctrl_regs = GCTRL_IT8XXX2_REGS_BASE;

	LOG_ERR("[PECI] Module Reset for Status Error.\r\n");
	/* Reset IT8XXX2 PECI Module Thoroughly */
	gctrl_regs->GCTRL_RSTC4 |= IT8XXX2_GCTRL_RPECI;
	/*
	 * Due to the fact that we've checked if the peci_enable()
	 * called before calling the peci_transfer(), so the peci
	 * were definitely enabled before the error occurred.
	 * Here is the recovery mechanism for recovering the PECI
	 * bus when the errors occur.
	 */
	peci_regs->PADCTLR |= PECI_DVIE;
	peci_it8xxx2_init_vtts(peci_regs, HOVTTS0P95V);
	peci_it8xxx2_configure(dev, PECI_IT8XXX2_BITRATE_1MHZ);
	peci_it8xxx2_enable(dev);
	LOG_ERR("[PECI] Reinitialization Finished.\r\n");
}

static int peci_it8xxx2_transfer(const struct device *dev, struct peci_msg *msg)
{
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;

	struct peci_buf *peci_rx_buf = &msg->rx_buffer;
	struct peci_buf *peci_tx_buf = &msg->tx_buffer;

	int cnt, ret_code;

	ret_code = 0;

	if (!(peci_regs->HOCTLR & PECIHEN)) {
		LOG_ERR("[PECI] Please call the peci_enable() first.\r\n");
		return -ECONNREFUSED;
	}

	if (peci_it8xxx2_check_host_busy(peci_regs) != 0) {
		return -EBUSY;
	}

	peci_regs->HOTRADDR = msg->addr;
	peci_regs->HOWRLR = peci_tx_buf->len;
	peci_regs->HORDLR = peci_rx_buf->len;
	peci_regs->HOCMDR = msg->cmd_code;

	if (msg->cmd_code != PECI_CMD_PING) {
		for (cnt = 0; cnt < (peci_tx_buf->len - 1); cnt++) {
			peci_regs->HOWRDR = peci_tx_buf->buf[cnt];
		}
	}

	/* Host Available */
	irq_enable(config->irq_no);
	peci_regs->HOCTLR |= START;
	ret_code = peci_it8xxx2_check_host_finish(dev);

	if (!ret_code) {
		/* Host Transactions Finished, Fetch Data from the regs */
		if (peci_rx_buf->len) {
			for (cnt = 0; cnt < (peci_rx_buf->len); cnt++) {
				peci_rx_buf->buf[cnt] = peci_regs->HORDDR;
			}
		}
		peci_it8xxx2_rst_status(peci_regs);

	} else {
		/* Host Transactions Failure */
		peci_it8xxx2_rst_module(dev);
	}

	return (ret_code);
}

static void peci_it8xxx2_isr(const struct device *dev)
{
	struct peci_it8xxx2_data *data = dev->data;
	const struct peci_it8xxx2_config *config = dev->config;

	irq_disable(config->irq_no);
	k_sem_give(&data->device_sync_sem);
}

static DEVICE_API(peci, peci_it8xxx2_driver_api) = {
	.config = peci_it8xxx2_configure,
	.enable = peci_it8xxx2_enable,
	.disable = peci_it8xxx2_disable,
	.transfer = peci_it8xxx2_transfer,
};

static int peci_it8xxx2_init(const struct device *dev)
{
	struct peci_it8xxx2_data *data = dev->data;
	const struct peci_it8xxx2_config *config = dev->config;
	struct peci_it8xxx2_regs *const peci_regs =
		(struct peci_it8xxx2_regs *)config->base_addr;
	int status;

	/* Initialize Semaphore */
	k_sem_init(&data->device_sync_sem, 0, 1);

	/* Configure the GPF6 to Alternative Function 3: PECI */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure PECI pins");
		return status;
	}

	peci_regs->PADCTLR |= PECI_DVIE;
	peci_it8xxx2_init_vtts(peci_regs, HOVTTS0P95V);
	peci_it8xxx2_configure(dev, PECI_IT8XXX2_BITRATE_1MHZ);

	/* Interrupt Assignment */
	IRQ_CONNECT(DT_INST_IRQN(0),
	0,
	peci_it8xxx2_isr,
	DEVICE_DT_INST_GET(0),
	0);

	return 0;
}

DEVICE_DT_INST_DEFINE(0,
	&peci_it8xxx2_init,
	NULL,
	&peci_it8xxx2_data0,
	&peci_it8xxx2_config0,
	POST_KERNEL, CONFIG_PECI_INIT_PRIORITY,
	&peci_it8xxx2_driver_api);

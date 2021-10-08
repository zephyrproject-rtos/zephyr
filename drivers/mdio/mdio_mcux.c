/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright (c) 2021 Bernhard Kraemer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_mdio

#include <errno.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include "mdio_mcux.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mdio_mcux, CONFIG_MDIO_LOG_LEVEL);

/*! @brief MDC frequency. */
#define MDC_FREQUENCY 2500000U
/*! @brief NanoSecond in one second. */
#define NANOSECOND_ONE_SECOND 1000000000U

struct mdio_mcux_dev_data {
	struct k_sem idle_sem;
	struct k_sem complete_sem;
};

/** MII - Register Layout Typedef */
struct mdio_mcux_regs {
		 uint8_t RESERVED_0[64];
	__IO uint32_t MMFR;	/**< MII Management Frame Register, offset: 0x40 */
	__IO uint32_t MSCR;	/**< MII Speed Control Register, offset: 0x44 */
};
#define MII_Type struct mdio_mcux_regs

struct mdio_mcux_dev_config {
	MII_Type * const base;
	int protocol;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_DATA(dev) ((struct mdio_mcux_dev_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct mdio_mcux_dev_config *const)(dev)->config)

static int mdio_transfer(const struct device *dev, uint8_t prtad,
			  uint8_t devad, uint8_t rw, uint16_t data_in,
			  uint16_t *data_out)
{
	const struct mdio_mcux_dev_config *const cfg = DEV_CFG(dev);
	struct mdio_mcux_dev_data *const data = DEV_DATA(dev);

	k_sem_take(&data->idle_sem, K_FOREVER);
	k_sem_take(&data->complete_sem, K_NO_WAIT);

	LOG_DBG("rw: %u, prtad: 0x%02x, devad: 0x%02x, data_in: 0x%04x", rw, prtad, devad, data_in);

	/* Write mdio transaction */
	if (cfg->protocol == CLAUSE_45) {
		cfg->base->MMFR = ENET_MMFR_ST(0U)
					 | (ENET_MMFR_OP(rw ? 0x2 : 0x3))
				     | ENET_MMFR_PA(prtad)
				     | ENET_MMFR_RA(devad)
				     | ENET_MMFR_TA(2U)
				     | ENET_MMFR_DATA(data_in);
	} else if (cfg->protocol == CLAUSE_22) {
		cfg->base->MMFR = ENET_MMFR_ST(1U)
				     | (ENET_MMFR_OP(rw ? 0x2 : 0x1))
				     | ENET_MMFR_PA(prtad)
				     | ENET_MMFR_RA(devad)
					 | ENET_MMFR_TA(2U)
				     | ENET_MMFR_DATA(data_in);
	} else {
		LOG_ERR("Unsupported protocol");
	}

	/* Wait until done */
	if (k_sem_take(&data->complete_sem, K_MSEC(5)) != 0) {
		k_sem_give(&data->idle_sem);
		LOG_ERR("transfer timedout %s", DEV_NAME(dev));
		return -ETIMEDOUT;
	}

	if (data_out) {
		*data_out =
			(uint16_t)((cfg->base->MMFR & ENET_MMFR_DATA_MASK) >> ENET_MMFR_DATA_SHIFT);
		LOG_DBG("data_out: 0x%04x", *data_out);
	}

	k_sem_give(&data->idle_sem);
	return 0;
}

static int mdio_mcux_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			 uint16_t *data)
{
	return mdio_transfer(dev, prtad, devad, 1, 0, data);
}

static int mdio_mcux_write(const struct device *dev, uint8_t prtad,
			  uint8_t devad, uint16_t data)
{
	return mdio_transfer(dev, prtad, devad, 0, data, NULL);
}

static void mdio_mcux_bus_enable(const struct device *dev)
{
	const struct mdio_mcux_dev_config *const cfg = DEV_CFG(dev);
	uint32_t srcClock_Hz = CLOCK_GetFreq(kCLOCK_CoreSysClk);
	uint32_t clkCycle = 0;
	uint32_t speed    = 0;

	/* Due to bits limitation of SPEED and HOLDTIME, */
	/* srcClock_Hz must ensure MDC <= 2.5M and holdtime >= 10ns. */
	assert((srcClock_Hz != 0U) && (srcClock_Hz <= 320000000U));

	/* Use (param + N - 1) / N to increase accuracy with rounding. */
	/* Calculate the MII speed which controls the frequency of the MDC. */
	speed = (srcClock_Hz + 2U * MDC_FREQUENCY - 1U) / (2U * MDC_FREQUENCY) - 1U;
	/* Calculate the hold time on the MDIO output. */
	clkCycle = (10U + NANOSECOND_ONE_SECOND / srcClock_Hz - 1U) /
				(NANOSECOND_ONE_SECOND / srcClock_Hz) - 1U;

	cfg->base->MSCR = ENET_MSCR_MII_SPEED(speed) | ENET_MSCR_HOLDTIME(clkCycle);
}

static void mdio_mcux_bus_disable(const struct device *dev)
{
	const struct mdio_mcux_dev_config *const cfg = DEV_CFG(dev);

	cfg->base->MSCR = 0U;
}

static int mdio_mcux_initialize(const struct device *dev)
{
	struct mdio_mcux_dev_data *const data = DEV_DATA(dev);

	k_sem_init(&data->idle_sem, 1, 1);
	k_sem_init(&data->complete_sem, 0, 1);

	return 0;
}

static const struct mdio_driver_api mdio_mcux_driver_api = {
	.read = mdio_mcux_read,
	.write = mdio_mcux_write,
	.bus_enable = mdio_mcux_bus_enable,
	.bus_disable = mdio_mcux_bus_disable,
};

void z_impl_mdio_mcux_transfer_complete(const struct device *dev)
{
	struct mdio_mcux_dev_data *const data = DEV_DATA(dev);

	k_sem_give(&data->complete_sem);
}

#define MDIO_MCUX_CONFIG(n)						\
static const struct mdio_mcux_dev_config mdio_mcux_dev_config_##n = {	\
	.base = (MII_Type *)DT_REG_ADDR(DT_PARENT(DT_DRV_INST(n))),				\
	.protocol = DT_ENUM_IDX(DT_DRV_INST(n), protocol),		\
};

#define MDIO_MCUX_DEVICE(n)						\
	MDIO_MCUX_CONFIG(n);						\
	static struct mdio_mcux_dev_data mdio_mcux_dev_data##n;		\
	DEVICE_DT_INST_DEFINE(n,					\
			    &mdio_mcux_initialize,			\
			    NULL,					\
			    &mdio_mcux_dev_data##n,			\
			    &mdio_mcux_dev_config_##n, POST_KERNEL,	\
			    CONFIG_MDIO_INIT_PRIORITY,			\
			    &mdio_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_MCUX_DEVICE)

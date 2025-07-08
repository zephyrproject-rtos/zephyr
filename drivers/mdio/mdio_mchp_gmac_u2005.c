/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mdio_mchp_gmac_u2005.c
 * @brief MDIO driver for Microchip devices.
 *
 * This file provides the implementation of mdio functions
 * for Microchip-based systems.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/net/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_mchp_gmac_u2005, CONFIG_MDIO_LOG_LEVEL);

/* Define compatible string */
#define DT_DRV_COMPAT microchip_gmac_u2005_mdio

/**
 * @brief Define the HAL configuration for MDIO.
 *
 * This macro sets up the HAL configuration for the MDIO peripheral
 *
 * @param n Instance number.
 */
#define HAL_MCHP_MDIO_CONFIG(n) .regs = (gmac_registers_t *)DT_INST_REG_ADDR(n),

#define MDIO_MCHP_ESUCCESS 0

#define MDIO_MCHP_OP_TIMEOUT 50

/* Do the peripheral clock related configuration */

/**
 * @brief Clock configuration structure for the MDIO.
 *
 * This structure contains the clock configuration parameters for the MDIO
 * peripheral.
 */
typedef struct mchp_mdio_clock {
	/* Clock driver */
	const struct device *clock_dev;

	/* Main APB clock subsystem. */
	clock_control_subsys_t mclk_apb_sys;

	/* Main AHB clock subsystem. */
	clock_control_subsys_t mclk_ahb_sys;
} mchp_mdio_clock_t;

/**
 * @brief Run time data structure for the MDIO device.
 *
 * This structure contains the run time parameters for the MDIO device.
 */
struct mdio_mchp_dev_data {
	/* Semaphore to access registers */
	struct k_sem sem;
};

/**
 * @brief device configuration structure for the MDIO device.
 *
 * This structure contains the Device constant configuration parameters
 * for the MDIO device.
 */
typedef struct mdio_mchp_dev_config {
	/* Pin control structure */
	const struct pinctrl_dev_config *pcfg;

	/* GMAC register */
	gmac_registers_t *const regs;

	/* clock structure */
	mchp_mdio_clock_t mdio_clock;
} mdio_mchp_dev_config_t;

/* clang-format off */
#define MDIO_MCHP_CLOCK_DEFN(n)                                                             \
	.mdio_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                             \
	.mdio_clock.mclk_apb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb, subsystem), \
	.mdio_clock.mclk_ahb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb, subsystem)
/* clang-format on */

/**
 * @brief Register configuration structure
 *
 * This structure contains the configuration parameters for
 * reading/ writing onto the MDIO bus.
 */
typedef struct mdio_config_transfer {
	/* Operation - read/ write */
	enum mdio_opcode op;

	/* Data to be written */
	uint16_t data_in;

	/* BUffer for data to be read */
	uint16_t *data_out;

	/* Port Address */
	uint8_t prtad;

	/* Register Address */
	uint8_t regad;

	/* Using clause c34 or not */
	bool c45;
} mdio_config_transfer_t;

/**
 * @brief Read/ Write to MDIO bus
 *
 * This function is used to read/ write to MDIO bus.
 *
 * @param regs Pointer to gmac registers.
 * @param cfg Pointer to config related to the read/ write operation.
 *
 * @return 0 if successful else -ETIMEDOUT if the request timedout.
 */
static inline int mdio_transfer(gmac_registers_t *regs, mdio_config_transfer_t *cfg)
{
	int timeout = MDIO_MCHP_OP_TIMEOUT;
	int result = MDIO_MCHP_ESUCCESS;

	/* Evaluate the register value to be set */
	uint32_t reg_val = (cfg->c45 ? 0U : GMAC_MAN_CLTTO_Msk) | GMAC_MAN_OP(cfg->op) |
			   GMAC_MAN_WTN(0x02) | GMAC_MAN_PHYA(cfg->prtad) |
			   GMAC_MAN_REGA(cfg->regad) | GMAC_MAN_DATA(cfg->data_in);

	do {
		/* Set the value in the register */
		regs->GMAC_MAN = reg_val;

		/* Wait until done */
		while ((regs->GMAC_NSR & GMAC_NSR_IDLE_Msk) == 0) {
			if (timeout-- == 0U) {
				LOG_ERR("transfer timedout");

				result = -ETIMEDOUT;
				break;
			}

			k_sleep(K_MSEC(5));
		}

		/* Copy the value in case of read operation */
		if ((cfg->data_out) != NULL) {
			*(cfg->data_out) = regs->GMAC_MAN & GMAC_MAN_DATA_Msk;
		}
	} while (0);

	return result;
}

/**
 * @brief Read from register
 *
 * This function is used to read from MII register.
 *
 * @param dev Pointer to the MDIO device structure.
 * @param prtad Port Address.
 * @param regad Register Address.
 * @param data Pointer to the buffer to read the value.
 *
 * @return 0 if successful else -ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	int ret = MDIO_MCHP_ESUCCESS;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	mdio_config_transfer_t cfg_xfer;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	cfg_xfer.prtad = prtad;
	cfg_xfer.regad = regad;
	cfg_xfer.op = MDIO_OP_C22_READ;
	cfg_xfer.c45 = false;
	cfg_xfer.data_in = 0;
	cfg_xfer.data_out = data;

	/* Calling HAL API to read the data */
	ret = mdio_transfer(cfg->regs, &cfg_xfer);

	/* Release Semaphore */
	k_sem_give(&mdio_data->sem);
	return ret;
}

/**
 * @brief Write to register
 *
 * This function is used to write to MII register.
 *
 * @param dev Pointer to the MDIO device structure.
 * @param prtad Port Address.
 * @param regad Register Address.
 * @param data Buffer to write the value from.
 *
 * @return 0 if successful else -ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	int ret = MDIO_MCHP_ESUCCESS;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	mdio_config_transfer_t cfg_xfer;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	cfg_xfer.prtad = prtad;
	cfg_xfer.regad = regad;
	cfg_xfer.op = MDIO_OP_C22_WRITE;
	cfg_xfer.c45 = false;
	cfg_xfer.data_in = data;
	cfg_xfer.data_out = NULL;

	/* Calling HAL API to write the data */
	ret = mdio_transfer(cfg->regs, &cfg_xfer);

	/* Release Semaphore */
	k_sem_give(&mdio_data->sem);
	return ret;
}

/**
 * @brief Read from MDIO bus using clause 45 access
 *
 * This function is used to read from MDIO bus using clause 45 access.
 *
 * @param dev Pointer to the MDIO device structure.
 * @param prtad Port Address.
 * @param devad Device Address.
 * @param regad Register Address.
 * @param data Pointer to the buffer to read the value.
 *
 * @return 0 if successful else -ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t regad, uint16_t *data)
{
	int err = MDIO_MCHP_ESUCCESS;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	mdio_config_transfer_t cfg_xfer;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	cfg_xfer.prtad = prtad;
	cfg_xfer.regad = devad;
	cfg_xfer.op = MDIO_OP_C45_ADDRESS;
	cfg_xfer.c45 = true;
	cfg_xfer.data_in = regad;
	cfg_xfer.data_out = NULL;

	/* Calling HAL API to write address from which to read */
	err = mdio_transfer(cfg->regs, &cfg_xfer);
	if (err == MDIO_MCHP_ESUCCESS) {
		cfg_xfer.prtad = prtad;
		cfg_xfer.regad = devad;
		cfg_xfer.op = MDIO_OP_C45_READ;
		cfg_xfer.c45 = true;
		cfg_xfer.data_in = 0;
		cfg_xfer.data_out = data;

		/* Calling HAL API to read the data */
		err = mdio_transfer(cfg->regs, &cfg_xfer);
	}

	/* Release Semaphore */
	k_sem_give(&mdio_data->sem);
	return err;
}

/**
 * @brief Write to MDIO bus using clause 45 access
 *
 * This function is used to write to MDIO bus using clause 45 access.
 *
 * @param dev Pointer to the MDIO device structure.
 * @param prtad Port Address.
 * @param devad Device Address.
 * @param regad Register Address.
 * @param data Buffer to write the value to.
 *
 * @return 0 if successful else -ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t data)
{
	int err = MDIO_MCHP_ESUCCESS;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	mdio_config_transfer_t cfg_xfer;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	cfg_xfer.prtad = prtad;
	cfg_xfer.regad = devad;
	cfg_xfer.op = MDIO_OP_C45_ADDRESS;
	cfg_xfer.c45 = true;
	cfg_xfer.data_in = regad;
	cfg_xfer.data_out = NULL;

	/* Calling HAL API to write address from which to read */
	err = mdio_transfer(cfg->regs, &cfg_xfer);
	if (err == MDIO_MCHP_ESUCCESS) {
		cfg_xfer.prtad = prtad;
		cfg_xfer.regad = devad;
		cfg_xfer.op = MDIO_OP_C45_READ;
		cfg_xfer.c45 = true;
		cfg_xfer.data_in = data;
		cfg_xfer.data_out = NULL;

		/* Calling HAL API to write the data */
		err = mdio_transfer(cfg->regs, &cfg_xfer);
	}

	/* Release Semaphore */
	k_sem_give(&mdio_data->sem);
	return err;
}

/**
 * @brief Enable MDIO bus
 *
 * This function is used to enable MDIO bus.
 *
 * @param dev Pointer to the MDIO device structure.
 */
static void mdio_mchp_bus_enable(const struct device *dev)
{
	const struct mdio_mchp_dev_config *const cfg = dev->config;

	cfg->regs->GMAC_NCR |= GMAC_NCR_MPE_Msk;
}

/**
 * @brief Disable MDIO bus
 *
 * This function is used to disable MDIO bus.
 *
 * @param dev Pointer to the MDIO device structure.
 */
static void mdio_mchp_bus_disable(const struct device *dev)
{
	const struct mdio_mchp_dev_config *const cfg = dev->config;

	cfg->regs->GMAC_NCR &= ~GMAC_NCR_MPE_Msk;
}

/**
 * @brief MDIO Device Initialization
 *
 * This function is used to initialize the MDIO device, setting the
 * pin control state, and enabling the clock.
 *
 * @param dev Pointer to the Eth MAC device structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int mdio_mchp_initialize(const struct device *dev)
{
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	struct mdio_mchp_dev_data *const data = dev->data;
	int retval = MDIO_MCHP_ESUCCESS;

	do {
		/* Initialize the Semaphore */
		k_sem_init(&data->sem, 1, 1);

		/* Enable clocks */
		retval = clock_control_on(
			((const mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.clock_dev,
			(((mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.mclk_apb_sys));
		if ((retval != 0) && (retval != -EALREADY)) {
			LOG_ERR("Failed to enable the MCLK APB for Mdio: %d", retval);
			break;
		}

		retval = clock_control_on(
			((const mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.clock_dev,
			(((mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.mclk_ahb_sys));
		if ((retval != 0) && (retval != -EALREADY)) {
			LOG_ERR("Failed to enable the MCLK AHB for Mdio: %d", retval);
			break;
		}

		/* Connect pins to the peripheral */
		retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (retval != 0) {
			LOG_ERR("pinctrl_apply_state() Failed for Mdio driver: %d", retval);
			break;
		}
	} while (0);

	return retval;
}

/**
 * @brief MDIO device API structure for Microchip MDIO driver.
 *
 * This structure defines the function pointers for MDIO operations,
 * including reading/ writing to bus, and enabling/ disabling of the bus.
 */
static const struct mdio_driver_api mdio_mchp_driver_api = {
	/* read function */
	.read = mdio_mchp_read,

	/* write function */
	.write = mdio_mchp_write,

	/* read c45 function */
	.read_c45 = mdio_mchp_read_c45,

	/* write c45 function */
	.write_c45 = mdio_mchp_write_c45,

	/* enable bus function */
	.bus_enable = mdio_mchp_bus_enable,

	/* disable bus function */
	.bus_disable = mdio_mchp_bus_disable,
};

#define MDIO_MCHP_CONFIG(n)                                                                        \
	static const struct mdio_mchp_dev_config mdio_mchp_dev_config_##n = {                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.regs = (gmac_registers_t *)DT_INST_REG_ADDR(n),                                   \
		MDIO_MCHP_CLOCK_DEFN(n)};

#define MDIO_MCHP_DEVICE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	MDIO_MCHP_CONFIG(n);                                                                       \
	static struct mdio_mchp_dev_data mdio_mchp_dev_data##n;                                    \
	DEVICE_DT_INST_DEFINE(n, &mdio_mchp_initialize, NULL, &mdio_mchp_dev_data##n,              \
			      &mdio_mchp_dev_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_MCHP_DEVICE)

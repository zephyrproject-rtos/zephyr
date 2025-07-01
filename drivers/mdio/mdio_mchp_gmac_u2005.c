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
#define MDIO_MCHP_CLOCK_DEFN(n)                                                                \
	.mdio_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                \
	.mdio_clock.mclk_apb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_apb, subsystem),     \
	.mdio_clock.mclk_ahb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk_ahb, subsystem)

#define MDIO_MCHP_ENABLE_CLOCK(dev)                                                           \
	clock_control_on(((const mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.clock_dev,   \
			 (((mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.mclk_apb_sys));   \
	clock_control_on(((const mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.clock_dev,   \
			 (((mdio_mchp_dev_config_t *)(dev->config))->mdio_clock.mclk_ahb_sys))
/* clang-format on */

/**
 * @brief Register configuration structure
 *
 * This structure contains the configuration parameters for
 * reading/ writing onto the MDIO bus.
 */
typedef struct hal_mchp_mdio_config_transfer {
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
} hal_mchp_mdio_config_transfer_t;

/**
 * @brief Read/ Write to MDIO bus
 *
 * This function is used to read/ write to MDIO bus.
 *
 * @param regs Pointer to gmac registers.
 * @param cfg Pointer to config related to the read/ write operation.
 *
 * @return 0 if successful else ETIMEDOUT if the request timedout.
 */
static inline int hal_mchp_mdio_transfer(gmac_registers_t *regs,
					 hal_mchp_mdio_config_transfer_t *cfg)
{
	int timeout = 50;

	/* Evaluate the register value to be set */
	uint32_t reg_val = (cfg->c45 ? 0U : GMAC_MAN_CLTTO_Msk) | GMAC_MAN_OP(cfg->op) |
			   GMAC_MAN_WTN(0x02) | GMAC_MAN_PHYA(cfg->prtad) |
			   GMAC_MAN_REGA(cfg->regad) | GMAC_MAN_DATA(cfg->data_in);

	/* Set the value in the register */
	regs->GMAC_MAN = reg_val;

	/* Wait until done */
	while (!(regs->GMAC_NSR & GMAC_NSR_IDLE_Msk)) {
		if (timeout-- == 0U) {
			LOG_ERR("transfer timedout");

			return -ETIMEDOUT;
		}

		k_sleep(K_MSEC(5));
	}

	/* Copy the value in case of read operation */
	if (cfg->data_out) {
		*(cfg->data_out) = regs->GMAC_MAN & GMAC_MAN_DATA_Msk;
	}

	return 0;
}

/**
 * @brief Enable/ Disable MDIO bus
 *
 * This function is used to enable/ disable MDIO bus
 *
 * @param regs Pointer to gmac registers.
 * @param enable to specify if to enable or disable the MDIO bus.
 */
static inline void hal_mchp_mdio_bus_enable(gmac_registers_t *regs, bool enable)
{
	if (enable == true) {
		regs->GMAC_NCR |= GMAC_NCR_MPE_Msk;
	} else {
		regs->GMAC_NCR &= ~GMAC_NCR_MPE_Msk;
	}
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
 * @return 0 if successful else ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_read(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t *data)
{
	int ret = 0;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	hal_mchp_mdio_config_transfer_t hal_cfg;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	hal_cfg.prtad = prtad;
	hal_cfg.regad = regad;
	hal_cfg.op = MDIO_OP_C22_READ;
	hal_cfg.c45 = false;
	hal_cfg.data_in = 0;
	hal_cfg.data_out = data;

	/* Calling HAL API to read the data */
	ret = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);

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
 * @return 0 if successful else ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_write(const struct device *dev, uint8_t prtad, uint8_t regad, uint16_t data)
{
	int ret = 0;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	hal_mchp_mdio_config_transfer_t hal_cfg;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	hal_cfg.prtad = prtad;
	hal_cfg.regad = regad;
	hal_cfg.op = MDIO_OP_C22_WRITE;
	hal_cfg.c45 = false;
	hal_cfg.data_in = data;
	hal_cfg.data_out = NULL;

	/* Calling HAL API to write the data */
	ret = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);

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
 * @return 0 if successful else ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_read_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
			      uint16_t regad, uint16_t *data)
{
	int err;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	hal_mchp_mdio_config_transfer_t hal_cfg;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	hal_cfg.prtad = prtad;
	hal_cfg.regad = devad;
	hal_cfg.op = MDIO_OP_C45_ADDRESS;
	hal_cfg.c45 = true;
	hal_cfg.data_in = regad;
	hal_cfg.data_out = NULL;

	/* Calling HAL API to write address from which to read */
	err = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);
	if (!err) {
		hal_cfg.prtad = prtad;
		hal_cfg.regad = devad;
		hal_cfg.op = MDIO_OP_C45_READ;
		hal_cfg.c45 = true;
		hal_cfg.data_in = 0;
		hal_cfg.data_out = data;

		/* Calling HAL API to read the data */
		err = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);
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
 * @return 0 if successful else ETIMEDOUT if the request timedout.
 */
static int mdio_mchp_write_c45(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t data)
{
	int err;
	struct mdio_mchp_dev_data *const mdio_data = dev->data;
	const struct mdio_mchp_dev_config *const cfg = dev->config;
	hal_mchp_mdio_config_transfer_t hal_cfg;

	/* Take Semaphore */
	k_sem_take(&mdio_data->sem, K_FOREVER);

	hal_cfg.prtad = prtad;
	hal_cfg.regad = devad;
	hal_cfg.op = MDIO_OP_C45_ADDRESS;
	hal_cfg.c45 = true;
	hal_cfg.data_in = regad;
	hal_cfg.data_out = NULL;

	/* Calling HAL API to write address from which to read */
	err = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);
	if (!err) {
		hal_cfg.prtad = prtad;
		hal_cfg.regad = devad;
		hal_cfg.op = MDIO_OP_C45_READ;
		hal_cfg.c45 = true;
		hal_cfg.data_in = data;
		hal_cfg.data_out = NULL;

		/* Calling HAL API to write the data */
		err = hal_mchp_mdio_transfer(cfg->regs, &hal_cfg);
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

	/* Calling HAL API to enable mdio bus */
	hal_mchp_mdio_bus_enable(cfg->regs, true);
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

	/* Calling HAL API to disable mdio bus */
	hal_mchp_mdio_bus_enable(cfg->regs, false);
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
	int retval;

	/* Initialize the Semaphore */
	k_sem_init(&data->sem, 1, 1);

	/* Enable clocks */
	MDIO_MCHP_ENABLE_CLOCK(dev);

	/* Connect pins to the peripheral */
	retval = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

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
		HAL_MCHP_MDIO_CONFIG(n).pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                  \
		MDIO_MCHP_CLOCK_DEFN(n)};

#define MDIO_MCHP_DEVICE(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	MDIO_MCHP_CONFIG(n);                                                                       \
	static struct mdio_mchp_dev_data mdio_mchp_dev_data##n;                                    \
	DEVICE_DT_INST_DEFINE(n, &mdio_mchp_initialize, NULL, &mdio_mchp_dev_data##n,              \
			      &mdio_mchp_dev_config_##n, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_MCHP_DEVICE)

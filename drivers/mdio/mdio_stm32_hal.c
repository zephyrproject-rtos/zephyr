/*
 * Copyright (c) 2024 BayLibre, SAS
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdio_stm32_hal, CONFIG_MDIO_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_mdio

#define ADIN1100_REG_VALUE_MASK		GENMASK(15, 0)

struct mdio_stm32_data {
	struct k_sem sem;
	ETH_HandleTypeDef heth;
};

struct mdio_stm32_config {
	const struct pinctrl_dev_config *pincfg;
	struct stm32_pclken pclken;
};

static int mdio_stm32_read(const struct device *dev, uint8_t prtad,
			   uint8_t regad, uint16_t *data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	uint32_t read;
	int ret;

	k_sem_take(&dev_data->sem, K_FOREVER);

#ifdef CONFIG_ETH_STM32_HAL_API_V2
	ret = HAL_ETH_ReadPHYRegister(heth, prtad, regad, &read);
#else
	heth->Init.PhyAddress = prtad;

	ret = HAL_ETH_ReadPHYRegister(heth, regad, &read);
#endif

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	*data = read & ADIN1100_REG_VALUE_MASK;

	return ret;
}

static int mdio_stm32_write(const struct device *dev, uint8_t prtad,
			    uint8_t regad, uint16_t data)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int ret;

	k_sem_take(&dev_data->sem, K_FOREVER);

#ifdef CONFIG_ETH_STM32_HAL_API_V2
	ret = HAL_ETH_WritePHYRegister(heth, prtad, regad, data);
#else
	heth->Init.PhyAddress = prtad;

	ret = HAL_ETH_WritePHYRegister(heth, regad, data);
#endif

	k_sem_give(&dev_data->sem);

	if (ret != HAL_OK) {
		return -EIO;
	}

	return ret;
}

#ifdef CONFIG_ETH_STM32_HAL_API_V1
static void eth_set_mdio_clock_range_for_hal_v1(ETH_HandleTypeDef *heth)
{
	uint32_t tmpreg1 = 0U;

	/* Get the ETHERNET MACMIIAR value */
	tmpreg1 = (heth->Instance)->MACMIIAR;
	/* Clear CSR Clock Range CR[2:0] bits */
	tmpreg1 &= ETH_MACMIIAR_CR_MASK;

	/* Set CR bits depending on CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC value */
#ifdef SOC_SERIES_STM32F1X
	if ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= 20000000U) &&
	    (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < 35000000U)) {
		/* CSR Clock Range between 20-35 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_DIV16;
	} else if ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= 35000000U) &&
		   (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < 60000000U)) {
		/* CSR Clock Range between 35-60 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_DIV26;
	} else {
		/* CSR Clock Range between 60-72 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_DIV42;
	}
#else  /* SOC_SERIES_STM32F2X */
	if ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= 20000000U) &&
	    (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < 35000000U)) {
		/* CSR Clock Range between 20-35 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_Div16;
	} else if ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= 35000000U) &&
		   (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < 60000000U)) {
		/* CSR Clock Range between 35-60 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_Div26;
	} else if ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC >= 60000000U) &&
		   (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC < 100000000U)) {
		/* CSR Clock Range between 60-100 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_Div42;
	} else {
		/* CSR Clock Range between 100-120 MHz */
		tmpreg1 |= (uint32_t)ETH_MACMIIAR_CR_Div62;
	}
#endif /* SOC_SERIES_STM32F2X */

	/* Write to ETHERNET MAC MIIAR: Configure the ETHERNET CSR Clock Range */
	(heth->Instance)->MACMIIAR = (uint32_t)tmpreg1;
}
#endif /* CONFIG_ETH_STM32_HAL_API_V1 */

static int mdio_stm32_init(const struct device *dev)
{
	struct mdio_stm32_data *const dev_data = dev->data;
	const struct mdio_stm32_config *const config = dev->config;
	ETH_HandleTypeDef *heth = &dev_data->heth;
	int ret;

	/* enable clock */
	ret = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t)&config->pclken);
	if (ret < 0) {
		LOG_ERR("Failed to enable ethernet clock needed for MDIO (%d)", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_ETH_STM32_HAL_API_V2
	HAL_ETH_SetMDIOClockRange(heth);
#else
	/* The legacy V1 HAL API does not provide a way to set the MDC clock range
	 * via a separated function call. Implement an equivalent function ourselves
	 * based on what the V1 HAL performs in HAL_ETH_Init().
	 */
	eth_set_mdio_clock_range_for_hal_v1(heth);
#endif

	k_sem_init(&dev_data->sem, 1, 1);

	return 0;
}

static DEVICE_API(mdio, mdio_stm32_api) = {
	.read = mdio_stm32_read,
	.write = mdio_stm32_write,
};

#define MDIO_STM32_HAL_DEVICE(inst)                                                                \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct mdio_stm32_data mdio_stm32_data_##inst = {                                   \
		.heth = {.Instance = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst))},            \
	};                                                                                         \
	static struct mdio_stm32_config mdio_stm32_config_##inst = {                               \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.pclken = {.bus = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), stm_eth, bus),      \
			   .enr = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), stm_eth, bits)},    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mdio_stm32_init, NULL, &mdio_stm32_data_##inst,               \
			      &mdio_stm32_config_##inst, POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY,   \
			      &mdio_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(MDIO_STM32_HAL_DEVICE)

/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_rng.h>

#include "linklayer_plat.h"

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(802154_plat);

RAMCFG_HandleTypeDef hramcfg_SRAM1;
const struct device *rng_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

struct entropy_stm32_rng_dev_data {
	RNG_TypeDef *rng;
};

struct entropy_stm32_rng_dev_cfg {
	struct stm32_pclken *pclken;
};

void MX_RAMCFG_Init(void)
{
	/* Initialize RAMCFG SRAM1 */
	hramcfg_SRAM1.Instance = RAMCFG_SRAM1;
	if (HAL_RAMCFG_Init(&hramcfg_SRAM1) != HAL_OK) {
		LOG_ERR("Could not init RAMCFG");
	}
}

void Error_Handler(void)
{
	LOG_ERR("");
}

void enable_rng_clock(bool enable)
{
	const struct entropy_stm32_rng_dev_cfg *dev_cfg = rng_dev->config;
	struct entropy_stm32_rng_dev_data *dev_data = rng_dev->data;
	struct stm32_pclken *rng_pclken;
	const struct device *rcc;
	unsigned int key;

	rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	rng_pclken = (clock_control_subsys_t)&dev_cfg->pclken[0];

	key = irq_lock();

	/* Enable/Disable RNG clock only if not in use */
	if (!LL_RNG_IsEnabled((RNG_TypeDef *)dev_data->rng)) {
		if (enable) {
			clock_control_on(rcc, rng_pclken);
		} else {
			clock_control_off(rcc, rng_pclken);
		}
	}

	irq_unlock(key);
}

/* PKA IP requires RNG clock to be enabled
 * These APIs are used by BLE controller to enable/disable RNG clock,
 * based on PKA needs.
 */
void HW_RNG_DisableClock(uint8_t user_mask)
{
	ARG_UNUSED(user_mask);

	enable_rng_clock(false);
}

void HW_RNG_EnableClock(uint8_t user_mask)
{
	ARG_UNUSED(user_mask);

	enable_rng_clock(true);
}

/* BLE ctlr should not disable HSI on its own */
void SCM_HSI_CLK_OFF(void) {}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line)
{
  /* Prevent unused argument(s) compilation warning */
  (void)file;
  (void)line;

  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */
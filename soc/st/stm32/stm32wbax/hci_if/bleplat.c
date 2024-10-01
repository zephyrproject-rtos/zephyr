/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <stm32_ll_rng.h>

#include "bleplat.h"
#include "bpka.h"
#include "linklayer_plat.h"

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(ble_plat);

RAMCFG_HandleTypeDef hramcfg_SRAM1;
const struct device *rng_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

struct entropy_stm32_rng_dev_data {
	RNG_TypeDef *rng;
};

struct entropy_stm32_rng_dev_cfg {
	struct stm32_pclken *pclken;
};

void BLEPLAT_Init(void)
{
	BPKA_Reset();

	rng_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	if (!device_is_ready(rng_dev)) {
		LOG_ERR("error: random device not ready");
	}
}

void BLEPLAT_RngGet(uint8_t n, uint32_t *val)
{
	LINKLAYER_PLAT_GetRNG((uint8_t *)val, 4 * n);
}

int BLEPLAT_PkaStartP256Key(const uint32_t *local_private_key)
{
	return BPKA_StartP256Key(local_private_key);
}

void BLEPLAT_PkaReadP256Key(uint32_t *local_public_key)
{
	BPKA_ReadP256Key(local_public_key);
}

int BLEPLAT_PkaStartDhKey(const uint32_t *local_private_key,
			  const uint32_t *remote_public_key)
{
	return BPKA_StartDhKey(local_private_key, remote_public_key);
}

int BLEPLAT_PkaReadDhKey(uint32_t *dh_key)
{
	return BPKA_ReadDhKey(dh_key);
}

void BPKACB_Complete(void)
{
	BLEPLATCB_PkaComplete();
}

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

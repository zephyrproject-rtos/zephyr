/*
 * Copyright (C) 2021 Martin Schröder <info@swedishembedded.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <pm/pm.h>
#include <soc.h>
#include <init.h>

#include <stm32l0xx_ll_utils.h>
#include <stm32l0xx_ll_bus.h>
#include <stm32l0xx_ll_cortex.h>
#include <stm32l0xx_ll_pwr.h>
#include <stm32l0xx_ll_rcc.h>
#include <stm32l0xx_ll_gpio.h>
#include <stm32l0xx_ll_rtc.h>
#include <stm32l0xx_ll_system.h>
#include <clock_control/clock_stm32_ll_common.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* select MSI as wake-up system clock if configured, HSI otherwise */
#if STM32_SYSCLK_SRC_MSI
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_MSI
#else
#define RCC_STOP_WAKEUPCLOCK_SELECTED LL_RCC_STOP_WAKEUPCLOCK_HSI
#endif

struct gpio_backup {
	uint32_t MODER;
	uint32_t OTYPER;
	uint32_t OSPEEDR;
	uint32_t AFRL;
	uint32_t AFRH;
};

static GPIO_TypeDef *const _gpios[] = {
	GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOH
};
#define GPIO_CTRL_COUNT (sizeof(_gpios)/sizeof(_gpios[0]))

static struct gpio_backup _gpio[GPIO_CTRL_COUNT];

static void _gpio_save_state(){
	LL_GPIO_InitTypeDef cfg;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	for(unsigned c = 0; c < GPIO_CTRL_COUNT; c++){
		_gpio[c].MODER = _gpios[c]->MODER;
		_gpio[c].OTYPER = _gpios[c]->OTYPER;
		_gpio[c].OSPEEDR = _gpios[c]->OSPEEDR;
		_gpio[c].AFRL = _gpios[c]->AFR[0];
		_gpio[c].AFRH = _gpios[c]->AFR[1];
	}

	cfg.Pin = GPIO_PIN_All;
	cfg.Mode = GPIO_MODE_ANALOG;
	cfg.Pull = GPIO_NOPULL;

	LL_GPIO_Init(GPIOB, &cfg);
	LL_GPIO_Init(GPIOC, &cfg);
	LL_GPIO_Init(GPIOE, &cfg);


	//cfg.Pin = (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_9 | GPIO_PIN_10);
	//cfg.Mode = GPIO_MODE_ANALOG;
	//cfg.Pull = GPIO_NOPULL;
	LL_GPIO_Init(GPIOH, &cfg);

	// do not toggle PA0 since this is a wakeup pin!
	cfg.Pin = 0xfffffffe;
	LL_GPIO_Init(GPIOA, &cfg);

	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOE_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
}

static void _gpio_restore_state(){
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();

	for(unsigned c = 0; c < GPIO_CTRL_COUNT; c++){
		_gpios[c]->MODER = _gpio[c].MODER;
		_gpios[c]->OTYPER = _gpio[c].OTYPER;
		_gpios[c]->OSPEEDR = _gpio[c].OSPEEDR;
		_gpios[c]->AFR[0] = _gpio[c].AFRL;
		_gpios[c]->AFR[1] = _gpio[c].AFRH;
	}
}

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	switch(info.state){
		case PM_STATE_RUNTIME_IDLE:
			// simple idle mode
			LL_FLASH_DisableSleepPowerDown();
			LL_LPM_EnableSleep();
			break;
		case PM_STATE_SUSPEND_TO_IDLE:
		case PM_STATE_STANDBY:
			LL_RTC_ClearFlag_WUT(RTC);

			// power down as much as possible
			LL_PWR_EnableUltraLowPower();
			LL_PWR_EnableFastWakeUp();
			LL_FLASH_EnableSleepPowerDown();

			// voltage regulator in low power mode
			LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);

				/* ensure the proper wake-up system clock */
			LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);

			LL_LPM_EnableSleep();
			break;
		case PM_STATE_SUSPEND_TO_RAM:
			LL_RTC_ClearFlag_WUT(RTC);

			// power down as much as possible
			LL_PWR_EnableUltraLowPower();
			// don't wait for vref internal
			LL_PWR_EnableFastWakeUp();
			LL_FLASH_EnableSleepPowerDown();

			// voltage regulator in low power mode
			LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);

				/* ensure the proper wake-up system clock */
			LL_RCC_SetClkAfterWakeFromStop(RCC_STOP_WAKEUPCLOCK_SELECTED);

			LL_PWR_ClearFlag_WU();
			LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);
			LL_LPM_EnableDeepSleep();

			__disable_irq();
			_gpio_save_state();
			break;
		case PM_STATE_SOFT_OFF:
			LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
			LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);
			LL_PWR_ClearFlag_WU();
			// enter standby mode
			LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
			LL_LPM_EnableDeepSleep();
			break;
	}

	/* enter SLEEP mode : WFE or WFI */
	k_cpu_idle();
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	switch(info.state){
		case PM_STATE_RUNTIME_IDLE:
		case PM_STATE_SUSPEND_TO_IDLE:
		case PM_STATE_STANDBY:
			LL_PWR_DisableUltraLowPower();
			LL_LPM_DisableSleepOnExit();
			break;
		case PM_STATE_SUSPEND_TO_RAM:
			_gpio_restore_state();
			break;
		case PM_STATE_SOFT_OFF:
			// never get here, system reboots
			break;
	}

	/* need to restore the clock */
	stm32_clock_control_init(NULL);
	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}

/* Initialize STM32 Power */
static int stm32_power_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

#ifdef CONFIG_DEBUG
	/* Enable the Debug Module during STOP mode */
	//LL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGSleepMode();
	HAL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGStandbyMode();
#endif /* CONFIG_DEBUG */

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

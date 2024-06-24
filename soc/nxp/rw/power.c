/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#if CONFIG_GPIO && (DT_NODE_HAS_STATUS(DT_NODELABEL(pin0), okay) || \
		    DT_NODE_HAS_STATUS(DT_NODELABEL(pin1), okay))
#include <zephyr/drivers/gpio/gpio_mcux_lpc.h>
#endif

#include "fsl_power.h"

#include <zephyr/logging/log.h>
#include <zephyr/drivers/timer/system_timer.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Active mode */
#define POWER_MODE0		0
/* Idle mode */
#define POWER_MODE1		1
/* Standby mode */
#define POWER_MODE2		2
/* Sleep mode */
#define POWER_MODE3		3
/* Deep Sleep mode */
#define POWER_MODE4		4

#define NODE_ID DT_INST(0, nxp_pdcfg_power)

extern void clock_init(void);

power_sleep_config_t slp_cfg;

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin0)) || DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin1))
pinctrl_soc_pin_t pin_cfg;
#if CONFIG_GPIO
const struct device *gpio;
#endif
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin0))
static void pin0_isr(const struct device *dev)
{
	uint8_t level = ~(DT_ENUM_IDX(DT_NODELABEL(pin0), wakeup_level)) & 0x1;

	POWER_ConfigWakeupPin(kPOWER_WakeupPin0, level);
	NVIC_ClearPendingIRQ(DT_IRQN(DT_NODELABEL(pin0)));
	DisableIRQ(DT_IRQN(DT_NODELABEL(pin0)));
	POWER_DisableWakeup(DT_IRQN(DT_NODELABEL(pin0)));
}
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin1))
static void pin1_isr(const struct device *dev)
{
	uint8_t level = ~(DT_ENUM_IDX(DT_NODELABEL(pin1), wakeup_level)) & 0x1;

	POWER_ConfigWakeupPin(kPOWER_WakeupPin1, level);
	NVIC_ClearPendingIRQ(DT_IRQN(DT_NODELABEL(pin1)));
	DisableIRQ(DT_IRQN(DT_NODELABEL(pin1)));
	POWER_DisableWakeup(DT_IRQN(DT_NODELABEL(pin1)));
}
#endif

#ifdef CONFIG_MPU

#define MPU_NUM_REGIONS 8

typedef struct mpu_conf {
	uint32_t CTRL;
	uint32_t RNR;
	uint32_t RBAR[MPU_NUM_REGIONS];
	uint32_t RLAR[MPU_NUM_REGIONS];
	uint32_t RBAR_A1;
	uint32_t RLAR_A1;
	uint32_t RBAR_A2;
	uint32_t RLAR_A2;
	uint32_t RBAR_A3;
	uint32_t RLAR_A3;
	uint32_t MAIR0;
	uint32_t MAIR1;
} mpu_conf;
mpu_conf saved_mpu_conf;

static void save_mpu_state(void)
{
	uint32_t index;

	/*
	 * MPU is divided in `MPU_NUM_REGIONS` regions.
	 * Here we save those regions configuration to be restored after PM3 entry
	 */
	for (index = 0U; index < MPU_NUM_REGIONS; index++) {
		MPU->RNR = index;
		saved_mpu_conf.RBAR[index] = MPU->RBAR;
		saved_mpu_conf.RLAR[index] = MPU->RLAR;
	}

	saved_mpu_conf.CTRL = MPU->CTRL;
	saved_mpu_conf.RNR = MPU->RNR;
	saved_mpu_conf.RBAR_A1 = MPU->RBAR_A1;
	saved_mpu_conf.RLAR_A1 = MPU->RLAR_A1;
	saved_mpu_conf.RBAR_A2 = MPU->RBAR_A2;
	saved_mpu_conf.RLAR_A2 = MPU->RLAR_A2;
	saved_mpu_conf.RBAR_A3 = MPU->RBAR_A3;
	saved_mpu_conf.RLAR_A3 = MPU->RLAR_A3;
	saved_mpu_conf.MAIR0 = MPU->MAIR0;
	saved_mpu_conf.MAIR1 = MPU->MAIR1;
}

static void restore_mpu_state(void)
{
	uint32_t index;

	for (index = 0U; index < MPU_NUM_REGIONS; index++) {
		MPU->RNR = index;
		MPU->RBAR = saved_mpu_conf.RBAR[index];
		MPU->RLAR = saved_mpu_conf.RLAR[index];
	}

	MPU->RNR = saved_mpu_conf.RNR;
	MPU->RBAR_A1 = saved_mpu_conf.RBAR_A1;
	MPU->RLAR_A1 = saved_mpu_conf.RLAR_A1;
	MPU->RBAR_A2 = saved_mpu_conf.RBAR_A2;
	MPU->RLAR_A2 = saved_mpu_conf.RLAR_A2;
	MPU->RBAR_A3 = saved_mpu_conf.RBAR_A3;
	MPU->RLAR_A3 = saved_mpu_conf.RLAR_A3;
	MPU->MAIR0 = saved_mpu_conf.MAIR0;
	MPU->MAIR1 = saved_mpu_conf.MAIR1;

	/*
	 * CTRL register must be restored last in case MPU was enabled,
	 * because some MPU registers can't be programmed while the MPU is enabled
	 */
	MPU->CTRL = saved_mpu_conf.CTRL;
}

#endif /* CONFIG_MPU */

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin0))
	pin_cfg = IOMUX_GPIO_IDX(24) | IOMUX_TYPE(IOMUX_GPIO);
	pinctrl_configure_pins(&pin_cfg, 1, 0);
	POWER_ConfigWakeupPin(kPOWER_WakeupPin0, DT_ENUM_IDX(DT_NODELABEL(pin0), wakeup_level));
	POWER_ClearWakeupStatus(DT_IRQN(DT_NODELABEL(pin0)));
	NVIC_ClearPendingIRQ(DT_IRQN(DT_NODELABEL(pin0)));
	EnableIRQ(DT_IRQN(DT_NODELABEL(pin0)));
	POWER_EnableWakeup(DT_IRQN(DT_NODELABEL(pin0)));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin1))
	pin_cfg = IOMUX_GPIO_IDX(25) | IOMUX_TYPE(IOMUX_GPIO);
	pinctrl_configure_pins(&pin_cfg, 1, 0);
	POWER_ConfigWakeupPin(kPOWER_WakeupPin1, DT_ENUM_IDX(DT_NODELABEL(pin1), wakeup_level));
	POWER_ClearWakeupStatus(DT_IRQN(DT_NODELABEL(pin1)));
	NVIC_ClearPendingIRQ(DT_IRQN(DT_NODELABEL(pin1)));
	EnableIRQ(DT_IRQN(DT_NODELABEL(pin1)));
	POWER_EnableWakeup(DT_IRQN(DT_NODELABEL(pin1)));
#endif

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		POWER_SetSleepMode(POWER_MODE1);
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		POWER_EnterPowerMode(POWER_MODE2, &slp_cfg);
		break;
	case PM_STATE_STANDBY:
#ifdef CONFIG_MPU
		/* Save MPU state before entering PM3 */
		save_mpu_state();
#endif /* CONFIG_MPU */

		POWER_EnableWakeup(DT_IRQN(DT_NODELABEL(rtc)));
		if (POWER_EnterPowerMode(POWER_MODE3, &slp_cfg)) {
#ifdef CONFIG_MPU
			/* Restore MPU as is lost after PM3 exit*/
			restore_mpu_state();
#endif /* CONFIG_MPU */
			clock_init();

			sys_clock_idle_exit();
		}

		/* Clear the RTC wakeup bits */
		POWER_ClearWakeupStatus(DT_IRQN(DT_NODELABEL(rtc)));
		POWER_DisableWakeup(DT_IRQN(DT_NODELABEL(rtc)));

		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

#if CONFIG_GPIO && (DT_NODE_HAS_STATUS(DT_NODELABEL(pin0), okay) || \
		    DT_NODE_HAS_STATUS(DT_NODELABEL(pin1), okay))
	if (state == PM_STATE_STANDBY) {
		/* GPIO_0_24 & GPIO_0_25 are used for wakeup */
		uint32_t pins = PMU->WAKEUP_STATUS &
				(PMU_WAKEUP_STATUS_PIN0_MASK | PMU_WAKEUP_STATUS_PIN1_MASK);
		gpio_mcux_lpc_trigger_cb(gpio, (pins << 24));
	}
#endif
	/* Clear PRIMASK */
	__enable_irq();
}

void nxp_rw6xx_power_init(void)
{
	uint32_t suspend_sleepconfig[5] = DT_PROP_OR(NODE_ID, deep_sleep_config, {});

	slp_cfg.pm2MemPuCfg = suspend_sleepconfig[0];
	slp_cfg.pm2AnaPuCfg = suspend_sleepconfig[1];
	slp_cfg.clkGate = suspend_sleepconfig[2];
	slp_cfg.memPdCfg = suspend_sleepconfig[3];
	slp_cfg.pm3BuckCfg = suspend_sleepconfig[4];

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin0))
	/* PIN 0 uses GPIO0_24, confiure the pin as GPIO */
	pin_cfg = IOMUX_GPIO_IDX(24) | IOMUX_TYPE(IOMUX_GPIO);
	pinctrl_configure_pins(&pin_cfg, 1, 0);

	/* Initialize the settings in the PMU for this wakeup interrupt */
	pin0_isr(NULL);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(pin0)), DT_IRQ(DT_NODELABEL(pin0), priority), pin0_isr,
		    NULL, 0);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pin1))
	/* PIN 1 uses GPIO0_25, confiure the pin as GPIO */
	pin_cfg = IOMUX_GPIO_IDX(25) | IOMUX_TYPE(IOMUX_GPIO);
	pinctrl_configure_pins(&pin_cfg, 1, 0);

	/* Initialize the settings in the PMU for this wakeup interrupt */
	pin1_isr(NULL);

	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(pin1)), DT_IRQ(DT_NODELABEL(pin1), priority), pin1_isr,
		    NULL, 0);
#endif

	/* Clear the RTC wakeup bits */
	POWER_ClearWakeupStatus(DT_IRQN(DT_NODELABEL(rtc)));
	POWER_DisableWakeup(DT_IRQN(DT_NODELABEL(rtc)));

#if CONFIG_GPIO && (DT_NODE_HAS_STATUS(DT_NODELABEL(pin0), okay) || \
		    DT_NODE_HAS_STATUS(DT_NODELABEL(pin1), okay))
	gpio = DEVICE_DT_GET(DT_NODELABEL(hsgpio0));
	if (!device_is_ready(gpio)) {
		return;
	}
#endif
}

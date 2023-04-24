/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief System module to configure the power system WakeUp pins
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/types.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <stm32_ll_system.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/dt-bindings/power/stm32_power.h>

#define DT_DRV_COMPAT st_stm32_pwr

#define PWR_STM32_WAKEUP DT_NODELABEL(pwr_wkup)

/* Give one more entry to ease the indexing */
#define PWR_STM32_MAX_NB_WAKEUP_PINS	DT_INST_PROP(0, pin_nb)

#define PWR_STM32_WAKEUP_PINS_POL	DT_INST_PROP(0, pin_pol)

#define PWR_STM32_WAKEUP_PINS_PUPD	DT_INST_PROP(0, pin_pupd)

/* Configure the polarity is possible of not */
#define PWR_STM32_WAKEUP_PINS_TYPE0(i, _)	\
	DT_NODE_HAS_COMPAT(DT_PHANDLE_BY_IDX(PWR_STM32_WAKEUP, wkpins, i), st_stm32_wkup_pin0)

#define PWR_STM32_WAKEUP_PINS_TYPE1(i, _)	\
	DT_NODE_HAS_COMPAT(DT_PHANDLE_BY_IDX(PWR_STM32_WAKEUP, wkpins, i), st_stm32_wkup_pin1)

#define PWR_STM32_WAKEUP_PINS_TYPE2(i, _)	\
	DT_NODE_HAS_COMPAT(DT_PHANDLE_BY_IDX(PWR_STM32_WAKEUP, wkpins, i), st_stm32_wkup_pin2)

#define PWR_STM32_WAKEUP_PIN_DEF(i, _)	LL_PWR_WAKEUP_PIN##i
#define PWR_STM32_WAKEUP_PIN(i)		LL_PWR_WAKEUP_PIN##i

/*
 * LookUp Table to store the LL_PWR_WAKEUP_PINx for each pin.
 * Add one more to the list to have table_wakeup_pins[5] =  LL_PWR_WAKEUP_PIN5
 */
static const uint32_t table_wakeup_pins[PWR_STM32_MAX_NB_WAKEUP_PINS + 1] = {
		LISTIFY(PWR_STM32_MAX_NB_WAKEUP_PINS, PWR_STM32_WAKEUP_PIN_DEF, (,)),
		FOR_EACH(PWR_STM32_WAKEUP_PIN, (,), PWR_STM32_MAX_NB_WAKEUP_PINS)
	};

/* Nb of wakeup pin used for this &pwr_wkup node */
#define PWR_STM32_WAKEUP_PINS	DT_PROP_LEN(PWR_STM32_WAKEUP, wkpins)

#define PWR_STM32_WAKEUP_POS(i, _)	\
	DT_PROP_BY_PHANDLE_IDX(PWR_STM32_WAKEUP, wkpins, i, syswakeup_pin)

#define PWR_STM32_WAKEUP_SEL(i, _)							\
	COND_CODE_1(CONFIG_SOC_SERIES_STM32U5X,						\
		(.selection = DT_PROP_BY_PHANDLE_IDX(PWR_STM32_WAKEUP, wkpins, i,	\
							syswakeup_sel),),		\
		())

#define PWR_STM32_WAKEUP_POL(i, _)							\
	COND_CODE_1(PWR_STM32_WAKEUP_PINS_POL,						\
		(.polarity = DT_PHA_BY_IDX(PWR_STM32_WAKEUP, wkpins, i, polarity),),	\
		())

#define PWR_STM32_WAKEUP_PUPD(i, _)							\
	COND_CODE_1(PWR_STM32_WAKEUP_PINS_PUPD,						\
		(.pupd =  DT_PHA_BY_IDX(PWR_STM32_WAKEUP, wkpins, i, pupd),),		\
		())

/* Only one node_id = DT_NODELABEL(pwr_wkup) */
#define PWR_STM32_WAKEUP_PIN_ENTRY(node_id, prop, index)	\
	{							\
		.position = PWR_STM32_WAKEUP_POS(index, _),	\
		PWR_STM32_WAKEUP_SEL(index, _)			\
		PWR_STM32_WAKEUP_POL(index, _)			\
		PWR_STM32_WAKEUP_PUPD(index, _)			\
	},

/* Structure of each system WakeUp Pin */
struct pwr_stm32_wakeup_pin {
	uint8_t position;	/* The <syswakeup_pin> property of the phandle */
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	uint8_t selection;	/* The <syswakeup_sel> property of the phandle */
#endif /* CONFIG_SOC_SERIES_STM32U5X */
#if PWR_STM32_WAKEUP_PINS_POL
	bool polarity;		/* True if detection on the low level : FALLING edge */
#endif /* PWR_STM32_WAKEUP_PINS_POL */
#if PWR_STM32_WAKEUP_PINS_PUPD
	uint8_t pupd;		/* The pull-up/pull-down of the phandle */
#endif /* PWR_STM32_WAKEUP_PINS_PUPD */
};

/* Structure of the pwr driver configuration */
struct pwr_stm32_cfg {
	struct stm32_pclken pclken;
	PWR_TypeDef *Instance;
	const struct pwr_stm32_wakeup_pin *wakeup_pins;
};

static const struct pwr_stm32_wakeup_pin wakeup_pins_list[] = {
	DT_FOREACH_PROP_ELEM(PWR_STM32_WAKEUP, wkpins, PWR_STM32_WAKEUP_PIN_ENTRY)
};

/* One instanciation of the pwr device */
static const struct pwr_stm32_cfg pwr_stm32_dev_cfg = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus),
	},
	.Instance = (PWR_TypeDef *)DT_INST_REG_ADDR(0),
	.wakeup_pins = wakeup_pins_list,
};

/**
 * @brief Perform SoC configuration at boot.
 *
 * This should be run early during the boot process but after basic hardware
 * initialization is done.
 *
 * @return 0
 */
static int stm32_pwr_init(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	uint32_t table_index;

	if (!device_is_ready(clk)) {
		/* clock control device not ready */
		return -ENODEV;
	}

	/* enable Power clock */
	if (clock_control_on(clk,
		(clock_control_subsys_t) &pwr_stm32_dev_cfg.pclken) != 0) {
		return -EIO;
	}

	/* Get the nb of len_syswkup_pins in the phandle-array */
	uint32_t len_syswkup_pins = PWR_STM32_WAKEUP_PINS;

	if (len_syswkup_pins > PWR_STM32_MAX_NB_WAKEUP_PINS) {
		/*% selected pins exceeds PWR_STM32_MAX_NB_WAKEUP_PINS */
		return -ENODEV;
	}

	/* Get each element at index in the phandle array  */
	for (int index = 0; index < len_syswkup_pins; index++) {
		/*
		 * Get the syswakeup-pin property of this phandle index,
		 * this is the entry in the table_wakeup_pins
		 */
		table_index = pwr_stm32_dev_cfg.wakeup_pins[index].position;

		LL_PWR_EnableWakeUpPin(table_wakeup_pins[table_index]);
#if PWR_STM32_WAKEUP_PINS_POL
		/* Set the polarity at this index */
		if (pwr_stm32_dev_cfg.wakeup_pins[index].polarity == STM32_PWR_WAKEUP_RISING) {
			LL_PWR_SetWakeUpPinPolarityHigh(table_wakeup_pins[table_index]);
		} else {
			LL_PWR_SetWakeUpPinPolarityLow(table_wakeup_pins[table_index]);
		}
#endif /* PWR_STM32_WAKEUP_PINS_POL */

#if defined(CONFIG_SOC_SERIES_STM32U5X)
		/* Retrieve the multiplexed signal selection from the phandle and apply */
		if (pwr_stm32_dev_cfg.wakeup_pins[index].selection == 0) {
			LL_PWR_SetWakeUpPinSignal0Selection(table_wakeup_pins[table_index]);
		} else if (pwr_stm32_dev_cfg.wakeup_pins[index].selection == 1) {
			LL_PWR_SetWakeUpPinSignal1Selection(table_wakeup_pins[table_index]);
		} else if (pwr_stm32_dev_cfg.wakeup_pins[index].selection == 2) {
			LL_PWR_SetWakeUpPinSignal2Selection(table_wakeup_pins[table_index]);
		} else {
			LL_PWR_SetWakeUpPinSignal3Selection(table_wakeup_pins[table_index]);
		}
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#if PWR_STM32_WAKEUP_PINS_PUPD
		/* Set the pull-up/down at this index */
		if (pwr_stm32_dev_cfg.wakeup_pins[index].pupd == STM32_PWR_WAKEUP_PULLUP) {
			LL_PWR_SetWakeUpPinPullUp(table_wakeup_pins[table_index]);
		} else if (pwr_stm32_dev_cfg.wakeup_pins[index].pupd == STM32_PWR_WAKEUP_PULLDOWN) {
			LL_PWR_SetWakeUpPinPullDown(table_wakeup_pins[table_index]);
		} else {
			LL_PWR_SetWakeUpPinPullNone(table_wakeup_pins[table_index]);
		}
#endif /* PWR_STM32_WAKEUP_PINS_PUPD */
	}

	return 0;
}

/* Only one instance of st_stm32_pwr */

SYS_INIT(stm32_pwr_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/*
 * Copyright (c) 2021 Florin Stancu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Implements the RF driver callback to configure the on-board antenna
 * switch.
 */

#define DT_DRV_COMPAT skyworks_sky13317

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

#include <ti/drivers/rf/RF.h>
#include <driverlib/rom.h>
#include <driverlib/interrupt.h>

/* custom pinctrl states for the antenna mux */
#define PINCTRL_STATE_ANT_24G		1
#define PINCTRL_STATE_ANT_24G_PA	2
#define PINCTRL_STATE_ANT_SUBG		3
#define PINCTRL_STATE_ANT_SUBG_PA	4

#define BOARD_ANT_GPIO_24G   0
#define BOARD_ANT_GPIO_PA    1
#define BOARD_ANT_GPIO_SUBG  2

#define ANTENNA_MUX DT_NODELABEL(antenna_mux0)

static int board_antenna_init(const struct device *dev);
static void board_cc13xx_rf_callback(RF_Handle client, RF_GlobalEvent events,
		void *arg);

const RFCC26XX_HWAttrsV2 RFCC26XX_hwAttrs = {
	.hwiPriority        = INT_PRI_LEVEL7,
	.swiPriority        = 0,
	.xoscHfAlwaysNeeded = true,
	/* RF driver callback for custom antenna switching */
	.globalCallback = board_cc13xx_rf_callback,
	/* Subscribe to events */
	.globalEventMask = (RF_GlobalEventRadioSetup |
			RF_GlobalEventRadioPowerDown),
};

PINCTRL_DT_INST_DEFINE(0);
DEVICE_DT_INST_DEFINE(0, board_antenna_init, NULL, NULL, NULL,
					  POST_KERNEL, CONFIG_BOARD_ANTENNA_INIT_PRIO, NULL);

static const struct pinctrl_dev_config *ant_pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0);
static const struct gpio_dt_spec ant_gpios[] = {
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_MUX, gpios, BOARD_ANT_GPIO_24G, {0}),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_MUX, gpios, BOARD_ANT_GPIO_PA, {0}),
	GPIO_DT_SPEC_GET_BY_IDX_OR(ANTENNA_MUX, gpios, BOARD_ANT_GPIO_SUBG, {0}),
};


/**
 * Antenna switch GPIO init routine.
 */
int board_antenna_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int i;

	/* default pinctrl configuration: set all antenna mux control pins as GPIOs */
	pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_DEFAULT);
	/* set all GPIOs to 0 (all RF paths disabled) */
	for (i = 0; i < ARRAY_SIZE(ant_gpios); i++) {
		gpio_pin_configure_dt(&ant_gpios[i], 0);
	}
	return 0;
}

/**
 * Custom TI RFCC26XX callback for switching the on-board antenna mux on radio setup.
 */
void board_cc13xx_rf_callback(RF_Handle client, RF_GlobalEvent events, void *arg)
{
	bool    sub1GHz   = false;
	uint8_t loDivider = 0;
	int i;

	/* Clear all antenna switch GPIOs (for all cases). */
	for (i = 0; i < ARRAY_SIZE(ant_gpios); i++) {
		gpio_pin_configure_dt(&ant_gpios[i], 0);
	}

	if (events & RF_GlobalEventRadioSetup) {
		/* Decode the current PA configuration. */
		RF_TxPowerTable_PAType paType = (RF_TxPowerTable_PAType)
			RF_getTxPower(client).paType;
		/* Decode the generic argument as a setup command. */
		RF_RadioSetup *setupCommand = (RF_RadioSetup *)arg;

		switch (setupCommand->common.commandNo) {
		case CMD_RADIO_SETUP:
		case CMD_BLE5_RADIO_SETUP:
			loDivider = RF_LODIVIDER_MASK & setupCommand->common.loDivider;
			break;
		case CMD_PROP_RADIO_DIV_SETUP:
			loDivider = RF_LODIVIDER_MASK & setupCommand->prop_div.loDivider;
			break;
		default:
			break;
		}
		sub1GHz = (loDivider != 0);

		if (sub1GHz) {
			if (paType == RF_TxPowerTable_HighPA) {
				/* Note: RFC_GPO3 is a work-around because the RFC_GPO1 */
				/* is sometimes not de-asserted on CC1352 Rev A. */
				pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_ANT_SUBG_PA);
			} else {
				pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_ANT_SUBG);
				/* Manually set the sub-GHZ antenna switch DIO */
				gpio_pin_configure_dt(&ant_gpios[BOARD_ANT_GPIO_SUBG], 1);
			}
		} else /* 2.4 GHz */ {
			if (paType == RF_TxPowerTable_HighPA) {
				pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_ANT_24G_PA);
			} else {
				pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_ANT_24G);
				/* Manually set the 2.4GHZ antenna switch DIO */
				gpio_pin_configure_dt(&ant_gpios[BOARD_ANT_GPIO_24G], 1);
			}
		}
	} else {
		pinctrl_apply_state(ant_pcfg, PINCTRL_STATE_DEFAULT);
	}
}

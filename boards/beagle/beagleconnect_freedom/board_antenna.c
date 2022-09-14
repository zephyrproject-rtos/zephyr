/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Florin Stancu
 * Copyright (c) 2021 Jason Kridner, BeagleBoard.org Foundation
 *
 */

/*
 * Implements the RF driver callback to configure the on-board antenna
 * switch.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <ti/drivers/rf/RF.h>
#include <driverlib/gpio.h>
#include <driverlib/ioc.h>
#include <driverlib/rom.h>

/* DIOs for RF antenna paths */
#define BOARD_RF_HIGH_PA   29		/* TODO: pull from DT */
#define BOARD_RF_SUB1GHZ   30		/* TODO: pull from DT */

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

/**
 * Antenna switch GPIO init routine.
 */
static int board_antenna_init(void)
{

	/* set all paths to low */
	IOCPinTypeGpioOutput(BOARD_RF_HIGH_PA);
	GPIO_setOutputEnableDio(BOARD_RF_HIGH_PA, GPIO_OUTPUT_DISABLE);
	IOCPinTypeGpioOutput(BOARD_RF_SUB1GHZ);
	GPIO_setOutputEnableDio(BOARD_RF_SUB1GHZ, GPIO_OUTPUT_DISABLE);
	return 0;
}

SYS_INIT(board_antenna_init, POST_KERNEL, CONFIG_BOARD_ANTENNA_INIT_PRIO);

void board_cc13xx_rf_callback(RF_Handle client, RF_GlobalEvent events, void *arg)
{
	bool    sub1GHz   = false;
	uint8_t loDivider = 0;

	/* Switch off all paths first. Needs to be done anyway in every sub-case below. */
	GPIO_setOutputEnableDio(BOARD_RF_HIGH_PA, GPIO_OUTPUT_DISABLE);
	GPIO_setOutputEnableDio(BOARD_RF_SUB1GHZ, GPIO_OUTPUT_DISABLE);

	if (events & RF_GlobalEventRadioSetup) {
		/* Decode the current PA configuration. */
		RF_TxPowerTable_PAType paType = (RF_TxPowerTable_PAType)
			RF_getTxPower(client).paType;
		/* Decode the generic argument as a setup command. */
		RF_RadioSetup *setupCommand = (RF_RadioSetup *)arg;

		switch (setupCommand->common.commandNo) {
		case (CMD_RADIO_SETUP):
		case (CMD_BLE5_RADIO_SETUP):
			loDivider = RF_LODIVIDER_MASK & setupCommand->common.loDivider;
			/* Sub-1GHz front-end. */
			if (loDivider != 0)
				sub1GHz = true;
			break;
		case (CMD_PROP_RADIO_DIV_SETUP):
			loDivider = RF_LODIVIDER_MASK & setupCommand->prop_div.loDivider;
			/* Sub-1GHz front-end. */
			if (loDivider != 0)
				sub1GHz = true;
			break;
		default:
			break;
		}

		/* Sub-1 GHz */
		if (paType == RF_TxPowerTable_HighPA) {
			/* PA enable --> HIGH PA */
			/* LNA enable --> Sub-1 GHz */
			/* Note: RFC_GPO3 is a work-around because the RFC_GPO1 */
			/* is sometimes not de-asserted on CC1352 Rev A. */
			IOCPortConfigureSet(BOARD_RF_HIGH_PA,
					IOC_PORT_RFC_GPO3, IOC_IOMODE_NORMAL);
			IOCPortConfigureSet(BOARD_RF_SUB1GHZ,
					IOC_PORT_RFC_GPO0, IOC_IOMODE_NORMAL);
		} else {
			/* RF core active --> Sub-1 GHz */
			IOCPortConfigureSet(BOARD_RF_HIGH_PA,
					IOC_PORT_GPIO, IOC_IOMODE_NORMAL);
			IOCPortConfigureSet(BOARD_RF_SUB1GHZ,
					IOC_PORT_GPIO, IOC_IOMODE_NORMAL);
			GPIO_setOutputEnableDio(BOARD_RF_SUB1GHZ, GPIO_OUTPUT_ENABLE);
		}
	} else {
		/* Reset the IO multiplexer to GPIO functionality */
		IOCPortConfigureSet(BOARD_RF_HIGH_PA,
				IOC_PORT_GPIO, IOC_IOMODE_NORMAL);
		IOCPortConfigureSet(BOARD_RF_SUB1GHZ,
				IOC_PORT_GPIO, IOC_IOMODE_NORMAL);
	}
}

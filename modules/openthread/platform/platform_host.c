/*
 * Copyright (c) 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */

#include <stdlib.h>

#include <openthread/instance.h>
#include <openthread/tasklet.h>

#include "platform-zephyr.h"
#include "platform.h"

/* -------------------------------------------------------------------------- */
/*                               Private macros                               */
/* -------------------------------------------------------------------------- */
#define DT_DRV_COMPAT nxp_hci_ble
#define HCI_IRQ_N        DT_INST_IRQ_BY_IDX(0, 0, irq)
#define HCI_IRQ_P        DT_INST_IRQ_BY_IDX(0, 0, priority)
#define HCI_WAKEUP_IRQ_N DT_INST_IRQ_BY_IDX(0, 1, irq)
#define HCI_WAKEUP_IRQ_P DT_INST_IRQ_BY_IDX(0, 1, priority)

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */
extern int32_t ble_hci_handler(void);
extern int32_t ble_wakeup_done_handler(void);

void otSysInit(int argc, char *argv[])
{
	/* Temporary Interrupt source code for the RC3 delivery */
	/* Shall be move in the IMU driver */

	/* HCI Interrupt */
	IRQ_CONNECT(HCI_IRQ_N, HCI_IRQ_P, ble_hci_handler, 0, 0);
	irq_enable(HCI_IRQ_N);

	/* Wake up done interrupt */
	IRQ_CONNECT(HCI_WAKEUP_IRQ_N, HCI_WAKEUP_IRQ_P, ble_wakeup_done_handler, 0, 0);
	irq_enable(HCI_WAKEUP_IRQ_N);
	
	/* End of Temporary Interrupt source code */

	otPlatRadioInit();
	platformAlarmInit();
}

void otSysDeinit(void)
{
	otPlatRadioDeinit();
}

void otSysProcessDrivers(otInstance *aInstance)
{
	otPlatRadioProcess(aInstance);
	platformAlarmProcess(aInstance);
}

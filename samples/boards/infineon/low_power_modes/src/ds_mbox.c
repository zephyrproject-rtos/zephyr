/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <cy_ipc_drv.h>

#include "ds_mbox.h"

/* SRF owns MTB_IPC_CHAN_0 (CY_IPC_CHAN_USER) and interrupt structs
 * CY_IPC_INTR_USER..+3, so the DeepSleep mailbox uses the next free user
 * channel and interrupt structure.  Both cores agree on these numbers.
 */
#define DS_MBOX_IPC_CHAN (CY_IPC_CHAN_USER + 1U)
#define DS_MBOX_IPC_INTR (CY_IPC_INTR_USER + 4U)

#if defined(CONFIG_APP_COMPANION_MBOX)

#define DS_MBOX_NOTIFY_MASK  CY_IPC_INTR_MASK(DS_MBOX_IPC_INTR)
#define DS_MBOX_SEND_RETRIES 10U

void ds_mbox_send(uint32_t mode)
{
	IPC_STRUCT_Type *ipc = Cy_IPC_Drv_GetIpcBaseAddress(DS_MBOX_IPC_CHAN);
	cy_en_ipcdrv_status_t st = CY_IPC_DRV_ERROR;
	uint32_t retries = DS_MBOX_SEND_RETRIES;

	/* SendMsgWord acquires the channel lock, writes the command word and
	 * raises the CM33-NS notify interrupt.  If the CM33 has not yet released
	 * a previous message the channel is still locked, so retry briefly.
	 */
	do {
		st = Cy_IPC_Drv_SendMsgWord(ipc, DS_MBOX_NOTIFY_MASK, mode);
		if (st == CY_IPC_DRV_SUCCESS) {
			break;
		}
		k_busy_wait(1000);
	} while (--retries > 0U);

	if (st == CY_IPC_DRV_SUCCESS) {
		printk("DS mbox: sent mode=%u to CM33-NS\n", (unsigned int)mode);
	} else {
		printk("DS mbox: send failed (%d)\n", (int)st);
	}
}

#endif /* CONFIG_APP_COMPANION_MBOX */

#if defined(CONFIG_APP_ROLE_COMPANION)

#define DS_MBOX_CH_MASK CY_IPC_CH_MASK(DS_MBOX_IPC_CHAN)

/* Latch a DeepSleep request from the mailbox ISR so the blink loop enters the
 * requested low-power mode at its next wait point.  The SW0 wake source is armed
 * by the CM55 controller, so this core only reacts to the mailbox command.
 */
static K_SEM_DEFINE(ds_off_req_sem, 0, 1);

/* Low-power mode requested by the last mailbox message; defaults to DS-RAM so
 * an SW0 press before any CM55 command still uses the previous behavior.
 */
static volatile uint32_t ds_mbox_mode = DS_MBOX_CMD_DS_RAM;

static void ds_mbox_isr(const void *arg)
{
	IPC_INTR_STRUCT_Type *intr = Cy_IPC_Drv_GetIntrBaseAddr(DS_MBOX_IPC_INTR);
	IPC_STRUCT_Type *ipc = Cy_IPC_Drv_GetIpcBaseAddress(DS_MBOX_IPC_CHAN);
	uint32_t msg = 0U;

	ARG_UNUSED(arg);

	/* Read the command word the CM55 placed in the channel, latch the
	 * requested mode and wake the blink loop, then release the channel lock
	 * so the sender can reuse it and clear our notify interrupt.
	 */
	if (Cy_IPC_Drv_ReadMsgWord(ipc, &msg) == CY_IPC_DRV_SUCCESS) {
		ds_mbox_mode = msg;
		k_sem_give(&ds_off_req_sem);
	}

	Cy_IPC_Drv_ReleaseNotify(ipc, 0U);
	Cy_IPC_Drv_ClearInterrupt(intr, 0U, DS_MBOX_CH_MASK);
}

void ds_mbox_receiver_init(void)
{
	IPC_INTR_STRUCT_Type *intr = Cy_IPC_Drv_GetIntrBaseAddr(DS_MBOX_IPC_INTR);

	/* Route a notify event on our channel to our interrupt structure. */
	Cy_IPC_Drv_SetInterruptMask(intr, 0U, DS_MBOX_CH_MASK);

	IRQ_CONNECT(CY_IPC_INTR_MUX(DS_MBOX_IPC_INTR), 3, ds_mbox_isr, NULL, 0);
	irq_enable(CY_IPC_INTR_MUX(DS_MBOX_IPC_INTR));
}

int ds_mbox_wait(uint32_t *mode, k_timeout_t timeout)
{
	if (k_sem_take(&ds_off_req_sem, timeout) != 0) {
		return -EAGAIN;
	}

	if (mode != NULL) {
		*mode = ds_mbox_mode;
	}

	return 0;
}

#endif /* CONFIG_APP_ROLE_COMPANION */

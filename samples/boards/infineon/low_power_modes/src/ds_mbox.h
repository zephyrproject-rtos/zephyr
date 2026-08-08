/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DS_MBOX_H_
#define DS_MBOX_H_

#include <stdint.h>
#include <zephyr/kernel.h>

/*
 * Inter-core DeepSleep command mailbox (CM55 controller -> CM33-NS companion).
 *
 * A raw Cy_IPC_Drv channel lets the CM55 application command which system
 * low-power mode the CM33-NS core should enter.  SRF owns MTB_IPC_CHAN_0
 * (CY_IPC_CHAN_USER) and interrupt structs CY_IPC_INTR_USER..+3, so the mailbox
 * uses the next free user channel and interrupt structure.  These reservations
 * match soc/infineon/edge/pse84/mtb_ipc_config.h (MTB_IPC_CHANNEL_DS_MBOX /
 * MTB_IPC_IRQ_DS_MBOX).  Both cores include this header and link ds_mbox.c.
 */

/* Command word exchanged over the mailbox channel data register. */
#define DS_MBOX_CMD_DS     0U
#define DS_MBOX_CMD_DS_RAM 1U
#define DS_MBOX_CMD_DS_OFF 2U

#if defined(CONFIG_APP_COMPANION_MBOX)
/* DUT side: post a DeepSleep command word to the companion receiver. */
void ds_mbox_send(uint32_t mode);
#endif /* CONFIG_APP_COMPANION_MBOX */

#if defined(CONFIG_APP_ROLE_COMPANION)
/* Companion side: bring up the mailbox receiver (interrupt + channel mask). */
void ds_mbox_receiver_init(void);

/* Wait up to timeout for a mailbox command.  On success stores the requested
 * command word in *mode and returns 0; returns -EAGAIN on timeout.
 */
int ds_mbox_wait(uint32_t *mode, k_timeout_t timeout);
#endif /* CONFIG_APP_ROLE_COMPANION */

#endif /* DS_MBOX_H_ */

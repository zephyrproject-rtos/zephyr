/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************
 * NOTE: This file is extracted from the publicly released BSP and can be found at
 * https://github.com/Infineon/TARGET_KIT_PSE84_EVAL_EPC2/blob/master/mtb_ipc_config.h
 ******************************************************************************/

/**
 * The ModusToolbox Interprocessor communication library (MTB-IPC) requires reservation of IPC
 * channels and interrupts.
 * Please refer to device documentation for what channels and interrupts are available on the
 * device.
 * Reservation ensures that there is no overlap between libraries that use IPC.
 * By default, the only library that has a reservation is MTB-SRF and these values
 * must not be deleted for SRF to function correctly.
 *
 * This file is owned by the application.  The expectation is for the user to store channel and
 * interrupt reservations
 * as needed.  Should the user wish to use a middleware library that depends on MTB IPC,
 * the user can keep track of which channels and interrupts have been used here.
 */

#ifndef SOC_INFINEON_EDGE_PSE84_MTB_IPC_CONFIG_H_
#define SOC_INFINEON_EDGE_PSE84_MTB_IPC_CONFIG_H_

#include "mtb_ipc.h"


/** Define the channel used to set up the IPC instance set up for SRF operations*/
#define MTB_IPC_CHANNEL_SRF                          (MTB_IPC_CHAN_0)

/** Define Semaphore index used to initialize the IPC instance set up for SRF operations */
#define MTB_IPC_SEMA_NUM_SRF                         (0UL)

/** Define reserved semaphore IRQ for SRF operations on Relay core */
#define MTB_IPC_IRQ_SEMA_SRF_RELAY                   (MTB_IPC_IRQ_USER)

/** Define reserved semaphore IRQ for SRF operations on Relay core */
#define MTB_IPC_IRQ_QUEUE_SRF_RELAY                  (MTB_IPC_IRQ_USER + 1)

/** Define reserved semaphore IRQ for SRF operations on client core*/
#define MTB_IPC_IRQ_SEMA_SRF_CLIENT                  (MTB_IPC_IRQ_USER + 2)

/** Define reserved semaphore IRQ for SRF operations on client core*/
#define MTB_IPC_IRQ_QUEUE_SRF_CLIENT                 (MTB_IPC_IRQ_USER + 3)

/** IPC Read Semaphore number to be used by SRF IPC mailbox */
#define MTB_IPC_SEMA_NUM_SRF_MBOX_READ              (MTB_IPC_SEMA_NUM_SRF + 1)

/** IPC Write Semaphore number to be used by SRF IPC mailbox */
#define MTB_IPC_SEMA_NUM_SRF_MBOX_WRITE             (MTB_IPC_SEMA_NUM_SRF_MBOX_READ + 1)

/** (Inclusive) Start of Semaphore indexes reserved by SRF for requests */
#define MTB_IPC_SEMA_NUM_SRF_REQ_START              (MTB_IPC_SEMA_NUM_SRF_MBOX_WRITE + 1)

/** (Inclusive) End of Semaphore indexes reserved by SRF for requests,
 *  Default of pool size 3UL
 */
#define MTB_IPC_SEMA_NUM_SRF_REQ_END                (MTB_IPC_SEMA_NUM_SRF_REQ_START + 2UL)

/** Mailbox Index to use in SRF IPC messaging */
#define MTB_IPC_MBOX_IDX_SRF                        (0UL)

#endif /* SOC_INFINEON_EDGE_PSE84_MTB_IPC_CONFIG_H_ */

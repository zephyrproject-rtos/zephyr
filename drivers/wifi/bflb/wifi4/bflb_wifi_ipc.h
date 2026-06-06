/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BFLB_WIFI_IPC_H_
#define BFLB_WIFI_IPC_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/sys_io.h>

#include <reg_ipc_app.h>

#include "bflb_wifi.h"

/* IPC mailbox MMIO: WiFi block base (DT wifi0 reg) + IPC_REG_BASE_ADDR,
 * register offsets from the vendor reg_ipc_app.h.
 */
#define BFLB_IPC_REG_BASE       (BFLB_WIFI_REG_BASE + IPC_REG_BASE_ADDR)
#define BFLB_IPC_A2E_TRIGGER    (BFLB_IPC_REG_BASE + IPC_APP2EMB_TRIGGER_OFFSET)
#define BFLB_IPC_E2A_RAWSTATUS  (BFLB_IPC_REG_BASE + IPC_EMB2APP_RAWSTATUS_OFFSET)
#define BFLB_IPC_E2A_ACK        (BFLB_IPC_REG_BASE + IPC_EMB2APP_ACK_OFFSET)
#define BFLB_IPC_E2A_UNMASK_SET (BFLB_IPC_REG_BASE + IPC_EMB2APP_UNMASK_SET_OFFSET)
#define BFLB_IPC_E2A_UNMASK_CLR (BFLB_IPC_REG_BASE + IPC_EMB2APP_UNMASK_CLEAR_OFFSET)
#define BFLB_IPC_E2A_STATUS     (BFLB_IPC_REG_BASE + IPC_EMB2APP_STATUS_OFFSET)

#define BFLB_IPC_UNMASK_ALL_BITS 0xFFFFFFFFU

static inline void ipc_a2e_trigger(uint32_t bit)
{
	sys_write32(bit, BFLB_IPC_A2E_TRIGGER);
}

static inline void ipc_e2a_ack(uint32_t bits)
{
	sys_write32(bits, BFLB_IPC_E2A_ACK);
}

static inline void ipc_e2a_unmask_set(uint32_t bits)
{
	sys_write32(bits, BFLB_IPC_E2A_UNMASK_SET);
}

int bflb_wifi_ipc_init(struct bflb_wifi_dev *d);
int bflb_wifi_ipc_send_cmd(struct bflb_wifi_dev *d, uint16_t msg_id, uint16_t cfm_id,
			   const void *params, uint32_t params_len, void *cfm_out,
			   uint32_t cfm_len);

/* Per-SoC IPC glue and TX path (soc/bflb/<series>/wifi/). */

/* Seed the blob's shared-env descriptors/lists at driver init. */
void bflb_wifi_ipc_seed(void);
/* ISR body for the "wifi-ipc" interrupt (E2A mailbox). */
void bflb_wifi_ipc_isr(const void *arg);
/* Wake the firmware scheduler after an A2E MSG doorbell trigger. */
void bflb_wifi_ipc_msg_kick(void);
/* Hook run once after the MM/ME MAC bring-up completed. */
void bflb_wifi_ipc_mac_init_done(struct bflb_wifi_dev *d);

/* tx (per-SoC implementation) */
int bflb_wifi_tx_eth(const uint8_t *frame, uint16_t len, uint8_t vif_idx, uint8_t sta_idx);
extern volatile uint32_t bflb_wifi_tx_status;

#endif /* BFLB_WIFI_IPC_H_ */

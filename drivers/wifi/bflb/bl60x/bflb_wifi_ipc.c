/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host side of the shared-memory mailbox exposed by the BL60x wifi4
 * firmware blob.
 *
 *   shared memory : struct ipc_shared_env_tag ipc_shared_env (SHAREDRAMIPC).
 *   ctrl regs     : 0x44800000 APP2EMB_TRIGGER (host->fw doorbell),
 *                   +0x04 EMB2APP_RAWSTATUS, +0x08 EMB2APP_ACK (W1C),
 *                   +0x0C EMB2APP_UNMASK_SET, +0x10 EMB2APP_UNMASK_CLR,
 *                   +0x1C EMB2APP_STATUS (masked).
 *
 * Flow: host fills ipc_shared_env.msg_a2e_buf with {lmac_msg, params}, writes
 * IPC_IRQ_A2E_MSG to the trigger.  Firmware ACKs via IPC_IRQ_E2A_MSG_ACK
 * (command accepted) then sends IPC_IRQ_E2A_MSG (a confirmation or event).
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <lmac_msg.h>
#include <utils_list.h>
#include <ipc_compat.h>
#include <ipc_shared.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define WIFI_DT_NODE     DT_NODELABEL(wifi0)
#define WIFI_IRQ_NUM     DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi, irq)
#define WIFI_IRQ_PRI     DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi, priority)
#define WIFI_IPC_IRQ_NUM DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi_ipc, irq)
#define WIFI_IPC_IRQ_PRI DT_IRQ_BY_NAME(WIFI_DT_NODE, wifi_ipc, priority)

/* The blob provides ipc_shared_env (in its SHAREDRAMIPC section). */
extern struct ipc_shared_env_tag ipc_shared_env;

extern void ipc_emb_notify(void);
extern void mac_irq(void);
extern void bl_irq_handler(void);
extern void ipc_emb_tx_irq(void);
extern void ipc_emb_msg_irq(void);
extern void ipc_emb_cfmback_irq(void);

/* IPC mailbox MMIO: REG_WIFI_REG_BASE (0x44000000) + IPC_REG_BASE_ADDR
 * (0x00800000) -> host side MMIO at 0x44800000.
 */
#define BFLB_IPC_REG_BASE       0x44800000U
#define BFLB_IPC_A2E_TRIGGER    (BFLB_IPC_REG_BASE + 0x00U)
#define BFLB_IPC_E2A_RAWSTATUS  (BFLB_IPC_REG_BASE + 0x04U)
#define BFLB_IPC_E2A_ACK        (BFLB_IPC_REG_BASE + 0x08U)
#define BFLB_IPC_E2A_UNMASK_SET (BFLB_IPC_REG_BASE + 0x0CU)
#define BFLB_IPC_E2A_UNMASK_CLR (BFLB_IPC_REG_BASE + 0x10U)
#define BFLB_IPC_E2A_STATUS     (BFLB_IPC_REG_BASE + 0x1CU)

#define BFLB_IPC_UNMASK_ALL_BITS 0xFFFFFFFFU

/* A2E_STATUS bits dispatched to the firmware-side IPC IRQ chain. */
#define BFLB_A2E_MSG_BIT         BIT(1)
#define BFLB_A2E_RXDESC_BACK_BIT BIT(4)
#define BFLB_A2E_RXBUF_BACK_BIT  BIT(5)
#define BFLB_A2E_TXDESC_BITS     (BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12))

/* TXCFM IRQ bit mask (NX_TXQ_CNT=4, bits start at 7). */
#define BFLB_TXQ_CNT           4U
#define BFLB_TXCFM_FIRST_BIT   7U
#define IPC_IRQ_E2A_TXCFM_MASK (((1U << BFLB_TXQ_CNT) - 1U) << BFLB_TXCFM_FIRST_BIT)

/* Submit/done lists in shared env.
 *
 * The blob's `struct ipc_shared_env_tag` is 0x6f4 bytes; its list pointers
 * live at fixed offsets addressed by raw pointer.  +0x6e4 is the submit
 * queue (host pushes, FW pops), +0x6ec the recycle queue.
 */
#define BFLB_IPC_LIST_SUBMIT_OFF 0x6E4U
#define BFLB_IPC_LIST_DONE_OFF   0x6ECU
#define BFLB_IPC_LIST_BYTES      16U /* 2 lists * sizeof(utils_list) */

/* Per-descriptor stride in the blob's txdesc0 array: vendor txdesc_host is
 * { list_hdr 4 + host_id 4 + ready 4 + pad_txdesc[208] + pad_buf[400] },
 * padded to 624 bytes, two descriptors total.
 */
#define BFLB_IPC_TXDESC0_OFF 516U /* msg_a2e_buf(512) + pattern_addr(4) */

/* status_addr lives at hostdesc word 7, which is td_words[3 + 4]. */
#define BFLB_TXDESC_WORD_STATUS_ADDR (3U + 4U)

#define BFLB_IPC_CFM_TIMEOUT_MS 3000U

/* Vendor src_id base (DRV_TASK_ID) and an 8-bit rolling token field. */
#define BFLB_DRV_TASK_ID        100U
#define BFLB_DRV_TASK_TKN_SHIFT 8
#define BFLB_DRV_TASK_TKN_MASK  0xFFU

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

static struct utils_list *bflb_ipc_list_submit(void)
{
	return (struct utils_list *)((uint8_t *)&ipc_shared_env + BFLB_IPC_LIST_SUBMIT_OFF);
}

static struct utils_list *bflb_ipc_list_done(void)
{
	return (struct utils_list *)((uint8_t *)&ipc_shared_env + BFLB_IPC_LIST_DONE_OFF);
}

volatile uint8_t *bflb_wifi_ipc_txdesc(uint32_t idx)
{
	return (volatile uint8_t *)&ipc_shared_env + BFLB_IPC_TXDESC0_OFF +
	       (idx * BFLB_WIFI_TXDESC_STRIDE);
}

/* TXCFM handler: the blob has no list_cfm; txu_cntrl_cfm clears `ready` in
 * place.  Just drain the done list so it doesn't grow unbounded.
 */
static void bflb_wifi_ipc_recycle_txcfm(void)
{
	struct utils_list *done = bflb_ipc_list_done();
	struct utils_list_hdr *cur = done->first;

	while (cur != NULL) {
		struct utils_list_hdr *next = cur->next;

		cur->next = NULL;
		cur = next;
	}
	done->first = NULL;
	done->last = NULL;
}

static void wifi_ipc_isr(const void *arg)
{
	uint32_t status;
	uint32_t emb_status;

	ARG_UNUSED(arg);

	/* ACK the latched E2A bits -- the blob's bl_irq_handler only disables
	 * the E2A unmask and wakes the scheduler, it doesn't ACK.  Without an
	 * ACK the latched bits stay set and the IRQ re-fires.
	 */
	status = sys_read32(BFLB_IPC_E2A_RAWSTATUS);
	if (status != 0U) {
		ipc_e2a_ack(status);
	}

	/* Drive the FW-side IPC IRQ chain inline.  The blob's intc_init wires
	 * IRQs 61-63 to ipc_emb_msg_irq / ipc_emb_cfmback_irq / ipc_emb_tx_irq
	 * in its own intc table, but Zephyr routes the IRQ here first.
	 * Without these calls A2E_TXDESC triggers never wake ipc_emb_tx_evt.
	 */
	emb_status = sys_read32(BFLB_IPC_E2A_STATUS);
	if ((emb_status & BFLB_A2E_TXDESC_BITS) != 0U) {
		ipc_emb_tx_irq();
	}
	if ((emb_status & BFLB_A2E_MSG_BIT) != 0U) {
		ipc_emb_msg_irq();
	}
	if ((emb_status & (BFLB_A2E_RXDESC_BACK_BIT | BFLB_A2E_RXBUF_BACK_BIT)) != 0U) {
		ipc_emb_cfmback_irq();
	}

	bl_irq_handler();
	if ((status & IPC_IRQ_E2A_TXCFM_MASK) != 0U) {
		bflb_wifi_ipc_recycle_txcfm();
	}
}

/* mac_irq's per-source handlers only call ke_evt_set, not notify.  If the
 * fw task is blocked in ipc_emb_wait when a MAC IRQ fires the events would
 * sit in ke_env forever -- notify the same sem so any MAC IRQ wakes the
 * scheduler.
 */
static void wifi_mac_isr(const void *arg)
{
	ARG_UNUSED(arg);
	mac_irq();
	ipc_emb_notify();
}

int bflb_wifi_ipc_init(struct bflb_wifi_dev *d)
{
	struct utils_list *submit;
	struct utils_list *done;

	k_sem_init(&d->pending.cfm, 0, 1);
	d->pending.cfm_id = BFLB_WIFI_PENDING_CFM_NONE;

	submit = bflb_ipc_list_submit();
	done = bflb_ipc_list_done();
	memset(submit, 0, BFLB_IPC_LIST_BYTES);
	memset((void *)(uintptr_t)bflb_wifi_ipc_txdesc(0), 0,
	       BFLB_WIFI_TXDESC_COUNT * BFLB_WIFI_TXDESC_STRIDE);

	/* Pre-seed every descriptor's hostdesc.status_addr with a non-NULL
	 * host word.  The blob's txu_cntrl_cfm dereferences status_addr
	 * unconditionally and would fault on NULL.
	 */
	for (uint32_t i = 0; i < BFLB_WIFI_TXDESC_COUNT; i++) {
		volatile uint32_t *td = (volatile uint32_t *)bflb_wifi_ipc_txdesc(i);

		td[BFLB_TXDESC_WORD_STATUS_ADDR] = (uint32_t)(uintptr_t)&bflb_wifi_tx_status;
	}

	ipc_e2a_ack(BFLB_IPC_UNMASK_ALL_BITS);
	ipc_e2a_unmask_set(BFLB_IPC_UNMASK_ALL_BITS);

	IRQ_CONNECT(WIFI_IRQ_NUM, WIFI_IRQ_PRI, wifi_mac_isr, NULL, 0);
	IRQ_CONNECT(WIFI_IPC_IRQ_NUM, WIFI_IPC_IRQ_PRI, wifi_ipc_isr, NULL, 0);
	irq_enable(WIFI_IRQ_NUM);
	irq_enable(WIFI_IPC_IRQ_NUM);

	LOG_DBG("ipc init: shared_env=%p submit=%p done=%p td0=%p", &ipc_shared_env, submit, done,
		(const void *)bflb_wifi_ipc_txdesc(0));
	return 0;
}

/* Build an lmac_msg inside ipc_shared_env.msg_a2e_buf, trigger the
 * doorbell, and wait for the confirmation message via the IPC ISR.
 */
int bflb_wifi_ipc_send_cmd(struct bflb_wifi_dev *d, uint16_t msg_id, uint16_t cfm_id,
			   const void *params, uint32_t params_len, void *cfm_out, uint32_t cfm_len)
{
	volatile uint32_t *buf;
	struct lmac_msg *msg;
	int ret = 0;

	k_mutex_lock(&d->cmd_mutex, K_FOREVER);

	d->pending.cfm_id = cfm_id;
	d->pending.cfm_buf = cfm_out;
	d->pending.cfm_len = cfm_len;
	k_sem_reset(&d->pending.cfm);

	d->msga2e_tkn++;

	buf = ipc_shared_env.msg_a2e_buf.msg;
	msg = (struct lmac_msg *)buf;

	msg->id = msg_id;
	msg->dest_id = MSG_T(msg_id);
	msg->src_id = BFLB_DRV_TASK_ID |
		      ((d->msga2e_tkn & BFLB_DRV_TASK_TKN_MASK) << BFLB_DRV_TASK_TKN_SHIFT);
	msg->param_len = params_len;
	if ((params_len > 0U) && (params != NULL)) {
		memcpy((void *)msg->param, params, params_len);
	}

	ipc_a2e_trigger(IPC_IRQ_A2E_MSG);

	if (cfm_id != BFLB_WIFI_PENDING_CFM_NONE) {
		if (k_sem_take(&d->pending.cfm, K_MSEC(BFLB_IPC_CFM_TIMEOUT_MS)) != 0) {
			LOG_WRN("msg 0x%04x: no CFM (expected 0x%04x)", msg_id, cfm_id);
			ret = -ETIMEDOUT;
		}
	}

	d->pending.cfm_id = BFLB_WIFI_PENDING_CFM_NONE;
	k_mutex_unlock(&d->cmd_mutex);
	return ret;
}

/* Host symbols the firmware blob imports for E2A message flow. */

void ipc_host_disable_irq_e2a(void)
{
	sys_write32(IPC_IRQ_E2A_ALL, BFLB_IPC_E2A_UNMASK_CLR);
}

/* Invoked by the blob bottom-half for each E2A command message.  Routes
 * confirmation messages to the matching pending request and dispatches
 * everything else through the driver event handler.
 */
void bl_rx_e2a_handler(void *arg)
{
	struct ipc_e2a_msg *msg = arg;

	if (msg != NULL) {
		struct bflb_wifi_dev *d = &bflb_wifi;
		uint16_t id = msg->id;

		if ((d->pending.cfm_id != BFLB_WIFI_PENDING_CFM_NONE) &&
		    (id == d->pending.cfm_id)) {
			if ((d->pending.cfm_buf != NULL) && (msg->param_len != 0U)) {
				uint32_t n = msg->param_len;

				if (n > d->pending.cfm_len) {
					n = d->pending.cfm_len;
				}
				memcpy(d->pending.cfm_buf, msg->param, n);
			}
			k_sem_give(&d->pending.cfm);
		} else {
			bflb_wifi_handle_e2a_msg(d, id, msg->param, msg->param_len);
		}
	}

	/* Re-arm the E2A unmask the blob's IRQ handler cleared. */
	ipc_e2a_unmask_set(BFLB_IPC_UNMASK_ALL_BITS);
}

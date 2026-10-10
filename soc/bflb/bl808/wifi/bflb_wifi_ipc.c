/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <ipc_shared.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

/* Headroom in front of the ethernet header, as the firmware expects it. */
#define BFLB_TX_HEADROOM 48U

#define BFLB_TX_FREE_WAIT_MAX_MS    200U
#define BFLB_TX_CFM_WAIT_STEP_TICKS 1
/* A confirmation that has not arrived by now is not coming: the firmware
 * is recovering the MAC, which takes hundreds of milliseconds.
 */
#define BFLB_TX_CFM_WAIT_MAX_MS     16U

#define BFLB_TX_STATINFO_DONE    BIT(31)
#define BFLB_TX_STATINFO_SUCCESS BIT(23)

#define BFLB_TXDESC_READY_FILLED 0xFFFFFFFFU

/* packet_addr == this sentinel means "use chained-pbuf pointers". */
#define BFLB_HOSTDESC_PBUF_CHAINED_MAGIC 0x11111111U

#define BFLB_ETH_MAC_LEN  6U
#define BFLB_VIF_TYPE_STA 1U /* MM_STA */
#define BFLB_TID_BE       0U

struct bflb_eth_frame_hdr {
	uint8_t dst[BFLB_ETH_MAC_LEN];
	uint8_t src[BFLB_ETH_MAC_LEN];
	uint16_t etype_be;
} __packed;

extern struct ipc_shared_env_tag ipc_shared_env;

extern void bl_irq_handler(void);
extern void ipc_emb_notify(void);

/* Serialise TX -- one frame in flight, multiple callers. */
static K_MUTEX_DEFINE(bflb_tx_mutex);

/* Given by the E2A_TXCFM ISR so the TX path wakes on completion instead of
 * sleep-polling the status word.
 */
static K_SEM_DEFINE(bflb_tx_cfm_sem, 0, 1);

/* Status word the FW writes via host->status_addr, one per descriptor.
 * Bits: 31 DESC_DONE_TX, 30 DESC_DONE_SW_TX, 23 FRAME_SUCCESSFUL_TX.
 */
volatile uint32_t bflb_wifi_tx_status[NX_TXDESC_CNT0] Z_GENERIC_SECTION(.fw.SHRAM);

static void bflb_ipc_txcfm_recycle(void);
static volatile struct txdesc_host *bflb_tx_alloc_desc(void);
static bool bflb_tx_wait_cfm(uint32_t idx);

/* The firmware only moves descriptors from list_ongoing to list_cfm;
 * returning them to list_free is the host's job.
 */
static void bflb_ipc_txcfm_recycle(void)
{
	sys_snode_t *node = sys_slist_get(&ipc_shared_env.list_cfm);

	while (node != NULL) {
		struct txdesc_host *td = CONTAINER_OF(node, struct txdesc_host, list_hdr);

		td->host_id = NULL;
		sys_slist_append(&ipc_shared_env.list_free, node);
		node = sys_slist_get(&ipc_shared_env.list_cfm);
	}
}

static volatile struct txdesc_host *bflb_tx_alloc_desc(void)
{
	sys_snode_t *node;
	uint32_t waited;
	unsigned int key;

	for (waited = 0; waited < k_ms_to_ticks_ceil32(BFLB_TX_FREE_WAIT_MAX_MS); waited++) {
		key = irq_lock();
		bflb_ipc_txcfm_recycle();
		node = sys_slist_get(&ipc_shared_env.list_free);
		irq_unlock(key);

		if (node != NULL) {
			return CONTAINER_OF(node, struct txdesc_host, list_hdr);
		}
		(void)k_sem_take(&bflb_tx_cfm_sem, K_TICKS(BFLB_TX_CFM_WAIT_STEP_TICKS));
	}

	return NULL;
}

/* Wait for DESC_DONE_TX in the status word.  The FW scheduler thread must
 * run to produce it, so this always blocks -- never busy-waits -- and the
 * sem is only a wakeup hint; the status word stays authoritative.
 */
static bool bflb_tx_wait_cfm(uint32_t idx)
{
	uint32_t waited;

	for (waited = 0; waited < k_ms_to_ticks_ceil32(BFLB_TX_CFM_WAIT_MAX_MS);
	     waited += BFLB_TX_CFM_WAIT_STEP_TICKS) {
		if ((bflb_wifi_tx_status[idx] & BFLB_TX_STATINFO_DONE) != 0U) {
			return true;
		}
		(void)k_sem_take(&bflb_tx_cfm_sem, K_TICKS(BFLB_TX_CFM_WAIT_STEP_TICKS));
	}

	return (bflb_wifi_tx_status[idx] & BFLB_TX_STATINFO_DONE) != 0U;
}

void bflb_wifi_ipc_seed(void)
{
	uint32_t i;

	sys_slist_init(&ipc_shared_env.list_free);
	sys_slist_init(&ipc_shared_env.list_ongoing);
	sys_slist_init(&ipc_shared_env.list_cfm);

	for (i = 0; i < NX_TXDESC_CNT0; i++) {
		struct txdesc_host *td = (struct txdesc_host *)&ipc_shared_env.txdesc0[i];
		struct txdesc_upper *up = (struct txdesc_upper *)td->pad_txdesc;

		/* txu_cntrl_cfm dereferences status_addr unconditionally. */
		up->host.status_addr = (uint32_t)(uintptr_t)&bflb_wifi_tx_status[i];
		sys_slist_append(&ipc_shared_env.list_free, &td->list_hdr);
	}
}

void bflb_wifi_ipc_isr(const void *arg)
{
	uint32_t status;

	ARG_UNUSED(arg);

	/* ACK the latched E2A bits -- the blob's bl_irq_handler only disables
	 * the E2A unmask and wakes the scheduler, it doesn't ACK.  Without an
	 * ACK the latched bits stay set and the IRQ re-fires.
	 */
	status = sys_read32(BFLB_IPC_E2A_RAWSTATUS);
	if (status != 0U) {
		ipc_e2a_ack(status);
	}

	bl_irq_handler();

	if ((status & IPC_IRQ_E2A_TXCFM) != 0U) {
		k_sem_give(&bflb_tx_cfm_sem);
	}
}

/* The A2E doorbell reaches the firmware through the WiFi interrupt
 * controller, whose MAC interrupt already wakes the scheduler.
 */
void bflb_wifi_ipc_msg_kick(void)
{
}

void bflb_wifi_ipc_mac_init_done(struct bflb_wifi_dev *d)
{
	ARG_UNUSED(d);
}

int bflb_wifi_tx_eth(const uint8_t *frame, uint16_t len, uint8_t vif_idx, uint8_t sta_idx)
{
	const struct bflb_eth_frame_hdr *eth = (const struct bflb_eth_frame_hdr *)frame;
	volatile struct txdesc_host *td;
	volatile struct txbuf_host *txbuf;
	struct hostdesc *host;
	uint16_t payload_len;
	uint8_t *buf;
	uint32_t slot;
	unsigned int key;

	if ((frame == NULL) || (len < BFLB_WIFI_ETH_HDR_LEN) ||
	    (((uint32_t)len + BFLB_TX_HEADROOM) > sizeof(ipc_shared_env.txbuf[0].buf))) {
		return -EINVAL;
	}

	k_mutex_lock(&bflb_tx_mutex, K_FOREVER);

	td = bflb_tx_alloc_desc();
	if (td == NULL) {
		LOG_WRN("tx: no free txdesc after %ums", BFLB_TX_FREE_WAIT_MAX_MS);
		k_mutex_unlock(&bflb_tx_mutex);
		return -ENOMEM;
	}

	slot = (uint32_t)(td - ipc_shared_env.txdesc0);
	txbuf = &ipc_shared_env.txbuf[slot];
	buf = (uint8_t *)(uintptr_t)txbuf->buf;
	payload_len = len - BFLB_WIFI_ETH_HDR_LEN;

	/* WIFI_RAM is the uncached view, so the MAC DMA sees these writes
	 * without cache maintenance.
	 */
	memset(buf, 0, BFLB_TX_HEADROOM);
	memcpy(buf + BFLB_TX_HEADROOM, frame, len);

	memset((void *)(uintptr_t)td->pad_txdesc, 0, sizeof(td->pad_txdesc));
	host = &((struct txdesc_upper *)(uintptr_t)td->pad_txdesc)->host;
	memcpy(host->eth_dest_addr.array, eth->dst, BFLB_ETH_MAC_LEN);
	memcpy(host->eth_src_addr.array, eth->src, BFLB_ETH_MAC_LEN);
	host->ethertype = eth->etype_be;
	host->packet_addr = BFLB_HOSTDESC_PBUF_CHAINED_MAGIC;
	host->packet_len = payload_len;
	host->vif_idx = vif_idx;
	host->vif_type = BFLB_VIF_TYPE_STA;
	host->staid = sta_idx;
	host->tid = BFLB_TID_BE;
	host->pbuf_addr = (uint32_t)(uintptr_t)txbuf;
	host->pbuf_chained_ptr[0] =
		(uint32_t)(uintptr_t)(buf + BFLB_TX_HEADROOM + BFLB_WIFI_ETH_HDR_LEN);
	host->pbuf_chained_len[0] = payload_len;
	bflb_wifi_tx_status[slot] = 0;
	host->status_addr = (uint32_t)(uintptr_t)&bflb_wifi_tx_status[slot];

	key = irq_lock();
	td->host_id = (void *)txbuf;
	td->ready = BFLB_TXDESC_READY_FILLED;
	sys_slist_append(&ipc_shared_env.list_ongoing, (sys_snode_t *)&td->list_hdr);
	irq_unlock(key);

	ipc_a2e_trigger(BIT(IPC_IRQ_A2E_TXDESC_FIRSTBIT));
	ipc_emb_notify();

	/* The frame is the firmware's once the doorbell is rung, so the send
	 * has succeeded.  Waiting for the confirmation only paces the next
	 * one against the descriptor pool.
	 */
	if (!bflb_tx_wait_cfm(slot)) {
		LOG_DBG("tx: no cfm within %ums, status=0x%08x", BFLB_TX_CFM_WAIT_MAX_MS,
			bflb_wifi_tx_status[slot]);
	} else if ((bflb_wifi_tx_status[slot] & BFLB_TX_STATINFO_SUCCESS) == 0U) {
		LOG_DBG("tx: no ack, status=0x%08x", bflb_wifi_tx_status[slot]);
	}

	k_mutex_unlock(&bflb_tx_mutex);
	return 0;
}

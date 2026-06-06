/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL602 SoC side of the WiFi IPC: shared-environment seeding, the E2A
 * (emb-to-app) mailbox ISR, firmware-blob quirk workarounds and the TX
 * path.  "Emb" is the embedded MAC firmware blob, "app" is the host
 * (Zephyr); A2E is the app-to-emb direction.
 *
 * TX: the wifi4 firmware blob drains TX work from txdesc_host
 * descriptors in IPC shared memory.  The host fills a free descriptor
 * (one with `ready` cleared by the blob), sets `ready`, then calls the
 * blob's TX processing chain directly so the MAC HW is armed
 * synchronously.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/barrier.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <ipc_shared.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

/* Blob txdesc_host layout: { list_hdr 4 + host_id 4 + ready 4 +
 * pad_txdesc[208] + pad_buf[400] } padded to 624 bytes, two descriptors.
 * The blob's `struct ipc_shared_env_tag` is 0x6f4 bytes and does not
 * match the SDK header, so its fields are addressed by raw offset.
 */
#define BFLB_TXDESC_STRIDE   624U
#define BFLB_TXDESC_COUNT    2U
#define BFLB_IPC_TXDESC0_OFF 516U /* msg_a2e_buf(512) + pattern_addr(4) */

/* Descriptor queues in the shared environment, as this firmware build
 * uses them: ipc_emb_tx_evt() pops from +0x6e4 and ipc_emb_txcfm() pushes
 * completions to +0x6ec.  The three-list layout in newer vendor headers
 * does not apply -- nothing in the blob touches +0x6f4.
 */
#define BFLB_IPC_LIST_SUBMIT_OFF 0x6E4U
#define BFLB_IPC_LIST_DONE_OFF   0x6ECU /* FW-owned, host never touches it */
#define BFLB_IPC_LIST_BYTES      16U    /* 2 * sizeof(struct bflb_ipc_list) */

/* A2E doorbell bit that tells the FW a txdesc is ready. */
#define BFLB_IPC_A2E_TXDESC_FIRSTBIT 8U

/* txdesc word offsets indexed directly. */
#define BFLB_TXDESC_WORD_HOST_ID 1U
#define BFLB_TXDESC_WORD_READY   2U
#define BFLB_TXDESC_HOSTDESC_OFF 12U /* list_hdr(4) + host_id(4) + ready(4) */
#define BFLB_TXDESC_READY_FILLED 0xFFFFFFFFU
#define BFLB_TXDESC_READY_FREE   0U
#define BFLB_HOSTDESC_OFF_IN_PAD 4U /* txdesc_upper layout: co_list_hdr then hostdesc */

/* status_addr lives at hostdesc word 7, which is td_words[3 + 4]. */
#define BFLB_TXDESC_WORD_STATUS_ADDR (3U + 4U)

/* Blob-ABI hostdesc offsets, asserted against the SDK header layout. */
#define BFLB_HOSTDESC_OFF_ETH_DEST_ADDR 16U
#define BFLB_HOSTDESC_OFF_ETHERTYPE     28U
#define BFLB_HOSTDESC_OFF_TID           42U
#define BFLB_HOSTDESC_OFF_VIF_IDX       43U
#define BFLB_HOSTDESC_OFF_VIF_TYPE      44U
#define BFLB_HOSTDESC_OFF_STAID         45U

/* RX-batch quirk: rxu_swdesc_upload_evt drops new RX descs once
 * `rxl_cntrl_env + 20` reaches 5 (see bflb_rx_batch_reset_handler).
 */
#define BFLB_RXL_CNTRL_BATCH_OFF 20U
#define BFLB_RX_BATCH_RESET_MS   20

#define BFLB_TXBUF_ALIGN 4U

/* The FW leaves a 48-byte headroom before the ethernet header for the
 * 802.11 MAC/QoS/SNAP/IV/MIC encapsulation it prepends.
 */
#define BFLB_TX_HEADROOM   48U
#define BFLB_TX_PAYLOAD_SZ 1536U /* MTU + slack */
#define BFLB_TXBUF_SZ      (BFLB_TX_HEADROOM + BFLB_TX_PAYLOAD_SZ)

#define BFLB_TX_FREE_WAIT_MAX_MS    200U
#define BFLB_TX_CFM_WAIT_STEP_TICKS 1
/* A confirmation that has not arrived by now is not coming: the firmware
 * is recovering the MAC, which takes hundreds of milliseconds.  Riding
 * that out blocks the interface on ~4% of frames for ~60% of the airtime.
 */
#define BFLB_TX_CFM_WAIT_MAX_MS     16U

#define BFLB_TX_STATINFO_DONE    BIT(31)
#define BFLB_TX_STATINFO_SUCCESS BIT(23)

/* packet_addr == this sentinel means "use chained-pbuf pointers". */
#define BFLB_HOSTDESC_PBUF_CHAINED_MAGIC 0x11111111U

#define BFLB_ETH_MAC_LEN  6U
#define BFLB_VIF_TYPE_STA 1U /* MM_STA per vendor bl_output */
#define BFLB_TID_BE       0U

/* The FW walks these itself, so they use the vendor utils_list layout. */
struct bflb_ipc_list_hdr {
	volatile struct bflb_ipc_list_hdr *next;
};

struct bflb_ipc_list {
	volatile struct bflb_ipc_list_hdr *first;
	volatile struct bflb_ipc_list_hdr *last;
};

struct bflb_eth_frame_hdr {
	uint8_t dst[BFLB_ETH_MAC_LEN];
	uint8_t src[BFLB_ETH_MAC_LEN];
	uint16_t etype_be;
} __packed;

extern struct ipc_shared_env_tag ipc_shared_env;

extern void bl_irq_handler(void);
extern void ipc_emb_notify(void);

extern uint8_t rxl_cntrl_env[];

/* Per-slot staging buffer in WIFI_RAM.  MAC HW DMA reads
 * pbuf_chained_ptr[0] from this region, so it must rotate with the
 * descriptor: a shared buffer is overwritten while the previous frame is
 * still being fetched.
 */
static uint8_t bflb_txbuf[BFLB_TXDESC_COUNT][BFLB_TXBUF_SZ]
	__aligned(BFLB_TXBUF_ALIGN) Z_GENERIC_SECTION(SHAREDRAM);

/* Serialise TX -- one frame in flight, multiple callers. */
static K_MUTEX_DEFINE(bflb_tx_mutex);

/* Given by the E2A_TXCFM ISR so the TX path wakes on completion instead of
 * sleep-polling the status word.
 */
static K_SEM_DEFINE(bflb_tx_cfm_sem, 0, 1);

/* Status word the FW writes via host->status_addr, one per descriptor;
 * uint32_t is naturally word-aligned, which is all the MAC HW DMA requires.
 * Bits: 31 DESC_DONE_TX, 30 DESC_DONE_SW_TX, 23 FRAME_SUCCESSFUL_TX.
 *
 * Per-slot, not shared: with one word for both descriptors a late write
 * from the previous frame reads as the current frame's completion and the
 * slot gets refilled while the MAC is still DMA-ing it.
 */
volatile uint32_t bflb_wifi_tx_status[BFLB_TXDESC_COUNT]
	__aligned(BFLB_TXBUF_ALIGN) Z_GENERIC_SECTION(SHAREDRAM);

static volatile uint8_t *bflb_ipc_txdesc(uint32_t idx);
static volatile struct bflb_ipc_list *bflb_ipc_list(uint32_t off);
static uint32_t bflb_ipc_txdesc_idx(const volatile uint8_t *td_raw);
static bool bflb_tx_wait_cfm(uint32_t idx);
static void bflb_rx_batch_reset_handler(struct k_timer *t);
static volatile uint8_t *bflb_tx_alloc_slot(void);
static void bflb_tx_fill_hostdesc(volatile uint8_t *td_raw, const struct bflb_eth_frame_hdr *eth,
				  const uint8_t *frame, uint16_t len, uint8_t vif_idx,
				  uint8_t sta_idx);

static K_TIMER_DEFINE(bflb_rx_batch_timer, bflb_rx_batch_reset_handler, NULL);

static volatile uint8_t *bflb_ipc_txdesc(uint32_t idx)
{
	return (volatile uint8_t *)&ipc_shared_env + BFLB_IPC_TXDESC0_OFF +
	       (idx * BFLB_TXDESC_STRIDE);
}

/* utils_list semantics, reproduced exactly.  sys_slist_t has the same
 * {first, last} layout but appends in the opposite order: it clears the
 * new node's next pointer before relinking the old tail, so re-pushing the
 * current tail leaves a permanent self-loop that traps the FW mid-walk.
 */
static void bflb_ipc_list_init(volatile struct bflb_ipc_list *list)
{
	list->first = NULL;
	list->last = NULL;
}

/* The firmware clears the ready word before it unlinks the descriptor, so
 * a slot can come back to the host while still queued.  Refilling it then
 * rewrites a descriptor the firmware is about to walk.
 */
static bool bflb_ipc_on_submit_list(const volatile uint32_t *td)
{
	const volatile struct bflb_ipc_list *list = bflb_ipc_list(BFLB_IPC_LIST_SUBMIT_OFF);
	const volatile void *p = (const volatile void *)td;

	return ((const volatile void *)list->first == p) ||
	       ((const volatile void *)list->last == p);
}

/* The tail must be one of the descriptors the shared environment holds:
 * the firmware walks this list itself, and following a stale tail stores
 * through a wild pointer.
 */
static bool bflb_ipc_tail_valid(const volatile struct bflb_ipc_list_hdr *hdr)
{
	uint32_t i;

	for (i = 0; i < BFLB_TXDESC_COUNT; i++) {
		if ((const volatile void *)hdr == (const volatile void *)bflb_ipc_txdesc(i)) {
			return true;
		}
	}

	return false;
}

static void bflb_ipc_list_push_back(volatile struct bflb_ipc_list *list,
				    volatile struct bflb_ipc_list_hdr *hdr)
{
	/* An unmapped store here wedges the bus with interrupts locked, and
	 * linking a descriptor behind itself makes next point at itself,
	 * which the firmware's "while (first != NULL)" walk never leaves.
	 * Either way the list state is stale, so restart it.
	 */
	if ((list->first == NULL) || !bflb_ipc_tail_valid(list->last) || (list->last == hdr) ||
	    (list->first == hdr)) {
		list->first = hdr;
	} else {
		list->last->next = hdr;
	}

	list->last = hdr;
	hdr->next = NULL;
}

static volatile struct bflb_ipc_list *bflb_ipc_list(uint32_t off)
{
	return (volatile struct bflb_ipc_list *)((uint8_t *)&ipc_shared_env + off);
}

static uint32_t bflb_ipc_txdesc_idx(const volatile uint8_t *td_raw)
{
	return (uint32_t)(td_raw - bflb_ipc_txdesc(0)) / BFLB_TXDESC_STRIDE;
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

/* RX-batch counter unstick: on a busy channel the batch counter pegs and
 * unicast RX (e.g. DHCP OFFER) gets dropped -- periodically reset it.
 */
static void bflb_rx_batch_reset_handler(struct k_timer *t)
{
	ARG_UNUSED(t);
	*(volatile uint32_t *)&rxl_cntrl_env[BFLB_RXL_CNTRL_BATCH_OFF] = 0U;
}

/* Claim a descriptor the FW has finished with: txu_cntrl_cfm clears
 * `ready` in place, so that word is the ownership signal.
 */
static volatile uint8_t *bflb_tx_alloc_slot(void)
{
	uint32_t waited;
	uint32_t i;

	for (waited = 0; waited < k_ms_to_ticks_ceil32(BFLB_TX_FREE_WAIT_MAX_MS); waited++) {
		for (i = 0; i < BFLB_TXDESC_COUNT; i++) {
			volatile uint32_t *td = (volatile uint32_t *)bflb_ipc_txdesc(i);

			if ((td[BFLB_TXDESC_WORD_READY] == BFLB_TXDESC_READY_FREE) &&
			    !bflb_ipc_on_submit_list(td)) {
				return (volatile uint8_t *)td;
			}
		}
		(void)k_sem_take(&bflb_tx_cfm_sem, K_TICKS(BFLB_TX_CFM_WAIT_STEP_TICKS));
	}

	return NULL;
}

static void bflb_tx_fill_hostdesc(volatile uint8_t *td_raw, const struct bflb_eth_frame_hdr *eth,
				  const uint8_t *frame, uint16_t len, uint8_t vif_idx,
				  uint8_t sta_idx)
{
	struct hostdesc *host;
	volatile uint32_t *td_words;
	uint32_t slot = bflb_ipc_txdesc_idx(td_raw);
	uint16_t payload_len = len - BFLB_WIFI_ETH_HDR_LEN;

	/* The blob walks fields beyond struct hostdesc, so zero the entire
	 * pad_txdesc area, not just sizeof(struct hostdesc).
	 */
	memset((void *)(uintptr_t)(td_raw + BFLB_TXDESC_HOSTDESC_OFF), 0,
	       BFLB_TXDESC_STRIDE - BFLB_TXDESC_HOSTDESC_OFF);

	memset(bflb_txbuf[slot], 0, BFLB_TX_HEADROOM);
	memcpy(bflb_txbuf[slot] + BFLB_TX_HEADROOM, frame, len);

	host = (struct hostdesc *)((uintptr_t)td_raw + BFLB_TXDESC_HOSTDESC_OFF +
				   BFLB_HOSTDESC_OFF_IN_PAD);
	memcpy(host->eth_dest_addr.array, eth->dst, BFLB_ETH_MAC_LEN);
	memcpy(host->eth_src_addr.array, eth->src, BFLB_ETH_MAC_LEN);
	host->ethertype = eth->etype_be;
	host->packet_addr = BFLB_HOSTDESC_PBUF_CHAINED_MAGIC;
	host->packet_len = payload_len;
	host->vif_idx = vif_idx;
	host->vif_type = BFLB_VIF_TYPE_STA;
	host->staid = sta_idx;
	host->tid = BFLB_TID_BE;
	host->pbuf_addr = (uint32_t)(uintptr_t)bflb_txbuf[slot];
	host->pbuf_chained_ptr[0] =
		(uint32_t)(uintptr_t)(bflb_txbuf[slot] + BFLB_TX_HEADROOM + BFLB_WIFI_ETH_HDR_LEN);
	host->pbuf_chained_len[0] = payload_len;
	bflb_wifi_tx_status[slot] = 0;
	host->status_addr = (uint32_t)(uintptr_t)&bflb_wifi_tx_status[slot];

	td_words = (volatile uint32_t *)td_raw;
	td_words[BFLB_TXDESC_WORD_HOST_ID] = (uint32_t)(uintptr_t)bflb_txbuf[slot];
	td_words[BFLB_TXDESC_WORD_READY] = BFLB_TXDESC_READY_FILLED;
}

void bflb_wifi_ipc_seed(void)
{
	uint32_t i;

	memset((void *)(uintptr_t)bflb_ipc_list(BFLB_IPC_LIST_SUBMIT_OFF), 0, BFLB_IPC_LIST_BYTES);
	memset((void *)(uintptr_t)bflb_ipc_txdesc(0), 0, BFLB_TXDESC_COUNT * BFLB_TXDESC_STRIDE);

	bflb_ipc_list_init(bflb_ipc_list(BFLB_IPC_LIST_SUBMIT_OFF));
	bflb_ipc_list_init(bflb_ipc_list(BFLB_IPC_LIST_DONE_OFF));

	/* hostdesc.status_addr must be non-NULL: txu_cntrl_cfm dereferences
	 * it unconditionally.
	 */
	for (i = 0; i < BFLB_TXDESC_COUNT; i++) {
		volatile uint32_t *td = (volatile uint32_t *)bflb_ipc_txdesc(i);

		td[BFLB_TXDESC_WORD_STATUS_ADDR] = (uint32_t)(uintptr_t)&bflb_wifi_tx_status[i];
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

	/* The A2E lines (61-63) are attached by the blob's own intc_init and
	 * routed by the shim trampolines, so ipc_emb_tx_irq() and friends run
	 * from their own interrupts -- calling them here as well would run
	 * the FW event code twice and race that handler.
	 */
	bl_irq_handler();
	/* Only wake the sender.  The done queue is write-only for the
	 * firmware -- nothing in the blob ever pops it -- and draining it
	 * from here would race ipc_emb_txcfm() running on the FW thread,
	 * which this interrupt can preempt mid-push.
	 */
	if ((status & IPC_IRQ_E2A_TXCFM) != 0U) {
		k_sem_give(&bflb_tx_cfm_sem);
	}
}

/* On BL602 the A2E doorbell raises the shared IPC interrupt line, which
 * re-enters the ISR above and drives the blob-side IRQ chain -- no
 * extra wake needed.
 */
void bflb_wifi_ipc_msg_kick(void)
{
}

void bflb_wifi_ipc_mac_init_done(struct bflb_wifi_dev *d)
{
	ARG_UNUSED(d);
	k_timer_start(&bflb_rx_batch_timer, K_MSEC(BFLB_RX_BATCH_RESET_MS),
		      K_MSEC(BFLB_RX_BATCH_RESET_MS));
}

int bflb_wifi_tx_eth(const uint8_t *frame, uint16_t len, uint8_t vif_idx, uint8_t sta_idx)
{
	const struct bflb_eth_frame_hdr *eth = (const struct bflb_eth_frame_hdr *)frame;
	volatile uint8_t *td_raw;
	volatile uint32_t *td_words;
	uint32_t slot;
	unsigned int key;

	BUILD_ASSERT(offsetof(struct hostdesc, eth_dest_addr) == BFLB_HOSTDESC_OFF_ETH_DEST_ADDR,
		     "hostdesc.eth_dest_addr offset");
	BUILD_ASSERT(offsetof(struct hostdesc, ethertype) == BFLB_HOSTDESC_OFF_ETHERTYPE,
		     "hostdesc.ethertype offset");
	BUILD_ASSERT(offsetof(struct hostdesc, tid) == BFLB_HOSTDESC_OFF_TID,
		     "hostdesc.tid offset");
	BUILD_ASSERT(offsetof(struct hostdesc, vif_idx) == BFLB_HOSTDESC_OFF_VIF_IDX,
		     "hostdesc.vif_idx offset");
	BUILD_ASSERT(offsetof(struct hostdesc, vif_type) == BFLB_HOSTDESC_OFF_VIF_TYPE,
		     "hostdesc.vif_type offset");
	BUILD_ASSERT(offsetof(struct hostdesc, staid) == BFLB_HOSTDESC_OFF_STAID,
		     "hostdesc.staid offset");

	if ((frame == NULL) || (len < BFLB_WIFI_ETH_HDR_LEN) ||
	    (((uint32_t)len + BFLB_TX_HEADROOM) > BFLB_TXBUF_SZ)) {
		return -EINVAL;
	}

	k_mutex_lock(&bflb_tx_mutex, K_FOREVER);

	td_raw = bflb_tx_alloc_slot();
	if (td_raw == NULL) {
		LOG_WRN("tx: no free txdesc after %ums", BFLB_TX_FREE_WAIT_MAX_MS);
		k_mutex_unlock(&bflb_tx_mutex);
		return -ENOMEM;
	}

	slot = bflb_ipc_txdesc_idx(td_raw);
	bflb_tx_fill_hostdesc(td_raw, eth, frame, len, vif_idx, sta_idx);

	/* Hand the descriptor over the way ipc_host_txdesc_push() does: take
	 * it off list_free, mark it ready, put it on list_ongoing and ring
	 * the A2E doorbell.  The FW owns it from here -- it builds the
	 * 802.11 frame, encrypts with the key installed through
	 * bl_wifi_set_sta_key_internal() and arms the MAC queue itself.
	 */
	td_words = (volatile uint32_t *)td_raw;
	key = irq_lock();
	td_words[BFLB_TXDESC_WORD_READY] = BFLB_TXDESC_READY_FILLED;
	td_words[BFLB_TXDESC_WORD_HOST_ID] = (uint32_t)(uintptr_t)bflb_txbuf[slot];
	bflb_ipc_list_push_back(bflb_ipc_list(BFLB_IPC_LIST_SUBMIT_OFF),
				(volatile struct bflb_ipc_list_hdr *)td_words);
	irq_unlock(key);

	ipc_a2e_trigger(BIT(BFLB_IPC_A2E_TXDESC_FIRSTBIT));

	/* The doorbell raises the blob's A2E TX line, which the shim routes
	 * to ipc_emb_tx_irq() in interrupt context; wake the FW scheduler
	 * thread as well so it drains the event promptly.
	 */
	ipc_emb_notify();

	/* The frame is the firmware's once the doorbell is rung, so the send
	 * has succeeded.  Waiting for the confirmation only paces the next
	 * one against the descriptor pool: giving up on it is not a failure,
	 * and neither is a frame the peer did not acknowledge.
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

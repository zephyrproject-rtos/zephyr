/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL60x WiFi TX path.
 *
 * The wifi4 firmware drains TX work from txdesc_host descriptors in IPC
 * shared memory.  The host fills a free descriptor (one with `ready`
 * cleared by the FW), sets `ready`, then calls the FW's TX processing
 * chain directly so the MAC HW is armed synchronously.
 *
 * The blob's native data-TX path doesn't transmit in this setup (status
 * DESC_DONE_TX|RETRY_LIMIT_REACHED, nothing on air).  Workaround: the
 * txl_cntrl_push wrap redirects the FW-set level-1 hwdesc pointer to a
 * private mgmt-style hwdesc with a contiguous frame buffer and the
 * cafebabe cfm-magic MAC HW expects.  For encrypted frames the MAC HW
 * HW-CCMP path doesn't apply the per-STA key context through this
 * redirect, so the payload is SW CCMP-encrypted before DMA.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#include <lmac_types.h>
#include <bl60x_fw_api.h>
#include <lmac_mac.h>
#include <utils_list.h>
#include <ipc_compat.h>
#include <ipc_shared.h>

#include "bflb_wifi.h"
#include "bflb_wifi_ipc.h"
#include "bflb_wifi_ccmp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

extern void ipc_emb_notify(void);
extern void txu_cntrl_push(void *pad_txdesc, uint32_t arg1);
extern struct ipc_shared_env_tag ipc_shared_env;

/* txdesc word offsets indexed directly. */
#define BFLB_TXDESC_WORD_HOST_ID 1U
#define BFLB_TXDESC_WORD_READY   2U
#define BFLB_TXDESC_HOSTDESC_OFF 12U /* list_hdr(4) + host_id(4) + ready(4) */
#define BFLB_TXDESC_READY_FILLED 0xFFFFFFFFU
#define BFLB_TXDESC_READY_FREE   0U
#define BFLB_HOSTDESC_OFF_IN_PAD 4U /* txdesc_upper layout: co_list_hdr then hostdesc */

#define BFLB_PRIVATE_SLOT_CNT   2U
#define BFLB_PRIVATE_FRAME_SZ   1600U
#define BFLB_PRIVATE_BUFCTRL_SZ 60U
#define BFLB_SHAREDRAM_ALIGN    8U
#define BFLB_TXBUF_ALIGN        4U

/* Per-slot contiguous frame buffer for MAC HW DMA: the data descriptor's
 * THD-chain isn't honoured by the dispatcher, so a contiguous frame is
 * built here at TX time.
 */
static uint8_t bflb_private_frame[BFLB_PRIVATE_SLOT_CNT][BFLB_PRIVATE_FRAME_SZ] __aligned(
	BFLB_SHAREDRAM_ALIGN) Z_GENERIC_SECTION(SHAREDRAM);
/* Per-slot buffer_control_desc mirror for hwdesc[+10] -- the FW's embedded
 * copy lives in the IPC shared region the MAC HW DMA can't always reach
 * reliably for control reads.
 */
static uint8_t bflb_private_bufctrl[BFLB_PRIVATE_SLOT_CNT][BFLB_PRIVATE_BUFCTRL_SZ] __aligned(
	BFLB_SHAREDRAM_ALIGN) Z_GENERIC_SECTION(SHAREDRAM);

/* Per-frame staging buffer in WIFI_RAM.  The FW leaves a 48-byte headroom
 * before the ethernet header for the 802.11 MAC/QoS/SNAP/IV/MIC
 * encapsulation it prepends.  MAC HW DMA reads pbuf_chained_ptr[0] from
 * this region.
 */
#define BFLB_TX_HEADROOM   48U
#define BFLB_TX_PAYLOAD_SZ 1536U /* MTU + slack */
#define BFLB_TXBUF_SZ      (BFLB_TX_HEADROOM + BFLB_TX_PAYLOAD_SZ)

static uint8_t bflb_txbuf[BFLB_TXBUF_SZ] __aligned(BFLB_TXBUF_ALIGN) Z_GENERIC_SECTION(SHAREDRAM);

/* Host word the FW writes the TX status into via host->status_addr.
 * Bits: 31 DESC_DONE_TX, 30 DESC_DONE_SW_TX, 23 FRAME_SUCCESSFUL_TX.
 */
/* Status word the FW writes via host->status_addr; uint32_t is naturally
 * word-aligned, which is all the MAC HW DMA requires.
 */
volatile uint32_t bflb_wifi_tx_status;

/* Serialise TX -- single shared txbuf, multiple callers. */
static K_MUTEX_DEFINE(bflb_tx_mutex);

#define BFLB_TX_FREE_WAIT_MAX_MS  200U
#define BFLB_TX_FREE_WAIT_STEP_MS 1U
#define BFLB_TX_CFM_POLL_STEP_MS  2U
#define BFLB_TX_CFM_POLL_MAX_MS   200U

#define BFLB_TX_STATINFO_DONE    BIT(31)
#define BFLB_TX_STATINFO_SUCCESS BIT(23)

/* Inline hwdesc/cfm chain offsets within pad_txdesc, as set by
 * ipc_emb_tx_evt: pad[112] is the hwdesc pointer, an inline hwdesc lives
 * at pad+116 and its cfm at pad+188.
 */
#define BFLB_PAD_OFF_HWDESC_PTR    112U
#define BFLB_PAD_OFF_INLINE_HWDESC 116U
#define BFLB_PAD_OFF_INLINE_CFM    188U

/* packet_addr == this sentinel means "use chained-pbuf pointers". */
#define BFLB_HOSTDESC_PBUF_CHAINED_MAGIC 0x11111111U

#define BFLB_ETH_MAC_LEN  6U
#define BFLB_VIF_TYPE_STA 1U /* MM_STA per vendor bl_output */
#define BFLB_TID_BE       0U

struct bflb_eth_frame_hdr {
	uint8_t dst[BFLB_ETH_MAC_LEN];
	uint8_t src[BFLB_ETH_MAC_LEN];
	uint16_t etype_be;
} __packed;

/* Find a free txdesc slot (ready==0).  Both descriptors can be in flight;
 * wait briefly for the FW to clear `ready` rather than dropping the frame
 * (losing EAPOL msg 4 times out the 4-way handshake).
 */
static volatile uint8_t *bflb_tx_alloc_slot(void)
{
	for (uint32_t wait = 0; wait < BFLB_TX_FREE_WAIT_MAX_MS; wait++) {
		for (uint32_t i = 0; i < BFLB_WIFI_TXDESC_COUNT; i++) {
			volatile uint32_t *td = (volatile uint32_t *)bflb_wifi_ipc_txdesc(i);

			if (td[BFLB_TXDESC_WORD_READY] == BFLB_TXDESC_READY_FREE) {
				return (volatile uint8_t *)td;
			}
		}
		k_msleep(BFLB_TX_FREE_WAIT_STEP_MS);
	}
	return NULL;
}

static void bflb_tx_fill_hostdesc(volatile uint8_t *td_raw, const struct bflb_eth_frame_hdr *eth,
				  const uint8_t *frame, uint16_t len, uint8_t vif_idx,
				  uint8_t sta_idx)
{
	struct hostdesc *host;
	volatile uint32_t *td_words;
	uint16_t payload_len = len - BFLB_WIFI_ETH_HDR_LEN;

	/* The blob walks fields beyond struct hostdesc, so zero the entire
	 * pad_txdesc area, not just sizeof(struct hostdesc).
	 */
	memset((void *)(uintptr_t)(td_raw + BFLB_TXDESC_HOSTDESC_OFF), 0,
	       BFLB_WIFI_TXDESC_STRIDE - BFLB_TXDESC_HOSTDESC_OFF);

	memset(bflb_txbuf, 0, BFLB_TX_HEADROOM);
	memcpy(bflb_txbuf + BFLB_TX_HEADROOM, frame, len);

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
	host->pbuf_addr = (uint32_t)(uintptr_t)bflb_txbuf;
	host->pbuf_chained_ptr[0] =
		(uint32_t)(uintptr_t)(bflb_txbuf + BFLB_TX_HEADROOM + BFLB_WIFI_ETH_HDR_LEN);
	host->pbuf_chained_len[0] = payload_len;
	bflb_wifi_tx_status = 0;
	host->status_addr = (uint32_t)(uintptr_t)&bflb_wifi_tx_status;

	td_words = (volatile uint32_t *)td_raw;
	td_words[BFLB_TXDESC_WORD_HOST_ID] = (uint32_t)(uintptr_t)bflb_txbuf;
	td_words[BFLB_TXDESC_WORD_READY] = BFLB_TXDESC_READY_FILLED;
}

int bflb_wifi_tx_eth(const uint8_t *frame, uint16_t len, uint8_t vif_idx, uint8_t sta_idx)
{
	const struct bflb_eth_frame_hdr *eth = (const struct bflb_eth_frame_hdr *)frame;
	volatile uint8_t *td_raw;
	volatile uint32_t *td_words;
	uint8_t *pad;
	bool done = false;

	BUILD_ASSERT(offsetof(struct hostdesc, eth_dest_addr) == 16,
		     "hostdesc.eth_dest_addr offset");
	BUILD_ASSERT(offsetof(struct hostdesc, ethertype) == 28, "hostdesc.ethertype offset");
	BUILD_ASSERT(offsetof(struct hostdesc, tid) == 42, "hostdesc.tid offset");
	BUILD_ASSERT(offsetof(struct hostdesc, vif_idx) == 43, "hostdesc.vif_idx offset");
	BUILD_ASSERT(offsetof(struct hostdesc, vif_type) == 44, "hostdesc.vif_type offset");
	BUILD_ASSERT(offsetof(struct hostdesc, staid) == 45, "hostdesc.staid offset");

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

	bflb_tx_fill_hostdesc(td_raw, eth, frame, len, vif_idx, sta_idx);

	/* Replicate ipc_emb_tx_evt's inline hwdesc/cfm chain setup. */
	pad = (uint8_t *)(uintptr_t)td_raw + BFLB_TXDESC_HOSTDESC_OFF;
	*(uint32_t *)(pad + BFLB_PAD_OFF_INLINE_HWDESC) =
		(uint32_t)(uintptr_t)(pad + BFLB_PAD_OFF_INLINE_CFM);
	*(uint32_t *)(pad + BFLB_PAD_OFF_HWDESC_PTR) =
		(uint32_t)(uintptr_t)(pad + BFLB_PAD_OFF_INLINE_HWDESC);

	sys_cache_data_flush_all();

	/* Run the FW's TX chain synchronously in this thread:
	 * txu_cntrl_push -> __wrap_txl_cntrl_push -> fixup -> arm AC0.
	 */
	txu_cntrl_push(pad, 0U);

	for (uint32_t poll = 0; poll < BFLB_TX_CFM_POLL_MAX_MS; poll += BFLB_TX_CFM_POLL_STEP_MS) {
		if ((bflb_wifi_tx_status & BFLB_TX_STATINFO_DONE) != 0U) {
			done = true;
			break;
		}
		k_msleep(BFLB_TX_CFM_POLL_STEP_MS);
	}

	td_words = (volatile uint32_t *)td_raw;
	if (!done) {
		LOG_DBG("tx: cfm timeout %ums tx_status=0x%08x", BFLB_TX_CFM_POLL_MAX_MS,
			bflb_wifi_tx_status);
	}

	td_words[BFLB_TXDESC_WORD_READY] = BFLB_TXDESC_READY_FREE;

	/* Wake the FW thread so it can recycle hwdesc/BD entries. */
	ipc_emb_notify();

	k_mutex_unlock(&bflb_tx_mutex);
	return 0;
}

/* txl_cntrl_push wrap: descriptor fixup + MAC HW arming. */

#define BFLB_TXDESC_OFF_IN_SHARED       516U
#define BFLB_HOSTDESC_PAD_OFF           12U
#define BFLB_HOSTDESC_OFF_PAD_MAC_TOTAL 98U
#define BFLB_HOSTDESC_OFF_PAD_SEC_HDR   100U
#define BFLB_HOSTDESC_OFF_PAD_ETHERTYPE 32U /* pad+4+hostdesc.ethertype */
#define BFLB_HOSTDESC_OFF_PAD_PBUF_PTR  52U
#define BFLB_HOSTDESC_OFF_PAD_PBUF_LEN  68U
#define BFLB_PAD_HDR_BUF_OFF            556U /* buf_end-mac_total */

#define BFLB_HWDESC_MAGIC             0xCAFEBABEU
#define BFLB_BUFCTRL_MAGIC            0xBADCAB1EU
#define BFLB_MACCTRL2_ENCRYPT_BIT     0x00002000U
#define BFLB_HWDESC_TXBC_RATE_DEFAULT 0xFFFF0704U

#define BFLB_HWDESC_WORD_MAGIC    1U
#define BFLB_HWDESC_WORD_THD_HEAD 4U
#define BFLB_HWDESC_WORD_BUFCTRL  10U
#define BFLB_HWDESC_WORD_MACCTRL2 14U

#define BFLB_DMA_BD_WORD_MAGIC   0U
#define BFLB_DMA_BD_WORD_NEXT    1U
#define BFLB_DMA_BD_WORD_START   2U
#define BFLB_DMA_BD_WORD_END     3U
#define BFLB_DMA_BD_WORD_STATUS  4U
#define BFLB_DMA_BD_MAGIC        0xCAFEFADEU
#define BFLB_DMA_BD_STATUS_READY 0x80000000U

#define BFLB_MAC_REG_BASE        0x44B08000U
#define BFLB_MAC_REG_TRIGGER     (BFLB_MAC_REG_BASE + 0x180U)
#define BFLB_MAC_REG_AC0_HEAD    (BFLB_MAC_REG_BASE + 0x19CU)
#define BFLB_MAC_TRIGGER_AC0     0x00000200U
#define BFLB_HWDESC_THD_BYTE_OFF 4U

#define BFLB_FCS_LEN             4U
#define BFLB_802_11_QOS_FC_MASK  0xFCU
#define BFLB_802_11_QOS_FC_VALUE 0x88U
#define BFLB_802_11_QOS_TID_OFF  24U
#define BFLB_802_11_QOS_TID_MASK 0x0FU
#define BFLB_CCMP_IV_LEN         8U
#define BFLB_LLC_LEN             8U
#define BFLB_LLC_DSAP_SNAP       0xAAU
#define BFLB_LLC_SSAP_SNAP       0xAAU
#define BFLB_LLC_CTRL_UNNUMBERED 0x03U
/* Smallest QoS-data MAC header (26) + CCMP IV (8) + LLC SNAP (8). */
#define BFLB_MIN_ENCRYPTED_FRAME 34U
#define BFLB_PLAIN_BUF_SZ        1518U

#define BFLB_802_11_HDR_A3_OFF      16U /* A3 = DA in ToDS=1 frame */
#define BFLB_FC_BYTE1_PROTECTED_BIT 0x40U
#define BFLB_FC1_POWER_MGMT         0x10U
#define BFLB_ETH_GROUP_ADDR_BIT     0x01U

extern void __real_txl_cntrl_push(void *pad_txdesc, uint32_t arg1);

/* The blob TX pipeline uses two slots whose pad_txdesc start addresses are
 * one stride apart; pick the matching private hwdesc.
 */
static int bflb_pad_to_idx(const void *pad_txdesc)
{
	const uint8_t *base = (const uint8_t *)&ipc_shared_env + BFLB_TXDESC_OFF_IN_SHARED;
	int diff = (int)((const uint8_t *)pad_txdesc - BFLB_HOSTDESC_PAD_OFF - base);

	return (diff >= (int)BFLB_WIFI_TXDESC_STRIDE) ? 1 : 0;
}

/* Snapshot a stable bufctrl with the default rate-retry chain into
 * WIFI_RAM and point hwdesc[+10] at it -- the FW recycles its own pool
 * entries and MAC HW silently drops longer frames without these fields.
 */
static void bflb_tx_setup_bufctrl(int idx, volatile uint32_t *hw)
{
	volatile uint32_t *bc = (volatile uint32_t *)bflb_private_bufctrl[idx];

	memset((void *)(uintptr_t)bc, 0, BFLB_PRIVATE_BUFCTRL_SZ);
	bc[0] = BFLB_BUFCTRL_MAGIC;
	bc[2] = 0x00000001U;
	bc[4] = BFLB_HWDESC_TXBC_RATE_DEFAULT;

	hw[BFLB_HWDESC_WORD_BUFCTRL] = (uint32_t)bc;
}

static void bflb_make_llc_snap(uint8_t llc[BFLB_LLC_LEN], uint16_t eth_type)
{
	llc[0] = BFLB_LLC_DSAP_SNAP;
	llc[1] = BFLB_LLC_SSAP_SNAP;
	llc[2] = BFLB_LLC_CTRL_UNNUMBERED;
	llc[3] = 0;
	llc[4] = 0;
	llc[5] = 0;
	llc[6] = (uint8_t)(eth_type >> 8);
	llc[7] = (uint8_t)eth_type;
}

/* SW CCMP encrypt path.  Layout at pad+556 after frame_build:
 *   [+0..+25]   MAC hdr (FC has Protected=1, includes QoS Control byte)
 *   [+26..+33]  CCMP IV slot (FW-prefilled with PN counter)
 *   [+34..+41]  LLC SNAP (not reliable in all FW paths; rebuilt here)
 * Payload comes via pbuf_chained_ptr[0].
 */
/* Plaintext staging off-stack: TX runs in arbitrary caller context (net
 * TX, net_mgmt, supplicant) where 1.5 KB of stack is not guaranteed.
 * Serialized by the tx mutex held across txu_cntrl_push.
 */
static uint8_t ccmp_plain[BFLB_PLAIN_BUF_SZ];

static int bflb_tx_ccmp_encrypt(void *pad_txdesc, const uint8_t *hdr_src, uint8_t mac_total,
				uint16_t eth_type, uint32_t payload_addr, uint16_t payload_len,
				uint8_t *frame)
{
	uint8_t mac_hdr_only = mac_total - BFLB_CCMP_IV_LEN - BFLB_LLC_LEN;
	uint8_t *plain = ccmp_plain;
	uint8_t llc[BFLB_LLC_LEN];
	size_t plain_len = (size_t)BFLB_LLC_LEN + payload_len;
	int qos_tid;
	int n;

	if (plain_len > sizeof(ccmp_plain)) {
		return -EMSGSIZE;
	}

	/* Build LLC/SNAP from hostdesc.ethertype instead of copying the FW
	 * pad -- those bytes are not stable for every frame type.
	 */
	bflb_make_llc_snap(llc, eth_type);
	memcpy(plain, llc, sizeof(llc));
	memcpy(plain + BFLB_LLC_LEN, (const void *)(uintptr_t)payload_addr, payload_len);

	if ((hdr_src[0] & BFLB_802_11_QOS_FC_MASK) == BFLB_802_11_QOS_FC_VALUE) {
		qos_tid = hdr_src[BFLB_802_11_QOS_TID_OFF] & BFLB_802_11_QOS_TID_MASK;
	} else {
		qos_tid = -1;
	}

	n = bflb_wifi_ccmp_encrypt(frame, BFLB_PRIVATE_FRAME_SZ - BFLB_FCS_LEN, hdr_src,
				   mac_hdr_only, plain, plain_len, qos_tid);
	if (n <= 0) {
		return -EIO;
	}

	/* Tell MAC HW the descriptor has no security header -- already
	 * SW-encrypted.  Leave pad+98 (mac_total) untouched: the FW computed
	 * downstream lengths from the original values.
	 */
	((uint8_t *)pad_txdesc)[BFLB_HOSTDESC_OFF_PAD_SEC_HDR] = 0;

	memset(frame + n, 0, BFLB_FCS_LEN);
	return n;
}

static uint16_t bflb_hostdesc_eth_type(const uint8_t *pad_txdesc)
{
	return ((uint16_t)pad_txdesc[BFLB_HOSTDESC_OFF_PAD_ETHERTYPE] << 8) |
	       pad_txdesc[BFLB_HOSTDESC_OFF_PAD_ETHERTYPE + 1U];
}

static void bflb_tx_fixup_bd_chain(volatile uint32_t *hw, const uint8_t *frame,
				   uint32_t frame_total, uint8_t orig_mac_total)
{
	uint32_t dma_len = frame_total - BFLB_FCS_LEN;
	volatile uint32_t *bd1 = (volatile uint32_t *)(uintptr_t)hw[BFLB_HWDESC_WORD_THD_HEAD];
	uint32_t bd1_len;
	uint32_t bd2_ptr;

	if ((bd1 == NULL) || (bd1[BFLB_DMA_BD_WORD_MAGIC] != BFLB_DMA_BD_MAGIC)) {
		return;
	}

	bd1_len = orig_mac_total;
	bd2_ptr = bd1[BFLB_DMA_BD_WORD_NEXT];

	bd1[BFLB_DMA_BD_WORD_START] = (uint32_t)frame;
	bd1[BFLB_DMA_BD_WORD_STATUS] = BFLB_DMA_BD_STATUS_READY;

	if (bd2_ptr != 0U) {
		volatile uint32_t *bd2 = (volatile uint32_t *)(uintptr_t)bd2_ptr;

		if (bd2[BFLB_DMA_BD_WORD_MAGIC] == BFLB_DMA_BD_MAGIC) {
			bd1[BFLB_DMA_BD_WORD_END] = (uint32_t)(frame + bd1_len - 1U);
			bd2[BFLB_DMA_BD_WORD_START] = (uint32_t)(frame + bd1_len);
			bd2[BFLB_DMA_BD_WORD_END] = (uint32_t)(frame + dma_len - 1U);
			bd2[BFLB_DMA_BD_WORD_STATUS] = BFLB_DMA_BD_STATUS_READY;
		} else {
			bd1[BFLB_DMA_BD_WORD_END] = (uint32_t)(frame + dma_len - 1U);
		}
	} else {
		bd1[BFLB_DMA_BD_WORD_END] = (uint32_t)(frame + dma_len - 1U);
	}
}

static void bflb_tx_fixup(void *pad_txdesc, volatile uint32_t *hw)
{
	int idx = bflb_pad_to_idx(pad_txdesc);
	uint8_t *frame = bflb_private_frame[idx];
	const uint8_t *pad_b = (const uint8_t *)pad_txdesc;
	uint8_t mac_total = pad_b[BFLB_HOSTDESC_OFF_PAD_MAC_TOTAL];
	uint8_t sec_hdr_len = pad_b[BFLB_HOSTDESC_OFF_PAD_SEC_HDR];
	uint8_t orig_mac_total = mac_total;
	uint8_t *hdr_src = (uint8_t *)pad_txdesc + BFLB_PAD_HDR_BUF_OFF;
	uint32_t payload_addr = *(const uint32_t *)(pad_b + BFLB_HOSTDESC_OFF_PAD_PBUF_PTR);
	uint16_t payload_len = *(const uint16_t *)(pad_b + BFLB_HOSTDESC_OFF_PAD_PBUF_LEN);
	uint32_t frame_total = mac_total + payload_len;
	uint16_t eth_type = bflb_hostdesc_eth_type(pad_b);
	bool ccmp_done = false;
	bool force_sw_ccmp;

	hdr_src[1] &= (uint8_t)~BFLB_FC1_POWER_MGMT;

	force_sw_ccmp = (mac_total >= BFLB_MIN_ENCRYPTED_FRAME) &&
			((hdr_src[1] & BFLB_FC_BYTE1_PROTECTED_BIT) != 0U) &&
			((hdr_src[BFLB_802_11_HDR_A3_OFF] & BFLB_ETH_GROUP_ADDR_BIT) != 0U);

	if (force_sw_ccmp && (sec_hdr_len == 0U)) {
		sec_hdr_len = BFLB_CCMP_IV_LEN;
		mac_total += BFLB_CCMP_IV_LEN;
	}

	if ((sec_hdr_len > 0U) && (mac_total >= BFLB_MIN_ENCRYPTED_FRAME)) {
		int n = bflb_tx_ccmp_encrypt(pad_txdesc, hdr_src, mac_total, eth_type, payload_addr,
					     payload_len, frame);
		if (n > 0) {
			frame_total = (uint32_t)n + BFLB_FCS_LEN;
			ccmp_done = true;
		} else {
			LOG_WRN("tx ccmp et=%04x failed: %d", eth_type, n);
		}
	}

	if (!ccmp_done) {
		if ((frame_total + BFLB_FCS_LEN) > BFLB_PRIVATE_FRAME_SZ) {
			return;
		}
		memcpy(frame, hdr_src, mac_total);
		memcpy(frame + mac_total, (const void *)(uintptr_t)payload_addr, payload_len);
		memset(frame + frame_total, 0, BFLB_FCS_LEN);
		frame_total += BFLB_FCS_LEN;
	}

	/* Point the BD chain at the private frame buffer.  The FW's
	 * completion handler only recycles BD structs by address -- it
	 * doesn't touch the data pointers.
	 */
	bflb_tx_fixup_bd_chain(hw, frame, frame_total, orig_mac_total);

	if (ccmp_done) {
		hw[BFLB_HWDESC_WORD_MACCTRL2] &= ~BFLB_MACCTRL2_ENCRYPT_BIT;
	}

	bflb_tx_setup_bufctrl(idx, hw);
}

static void bflb_tx_arm_ac0(volatile uint32_t *hw)
{
	volatile uint32_t *ac0_head = (volatile uint32_t *)(uintptr_t)BFLB_MAC_REG_AC0_HEAD;
	volatile uint32_t *trigger = (volatile uint32_t *)(uintptr_t)BFLB_MAC_REG_TRIGGER;

	*ac0_head = (uint32_t)hw + BFLB_HWDESC_THD_BYTE_OFF;
	*trigger = BFLB_MAC_TRIGGER_AC0;
}

void __wrap_txl_cntrl_push(void *pad_txdesc, uint32_t arg1)
{
	volatile uint32_t *hw;
	volatile uint32_t *pad_hwdesc =
		(volatile uint32_t *)((uint8_t *)pad_txdesc + BFLB_PAD_OFF_HWDESC_PTR);

	__real_txl_cntrl_push(pad_txdesc, arg1);

	hw = (volatile uint32_t *)(uintptr_t)*pad_hwdesc;
	if ((hw == NULL) || (hw[BFLB_HWDESC_WORD_MAGIC] != BFLB_HWDESC_MAGIC)) {
		return;
	}

	bflb_tx_fixup(pad_txdesc, hw);
	bflb_tx_arm_ac0(hw);
}

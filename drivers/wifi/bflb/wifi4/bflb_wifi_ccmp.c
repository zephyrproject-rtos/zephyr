/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * CCMP encrypt for software-driven 802.11 data TX.
 *
 * The MAC HW HW-CCMP path doesn't engage when the TX descriptor wrap
 * (bflb_wifi_tx.c) redirects MAC HW DMA to a private contiguous buffer --
 * frames go on air as plaintext and the AP rejects them.  The host
 * encrypts the LLC + payload here using AES-CCM with the PTK's TK before
 * placing the frame in the buffer.  IEEE 802.11-2020 Section 12.5.3.
 *
 * Uses a persistent mbedtls_ccm_context keyed once per connection -- the
 * PSA one-shot AEAD path allocates a cipher context from the (small)
 * libc arena on every call, which eventually fails mid-traffic.
 */

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <mbedtls/private/ccm.h>

#include "bflb_wifi_ccmp.h"

LOG_MODULE_DECLARE(bflb_wifi, CONFIG_WIFI_LOG_LEVEL);

#define CCMP_TK_LEN      16U
#define CCMP_IV_LEN      8U
#define CCMP_MIC_LEN     8U
#define CCMP_NONCE_LEN   13U
#define CCMP_AAD_MAX     32U
#define CCMP_KEY_BITS    128U
#define CCMP_KEYID_EXTIV 0x20U /* ExtIV=1, KeyID=0 in CCMP IV byte 3 */

#define FC_PROTECTED_BIT    0x40U
#define FC_TODS_BIT         0x01U
#define FC_FROMDS_BIT       0x02U
#define FC_TODS_FROMDS_MASK (FC_TODS_BIT | FC_FROMDS_BIT)
#define FC_AAD_BYTE0_MASK   0xCFU /* clear Subtype bits for non-QoS */
#define FC_AAD_BYTE1_MASK   0x47U /* keep ToDS/FromDS/MoreFrag/Protected */
#define QOS_TID_MASK        0x0FU
#define SEQ_FRAG_MASK       0x0FU

#define MAC_HDR_A1_OFF 4U
#define MAC_HDR_A2_OFF 10U
#define MAC_HDR_SC_OFF 22U
#define MAC_HDR_A4_OFF 24U
#define MAC_ADDR_LEN   6U

/* Monotonic per-key PN; the AP uses the receive PN to reject replays.
 * Start at 1 (PN=0 is reserved for IGTK).
 */
static uint64_t ccmp_tx_pn = 1;
static mbedtls_ccm_context ccmp_ctx;
static bool ccmp_ready;
static K_MUTEX_DEFINE(ccmp_lock);

static void ccmp_make_nonce(uint8_t nonce[CCMP_NONCE_LEN], uint8_t prio,
			    const uint8_t addr2[MAC_ADDR_LEN], uint64_t pn);
static size_t ccmp_make_aad(uint8_t aad[CCMP_AAD_MAX], const uint8_t *mac_hdr, bool is_qos,
			    bool has_a4, uint8_t qos_tid);

/* CCMP nonce (IEEE 802.11-2020 Section 12.5.3.3.4): priority byte, A2, PN5..PN0. */
static void ccmp_make_nonce(uint8_t nonce[CCMP_NONCE_LEN], uint8_t prio,
			    const uint8_t addr2[MAC_ADDR_LEN], uint64_t pn)
{
	nonce[0] = prio & QOS_TID_MASK;
	memcpy(&nonce[1], addr2, MAC_ADDR_LEN);
	nonce[7] = (uint8_t)(pn >> 40);
	nonce[8] = (uint8_t)(pn >> 32);
	nonce[9] = (uint8_t)(pn >> 24);
	nonce[10] = (uint8_t)(pn >> 16);
	nonce[11] = (uint8_t)(pn >> 8);
	nonce[12] = (uint8_t)(pn);
}

/* AAD per IEEE 802.11-2020 Section 12.5.3.3.3.  Returns AAD length. */
static size_t ccmp_make_aad(uint8_t aad[CCMP_AAD_MAX], const uint8_t *mac_hdr, bool is_qos,
			    bool has_a4, uint8_t qos_tid)
{
	size_t pos;

	aad[0] = mac_hdr[0];
	if (!is_qos) {
		aad[0] &= FC_AAD_BYTE0_MASK;
	}
	aad[1] = mac_hdr[1] & FC_AAD_BYTE1_MASK;
	memcpy(&aad[2], &mac_hdr[MAC_HDR_A1_OFF], 3U * MAC_ADDR_LEN);
	aad[20] = mac_hdr[MAC_HDR_SC_OFF] & SEQ_FRAG_MASK;
	aad[21] = 0;
	pos = 22;
	if (has_a4) {
		memcpy(&aad[pos], &mac_hdr[MAC_HDR_A4_OFF], MAC_ADDR_LEN);
		pos += MAC_ADDR_LEN;
	}
	if (is_qos) {
		aad[pos] = qos_tid & QOS_TID_MASK;
		aad[pos + 1] = 0;
		pos += 2;
	}
	return pos;
}

int bflb_wifi_ccmp_set_key(const uint8_t *tk)
{
	int rc;

	k_mutex_lock(&ccmp_lock, K_FOREVER);

	if (ccmp_ready) {
		mbedtls_ccm_free(&ccmp_ctx);
		ccmp_ready = false;
	}

	mbedtls_ccm_init(&ccmp_ctx);
	rc = mbedtls_ccm_setkey(&ccmp_ctx, MBEDTLS_CIPHER_ID_AES, tk, CCMP_KEY_BITS);
	if (rc != 0) {
		LOG_ERR("CCM setkey failed: -0x%04x", -rc);
		mbedtls_ccm_free(&ccmp_ctx);
		k_mutex_unlock(&ccmp_lock);
		return -EIO;
	}

	ccmp_tx_pn = 1;
	ccmp_ready = true;
	k_mutex_unlock(&ccmp_lock);
	return 0;
}

void bflb_wifi_ccmp_clear(void)
{
	k_mutex_lock(&ccmp_lock, K_FOREVER);
	if (ccmp_ready) {
		mbedtls_ccm_free(&ccmp_ctx);
		ccmp_ready = false;
	}
	ccmp_tx_pn = 1;
	k_mutex_unlock(&ccmp_lock);
}

/* Encrypt plaintext (LLC + payload) with CCMP.  Output layout:
 *   [mac_hdr_len] MAC header (Protected bit set here)
 *   [8]           CCMP IV header (PN0,PN1,0,KeyID|ExtIV,PN2..PN5)
 *   [plain_len]   ciphertext
 *   [8]           MIC (CCM tag, 8-byte truncated)
 * qos_tid is the TID for QoS data frames, -1 for non-QoS frames.
 * Returns total bytes written, or -errno.
 */
int bflb_wifi_ccmp_encrypt(uint8_t *out, size_t out_size, const uint8_t *mac_hdr,
			   size_t mac_hdr_len, const uint8_t *plain, size_t plain_len, int qos_tid)
{
	bool is_qos = (qos_tid >= 0);
	uint8_t tid = is_qos ? (uint8_t)qos_tid : 0U;
	uint8_t nonce[CCMP_NONCE_LEN];
	uint8_t aad[CCMP_AAD_MAX];
	const uint8_t *addr2;
	uint8_t *iv;
	uint8_t *cipher;
	size_t total;
	size_t aad_len;
	uint64_t pn;
	bool has_a4;
	int rc;

	total = mac_hdr_len + CCMP_IV_LEN + plain_len + CCMP_MIC_LEN;
	if (out_size < total) {
		return -ENOSPC;
	}

	k_mutex_lock(&ccmp_lock, K_FOREVER);
	if (!ccmp_ready) {
		k_mutex_unlock(&ccmp_lock);
		return -EINVAL;
	}

	pn = ++ccmp_tx_pn;

	memcpy(out, mac_hdr, mac_hdr_len);
	out[1] |= FC_PROTECTED_BIT;

	iv = out + mac_hdr_len;
	iv[0] = (uint8_t)(pn >> 0);
	iv[1] = (uint8_t)(pn >> 8);
	iv[2] = 0;
	iv[3] = CCMP_KEYID_EXTIV;
	iv[4] = (uint8_t)(pn >> 16);
	iv[5] = (uint8_t)(pn >> 24);
	iv[6] = (uint8_t)(pn >> 32);
	iv[7] = (uint8_t)(pn >> 40);

	addr2 = &mac_hdr[MAC_HDR_A2_OFF];
	has_a4 = ((mac_hdr[1] & FC_TODS_FROMDS_MASK) == FC_TODS_FROMDS_MASK);

	ccmp_make_nonce(nonce, tid, addr2, pn);
	aad_len = ccmp_make_aad(aad, mac_hdr, is_qos, has_a4, tid);

	cipher = out + mac_hdr_len + CCMP_IV_LEN;
	rc = mbedtls_ccm_encrypt_and_tag(&ccmp_ctx, plain_len, nonce, sizeof(nonce), aad, aad_len,
					 plain, cipher, cipher + plain_len, CCMP_MIC_LEN);
	k_mutex_unlock(&ccmp_lock);

	if (rc != 0) {
		LOG_ERR("CCM encrypt failed: -0x%04x", -rc);
		return -EIO;
	}

	return (int)total;
}

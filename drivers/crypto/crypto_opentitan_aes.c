/*
 * Copyright (c) 2026, Bana Tawalbeh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lowrisc_opentitan_aes

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/crypto/cipher.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(opentitan_aes, CONFIG_CRYPTO_LOG_LEVEL);

/* Base Offsets for Registers (Keys, IV, Data) */
#define AES_KEY_SHARE0_0_REG_OFFSET 0x4
#define AES_KEY_SHARE1_0_REG_OFFSET 0x24
#define AES_IV_0_REG_OFFSET         0x44
#define AES_DATA_IN_0_REG_OFFSET    0x54
#define AES_DATA_OUT_0_REG_OFFSET   0x64

/* Control Register & Bitmasks */
#define AES_CTRL_SHADOWED_REG_OFFSET              0x74
#define AES_CTRL_SHADOWED_OPERATION_OFFSET        0
#define AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENC 0x1
#define AES_CTRL_SHADOWED_OPERATION_VALUE_AES_DEC 0x2

#define AES_CTRL_SHADOWED_MODE_OFFSET        2
#define AES_CTRL_SHADOWED_MODE_VALUE_AES_ECB 0x1
#define AES_CTRL_SHADOWED_MODE_VALUE_AES_CBC 0x2
#define AES_CTRL_SHADOWED_MODE_VALUE_AES_CTR 0x10

#define AES_CTRL_SHADOWED_KEY_LEN_OFFSET        8
#define AES_CTRL_SHADOWED_KEY_LEN_VALUE_AES_128 0x1
#define AES_CTRL_SHADOWED_KEY_LEN_VALUE_AES_256 0x4
/* AES-192 is intentionally not supported. */

#define AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT 15

/*
 * CTRL_SHADOWED bit layout:
 *   [1:0]  OPERATION  (1 = encrypt, 2 = decrypt)
 *   [7:2]  MODE
 *   [11:8] KEY_LEN    (sess->reg_ctrl_key_len is pre-shifted)
 *   [15]   MANUAL_OP  (0 = autostart enabled)
 */

/* Trigger Register */
#define AES_TRIGGER_REG_OFFSET               0x80
#define AES_TRIGGER_START_BIT                0
#define AES_TRIGGER_KEY_IV_DATA_IN_CLEAR_BIT 1
#define AES_TRIGGER_DATA_OUT_CLEAR_BIT       2

/* Status Register */
#define AES_STATUS_REG_OFFSET       0x84
#define AES_STATUS_IDLE_BIT         0
#define AES_STATUS_OUTPUT_VALID_BIT 3
#define AES_STATUS_INPUT_READY_BIT  4

struct opentitan_aes_config {
	mm_reg_t base_addr;
};

struct opentitan_aes_session {
	bool in_use;
	const struct device *dev;
	enum cipher_op dir;        /* CRYPTO_CIPHER_OP_ENCRYPT / _DECRYPT */
	uint32_t reg_ctrl_key_len; /* KEY_LEN field, pre-shifted for CTRL */
	uint32_t key_words_count;  /* 4 = AES-128, 8 = AES-256 */
	uint32_t key_words[8];     /* up to a 256-bit key */
};

struct opentitan_aes_data {
	/* Serializes AES hardware access; the driver polls STATUS (no IRQ line). */
	struct k_mutex lock;
	struct opentitan_aes_session sessions[CONFIG_CRYPTO_OPENTITAN_MAX_SESSION];
};

#define AES_TIMEOUT_US 10000

static int poll_idle(mm_reg_t base)
{
	uint32_t t = 0;

	while (!(sys_read32(base + AES_STATUS_REG_OFFSET) & (1u << AES_STATUS_IDLE_BIT))) {
		if (t++ > (AES_TIMEOUT_US / 10)) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10);
	}
	return 0;
}

/*
 * Hardware-level flush.
 * - Clears keys, IVs, and data registers and returns hardware to IDLE.
 * - Called at boot and during session teardown to prevent key leakage.
 */
static int opentitan_aes_hw_flush(mm_reg_t base)
{
	int ret;

	/* Wait for IDLE before touching Control Register */
	/* Writes to Control Register are silently ignored if hardware is not idle */
	ret = poll_idle(base);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Set MANUAL_OPERATION=1 to disable autostart the enc/dec. This is
	 * required for a successful flush (see OpenTitan AES HW docs).
	 * The MANUAL_OPERATION bit will be cleared by begin_session() because
	 * normal encryption/decryption relies on autostart. We only need to
	 * disable it for the flush operation at boot and during session teardown.
	 */
	uint32_t ctrl_val = (1u << AES_CTRL_SHADOWED_MANUAL_OPERATION_BIT);
	/* Double-write required for all shadowed registers. */
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);

	/* Re-poll IDLE: a CTRL write may trigger an internal PRNG reseed. */
	ret = poll_idle(base);
	if (ret != 0) {
		return ret;
	}

	/* Trigger hardware clear of keys, IVs, and all data registers */
	sys_write32((1 << AES_TRIGGER_KEY_IV_DATA_IN_CLEAR_BIT) |
			    (1 << AES_TRIGGER_DATA_OUT_CLEAR_BIT),
		    base + AES_TRIGGER_REG_OFFSET);

	/*
	 * Wait for IDLE after clear completes. The hardware clear takes several
	 * clock cycles to overwrite all registers with PRNG-generated data.
	 */
	return poll_idle(base);
}

/* Zephyr device initialization callback */
static int opentitan_aes_init(const struct device *dev)
{
	struct opentitan_aes_data *data = dev->data;
	const struct opentitan_aes_config *cfg = dev->config;

	k_mutex_init(&data->lock);

	/* Clear all software session slots */
	for (uint32_t i = 0; i < CONFIG_CRYPTO_OPENTITAN_MAX_SESSION; i++) {
		data->sessions[i].in_use = false;
		memset(data->sessions[i].key_words, 0, sizeof(data->sessions[i].key_words));
	}

	/* Flush hardware state at boot */
	int ret = opentitan_aes_hw_flush(cfg->base_addr);

	if (ret != 0) {
		LOG_ERR("OpenTitan AES hardware flush failed (timed out)");
	} else {
		LOG_INF("OpenTitan AES driver initialized.");
	}

	return ret;
}

/*
 * The effective key is SHARE0 XOR SHARE1. All 8 slots of each share must be
 * written; key_word_count (4 or 8) selects how many hold real key material,
 * the rest are zero-padded.
 */
static void aes_write_key(mm_reg_t base, const uint32_t *key_words,
			  uint32_t key_word_count)
{
	for (uint32_t i = 0; i < 8; i++) {
		uint32_t share0_word = 0u;

		if (i < key_word_count) {
			share0_word = key_words[i];
			LOG_DBG("KEY_SHARE0_%d = 0x%08x", i, share0_word);
		}

		sys_write32(share0_word, base + AES_KEY_SHARE0_0_REG_OFFSET + i * 4);
	}

	/* SHARE1 is always zero: side-channel masking is not implemented. */
	for (uint32_t i = 0; i < 8; i++) {
		sys_write32(0u, base + AES_KEY_SHARE1_0_REG_OFFSET + i * 4);
	}
}

static void aes_write_iv(mm_reg_t base, const uint32_t *iv_words)
{
	for (uint32_t i = 0; i < 4; i++) { /* IV is always 128 bits */
		sys_write32(iv_words[i], base + AES_IV_0_REG_OFFSET + i * 4);
	}
}

/*
 * Write a 16-byte input block. src is a raw byte buffer (pkt->in_buf), so
 * sys_get_le32() is used to assemble each word: it tolerates unaligned
 * pointers and writes the little-endian order the DATA_IN registers expect,
 * independent of host endianness.
 */
static void aes_write_block(mm_reg_t base, const uint8_t *src)
{
	for (uint32_t i = 0; i < 4; i++) {
		uint32_t word = sys_get_le32(src + i * 4);

		LOG_DBG("DATA_IN_%d = 0x%08x", i, word);
		sys_write32(word, base + AES_DATA_IN_0_REG_OFFSET + i * 4);
	}
}

/*
 * Read a 16-byte output block. The caller must have confirmed OUTPUT_VALID=1.
 * All four DATA_OUT words must be read: the hardware will not start the next
 * block until every output word has been consumed (read-before-overwrite
 * interlock). sys_put_le32() mirrors the LE handling in aes_write_block().
 */
static void aes_read_block(mm_reg_t base, uint8_t *dst)
{
	for (uint32_t i = 0; i < 4; i++) {
		uint32_t word = sys_read32(base + AES_DATA_OUT_0_REG_OFFSET + i * 4);

		LOG_DBG("DATA_OUT_%d raw = 0x%08x", i, word);
		sys_put_le32(word, dst + i * 4);
	}
}

/* ------------------------------------------------------ */
/* AES Modes - ECB, CBC, CTR */
/* ------------------------------------------------------ */

/* loop until STATUS.INPUT_READY (bit 4) is set, meaning the hardware */
/* has consumed the previous DATA_IN write and is ready for a new block. */
static int poll_input_ready(mm_reg_t base)
{
	uint32_t t = 0;

	while (!(sys_read32(base + AES_STATUS_REG_OFFSET) & (1u << AES_STATUS_INPUT_READY_BIT))) {
		if (t++ > (AES_TIMEOUT_US / 10)) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10);
	}
	return 0;
}

/* loop until STATUS.OUTPUT_VALID (bit 3) is set, meaning the hardware */
/* has finished processing a block and the result is ready in DATA_OUT. */
static int poll_output_valid(mm_reg_t base)
{
	uint32_t t = 0;

	while (!(sys_read32(base + AES_STATUS_REG_OFFSET) & (1u << AES_STATUS_OUTPUT_VALID_BIT))) {
		if (t++ > (AES_TIMEOUT_US / 10)) {
			return -ETIMEDOUT;
		}
		k_busy_wait(10);
	}
	return 0;
}

/* --- ECB mode --- */
static int opentitan_aes_ecb_op(struct cipher_ctx *ctx,
					 struct cipher_pkt *pkt)
{
	const struct opentitan_aes_session *sess =
		(const struct opentitan_aes_session *)ctx->drv_sessn_state;
	const struct opentitan_aes_config *cfg =
		(const struct opentitan_aes_config *)sess->dev->config;
	mm_reg_t base = cfg->base_addr;

	/* No padding: the caller must supply whole 16-byte blocks. */
	if (pkt->in_len == 0 || (pkt->in_len % 16) != 0) {
		return -EINVAL;
	}

	uint32_t num_blocks = pkt->in_len / 16;

	uint32_t op_val = (sess->dir == CRYPTO_CIPHER_OP_ENCRYPT)
				  ? AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENC
				  : AES_CTRL_SHADOWED_OPERATION_VALUE_AES_DEC;

	uint32_t ctrl_val =
		(op_val << AES_CTRL_SHADOWED_OPERATION_OFFSET) |
		(AES_CTRL_SHADOWED_MODE_VALUE_AES_ECB << AES_CTRL_SHADOWED_MODE_OFFSET) |
		sess->reg_ctrl_key_len;
	LOG_DBG("CTRL_SHADOWED = 0x%08x", ctrl_val);

	/* CTRL writes are silently ignored unless the hardware is idle. */
	int ret = poll_idle(base);

	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Before writing CTRL");
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET); /* shadowed */
	LOG_DBG("After writing CTRL, STATUS=0x%08x", sys_read32(base + AES_STATUS_REG_OFFSET));

	ret = poll_idle(base);
	if (ret != 0) {
		return ret;
	}

	aes_write_key(base, sess->key_words, sess->key_words_count);

	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	aes_write_block(base, pkt->in_buf); /* block 0 — hardware auto-starts */

	/* Wait for block 0 to be latched before optionally pre-loading block 1. */
	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	/* Pre-load block 1 if present (pipeline warm-up). */
	if (num_blocks > 1) {
		aes_write_block(base, pkt->in_buf + 16);
	}

	for (uint32_t i = 0; i < num_blocks; i++) {
		ret = poll_output_valid(base);
		if (ret) {
			return ret;
		}

		aes_read_block(base, pkt->out_buf + i * 16);

		/*
		 * Feed a new block on every read so processing of block N overlaps
		 * with block N+1. Per the OpenTitan programmer's guide, INPUT_READY
		 * is guaranteed set when OUTPUT_VALID is set, so no extra
		 * poll_input_ready() is required here.
		 */
		if (i + 2 < num_blocks) {
			aes_write_block(base, pkt->in_buf + (i + 2) * 16);
		}
	}

	return 0;
}

/* --- CBC mode --- */
/* similar to ECB but with the IV handling */
static int opentitan_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	const struct opentitan_aes_session *sess =
		(const struct opentitan_aes_session *)ctx->drv_sessn_state;
	const struct opentitan_aes_config *cfg =
		(const struct opentitan_aes_config *)sess->dev->config;
	mm_reg_t base = cfg->base_addr;

	/* CBC requires a 16-byte IV. */
	if (iv == NULL) {
		LOG_ERR("CBC mode requires a non-NULL IV");
		return -EINVAL;
	}

	/* Input must be a non-empty multiple of 16 bytes. */
	if (pkt->in_len == 0 || (pkt->in_len % 16) != 0) {
		return -EINVAL;
	}

	uint32_t num_blocks = pkt->in_len / 16;
	const uint8_t *iv_bytes = iv;

	uint32_t op_val = (sess->dir == CRYPTO_CIPHER_OP_ENCRYPT)
				  ? AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENC
				  : AES_CTRL_SHADOWED_OPERATION_VALUE_AES_DEC;

	uint32_t ctrl_val =
		(op_val << AES_CTRL_SHADOWED_OPERATION_OFFSET) |
		(AES_CTRL_SHADOWED_MODE_VALUE_AES_CBC << AES_CTRL_SHADOWED_MODE_OFFSET) |
		sess->reg_ctrl_key_len;
	LOG_DBG("CTRL_SHADOWED = 0x%08x", ctrl_val);

	/* Wait for IDLE before writing CTRL. */
	int ret = poll_idle(base);

	if (ret != 0) {
		return ret;
	}

	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);

	ret = poll_idle(base);
	if (ret != 0) {
		return ret;
	}

	aes_write_key(base, sess->key_words, sess->key_words_count);

	/* Load the user-supplied IV for the first block. */
	uint32_t iv_words[4];

	for (uint32_t i = 0; i < 4; i++) {
		iv_words[i] = sys_get_le32(iv_bytes + i * 4);
	}
	aes_write_iv(base, iv_words);
	/*
	 * Software must not re-write the IV between blocks: in CBC mode the
	 * hardware updates the IV registers after each block.
	 * Ref: OpenTitan AES Theory of Operation, Datapath step 7.
	 * https://opentitan.org/book/hw/ip/aes/doc/theory_of_operation.html
	 */

	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	aes_write_block(base, pkt->in_buf);

	/* Wait for block 0 to be latched before optionally pre-loading block 1. */
	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	/* Pre-load block 1 if present (pipeline warm-up). */
	if (num_blocks > 1) {
		aes_write_block(base, pkt->in_buf + 16);
	}

	/* Pipeline loop: read output N, write input N+2. */
	for (uint32_t i = 0; i < num_blocks; i++) {
		ret = poll_output_valid(base);
		if (ret) {
			return ret;
		}

		aes_read_block(base, pkt->out_buf + i * 16);

		if (i + 2 < num_blocks) {
			aes_write_block(base, pkt->in_buf + (i + 2) * 16);
		}
	}

	return 0;
}

/*
 * ---- CTR mode ----
 * Unlike CBC: any input length is accepted (the final block may be partial),
 * and the hardware auto-increments the counter, so software must not re-write
 * the IV between blocks.
 *
 * Write the input block at 'block_idx' into the AES data-in registers.
 * The final block is zero-padded when the packet length isn't a multiple
 * of 16 bytes.
 */
static void aes_ctr_write_block(mm_reg_t base, struct cipher_pkt *pkt, uint32_t block_idx,
				 uint32_t num_blocks, uint32_t remainder)
{
	uint8_t padded_in[16] = {0};

	if (block_idx == num_blocks - 1 && remainder != 0) {
		memcpy(padded_in, pkt->in_buf + block_idx * 16, remainder);
		aes_write_block(base, padded_in);
	} else {
		aes_write_block(base, pkt->in_buf + block_idx * 16);
	}
}

/*
 * Read the output block at 'block_idx' from the AES data-out registers.
 * Only 'remainder' bytes are copied out for the final, partial block.
 */
static void aes_ctr_read_block(mm_reg_t base, struct cipher_pkt *pkt, uint32_t block_idx,
				uint32_t num_blocks, uint32_t remainder)
{
	uint8_t padded_out[16];

	if (block_idx == num_blocks - 1 && remainder != 0) {
		aes_read_block(base, padded_out);
		memcpy(pkt->out_buf + block_idx * 16, padded_out, remainder);
	} else {
		aes_read_block(base, pkt->out_buf + block_idx * 16);
	}
}

static int opentitan_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *ctr)
{
	const struct opentitan_aes_session *sess =
		(const struct opentitan_aes_session *)ctx->drv_sessn_state;
	const struct opentitan_aes_config *cfg =
		(const struct opentitan_aes_config *)sess->dev->config;
	mm_reg_t base = cfg->base_addr;

	if (ctr == NULL) {
		LOG_ERR("CTR mode requires a non-NULL counter/IV");
		return -EINVAL;
	}

	if (pkt->in_len == 0) {
		return -EINVAL;
	}

	uint32_t num_full_blocks = pkt->in_len / 16;
	uint32_t remainder = pkt->in_len % 16;

	uint32_t num_blocks = num_full_blocks + (remainder ? 1 : 0);

	/*
	 * CTR mode always uses AES_ENC for the keystream generation,
	 * regardless of whether the session is encrypting or decrypting.
	 * The OPERATION field in CTRL must be AES_ENC (0x1).
	 */
	uint32_t ctrl_val =
		(AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENC << AES_CTRL_SHADOWED_OPERATION_OFFSET) |
		(AES_CTRL_SHADOWED_MODE_VALUE_AES_CTR << AES_CTRL_SHADOWED_MODE_OFFSET) |
		sess->reg_ctrl_key_len;

	LOG_DBG("CTR CTRL_SHADOWED = 0x%08x", ctrl_val);

	/* Wait for IDLE before writing CTRL. */
	int ret = poll_idle(base);

	if (ret != 0) {
		return ret;
	}

	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);
	sys_write32(ctrl_val, base + AES_CTRL_SHADOWED_REG_OFFSET);

	/* Re-poll IDLE: CTRL write may trigger PRNG reseed. */
	ret = poll_idle(base);
	if (ret != 0) {
		return ret;
	}

	aes_write_key(base, sess->key_words, sess->key_words_count);

	/*
	 * Load the initial counter value into IV_0..IV_3.
	 * Hardware auto-increments after each block — do NOT re-write between blocks.
	 */
	uint32_t iv_words[4];

	for (uint32_t i = 0; i < 4; i++) {
		iv_words[i] = sys_get_le32(ctr + i * 4);
	}
	aes_write_iv(base, iv_words);

	/* --- Pipeline loop (same pattern as ECB/CBC) --- */
	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	/*
	 * For the partial last block, pad the input to 16 bytes with zeros,
	 * feed the full padded block to hardware, then copy only 'remainder'
	 * bytes from the output. The hardware always processes 128-bit blocks.
	 */
	if (num_full_blocks == 0) {
		aes_ctr_write_block(base, pkt, 0, num_blocks, remainder);
		ret = poll_output_valid(base);
		if (ret) {
			return ret;
		}
		aes_ctr_read_block(base, pkt, 0, num_blocks, remainder);
		return 0;
	}

	/* Write first full block — hardware auto-starts. */
	aes_write_block(base, pkt->in_buf);

	ret = poll_input_ready(base);
	if (ret) {
		return ret;
	}

	if (num_blocks > 1) {
		aes_ctr_write_block(base, pkt, 1, num_blocks, remainder);
	}

	/* Pipeline: read output i, write input i+2. */
	for (uint32_t i = 0; i < num_blocks; i++) {
		ret = poll_output_valid(base);
		if (ret) {
			return ret;
		}

		aes_ctr_read_block(base, pkt, i, num_blocks, remainder);

		uint32_t next = i + 2;

		if (next < num_blocks) {
			aes_ctr_write_block(base, pkt, next, num_blocks, remainder);
		}
	}

	return 0;
}

/* ------------------------------------------------------ */
/* Session Management */
/* ------------------------------------------------------ */

#define OPENTITAN_AES_LOCK_TIMEOUT K_MSEC(100)

static int
opentitan_aes_begin_session(const struct device *dev,
				struct cipher_ctx *ctx,
				enum cipher_algo algo,
				enum cipher_mode mode,
				enum cipher_op op_type)
{
	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algorithm %d; only AES is supported", algo);
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CTR) {
		LOG_ERR("Unsupported mode %d; supported: ECB, CBC, CTR", mode);
		return -ENOTSUP;
	}

	if (ctx->key.bit_stream == NULL) {
		LOG_ERR("Key pointer (ctx->key.bit_stream) is NULL");
		return -EINVAL;
	}

	/* ctx->keylen is in bytes (Zephyr convention). */
	uint32_t key_len_bits = (uint32_t)ctx->keylen * 8u;

	/* Pre-shifted KEY_LEN field for CTRL, and the count of 32-bit key words. */
	uint32_t reg_ctrl_key_len;
	uint32_t key_words_count;

	if (key_len_bits == 128u) {
		reg_ctrl_key_len =
			AES_CTRL_SHADOWED_KEY_LEN_VALUE_AES_128
			<< AES_CTRL_SHADOWED_KEY_LEN_OFFSET;
		key_words_count = 4u;
	} else if (key_len_bits == 256u) {
		reg_ctrl_key_len =
			AES_CTRL_SHADOWED_KEY_LEN_VALUE_AES_256
			<< AES_CTRL_SHADOWED_KEY_LEN_OFFSET;
		key_words_count = 8u;
	} else {
		LOG_ERR("Unsupported key length %u bits; only 128 and 256 are supported",
			key_len_bits);
		return -EINVAL;
	}

	/* Claim a free session slot under the pool mutex. */
	struct opentitan_aes_data *data = dev->data;

	int lock_ret = k_mutex_lock(&data->lock, OPENTITAN_AES_LOCK_TIMEOUT);

	if (lock_ret != 0) {
		LOG_ERR("Could not acquire session-pool mutex (timeout)");
		return -EBUSY;
	}

	struct opentitan_aes_session *sess = NULL;

	for (uint32_t i = 0; i < CONFIG_CRYPTO_OPENTITAN_MAX_SESSION; i++) {
		if (!data->sessions[i].in_use) {
			sess = &data->sessions[i];
			break;
		}
	}

	if (sess == NULL) {
		k_mutex_unlock(&data->lock);
		LOG_ERR("No free AES session slots (max = %d)",
			CONFIG_CRYPTO_OPENTITAN_MAX_SESSION);
		return -ENOMEM;
	}

	sess->dev = dev;
	sess->dir = op_type;
	sess->reg_ctrl_key_len = reg_ctrl_key_len;
	sess->key_words_count = key_words_count;

	for (uint32_t i = 0; i < key_words_count; i++) {
		sess->key_words[i] = sys_get_le32(ctx->key.bit_stream + i * 4u);
	}

	sess->in_use = true;

	k_mutex_unlock(&data->lock);

	/* Install the mode-specific handler the crypto core will dispatch to. */
	if (mode == CRYPTO_CIPHER_MODE_ECB) {
		ctx->ops.block_crypt_hndlr = opentitan_aes_ecb_op;
		LOG_INF("AES session started: mode=ECB dir=%d key=%u bits", op_type, key_len_bits);
	} else if (mode == CRYPTO_CIPHER_MODE_CBC) {
		ctx->ops.cbc_crypt_hndlr = opentitan_aes_cbc_op;
		LOG_INF("AES session started: mode=CBC dir=%d key=%u bits", op_type, key_len_bits);
	} else { /* CTR */
		ctx->ops.ctr_crypt_hndlr = opentitan_aes_ctr_op;
		LOG_INF("AES session started: mode=CTR key=%u bits", key_len_bits);
	}

	/* Link the session state to ctx so the op handlers can retrieve it. */
	ctx->drv_sessn_state = sess;

	uint32_t op_hw = (op_type == CRYPTO_CIPHER_OP_ENCRYPT)
				 ? AES_CTRL_SHADOWED_OPERATION_VALUE_AES_ENC
				 : AES_CTRL_SHADOWED_OPERATION_VALUE_AES_DEC;
	LOG_DBG("BEGIN_SESSION CTRL_MASK = 0x%08x", sess->reg_ctrl_key_len | op_hw);

	return 0;
}

/*
 * Tears down a session created by opentitan_aes_begin_session().
 * Zeroes all key material in the session struct before releasing the slot.
 */
static int opentitan_aes_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	if (ctx == NULL || ctx->drv_sessn_state == NULL) {
		LOG_ERR("free_session called with NULL ctx or drv_sessn_state");
		return -EINVAL;
	}

	struct opentitan_aes_session *sess = (struct opentitan_aes_session *)ctx->drv_sessn_state;
	struct opentitan_aes_data *data = dev->data;
	const struct opentitan_aes_config *cfg = dev->config;

	int lock_ret = k_mutex_lock(&data->lock, OPENTITAN_AES_LOCK_TIMEOUT);

	if (lock_ret != 0) {
		LOG_ERR("Could not acquire session-pool mutex in free_session (timeout)");
		return -EBUSY;
	}

	/* Clears key material and marks the slot free (in_use = false). */
	memset(sess, 0, sizeof(struct opentitan_aes_session));

	/*
	 * Scrub the KEY_SHARE0/SHARE1 hardware registers; the memset above only
	 * clears the SRAM copy. OpenTitan retains key material in these registers
	 * until explicitly cleared, which a later session could otherwise observe
	 * via timing side-channels.
	 * See https://github.com/lowRISC/opentitan/issues/2382
	 */
	opentitan_aes_hw_flush(cfg->base_addr);

	k_mutex_unlock(&data->lock);

	/* Break the ctx/session link so a freed session can't be reused. */
	ctx->drv_sessn_state = NULL;

	LOG_INF("AES session freed");

	return 0;
}

#define OPENTITAN_AES_HW_CAPS (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

static int opentitan_aes_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return OPENTITAN_AES_HW_CAPS;
}

static DEVICE_API(crypto, opentitan_aes_api) = {
	.query_hw_caps = opentitan_aes_query_hw_caps,
	.cipher_begin_session = opentitan_aes_begin_session,
	.cipher_free_session = opentitan_aes_free_session,
};

#define OPENTITAN_AES_INIT(n)                                                                      \
                                                                                                   \
	static const struct opentitan_aes_config opentitan_aes_cfg_##n = {                         \
		.base_addr = DT_INST_REG_ADDR(n),                                                  \
	};                                                                                         \
                                                                                                   \
	static struct opentitan_aes_data opentitan_aes_data_##n = {                                \
		.lock = Z_MUTEX_INITIALIZER(opentitan_aes_data_##n.lock),                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, opentitan_aes_init, NULL, &opentitan_aes_data_##n,                \
			      &opentitan_aes_cfg_##n, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,    \
			      &opentitan_aes_api);

DT_INST_FOREACH_STATUS_OKAY(OPENTITAN_AES_INIT)

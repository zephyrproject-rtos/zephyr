/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/irq.h>
#include <DA1469xAB.h>
#include <da1469x_config.h>
#include <da1469x_otp.h>
#include <system_DA1469x.h>
#include <da1469x_pd.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(crypto_smartbond_crypto, CONFIG_CRYPTO_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_smartbond_crypto

#define SMARTBOND_IRQN      DT_INST_IRQN(0)
#define SMARTBOND_IRQ_PRIO  DT_INST_IRQ(0, priority)

#if defined(CONFIG_CRYPTO_ASYNC)
#define CRYPTO_HW_CAPS  (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_ASYNC_OPS | CAP_NO_IV_PREFIX)
#else
#define CRYPTO_HW_CAPS  (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#endif

#define SWAP32(_w)   __REV(_w)

#define CRYPTO_CTRL_REG_SET(_field, _val) \
	AES_HASH->CRYPTO_CTRL_REG = \
	(AES_HASH->CRYPTO_CTRL_REG & ~AES_HASH_CRYPTO_CTRL_REG_ ## _field ## _Msk) | \
	((_val) << AES_HASH_CRYPTO_CTRL_REG_ ## _field ## _Pos)

#define CRYPTO_CTRL_REG_GET(_field) \
	((AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_ ## _field ## _Msk) >> \
	AES_HASH_CRYPTO_CTRL_REG_ ## _field ## _Pos)


struct crypto_smartbond_data {
    /*
     * Semaphore to provide mutual exlusion when a crypto session is requested.
     */
	struct k_sem session_sem;

    /*
     * Semaphore to provide mutual exlusion when a cryptographic task is requested.
     * (a session should be requested at this point).
     */
	struct k_sem device_sem;
#if defined(CONFIG_CRYPTO_ASYNC)
    /*
     * User-defined callbacks to be called upon completion of asynchronous
     * cryptographic operations. Note that the AES and HASH modes can work
     * complementary to each other.
     */
	union {
		cipher_completion_cb cipher_user_cb;
		hash_completion_cb hash_user_cb;
	};

    /*
     * Packet context should be stored during a session so that can be rertieved
     * from within the crypto engine ISR context.
     */
	union {
		struct cipher_pkt *cipher_pkt;
		struct hash_pkt *hash_pkt;
	};
#else
    /*
     * Semaphore used to block for as long as a synchronous cryptographic operation
     * is in progress.
     */
	struct k_sem sync_sem;
#endif
};

/*
 * Status flag to indicate if the crypto engine resources have been granted. Note that the
 * device integrates a single crypto engine instance.
 */
static bool in_use;

static void crypto_smartbond_set_status(bool enable);

static void smartbond_crypto_isr(const void *arg)
{
	struct crypto_smartbond_data *data = ((const struct device *)arg)->data;
	uint32_t status = AES_HASH->CRYPTO_STATUS_REG;

	if (status & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_IRQ_ST_Msk) {
	    /* Clear interrupt source. Otherwise the handler will be fire constantly! */
		AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;

#if defined(CONFIG_CRYPTO_ASYNC)
		/* Define the slected crypto mode (AES/HASH). */
		if (AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_HASH_SEL_Msk) {
			if (data->hash_user_cb) {
				data->hash_user_cb(data->hash_pkt, status);
			}
		} else {
			if (data->cipher_user_cb) {
				data->cipher_user_cb(data->cipher_pkt, status);
			}
		}
#else
		/* Designate the requested cryptographic tasks is finished. */
		k_sem_give(&data->sync_sem);
#endif
	}
}

static inline void crypto_smartbond_pm_policy_state_lock_get(const struct device *dev)
{
	/*
	 * Prevent the SoC from entering the normal sleep state as PDC does not support
	 * waking up the application core following AES/HASH events.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static inline void crypto_smartbond_pm_policy_state_lock_put(const struct device *dev)
{
	/* Allow the SoC to enter the normal sleep state once AES/HASH operations are done. */
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static bool crypto_smartbond_lock_session(const struct device *dev)
{
	bool lock = false;
	struct crypto_smartbond_data *data = dev->data;

	k_sem_take(&data->session_sem, K_FOREVER);

	if (!in_use) {
		in_use = true;
		/* Prevent sleep as long as a cryptographic session is in place */
		da1469x_pd_acquire(MCU_PD_DOMAIN_SYS);
		crypto_smartbond_pm_policy_state_lock_get(dev);
		crypto_smartbond_set_status(true);
		lock = true;
	}

	k_sem_give(&data->session_sem);

	return lock;
}

static void crypto_smartbond_unlock_session(const struct device *dev)
{
	struct crypto_smartbond_data *data = dev->data;

	k_sem_take(&data->session_sem, K_FOREVER);

	if (in_use) {
		in_use = false;
		crypto_smartbond_set_status(false);
		crypto_smartbond_pm_policy_state_lock_put(dev);
		da1469x_pd_release_nowait(MCU_PD_DOMAIN_SYS);
	}

	k_sem_give(&data->session_sem);
}

/*
 * Input vector should comply with the following restrictions:
 *
 * mode        | CRYPTO_MORE_IN = true  | CRYPTO_MORE_IN = false
 * ------------| -----------------------| ----------------------
 * ECB         | multiple of 16 (bytes) | multiple of 16 (bytes)
 * CBC         | multiple of 16         | no restrictions
 * CTR         | multiple of 16         | no restrictions
 * MD5         | multiple of 8          | no restrictions
 * SHA_1       | multiple of 8          | no restrictions
 * SHA_256_224 | multiple of 8          | no restrictions
 * SHA_256     | multiple of 8          | no restrictions
 * SHA_384     | multiple of 8          | no restrictions
 * SHA_512     | multiple of 8          | no restrictions
 * SHA_512_224 | multiple of 8          | no restrictions
 * SHA_512_256 | multiple of 8          | no restrictions
 */
static int crypto_smartbond_check_in_restrictions(uint16_t in_len)
{
#define CRYPTO_ALG_MD_ECB_MAGIC_0   0x00
#define CRYPTO_ALG_MD_ECB_MAGIC_1   0x01

	bool not_last_in_block = !!(AES_HASH->CRYPTO_CTRL_REG &
						AES_HASH_CRYPTO_CTRL_REG_CRYPTO_MORE_IN_Msk);

	/* Define the slected crypto mode (AES/HASH). */
	if (AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_HASH_SEL_Msk) {
		if (not_last_in_block && (in_len & 0x7)) {
			return -EINVAL;
		}
	} else {
		if (in_len & 0xF) {
			if (not_last_in_block) {
				return -EINVAL;
			}

			uint32_t crypto_mode = CRYPTO_CTRL_REG_GET(CRYPTO_ALG_MD);

			/* Check if AES mode is ECB */
			if (crypto_mode == CRYPTO_ALG_MD_ECB_MAGIC_0 ||
							crypto_mode == CRYPTO_ALG_MD_ECB_MAGIC_1) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

/*
 * The driver model does not define the max. output length. As such, the max supported length
 * per mode is applied.
 */
static int crypto_smartbond_hash_set_out_len(void)
{
	uint32_t hash_algo = (AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Msk);

	if (AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk) {
	    /* 64-bit HASH operations */
		switch (hash_algo) {
		case 0x0:
			/* SHA-384: 0..47 --> 1..48 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 47);
			break;
		case 0x1:
			/* SHA-512: 0..63 --> 1..64 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 63);
			break;
		case 0x2:
			/* SHA-512/224: 0..27 --> 1..28 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 27);
			break;
		case 0x3:
			/* SHA-512/256: 0..31 --> 1..32 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 31);
			break;
		default:
			break;
		}
	} else {
	    /* 32-bit HASH operations */
		switch (hash_algo) {
		case 0x0:
			/* MD5: 0..15 --> 1..16 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 15);
			break;
		case 0x1:
			/* SHA-1: 0..19 --> 1..20 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 19);
			break;
		case 0x2:
			/* SHA-256/224: 0..27 --> 1..28 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 27);
			break;
		case 0x3:
			/* SHA-256: 0..31 --> 1..32 bytes */
			CRYPTO_CTRL_REG_SET(CRYPTO_HASH_OUT_LEN, 31);
			break;
		default:
			break;
		}
	}

	/* Return the OUT size applied. */
	return CRYPTO_CTRL_REG_GET(CRYPTO_HASH_OUT_LEN) + 1;
}

static uint32_t crypto_smartbond_swap_word(uint8_t *data)
{
    /* Check word boundaries of given address and if possible accellerate swapping */
	if ((uint32_t)data & 0x3) {
		return SWAP32(sys_get_le32(data));
	} else {
		return SWAP32(*(uint32_t *)data);
	}
}

static int crypto_smartbond_cipher_key_load(uint8_t *key, uint16_t key_len)
{
	if (key == NULL) {
		return -EIO;
	}

	AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_AES_KEY_SZ_Msk);

	if (key_len == 32) {
		AES_HASH->CRYPTO_CTRL_REG |=
		(0x2 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_AES_KEY_SZ_Pos);
	} else if (key_len == 24) {
		AES_HASH->CRYPTO_CTRL_REG |=
		(0x1 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_AES_KEY_SZ_Pos);
	} else if (key_len == 16) {
		/* Nothing to do */
	} else {
		return -EINVAL;
	}

	/* Key expansion is performed by the crypto engine */
	AES_HASH->CRYPTO_CTRL_REG |= AES_HASH_CRYPTO_CTRL_REG_CRYPTO_AES_KEXP_Msk;

	/* Check whether the cipher key is located in OTP (user keys segment) */
	if (IS_ADDRESS_USER_DATA_KEYS_SEGMENT((uint32_t)key)) {

		/* User keys segmnet can be accessed if not locked (stick bits are not set) */
		if (CRG_TOP->SECURE_BOOT_REG & CRG_TOP_SECURE_BOOT_REG_PROT_AES_KEY_READ_Msk) {
			return -EIO;
		}

		uint32_t cell_offset = da1469x_otp_address_to_cell_offset((uint32_t)key);

		da1469x_otp_read(cell_offset,
				(void *)&AES_HASH->CRYPTO_KEYS_START, (uint32_t)key_len);
	} else {
		volatile uint32_t *kmem_ptr = &AES_HASH->CRYPTO_KEYS_START;

		do {
			*(kmem_ptr++) = crypto_smartbond_swap_word(key);
			key += 4;
			key_len -= 4;
		} while (key_len);
	}

	return 0;
}

static int crypto_smartbond_cipher_set_mode(enum cipher_mode mode)
{
	/* Select AES mode */
	AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk  |
						AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Msk        |
						AES_HASH_CRYPTO_CTRL_REG_CRYPTO_HASH_SEL_Msk);
	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		/* Already done; CRYPTO_ALG_MD = 0x0 or 0x1 defines ECB. */
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		AES_HASH->CRYPTO_CTRL_REG |= (0x2 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Pos);
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		AES_HASH->CRYPTO_CTRL_REG |= (0x3 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Pos);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int crypto_smartbond_hash_set_algo(enum hash_algo algo)
{
	/* Select HASH mode and reset to 32-bit mode */
	AES_HASH->CRYPTO_CTRL_REG =
			(AES_HASH->CRYPTO_CTRL_REG & ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Msk |
						AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk)) |
					AES_HASH_CRYPTO_CTRL_REG_CRYPTO_HASH_SEL_Msk;

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		/* CRYPTO_ALG_MD = 0x0 defines 32-bit operations */
		AES_HASH->CRYPTO_CTRL_REG |= (0x2 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Pos);
		break;
	case CRYPTO_HASH_ALGO_SHA256:
		/* CRYPTO_ALG_MD = 0x0 defines 32-bit operations */
		AES_HASH->CRYPTO_CTRL_REG |= (0x3 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Pos);
		break;
	case CRYPTO_HASH_ALGO_SHA384:
		/* CRYPTO_ALG_MD = 0x1 defines 64-bit operations */
		AES_HASH->CRYPTO_CLRIRQ_REG |= AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk;
		break;
	case CRYPTO_HASH_ALGO_SHA512:
		/* CRYPTO_ALG_MD = 0x1 defines 64-bit operations */
		AES_HASH->CRYPTO_CTRL_REG |= (AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_MD_Msk |
						(0x1 << AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ALG_Pos));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int crypto_smartbond_set_in_out_buf(uint8_t *in_buf, uint8_t *out_buf, int len)
{
	if (in_buf == NULL) {
		return -EIO;
	}

	/*
	 * Input data can reside in any address space. Cryto DMA can only access physical addresses
	 * (not remapped).
	 */
	uint32_t phy_addr = black_orca_phy_addr((uint32_t)in_buf);

	if (IS_QSPIF_CACHED_ADDRESS(phy_addr)) {
		/*
		 * To achiebe max. perfomance, peripherals should not access the Flash memory
		 * through the instruction cache controller (avoid cache misses).
		 */
		phy_addr += (MCU_QSPIF_M_BASE - MCU_QSPIF_M_CACHED_BASE);
	} else if (IS_OTP_ADDRESS(phy_addr)) {
		/* Peripherals should access the OTP memory through its peripheral address space. */
		phy_addr += (MCU_OTP_M_P_BASE - MCU_OTP_M_BASE);
	}

	AES_HASH->CRYPTO_FETCH_ADDR_REG = phy_addr;

	/*
	 * OUT buffer can be NULL in case of fregmented data processing. CRYPTO_DEST_ADDR and
	 * CRYPTO_FETCH_ADDR are being updated as calculations prceed and OUT data are written
	 * into memory.
	 */
	if (out_buf) {
		uint32_t remap_adr0 = CRG_TOP->SYS_CTRL_REG & CRG_TOP_SYS_CTRL_REG_REMAP_ADR0_Msk;

		/*
		 * OUT data can only be written in SYSRAM, non-cached remapped SYSRAM and
		 * cached non-remapped SYSRAM.
		 */
		if (IS_SYSRAM_ADDRESS(out_buf) ||
			(IS_REMAPPED_ADDRESS(out_buf) && remap_adr0 == 3)) {
			AES_HASH->CRYPTO_DEST_ADDR_REG = black_orca_phy_addr((uint32_t)out_buf);
		} else {
			return -EIO;
		}
	}

	AES_HASH->CRYPTO_LEN_REG = len;

	return 0;
}

static inline void crypto_smartbond_cipher_store_dep_data(uint32_t *words, uint32_t len_words)
{
	volatile uint32_t *mreg3 = &AES_HASH->CRYPTO_MREG3_REG;

	for (int i = 0; i < len_words; i++) {
		*(mreg3--) = crypto_smartbond_swap_word((uint8_t *)(words++));
	}
}

static int crypto_smartbond_cipher_set_mreg(uint8_t *mreg, uint32_t len_words)
{
	if (mreg == NULL || len_words == 0 || len_words > 4) {
		return -EINVAL;
	}

	AES_HASH->CRYPTO_MREG0_REG = 0;
	AES_HASH->CRYPTO_MREG1_REG = 0;
	AES_HASH->CRYPTO_MREG2_REG = 0;
	AES_HASH->CRYPTO_MREG3_REG = 0;

	crypto_smartbond_cipher_store_dep_data((uint32_t *)mreg, len_words);

	return 0;
}

static void crypto_smartbond_set_status(bool enable)
{
	unsigned int key;

	key = irq_lock();

	if (enable) {
		CRG_TOP->CLK_AMBA_REG |= (CRG_TOP_CLK_AMBA_REG_AES_CLK_ENABLE_Msk);

		AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;
		AES_HASH->CRYPTO_CTRL_REG |= (AES_HASH_CRYPTO_CTRL_REG_CRYPTO_IRQ_EN_Msk);

		irq_enable(SMARTBOND_IRQN);
	} else {
		AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_IRQ_EN_Msk);
		AES_HASH->CRYPTO_CLRIRQ_REG = 0x1;

		irq_disable(SMARTBOND_IRQN);

		CRG_TOP->CLK_AMBA_REG &= ~(CRG_TOP_CLK_AMBA_REG_AES_CLK_ENABLE_Msk);
	}

	irq_unlock(key);
}

static int crypto_smartbond_query_hw_caps(const struct device *dev)
{
	return CRYPTO_HW_CAPS;
}

static int crypto_smartbond_cipher_ecb_handler(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int ret;
	struct crypto_smartbond_data *data = ctx->device->data;

	if ((AES_HASH->CRYPTO_STATUS_REG & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_INACTIVE_Msk) == 0) {
		LOG_ERR("Crypto engine is already employed");
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("OUT buffer cannot be less that IN buffer");
		return -EINVAL;
	}

	if (pkt->in_buf == NULL || pkt->out_buf == NULL) {
		LOG_ERR("Missing IN or OUT buffer declaration");
		return -EIO;
	}

	if (pkt->in_len > 16) {
		LOG_ERR("For security reasons, do not operate on more than 16 bytes");
		return -EINVAL;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_smartbond_check_in_restrictions(pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN buffer size");
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_smartbond_set_in_out_buf(pkt->in_buf, pkt->out_buf, pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN or OUT buffer location");
		k_sem_give(&data->device_sem);
		return ret;
	}

#if defined(CONFIG_CRYPTO_ASYNC)
	data->cipher_pkt = pkt;
#endif

	/* Start crypto processing */
	AES_HASH->CRYPTO_START_REG = 1;

#if !defined(CONFIG_CRYPTO_ASYNC)
	/* Wait for crypto to finish its task */
	k_sem_take(&data->sync_sem, K_FOREVER);
#endif

	/* Report that number of bytes operated upon. */
	pkt->out_len = pkt->in_len;

	k_sem_give(&data->device_sem);

	return 0;
}

static int
crypto_smartbond_cipher_cbc_handler(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;
	int offset = 0;
	struct crypto_smartbond_data *data = ctx->device->data;
	bool is_op_encryption =
			!!(AES_HASH->CRYPTO_CTRL_REG & AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ENCDEC_Msk);

	if ((AES_HASH->CRYPTO_STATUS_REG & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_INACTIVE_Msk) == 0) {
		LOG_ERR("Crypto engine is already employed");
		return -EINVAL;
	}

	if ((is_op_encryption && pkt->out_buf_max < (pkt->in_len + 16)) ||
							pkt->out_buf_max < (pkt->in_len - 16)) {
		LOG_ERR("Invalid OUT buffer size");
		return -EINVAL;
	}

	if (pkt->in_buf == NULL || pkt->out_buf == NULL) {
		LOG_ERR("Missing IN or OUT buffer declaration");
		return -EIO;
	}

	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0) {
		offset = 16;
		if (is_op_encryption) {
			/* Prefix IV to ciphertet unless CAP_NO_IV_PREFIX is set. */
			memcpy(pkt->out_buf, iv, offset);
		}
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_smartbond_check_in_restrictions(pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN buffer size");
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_smartbond_cipher_set_mreg(iv, 4);
	if (ret < 0) {
		LOG_ERR("Missing Initialization Vector (IV)");
		k_sem_give(&data->device_sem);
		return ret;
	}

	if (is_op_encryption) {
		ret = crypto_smartbond_set_in_out_buf(pkt->in_buf,
						pkt->out_buf + offset, pkt->in_len);
	} else {
		ret = crypto_smartbond_set_in_out_buf(pkt->in_buf + offset,
								pkt->out_buf, pkt->in_len - offset);
	}

	if (ret < 0) {
		LOG_ERR("Unsupported IN or OUT buffer location");
		k_sem_give(&data->device_sem);
		return ret;
	}

#if defined(CONFIG_CRYPTO_ASYNC)
	data->cipher_pkt = pkt;
#endif

    /* Start crypto processing */
	AES_HASH->CRYPTO_START_REG = 1;

#if !defined(CONFIG_CRYPTO_ASYNC)
    /* Wait for crypto to finish its task */
	k_sem_take(&data->sync_sem, K_FOREVER);
#endif

	/* Report that number of bytes operated upon. */
	if (is_op_encryption) {
		pkt->out_len = pkt->in_len + offset;
	} else {
		pkt->out_len = pkt->in_len - offset;
	}

	k_sem_give(&data->device_sem);

	return 0;
}

static int crypto_smartbond_cipher_ctr_handler(struct cipher_ctx *ctx,
								struct cipher_pkt *pkt, uint8_t *ic)
{
	int ret;
	/* ivlen + ctrlen = keylen, ctrl_len is expressed in bits */
	uint32_t iv_len = ctx->keylen - (ctx->mode_params.ctr_info.ctr_len >> 3);
	struct crypto_smartbond_data *data = ctx->device->data;

	if ((AES_HASH->CRYPTO_STATUS_REG & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_INACTIVE_Msk) == 0) {
		LOG_ERR("Crypto engine is already employed");
		return -EINVAL;
	}

	if (pkt->out_buf_max < pkt->in_len) {
		LOG_ERR("OUT buffer cannot be less that IN buffer");
		return -EINVAL;
	}

	if (pkt->in_buf == NULL || pkt->out_buf == NULL) {
		LOG_ERR("Missing IN or OUT buffer declaration");
		return -EIO;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	ret = crypto_smartbond_check_in_restrictions(pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN buffer size");
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_smartbond_cipher_set_mreg(ic, iv_len >> 2);
	if (ret < 0) {
		LOG_ERR("Missing Initialization Counter (IC)");
		k_sem_give(&data->device_sem);
		return ret;
	}

	ret = crypto_smartbond_set_in_out_buf(pkt->in_buf, pkt->out_buf, pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN or OUT buffer location");
		k_sem_give(&data->device_sem);
		return ret;
	}

#if defined(CONFIG_CRYPTO_ASYNC)
	data->cipher_pkt = pkt;
#endif

    /* Start crypto processing */
	AES_HASH->CRYPTO_START_REG = 1;

#if !defined(CONFIG_CRYPTO_ASYNC)
    /* Wait for crypto to finish its task */
	k_sem_take(&data->sync_sem, K_FOREVER);
#endif

	/* Report that number of bytes operated upon. */
	pkt->out_len = pkt->in_len;

	k_sem_give(&data->device_sem);

	return 0;
}

static int crypto_smartbond_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	int ret;
	struct crypto_smartbond_data *data = ctx->device->data;
	/*
	 * In case of framgemented data processing crypto status should be visible as busy for
	 * as long as the last block is to be processed.
	 */
	bool is_multipart_started =
		(AES_HASH->CRYPTO_STATUS_REG & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_WAIT_FOR_IN_Msk) &&
		!(AES_HASH->CRYPTO_STATUS_REG & AES_HASH_CRYPTO_STATUS_REG_CRYPTO_INACTIVE_Msk);

	if (pkt->in_buf == NULL || (pkt->out_buf == NULL)) {
		LOG_ERR("Missing IN or OUT buffer declaration");
		return -EIO;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	/* Check if this is the last block to process or more blocks will follow */
	if (finish) {
		AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_MORE_IN_Msk);
	} else {
		AES_HASH->CRYPTO_CTRL_REG |= AES_HASH_CRYPTO_CTRL_REG_CRYPTO_MORE_IN_Msk;
	}

	/* CRYPTO_MORE_IN should be updated prior to checking for IN restrictions! */
	ret = crypto_smartbond_check_in_restrictions(pkt->in_len);
	if (ret < 0) {
		LOG_ERR("Unsupported IN buffer size");
		k_sem_give(&data->device_sem);
		return ret;
	}

	if (!is_multipart_started) {
		ret = crypto_smartbond_hash_set_out_len();
		if (ret < 0) {
			LOG_ERR("Invalid OUT buffer size");
			k_sem_give(&data->device_sem);
			return ret;
		}
	}

	if (!is_multipart_started) {
		ret = crypto_smartbond_set_in_out_buf(pkt->in_buf, pkt->out_buf, pkt->in_len);
	} else {
		/* Destination buffer is being updated as fragmented input is being processed. */
		ret = crypto_smartbond_set_in_out_buf(pkt->in_buf, NULL, pkt->in_len);
	}

	if (ret < 0) {
		LOG_ERR("Unsupported IN or OUT buffer location");
		k_sem_give(&data->device_sem);
		return ret;
	}

#if defined(CONFIG_CRYPTO_ASYNC)
	data->hash_pkt = pkt;
#endif

    /* Start hash processing */
	AES_HASH->CRYPTO_START_REG = 1;

#if !defined(CONFIG_CRYPTO_ASYNC)
	k_sem_take(&data->sync_sem, K_FOREVER);
#endif

	k_sem_give(&data->device_sem);

	return 0;
}

static int
crypto_smartbond_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
		enum cipher_algo algo, enum cipher_mode mode, enum cipher_op op_type)
{
	int ret;

	if (ctx->flags & ~(CRYPTO_HW_CAPS)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported cipher algo");
		return -EINVAL;
	}

	if (!crypto_smartbond_lock_session(dev)) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	/* First check if the requested cryptographic algo is supported */
	ret = crypto_smartbond_cipher_set_mode(mode);
	if (ret < 0) {
		LOG_ERR("Unsupported cipher mode");
		crypto_smartbond_unlock_session(dev);
		return ret;
	}

	ret = crypto_smartbond_cipher_key_load((uint8_t *)ctx->key.bit_stream, ctx->keylen);
	if (ret < 0) {
		LOG_ERR("Invalid key length or key cannot be accessed");
		crypto_smartbond_unlock_session(dev);
		return ret;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		AES_HASH->CRYPTO_CTRL_REG |= AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ENCDEC_Msk;
	} else {
		AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_ENCDEC_Msk);
	}

	/* IN buffer fragmentation is not supported by the driver model */
	AES_HASH->CRYPTO_CTRL_REG &= ~(AES_HASH_CRYPTO_CTRL_REG_CRYPTO_MORE_IN_Msk);

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		ctx->ops.block_crypt_hndlr = crypto_smartbond_cipher_ecb_handler;
		break;
	case CRYPTO_CIPHER_MODE_CBC:
		ctx->ops.cbc_crypt_hndlr = crypto_smartbond_cipher_cbc_handler;
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		ctx->ops.ctr_crypt_hndlr = crypto_smartbond_cipher_ctr_handler;
		break;
	default:
		break;
	}

	ctx->drv_sessn_state = NULL;

	return 0;
}

static int crypto_smartbond_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	ARG_UNUSED(ctx);
	crypto_smartbond_unlock_session(dev);

	return 0;
}

#if defined(CONFIG_CRYPTO_ASYNC)
static int
crypto_smartbond_cipher_set_async_callback(const struct device *dev, cipher_completion_cb cb)
{
	struct crypto_smartbond_data *data = dev->data;

	data->cipher_user_cb = cb;

	return 0;
}
#endif

static int
crypto_smartbond_hash_begin_session(const struct device *dev,
					struct hash_ctx *ctx, enum hash_algo algo)
{
	int ret;

	if (ctx->flags & ~(CRYPTO_HW_CAPS)) {
		LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	if (!crypto_smartbond_lock_session(dev)) {
		LOG_ERR("No free session for now");
		return -ENOSPC;
	}

	/*
	 * Crypto should be disabled only if not used in other sessions. In case of failure,
	 * developer should next free the current session.
	 */
	crypto_smartbond_set_status(true);

	ret = crypto_smartbond_hash_set_algo(algo);
	if (ret < 0) {
		LOG_ERR("Unsupported HASH algo");
		crypto_smartbond_unlock_session(dev);
		return ret;
	}

	ctx->hash_hndlr = crypto_smartbond_hash_handler;

	ctx->drv_sessn_state = NULL;

	return 0;
}

static int crypto_smartbond_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	ARG_UNUSED(ctx);
	crypto_smartbond_unlock_session(dev);

	return 0;
}

#if defined(CONFIG_CRYPTO_ASYNC)
static int
crypto_smartbond_hash_set_async_callback(const struct device *dev, hash_completion_cb cb)
{
	struct crypto_smartbond_data *data = dev->data;

	data->hash_user_cb = cb;

	return 0;
}
#endif

static const struct crypto_driver_api crypto_smartbond_driver_api = {
	.cipher_begin_session = crypto_smartbond_cipher_begin_session,
	.cipher_free_session = crypto_smartbond_cipher_free_session,
#if defined(CONFIG_CRYPTO_ASYNC)
	.cipher_async_callback_set = crypto_smartbond_cipher_set_async_callback,
#endif
	.hash_begin_session = crypto_smartbond_hash_begin_session,
	.hash_free_session = crypto_smartbond_hash_free_session,
#if defined(CONFIG_CRYPTO_ASYNC)
	.hash_async_callback_set = crypto_smartbond_hash_set_async_callback,
#endif
	.query_hw_caps = crypto_smartbond_query_hw_caps
};

#if defined(CONFIG_PM_DEVICE)
static int crypto_smartbond_pm_action(const struct device *dev,
	enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * No need to perform any actions here as the AES/HASH controller
		 * should already be turned off.
		 */
		break;
	case PM_DEVICE_ACTION_RESUME:
		/*
		 * No need to perform any actions here as the AES/HASH controller
		 * will be initialized upon acquiring a cryptographic session.
		 */
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int crypto_smartbond_init(const struct device *dev)
{
	struct crypto_smartbond_data *data = dev->data;

	/* Semaphore used during sessions (begin/free) */
	k_sem_init(&data->session_sem, 1, 1);

	/* Semaphore used to employ the crypto device */
	k_sem_init(&data->device_sem, 1, 1);

#if !defined(CONFIG_CRYPTO_ASYNC)
    /* Sempahore used when sync operations are enabled */
	k_sem_init(&data->sync_sem, 0, 1);
#endif

	IRQ_CONNECT(SMARTBOND_IRQN, SMARTBOND_IRQ_PRIO, smartbond_crypto_isr,
			DEVICE_DT_INST_GET(0), 0);

	/* Controller should be initialized once a crypyographic session is requested */
	crypto_smartbond_set_status(false);

	return 0;
}

/*
 * There is only one instance integrated on the SoC. Just in case that assumption becomes invalid
 * in the future, we use a BUILD_ASSERT().
 */
#define SMARTBOND_CRYPTO_INIT(inst)                                     \
	BUILD_ASSERT((inst) == 0,                                           \
		"multiple instances are not supported");                        \
	\
	PM_DEVICE_DT_INST_DEFINE(inst, crypto_smartbond_pm_action);	\
	\
	static struct crypto_smartbond_data crypto_smartbond_data_##inst;   \
	\
	DEVICE_DT_INST_DEFINE(0,                            \
		crypto_smartbond_init,		                    \
		PM_DEVICE_DT_INST_GET(inst),					\
		&crypto_smartbond_data_##inst, NULL,            \
		POST_KERNEL,                                    \
		CONFIG_CRYPTO_INIT_PRIORITY,                    \
		&crypto_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMARTBOND_CRYPTO_INIT)

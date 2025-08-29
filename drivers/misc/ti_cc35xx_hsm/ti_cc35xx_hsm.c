/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_hsm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/drivers/misc/ti_cc35xx_hsm/ti_cc35xx_hsm.h>

#include <inc/hw_memmap.h>
#include <inc/hw_soc_aon.h>
#include <inc/hw_hsm.h>
#include <inc/hw_hsm_non_sec.h>
#include <inc/hw_hsm_sec.h>
#include <ti/drivers/cryptoutils/hsm/HSMXXF3.h>

#include <third_party/hsmddk/include/Integration/HSMSAL/HSMSAL.h>
#include <third_party/hsmddk/include/Kit/EIP201/incl/eip201.h>
#include <third_party/hsmddk/include/Kit/DriverFramework/Device_API/incl/device_mgmt.h>

#include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintext.h>

#include <string.h>

#define SYSTEMINFO_TOKEN_WORD0      0x0F030000
#define CRYPTO_OFFICER_ID           0x4F5A3647
#define OUTPUT_TOKEN_ERROR          0x80000000
#define HSM_TRNG_RAW_KEY_ENC        0x5244
#define HSM_O_UNLOCK_CPUID0_CPUID1  0xFFFFFCFC
#define HSM_O_CPUID0_MB1_MB2_UNLOCK 0xFFFFFF77

#define HSMCRYPTO_BASE              HSM_BASE
#define HSMCRYPTO_O_MBSTA           HSM_O_MBXSTA
#define HSMCRYPTO_MBSTA_MB1IN_M     HSM_MBXSTA_INFULL1
#define HSMCRYPTO_MBSTA_MB1IN_FULL  HSM_MBXSTA_INFULL1
#define HSMCRYPTO_O_MB1IN           HSM_O_EIP130_072_MAILBOX1_IN
#define HSMCRYPTO_O_MBCTL           HSM_O_MBXCTL
#define HSMCRYPTO_MBCTL_MB1IN_FULL  HSM_MBXCTL_INFULL1
#define HSMCRYPTO_MBCTL_MB1LNK_LNK  HSM_MBXCTL_LINK1
#define HSMCRYPTO_O_MBLNKID         HSM_O_MBXLINKID
#define HSMCRYPTO_O_MBLCKOUT        HSM_O_MBXLCKOUT
#define HSMCRYPTO_MBSTA_MB1OUT_M    HSM_MBXSTA_OUTFULL1
#define HSMCRYPTO_MBSTA_MB1OUT_FULL HSM_MBXSTA_OUTFULL1
#define HSMCRYPTO_O_MB1OUT          HSM_O_EIP130_072_MAILBOX1_IN
#define HSMCRYPTO_MBCTL_MB1OUT_EMTY HSM_MBXCTL_OUTEMP1

#define HSM_RAW_RNG_BLOCK_SIZE (256U)

#define TI_CC35XX_HSM_CLK_MEM_CTRL_MSK                                                             \
	(HSM_NON_SEC_CLK_MEM_CTRL_MEM_CLK_GO_M | HSM_NON_SEC_CLK_MEM_CTRL_MEM_SLV_CLK_GO_M |       \
	 HSM_NON_SEC_CLK_MEM_CTRL_MEM_CTR_CLK_GO_M | HSM_NON_SEC_CLK_MEM_CTRL_MEM_CLK_GO_M3_M |    \
	 HSM_NON_SEC_CLK_MEM_CTRL_MEM_SLV_CLK_GO_M3_M |                                            \
	 HSM_NON_SEC_CLK_MEM_CTRL_MEM_CTR_CLK_GO_M3_M)

#define HSM_OPERATION_SEM_TIMEOUT_MS 1000

#define HSM_TRNG_RESCHEDULE_DELAY_MS 100

#if defined(CONFIG_CRYPTO_TI_CC35XX)
const AESECB_Params AESECB_defaultParams = {
	.returnBehavior = AESECB_RETURN_BEHAVIOR_POLLING,
	.callbackFxn = NULL,
	.timeout = SemaphoreP_WAIT_FOREVER,
	.custom = NULL,
};

const AESCTR_Params AESCTR_defaultParams = {
	.returnBehavior = AESCTR_RETURN_BEHAVIOR_POLLING,
	.callbackFxn = NULL,
	.timeout = SemaphoreP_WAIT_FOREVER,
	.custom = NULL,
};

const AESCCM_Params AESCCM_defaultParams = {
	.returnBehavior = AESCCM_RETURN_BEHAVIOR_POLLING,
	.callbackFxn = NULL,
	.timeout = SemaphoreP_WAIT_FOREVER,
	.custom = NULL,
};
#endif

struct entropy_ctx {
	int ret;
	struct k_sem sem;
	struct ring_buf pool;
	uint8_t buffer[CONFIG_TI_CC35XX_HSM_ENTROPY_POOL_SIZE] __aligned(4);
	struct k_work_delayable trng_work;
};

struct hsm_ti_cc35xx_data {
	struct k_sem pool_lock;
	struct k_sem operation_sem;
	struct entropy_ctx entropy;
};

static int hsm_ti_cc35xx_get_hw_caps(const struct device *dev)
{
	return (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int hsm_ti_cc35xx_init_clock(void)
{
	uintptr_t key;
	uint32_t hsm_reg_value;
	uintptr_t hsm_reg_addr;

	key = irq_lock();

	/* Disable HSM Clock */
	hsm_reg_addr = HSM_NON_SEC_BASE + HSM_NON_SEC_O_CLK_MEM_CTRL;
	hsm_reg_value = sys_read32(hsm_reg_addr) & ~TI_CC35XX_HSM_CLK_MEM_CTRL_MSK;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	hsm_reg_addr = HSM_SEC_BASE + HSM_SEC_O_CLKCTL;
	hsm_reg_value =
		sys_read32(hsm_reg_addr) &
		~(HSM_SEC_CLKCTL_CLKGO_M | HSM_SEC_CLKCTL_HIFCLKGO_M | HSM_SEC_CLKCTL_CNTCLKGO_M);
	sys_write32(hsm_reg_value, hsm_reg_addr);

	/* Initialize HSM Clock */
	hsm_reg_addr = HSM_NON_SEC_BASE + HSM_NON_SEC_O_CLK_MEM_CTRL;
	hsm_reg_value = TI_CC35XX_HSM_CLK_MEM_CTRL_MSK;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	hsm_reg_addr = HSM_SEC_BASE + HSM_SEC_O_CLKCTL;
	hsm_reg_value = sys_read32(hsm_reg_addr) | HSM_SEC_CLKCTL_CLKGO_EN |
			HSM_SEC_CLKCTL_HIFCLKGO_EN | HSM_SEC_CLKCTL_CNTCLKGO_EN;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	irq_unlock(key);

	return 0;
}

static int hsm_ti_cc35xx_unlock_cpus(void)
{
	uintptr_t key;
	uint32_t hsm_reg_value;
	uintptr_t hsm_reg_addr;

	key = irq_lock();

	/* Unlock CPUID0 and CPUID1 */
	hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBLCKOUT;
	hsm_reg_value = HSM_O_UNLOCK_CPUID0_CPUID1;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	irq_unlock(key);

	return 0;
}

static int hsm_ti_cc35xx_init_mailbox(void)
{
	uintptr_t key;
	uint32_t hsm_reg_value;
	uintptr_t hsm_reg_addr;

	key = irq_lock();

	/* Link mailbox */
	hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBSTA;
	hsm_reg_value = sys_read32(hsm_reg_addr) | HSMCRYPTO_MBCTL_MB1LNK_LNK;
	hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBCTL;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	/* Allow non-secure/secure access (Set bits 7 and 3 to 1 if we need secure access) */
	hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBLNKID;
	hsm_reg_value = (0 << HSM_MBXLINKID_LINKID1_S) | (0 << HSM_MBXLINKID_LINKID2_S) |
			(0 << HSM_MBXLINKID_PROTACC1_S) | (0 << HSM_MBXLINKID_PORTACC2_S);
	sys_write32(hsm_reg_value, hsm_reg_addr);

	/* Make sure CPU_ID=0 host can access mailbox 1 & 2 (no lockout) */
	hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBLCKOUT;
	hsm_reg_value = sys_read32(hsm_reg_addr) & HSM_O_CPUID0_MB1_MB2_UNLOCK;
	sys_write32(hsm_reg_value, hsm_reg_addr);

	irq_unlock(key);

	return 0;
}

static int hsm_ti_cc35xx_init_aic(void)
{
	Device_Handle_t gl_aic;
	uintptr_t key;

	/* Initialize AIC for EIP130 */
	key = irq_lock();
	gl_aic = Device_Find("EIP130_AIC");
	if (gl_aic != NULL) {
		/*
		 * Configure all sources for edge detect. Future improvement
		 * is to only enable the interrupts we need. We could also
		 * use EIP201_Config_Change(), but there is no adapter function()
		 */
		EIP201_SourceSettings_t settings = {
			.Source = 0xFF,
			.Config = EIP201_CONFIG_RISING_EDGE,
			.fEnable = false /* enable source only when active */
		};
		EIP201_Initialize(gl_aic, &settings, 1);
	}

	irq_unlock(key);

	return 0;
}

static void hsm_ti_cc35xx_kick_trng(struct hsm_ti_cc35xx_data *data)
{
	TRNG_Handle trng_handle = NULL;
	TRNG_Params trng_params;
	TRNG_Config trng_config;
	TRNGXXF3HSM_Object trng_object = {};
	TRNGXXF3HSM_HWAttrs trng_hw_attrs = {};
	uint8_t *ring_buf_dst;
	size_t buffer_space;

	if (ring_buf_space_get(&data->entropy.pool) == 0) {
		/* Entropy ring buffer is full. Refill not needed so far */
		return;
	} else if (k_sem_take(&data->operation_sem, K_MSEC(HSM_OPERATION_SEM_TIMEOUT_MS)) != 0) {
		return;
	}

	TRNG_init();
	TRNG_Params_init(&trng_params);
	trng_params.returnBehavior = TRNG_RETURN_BEHAVIOR_POLLING;
	trng_config.object = &trng_object;
	trng_config.hwAttrs = &trng_hw_attrs;
	trng_handle = TRNG_construct(&trng_config, &trng_params);
	buffer_space =
		ring_buf_put_claim(&data->entropy.pool, &ring_buf_dst, HSM_RAW_RNG_BLOCK_SIZE);
	TRNG_getRandomBytes(trng_handle, ring_buf_dst, buffer_space);
	ring_buf_put_finish(&data->entropy.pool, buffer_space);
	TRNG_close(trng_handle);

	k_sem_give(&data->entropy.sem);
	k_sem_give(&data->operation_sem);
	k_work_reschedule(&data->entropy.trng_work, K_MSEC(HSM_TRNG_RESCHEDULE_DELAY_MS));
}

static void hsm_ti_cc35xx_trng_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);

	struct hsm_ti_cc35xx_data *data =
		CONTAINER_OF(dwork, struct hsm_ti_cc35xx_data, entropy.trng_work);
	hsm_ti_cc35xx_kick_trng(data);
}

static int hsm_ti_cc35xx_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	struct hsm_ti_cc35xx_data *data = dev->data;
	uint32_t cnt;

	while (len) {
		k_sem_take(&data->pool_lock, K_FOREVER);
		cnt = ring_buf_get(&data->entropy.pool, buf, len);
		k_sem_give(&data->pool_lock);

		buf += cnt;
		len -= cnt;

		if (cnt == 0) {
			k_work_reschedule(&data->entropy.trng_work, K_NO_WAIT);
			k_sem_take(&data->entropy.sem, K_FOREVER);
		}
	}

	k_work_reschedule(&data->entropy.trng_work, K_MSEC(HSM_TRNG_RESCHEDULE_DELAY_MS));

	return 0;
}

#if defined(CONFIG_CRYPTO_TI_CC35XX)
static int hsm_ti_cc35xx_do_crypto_aes_ecb(struct hsm_ti_cc35xx_data *data, enum cipher_op op_type,
					   struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int result;
	AESECB_Handle aesecb_handler = NULL;
	AESECB_Config aesecb_config;
	AESECBXXF3_Object aesecb_object = {};
	CryptoKey crypto_key;
	AESECBXXF3_HWAttrs aesecb_hw_attrs = {.intPriority = (~0)};
	AESECB_Operation operation = {};
	const AESECB_Params *aesecb_params = &AESECB_defaultParams;

	if (ctx->key.bit_stream == NULL || pkt->in_buf == NULL || pkt->out_buf == NULL) {
		return -EINVAL;
	}

	AESECB_init();
	aesecb_config.object = &aesecb_object;
	aesecb_config.hwAttrs = &aesecb_hw_attrs;

	aesecb_handler = AESECB_construct(&aesecb_config, aesecb_params);

	if (aesecb_handler == NULL) {
		return -EIO;
	}

	CryptoKeyPlaintextHSM_initKey(&crypto_key, (uint8_t *)ctx->key.bit_stream, ctx->keylen);

	operation.key = &crypto_key;
	operation.input = pkt->in_buf;
	operation.inputLength = pkt->in_len;
	operation.output = pkt->out_buf;

	switch (op_type) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		result = AESECB_oneStepEncrypt(aesecb_handler, &operation);
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		result = AESECB_oneStepDecrypt(aesecb_handler, &operation);
		break;
	default:
		result = AESECB_STATUS_FEATURE_NOT_SUPPORTED;
		break;
	}

	AESECB_close(aesecb_handler);

	return result == AESECB_STATUS_SUCCESS ? 0 : -EIO;
}

static int hsm_ti_cc35xx_do_crypto_aes_ctr(struct hsm_ti_cc35xx_data *data, enum cipher_op op_type,
					   struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;
	AESCTR_Handle aesctr_handler = NULL;
	AESCTRXXF3_Object aesctr_object = {};
	AESCTR_Config aesctr_config;
	AESCTR_Operation operation = {};
	CryptoKey crypto_key;
	AESCTRXXF3_HWAttrs aesctr_hw_attrs = {.intPriority = (~0)};
	const AESCTR_Params *aesctr_params = &AESCTR_defaultParams;

	if (ctx->key.bit_stream == NULL || pkt->in_buf == NULL || pkt->out_buf == NULL ||
	    ctx->drv_sessn_state == NULL) {
		return -EINVAL;
	}
	session = ((struct hsm_ti_cc35xx_driver_session *)(ctx)->drv_sessn_state);

	AESCTR_init();
	aesctr_config.object = &aesctr_object;
	aesctr_config.hwAttrs = &aesctr_hw_attrs;

	aesctr_handler = AESCTR_construct(&aesctr_config, aesctr_params);

	if (aesctr_handler == NULL) {
		return -EIO;
	}

	CryptoKeyPlaintextHSM_initKey(&crypto_key, (uint8_t *)ctx->key.bit_stream, ctx->keylen);

	operation.key = &crypto_key;
	operation.input = pkt->in_buf;
	operation.inputLength = pkt->in_len;
	operation.output = pkt->out_buf;

	operation.initialCounter = session->ctr;

	switch (op_type) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		result = AESCTR_oneStepEncrypt(aesctr_handler, &operation);
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		result = AESCTR_oneStepDecrypt(aesctr_handler, &operation);
		break;
	default:
		result = AESCTR_STATUS_FEATURE_NOT_SUPPORTED;
		break;
	}

	AESCTR_close(aesctr_handler);

	return result == AESCTR_STATUS_SUCCESS ? 0 : -EIO;
}

static int hsm_ti_cc35xx_do_crypto_aes_ccm(struct hsm_ti_cc35xx_data *data, enum cipher_op op_type,
					   struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt)
{
	int result;
	struct hsm_ti_cc35xx_driver_session *session;
	AESCCM_Handle aesccm_handler = NULL;
	AESCCMXXF3_Object aesccm_object = {};
	AESCCM_Config aesccm_config;
	AESCCM_OneStepOperation operation = {};
	CryptoKey crypto_key;
	AESCCMXXF3_HWAttrs aesccm_hw_attrs = {.intPriority = (~0)};
	const AESCCM_Params *aesccm_params = &AESCCM_defaultParams;

	if (ctx->key.bit_stream == NULL || pkt->pkt == NULL || pkt->pkt->in_buf == NULL ||
	    pkt->pkt->out_buf == NULL || ctx->drv_sessn_state == NULL) {
		return -EINVAL;
	}
	session = ((struct hsm_ti_cc35xx_driver_session *)(ctx)->drv_sessn_state);

	if (session->nonce == NULL) {
		return -EINVAL;
	}

	AESCCM_init();
	aesccm_config.object = &aesccm_object;
	aesccm_config.hwAttrs = &aesccm_hw_attrs;

	aesccm_handler = AESCCM_construct(&aesccm_config, aesccm_params);

	if (aesccm_handler == NULL) {
		return -EIO;
	}

	CryptoKeyPlaintextHSM_initKey(&crypto_key, (uint8_t *)ctx->key.bit_stream, ctx->keylen);

	operation.key = &crypto_key;
	operation.aad = pkt->ad;
	operation.aadLength = pkt->ad_len;
	operation.input = pkt->pkt->in_buf;
	operation.output = pkt->pkt->out_buf;
	operation.inputLength = pkt->pkt->in_len;
	operation.nonce = session->nonce;
	operation.nonceLength = ctx->mode_params.ccm_info.nonce_len;
	operation.mac = pkt->tag;
	operation.macLength = ctx->mode_params.ccm_info.tag_len;

	switch (op_type) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		result = AESCCM_oneStepEncrypt(aesccm_handler, &operation);
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		result = AESCCM_oneStepDecrypt(aesccm_handler, &operation);
		break;
	default:
		result = AESCCM_STATUS_FEATURE_NOT_SUPPORTED;
		break;
	}

	AESCCM_close(aesccm_handler);

	return result == AESCCM_STATUS_SUCCESS ? 0 : -EIO;
}

static int hsm_ti_cc35xx_do_crypto(const struct device *dev, enum cipher_algo algo,
				   enum cipher_mode mode, enum cipher_op op_type,
				   struct cipher_ctx *ctx, void *pkt)
{
	struct hsm_ti_cc35xx_data *data;
	int result = 0;
	struct cipher_pkt *cphr_pkt;
	struct cipher_aead_pkt *aead_pkt;

	if (dev == NULL || dev->data == NULL || ctx == NULL || pkt == NULL) {
		return -EINVAL;
	}

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		return -ENOTSUP;
	}

	data = dev->data;

	/* Wait for HSM to be ready */
	if (k_sem_take(&data->operation_sem, K_MSEC(HSM_OPERATION_SEM_TIMEOUT_MS)) != 0) {
		return -EBUSY;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_ECB:
		cphr_pkt = (struct cipher_pkt *)pkt;
		result = hsm_ti_cc35xx_do_crypto_aes_ecb(data, op_type, ctx, cphr_pkt);
		break;

	case CRYPTO_CIPHER_MODE_CTR:
		cphr_pkt = (struct cipher_pkt *)pkt;
		result = hsm_ti_cc35xx_do_crypto_aes_ctr(data, op_type, ctx, cphr_pkt);
		break;

	case CRYPTO_CIPHER_MODE_CCM:
		aead_pkt = (struct cipher_aead_pkt *)pkt;
		result = hsm_ti_cc35xx_do_crypto_aes_ccm(data, op_type, ctx, aead_pkt);
		break;

	default:
		result = -ENOTSUP;
	}

	k_sem_give(&data->operation_sem);

	return result;
}
#endif

static int hsm_ti_cc35xx_init(const struct device *dev)
{
	struct hsm_ti_cc35xx_data *data = dev->data;

	uint32_t result = HSMXXF3_STATUS_ERROR;
	uint32_t token[] = {SYSTEMINFO_TOKEN_WORD0, CRYPTO_OFFICER_ID};

	uint32_t idx, hsm_status, hsm_reg_value;
	uintptr_t key, hsm_reg_addr;

	k_work_init_delayable(&data->entropy.trng_work, hsm_ti_cc35xx_trng_work_handler);

	/* Initialize ring buffer */
	ring_buf_init(&data->entropy.pool, sizeof(data->entropy.buffer), data->entropy.buffer);

	/* Initialize HSM clock and mailbox, then boot it */
	hsm_ti_cc35xx_init_clock();
	hsm_ti_cc35xx_unlock_cpus();
	hsm_ti_cc35xx_init_mailbox();
	hsm_ti_cc35xx_init_aic();

	key = irq_lock();

	hsm_reg_addr = HSMCRYPTO_BASE + HSM_O_MODULESTA;
	hsm_status = sys_read32(hsm_reg_addr);
	if ((hsm_status & HSM_MODULESTA_FATALERR) == 0) {
		/*
		 * At PRE_KERNEL_1 we don't use k_sleep() and other scheduling
		 * functions, so we use a busy wait loop here to wait for the
		 * input token to be processed inside the HSM.
		 * Based on experiments, it usually takes 0 cycles of the while loop
		 * to process the input token.
		 */
		hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBSTA;
		while ((sys_read32(hsm_reg_addr) & HSMCRYPTO_MBSTA_MB1IN_M) ==
		       HSMCRYPTO_MBSTA_MB1IN_FULL) {
		}

		/* Mailbox is empty so we can write the system info token to mbx1_in */
		for (idx = 0; idx < ARRAY_SIZE(token); idx++) {
			hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MB1IN + idx * 4;
			sys_write32(token[idx], hsm_reg_addr);
		}

		/* Mark mbx1 in as full */
		hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBCTL;
		hsm_reg_value = HSMCRYPTO_MBCTL_MB1IN_FULL;
		sys_write32(hsm_reg_value, hsm_reg_addr);

		/*
		 * At PRE_KERNEL_1 we don't use k_sleep() and other scheduling
		 * functions, so we use a busy wait loop here to wait for the
		 * output token to be available in mbx1_out.
		 * Based on experiments, it usually takes 12 cycles of the while loop
		 * to get response from HSM.
		 */
		hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBSTA;
		while ((sys_read32(hsm_reg_addr) & HSMCRYPTO_MBSTA_MB1OUT_M) !=
		       HSMCRYPTO_MBSTA_MB1OUT_FULL) {
		}

		/* Check for output token error */
		hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MB1OUT;
		hsm_reg_value = sys_read32(hsm_reg_addr);
		if ((hsm_reg_value & OUTPUT_TOKEN_ERROR) == 0) {
			result = HSMXXF3_STATUS_SUCCESS;
		}

		hsm_reg_addr = HSMCRYPTO_BASE + HSMCRYPTO_O_MBCTL;
		hsm_reg_value = HSMCRYPTO_MBCTL_MB1OUT_EMTY;
		sys_write32(hsm_reg_value, hsm_reg_addr);
	}

	irq_unlock(key);

	if (result != HSMXXF3_STATUS_SUCCESS) {
		return -EIO;
	}

	if (HSMSAL_Init() != HSMSAL_SUCCESS) {
		return -EIO;
	}

	/*
	 * According to the CC35XX SDK:
	 * Reset of HSM, as well as HUK (Hardware Unique Key) provisioning on CC35XX, is performed
	 * at boot time by the TI Device boot loader. As a result, HUK provisioning is not needed.
	 */

	key = irq_lock();
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	irq_unlock(key);

	k_work_reschedule(&data->entropy.trng_work, K_MSEC(HSM_TRNG_RESCHEDULE_DELAY_MS));

	return 0;
}

static DEVICE_API(hsm_ti_cc35xx, hsm_ti_cc35xx_driver_api) = {
	.get_entropy = hsm_ti_cc35xx_get_entropy,
#if defined(CONFIG_CRYPTO_TI_CC35XX)
	.do_crypto = hsm_ti_cc35xx_do_crypto,
#endif
	.get_hw_caps = hsm_ti_cc35xx_get_hw_caps,
};

static struct hsm_ti_cc35xx_data hsm_ti_cc35xx_data = {
	.pool_lock = Z_SEM_INITIALIZER(hsm_ti_cc35xx_data.pool_lock, 1, 1),
	.operation_sem = Z_SEM_INITIALIZER(hsm_ti_cc35xx_data.operation_sem, 1, 1),
	.entropy.sem = Z_SEM_INITIALIZER(hsm_ti_cc35xx_data.entropy.sem, 0, 1),
};

DEVICE_DT_INST_DEFINE(0, hsm_ti_cc35xx_init, PM_DEVICE_DT_INST_GET(0), &hsm_ti_cc35xx_data, NULL,
		      PRE_KERNEL_1, CONFIG_TI_CC35XX_HSM_INIT_PRIORITY, &hsm_ti_cc35xx_driver_api);

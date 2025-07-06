/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Design decisions:
 *  - As there is only one AES controller, this implementation is not using a device configuration.
 *
 * Notes:
 *  - If not noted otherwise, chapter numbers refer to the SiM3U1XX/SiM3C1XX reference manual
 *    (SiM3U1xx-SiM3C1xx-RM.pdf, revision 1.0)
 *  - Each DMA channel has one word of unused data (=> 3 x 4 = 12 bytes of unused RAM)
 */

#define DT_DRV_COMPAT silabs_si32_aes

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aes_silabs_si32);

#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include <SI32_AES_A_Type.h>
#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_DMACTRL_A_Type.h>
#include "SI32_DMAXBAR_A_Type.h"
#include <si32_device.h>

#include <errno.h>

#define AES_KEY_SIZE   16
#define AES_BLOCK_SIZE 16

#define DMA_CHANNEL_COUNT DT_PROP(DT_INST(0, silabs_si32_dma), dma_channels)

#define DMA_CHANNEL_ID_RX  DT_INST_DMAS_CELL_BY_NAME(0, rx, channel)
#define DMA_CHANNEL_ID_TX  DT_INST_DMAS_CELL_BY_NAME(0, tx, channel)
#define DMA_CHANNEL_ID_XOR DT_INST_DMAS_CELL_BY_NAME(0, xor, channel)

BUILD_ASSERT(DMA_CHANNEL_ID_RX < DMA_CHANNEL_COUNT, "Too few DMA channels");
BUILD_ASSERT(DMA_CHANNEL_ID_TX < DMA_CHANNEL_COUNT, "Too few DMA channels");
BUILD_ASSERT(DMA_CHANNEL_ID_XOR < DMA_CHANNEL_COUNT, "Too few DMA channels");

struct crypto_session {
	/* Decryption key needed only by ECB and CBC, and counter only by CTR. */
	union {
		uint8_t decryption_key[32]; /* only used for decryption sessions */
		uint32_t current_ctr;       /* only used for AES-CTR sessions */
	};

	bool in_use;
};

struct crypto_data {
	struct crypto_session sessions[CONFIG_CRYPTO_SI32_MAX_SESSION];
};

K_MUTEX_DEFINE(crypto_si32_in_use);
K_SEM_DEFINE(crypto_si32_work_done, 0, 1);

static struct crypto_data crypto_si32_data;

static void crypto_si32_dma_completed(const struct device *dev, void *user_data, uint32_t channel,
				      int status)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	const char *const result = status == DMA_STATUS_COMPLETE ? "succeeded" : "failed";

	switch (channel) {
	case DMA_CHANNEL_ID_RX:
		LOG_DBG("AES0 RX DMA channel %s", result);
		k_sem_give(&crypto_si32_work_done);
		break;
	case DMA_CHANNEL_ID_TX:
		LOG_DBG("AES0 TX DMA channel %s", result);
		break;
	case DMA_CHANNEL_ID_XOR:
		LOG_DBG("AES0 XOR DMA channel %s", result);
		break;
	default:
		LOG_ERR("Unknown DMA channel number: %d", channel);
		break;
	}
}

static int crypto_si32_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS |
		CAP_NO_IV_PREFIX);
}

static void crypto_si32_irq_error_handler(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* 12.3 Interrupts: An AES0 error interrupt can be generated whenever an input/output data
	 * FIFO overrun (DORF = 1) or underrun (DURF = 1) error occurs, or when an XOR data FIFO
	 * overrun (XORF = 1) occurs.
	 */
	if (SI32_AES_0->STATUS.ERRI) {
		LOG_ERR("AES0 FIFO overrun (%u), underrun (%u), XOR FIF0 overrun (%u)",
			SI32_AES_0->STATUS.DORF, SI32_AES_0->STATUS.DURF, SI32_AES_0->STATUS.XORF);
		SI32_AES_A_clear_error_interrupt(SI32_AES_0);
	}
}

/* For simplicity, the AES HW does not get turned of when not in use. */
static int crypto_si32_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Enable clock for AES HW */
	SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0, SI32_CLKCTRL_A_APBCLKG0_AES0);

	/* To use the AES0 module, firmware must first clear the RESET bit before initializing the
	 * registers.
	 */
	SI32_AES_A_reset_module(SI32_AES_0);

	__ASSERT(SI32_AES_0->CONTROL.RESET == 0, "Reset done");

	/* 12.3. Interrupts: The completion interrupt should only be used in conjunction
	 * with software mode (SWMDEN bit is set to 1) and not with DMA operations, where the DMA
	 * completion interrupt should be used.
	 */
	SI32_AES_A_disable_operation_complete_interrupt(SI32_AES_0); /* default */

	/* 12.3. Interrupts: The error interrupt should always be enabled (ERRIEN = 1), even when
	 * using the DMA with the AES module.
	 */
	SI32_AES_A_enable_error_interrupt(SI32_AES_0);

	/* Install error handler */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), crypto_si32_irq_error_handler,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* Halt AES0 module on debug breakpoint */
	SI32_AES_A_enable_stall_in_debug_mode(SI32_AES_0);

	/* For peripheral transfers, firmware should configure the peripheral for the DMA transfer
	 * and set the device’s DMA crossbar (DMAXBAR) to map a DMA channel to the peripheral.
	 */
	SI32_DMAXBAR_A_select_channel_peripheral(SI32_DMAXBAR_0, SI32_DMAXBAR_CHAN5_AES0_TX);
	SI32_DMAXBAR_A_select_channel_peripheral(SI32_DMAXBAR_0, SI32_DMAXBAR_CHAN6_AES0_RX);
	SI32_DMAXBAR_A_select_channel_peripheral(SI32_DMAXBAR_0, SI32_DMAXBAR_CHAN7_AES0_XOR);

	return 0;
}

static int crypto_si32_aes_set_key(const uint8_t *key, uint8_t key_len)
{
	const uint32_t *key_as_word = (const uint32_t *)key;

	switch (key_len) {
	case 32:
		SI32_AES_0->HWKEY7.U32 = key_as_word[7];
		SI32_AES_0->HWKEY6.U32 = key_as_word[6];
		__fallthrough;
	case 24:
		SI32_AES_0->HWKEY5.U32 = key_as_word[5];
		SI32_AES_0->HWKEY4.U32 = key_as_word[4];
		__fallthrough;
	case 16:
		SI32_AES_0->HWKEY3.U32 = key_as_word[3];
		SI32_AES_0->HWKEY2.U32 = key_as_word[2];
		SI32_AES_0->HWKEY1.U32 = key_as_word[1];
		SI32_AES_0->HWKEY0.U32 = key_as_word[0];
		break;
	default:
		LOG_ERR("Invalid key len: %" PRIu16, key_len);
		return -EINVAL;
	}

	return 0;
}

static int crypto_si32_aes_calc_decryption_key(const struct cipher_ctx *ctx,
					       uint8_t *decryption_key)
{
	uint32_t *decryption_key_word = (uint32_t *)decryption_key;
	int ret;

	ret = crypto_si32_aes_set_key(ctx->key.bit_stream, ctx->keylen);
	if (ret) {
		return ret;
	}

	LOG_INF("Generating decryption key");
	/* TODO: How much of this can be removed? */
	SI32_AES_A_write_xfrsize(SI32_AES_0, 0);
	SI32_AES_A_enable_error_interrupt(SI32_AES_0);
	SI32_AES_A_exit_cipher_block_chaining_mode(SI32_AES_0);
	SI32_AES_A_exit_counter_mode(SI32_AES_0);
	SI32_AES_A_exit_bypass_hardware_mode(SI32_AES_0);
	SI32_AES_A_select_xor_path_none(SI32_AES_0);
	SI32_AES_A_select_software_mode(SI32_AES_0);
	SI32_AES_A_select_encryption_mode(SI32_AES_0);
	SI32_AES_A_enable_key_capture(SI32_AES_0);

	for (unsigned int i = 0; i < 4; i++) {
		SI32_AES_A_write_datafifo(SI32_AES_0, 0x00000000);
	}

	SI32_AES_A_clear_operation_complete_interrupt(SI32_AES_0);
	SI32_AES_A_start_operation(SI32_AES_0);
	while (!SI32_AES_A_is_operation_complete_interrupt_pending(SI32_AES_0)) {
		/* This should not take long */
	}

	for (unsigned int i = 0; i < 4; i++) {
		SI32_AES_A_read_datafifo(SI32_AES_0);
	}

	switch (ctx->keylen) {
	case 32:
		decryption_key_word[7] = SI32_AES_0->HWKEY7.U32;
		decryption_key_word[6] = SI32_AES_0->HWKEY6.U32;
		__fallthrough;
	case 24:
		decryption_key_word[5] = SI32_AES_0->HWKEY5.U32;
		decryption_key_word[4] = SI32_AES_0->HWKEY4.U32;
		__fallthrough;
	case 16:
		decryption_key_word[3] = SI32_AES_0->HWKEY3.U32;
		decryption_key_word[2] = SI32_AES_0->HWKEY2.U32;
		decryption_key_word[1] = SI32_AES_0->HWKEY1.U32;
		decryption_key_word[0] = SI32_AES_0->HWKEY0.U32;
		break;
	default:
		LOG_ERR("Invalid key len: %" PRIu16, ctx->keylen);
		return -EINVAL;
	}

	return 0;
}

static int crypto_si32_aes_set_key_size(const struct cipher_ctx *ctx)
{
	switch (ctx->keylen) {
	case 32:
		SI32_AES_A_select_key_size_256(SI32_AES_0);
		break;
	case 24:
		SI32_AES_A_select_key_size_192(SI32_AES_0);
		break;
	case 16:
		SI32_AES_A_select_key_size_128(SI32_AES_0);
		break;
	default:
		LOG_ERR("Invalid key len: %" PRIu16, ctx->keylen);
		return -EINVAL;
	}

	return 0;
}

static void assert_dma_settings_common(struct SI32_DMADESC_A_Struct *channel_descriptor)
{
	ARG_UNUSED(channel_descriptor);

	__ASSERT(channel_descriptor->CONFIG.SRCSIZE == 2,
		 "Source size (SRCSIZE) and destination size (DSTSIZE) are 2 for a word transfer.");
	__ASSERT(channel_descriptor->CONFIG.DSTSIZE == 2,
		 "Source size (SRCSIZE) and destination size (DSTSIZE) are 2 for a word transfer.");
	__ASSERT(channel_descriptor->CONFIG.RPOWER == 2,
		 "RPOWER = 2 (4 data transfers per transaction).");
}

static void assert_dma_settings_channel_rx(struct SI32_DMADESC_A_Struct *channel_descriptor)
{
	ARG_UNUSED(channel_descriptor);

	assert_dma_settings_common(channel_descriptor);

	__ASSERT(channel_descriptor->SRCEND.U32 == (uintptr_t)&SI32_AES_0->DATAFIFO,
		 "Source end pointer set to the DATAFIFO register.");
	__ASSERT(channel_descriptor->CONFIG.DSTAIMD == 0b10,
		 "The DSTAIMD field should be set to 010b for word increments.");
	__ASSERT(channel_descriptor->CONFIG.SRCAIMD == 0b11,
		 "The SRCAIMD field should be set to 011b for no increment.");
}

static void assert_dma_settings_channel_tx(struct SI32_DMADESC_A_Struct *channel_descriptor)
{
	ARG_UNUSED(channel_descriptor);

	assert_dma_settings_common(channel_descriptor);

	__ASSERT(channel_descriptor->DSTEND.U32 == (uintptr_t)&SI32_AES_0->DATAFIFO,
		 "Destination end pointer set to the DATAFIFO register.");
	__ASSERT(channel_descriptor->CONFIG.DSTAIMD == 0b11,
		 "The DSTAIMD field should be set to 011b for no increment.");
	__ASSERT(channel_descriptor->CONFIG.SRCAIMD == 0b10,
		 "The SRCAIMD field should be set to 010b for word increments.");
}

static void assert_dma_settings_channel_xor(struct SI32_DMADESC_A_Struct *channel_descriptor)
{
	ARG_UNUSED(channel_descriptor);

	assert_dma_settings_common(channel_descriptor);

	__ASSERT(channel_descriptor->DSTEND.U32 == (uintptr_t)&SI32_AES_0->XORFIFO,
		 "Destination end pointer set to the XORFIFO register.");
	__ASSERT(channel_descriptor->CONFIG.DSTAIMD == 0b11,
		 "The DSTAIMD field should be set to 011b for no increment.");
	__ASSERT(channel_descriptor->CONFIG.SRCAIMD == 0b10,
		 "The SRCAIMD field should be set to 010b for word increments.");
}

/* Set up and start input (TX) DMA channel */
static int crypto_si32_dma_setup_tx(struct cipher_pkt *pkt, unsigned int in_buf_offset)
{
	struct dma_block_config dma_block_cfg = {};
	const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma));
	struct dma_config dma_cfg;
	int ret;

	if (!pkt->in_len) {
		LOG_WRN("Zero-sized data");
		return 0;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Data size must be 4-word aligned");
		return -EINVAL;
	}

	dma_block_cfg.block_size = pkt->in_len - in_buf_offset;
	dma_block_cfg.source_address = (uintptr_t)pkt->in_buf + in_buf_offset;
	dma_block_cfg.source_addr_adj = 0b00; /* increment */
	dma_block_cfg.dest_address = (uintptr_t)&SI32_AES_0->DATAFIFO;
	dma_block_cfg.dest_addr_adj = 0b10; /* no change (no increment) */

	dma_cfg = (struct dma_config){
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = 4, /* SiM3x1xx limitation: must match dest_data_size */
		.dest_data_size = 4,   /* DATAFIFO must be written to in word chunks (4 bytes) */
		.source_burst_length = AES_BLOCK_SIZE,
		.dest_burst_length = AES_BLOCK_SIZE,
		.block_count = 1,
		.head_block = &dma_block_cfg,
		.dma_callback = crypto_si32_dma_completed,
	};

	/* Stop channel to ensure we are not messing with an ongoing DMA operation */
	ret = dma_stop(dma, DMA_CHANNEL_ID_TX);
	if (ret) {
		LOG_ERR("TX DMA channel stop failed: %d", ret);
		return ret;
	}

	ret = dma_config(dma, DMA_CHANNEL_ID_TX, &dma_cfg);
	if (ret) {
		LOG_ERR("TX DMA channel setup failed: %d", ret);
		return ret;
	}

	ret = dma_start(dma, DMA_CHANNEL_ID_TX);
	if (ret) {
		LOG_ERR("TX DMA channel start failed: %d", ret);
		return ret;
	}

	/* Some assertions, helpful during development */
	{
		struct SI32_DMADESC_A_Struct *d =
			(struct SI32_DMADESC_A_Struct *)SI32_DMACTRL_0->BASEPTR.U32;

		/* Verify 12.5.2. General DMA Transfer Setup */
		assert_dma_settings_channel_tx(d + DMA_CHANNEL_ID_TX);

		/* Other checks */
		__ASSERT(SI32_DMACTRL_A_is_channel_enabled(SI32_DMACTRL_0, DMA_CHANNEL_ID_TX),
			 "The channel request mask (CHREQMCLR) must be cleared for the channel to "
			 "use peripheral transfers.");

		__ASSERT(SI32_DMAXBAR_0->DMAXBAR0.CH5SEL == 0b0001,
			 "0001: Service AES0 TX data requests.");
	}

	return 0;
}

/* Set up and start output (RX) DMA channel */
static int crypto_si32_dma_setup_rx(struct cipher_pkt *pkt, unsigned int in_buf_offset,
				    unsigned int out_buf_offset)
{
	struct dma_block_config dma_block_cfg = {};
	const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma));
	struct dma_config dma_cfg;
	int ret;
	uint32_t dest_address;

	if (!pkt->in_len) {
		LOG_WRN("Zero-sized data");
		return 0;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Data size must be 4-word aligned");
		return -EINVAL;
	}

	/* A NULL out_buf indicates an in-place operation. */
	if (pkt->out_buf == NULL) {
		dest_address = (uintptr_t)pkt->in_buf;
	} else {
		if ((pkt->out_buf_max - out_buf_offset) < (pkt->in_len - in_buf_offset)) {
			LOG_ERR("Output buf too small");
			return -ENOMEM;
		}

		dest_address = (uintptr_t)(pkt->out_buf + out_buf_offset);
	}

	/* Set up output (RX) DMA channel */
	dma_block_cfg.block_size = pkt->in_len - in_buf_offset;
	dma_block_cfg.source_address = (uintptr_t)&SI32_AES_0->DATAFIFO;
	dma_block_cfg.source_addr_adj = 0b10; /* no change */
	dma_block_cfg.dest_address = dest_address;
	dma_block_cfg.dest_addr_adj = 0b00; /* increment */

	dma_cfg = (struct dma_config){
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.source_data_size = 4, /* DATAFIFO must be read from in word chunks (4 bytes) */
		.dest_data_size = 4,   /* SiM3x1xx limitation: must match source_data_size */
		.source_burst_length = AES_BLOCK_SIZE,
		.dest_burst_length = AES_BLOCK_SIZE,
		.block_count = 1,
		.head_block = &dma_block_cfg,
		.dma_callback = crypto_si32_dma_completed,
	};

	/* Stop channel to ensure we are not messing with an ongoing DMA operation */
	ret = dma_stop(dma, DMA_CHANNEL_ID_RX);
	if (ret) {
		LOG_ERR("RX DMA channel stop failed: %d", ret);
		return ret;
	}

	ret = dma_config(dma, DMA_CHANNEL_ID_RX, &dma_cfg);
	if (ret) {
		LOG_ERR("RX DMA channel setup failed: %d", ret);
		return ret;
	}

	ret = dma_start(dma, DMA_CHANNEL_ID_RX);
	if (ret) {
		LOG_ERR("RX DMA channel start failed: %d", ret);
		return ret;
	}

	/* Some assertions, helpful during development */
	{
		struct SI32_DMADESC_A_Struct *d =
			(struct SI32_DMADESC_A_Struct *)SI32_DMACTRL_0->BASEPTR.U32;

		/* As per 12.5.2. General DMA Transfer Setup, check input and output channel
		 * programming
		 */
		assert_dma_settings_channel_rx(d + DMA_CHANNEL_ID_RX);

		/* Other checks */
		__ASSERT(SI32_DMACTRL_A_is_channel_enabled(SI32_DMACTRL_0, DMA_CHANNEL_ID_RX),
			 "The channel request mask (CHREQMCLR) must be cleared for the channel to "
			 "use peripheral transfers.");

		__ASSERT(SI32_DMAXBAR_0->DMAXBAR0.CH6SEL == 0b0001,
			 "0001: Service AES0 RX data requests.");
	}

	return 0;
}

/* Set up and start XOR DMA channel */
static int crypto_si32_dma_setup_xor(struct cipher_pkt *pkt)
{
	struct dma_block_config dma_block_cfg = {};
	const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma));
	struct dma_config dma_cfg;
	int ret;

	if (!pkt->in_len) {
		LOG_WRN("Zero-sized data");
		return 0;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Data size must be 4-word aligned");
		return -EINVAL;
	}

	dma_block_cfg.block_size = pkt->in_len;
	dma_block_cfg.source_address = (uintptr_t)pkt->in_buf;
	dma_block_cfg.source_addr_adj = 0b00; /* increment */
	dma_block_cfg.dest_address = (uintptr_t)&SI32_AES_0->XORFIFO;
	dma_block_cfg.dest_addr_adj = 0b10; /* no change (no increment) */

	dma_cfg = (struct dma_config){
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.source_data_size = 4, /* SiM3x1xx limitation: must match dest_data_size */
		.dest_data_size = 4,   /* DATAFIFO must be written to in word chunks (4 bytes) */
		.source_burst_length = AES_BLOCK_SIZE,
		.dest_burst_length = AES_BLOCK_SIZE,
		.block_count = 1,
		.head_block = &dma_block_cfg,
		.dma_callback = crypto_si32_dma_completed,
	};

	/* Stop channel to ensure we are not messing with an ongoing DMA operation */
	ret = dma_stop(dma, DMA_CHANNEL_ID_XOR);
	if (ret) {
		LOG_ERR("XOR DMA channel stop failed: %d", ret);
		return ret;
	}

	ret = dma_config(dma, DMA_CHANNEL_ID_XOR, &dma_cfg);
	if (ret) {
		LOG_ERR("XOR DMA channel setup failed: %d", ret);
		return ret;
	}

	ret = dma_start(dma, DMA_CHANNEL_ID_XOR);
	if (ret) {
		LOG_ERR("XOR DMA channel start failed: %d", ret);
		return ret;
	}

	/* Some assertions, helpful during development */
	{
		struct SI32_DMADESC_A_Struct *d =
			(struct SI32_DMADESC_A_Struct *)SI32_DMACTRL_0->BASEPTR.U32;

		/* As per 12.5.2. General DMA Transfer Setup, check input and output channel
		 * programming
		 */
		assert_dma_settings_channel_xor(d + DMA_CHANNEL_ID_XOR);

		/* Other checks */
		__ASSERT(SI32_DMACTRL_A_is_channel_enabled(SI32_DMACTRL_0, DMA_CHANNEL_ID_XOR),
			 "The channel request mask (CHREQMCLR) must be cleared for the channel to "
			 "use peripheral transfers.");

		__ASSERT(SI32_DMAXBAR_0->DMAXBAR0.CH7SEL == 0b0001,
			 "0001: Service AES0 XOR data requests.");
	}

	return 0;
}

static int crypto_si32_aes_ecb_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
				  const enum cipher_op op)
{
	struct crypto_session *session;
	int ret;

	if (!ctx) {
		LOG_WRN("Missing context");
		return -EINVAL;
	}

	session = (struct crypto_session *)ctx->drv_sessn_state;

	if (!pkt) {
		LOG_WRN("Missing packet");
		return -EINVAL;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Can't work on partial blocks");
		return -EINVAL;
	}

	if (pkt->in_len > 16) {
		LOG_ERR("Refusing to work on multiple ECB blocks");
		return -EINVAL;
	}

	if (pkt->in_len == 0) {
		LOG_DBG("Zero-sized packet");
		return 0;
	}

	if ((ctx->flags & CAP_INPLACE_OPS) && (pkt->out_buf != NULL)) {
		LOG_ERR("In-place must not have an out_buf");
		return -EINVAL;
	}

	/* As per 12.6.1./12.6.2. Configuring the DMA for ECB Encryption/Decryption */

	/* DMA Input Channel */
	ret = crypto_si32_dma_setup_tx(pkt, 0);
	if (ret) {
		return ret;
	}

	/* DMA Output Channel */
	ret = crypto_si32_dma_setup_rx(pkt, 0, 0);
	if (ret) {
		return ret;
	}

	/* AES Module */

	/* 1. The XFRSIZE register should be set to N-1, where N is the number of 4-word blocks. */
	SI32_AES_A_write_xfrsize(SI32_AES_0, pkt->in_len / AES_BLOCK_SIZE - 1);

	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		/* 2. The HWKEYx registers should be written with the desired key in little endian
		 * format.
		 */
		ret = crypto_si32_aes_set_key(ctx->key.bit_stream, ctx->keylen);
		if (ret) {
			return ret;
		}
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		/* 2. The HWKEYx registers should be written with decryption key value
		 * (automatically generated in the HWKEYx registers after the encryption process).
		 */
		ret = crypto_si32_aes_set_key(session->decryption_key, ctx->keylen);
		if (ret) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		return -ENOSYS;
	}

	/* 3. The CONTROL register should be set as follows: */
	{
		__ASSERT(SI32_AES_0->CONTROL.ERRIEN == 1, "a. ERRIEN set to 1.");

		/* KEYSIZE set to the appropriate number of bits for the key. */
		ret = crypto_si32_aes_set_key_size(ctx);
		if (ret) {
			return ret;
		}

		switch (op) {
		/* c. EDMD set to 1 for encryption. */
		case CRYPTO_CIPHER_OP_ENCRYPT:
			SI32_AES_A_select_encryption_mode(SI32_AES_0);
			break;
		/* c. EDMD set to 1 for DEcryption. (documentation is wrong here) */
		case CRYPTO_CIPHER_OP_DECRYPT:
			SI32_AES_A_select_decryption_mode(SI32_AES_0);
			break;
		default:
			LOG_ERR("Unsupported cipher_op: %d", op);
			return -ENOSYS;
		}

		/* d. KEYCPEN set to 1 to enable key capture at the end of the transaction. */
		SI32_AES_A_enable_key_capture(SI32_AES_0);

		/* e. The HCBCEN, HCTREN, XOREN, BEN, SWMDEN bits should all be cleared to 0. */
		SI32_AES_A_exit_cipher_block_chaining_mode(SI32_AES_0); /* Clear HCBCEN */
		SI32_AES_A_exit_counter_mode(SI32_AES_0);               /* Clear HCTREN */
		SI32_AES_A_select_xor_path_none(SI32_AES_0);            /* Clear XOREN */
		SI32_AES_A_exit_bypass_hardware_mode(SI32_AES_0);       /* Clear BEN */
		SI32_AES_A_select_dma_mode(SI32_AES_0);                 /* Clear SWMDEN*/
	}

	k_sem_reset(&crypto_si32_work_done);

	/* Once the DMA and AES settings have been set, the transfer should be started by writing 1
	 * to the XFRSTA bit.
	 */
	SI32_AES_A_start_operation(SI32_AES_0);

	ret = k_sem_take(&crypto_si32_work_done, Z_TIMEOUT_MS(50)); /* TODO: Verify 50 ms */
	if (ret) {
		LOG_ERR("AES operation timed out: %d", ret);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	return 0;
}

static int crypto_si32_aes_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
				  const enum cipher_op op, const uint8_t iv[16])
{
	struct crypto_session *session;
	int ret;
	unsigned int in_buf_offset = 0;
	unsigned int out_buf_offset = 0;

	if (!ctx) {
		LOG_WRN("Missing context");
		return -EINVAL;
	}

	session = (struct crypto_session *)ctx->drv_sessn_state;

	if (!pkt) {
		LOG_WRN("Missing packet");
		return -EINVAL;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Can't work on partial blocks");
		return -EINVAL;
	}

	if (pkt->in_len == 0) {
		LOG_WRN("Zero-sized packet");
		return 0;
	}

	/* Prefix IV to/remove from ciphertext unless CAP_NO_IV_PREFIX is set. */
	if ((ctx->flags & CAP_NO_IV_PREFIX) == 0U) {
		switch (op) {
		case CRYPTO_CIPHER_OP_ENCRYPT:
			if (pkt->out_buf_max < 16) {
				LOG_ERR("Output buf too small");
				return -ENOMEM;
			}
			if (!pkt->out_buf) {
				LOG_ERR("Missing output buf");
				return -EINVAL;
			}
			memcpy(pkt->out_buf, iv, 16);
			out_buf_offset = 16;
			break;
		case CRYPTO_CIPHER_OP_DECRYPT:
			in_buf_offset = 16;
			break;
		default:
			LOG_ERR("Unsupported cipher_op: %d", op);
			return -ENOSYS;
		}
	}

	/* As per 12.7.1.1./12.7.1.2. Configuring the DMA for Hardware CBC Encryption/Decryption */

	/* DMA Input Channel */
	ret = crypto_si32_dma_setup_tx(pkt, in_buf_offset);
	if (ret) {
		return ret;
	}

	/* DMA Output Channel */
	ret = crypto_si32_dma_setup_rx(pkt, in_buf_offset, out_buf_offset);
	if (ret) {
		return ret;
	}

	/* Initialization Vector */

	/* The initialization vector should be initialized to the HWCTRx registers. */
	SI32_AES_0->HWCTR0.U32 = *((uint32_t *)iv);
	SI32_AES_0->HWCTR1.U32 = *((uint32_t *)iv + 1);
	SI32_AES_0->HWCTR2.U32 = *((uint32_t *)iv + 2);
	SI32_AES_0->HWCTR3.U32 = *((uint32_t *)iv + 3);

	/* AES Module */

	/* 1. The XFRSIZE register should be set to N-1, where N is the number of 4-word blocks. */
	SI32_AES_A_write_xfrsize(SI32_AES_0, (pkt->in_len - in_buf_offset) / AES_BLOCK_SIZE - 1);

	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		/* 2. The HWKEYx registers should be written with the desired key in little endian
		 * format.
		 */
		ret = crypto_si32_aes_set_key(ctx->key.bit_stream, ctx->keylen);
		if (ret) {
			return ret;
		}
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		/* 2. The HWKEYx registers should be written with decryption key value
		 * (automatically generated in the HWKEYx registers after the encryption process).
		 */
		ret = crypto_si32_aes_set_key(session->decryption_key, ctx->keylen);
		if (ret) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		return -ENOSYS;
	}

	/* 3. The CONTROL register should be set as follows: */
	{
		__ASSERT(SI32_AES_0->CONTROL.ERRIEN == 1, "a. ERRIEN set to 1.");

		/* b. KEYSIZE set to the appropriate number of bits for the key. */
		ret = crypto_si32_aes_set_key_size(ctx);
		if (ret) {
			return ret;
		}

		switch (op) {
		case CRYPTO_CIPHER_OP_ENCRYPT:
			/* c. XOREN bits set to 01b to enable the XOR input path. */
			SI32_AES_A_select_xor_path_input(SI32_AES_0);

			/* d. EDMD set to 1 for encryption. */
			SI32_AES_A_select_encryption_mode(SI32_AES_0);

			/* e. KEYCPEN set to 1 to enable key capture at the end of the transaction.
			 */
			SI32_AES_A_enable_key_capture(SI32_AES_0);
			break;
		case CRYPTO_CIPHER_OP_DECRYPT:
			/* c. XOREN set to 10b to enable the XOR output path. */
			SI32_AES_A_select_xor_path_output(SI32_AES_0);

			/* d. EDMD set to 0 for decryption. */
			SI32_AES_A_select_decryption_mode(SI32_AES_0);

			/* e. KEYCPEN set to 0 to disable key capture at the end of the transaction.
			 */
			SI32_AES_A_disable_key_capture(SI32_AES_0);
			break;
		default:
			LOG_ERR("Unsupported cipher_op: %d", op);
			return -ENOSYS;
		}

		/* f. HCBCEN set to 1 to enable Hardware Cipher Block Chaining mode. */
		SI32_AES_A_enter_cipher_block_chaining_mode(SI32_AES_0);

		/* g. The HCTREN, BEN, SWMDEN bits should all be cleared to 0. */
		SI32_AES_A_exit_counter_mode(SI32_AES_0);         /* Clear HCTREN */
		SI32_AES_A_exit_bypass_hardware_mode(SI32_AES_0); /* Clear BEN */
		SI32_AES_A_select_dma_mode(SI32_AES_0);           /* Clear SWMDEN*/
	}

	k_sem_reset(&crypto_si32_work_done);

	/* Once the DMA and AES settings have been set, the transfer should be started by writing 1
	 * to the XFRSTA bit.
	 */
	SI32_AES_A_start_operation(SI32_AES_0);

	ret = k_sem_take(&crypto_si32_work_done, Z_TIMEOUT_MS(50)); /* TODO: Verify 50 ms */
	if (ret) {
		LOG_ERR("AES operation timed out: %d", ret);
		return -EIO;
	}

	/* Update passed IV buffer with new version */
	*((uint32_t *)iv) = SI32_AES_0->HWCTR0.U32;
	*((uint32_t *)iv + 1) = SI32_AES_0->HWCTR1.U32;
	*((uint32_t *)iv + 2) = SI32_AES_0->HWCTR2.U32;
	*((uint32_t *)iv + 3) = SI32_AES_0->HWCTR3.U32;

	pkt->out_len = pkt->in_len - in_buf_offset + out_buf_offset;

	return 0;
}

static int crypto_si32_aes_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t iv[12])
{
	struct crypto_session *session;
	int ret;

	if (!ctx) {
		LOG_WRN("Missing context");
		return -EINVAL;
	}

	session = (struct crypto_session *)ctx->drv_sessn_state;

	if (!pkt) {
		LOG_WRN("Missing packet");
		return -EINVAL;
	}

	if (pkt->in_len % 16) {
		LOG_ERR("Can't work on partial blocks");
		return -EINVAL;
	}

	if (pkt->in_len == 0) {
		LOG_WRN("Zero-sized packet");
		return 0;
	}

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);

	/* 12.8.1./12.8.2. Configuring the DMA for CTR Encryption/Decryption */

	/* DMA Output Channel */
	ret = crypto_si32_dma_setup_rx(pkt, 0, 0);
	if (ret) {
		goto out_unlock;
	}

	/* DMA XOR Channel */
	ret = crypto_si32_dma_setup_xor(pkt);
	if (ret) {
		goto out_unlock;
	}

	/* Initialization Vector */

	/* The initialization vector should be initialized to the HWCTRx registers. */
	switch (ctx->mode_params.ctr_info.ctr_len) {
	case 32:
		SI32_AES_0->HWCTR3.U32 = sys_cpu_to_be32(session->current_ctr);
		SI32_AES_0->HWCTR2.U32 = *((uint32_t *)iv + 2);
		SI32_AES_0->HWCTR1.U32 = *((uint32_t *)iv + 1);
		SI32_AES_0->HWCTR0.U32 = *((uint32_t *)iv);
		break;
	default:
		LOG_ERR("Unsupported counter length: %" PRIu16, ctx->mode_params.ctr_info.ctr_len);
		ret = -ENOSYS;
		goto out_unlock;
	}

	/* AES Module */

	/* 1. The XFRSIZE register should be set to N-1, where N is the number of 4-word blocks. */
	SI32_AES_A_write_xfrsize(SI32_AES_0, pkt->in_len / AES_BLOCK_SIZE - 1);

	/* 2. The HWKEYx registers should be written with the desired key in little endian format.
	 */
	ret = crypto_si32_aes_set_key(ctx->key.bit_stream, ctx->keylen);
	if (ret) {
		goto out_unlock;
	}

	/* 3. The CONTROL register should be set as follows: */
	{
		__ASSERT(SI32_AES_0->CONTROL.ERRIEN == 1, "a. ERRIEN set to 1.");

		/* b. KEYSIZE set to the appropriate number of bits for the key. */
		ret = crypto_si32_aes_set_key_size(ctx);
		if (ret) {
			goto out_unlock;
		}

		/* c. EDMD set to 1 for encryption. */
		SI32_AES_A_select_encryption_mode(SI32_AES_0);

		/* d. KEYCPEN set to 0 to disable key capture at the end of the transaction. */
		SI32_AES_A_disable_key_capture(SI32_AES_0);

		/* e. HCTREN set to 1 to enable Hardware Counter mode. */
		SI32_AES_A_enter_counter_mode(SI32_AES_0);

		/* f. XOREN set to 10b to enable the XOR output path. */
		SI32_AES_A_select_xor_path_output(SI32_AES_0);

		/* g. The HCBCEN, BEN, SWMDEN bits should all be cleared to 0. */
		SI32_AES_A_exit_cipher_block_chaining_mode(SI32_AES_0); /* Clear HCBCEN */
		SI32_AES_A_exit_bypass_hardware_mode(SI32_AES_0);       /* Clear BEN */
		SI32_AES_A_select_dma_mode(SI32_AES_0);                 /* Clear SWMDEN*/
	}

	k_sem_reset(&crypto_si32_work_done);

	/* Once the DMA and AES settings have been set, the transfer should be started by writing 1
	 * to the XFRSTA bit.
	 */
	SI32_AES_A_start_operation(SI32_AES_0);

	ret = k_sem_take(&crypto_si32_work_done, Z_TIMEOUT_MS(50)); /* TODO: Verify 50 ms */
	if (ret) {
		LOG_ERR("AES operation timed out: %d", ret);
		ret = -EIO;
		goto out_unlock;
	}

	/* Update session with new counter value */
	switch (ctx->mode_params.ctr_info.ctr_len) {
	case 32:
		session->current_ctr = sys_be32_to_cpu(SI32_AES_0->HWCTR3.U32);
		break;
	default:
		LOG_ERR("Unsupported counter length: %" PRIu16, ctx->mode_params.ctr_info.ctr_len);
		ret = -ENOSYS;
		goto out_unlock;
	}

	pkt->out_len = pkt->in_len;

out_unlock:
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_aes_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int ret;

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);
	ret = crypto_si32_aes_ecb_op(ctx, pkt, CRYPTO_CIPHER_OP_ENCRYPT);
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_aes_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	int ret;

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);
	ret = crypto_si32_aes_ecb_op(ctx, pkt, CRYPTO_CIPHER_OP_DECRYPT);
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);
	ret = crypto_si32_aes_cbc_op(ctx, pkt, CRYPTO_CIPHER_OP_ENCRYPT, iv);
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *iv)
{
	int ret;

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);
	ret = crypto_si32_aes_cbc_op(ctx, pkt, CRYPTO_CIPHER_OP_DECRYPT, iv);
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_begin_session(const struct device *dev, struct cipher_ctx *ctx,
				     const enum cipher_algo algo, const enum cipher_mode mode,
				     const enum cipher_op op)
{
	int ret = 0;
	struct crypto_session *session = 0;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("This driver supports only AES");
		return -ENOTSUP;
	}

	if (!(ctx->flags & CAP_SYNC_OPS)) {
		LOG_ERR("This driver supports only synchronous mode");
		return -ENOTSUP;
	}

	if (ctx->key.bit_stream == NULL) {
		LOG_ERR("No key provided");
		return -EINVAL;
	}

	if (ctx->keylen != 16) {
		LOG_ERR("Only AES-128 implemented");
		return -ENOSYS;
	}

	switch (mode) {
	case CRYPTO_CIPHER_MODE_CBC:
		if (ctx->flags & CAP_INPLACE_OPS && (ctx->flags & CAP_NO_IV_PREFIX) == 0) {
			LOG_ERR("In-place requires no IV prefix");
			return -EINVAL;
		}
		break;
	case CRYPTO_CIPHER_MODE_CTR:
		if (ctx->mode_params.ctr_info.ctr_len != 32U) {
			LOG_ERR("Only 32 bit counter implemented");
			return -ENOSYS;
		}
		break;
	case CRYPTO_CIPHER_MODE_ECB:
	case CRYPTO_CIPHER_MODE_CCM:
	case CRYPTO_CIPHER_MODE_GCM:
	default:
		break;
	}

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);

	for (unsigned int i = 0; i < ARRAY_SIZE(crypto_si32_data.sessions); i++) {
		if (crypto_si32_data.sessions[i].in_use) {
			continue;
		}

		LOG_INF("Claiming session %u", i);
		session = &crypto_si32_data.sessions[i];
		break;
	}

	if (!session) {
		LOG_INF("All %d session(s) in use", CONFIG_CRYPTO_SI32_MAX_SESSION);
		ret = -ENOSPC;
		goto out;
	}

	switch (op) {
	case CRYPTO_CIPHER_OP_ENCRYPT:
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_si32_aes_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_si32_aes_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_si32_aes_ctr_op;
			session->current_ctr = 0;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
		case CRYPTO_CIPHER_MODE_GCM:
		default:
			LOG_ERR("Unsupported encryption mode: %d", mode);
			ret = -ENOSYS;
			goto out;
		}
		break;
	case CRYPTO_CIPHER_OP_DECRYPT:
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_si32_aes_ecb_decrypt;
			ret = crypto_si32_aes_calc_decryption_key(ctx, session->decryption_key);
			if (ret) {
				goto out;
			}
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_si32_aes_cbc_decrypt;
			ret = crypto_si32_aes_calc_decryption_key(ctx, session->decryption_key);
			if (ret) {
				goto out;
			}
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_si32_aes_ctr_op;
			session->current_ctr = 0;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
		case CRYPTO_CIPHER_MODE_GCM:
		default:
			LOG_ERR("Unsupported decryption mode: %d", mode);
			ret = -ENOSYS;
			goto out;
		}
		break;
	default:
		LOG_ERR("Unsupported cipher_op: %d", op);
		ret = -ENOSYS;
		goto out;
	}

	session->in_use = true;
	ctx->drv_sessn_state = session;

out:
	k_mutex_unlock(&crypto_si32_in_use);

	return ret;
}

static int crypto_si32_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);

	if (!ctx) {
		LOG_WRN("Missing context");
		return -EINVAL;
	}

	struct crypto_session *session = (struct crypto_session *)ctx->drv_sessn_state;

	k_mutex_lock(&crypto_si32_in_use, K_FOREVER);
	session->in_use = false;
	k_mutex_unlock(&crypto_si32_in_use);

	return 0;
}

/* AES only, no support for hashing */
static DEVICE_API(crypto, crypto_si32_api) = {
	.query_hw_caps = crypto_si32_query_hw_caps,
	.cipher_begin_session = crypto_si32_begin_session,
	.cipher_free_session = crypto_si32_free_session,
};

DEVICE_DT_INST_DEFINE(0, crypto_si32_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_si32_api);

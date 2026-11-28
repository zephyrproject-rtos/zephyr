/*
 * Copyright (c) 2026 Nokia
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_armv8_crc32

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/crc.h>
#include <zephyr/sys/util.h>

/*
 * The CRC32 and CRC32C instructions (Armv8 CRC extension, FEAT_CRC32) process
 * the data bytes in little-endian order. The big-endian case is not handled.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_BIG_ENDIAN), "CRC32 instructions assume little-endian data");

/*
 * The instructions are emitted with inline assembly so that the driver does not
 * depend on the toolchain being invoked with a -march/-mcpu value that includes
 * the CRC extension. Clang and GCC accept the same A64 and A32/T32 mnemonics.
 * The ".arch_extension crc" directive enables the instructions in the assembler
 * only; it does not change code generation for the rest of the file.
 */
#define CRC_ARMV8_ASM(insn, crc, val, val_mod)                                                     \
	__asm__ volatile(".arch_extension crc\n\t" insn " %" val_mod "0, %" val_mod "0, %" val_mod \
			 "1"                                                                       \
			 : "+r"(crc)                                                               \
			 : "r"(val))

#if defined(CONFIG_ARM64)

#define CRC_ARMV8_WORD_SIZE 8U

static inline uint32_t crc32_u8(uint32_t crc, uint8_t val)
{
	uint32_t v = val;

	CRC_ARMV8_ASM("crc32b", crc, v, "w");
	return crc;
}

static inline uint32_t crc32_u64(uint32_t crc, uint64_t val)
{
	__asm__ volatile(".arch_extension crc\n\tcrc32x %w0, %w0, %x1" : "+r"(crc) : "r"(val));
	return crc;
}

static inline uint32_t crc32c_u8(uint32_t crc, uint8_t val)
{
	uint32_t v = val;

	CRC_ARMV8_ASM("crc32cb", crc, v, "w");
	return crc;
}

static inline uint32_t crc32c_u64(uint32_t crc, uint64_t val)
{
	__asm__ volatile(".arch_extension crc\n\tcrc32cx %w0, %w0, %x1" : "+r"(crc) : "r"(val));
	return crc;
}

#else /* AArch32 */

#define CRC_ARMV8_WORD_SIZE 4U

static inline uint32_t crc32_u8(uint32_t crc, uint8_t val)
{
	uint32_t v = val;

	CRC_ARMV8_ASM("crc32b", crc, v, "");
	return crc;
}

static inline uint32_t crc32_u32(uint32_t crc, uint32_t val)
{
	CRC_ARMV8_ASM("crc32w", crc, val, "");
	return crc;
}

static inline uint32_t crc32c_u8(uint32_t crc, uint8_t val)
{
	uint32_t v = val;

	CRC_ARMV8_ASM("crc32cb", crc, v, "");
	return crc;
}

static inline uint32_t crc32c_u32(uint32_t crc, uint32_t val)
{
	CRC_ARMV8_ASM("crc32cw", crc, val, "");
	return crc;
}

#endif /* CONFIG_ARM64 */

/*
 * Feed one machine word. The word is read with memcpy() so that the compiler
 * picks the load instruction; buffers are word-aligned before this is called.
 */
static inline uint32_t crc_word(uint32_t crc, const uint8_t *src, bool castagnoli)
{
#if defined(CONFIG_ARM64)
	uint64_t word;

	memcpy(&word, src, sizeof(word));
	return castagnoli ? crc32c_u64(crc, word) : crc32_u64(crc, word);
#else
	uint32_t word;

	memcpy(&word, src, sizeof(word));
	return castagnoli ? crc32c_u32(crc, word) : crc32_u32(crc, word);
#endif
}

/*
 * Update a reflected CRC32 state with a buffer.
 *
 * @p crc is the running register value; no initial or final XOR is applied.
 * Leading bytes are processed until the pointer is word-aligned so that the
 * word loads are aligned even on memory that does not allow unaligned access.
 */
static uint32_t crc_armv8_compute(uint32_t crc, const uint8_t *src, size_t len, bool castagnoli)
{
	while ((len > 0U) && (((uintptr_t)src & (CRC_ARMV8_WORD_SIZE - 1U)) != 0U)) {
		crc = castagnoli ? crc32c_u8(crc, *src) : crc32_u8(crc, *src);
		src++;
		len--;
	}

	while (len >= CRC_ARMV8_WORD_SIZE) {
		crc = crc_word(crc, src, castagnoli);
		src += CRC_ARMV8_WORD_SIZE;
		len -= CRC_ARMV8_WORD_SIZE;
	}

	while (len > 0U) {
		crc = castagnoli ? crc32c_u8(crc, *src) : crc32_u8(crc, *src);
		src++;
		len--;
	}

	return crc;
}

static int crc_armv8_begin(const struct device *dev, struct crc_ctx *ctx)
{
	ARG_UNUSED(dev);

	/*
	 * The instructions implement the two reflected CRC32 algorithms with a
	 * fixed polynomial. Reject other requests so that callers see an error
	 * instead of a wrong result.
	 */
	if ((ctx->type != CRC32_IEEE) && (ctx->type != CRC32_C)) {
		return -ENOTSUP;
	}

	if (ctx->reversed != (CRC_FLAG_REVERSE_INPUT | CRC_FLAG_REVERSE_OUTPUT)) {
		return -ENOTSUP;
	}

	if (ctx->polynomial != ((ctx->type == CRC32_IEEE) ? CRC32_IEEE_POLY : CRC32C_POLY)) {
		return -ENOTSUP;
	}

	ctx->result = ctx->seed;
	ctx->state = CRC_STATE_IN_PROGRESS;

	return 0;
}

static int crc_armv8_update(const struct device *dev, struct crc_ctx *ctx, const void *buffer,
			    size_t bufsize)
{
	ARG_UNUSED(dev);

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	ctx->result = crc_armv8_compute(ctx->result, buffer, bufsize, ctx->type == CRC32_C);

	return 0;
}

static int crc_armv8_finish(const struct device *dev, struct crc_ctx *ctx)
{
	ARG_UNUSED(dev);

	if (ctx->state != CRC_STATE_IN_PROGRESS) {
		return -EINVAL;
	}

	/*
	 * CRC32_IEEE requires the final XOR; the CRC32_C caller applies it for
	 * the last packet (see crc32_c()).
	 */
	if (ctx->type == CRC32_IEEE) {
		ctx->result ^= 0xFFFFFFFFU;
	}

	ctx->state = CRC_STATE_IDLE;

	return 0;
}

static DEVICE_API(crc, crc_armv8_api) = {
	.begin = crc_armv8_begin,
	.update = crc_armv8_update,
	.finish = crc_armv8_finish,
};

/* The CRC state lives in the caller's context, so the driver has no data. */
#define CRC_ARMV8_INIT(n)                                                                          \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                              \
			      CONFIG_CRC_DRIVER_INIT_PRIORITY, &crc_armv8_api);

DT_INST_FOREACH_STATUS_OKAY(CRC_ARMV8_INIT)

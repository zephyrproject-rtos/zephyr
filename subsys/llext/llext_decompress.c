/*
 * Copyright (c) 2025 CISPA Helmholtz Center for Information Security gGmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_LLEXT_COMPRESSION_LZ4
#include <zephyr/llext/buf_loader.h>
#include <lz4frame.h>
#endif /* CONFIG_LLEXT_COMPRESSION_LZ4 */

#include <zephyr/kernel.h>
#include <zephyr/llext/llext.h>
#include "llext_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#ifdef CONFIG_LLEXT_COMPRESSION_LZ4

/* need to be able to load the lz4 header at once */
#define COMPRESSED_STORAGE_BUFSIZE                                                                 \
	MAX(CONFIG_LLEXT_COMPRESSION_LOAD_INCREMENT, LZ4F_HEADER_SIZE_MAX)

#ifndef CONFIG_LLEXT_COMPRESSION_LZ4_USE_SYSTEM_HEAP
/* some arches provide very little malloc heap, so allocate from llext heap for decompression */
static void *llext_lz4_malloc(void *opaque_state, size_t size)
{
	ARG_UNUSED(opaque_state);
	return llext_alloc_data(size);
}

static void llext_lz4_free(void *opaque_state, void *address)
{
	ARG_UNUSED(opaque_state);
	return llext_free(address);
}

static const LZ4F_CustomMem llext_lz4_mem = {.customAlloc = llext_lz4_malloc,
					     .customCalloc = NULL,
					     .customFree = llext_lz4_free,
					     .opaqueState = NULL};
#endif /* !CONFIG_LLEXT_COMPRESSION_LZ4_USE_SYSTEM_HEAP */

static int llext_decompress_lz4(struct llext_loader **orig_ldr, struct llext *ext,
				const struct llext_load_param *ldr_parm)
{
	size_t compressed_size;
	int ret;
	void *decompressed_storage = NULL;
	struct llext_loader *ldr = *orig_ldr;
	/* permanent, writable storage - will be destroyed in llext_decompress_free_ext_buffer */
	struct llext_buf_loader buf_ldr = LLEXT_WRITABLE_BUF_LOADER(NULL, 0);
	uint8_t load_increment[COMPRESSED_STORAGE_BUFSIZE];
	uint8_t *iterator_in, *iterator_out;
	size_t bytes_avail, bytes_total = 0, bytes_consumed, lz4f_return, decompressed_size;
	size_t bytes_read = 0;
	LZ4F_dctx *dctx = NULL;
	LZ4F_frameInfo_t frame_info;
#ifdef CONFIG_LLEXT_COMPRESSION_LZ4_USE_SYSTEM_HEAP
	/* defaults to system heap */
	LZ4F_errorCode_t context_create_ok = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);

	if (context_create_ok) {
		LOG_ERR("Could not prepare decompression context!");
		return -ENOTSUP;
	}
#else
	dctx = LZ4F_createDecompressionContext_advanced(llext_lz4_mem, LZ4F_VERSION);

	if (!dctx) {
		LOG_ERR("Could not prepare decompression context!");
		return -ENOTSUP;
	}
#endif
	/* cleared at the end of */
	ext->decompression_context_lz4 = dctx;
	compressed_size = llext_get_size(ldr);

	if (compressed_size < LZ4F_HEADER_SIZE_MIN) {
		LOG_ERR("Compressed size %zu too small - expected at least %d bytes!",
			compressed_size, LZ4F_HEADER_SIZE_MIN);
		return -EINVAL;
	}

	/* LZ4 frame header size is variable */
	bytes_avail = MIN(compressed_size, LZ4F_HEADER_SIZE_MAX);

	/* do not want to make assumptions about internal state */
	ret = llext_seek(ldr, 0);
	if (ret != 0) {
		LOG_ERR("Could not set loader offset!");
		return ret;
	}

	ret = llext_read(ldr, load_increment, bytes_avail);
	if (ret != 0) {
		LOG_ERR("Could not read LZ4 frame header!");
		return ret;
	}

	bytes_read += bytes_avail;

	bytes_consumed = bytes_avail;

	iterator_in = load_increment;

	lz4f_return = LZ4F_getFrameInfo(dctx, &frame_info, load_increment, &bytes_consumed);

	if (LZ4F_isError(lz4f_return)) {
		LOG_ERR("Could not get LZ4 frame info!");
		return -EIO;
	}

	if (!frame_info.contentSize) {
		LOG_ERR("No contentSize provided in LZ4 frame header!");

		return -EINVAL;
	}

	decompressed_size = frame_info.contentSize;

	decompressed_storage = llext_alloc_data(decompressed_size);
	ext->decompressed_storage = decompressed_storage;

	iterator_out = decompressed_storage;

	if (!decompressed_storage) {
		LOG_ERR("Could not allocate %zu bytes for decompressed image!", decompressed_size);
		return -ENOMEM;
	}

	/* header size is variable - might have data bytes in the buffer already */
	iterator_in += bytes_consumed;
	bytes_avail -= bytes_consumed;

	do {
		size_t in_bytes = bytes_avail;

		if (bytes_avail) {
			size_t out_bytes = decompressed_size - bytes_total;

			lz4f_return = LZ4F_decompress(dctx, iterator_out, &out_bytes, iterator_in,
						      &in_bytes, NULL);

			/* produced 0 or more bytes of output */
			bytes_total += out_bytes;
			iterator_out += out_bytes;
			iterator_in += in_bytes;
			bytes_avail -= in_bytes;

			if (LZ4F_isError(lz4f_return)) {
				LOG_ERR("Could not decompress with LZ4!");
				return -EIO;
			}
		}

		if (!in_bytes && bytes_total < decompressed_size) {
			size_t bytes_to_read;
			/* LZ4 did not consume any bytes, so it MIGHT need more bytes */
			if (bytes_avail) {
				/* need to present any remaining bytes again */
				memmove(load_increment, iterator_in, bytes_avail);
				iterator_in = load_increment + bytes_avail;
				bytes_to_read = COMPRESSED_STORAGE_BUFSIZE - bytes_avail;
			} else {
				/* input buffer is completely empty */
				iterator_in = load_increment;
				bytes_to_read = COMPRESSED_STORAGE_BUFSIZE;
			}
			bytes_to_read = MIN(compressed_size - bytes_read, bytes_to_read);
			ret = llext_read(ldr, iterator_in, bytes_to_read);
			if (ret != 0) {
				LOG_ERR("Could not read LZ4 frame header!");
				return ret;
			}
			iterator_in = load_increment;
			bytes_avail += bytes_to_read;
			bytes_read += bytes_to_read;
		}
	} while (bytes_total < decompressed_size);

	/* this loader is done - read from the decompressed storage later */
	llext_finalize(ldr);

	*orig_ldr = llext_alloc_data(sizeof(struct llext_buf_loader));
	if (!*orig_ldr) {
		LOG_ERR("Could not allocate buf loader!");
		return -ENOMEM;
	}
	ext->decompression_loader = *orig_ldr;

	buf_ldr.buf = decompressed_storage;
	buf_ldr.len = decompressed_size;
	memcpy(*orig_ldr, &buf_ldr, sizeof(buf_ldr));

	return 0;
}
#endif /* CONFIG_LLEXT_COMPRESSION_LZ4 */

/*
 * Main decompression function: calls algorithm-specific decompression handler per LLEXT
 */
int llext_decompress(struct llext_loader **ldr, struct llext *ext,
		     const struct llext_load_param *ldr_parm)
{
	switch (ldr_parm->compression_type) {
	case LLEXT_COMPRESSION_TYPE_NONE:
		return 0;
#ifdef CONFIG_LLEXT_COMPRESSION_LZ4
	case LLEXT_COMPRESSION_TYPE_LZ4:
		return llext_decompress_lz4(ldr, ext, ldr_parm);
#endif
	default:
		LOG_ERR("Unknown compression type!");
		return -EINVAL;
	}
}
/*
 * Final cleanup function: destroys buffer that the extension lives in and temporary ext loader
 */
void llext_decompress_free_ext_buffer(struct llext *ext)
{
	if (ext->decompressed_storage) {
		/* this is where the extension used to live */
		llext_free(ext->decompressed_storage);
		ext->decompressed_storage = NULL;
	}
	if (ext->decompression_loader) {
		/* loader is single-use */
		llext_free(ext->decompression_loader);
		ext->decompression_loader = NULL;
	}
}

/*
 * Main decompression cleanup function: deallocates any temporary data structures
 * allocated during decompression
 */
void llext_decompress_free(int ret, struct llext *ext)
{
	if (ret != 0) {
		/* free buffer and loader on error - otherwise done from llext_unload */
		llext_decompress_free_ext_buffer(ext);
	}

#ifdef CONFIG_LLEXT_COMPRESSION_LZ4
	if (ext->decompression_context_lz4) {
		LZ4F_freeDecompressionContext((LZ4F_dctx *)ext->decompression_context_lz4);
		ext->decompression_context_lz4 = NULL;
	}
#endif /* CONFIG_LLEXT_COMPRESSION_LZ4 */
}

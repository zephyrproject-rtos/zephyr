/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/sys/util.h>

#include <zephyr/debug/coredump.h>
#include "coredump_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(coredump, CONFIG_DEBUG_COREDUMP_LOG_LEVEL);

/**
 * @file
 * @brief Simple coredump backend to store data in flash partition.
 *
 * This provides a simple backend to store coredump data in a flash
 * partition, labeled "coredump-partition" in devicetree.
 *
 * On the partition, a header is stored at the beginning with padding
 * at the end to align with flash write size. Then the actual
 * coredump data follows. The padding is to simplify the data read
 * function so that the first read of a data stream is always
 * aligned to flash write size.
 */
#define FLASH_PARTITION		coredump_partition
#define FLASH_PARTITION_ID	FIXED_PARTITION_ID(FLASH_PARTITION)

#if !FIXED_PARTITION_EXISTS(FLASH_PARTITION)
#error "Need a fixed partition named 'coredump-partition'!"

#else

/* Note that currently external memories are not supported */
#define FLASH_CONTROLLER	\
	DT_PARENT(DT_PARENT(DT_NODELABEL(FLASH_PARTITION)))

#define FLASH_WRITE_SIZE	DT_PROP(FLASH_CONTROLLER, write_block_size)
#define FLASH_BUF_SIZE \
	MAX(FLASH_WRITE_SIZE, ROUND_UP(CONFIG_DEBUG_COREDUMP_FLASH_CHUNK_SIZE, FLASH_WRITE_SIZE))
#if DT_NODE_HAS_PROP(FLASH_CONTROLLER, erase_block_size)
#define DEVICE_ERASE_BLOCK_SIZE DT_PROP(FLASH_CONTROLLER, erase_block_size)
#else
/* Device has no erase block size */
#define DEVICE_ERASE_BLOCK_SIZE 1
#endif

#define HEADER_SCRAMBLE_SIZE	ROUND_UP(sizeof(struct flash_hdr_t),	\
					 DEVICE_ERASE_BLOCK_SIZE)

#define HDR_VER			1

#define FLASH_BACKEND_SEM_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

typedef int (*data_read_cb_t)(void *arg, uint8_t *buf, size_t len);

static struct {
	/* For use with flash map */
	const struct flash_area		*flash_area;

	/* For use with streaming flash */
	struct stream_flash_ctx		stream_ctx;

	/* Checksum of data so far */
	uint16_t			checksum;

	/* Error encountered */
	int				error;
} backend_ctx;

/* Buffer used in stream flash context */
static uint8_t stream_flash_buf[FLASH_BUF_SIZE];

/* Buffer used in data_read() */
static uint8_t data_read_buf[FLASH_BUF_SIZE];

/* Semaphore for exclusive flash access */
K_SEM_DEFINE(flash_sem, 1, 1);


struct flash_hdr_t {
	/* 'C', 'D' */
	char		id[2];

	/* Header version */
	uint16_t	hdr_version;

	/* Coredump size, excluding this header */
	size_t		size;

	/* Flags */
	uint16_t	flags;

	/* Checksum */
	uint16_t	checksum;

	/* Error */
	int		error;
} __packed;


/**
 * @brief Open the flash partition.
 *
 * @return Same as flash_area_open().
 */
static int partition_open(void)
{
	int ret;

	(void)k_sem_take(&flash_sem, FLASH_BACKEND_SEM_TIMEOUT);

	ret = flash_area_open(FLASH_PARTITION_ID, &backend_ctx.flash_area);
	if (ret != 0) {
		LOG_ERR("Error opening flash partition for coredump!");

		backend_ctx.flash_area = NULL;
		k_sem_give(&flash_sem);
	}

	return ret;
}

/**
 * @brief Close the flash partition.
 */
static void partition_close(void)
{
	if (backend_ctx.flash_area == NULL) {
		return;
	}

	flash_area_close(backend_ctx.flash_area);
	backend_ctx.flash_area = NULL;

	k_sem_give(&flash_sem);
}

/**
 * @brief Read data from flash partition.
 *
 * This reads @p len bytes in the flash partition starting
 * from @p off and put into buffer pointed by @p dst if
 * @p dst is not NULL.
 *
 * If @p cb is not NULL, data being read are passed to
 * the callback for processing. Note that the data being
 * passed to callback may only be part of the data requested.
 * The end of read is signaled to the callback with NULL
 * buffer pointer and zero length as arguments.
 *
 * @param off offset of partition to begin reading
 * @param dst buffer to read data into (can be NULL)
 * @param len number of bytes to read
 * @param cb callback to process read data (can be NULL)
 * @param cb_arg argument passed to callback
 * @return 0 if successful, error otherwise.
 */
static int data_read(off_t off, uint8_t *dst, size_t len,
		     data_read_cb_t cb, void *cb_arg)
{
	int ret = 0;
	off_t offset = off;
	size_t remaining = len;
	size_t copy_sz;
	uint8_t *ptr = dst;

	if (backend_ctx.flash_area == NULL) {
		return -ENODEV;
	}

	copy_sz = FLASH_BUF_SIZE;
	while (remaining > 0) {
		if (remaining < FLASH_BUF_SIZE) {
			copy_sz = remaining;
		}

		ret = flash_area_read(backend_ctx.flash_area, offset,
				      data_read_buf, FLASH_BUF_SIZE);
		if (ret != 0) {
			break;
		}

		if (dst != NULL) {
			(void)memcpy(ptr, data_read_buf, copy_sz);
		}

		if (cb != NULL) {
			ret = (*cb)(cb_arg, data_read_buf, copy_sz);
			if (ret != 0) {
				break;
			}
		}

		ptr += copy_sz;
		offset += copy_sz;
		remaining -= copy_sz;
	}

	if (cb != NULL) {
		ret = (*cb)(cb_arg, NULL, 0);
	}

	return ret;
}

/**
 * @brief Callback to calculate checksum.
 *
 * @param arg callback argument (not being used)
 * @param buf data buffer
 * @param len number of bytes in buffer to process
 * @return 0
 */
static int cb_calc_buf_checksum(void *arg, uint8_t *buf, size_t len)
{
	int i;

	ARG_UNUSED(arg);

	for (i = 0; i < len; i++) {
		backend_ctx.checksum += buf[i];
	}

	return 0;
}

/**
 * @brief Process the stored coredump in flash partition.
 *
 * This reads the stored coredump data and processes it via
 * the callback function.
 *
 * @param cb callback to process the stored coredump data
 * @param cb_arg argument passed to callback
 * @return 1 if successful; 0 if stored coredump is not found
 *         or is not valid; error otherwise
 */
static int process_stored_dump(data_read_cb_t cb, void *cb_arg)
{
	int ret;
	struct flash_hdr_t hdr;
	off_t offset;

	ret = partition_open();
	if (ret != 0) {
		goto out;
	}

	/* Read header */
	ret = data_read(0, (uint8_t *)&hdr, sizeof(hdr), NULL, NULL);

	/* Verify header signature */
	if ((hdr.id[0] != 'C') && (hdr.id[1] != 'D')) {
		ret = 0;
		goto out;
	}

	/* Error encountered while dumping, so non-existent */
	if (hdr.error != 0) {
		ret = 0;
		goto out;
	}

	backend_ctx.checksum = 0;

	offset = ROUND_UP(sizeof(struct flash_hdr_t), FLASH_WRITE_SIZE);
	ret = data_read(offset, NULL, hdr.size, cb, cb_arg);

	if (ret == 0) {
		ret = (backend_ctx.checksum == hdr.checksum) ? 1 : 0;
	}

out:
	partition_close();

	return ret;
}

/**
 * @brief Get the stored coredump in flash partition.
 *
 * This reads the stored coredump data and copies the raw data
 * to the destination buffer.
 *
 * If the destination buffer is NULL, the offset and length are
 * ignored and the entire dump size is returned.
 *
 * @param off offset of partition to begin reading
 * @param dst buffer to read data into (can be NULL)
 * @param len number of bytes to read
 * @return dump size if successful; 0 if stored coredump is not found
 *         or is not valid; error otherwise
 */
static int get_stored_dump(off_t off, uint8_t *dst, size_t len)
{
	int ret;
	struct flash_hdr_t hdr;

	ret = partition_open();
	if (ret != 0) {
		goto out;
	}

	/* Read header */
	ret = data_read(0, (uint8_t *)&hdr, sizeof(hdr), NULL, NULL);
	if (ret != 0) {
		goto out;
	}

	/* Verify header signature */
	if ((hdr.id[0] != 'C') && (hdr.id[1] != 'D')) {
		ret = 0;
		goto out;
	}

	/* Error encountered while dumping, so non-existent */
	if (hdr.error != 0) {
		ret = 0;
		goto out;
	}

	/* Return the dump size if no destination buffer available */
	if (!dst) {
		ret = (int)hdr.size;
		goto out;
	}

	/* Offset larger than dump size */
	if (off >= hdr.size) {
		ret = 0;
		goto out;
	}

	/* Start reading the data, skip write-aligned header */
	off += ROUND_UP(sizeof(struct flash_hdr_t), FLASH_WRITE_SIZE);

	ret = data_read(off, dst, len, NULL, NULL);
	if (ret == 0) {
		ret = (int)len;
	}
out:
	partition_close();

	return ret;
}

/**
 * @brief Erase or scramble the stored coredump header from flash partition.
 *
 * This erases or scrambles the stored coredump header from the flash partition,
 * invalidating the coredump data.
 *
 * @return 0 if successful; error otherwise
 */
static int erase_coredump_header(void)
{
	int ret;

	ret = partition_open();
	if (ret == 0) {
		/* Erase or scramble header block */
		ret = flash_area_flatten(backend_ctx.flash_area, 0,
					 HEADER_SCRAMBLE_SIZE);
	}

	partition_close();

	return ret;
}

/**
 * @brief Erase the stored coredump in flash partition.
 *
 * This erases the stored coredump data from the flash partition.
 *
 * @return 0 if successful; error otherwise
 */
static int erase_flash_partition(void)
{
	int ret;

	ret = partition_open();
	if (ret == 0) {
		/* Erase whole flash partition */
		ret = flash_area_flatten(backend_ctx.flash_area, 0,
					 backend_ctx.flash_area->fa_size);
	}

	partition_close();

	return ret;
}

/**
 * @brief Start of coredump session.
 *
 * This opens the flash partition for processing.
 */
static void coredump_flash_backend_start(void)
{
	const struct device *flash_dev;
	size_t offset, header_size;
	int ret;

	ret = partition_open();

	if (ret == 0) {
		/* Erase whole flash partition */
		ret = flash_area_flatten(backend_ctx.flash_area, 0,
					 backend_ctx.flash_area->fa_size);
	}

	if (ret == 0) {
		backend_ctx.checksum = 0;

		flash_dev = flash_area_get_device(backend_ctx.flash_area);

		/*
		 * Reserve space for header from beginning of flash device.
		 * The header size is rounded up so the beginning of coredump
		 * is aligned to write size (for easier read and seek).
		 */
		header_size = ROUND_UP(sizeof(struct flash_hdr_t), FLASH_WRITE_SIZE);
		offset = backend_ctx.flash_area->fa_off + header_size;

		ret = stream_flash_init(&backend_ctx.stream_ctx, flash_dev,
					stream_flash_buf,
					sizeof(stream_flash_buf),
					offset,
					backend_ctx.flash_area->fa_size - header_size,
					NULL);
	}

	if (ret != 0) {
		LOG_ERR("Cannot start coredump!");
		backend_ctx.error = ret;
		partition_close();
	}
}

/**
 * @brief End of coredump session.
 *
 * This ends the coredump session by flushing coredump data
 * flash, and writes the header in the beginning of flash
 * related to the stored coredump data.
 */
static void coredump_flash_backend_end(void)
{
	int ret;

	struct flash_hdr_t hdr = {
		.id = {'C', 'D'},
		.hdr_version = HDR_VER,
	};

	if (backend_ctx.flash_area == NULL) {
		return;
	}

	/* Flush buffer */
	backend_ctx.error = stream_flash_buffered_write(
				&backend_ctx.stream_ctx,
				stream_flash_buf, 0, true);

	/* Write header */
	hdr.size = stream_flash_bytes_written(&backend_ctx.stream_ctx);
	hdr.checksum = backend_ctx.checksum;
	hdr.error = backend_ctx.error;
	hdr.flags = 0;

	ret = flash_area_write(backend_ctx.flash_area, 0, (void *)&hdr, sizeof(hdr));
	if (ret != 0) {
		LOG_ERR("Cannot write coredump header!");
		backend_ctx.error = ret;
	}

	if (backend_ctx.error != 0) {
		LOG_ERR("Error in coredump backend (%d)!",
			backend_ctx.error);
	}

	partition_close();
}

/**
 * @brief Write a buffer to flash partition.
 *
 * This writes @p buf into the flash partition. Note that this is
 * using the stream flash interface, so there is no need to keep
 * track of where on flash to write next.
 *
 * @param buf buffer of data to write to flash
 * @param buflen number of bytes to write
 */
static void coredump_flash_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	int i;
	size_t remaining = buflen;
	size_t copy_sz;
	uint8_t *ptr = buf;
	uint8_t tmp_buf[FLASH_BUF_SIZE];

	if ((backend_ctx.error != 0) || (backend_ctx.flash_area == NULL)) {
		return;
	}

	/*
	 * Since the system is still running, memory content is constantly
	 * changing (e.g. stack of this thread). We need to make a copy of
	 * part of the buffer, so that the checksum corresponds to what is
	 * being written.
	 */
	copy_sz = FLASH_BUF_SIZE;
	while (remaining > 0) {
		if (remaining < FLASH_BUF_SIZE) {
			copy_sz = remaining;
		}

		(void)memmove(tmp_buf, ptr, copy_sz);

		for (i = 0; i < copy_sz; i++) {
			backend_ctx.checksum += tmp_buf[i];
		}

		backend_ctx.error = stream_flash_buffered_write(
					&backend_ctx.stream_ctx,
					tmp_buf, copy_sz, false);
		if (backend_ctx.error != 0) {
			LOG_ERR("Flash write error: %d", backend_ctx.error);
			break;
		}

		ptr += copy_sz;
		remaining -= copy_sz;
	}
}

/**
 * @brief Perform query on this backend.
 *
 * @param query_id ID of query
 * @param arg argument of query
 * @return depends on query
 */
static int coredump_flash_backend_query(enum coredump_query_id query_id,
					void *arg)
{
	int ret;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = backend_ctx.error;
		break;
	case COREDUMP_QUERY_HAS_STORED_DUMP:
		ret = process_stored_dump(cb_calc_buf_checksum, NULL);
		break;
	case COREDUMP_QUERY_GET_STORED_DUMP_SIZE:
		ret = get_stored_dump(0, NULL, 0);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

/**
 * @brief Perform command on this backend.
 *
 * @param cmd_id command ID
 * @param arg argument of command
 * @return depends on query
 */
static int coredump_flash_backend_cmd(enum coredump_cmd_id cmd_id,
				      void *arg)
{
	int ret;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		backend_ctx.error = 0;
		break;
	case COREDUMP_CMD_VERIFY_STORED_DUMP:
		ret = process_stored_dump(cb_calc_buf_checksum, NULL);
		break;
	case COREDUMP_CMD_ERASE_STORED_DUMP:
		ret = erase_flash_partition();
		break;
	case COREDUMP_CMD_COPY_STORED_DUMP:
		if (arg) {
			struct coredump_cmd_copy_arg *copy_arg
				= (struct coredump_cmd_copy_arg *)arg;

			ret = get_stored_dump(copy_arg->offset,
					      copy_arg->buffer,
					      copy_arg->length);
		} else {
			ret = -EINVAL;
		}
		break;
	case COREDUMP_CMD_INVALIDATE_STORED_DUMP:
		ret = erase_coredump_header();
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}


struct coredump_backend_api coredump_backend_flash_partition = {
	.start = coredump_flash_backend_start,
	.end = coredump_flash_backend_end,
	.buffer_output = coredump_flash_backend_buffer_output,
	.query = coredump_flash_backend_query,
	.cmd = coredump_flash_backend_cmd,
};

#endif /* FIXED_PARTITION_EXISTS(coredump_partition) */

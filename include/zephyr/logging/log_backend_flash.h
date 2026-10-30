/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Read-back interface of the flash log backend.
 * @ingroup log_backend_flash
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_FLASH_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_FLASH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flash log backend
 * @defgroup log_backend_flash Flash log backend
 * @ingroup log_backend
 *
 * The flash log backend appends log messages to a flash partition, in the
 * dictionary wire format, and hands them back on request.
 *
 * Records are stored in the format emitted by log_dict_output_msg_process()
 * with additional framing. Use log_backend_flash_read() to read them out,
 * then decode with `scripts/logging/dictionary/log_parser.py` against the
 * `.elf` of the firmware that wrote the logs. The log partition has a header
 * that carries the build id from @kconfig{CONFIG_LOG_BACKEND_FLASH_BUILD_ID}.
 * It can be read out with log_backend_flash_query() so the reader can tell
 * which firmware it needs.
 *
 * The backend refuses to add to an existing log with a different build id.
 * An existing log with the same build id is appended to. Firmware built with
 * no build id at all makes no such check and appends to whatever it finds.
 *
 * A partition that has never held a log is formatted by whatever reaches it
 * first: a read, a query, or a log write. A partition that already holds a
 * a log partition header is only formatted by a call to
 * log_backend_flash_erase().
 *
 * A partition holding a log in a layout this firmware does not know is left
 * alone entirely: nothing in it can be located, not even its end, so reading it
 * and asking about it fail with @c -ENOTSUP and nothing is appended to it.
 * log_backend_flash_erase() is what recovers, and it is deliberately the only
 * way, so that a log written by other firmware is not discarded behind the back
 * of whoever might still want it.
 *
 * The log is only erased on request, never to make room. Once the partition is
 * full, further messages are dropped. Dropped messages are counted so there is
 * a trace of them, but this count is lost on reboot.
 *
 * Only flash that does not need an explicit erase is supported, such as RRAM
 * and MRAM. Memory that does is refused outright, for reading a stored log back
 * as much as for writing one.
 *
 * @{
 */

/**
 * @brief Bytes the partition header reserves for the build id.
 *
 * A build id longer than this is stored and compared truncated to it, so an id
 * has to distinguish one firmware from another within its first characters.
 */
#define LOG_BACKEND_FLASH_BUILD_ID_SIZE 56

/** @brief State of the log partition. */
struct log_backend_flash_info {
	/** Number of records that can be read back. */
	uint32_t record_cnt;
	/**
	 * Messages not written since boot or since the last
	 * log_backend_flash_erase(), whether dropped by the logging core or by
	 * this backend because the partition is full or holds a foreign log or
	 * flash writes fail. Lost over a reset.
	 */
	uint32_t dropped_cnt;
	/**
	 * Build id stored in the partition header, NUL-terminated. Empty when
	 * the firmware that wrote the log carried none.
	 */
	char build_id[LOG_BACKEND_FLASH_BUILD_ID_SIZE + 1];
	/** Bytes of the partition holding records, framing included. */
	size_t used;
	/** Bytes of the partition available for records, framing included. */
	size_t size;
	/**
	 * True when the stored log was written by the running firmware, so
	 * that its records can be formatted against it. An empty partition
	 * belongs to nobody and matches. After log_backend_flash_erase()
	 * it always matches. Firmware carrying no build id has nothing to
	 * compare, so this is always true and says nothing.
	 */
	bool build_id_matches;
};

/**
 * @brief Report the state of the log partition.
 *
 * Walks the log, so it costs time proportional to the number of records.
 *
 * @param[out] info State of the partition.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p info is NULL.
 * @retval -ENODEV The flash device backing the partition is not ready.
 * @retval -ENOTSUP The flash the partition is on is not supported, or it
 *                  was initialized in an unsupported layout.
 * @retval -errno Error reported by the flash driver.
 */
int log_backend_flash_query(struct log_backend_flash_info *info);

/**
 * @brief Read stored records.
 *
 * Copies whole records, one after the other and without the framing the
 * backend stores them with, so that what is returned is a dictionary log
 * stream. Reading stops at the end of the log, after @p max_cnt records, or
 * when the next record does not fit in @p buf, whichever comes first, and
 * reading from beyond the end of the log returns nothing rather than failing.
 *
 * Records are addressed by index, which keeps the call stateless at the cost
 * of walking the log from its start on every call.
 *
 * @param index   Index of the first record to read; 0 is the oldest.
 * @param max_cnt Most records to read, or 0 for as many as fit in @p buf.
 * @param[out] buf Destination buffer.
 * @param size    Size of @p buf in bytes.
 * @param[out] cnt Records copied. May be NULL.
 *
 * @retval nonnegative Bytes copied into @p buf.
 * @retval -EINVAL @p buf is NULL.
 * @retval -ENODEV The flash device backing the partition is not ready.
 * @retval -ENOTSUP The flash the partition is on is not supported, or it
 *                  was initialized in an unsupported layout.
 * @retval -ENOSPC The buffer is too small to store the first record.
 * @retval -errno Error reported by the flash driver.
 */
ssize_t log_backend_flash_read(uint32_t index, uint32_t max_cnt, void *buf, size_t size,
			       uint32_t *cnt);

/**
 * @brief Discard the stored log.
 *
 * Clears the partition and writes a fresh header carrying the build id of the
 * running firmware, which is also how logging is resumed after the backend has
 * found a log belonging to other firmware, or one in a layout it does not
 * know. It works whatever the partition holds.
 *
 * This function also resets the dropped counter.
 *
 * Any log messages that were added after the last log_backend_flash_read()
 * call are lost. The backend has no way to tell that a message is on its way
 * to it, so the caller is the one that has to know the log is not in use.
 *
 * @retval 0 Success.
 * @retval -ENODEV The flash device backing the partition is not ready.
 * @retval -ENOTSUP The flash the partition is on is not supported: it needs an
 *                  explicit erase before a write, or its write block size is
 *                  not one the backend can use.
 * @retval -errno Error reported by the flash driver.
 */
int log_backend_flash_erase(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_FLASH_H_ */

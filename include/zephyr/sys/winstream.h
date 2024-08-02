/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_WINSTREAM_H_
#define ZEPHYR_INCLUDE_SYS_WINSTREAM_H_

#include <stdint.h>

/** @brief Lockless shared memory byte stream IPC
 *
 * The sys_winstream utility implements a unidirectional byte stream
 * with simple read/write semantics on top of a memory region shared
 * by the writer and reader.  It requires no locking or
 * synchronization mechanisms beyond reliable ordering of memory
 * operations, and so is a good fit for use with heterogeneous shared
 * memory environments (for example, where Zephyr needs to talk to
 * other CPUs in the system running their own software).
 *
 * This object does not keep track of the last sequence number read: the
 * reader must keep that state and provide it on every read
 * operation. After reaching "steady state", 'end' and 'start' are one
 * byte apart because the buffer is always full.
 */
struct sys_winstream {
	uint32_t len;   /* Length of data[] in bytes */
	uint32_t start; /* Index of first valid byte in data[] */
	uint32_t end;   /* Index of next byte in data[] to write */
	uint32_t seq;   /* Mod-2^32 index of 'end' since stream init */
	uint8_t data[];
};

/** @brief Construct a sys_winstream from a region of memory
 *
 * This function initializes a sys_winstream in an arbitrarily-sized
 * region of memory, returning the resulting object (which is
 * guaranteed to be at the same address as the buffer).  The memory
 * must (obviously) be shared between the reader and writer, and all
 * operations to it must be coherent and consistently ordered.
 *
 * @param buf Pointer to a region of memory to contain the stream
 * @param buflen Length of the buffer, must be large enough to contain
 *               the struct sys_winstream and at least one byte of
 *               data.
 * @return A pointer to an initialized sys_winstream (same address as
 *         the buf parameter).
 */
static inline struct sys_winstream *sys_winstream_init(void *buf, int buflen)
{
	struct sys_winstream *ws = buf, tmp = { .len = buflen - sizeof(*ws) };

	*ws = tmp;
	return ws;
}

/** @brief Write bytes to a sys_winstream
 *
 * This function writes the specified number of bytes into the stream.
 * It will always return synchronously, it does not block or engage in
 * any kind of synchronization beyond memory write ordering.  Any
 * bytes passed beyond what can be stored in the buffer will be
 * silently dropped, but readers can detect their presence via the
 * sequence number.
 *
 * @param ws A sys_winstream to which to write
 * @param data Pointer to bytes to be written
 * @param len Number of bytes to write
 */
void sys_winstream_write(struct sys_winstream *ws,
			 const char *data, uint32_t len);

/** @brief Read bytes from a sys_winstream
 *
 * This function will read bytes from a sys_winstream into a specified
 * buffer.  It will always return in constant time, it does not block
 * or engage in any kind of synchronization beyond memory ordering.
 * The number of bytes read into the buffer will be returned, but note
 * that it is possible that an underflow can occur if the writer gets
 * ahead of our context.  That situation can be detected via the
 * sequence number returned via a pointer (i.e. if "*seq != old_seq +
 * return_value", an underflow occurred and bytes were dropped).
 *
 * @param ws A sys_winstream from which to read
 * @param seq A pointer to an integer containing the last sequence
 *            number read from the stream, or zero to indicate "start
 *            of stream". It is updated in place and returned for
 *            future calls and for detecting underflows.
 * @param buf A buffer into which to store the data read
 * @param buflen The length of buf in bytes
 * @return The number of bytes written into the buffer
 */
uint32_t sys_winstream_read(struct sys_winstream *ws,
			    uint32_t *seq, char *buf, uint32_t buflen);

#endif /* ZEPHYR_INCLUDE_SYS_WINSTREAM_H_ */

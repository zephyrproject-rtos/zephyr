/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_REASSEMBLY_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_REASSEMBLY_H_

#ifdef __cplusplus
extern "C" {
#endif

struct zephyr_smp_transport;

/**
 * Initialize re-assembly context within zephyr_smp_transport
 *
 * @param zst	the SMP transport.
 *
 * Note: for efficiency there is no NULL check on @p zst pointer and it is caller's responsibility
 * to validate the pointer before passing it to this function.
 */
void zephyr_smp_reassembly_init(struct zephyr_smp_transport *zst);

/**
 * Collect data to re-assembly buffer
 *
 * The function adds data to the end of current re-assembly buffer; it will allocate new buffer
 * if there isn't one allocated.
 *
 * Note: Currently the function is not able to concatenate buffers so re-assembled packet needs
 * to fit into one buffer.
 *
 * @param zst	the SMP transport;
 * @param buf	buffer with data to add;
 * @param len	length of data to add;
 *
 * Note: For efficiency there are ot NULL checks on @p zst and @p buf pointers and it is caller's
 * responsibility to make sure these are not NULL.  Also @p len should not be 0 as there is no
 * point in passing an empty fragment for re-assembly.
 *
 * @return	number of expected bytes left to complete the packet, 0 means buffer is complete
 *		and no more fragments are expected;
 *		-ENOSR if a packet length, read from header, is bigger than CONFIG_MCUMGR_BUF_SIZE,
 *		which means there is no way to fit it in the configured buffer;
 *		-EOVERFLOW if attempting to add a fragment that would make complete packet larger
 *		than expected;
 *		-ENOMEM if failed to allocate a new buffer for packet assembly;
 *		-ENODATA if the first received fragment was not big enough to figure out a size
 *		of the packet; MTU is set too low;
 */
int zephyr_smp_reassembly_collect(struct zephyr_smp_transport *zst, const void *buf, uint16_t len);

/**
 * Return number of expected bytes to complete the packet
 *
 * @param zst	the SMP transport;
 *
 * Note: for efficiency there is no NULL check on @p zst pointer and it is caller's responsibility
 * to validate the pointer before passing it to this function.
 *
 * @return	number of bytes needed to complete the packet;
 *		-EINVAL if there is no packet in re-assembly;
 */
int zephyr_smp_reassembly_expected(const struct zephyr_smp_transport *zst);

/**
 * Pass packet for further processing
 *
 * Checks if the packet has enough data to be re-assembled and passes it for further processing.
 * If successful then the re-assembly context in @p zst will indicate that there is no
 * re-assembly in progress.
 * The function can be forced to pass a data for processing even if the packet is not complete,
 * in which case it is users responsibility to use the user data, passed with the packet, to notify
 * receiving end of such case.
 *
 * @param zst	the SMP transport;
 * @param force	process anyway;
 *
 * Note: for efficiency there is no NULL check on @p zst pointer and it is caller's responsibility
 * to validate the pointer before passing it to this function.
 *
 * @return	0 on success and not forced;
 *		expected number of bytes if forced to complete buffer with not enough data;
 *		-EINVAL if there is no re-assembly in progress;
 *		-ENODATA if there is not enough data to consider packet re-assembled, it has not
 *		been passed further.
 */
int zephyr_smp_reassembly_complete(struct zephyr_smp_transport *zst, bool force);

/**
 * Drop packet and release buffer
 *
 * @param zst	the SMP transport;
 *
 * Note: for efficiency there is no NULL check on @p zst pointer and it is caller's responsibility
 * to validate the pointer before passing it to this function.
 *
 * @return	0 on success;
 *		-EINVAL if there is no re-assembly in progress.
 */
int zephyr_smp_reassembly_drop(struct zephyr_smp_transport *zst);

/**
 * Get "user data" pointer for current packet re-assembly
 *
 * @param zst	the SMP transport;
 *
 * Note: for efficiency there is no NULL check on @p zst pointer and it is caller's responsibility
 * to validate the pointer before passing it to this function.
 *
 * @return	pointer to "user data" of CONFIG_MCUMGR_BUF_USER_DATA_SIZE size;
 *		NULL if no re-assembly in progress.
 */
void *zephyr_smp_reassembly_get_ud(const struct zephyr_smp_transport *zst);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MGMT_SMP_REASSEMBLY_H_ */

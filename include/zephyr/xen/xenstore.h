/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_XENSTORE_H_
#define ZEPHYR_XEN_XENSTORE_H_

#include <sys/types.h>

struct buffered_data {
	/* Used to link buffers into singly-linked list */
	sys_snode_t node;
	/* The number of bytes that was processed for read/write */
	size_t used;
	/* The total size of message header and payload */
	size_t total_size;
	/* Buffer with header and payload */
	uint8_t *buffer;
};

struct xenstore {
	sys_slist_t out_list;
	struct buffered_data *in;
	/* Count the number of used out buffers to prevent Denial of Service attacks */
	int used_out_bufs;
	struct xenstore_domain_interface *domint;
	struct xen_domain *domain;
	struct k_sem xb_sem;
	struct k_thread thrd;
	atomic_t thrd_stop;
	evtchn_port_t remote_evtchn;
	evtchn_port_t local_evtchn;
	size_t xs_stack_slot;
	int transaction;
	int running_transaction;
	int stop_transaction_id;
	bool pending_stop_transaction;
};

typedef void (*xs_notify_cb)(char *buf, size_t len, void *param);

/**
 * @brief Read the value of a Xenstore path entry.
 *
 * Sends a read request to the Xenstore service and retrieves the
 * value at the specified path.
 *
 * @param path       Null-terminated Xenstore path string.
 * @param buf        Buffer to receive the returned value.
 * @param len        Maximum buffer length.
 * @return           Number of bytes read (excluding terminating null), or negative errno on error.
 */
ssize_t xs_read(struct xenstore *xs, const char *path, char *buf, size_t len);

/**
 * @brief List the entries in a Xenstore directory.
 *
 * Sends a directory request to the Xenstore service and populates
 * the buffer with a whitespace-separated list of entries.
 *
 * @param path       Null-terminated Xenstore directory path string.
 * @param buf        Buffer to receive the directory listing.
 * @param len        Maximum buffer length.
 * @return           Number of bytes written (excluding terminating null), or negative errno on
 * error.
 */
ssize_t xs_directory(struct xenstore *xs, const char *path, char *buf, size_t len);

ssize_t xs_watch(struct xenstore *xs, const char *path, const char *token, char *buf, size_t len);

void xs_set_notify_callback(xs_notify_cb cb, void *param);

int xs_init_xenstore(struct xenstore *xs);

#endif /* ZEPHYR_XEN_XENSTORE_H_ */

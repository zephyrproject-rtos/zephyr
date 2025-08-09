/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_XEN_XENSTORE_H_
#define ZEPHYR_XEN_XENSTORE_H_

#include <sys/types.h>

struct buffered_data {
	sys_snode_t node;  /**< Link for singly-linked list */
	size_t used;       /**< Bytes processed for read/write */
	size_t total_size; /**< Total size of header and payload */
	uint8_t *buffer;   /**< Buffer with header and payload */
};

struct xenstore {
	sys_slist_t out_list;
	struct buffered_data *in;
	int used_out_bufs; /**< Count of used out buffers (DoS prevention) */
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

/**
 * @brief Xenstore notification callback
 *
 * @param buf    Notification data buffer
 * @param len    Buffer length
 * @param param  User parameter
 */
typedef void (*xs_notify_cb)(char *buf, size_t len, void *param);

/**
 * @brief Read Xenstore path value
 *
 * @param xs    Xenstore connection context
 * @param path  Xenstore path string
 * @param buf   Buffer to receive value
 * @param len   Maximum buffer length
 *
 * @retval positive  Bytes read (excluding null terminator)
 * @retval negative  Read failed
 */
ssize_t xs_read(struct xenstore *xs, const char *path, char *buf, size_t len);

/**
 * @brief List Xenstore directory entries
 *
 * @param xs    Xenstore connection context
 * @param path  Xenstore directory path
 * @param buf   Buffer to receive directory listing
 * @param len   Maximum buffer length
 *
 * @retval positive  Bytes written (excluding final null)
 * @retval 0         Directory empty
 * @retval negative  Listing failed
 */
ssize_t xs_directory(struct xenstore *xs, const char *path, char *buf, size_t len);

/**
 * @brief Watch Xenstore path for changes
 *
 * @param xs     Xenstore connection context
 * @param path   Xenstore path to watch
 * @param token  Unique token string for this watch
 * @param buf    Buffer to receive watch confirmation
 * @param len    Maximum buffer length
 *
 * @return Bytes written or negative on error
 */
ssize_t xs_watch(struct xenstore *xs, const char *path, const char *token, char *buf, size_t len);

/**
 * @brief Set Xenstore notification callback
 *
 * @param cb     Callback function (NULL to disable)
 * @param param  User parameter for callback
 */
void xs_set_notify_callback(xs_notify_cb cb, void *param);

/**
 * @brief Initialize Xenstore connection
 *
 * @param xs  Xenstore structure to initialize
 *
 * @retval 0         Success
 * @retval negative  Initialization failed
 */
int xs_init_xenstore(struct xenstore *xs);

#endif /* ZEPHYR_XEN_XENSTORE_H_ */

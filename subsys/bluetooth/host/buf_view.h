/** @file
 *  @brief Bluetooth "view" buffer abstraction
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_BUF_VIEW_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_BUF_VIEW_H_

#include <zephyr/net/buf.h>


struct bt_buf_view_meta {
	struct net_buf *parent;
	/* saves the data pointers while the parent buffer is locked. */
	struct net_buf_simple backup;
};

/** @internal
 *
 *  @brief Create a "view" or "window" into an existing buffer.
 *  - enforces one active view at a time per-buffer
 *  -> this restriction enables prepending data (ie. for headers)
 *  - forbids appending data to the view
 *  - pulls the size of the view from said buffer.
 *
 *  The "virtual buffer" that is generated has to be allocated from a buffer
 *  pool. This is to allow refcounting and attaching a destroy callback. The
 *  configured size of the buffers in that pool should be zero-length.
 *
 *  The user-data size is application-dependent, but should be minimized to save
 *  memory. user_data is not used by the view API.
 *
 *  The view mechanism needs to store extra metadata in order to unlock the
 *  original buffer when the view is destroyed.
 *
 *  The storage and allocation of the view buf pool and the view metadata is the
 *  application's responsibility.
 *
 *  @note The `headroom` param is only used for __ASSERT(). The idea is that
 *  it's easier to debug a headroom assert failure at allocation time, rather
 *  than later down the line when a lower layer tries to add its headers and
 *  fails.
 *
 *  @param view         Uninitialized "View" buffer
 *  @param parent       Buffer data is pulled from into `view`
 *  @param len          Amount to pull
 *  @param meta         Uninitialized metadata storage
 *
 *  @return view if the operation was successful. NULL on error.
 */
struct net_buf *bt_buf_make_view(struct net_buf *view,
				 struct net_buf *parent,
				 size_t len,
				 struct bt_buf_view_meta *meta);

/** @internal
 *
 *  @brief Check if `parent` has view.
 *
 *  If `parent` has been passed to @ref bt_buf_make_view() and the resulting
 *  view buffer has not been destroyed.
 */
bool bt_buf_has_view(const struct net_buf *parent);

/** @internal
 *
 *  @brief Destroy the view buffer
 *
 *  Equivalent of @ref net_buf_destroy.
 *  It is mandatory to call this from the view pool's `destroy` callback.
 *
 *  This frees the parent buffer, and allows calling @ref bt_buf_make_view again.
 *  The metadata is also freed for re-use.
 *
 *  @param view View to destroy
 *  @param meta Meta that was given to @ref bt_buf_make_view
 */
void bt_buf_destroy_view(struct net_buf *view, struct bt_buf_view_meta *meta);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_BUF_VIEW_H_ */

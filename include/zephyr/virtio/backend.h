/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct virtio_mmio_backend_bus_api {
	int (*register_device)(const struct device *parent, const struct device *child,
			       uint32_t queue_id,
			       void (*process)(void *buf, uint32_t len, void *ctx), void *ctx);
};

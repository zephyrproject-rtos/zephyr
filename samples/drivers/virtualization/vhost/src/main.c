/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/vhost.h>
#include <zephyr/drivers/vhost/vringh.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vhost);

struct vhost_iovec riovec[16];
struct vhost_iovec wiovec[16];

struct vringh_iov riov = {
	.iov = riovec,
	.max_num = 16,
};

struct vringh_iov wiov = {
	.iov = wiovec,
	.max_num = 16,
};

struct vringh vrh_inst;

static void vringh_kick_handler(struct vringh *vrh)
{
	LOG_DBG("%s: queue_id=%lu", __func__, vrh->queue_id);
	uint16_t head;

	while (true) {
		int ret = vringh_getdesc(vrh, &riov, &wiov, &head);

		if (ret < 0) {
			LOG_ERR("vringh_getdesc failed: %d", ret);
			return;
		}

		if (ret == 0) {
			return;
		}

		/* Process writable iovecs */
		for (uint32_t s = 0; s < wiov.used; s++) {
			uint8_t *dst = wiov.iov[s].iov_base;
			uint32_t len = wiov.iov[s].iov_len;

			LOG_DBG("%s: addr=%p len=%u", __func__, dst, len);

			for (uint32_t i = 0; i < len; i++) {
				sys_write8(i, (mem_addr_t)&dst[i]);
			}
		}

		barrier_dmem_fence_full();

		uint32_t total_len = 0;

		for (uint32_t i = 0; i < wiov.used; i++) {
			total_len += wiov.iov[i].iov_len;
		}

		vringh_complete(vrh, head, total_len);

		if (vringh_need_notify(vrh) > 0) {
			vringh_notify(vrh);
		}

		/* Reset iovecs for next iteration */
		vringh_iov_reset(&riov);
		vringh_iov_reset(&wiov);
	}
}

void queue_ready_handler(const struct device *dev, uint16_t qid, void *data)
{
	LOG_DBG("%s(dev=%p, qid=%u, data=%p)", __func__, dev, qid, data);

	/* Initialize iovecs before descriptor processing */
	vringh_iov_init(&riov, riov.iov, riov.max_num);
	vringh_iov_init(&wiov, wiov.iov, wiov.max_num);

	int err = vringh_init_device(&vrh_inst, dev, qid, vringh_kick_handler);

	if (err) {
		LOG_ERR("vringh_init_device failed: %d", err);
		return;
	}
}

int main(void)
{
	const struct device *device = DEVICE_DT_GET(DT_NODELABEL(vhost));

	if (!device_is_ready(device)) {
		LOG_ERR("VHost device not ready");
		return -ENODEV;
	}

	LOG_INF("VHost device ready");
	vhost_register_virtq_ready_cb(device, queue_ready_handler, (void *)device);

	LOG_INF("VHost sample application started, waiting for guest connections...");
	k_sleep(K_FOREVER);

	return 0;
}

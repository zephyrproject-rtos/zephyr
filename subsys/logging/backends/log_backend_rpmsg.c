/*
 * Copyright (c) 2024 Maurer Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/drivers/remoteproc/rpmsg_virtio_device.h>
#include <openamp/open_amp.h>

#define EPT_NODE DT_CHOSEN(zephyr_log_rpmsg)
#define VDEV_NODE DT_BUS(EPT_NODE)

static volatile bool ept_ready;
static volatile bool in_panic;
static uint32_t log_format_current;
static uint8_t rpmsg_output_buf[128];
static struct rpmsg_endpoint ept_log;
static const struct device *vdev = DEVICE_DT_GET(VDEV_NODE);

static int rpmsg_log_callback(struct rpmsg_endpoint *ept, void *data,
							  size_t len, uint32_t src, void *priv)
{
	static bool set;

	ARG_UNUSED(ept);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(src);
	ARG_UNUSED(priv);

	if (!set) {
		ept_ready = true;
		set = true;
	}

	return RPMSG_SUCCESS;
}


static void rpmsg_log_unbind(struct rpmsg_endpoint *ept)
{
	ept_ready = false;
}


#if defined(CONFIG_RAM_CONSOLE)
void *__printk_get_hook(void);
#endif

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	if (ept_ready && !in_panic) {

		int r = rpmsg_send(&ept_log, data, length);

		if (r >= 0)
			return length;
	}

#if defined(CONFIG_RAM_CONSOLE)
	int (*_char_out)(int c) = __printk_get_hook();

	if (_char_out) {
		for (int cnt = 0; cnt < length; cnt++) {
			_char_out(data[cnt]);
		}
	}
#endif

	return length;
}

LOG_OUTPUT_DEFINE(log_output_rpmsg, char_out, rpmsg_output_buf, sizeof(rpmsg_output_buf));

static void lb_rpmsg_process(const struct log_backend *backend,
					union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();
	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_rpmsg, &msg->log, flags);

}


static void lb_rpmsg_panic(struct log_backend const *backend)
{
	in_panic = true;
	log_backend_std_panic(&log_output_rpmsg);
}


static void lb_rpmsg_dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_rpmsg, cnt);
}


static int lb_rpmsg_format_set(const struct log_backend *backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}



static void lb_rpmsg_init(const struct log_backend *backend)
{
	struct rpmsg_device *rdev = openamp_get_rpmsg_device(vdev);
	uint32_t dest_addr = RPMSG_ADDR_ANY;
	int ret;

#if DT_REG_HAS_IDX(EPT_NODE, 1)
	dest_addr = DT_REG_ADDR_BY_IDX(EPT_NODE, 1);
#endif

	ret = rpmsg_create_ept(&ept_log, rdev, DT_PROP(EPT_NODE, type),
				  DT_REG_ADDR(EPT_NODE), dest_addr,
				  rpmsg_log_callback, rpmsg_log_unbind);

	if (ret) {
		return;
	}
}

const struct log_backend_api log_backend_rpmsg_api = {
							.process = lb_rpmsg_process,
							.dropped = lb_rpmsg_dropped,
							.panic = lb_rpmsg_panic,
							.init = lb_rpmsg_init,
							.format_set = lb_rpmsg_format_set,
						};

LOG_BACKEND_DEFINE(backend_rpmsg_service,
		   log_backend_rpmsg_api,
		   true);

/* Copyright (c) 2022, Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <adsp_memory.h>
#include <adsp_shim.h>
#include <intel_adsp_ipc.h>
#include <mem_window.h>

/* Matches SOF_IPC_MSG_MAX_SIZE, though in practice nothing anywhere
 * near that big is ever sent.  Should maybe consider making this a
 * kconfig to avoid waste.
 */
#define MAX_MSG 384

/* Note: these addresses aren't flexible!  We require that they match
 * current SOF ipc3/4 layout, which means that:
 *
 * + Buffer addresses are 4k-aligned (this is a hardware requirement)
 * + Inbuf must be 4k after outbuf, with no use of the intervening memory
 * + Outbuf must be 4k after the start of win0 (this is where the host driver looks)
 *
 * One side effect is that the word "before" MSG_INBUF is owned by our
 * code too, and can be used for a nice trick below.
 */

/* host windows */
#define DMWBA(win_base) (win_base + 0x0)
#define DMWLO(win_base) (win_base + 0x4)


struct ipm_cavs_host_data {
	ipm_callback_t callback;
	void *user_data;
	bool enabled;
};

/* Note: this call is unsynchronized. The IPM docs are silent as to
 * whether this is required, and the SOF code that will be using this
 * is externally synchronized already.
 */
static int send(const struct device *dev, int wait, uint32_t id,
		const void *data, int size)
{
	const struct device *mw0 = DEVICE_DT_GET(DT_NODELABEL(mem_window0));

	if (!device_is_ready(mw0)) {
		return -ENODEV;
	}
	const struct mem_win_config *mw0_config = mw0->config;
	uint32_t *buf = (uint32_t *)arch_xtensa_uncached_ptr((void *)((uint32_t)mw0_config->mem_base
		+ CONFIG_IPM_CAVS_HOST_OUTBOX_OFFSET));

	if (!intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV)) {
		return -EBUSY;
	}

	if ((size < 0) || (size > MAX_MSG)) {
		return -EMSGSIZE;
	}

	if ((id & 0xc0000000) != 0) {
		/* cAVS IDR register has only 30 usable bits */
		return -EINVAL;
	}

	uint32_t ext_data = 0;

	/* Protocol variant (used by SOF "ipc4"): store the first word
	 * of the message in the IPC scratch registers
	 */
	if (IS_ENABLED(CONFIG_IPM_CAVS_HOST_REGWORD) && size >= 4) {
		ext_data = ((uint32_t *)data)[0];
		data = &((const uint32_t *)data)[1];
		size -= 4;
	}

	memcpy(buf, data, size);

	int ret = intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, id, ext_data);

	/* The IPM docs call for "busy waiting" here, but in fact
	 * there's a blocking synchronous call available that might be
	 * better. But then we'd have to check whether we're in
	 * interrupt context, and it's not clear to me that SOF would
	 * benefit anyway as all its usage is async. This is OK for
	 * now.
	 */
	if (ret == -EBUSY && wait) {
		while (!intel_adsp_ipc_is_complete(INTEL_ADSP_IPC_HOST_DEV)) {
			k_busy_wait(1);
		}
	}

	return ret;
}

static bool ipc_handler(const struct device *dev, void *arg,
			uint32_t data, uint32_t ext_data)
{
	ARG_UNUSED(arg);
	struct device *ipmdev = arg;
	struct ipm_cavs_host_data *devdata = ipmdev->data;
	const struct device *mw1 = DEVICE_DT_GET(DT_NODELABEL(mem_window1));

	if (!device_is_ready(mw1)) {
		return -ENODEV;
	}
	const struct mem_win_config *mw1_config = mw1->config;
	uint32_t *msg = arch_xtensa_uncached_ptr((void *)mw1_config->mem_base);

	/* We play tricks to leave one word available before the
	 * beginning of the SRAM window, this way the host can see the
	 * same offsets it does with the original ipc4 protocol
	 * implementation, but here in the firmware we see a single
	 * contiguous buffer.  See above.
	 */
	if (IS_ENABLED(CONFIG_IPM_CAVS_HOST_REGWORD)) {
		msg = &msg[-1];
		msg[0] = ext_data;
	}

	if (devdata->enabled && (devdata->callback != NULL)) {
		devdata->callback(ipmdev, devdata->user_data,
				  data & 0x3fffffff, msg);
	}

	/* Return false for async handling */
	return !IS_ENABLED(IPM_CALLBACK_ASYNC);
}

static int max_data_size_get(const struct device *ipmdev)
{
	return MAX_MSG;
}

static uint32_t max_id_val_get(const struct device *ipmdev)
{
	/* 30 user-writable bits in cAVS IDR register */
	return 0x3fffffff;
}

static void register_callback(const struct device *port,
			      ipm_callback_t cb,
			      void *user_data)
{
	struct ipm_cavs_host_data *data = port->data;

	data->callback = cb;
	data->user_data = user_data;
}

static int set_enabled(const struct device *ipmdev, int enable)
{
	/* This protocol doesn't support any kind of queuing, and in
	 * fact will stall if a message goes unacknowledged.  Support
	 * it as best we can by gating the callbacks only.  That will
	 * allow the DONE notifications to proceed as normal, at the
	 * cost of dropping any messages received while not "enabled"
	 * of course.
	 */
	struct ipm_cavs_host_data *data = ipmdev->data;

	data->enabled = enable;
	return 0;
}

static void complete(const struct device *ipmdev)
{
	intel_adsp_ipc_complete(INTEL_ADSP_IPC_HOST_DEV);
}

static int init(const struct device *dev)
{
	struct ipm_cavs_host_data *data = dev->data;

	const struct device *mw1 = DEVICE_DT_GET(DT_NODELABEL(mem_window1));

	if (!device_is_ready(mw1)) {
		return -ENODEV;
	}
	const struct mem_win_config *mw1_config = mw1->config;
	/* Initialize hardware SRAM window.  SOF will give the host 8k
	 * here, let's limit it to just the memory we're using for
	 * futureproofing.
	 */

	sys_write32(ROUND_UP(MAX_MSG, 8) | 0x7, DMWLO(mw1_config->base_addr));
	sys_write32((mw1_config->mem_base | ADSP_DMWBA_ENABLE), DMWBA(mw1_config->base_addr));

	intel_adsp_ipc_set_message_handler(INTEL_ADSP_IPC_HOST_DEV, ipc_handler, (void *)dev);

	data->enabled = true;
	return 0;
}

static const struct ipm_driver_api api = {
	.send = send,
	.max_data_size_get = max_data_size_get,
	.max_id_val_get = max_id_val_get,
	.register_callback = register_callback,
	.set_enabled = set_enabled,
	.complete = complete,
};

static struct ipm_cavs_host_data data;

DEVICE_DEFINE(ipm_cavs_host, "ipm_cavs_host", init, NULL, &data, NULL,
	      PRE_KERNEL_2, 1, &api);

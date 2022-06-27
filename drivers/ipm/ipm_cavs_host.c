/* Copyright (c) 2022, Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <cavs-mem.h>
#include <cavs-shim.h>
#include <cavs_ipc.h>

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
#define BUFPTR(ptr, off) ((uint32_t *) \
	arch_xtensa_uncached_ptr((void *)((uint32_t)ptr + off)))
#define MSG_INBUF BUFPTR(L2_SRAM_BASE, CONFIG_IPM_CAVS_HOST_INBOX_OFFSET)
#define MSG_OUTBUF BUFPTR(HP_SRAM_WIN0_BASE, CONFIG_IPM_CAVS_HOST_OUTBOX_OFFSET)

struct ipm_cavs_host_data {
	ipm_callback_t callback;
	void *user_data;
	bool enabled;
};

/* Note: this call is unsynchronized.  The IPM docs are silent as to
 * whether this is required, and the SOF code that will be using this
 * is externally synchronized already.
 */
static int send(const struct device *ipmdev, int wait, uint32_t id,
		const void *data, int size)
{
	if (!cavs_ipc_is_complete(CAVS_HOST_DEV)) {
		return -EBUSY;
	}

	if (size > MAX_MSG) {
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

	memcpy(MSG_OUTBUF, data, size);

	bool ok = cavs_ipc_send_message(CAVS_HOST_DEV, id, ext_data);

	/* The IPM docs call for "busy waiting" here, but in fact
	 * there's a blocking synchronous call available that might be
	 * better.  But then we'd have to check whether we're in
	 * interrupt context, and it's not clear to me that SOF would
	 * benefit anyway as all its usage is async.  This is OK for
	 * now.
	 */
	if (ok && wait) {
		while (!cavs_ipc_is_complete(CAVS_HOST_DEV)) {
			k_busy_wait(1);
		}
	}

	return ok ? 0 : -EBUSY;
}

static bool ipc_handler(const struct device *dev, void *arg,
			uint32_t data, uint32_t ext_data)
{
	ARG_UNUSED(arg);
	struct device *ipmdev = arg;
	struct ipm_cavs_host_data *devdata = ipmdev->data;
	uint32_t *msg = MSG_INBUF;

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
	cavs_ipc_complete(CAVS_HOST_DEV);
}

static int init(const struct device *dev)
{
	struct ipm_cavs_host_data *data = dev->data;

	/* Initialize hardware SRAM window.  SOF will give the host 8k
	 * here, let's limit it to just the memory we're using for
	 * futureproofing.
	 */
	CAVS_WIN[1].dmwlo = ROUND_UP(MAX_MSG, 8);
	CAVS_WIN[1].dmwba = ((uint32_t) MSG_INBUF) | CAVS_DMWBA_ENABLE;

	cavs_ipc_set_message_handler(CAVS_HOST_DEV, ipc_handler, (void *)dev);

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

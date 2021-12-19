/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arm SiP services driver provides the capability to send the
 * SMC/HVC call from kernel to hypervisor/secure monitor firmware
 * running at EL2/EL3.
 *
 * Only allow one SMC and one HVC driver per system.
 */

#define DT_DRV_COMPAT arm_sip_svc

#include <device.h>
#include <drivers/sip_svc/sip_svc.h>

static uint32_t sip_svc_do_register(struct sip_svc_controller *ctrl,
				    void *priv_data)
{
	return sip_svc_ll_register(ctrl, priv_data);
}

static int sip_svc_do_unregister(struct sip_svc_controller *ctrl,
				 uint32_t c_token)
{
	return sip_svc_ll_unregister(ctrl, c_token);
}

static int sip_svc_do_open(struct sip_svc_controller *ctrl,
			   uint32_t c_token,
			   uint32_t timeout_us)
{
	return sip_svc_ll_open(ctrl, c_token, timeout_us);
}

static int sip_svc_do_close(struct sip_svc_controller *ctrl,
			    uint32_t c_token)
{
	return sip_svc_ll_close(ctrl, c_token);
}

static int sip_svc_do_send(struct sip_svc_controller *ctrl,
			   uint32_t c_token,
			   char *req,
			   int size,
			   sip_svc_cb_fn cb)
{
	return sip_svc_ll_send(ctrl, c_token, req, size, cb);
}

static void *sip_svc_do_get_priv_data(struct sip_svc_controller *ctrl,
				      uint32_t c_token)
{
	return sip_svc_ll_get_priv_data(ctrl, c_token);
}

static void sip_svc_do_print_info(struct sip_svc_controller *ctrl)
{
	sip_svc_ll_print_info(ctrl);
}

static int sip_svc_init(const struct device *dev)
{
	return sip_svc_ll_init((struct sip_svc_controller *)dev->data);
}

static const struct sip_svc_driver_api sip_svc_api = {
	.reg = sip_svc_do_register,
	.unreg = sip_svc_do_unregister,
	.open = sip_svc_do_open,
	.close = sip_svc_do_close,
	.send = sip_svc_do_send,
	.get_priv_data = sip_svc_do_get_priv_data,
	.print_info = sip_svc_do_print_info
};

#define CREATE_SIP_SVC_DEVICE(inst)                               \
	static struct sip_svc_controller sip_svc_ctrl_##inst = {  \
		.method = DT_INST_PROP(inst, method),             \
	};                                                        \
	DEVICE_DT_INST_DEFINE(inst,                               \
			sip_svc_init,                             \
			NULL,                                     \
			&sip_svc_ctrl_##inst,                     \
			NULL,                                     \
			PRE_KERNEL_1,                             \
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,       \
			&sip_svc_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_SIP_SVC_DEVICE)

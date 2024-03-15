/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT usb_c_connector

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/smf.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc_stack, CONFIG_USBC_STACK_LOG_LEVEL);

#include "usbc_stack.h"
#include "usbc_pe_common_internal.h"
#include "usbc_tc_common_internal.h"

static int usbc_subsys_init(const struct device *dev);

static ALWAYS_INLINE void usbc_handler(void *port_dev)
{
	const struct device *dev = (const struct device *)port_dev;
	struct usbc_port_data *port = dev->data;
	struct request_value *req;
	int32_t request;

	req = k_fifo_get(&port->request_fifo, K_NO_WAIT);
	request = (req != NULL) ? req->val : REQUEST_NOP;
	pe_run(dev, request);
	prl_run(dev);
	tc_run(dev, request);

	if (request == PRIV_PORT_REQUEST_SUSPEND) {
		k_thread_suspend(port->port_thread);
	}

	/* Check if there wasn't any request to do a one more iteration of USB-C state machines */
	if (!port->bypass_next_sleep) {
		k_msleep(CONFIG_USBC_STATE_MACHINE_CYCLE_TIME);
	} else {
		port->bypass_next_sleep = false;
	}
}

#define USBC_SUBSYS_INIT(inst)                                                                     \
	K_THREAD_STACK_DEFINE(my_stack_area_##inst, CONFIG_USBC_STACK_SIZE);                       \
                                                                                                   \
	static struct tc_sm_t tc_##inst;                                                           \
	static struct policy_engine pe_##inst;                                                     \
	static struct protocol_layer_rx_t prl_rx_##inst;                                           \
	static struct protocol_layer_tx_t prl_tx_##inst;                                           \
	static struct protocol_hard_reset_t prl_hr_##inst;                                         \
                                                                                                   \
	static void run_usbc_##inst(void *port_dev, void *unused1, void *unused2)                  \
	{                                                                                          \
		while (1) {                                                                        \
			usbc_handler(port_dev);                                                    \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static void create_thread_##inst(const struct device *dev)                                 \
	{                                                                                          \
		struct usbc_port_data *port = dev->data;                                           \
                                                                                                   \
		port->port_thread = k_thread_create(                                               \
			&port->thread_data, my_stack_area_##inst,                                  \
			K_THREAD_STACK_SIZEOF(my_stack_area_##inst), run_usbc_##inst, (void *)dev, \
			0, 0, CONFIG_USBC_THREAD_PRIORITY, K_ESSENTIAL, K_NO_WAIT);                \
		k_thread_suspend(port->port_thread);                                               \
	}                                                                                          \
                                                                                                   \
	static struct usbc_port_data usbc_port_data_##inst = {                                     \
		.tc = &tc_##inst,                                                                  \
		.pe = &pe_##inst,                                                                  \
		.prl_rx = &prl_rx_##inst,                                                          \
		.prl_tx = &prl_tx_##inst,                                                          \
		.prl_hr = &prl_hr_##inst,                                                          \
		.tcpc = DEVICE_DT_GET(DT_INST_PROP(inst, tcpc)),                                   \
		.vbus = DEVICE_DT_GET(DT_INST_PROP(inst, vbus)),                                   \
		.ppc = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, ppc),                               \
				   (DEVICE_DT_GET(DT_INST_PROP(inst, ppc))), (NULL)),              \
	};                                                                                         \
                                                                                                   \
	static const struct usbc_port_config usbc_port_config_##inst = {                           \
		.create_thread = create_thread_##inst,                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &usbc_subsys_init, NULL, &usbc_port_data_##inst,               \
			      &usbc_port_config_##inst, POST_KERNEL,                               \
			      CONFIG_USBC_STACK_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(USBC_SUBSYS_INIT)

/**
 * @brief Called by the Device Policy Manager to start the USB-C Subsystem
 */
int usbc_start(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/* Add private start request to fifo */
	data->request.val = PRIV_PORT_REQUEST_START;
	k_fifo_put(&data->request_fifo, &data->request);

	/* Start the port thread */
	k_thread_resume(data->port_thread);

	return 0;
}

/**
 * @brief Called by the Device Policy Manager to suspend the USB-C Subsystem
 */
int usbc_suspend(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	/* Add private suspend request to fifo */
	data->request.val = PRIV_PORT_REQUEST_SUSPEND;
	k_fifo_put(&data->request_fifo, &data->request);

	return 0;
}

/**
 * @brief Called by the Device Policy Manager to make a request of the
 *	  USB-C Subsystem
 */
int usbc_request(const struct device *dev, const enum usbc_policy_request_t req)
{
	struct usbc_port_data *data = dev->data;

	/* Add public request to fifo */
	data->request.val = req;
	k_fifo_put(&data->request_fifo, &data->request);

	return 0;
}

void usbc_bypass_next_sleep(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	data->bypass_next_sleep = true;
}

/**
 * @brief Sets the Device Policy Manager's data
 */
void usbc_set_dpm_data(const struct device *dev, void *dpm_data)
{
	struct usbc_port_data *data = dev->data;

	data->dpm_data = dpm_data;
}

/**
 * @brief Gets the Device Policy Manager's data
 */
void *usbc_get_dpm_data(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;

	return data->dpm_data;
}

#ifdef CONFIG_USBC_CSM_SINK_ONLY
/**
 * @brief Set the callback that gets the Sink Capabilities from the
 *	  Device Policy Manager
 */
void usbc_set_policy_cb_get_snk_cap(const struct device *dev,
				    const policy_cb_get_snk_cap_t policy_cb_get_snk_cap)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_get_snk_cap = policy_cb_get_snk_cap;
}

/**
 * @brief Set the callback that sends the received Source Capabilities to the
 *	  Device Policy Manager
 */
void usbc_set_policy_cb_set_src_cap(const struct device *dev,
				    const policy_cb_set_src_cap_t policy_cb_set_src_cap)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_set_src_cap = policy_cb_set_src_cap;
}

/**
 * @brief Set the callback for requesting the data object (RDO)
 */
void usbc_set_policy_cb_get_rdo(const struct device *dev,
				const policy_cb_get_rdo_t policy_cb_get_rdo)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_get_rdo = policy_cb_get_rdo;
}

/**
 * @brief Set the callback for checking if Sink Power Supply is at
 *	  default level
 */
void usbc_set_policy_cb_is_snk_at_default(const struct device *dev,
				const policy_cb_is_snk_at_default_t policy_cb_is_snk_at_default)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_is_snk_at_default = policy_cb_is_snk_at_default;
}

#else /* CONFIG_USBC_CSM_SOURCE_ONLY */

/**
 * @brief Set the callback for sending the Sink Caps to the DPM
 */
void usbc_set_policy_cb_set_port_partner_snk_cap(const struct device *dev,
				    const policy_cb_set_port_partner_snk_cap_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_set_port_partner_snk_cap = cb;
}

/**
 * @brief Set the callback that gets the Source Capabilities from the
 *        Device Policy Manager
 */
void usbc_set_policy_cb_get_src_caps(const struct device *dev,
				     const policy_cb_get_src_caps_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_get_src_caps = cb;
}

/**
 * @brief Set the callback that gets the Source Rp value from the
 *        Device Policy Manager
 */
void usbc_set_policy_cb_get_src_rp(const struct device *dev,
				   const policy_cb_get_src_rp_t policy_cb_get_src_rp)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_get_src_rp = policy_cb_get_src_rp;
}

/**
 * @brief Set the callback that controls the sourcing of VBUS from the
 *        Device Policy Manager
 */
void usbc_set_policy_cb_src_en(const struct device *dev,
			       const policy_cb_src_en_t policy_cb_src_en)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_src_en = policy_cb_src_en;
}

/**
 * @brief Set the callback for checking if a Sink Request is valid
 */
void usbc_set_policy_cb_check_sink_request(const struct device *dev,
					   const policy_cb_check_sink_request_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_check_sink_request = cb;
}

/**
 * @brief Set the callback for checking if the Source Power Supply is ready
 */
void usbc_set_policy_cb_is_ps_ready(const struct device *dev,
					const policy_cb_is_ps_ready_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_is_ps_ready = cb;
}

/**
 * @brief Set the callback for checking if the Present Contract is still valid
 */
void usbc_set_policy_cb_present_contract_is_valid(const struct device *dev,
					const policy_cb_present_contract_is_valid_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_present_contract_is_valid = cb;
}

/**
 * @brief Set the callback that requests the use of a new set of Sources Caps if
 *	  they're available
 */
void usbc_set_policy_cb_change_src_caps(const struct device *dev,
					const policy_cb_change_src_caps_t cb)
{
	struct usbc_port_data *data = dev->data;

	data->policy_change_src_caps = cb;
}

/**
 * @brief Set the callback that controls the sourcing of VCONN from the
 *        Device Policy Manager
 */
void usbc_set_vconn_control_cb(const struct device *dev,
			       const tcpc_vconn_control_cb_t cb)
{
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	tcpc_set_vconn_cb(tcpc, cb);
}

/**
 * @brief Set the callback that discharges VCONN from the
 *        Device Policy Manager
 */
void usbc_set_vconn_discharge(const struct device *dev,
			      const tcpc_vconn_discharge_cb_t cb)
{
	struct usbc_port_data *data = dev->data;
	const struct device *tcpc = data->tcpc;

	tcpc_set_vconn_discharge_cb(tcpc, cb);
}

#endif /* CONFIG_USBC_CSM_SINK_ONLY */

/**
 * @brief Set the callback for the Device Policy Manager policy check
 */
void usbc_set_policy_cb_check(const struct device *dev, const policy_cb_check_t policy_cb_check)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_check = policy_cb_check;
}

/**
 * @brief Set the callback for the Device Policy Manager policy change notify
 */
void usbc_set_policy_cb_notify(const struct device *dev, const policy_cb_notify_t policy_cb_notify)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_notify = policy_cb_notify;
}

/**
 * @brief Set the callback for the Device Policy Manager policy change notify
 */
void usbc_set_policy_cb_wait_notify(const struct device *dev,
				    const policy_cb_wait_notify_t policy_cb_wait_notify)
{
	struct usbc_port_data *data = dev->data;

	data->policy_cb_wait_notify = policy_cb_wait_notify;
}

/**
 * @brief Initialize the USB-C Subsystem
 */
static int usbc_subsys_init(const struct device *dev)
{
	struct usbc_port_data *data = dev->data;
	const struct usbc_port_config *const config = dev->config;
	const struct device *tcpc = data->tcpc;

	/* Make sure TCPC is ready */
	if (!device_is_ready(tcpc)) {
		LOG_ERR("TCPC NOT READY");
		return -ENODEV;
	}

	/* Initialize the state machines */
	tc_subsys_init(dev);
	pe_subsys_init(dev);
	prl_subsys_init(dev);

	/* Initialize the request fifo */
	k_fifo_init(&data->request_fifo);

	/* Create the thread for this port */
	config->create_thread(dev);
	return 0;
}

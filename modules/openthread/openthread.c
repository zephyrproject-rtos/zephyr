/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the OpenThread module initialization and state change handling.
 *
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_openthread_platform, CONFIG_OPENTHREAD_PLATFORM_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/version.h>
#include <zephyr/sys/check.h>

#include "platform/platform-zephyr.h"

#include <openthread.h>
#include <openthread_utils.h>

#include <openthread/child_supervision.h>
#include <openthread/cli.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/link_raw.h>
#include <openthread/ncp.h>
#include <openthread/message.h>
#include <openthread/platform/diag.h>
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/dataset.h>
#include <openthread/joiner.h>
#include <openthread-system.h>
#include <utils/uart.h>

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
#include "openthread_border_router.h"
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

#define OT_STACK_SIZE (CONFIG_OPENTHREAD_THREAD_STACK_SIZE)

#if defined(CONFIG_OPENTHREAD_THREAD_PREEMPTIVE)
#define OT_PRIORITY K_PRIO_PREEMPT(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#else
#define OT_PRIORITY K_PRIO_COOP(CONFIG_OPENTHREAD_THREAD_PRIORITY)
#endif

#if defined(CONFIG_OPENTHREAD_NETWORK_NAME)
#define OT_NETWORK_NAME CONFIG_OPENTHREAD_NETWORK_NAME
#else
#define OT_NETWORK_NAME ""
#endif

#if defined(CONFIG_OPENTHREAD_CHANNEL)
#define OT_CHANNEL CONFIG_OPENTHREAD_CHANNEL
#else
#define OT_CHANNEL 0
#endif

#if defined(CONFIG_OPENTHREAD_PANID)
#define OT_PANID CONFIG_OPENTHREAD_PANID
#else
#define OT_PANID 0
#endif

#if defined(CONFIG_OPENTHREAD_XPANID)
#define OT_XPANID CONFIG_OPENTHREAD_XPANID
#else
#define OT_XPANID ""
#endif

#if defined(CONFIG_OPENTHREAD_NETWORKKEY)
#define OT_NETWORKKEY CONFIG_OPENTHREAD_NETWORKKEY
#else
#define OT_NETWORKKEY ""
#endif

#if defined(CONFIG_OPENTHREAD_JOINER_PSKD)
#define OT_JOINER_PSKD CONFIG_OPENTHREAD_JOINER_PSKD
#else
#define OT_JOINER_PSKD ""
#endif

#if defined(CONFIG_OPENTHREAD_PLATFORM_INFO)
#define OT_PLATFORM_INFO CONFIG_OPENTHREAD_PLATFORM_INFO
#else
#define OT_PLATFORM_INFO ""
#endif

#if defined(CONFIG_OPENTHREAD_POLL_PERIOD)
#define OT_POLL_PERIOD CONFIG_OPENTHREAD_POLL_PERIOD
#else
#define OT_POLL_PERIOD 0
#endif

#if defined(CONFIG_OPENTHREAD_ROUTER_SELECTION_JITTER)
#define OT_ROUTER_SELECTION_JITTER CONFIG_OPENTHREAD_ROUTER_SELECTION_JITTER
#else
#define OT_ROUTER_SELECTION_JITTER 0
#endif

#define ZEPHYR_PACKAGE_NAME "Zephyr"
#define PACKAGE_VERSION     KERNEL_VERSION_STRING

static void openthread_process(struct k_work *work);

/* Global variables to store the OpenThread module context */
static otInstance *openthread_instance;
static sys_slist_t openthread_state_change_cbs = SYS_SLIST_STATIC_INIT(openthread_state_change_cbs);
static struct k_work_q openthread_work_q;

static K_WORK_DEFINE(openthread_work, openthread_process);
static K_MUTEX_DEFINE(openthread_lock);
K_KERNEL_STACK_DEFINE(ot_stack_area, OT_STACK_SIZE);

k_tid_t openthread_thread_id_get(void)
{
	return (k_tid_t)&openthread_work_q.thread;
}

static int ncp_hdlc_send(const uint8_t *buf, uint16_t len)
{
	otError err = OT_ERROR_NONE;

	err = otPlatUartSend(buf, len);
	if (err != OT_ERROR_NONE) {
		return 0;
	}

	return len;
}

static void openthread_process(struct k_work *work)
{
	ARG_UNUSED(work);

	openthread_mutex_lock();

	while (otTaskletsArePending(openthread_instance)) {
		otTaskletsProcess(openthread_instance);
	}

	otSysProcessDrivers(openthread_instance);

	openthread_mutex_unlock();
}

static void ot_joiner_start_handler(otError error, void *context)
{
	ARG_UNUSED(context);

	if (error != OT_ERROR_NONE) {
		LOG_ERR("Join failed [%d]", error);
	} else {
		LOG_INF("Join success");
		error = otThreadSetEnabled(openthread_instance, true);
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to start the OpenThread network [%d]", error);
		}
	}
}

static void ot_configure_instance(void)
{
#ifndef CONFIG_OPENTHREAD_COPROCESSOR_RCP
	/* Configure Child Supervision and MLE Child timeouts. */
	otChildSupervisionSetInterval(openthread_instance,
				      CONFIG_OPENTHREAD_CHILD_SUPERVISION_INTERVAL);
	otChildSupervisionSetCheckTimeout(openthread_instance,
					  CONFIG_OPENTHREAD_CHILD_SUPERVISION_CHECK_TIMEOUT);
	otThreadSetChildTimeout(openthread_instance, CONFIG_OPENTHREAD_MLE_CHILD_TIMEOUT);

	if (IS_ENABLED(CONFIG_OPENTHREAD_ROUTER_SELECTION_JITTER_OVERRIDE)) {
		otThreadSetRouterSelectionJitter(openthread_instance, OT_ROUTER_SELECTION_JITTER);
	}
#endif
}

static bool ot_setup_default_configuration(void)
{
	otExtendedPanId xpanid = {0};
	otNetworkKey networkKey = {0};
	otError error = OT_ERROR_NONE;

	error = otThreadSetNetworkName(openthread_instance, OT_NETWORK_NAME);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set %s [%d]", "network name", error);
		return false;
	}

	error = otLinkSetChannel(openthread_instance, OT_CHANNEL);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set %s [%d]", "channel", error);
		return false;
	}

	error = otLinkSetPanId(openthread_instance, OT_PANID);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set %s [%d]", "PAN ID", error);
		return false;
	}

	if (bytes_from_str(xpanid.m8, 8, (char *)OT_XPANID) != 0) {
		LOG_ERR("Failed to parse extended PAN ID");
		return false;
	}
	error = otThreadSetExtendedPanId(openthread_instance, &xpanid);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set %s [%d]", "ext PAN ID", error);
		return false;
	}

	if (strlen(OT_NETWORKKEY)) {
		if (bytes_from_str(networkKey.m8, OT_NETWORK_KEY_SIZE, (char *)OT_NETWORKKEY) !=
		    0) {
			LOG_ERR("Failed to parse network key");
			return false;
		}
		error = otThreadSetNetworkKey(openthread_instance, &networkKey);
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to set %s [%d]", "network key", error);
			return false;
		}
	}

	return true;
}

static void ot_state_changed_handler(uint32_t flags, void *context)
{
	ARG_UNUSED(context);

	struct openthread_state_changed_callback *entry, *next;

	bool is_up = otIp6IsEnabled(openthread_instance);

	LOG_INF("State changed! Flags: 0x%08" PRIx32 " Current role: %s Ip6: %s", flags,
		otThreadDeviceRoleToString(otThreadGetDeviceRole(openthread_instance)),
		(is_up ? "up" : "down"));

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&openthread_state_change_cbs, entry, next, node) {
		if (entry->otCallback != NULL) {
			entry->otCallback(flags, entry->user_data);
		}
	}
}

void otTaskletsSignalPending(otInstance *instance)
{
	ARG_UNUSED(instance);

	int error = k_work_submit_to_queue(&openthread_work_q, &openthread_work);

	if (error < 0) {
		LOG_ERR("Failed to submit work to queue, error: %d", error);
	}
}

void otSysEventSignalPending(void)
{
	otTaskletsSignalPending(NULL);
}

int openthread_state_changed_callback_register(struct openthread_state_changed_callback *cb)
{
	CHECKIF(cb == NULL || cb->otCallback == NULL) {
		return -EINVAL;
	}

	openthread_mutex_lock();
	sys_slist_append(&openthread_state_change_cbs, &cb->node);
	openthread_mutex_unlock();

	return 0;
}

int openthread_state_changed_callback_unregister(struct openthread_state_changed_callback *cb)
{
	bool removed = false;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	openthread_mutex_lock();
	removed = sys_slist_find_and_remove(&openthread_state_change_cbs, &cb->node);
	openthread_mutex_unlock();

	if (!removed) {
		return -EALREADY;
	}

	return 0;
}

struct otInstance *openthread_get_default_instance(void)
{
	__ASSERT(openthread_instance, "OT instance is not initialized");
	return openthread_instance;
}

int openthread_init(void)
{
	struct k_work_queue_config q_cfg = {
		.name = "openthread",
		.no_yield = true,
	};
	otError error = OT_ERROR_NONE;

	/* Prevent multiple initializations */
	if (openthread_instance) {
		return 0;
	}

	/* Initialize the OpenThread work queue */
	k_work_queue_init(&openthread_work_q);

	/* Start work queue for the OpenThread module */
	k_work_queue_start(&openthread_work_q, ot_stack_area,
			   K_KERNEL_STACK_SIZEOF(ot_stack_area),
			   OT_PRIORITY, &q_cfg);

	openthread_mutex_lock();

	otSysInit(0, NULL);
	openthread_instance = otInstanceInitSingle();

	__ASSERT(openthread_instance, "OT instance initialization failed");

	if (IS_ENABLED(CONFIG_OPENTHREAD_SHELL)) {
		platformShellInit(openthread_instance);
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		error = otPlatUartEnable();
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to enable UART: [%d]", error);
		}

		otNcpHdlcInit(openthread_instance, ncp_hdlc_send);
	} else {
		otIp6SetReceiveFilterEnabled(openthread_instance, true);

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

		otIp4Cidr nat64_cidr;

		if (otIp4CidrFromString(CONFIG_OPENTHREAD_NAT64_CIDR, &nat64_cidr) ==
		    OT_ERROR_NONE) {
			if (otNat64SetIp4Cidr(openthread_instance, &nat64_cidr) != OT_ERROR_NONE) {
				LOG_ERR("Incorrect NAT64 CIDR");
				return -EIO;
			}
		} else {
			LOG_ERR("Failed to parse NAT64 CIDR");
			return -EIO;
		}
#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

		error = otSetStateChangedCallback(openthread_instance, &ot_state_changed_handler,
						  NULL);
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Could not set state changed callback: %d", error);
			return -EIO;
		}
	}

	ot_configure_instance();
	openthread_mutex_unlock();

	(void)k_work_submit_to_queue(&openthread_work_q, &openthread_work);

	return error == OT_ERROR_NONE ? 0 : -EIO;
}

int openthread_run(void)
{
	openthread_mutex_lock();
	otError error = OT_ERROR_NONE;

	LOG_INF("OpenThread version: %s", otGetVersionString());

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		LOG_DBG("OpenThread co-processor.");
		goto exit;
	}

	error = otIp6SetEnabled(openthread_instance, true);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to set %s [%d]", "IPv6 support", error);
		goto exit;
	}

	/* Sleepy End Device specific configuration. */
	if (IS_ENABLED(CONFIG_OPENTHREAD_MTD_SED)) {
		otLinkModeConfig ot_mode = otThreadGetLinkMode(openthread_instance);

		/* A SED should always attach the network as a SED to indicate
		 * increased buffer requirement to a parent.
		 */
		ot_mode.mRxOnWhenIdle = false;

		error = otThreadSetLinkMode(openthread_instance, ot_mode);
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to set %s [%d]", "link mode", error);
			goto exit;
		}

		error = otLinkSetPollPeriod(openthread_instance, OT_POLL_PERIOD);
		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to set %s [%d]", "poll period", error);
			goto exit;
		}
	}

	if (otDatasetIsCommissioned(openthread_instance)) {
		/* OpenThread already has dataset stored - skip the
		 * configuration.
		 */
		LOG_DBG("OpenThread already commissioned.");
	} else if (IS_ENABLED(CONFIG_OPENTHREAD_JOINER_AUTOSTART)) {
		/* No dataset - initiate network join procedure. */
		LOG_DBG("Starting OpenThread join procedure.");

		error = otJoinerStart(openthread_instance, OT_JOINER_PSKD, NULL,
				      ZEPHYR_PACKAGE_NAME, OT_PLATFORM_INFO, PACKAGE_VERSION, NULL,
				      &ot_joiner_start_handler, NULL);

		if (error != OT_ERROR_NONE) {
			LOG_ERR("Failed to start joiner [%d]", error);
		}

		goto exit;
	} else {
		/* No dataset - load the default configuration. */
		LOG_DBG("Loading OpenThread default configuration.");

		if (!ot_setup_default_configuration()) {
			goto exit;
		}
	}

	LOG_INF("Network name: %s", otThreadGetNetworkName(openthread_instance));

	/* Start the network. */
	error = otThreadSetEnabled(openthread_instance, true);
	if (error != OT_ERROR_NONE) {
		LOG_ERR("Failed to start the OpenThread network [%d]", error);
	}

exit:

	openthread_mutex_unlock();

	return error == OT_ERROR_NONE ? 0 : -EIO;
}

int openthread_stop(void)
{
	otError error = OT_ERROR_NONE;

	if (IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		return 0;
	}

	openthread_mutex_lock();

	error = otThreadSetEnabled(openthread_instance, false);
	if (error == OT_ERROR_INVALID_STATE) {
		LOG_DBG("Openthread interface was not up [%d]", error);
	}

	openthread_mutex_unlock();

	return 0;
}

void openthread_set_receive_cb(openthread_receive_cb cb, void *context)
{
	__ASSERT(cb != NULL, "Receive callback is not set");
	__ASSERT(openthread_instance != NULL, "OpenThread instance is not initialized");

	if (!IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		openthread_mutex_lock();
		otIp6SetReceiveCallback(openthread_instance, cb, context);

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)
		otNat64SetReceiveIp4Callback(openthread_instance, cb, context);
#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

		openthread_mutex_unlock();
	}
}

void openthread_mutex_lock(void)
{
	(void)k_mutex_lock(&openthread_lock, K_FOREVER);
}

int openthread_mutex_try_lock(void)
{
	return k_mutex_lock(&openthread_lock, K_NO_WAIT);
}

void openthread_mutex_unlock(void)
{
	(void)k_mutex_unlock(&openthread_lock);
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
void openthread_notify_border_router_work(void)
{
	int error = k_work_submit_to_queue(&openthread_work_q, &openthread_border_router_work);

	if (error < 0) {
		LOG_ERR("Failed to submit work to queue, error: %d", error);
	}
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

#ifdef CONFIG_OPENTHREAD_SYS_INIT
static int openthread_sys_init(void)
{
	int error = openthread_init();

	if (error == 0) {
#ifndef CONFIG_OPENTHREAD_MANUAL_START
		error = openthread_run();
#endif
	}

	return error;
}

SYS_INIT(openthread_sys_init, POST_KERNEL, CONFIG_OPENTHREAD_SYS_INIT_PRIORITY);
#endif /* CONFIG_OPENTHREAD_SYS_INIT */

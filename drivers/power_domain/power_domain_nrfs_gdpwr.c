/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrfs_gdpwr

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include <nrfs_gdpwr.h>
#include <nrfs_backend_ipc_service.h>

LOG_MODULE_REGISTER(nrfs_gdpwr, CONFIG_POWER_DOMAIN_LOG_LEVEL);

#define MANAGER_REQUEST_TIMEOUT K_MSEC(CONFIG_POWER_DOMAIN_NRFS_GDPWR_TIMEOUT_MS)

static K_SEM_DEFINE(lock_sem, 1, 1);
static K_SEM_DEFINE(req_sem, 0, 1);
static nrfs_gdpwr_evt_type_t req_resp;
static const struct device *const domains[] = {
	DT_INST_FOREACH_CHILD_SEP(0, DEVICE_DT_GET, (,))
};

struct domain_data {
	bool on;
	bool synced;
};

struct domain_config {
	gdpwr_power_domain_t domain;
};

static void manager_event_handler(nrfs_gdpwr_evt_t const *evt, void *context)
{
	ARG_UNUSED(context);

	req_resp = evt->type;
	k_sem_give(&req_sem);
}

static void manager_lock(void)
{
	if (k_is_pre_kernel()) {
		return;
	}

	(void)k_sem_take(&lock_sem, K_FOREVER);
}

static void manager_unlock(void)
{
	if (k_is_pre_kernel()) {
		return;
	}

	k_sem_give(&lock_sem);
}

static int manager_set_domain_locked(gdpwr_power_domain_t domain, bool on)
{
	nrfs_err_t err;
	gdpwr_request_type_t req = on ? GDPWR_POWER_REQUEST_SET : GDPWR_POWER_REQUEST_CLEAR;
	int ret;

	err = nrfs_gdpwr_power_request(domain, req, NULL);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("%s %s", "nrfs gdpwr request", "failed");
		return -EIO;
	}

	ret = k_sem_take(&req_sem, MANAGER_REQUEST_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("%s %s", "nrfs gdpwr request", "timed out");
		return -ETIMEDOUT;
	}

	if (req_resp != NRFS_GDPWR_REQ_APPLIED) {
		LOG_ERR("%s %s", "nrfs gdpwr request", "rejected");
		return -EIO;
	}

	return 0;
}

static int manager_set_domain(const struct device *dev, bool on)
{
	struct domain_data *dev_data = dev->data;
	const struct domain_config *dev_config = dev->config;
	int ret;

	manager_lock();

	if (dev_data->synced) {
		/* NRFS GDPWR service is ready so we request domain change state */
		ret = manager_set_domain_locked(dev_config->domain, on);
	} else {
		/*
		 * NRFS GDPWR service is not ready so we track what the expected
		 * state of the power domain to be requested once the service
		 * is ready.
		 */
		ret = 0;
		dev_data->on = on;
	}

	if (ret == 0) {
		LOG_DBG("domain %s %ssynced and %s",
			dev->name,
			dev_data->synced ? "" : "un",
			on ? "on" : "off");
	}

	manager_unlock();
	return ret;
}

static int manager_sync_domain_locked(const struct device *dev)
{
	struct domain_data *dev_data = dev->data;
	const struct domain_config *dev_config = dev->config;

	/*
	 * NRFS service is now ready. We will now synchronize the state
	 * of the power domain with the expected state we tracked with
	 * the struct domain_data off member. Following this, tracking
	 * the power domain state is handled by device PM, thus the
	 * struct domain_data off is no longer used.
	 */
	dev_data->synced = true;

	/*
	 * Power domains initialize ON so we only need to send a request
	 * if the expected state of the power domain is not ON.
	 */
	if (!dev_data->on) {
		return manager_set_domain_locked(dev_config->domain, false);
	}

	return 0;
}

static int manager_sync_domains_locked(void)
{
	int ret;

	ARRAY_FOR_EACH(domains, i) {
		ret = manager_sync_domain_locked(domains[i]);
		if (ret) {
			break;
		}
	}

	return ret;
}

static int manager_init(void)
{
	nrfs_err_t err;
	int ret;

	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("%s %s", "nrfs backend connection", "failed");
		return -EIO;
	}

	err = nrfs_gdpwr_init(manager_event_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("%s %s", "nrfs gdpwr init", "failed");
		return -EIO;
	}

	manager_lock();
	ret = manager_sync_domains_locked();
	manager_unlock();
	return ret;
}

SYS_INIT(manager_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#if IS_ENABLED(CONFIG_DEVICE_DEPS) && IS_ENABLED(CONFIG_PM_DEVICE_POWER_DOMAIN)
static void domain_pm_notify_children(const struct device *dev,
				      enum pm_device_action action)
{
	pm_device_children_action_run(dev, action, NULL);
}
#else
static void domain_pm_notify_children(const struct device *dev,
				      enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
}
#endif

static int domain_pm_suspend(const struct device *dev)
{
	int ret;

	domain_pm_notify_children(dev, PM_DEVICE_ACTION_TURN_OFF);

	ret = manager_set_domain(dev, false);
	if (ret) {
		domain_pm_notify_children(dev, PM_DEVICE_ACTION_TURN_ON);
	}

	return ret;
}

static int domain_pm_resume(const struct device *dev)
{
	int ret;

	ret = manager_set_domain(dev, true);
	if (ret == 0) {
		domain_pm_notify_children(dev, PM_DEVICE_ACTION_TURN_ON);
	}

	return ret;
}

static int domain_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = domain_pm_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = domain_pm_resume(dev);
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		ret = -ENOTSUP;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int domain_init(const struct device *dev)
{
	return pm_device_driver_init(dev, domain_pm_action);
}

#define DOMAIN_NODE_SYMNAME(node, sym) \
	_CONCAT_4(domain, _, sym, DT_NODE_CHILD_IDX(node))

#define DOMAIN_NODE_TO_GDPWR_ENUM(node) \
	_CONCAT(GDPWR_GD_, DT_NODE_FULL_NAME_UPPER_TOKEN(node))

#define DOMAIN_DEFINE(node)									\
	static struct domain_data DOMAIN_NODE_SYMNAME(node, data);				\
	static const struct domain_config DOMAIN_NODE_SYMNAME(node, config) = {			\
		.domain = DOMAIN_NODE_TO_GDPWR_ENUM(node),					\
	};											\
												\
	PM_DEVICE_DT_DEFINE(node, domain_pm_action);						\
												\
	DEVICE_DT_DEFINE(									\
		node,										\
		domain_init,									\
		PM_DEVICE_DT_GET(node),								\
		&DOMAIN_NODE_SYMNAME(node, data),						\
		&DOMAIN_NODE_SYMNAME(node, config),						\
		PRE_KERNEL_1,									\
		0,										\
		NULL										\
	);

DT_INST_FOREACH_CHILD(0, DOMAIN_DEFINE)

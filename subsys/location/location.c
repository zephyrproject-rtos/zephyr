/*
 * Copyright (c) 2022, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/location.h>

#ifdef CONFIG_LOCATION
#include <zephyr/kernel.h>

/* Event handler */
struct location_evt_hdl_entry {
	location_event_handler_t handler;
	union location_event event_filter;
};

/* Static data */
static struct location_provider location_providers[CONFIG_LOCATION_PROVIDERS_MAX];
static struct location_evt_hdl_entry location_event_handlers[CONFIG_LOCATION_EVENT_HANDLERS_MAX];
static uint8_t location_providers_cnt;
static K_MUTEX_DEFINE(location_providers_mut);

int z_internal_location_event_handler_register(location_event_handler_t handler,
							union location_event event_filter)
{
	/* Variables */
	int index = -1;

	/* Verify arguments */
	if (handler == NULL || event_filter.value == 0) {
		return -EINVAL;
	}

	/* Lock mutex */
	k_mutex_lock(&location_providers_mut, K_FOREVER);

	/* Try to find free slot for location event handler */
	for (int i = 0; i < CONFIG_LOCATION_EVENT_HANDLERS_MAX; i++) {
		/* Verify slot is free */
		if (location_event_handlers[i].handler != NULL) {
			continue;
		}

		/* Free slot found */
		index = i;
		break;
	}

	/* Verify free slot found */
	if (index < 0) {
		k_mutex_unlock(&location_providers_mut);
		return -ENOMEM;
	}

	/* Store location event listener */
	location_event_handlers[index].handler = handler;
	location_event_handlers[index].event_filter = event_filter;

	/* Unlock mutex */
	k_mutex_unlock(&location_providers_mut);
	return 0;
}

int z_internal_location_event_handler_unregister(location_event_handler_t handler)
{
	/* Verify arguments */
	if (handler == NULL) {
		return -EINVAL;
	}

	/* Lock mutex */
	k_mutex_lock(&location_providers_mut, K_FOREVER);

	/* Try to find slot storing location event listener */
	for (uint8_t i = 0; i < CONFIG_LOCATION_EVENT_HANDLERS_MAX; i++) {
		/* Compare stored handler with handler to be unregistered */
		if (location_event_handlers[i].handler != handler) {
			continue;
		}

		/* Free slot containing handler to be unregistered */
		location_event_handlers[i].handler = NULL;
		break;
	}

	/* Unlock mutex */
	k_mutex_unlock(&location_providers_mut);
	return 0;
}

int z_internal_location_providers_get(const struct location_provider **providers)
{
	/* Variables */
	int cnt = 0;

	/* Verify arguments */
	if (providers == NULL) {
		return -EINVAL;
	}

	/* Set providers */
	*providers = location_providers;

	/* Copy location providers count */
	k_mutex_lock(&location_providers_mut, K_FOREVER);
	cnt = location_providers_cnt;
	k_mutex_unlock(&location_providers_mut);
	return cnt;
}

int location_provider_register(const struct device *dev,
			       const struct location_provider_api *api)
{
	/* Verify arguments */
	if (dev == NULL || api == NULL) {
		return -EINVAL;
	}

	/* Lock mutex */
	k_mutex_lock(&location_providers_mut, K_FOREVER);

	/* Verify memory not exceeded */
	if (location_providers_cnt == CONFIG_LOCATION_PROVIDERS_MAX) {
		k_mutex_unlock(&location_providers_mut);
		return -ENOMEM;
	}

	/* Store location provider */
	location_providers[location_providers_cnt].dev = dev;
	location_providers[location_providers_cnt].api = api;

	/* Increment location provider count */
	location_providers_cnt++;
	k_mutex_unlock(&location_providers_mut);
	return 0;
}

int location_provider_raise_event(const struct device *dev, union location_event event)
{
	/* Variables */
	struct location_provider *provider = NULL;

	/* Verify arguments */
	if (dev == NULL || event.value == 0) {
		return -EINVAL;
	}

	/* Lock mutex */
	k_mutex_lock(&location_providers_mut, K_FOREVER);

	/* Find the registered location provider which the provided device is associated with */
	for (uint8_t i = 0; i < location_providers_cnt; i++) {
		/* Compare location provider and provided devices */
		if (location_providers[i].dev != dev) {
			continue;
		}

		/* Found location provider */
		provider = &location_providers[i];
		break;
	}

	/* Verify provider was found */
	if (!provider) {
		k_mutex_unlock(&location_providers_mut);
		return -ENOENT;
	}

	/* Iterate through all registered location event handlers */
	for (uint8_t i = 0; i < CONFIG_LOCATION_EVENT_HANDLERS_MAX; i++) {
		/* Verify slot allocated */
		if (location_event_handlers[i].handler == NULL) {
			continue;
		}

		/* Verify that any raised event is enabled for handler */
		if (!(location_event_handlers[i].event_filter.value & event.value)) {
			continue;
		}

		/* Invoke handler */
		location_event_handlers[i].handler(provider, event);
	}

	/* Unlock mutex */
	k_mutex_unlock(&location_providers_mut);
	return 0;
}

#else
int z_internal_location_event_handler_register(location_event_handler_t handler,
						union location_event event_filter)
{
	ARG_UNUSED(handler);
	ARG_UNUSED(event_filter);
	return -ENOSYS;
}

int z_internal_location_event_handler_unregister(location_event_handler_t handler)
{
	ARG_UNUSED(handler);
	return -ENOSYS;
}

int z_internal_location_providers_get(const struct location_provider **providers)
{
	ARG_UNUSED(providers);
	return -ENOSYS;
}

int location_provider_register(const struct device *dev,
			       const struct location_provider_api *api)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(api);
	return 0;
}

int location_provider_raise_event(const struct device *dev, union location_event event)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(event);
	return 0;
}
#endif /* CONFIG_LOCATION */

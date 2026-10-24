/*
 * SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its
 * SPDX-FileCopyrightText: affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "internal.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/profiling/perf.h>
#include "provider.h"
#include <zephyr/sys/util.h>

/*
 * Tokens are private to the core and valid only within the running firmware build.
 * Bits 0-7: provider registry index + 1; zero is invalid.
 * Bits 8-15: reserved for the perf core; must be zero.
 * Bits 16-31: provider-local event ID.
 * Bits 32-63: reserved; must be zero.
 */
#define PERF_TOKEN_PROVIDER_MASK      GENMASK64(7, 0)
#define PERF_TOKEN_CORE_RESERVED_MASK GENMASK64(15, 8)
#define PERF_TOKEN_EVENT_MASK         GENMASK64(31, 16)
#define PERF_TOKEN_RESERVED_MASK      GENMASK64(63, 32)

static K_MUTEX_DEFINE(perf_event_lock);
static K_MUTEX_DEFINE(perf_session_lock);

static enum perf_session_type active_session;
static const struct perf_stat_config *active_config;
static struct perf_event_handle *active_events;
static size_t active_num_events;
static uint64_t active_tokens[CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS];

/* Encode a provider index and plain event ID without truncating either field. */
static int perf_token_encode(size_t provider_index, uint64_t event_id, uint64_t *token)
{
	if (token == NULL) {
		return -EINVAL;
	}
	/* The 8-bit provider ID encodes index + 1, reserving zero for invalid tokens. */
	if (provider_index >= UINT8_MAX) {
		return -EOVERFLOW;
	}
	if (event_id > UINT16_MAX) {
		return -ERANGE;
	}

	*token = FIELD_PREP(PERF_TOKEN_PROVIDER_MASK, provider_index + 1U) |
		 FIELD_PREP(PERF_TOKEN_EVENT_MASK, event_id);
	return 0;
}

/* Decode a token into a provider index and plain event ID. */
static int perf_token_decode(uint64_t token, size_t *provider_index, uint64_t *event_id)
{
	uint64_t provider_id = FIELD_GET(PERF_TOKEN_PROVIDER_MASK, token);

	if (provider_index == NULL || event_id == NULL || provider_id == 0U ||
	    (token & (PERF_TOKEN_CORE_RESERVED_MASK | PERF_TOKEN_RESERVED_MASK)) != 0U) {
		return -EINVAL;
	}

	*provider_index = (size_t)(provider_id - 1U);
	*event_id = FIELD_GET(PERF_TOKEN_EVENT_MASK, token);
	return 0;
}

static bool perf_provider_is_valid(const struct perf_event_provider *provider)
{
	return provider->name != NULL && provider->api != NULL && provider->api->list != NULL &&
	       provider->api->lookup != NULL && provider->api->start != NULL &&
	       provider->api->stop != NULL;
}

static const struct perf_event_provider *perf_provider_get(size_t index)
{
	size_t current = 0U;

	STRUCT_SECTION_FOREACH(perf_event_provider, provider) {
		if (current == index) {
			return provider;
		}
		current++;
	}

	return NULL;
}

static int perf_event_resolve(uint64_t token, const struct perf_event_provider **provider,
			      uint64_t *event_id)
{
	size_t provider_index;
	int ret;

	if (provider == NULL || event_id == NULL) {
		return -EINVAL;
	}

	ret = perf_token_decode(token, &provider_index, event_id);
	if (ret != 0) {
		return ret;
	}

	*provider = perf_provider_get(provider_index);
	if (*provider == NULL || !perf_provider_is_valid(*provider)) {
		return -EINVAL;
	}

	return 0;
}

int z_perf_session_claim(enum perf_session_type type)
{
	int ret = 0;

	if (type == PERF_SESSION_NONE) {
		return -EINVAL;
	}

	k_mutex_lock(&perf_session_lock, K_FOREVER);
	if (active_session != PERF_SESSION_NONE) {
		ret = -EBUSY;
	} else {
		active_session = type;
	}
	k_mutex_unlock(&perf_session_lock);

	return ret;
}

void z_perf_session_release(enum perf_session_type type)
{
	k_mutex_lock(&perf_session_lock, K_FOREVER);
	if (active_session == type) {
		active_session = PERF_SESSION_NONE;
	}
	k_mutex_unlock(&perf_session_lock);
}

bool z_perf_session_is_active(void)
{
	bool is_active;

	k_mutex_lock(&perf_session_lock, K_FOREVER);
	is_active = active_session != PERF_SESSION_NONE;
	k_mutex_unlock(&perf_session_lock);

	return is_active;
}

int perf_event_lookup(const char *name, struct perf_event_handle *handle)
{
	const struct perf_event_provider *matched_provider = NULL;
	const char *separator;
	size_t provider_name_len;
	size_t provider_index = 0U;
	uint64_t event_id;
	uint64_t token;
	int ret = -ENOENT;

	if (name == NULL || handle == NULL) {
		return -EINVAL;
	}
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	if (IS_ENABLED(CONFIG_USERSPACE) && k_is_user_context()) {
		return -EPERM;
	}

	separator = strchr(name, '.');
	if (separator == NULL || separator == name || separator[1] == '\0') {
		return -EINVAL;
	}
	provider_name_len = (size_t)(separator - name);

	k_mutex_lock(&perf_event_lock, K_FOREVER);
	STRUCT_SECTION_FOREACH(perf_event_provider, provider) {
		if (provider->name != NULL && strlen(provider->name) == provider_name_len &&
		    strncmp(provider->name, name, provider_name_len) == 0) {
			matched_provider = provider;
			break;
		}
		provider_index++;
	}

	if (matched_provider == NULL) {
		goto out;
	}
	if (!perf_provider_is_valid(matched_provider)) {
		ret = -ENOSYS;
		goto out;
	}
	ret = matched_provider->api->lookup(matched_provider->context, separator + 1, &event_id);
	if (ret != 0) {
		goto out;
	}
	ret = perf_token_encode(provider_index, event_id, &token);
	if (ret != 0) {
		goto out;
	}

	*handle = (struct perf_event_handle){
		.token = token,
	};

out:
	k_mutex_unlock(&perf_event_lock);
	return ret;
}

int z_perf_provider_list(z_perf_provider_list_cb_t callback, void *user_data)
{
	int ret = 0;

	if (callback == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&perf_event_lock, K_FOREVER);
	STRUCT_SECTION_FOREACH(perf_event_provider, provider) {
		if (!perf_provider_is_valid(provider)) {
			ret = -ENOSYS;
			break;
		}

		ret = callback(provider, user_data);
		if (ret != 0) {
			break;
		}
	}
	k_mutex_unlock(&perf_event_lock);

	return ret;
}

int z_perf_provider_event_list(const char *provider_name, perf_event_list_cb_t callback,
			       void *user_data)
{
	int ret = -ENOENT;

	if (provider_name == NULL || callback == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&perf_event_lock, K_FOREVER);
	STRUCT_SECTION_FOREACH(perf_event_provider, provider) {
		if (provider->name == NULL || strcmp(provider->name, provider_name) != 0) {
			continue;
		}
		if (!perf_provider_is_valid(provider)) {
			ret = -ENOSYS;
			break;
		}

		ret = provider->api->list(provider->context, callback, user_data);
		break;
	}
	k_mutex_unlock(&perf_event_lock);

	return ret;
}

static int perf_stat_validate(const struct perf_stat_config *config)
{
	const struct perf_event_provider *provider;
	uint64_t event_id;
	int ret;

	if (config == NULL || config->events == NULL || config->num_events == 0U) {
		return -EINVAL;
	}
	if (config->num_events > CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS) {
		return -ENOSPC;
	}

	for (size_t i = 0U; i < config->num_events; i++) {
		ret = perf_event_resolve(config->events[i].token, &provider, &event_id);
		if (ret != 0) {
			return ret;
		}

		for (size_t j = 0U; j < i; j++) {
			if (config->events[i].token == config->events[j].token) {
				return -EINVAL;
			}
		}
	}

	return 0;
}

int perf_stat_start(const struct perf_stat_config *config)
{
	const struct perf_event_provider *provider;
	uint64_t ignored_final_count;
	uint64_t event_id;
	size_t started = 0U;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	if (IS_ENABLED(CONFIG_USERSPACE) && k_is_user_context()) {
		return -EPERM;
	}

	k_mutex_lock(&perf_event_lock, K_FOREVER);
	ret = perf_stat_validate(config);
	if (ret != 0) {
		goto out;
	}

	ret = z_perf_session_claim(PERF_SESSION_STAT);
	if (ret != 0) {
		goto out;
	}

	for (size_t i = 0U; i < config->num_events; i++) {
		ret = perf_event_resolve(config->events[i].token, &provider, &event_id);
		if (ret != 0) {
			goto fail;
		}
		if (provider->api->prepare != NULL) {
			ret = provider->api->prepare(provider->context, event_id);
			if (ret != 0) {
				goto fail;
			}
		}
	}

	for (size_t i = 0U; i < config->num_events; i++) {
		ret = perf_event_resolve(config->events[i].token, &provider, &event_id);
		if (ret != 0) {
			goto fail;
		}
		ret = provider->api->start(provider->context, event_id,
					   &config->events[i].baseline);
		if (ret != 0) {
			goto fail;
		}
		config->events[i].final_count = 0U;
		config->events[i].status = 0;
		started++;
	}

	active_config = config;
	active_events = config->events;
	active_num_events = config->num_events;
	for (size_t i = 0U; i < config->num_events; i++) {
		active_tokens[i] = config->events[i].token;
	}
	goto out;

fail:
	for (size_t i = 0U; i < started; i++) {
		if (perf_event_resolve(config->events[i].token, &provider, &event_id) == 0) {
			(void)provider->api->stop(provider->context, event_id,
						  &ignored_final_count);
		}
	}
	z_perf_session_release(PERF_SESSION_STAT);

out:
	k_mutex_unlock(&perf_event_lock);
	return ret;
}

static bool perf_stat_config_is_active(const struct perf_stat_config *config)
{
	if (config != active_config || config == NULL || config->events != active_events ||
	    config->num_events != active_num_events) {
		return false;
	}

	for (size_t i = 0U; i < config->num_events; i++) {
		if (config->events[i].token != active_tokens[i]) {
			return false;
		}
	}

	return true;
}

int perf_stat_stop(const struct perf_stat_config *config)
{
	const struct perf_event_provider *provider;
	uint64_t event_id;
	/* Track resolution failures if the application corrupts or concurrently modifies config. */
	bool coherent = true;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	if (IS_ENABLED(CONFIG_USERSPACE) && k_is_user_context()) {
		return -EPERM;
	}

	k_mutex_lock(&perf_event_lock, K_FOREVER);
	if (!perf_stat_config_is_active(config)) {
		ret = -EINVAL;
		goto out;
	}

	for (size_t i = 0U; i < config->num_events; i++) {
		ret = perf_event_resolve(config->events[i].token, &provider, &event_id);
		if (ret != 0) {
			coherent = false;
			continue;
		}

		config->events[i].status = provider->api->stop(provider->context, event_id,
							       &config->events[i].final_count);
	}

	active_config = NULL;
	active_events = NULL;
	active_num_events = 0U;
	z_perf_session_release(PERF_SESSION_STAT);
	ret = coherent ? 0 : -EINVAL;

out:
	k_mutex_unlock(&perf_event_lock);
	return ret;
}

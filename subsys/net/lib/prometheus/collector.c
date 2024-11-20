/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/collector.h>

#include <zephyr/net/prometheus/metric.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>
#include <zephyr/net/prometheus/counter.h>
#include <zephyr/net/prometheus/gauge.h>

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_collector, CONFIG_PROMETHEUS_LOG_LEVEL);

int prometheus_collector_register_metric(struct prometheus_collector *collector,
					 struct prometheus_metric *metric)
{
	if (!collector || !metric) {
		LOG_ERR("Invalid arguments");
		return -EINVAL;
	}

	LOG_DBG("Registering metric type=%d", metric->type);

	k_mutex_lock(&collector->lock, K_FOREVER);

	/* Node cannot be added to list twice */
	(void)sys_slist_find_and_remove(&collector->metrics, &metric->node);

	sys_slist_prepend(&collector->metrics, &metric->node);

	k_mutex_unlock(&collector->lock);

	return 0;
}

const struct prometheus_counter *prometheus_get_counter_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_counter, entry) {
		LOG_DBG("entry->name: %s", entry->base.name);

		if (strncmp(entry->base.name, name, strlen(entry->base.name)) == 0) {
			LOG_DBG("Counter found %s", entry->base.name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Counter", name);

	return NULL;
}

const struct prometheus_gauge *prometheus_get_gauge_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_gauge, entry) {
		LOG_DBG("entry->name: %s", entry->base.name);

		if (strncmp(entry->base.name, name, strlen(entry->base.name)) == 0) {
			LOG_DBG("Counter found %s", entry->base.name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Gauge", name);

	return NULL;
}

const struct prometheus_histogram *prometheus_get_histogram_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_histogram, entry) {
		LOG_DBG("entry->name: %s", entry->base.name);

		if (strncmp(entry->base.name, name, strlen(entry->base.name)) == 0) {
			LOG_DBG("Counter found %s", entry->base.name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Histogram", name);

	return NULL;
}

const struct prometheus_summary *prometheus_get_summary_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_summary, entry) {
		LOG_DBG("entry->name: %s", entry->base.name);

		if (strncmp(entry->base.name, name, strlen(entry->base.name)) == 0) {
			LOG_DBG("Counter found %s", entry->base.name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Summary", name);

	return NULL;
}

const void *prometheus_collector_get_metric(struct prometheus_collector *collector,
					    const char *name)
{
	bool is_found = false;
	enum prometheus_metric_type type = 0;
	struct prometheus_metric *metric;
	struct prometheus_metric *tmp;

	if (collector == NULL || name == NULL) {
		return NULL;
	}

	k_mutex_lock(&collector->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&collector->metrics, metric, tmp, node) {

		if (strncmp(metric->name, name, strlen(metric->name)) == 0) {
			type = metric->type;
			is_found = true;

			LOG_DBG("metric found: %s", metric->name);
			break;
		}
	}

	k_mutex_unlock(&collector->lock);

	if (!is_found) {
		LOG_ERR("Metric %s not found", name);
		goto out;
	}

	switch (type) {
	case PROMETHEUS_COUNTER:
		return CONTAINER_OF(metric, struct prometheus_counter, base);
	case PROMETHEUS_GAUGE:
		return CONTAINER_OF(metric, struct prometheus_gauge, base);
	case PROMETHEUS_HISTOGRAM:
		return CONTAINER_OF(metric, struct prometheus_histogram, base);
	case PROMETHEUS_SUMMARY:
		return CONTAINER_OF(metric, struct prometheus_summary, base);
	default:
		LOG_ERR("Invalid metric type");
		break;
	}

out:
	return NULL;
}

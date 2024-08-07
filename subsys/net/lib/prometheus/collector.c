/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
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

	for (int i = 0; i < CONFIG_PROMETHEUS_MAX_METRICS; i++) {
		if (!collector->metric[i]) {
			collector->metric[i] = metric;
			collector->size++;
			return 0;
		}
	}

	return -ENOMEM;
}

const struct prometheus_counter *prometheus_get_counter_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_counter, entry) {
		LOG_DBG("entry->name: %s", entry->base->name);

		if (strncmp(entry->base->name, name, sizeof(entry->base->name)) == 0) {
			LOG_DBG("Counter found %s=%s", entry->base->name, name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Counter", name);

	return NULL;
}

const struct prometheus_gauge *prometheus_get_gauge_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_gauge, entry) {
		LOG_DBG("entry->name: %s", entry->base->name);

		if (strncmp(entry->base->name, name, sizeof(entry->base->name)) == 0) {
			LOG_DBG("Counter found %s=%s", entry->base->name, name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Gauge", name);

	return NULL;
}

const struct prometheus_histogram *prometheus_get_histogram_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_histogram, entry) {
		LOG_DBG("entry->name: %s", entry->base->name);

		if (strncmp(entry->base->name, name, sizeof(entry->base->name)) == 0) {
			LOG_DBG("Counter found %s=%s", entry->base->name, name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Histogram", name);

	return NULL;
}

const struct prometheus_summary *prometheus_get_summary_metric(const char *name)
{
	STRUCT_SECTION_FOREACH(prometheus_summary, entry) {
		LOG_DBG("entry->name: %s", entry->base->name);

		if (strncmp(entry->base->name, name, sizeof(entry->base->name)) == 0) {
			LOG_DBG("Counter found %s=%s", entry->base->name, name);
			return entry;
		}
	}

	LOG_DBG("%s %s not found", "Summary", name);

	return NULL;
}

const void *prometheus_collector_get_metric(const struct prometheus_collector *collector,
					    const char *name)
{
	bool is_found = false;
	enum prometheus_metric_type type = 0;

	for (size_t i = 0; i < collector->size; ++i) {
		if (strncmp(collector->metric[i]->name, name, sizeof(collector->metric[i]->name)) ==
		    0) {
			type = collector->metric[i]->type;
			is_found = true;

			LOG_DBG("metric found: %s", collector->metric[i]->name);
		}
	}

	if (!is_found) {
		LOG_ERR("Metric not found");
		return NULL;
	}

	switch (type) {
	case PROMETHEUS_COUNTER:
		return prometheus_get_counter_metric(name);
	case PROMETHEUS_GAUGE:
		return prometheus_get_gauge_metric(name);
	case PROMETHEUS_HISTOGRAM:
		return prometheus_get_histogram_metric(name);
	case PROMETHEUS_SUMMARY:
		return prometheus_get_summary_metric(name);
	default:
		LOG_ERR("Invalid metric type");
		return NULL;
	}

	return NULL;
}

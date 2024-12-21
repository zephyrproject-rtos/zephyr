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
#include <zephyr/net/prometheus/formatter.h>

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

	LOG_DBG("Registering metric type=%d name=\"%s\"", metric->type, metric->name);

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

int prometheus_collector_walk_metrics(struct prometheus_collector_walk_context *ctx,
				      uint8_t *buffer, size_t buffer_size)
{
	int ret = 0;

	if (ctx->collector == NULL) {
		LOG_ERR("Invalid arguments");
		return -EINVAL;
	}

	if (ctx->state == PROMETHEUS_WALK_START) {
		k_mutex_lock(&ctx->collector->lock, K_FOREVER);
		ctx->state = PROMETHEUS_WALK_CONTINUE;

		/* Start of the loop is taken from
		 * SYS_SLIST_FOR_EACH_CONTAINER_SAFE macro to simulate
		 * a loop.
		 */

		ctx->metric = Z_GENLIST_PEEK_HEAD_CONTAINER(slist,
							    &ctx->collector->metrics,
							    ctx->metric,
							    node);
		ctx->tmp = Z_GENLIST_PEEK_NEXT_CONTAINER(slist,
							 ctx->metric,
							 node);
	}

	if (ctx->state == PROMETHEUS_WALK_CONTINUE) {
		int len = 0;

		ctx->metric = ctx->tmp;
		ctx->tmp = Z_GENLIST_PEEK_NEXT_CONTAINER(slist,
							 ctx->metric,
							 node);

		if (ctx->metric == NULL) {
			ctx->state = PROMETHEUS_WALK_STOP;
			goto out;
		}

		/* If there is a user callback, use it to update the metric data. */
		if (ctx->collector->user_cb) {
			ret = ctx->collector->user_cb(ctx->collector, ctx->metric,
						      ctx->collector->user_data);
			if (ret < 0) {
				if (ret != -EAGAIN) {
					ctx->state = PROMETHEUS_WALK_STOP;
					goto out;
				}

				/* Skip this metric for now */
				goto out;
			}
		}

		ret = prometheus_format_one_metric(ctx->metric, buffer, buffer_size, &len);
		if (ret < 0) {
			ctx->state = PROMETHEUS_WALK_STOP;
			goto out;
		}

		ret = -EAGAIN;
	}

out:
	if (ctx->state == PROMETHEUS_WALK_STOP) {
		k_mutex_unlock(&ctx->collector->lock);
		ret = 0;
	}

	return ret;
}

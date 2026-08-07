/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/prometheus/formatter.h>
#include <zephyr/net/prometheus/collector.h>
#include <zephyr/net/prometheus/metric.h>
#include <zephyr/net/prometheus/histogram.h>
#include <zephyr/net/prometheus/summary.h>
#include <zephyr/net/prometheus/gauge.h>
#include <zephyr/net/prometheus/counter.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pm_formatter, CONFIG_PROMETHEUS_LOG_LEVEL);

static int write_metric_to_buffer(char *buffer, size_t buffer_size, const char *format, ...)
{
	/* helper function to write formatted metric to buffer */
	va_list args;
	size_t len = strlen(buffer);

	if (len >= buffer_size) {
		return -ENOMEM;
	}
	buffer += len;
	buffer_size -= len;

	va_start(args, format);
	len = vsnprintf(buffer, buffer_size, format, args);
	va_end(args);
	if (len >= buffer_size) {
		return -ENOMEM;
	}

	return 0;
}

int prometheus_format_one_metric(struct prometheus_metric *metric, char *buffer,
				 size_t buffer_size, int *written)
{
	int ret = 0;

	/* write HELP line if available */
	if (metric->description[0] != '\0') {
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# HELP %s %s\n", metric->name,
					     metric->description);
		if (ret < 0) {
			LOG_ERR("Error writing to buffer");
			goto out;
		}
	}

	/* write TYPE line */
	switch (metric->type) {
	case PROMETHEUS_COUNTER:
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# TYPE %s counter\n", metric->name);
		if (ret < 0) {
			LOG_ERR("Error writing counter");
			goto out;
		}

		break;

	case PROMETHEUS_GAUGE:
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# TYPE %s gauge\n", metric->name);
		if (ret < 0) {
			LOG_ERR("Error writing gauge");
			goto out;
		}

		break;

	case PROMETHEUS_HISTOGRAM:
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# TYPE %s histogram\n", metric->name);
		if (ret < 0) {
			LOG_ERR("Error writing histogram");
			goto out;
		}

		break;

	case PROMETHEUS_SUMMARY:
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# TYPE %s summary\n", metric->name);
		if (ret < 0) {
			LOG_ERR("Error writing summary");
			goto out;
		}

		break;

	default:
		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "# TYPE %s untyped\n", metric->name);
		if (ret < 0) {
			LOG_ERR("Error writing untyped");
			goto out;
		}

		break;
	}

	/* write metric-specific fields */
	switch (metric->type) {
	case PROMETHEUS_COUNTER: {
		const struct prometheus_counter *counter =
			CONTAINER_OF(metric, struct prometheus_counter, base);

		LOG_DBG("counter->value: %llu", counter->value);

		for (int i = 0; i < metric->num_labels; ++i) {
			ret = write_metric_to_buffer(
				buffer + *written, buffer_size - *written,
				"%s{%s=\"%s\"} %llu\n", metric->name, metric->labels[i].key,
				metric->labels[i].value, counter->value);
			if (ret < 0) {
				LOG_ERR("Error writing counter");
				goto out;
			}
		}

		break;
	}

	case PROMETHEUS_GAUGE: {
		const struct prometheus_gauge *gauge =
			CONTAINER_OF(metric, struct prometheus_gauge, base);

		LOG_DBG("gauge->value: %f", gauge->value);

		for (int i = 0; i < metric->num_labels; ++i) {
			ret = write_metric_to_buffer(
				buffer + *written, buffer_size - *written,
				"%s{%s=\"%s\"} %f\n", metric->name, metric->labels[i].key,
				metric->labels[i].value, gauge->value);
			if (ret < 0) {
				LOG_ERR("Error writing gauge");
				goto out;
			}
		}

		break;
	}

	case PROMETHEUS_HISTOGRAM: {
		const struct prometheus_histogram *histogram =
			CONTAINER_OF(metric, struct prometheus_histogram, base);

		LOG_DBG("histogram->count: %lu", histogram->count);

		for (int i = 0; i < histogram->num_buckets; ++i) {
			ret = write_metric_to_buffer(
				buffer + *written, buffer_size - *written,
				"%s_bucket{le=\"%f\"} %lu\n", metric->name,
				histogram->buckets[i].upper_bound,
				histogram->buckets[i].count);
			if (ret < 0) {
				LOG_ERR("Error writing histogram");
				goto out;
			}
		}

		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "%s_sum %f\n", metric->name, histogram->sum);
		if (ret < 0) {
			LOG_ERR("Error writing histogram");
			goto out;
		}

		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "%s_count %lu\n", metric->name,
					     histogram->count);
		if (ret < 0) {
			LOG_ERR("Error writing histogram");
			goto out;
		}

		break;
	}

	case PROMETHEUS_SUMMARY: {
		const struct prometheus_summary *summary =
			CONTAINER_OF(metric, struct prometheus_summary, base);

		LOG_DBG("summary->count: %lu", summary->count);

		for (int i = 0; i < summary->num_quantiles; ++i) {
			ret = write_metric_to_buffer(
				buffer + *written, buffer_size - *written,
				"%s{%s=\"%f\"} %f\n", metric->name, "quantile",
				summary->quantiles[i].quantile,
				summary->quantiles[i].value);
			if (ret < 0) {
				LOG_ERR("Error writing summary");
				goto out;
			}
		}

		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "%s_sum %f\n", metric->name, summary->sum);
		if (ret < 0) {
			LOG_ERR("Error writing summary");
			goto out;
		}

		ret = write_metric_to_buffer(buffer + *written, buffer_size - *written,
					     "%s_count %lu\n", metric->name,
					     summary->count);
		if (ret < 0) {
			LOG_ERR("Error writing summary");
			goto out;
		}

		break;
	}

	default:
		/* should not happen */
		LOG_ERR("Unsupported metric type %d", metric->type);
		ret = -EINVAL;
		goto out;
	}

out:
	return ret;
}

int prometheus_format_exposition(struct prometheus_collector *collector, char *buffer,
				 size_t buffer_size)
{
	struct prometheus_metric *metric;
	struct prometheus_metric *tmp;
	int written = 0;
	int ret = 0;

	if (collector == NULL || buffer == NULL || buffer_size == 0) {
		LOG_ERR("Invalid arguments");
		return -EINVAL;
	}

	k_mutex_lock(&collector->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&collector->metrics, metric, tmp, node) {

		/* If there is a user callback, use it to update the metric data. */
		if (collector->user_cb) {
			ret = collector->user_cb(collector, metric, collector->user_data);
			if (ret < 0) {
				if (ret == -EAGAIN) {
					/* Skip this metric for now */
					continue;
				}

				LOG_ERR("Error in user callback (%d)", ret);
				goto out;
			}
		}

		ret = prometheus_format_one_metric(metric, buffer, buffer_size, &written);
		if (ret < 0) {
			goto out;
		}
	}

out:
	k_mutex_unlock(&collector->lock);

	return ret;
}

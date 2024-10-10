/*
 * Copyright (c) 2024 Mustafa Abdullah Kus, Sparse Technology
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

static int write_to_buffer(char *buffer, size_t buffer_size, const char *str)
{
	int ret;

	/* helper function to write formatted string to buffer, handling buffer size */
	ret = snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%s", str);
	if (ret >= 0 && ret < buffer_size - strlen(buffer)) {
		return ret;
	}

	/* error (insufficient buffer size) */
	return 0;
}

static int write_metric_to_buffer(char *buffer, size_t buffer_size, const char *format, ...)
{
	/* helper function to write formatted metric to buffer */
	char temp_buffer[CONFIG_PROMETHEUS_METRIC_FORMAT_BUFFER_SIZE];
	va_list args;

	va_start(args, format);
	vsnprintf(temp_buffer, sizeof(temp_buffer), format, args);
	va_end(args);

	return write_to_buffer(buffer, buffer_size, temp_buffer);
}

int prometheus_format_exposition(const struct prometheus_collector *collector, char *buffer,
				 size_t buffer_size)
{
	int ret;
	int written = 0;

	if (collector == NULL || buffer == NULL || buffer_size == 0) {
		LOG_ERR("Invalid arguments");
		return -EINVAL;
	}

	/* iterate through each metric in the collector */
	for (size_t ind = 0; ind < collector->size; ind++) {

		const struct prometheus_metric *metric = collector->metric[ind];

		/* write HELP line if available */
		if (metric->description[0] != '\0') {
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# HELP %s %s\n", metric->name,
						     metric->description);
			if (ret < 0) {
				LOG_ERR("Error writing to buffer");
				return ret;
			}
		}

		/* write TYPE line */
		switch (metric->type) {
		case PROMETHEUS_COUNTER:
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# TYPE %s counter\n", metric->name);
			if (ret < 0) {
				LOG_ERR("Error writing counter");
				return ret;
			}
			break;
		case PROMETHEUS_GAUGE:
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# TYPE %s gauge\n", metric->name);
			if (ret < 0) {
				LOG_ERR("Error writing gauge");
				return ret;
			}
			break;
		case PROMETHEUS_HISTOGRAM:
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# TYPE %s histogram\n", metric->name);
			if (ret < 0) {
				LOG_ERR("Error writing histogram");
				return ret;
			}
			break;
		case PROMETHEUS_SUMMARY:
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# TYPE %s summary\n", metric->name);
			if (ret < 0) {
				LOG_ERR("Error writing summary");
				return ret;
			}
			break;
		default:
			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "# TYPE %s untyped\n", metric->name);
			if (ret < 0) {
				LOG_ERR("Error writing untyped");
				return ret;
			}
			break;
		}

		/* write metric-specific fields */
		switch (metric->type) {
		case PROMETHEUS_COUNTER: {
			const struct prometheus_counter *counter =
				(const struct prometheus_counter *)prometheus_collector_get_metric(
					collector, metric->name);

			LOG_DBG("counter->value: %llu", counter->value);

			for (int i = 0; i < metric->num_labels; ++i) {
				ret = write_metric_to_buffer(
					buffer + written, buffer_size - written,
					"%s{%s=\"%s\"} %llu\n", metric->name, metric->labels[i].key,
					metric->labels[i].value, counter->value);
				if (ret < 0) {
					LOG_ERR("Error writing counter");
					return ret;
				}
			}
			break;
		}
		case PROMETHEUS_GAUGE: {
			const struct prometheus_gauge *gauge =
				(const struct prometheus_gauge *)prometheus_collector_get_metric(
					collector, metric->name);

			LOG_DBG("gauge->value: %f", gauge->value);

			for (int i = 0; i < metric->num_labels; ++i) {
				ret = write_metric_to_buffer(
					buffer + written, buffer_size - written,
					"%s{%s=\"%s\"} %f\n", metric->name, metric->labels[i].key,
					metric->labels[i].value, gauge->value);
				if (ret < 0) {
					LOG_ERR("Error writing gauge");
					return ret;
				}
			}
			break;
		}
		case PROMETHEUS_HISTOGRAM: {
			const struct prometheus_histogram *histogram =
				(const struct prometheus_histogram *)
					prometheus_collector_get_metric(collector, metric->name);

			LOG_DBG("histogram->count: %lu", histogram->count);

			for (int i = 0; i < histogram->num_buckets; ++i) {
				ret = write_metric_to_buffer(
					buffer + written, buffer_size - written,
					"%s_bucket{le=\"%f\"} %lu\n", metric->name,
					histogram->buckets[i].upper_bound,
					histogram->buckets[i].count);
				if (ret < 0) {
					LOG_ERR("Error writing histogram");
					return ret;
				}
			}

			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "%s_sum %f\n", metric->name, histogram->sum);
			if (ret < 0) {
				LOG_ERR("Error writing histogram");
				return ret;
			}

			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "%s_count %lu\n", metric->name,
						     histogram->count);
			if (ret < 0) {
				LOG_ERR("Error writing histogram");
				return ret;
			}
			break;
		}
		case PROMETHEUS_SUMMARY: {
			const struct prometheus_summary *summary =
				(const struct prometheus_summary *)prometheus_collector_get_metric(
					collector, metric->name);

			LOG_DBG("summary->count: %lu", summary->count);

			for (int i = 0; i < summary->num_quantiles; ++i) {
				ret = write_metric_to_buffer(
					buffer + written, buffer_size - written,
					"%s{%s=\"%f\"} %f\n", metric->name, "quantile",
					summary->quantiles[i].quantile,
					summary->quantiles[i].value);
				if (ret < 0) {
					LOG_ERR("Error writing summary");
					return ret;
				}
			}

			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "%s_sum %f\n", metric->name, summary->sum);
			if (ret < 0) {
				LOG_ERR("Error writing summary");
				return ret;
			}

			ret = write_metric_to_buffer(buffer + written, buffer_size - written,
						     "%s_count %lu\n", metric->name,
						     summary->count);
			if (ret < 0) {
				LOG_ERR("Error writing summary");
				return ret;
			}
			break;
		}
		default:
			/* should not happen */
			LOG_ERR("Unsupported metric type %d", metric->type);
			return -EINVAL;
		}
	}

	return 0;
}

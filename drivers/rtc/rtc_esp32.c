/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_rtc

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <time.h>

#include <soc/periph_defs.h>
#include <soc/soc.h>

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/rtc.h"
#include "esp32/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#include "esp32s2/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/rtc.h"
#include "esp32s3/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#include "esp32c3/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C2
#include "esp32c2/rom/rtc.h"
#include "esp32c2/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6/rom/rtc.h"
#include "esp32c6/rtc.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_rtc, CONFIG_RTC_LOG_LEVEL);

struct rtc_esp32_data {
	uint64_t time_offset;
};

static inline uint64_t rtc_esp32_get_raw_seconds(void)
{
	/* esp_rtc_get_time_us() returns microseconds since last reset */
	return esp_rtc_get_time_us() / 1000000U;
}

static int rtc_esp32_init(const struct device *dev)
{
	struct rtc_esp32_data *data = dev->data;

	/* clear state */
	data->time_offset = 0;

	return 0;
}

static int rtc_esp32_set_time(const struct device *dev, const struct rtc_time *tp)
{
	struct rtc_esp32_data *data = dev->data;
	struct tm tm = {0};

	tm.tm_sec = tp->tm_sec;
	tm.tm_min = tp->tm_min;
	tm.tm_hour = tp->tm_hour;
	tm.tm_mday = tp->tm_mday;
	tm.tm_mon = tp->tm_mon;
	tm.tm_year = tp->tm_year;
	tm.tm_isdst = -1;

	time_t desired = mktime(&tm);
	uint64_t raw = rtc_esp32_get_raw_seconds();

	data->time_offset = (uint64_t)desired - raw;
	return 0;
}

static int rtc_esp32_get_time(const struct device *dev, struct rtc_time *tp)
{
	struct rtc_esp32_data *data = dev->data;
	struct tm tm;
	uint64_t actual = rtc_esp32_get_raw_seconds() + data->time_offset;

	gmtime_r((const time_t *)&actual, &tm);

	tp->tm_sec = tm.tm_sec;
	tp->tm_min = tm.tm_min;
	tp->tm_hour = tm.tm_hour;
	tp->tm_mday = tm.tm_mday;
	tp->tm_mon = tm.tm_mon;
	tp->tm_year = tm.tm_year;
	tp->tm_wday = tm.tm_wday;
	tp->tm_yday = tm.tm_yday;
	tp->tm_isdst = -1;
	tp->tm_nsec = 0;

	return 0;
}

/* RTC driver API structure */
static const struct rtc_driver_api rtc_esp32_api = {
	.set_time = rtc_esp32_set_time,
	.get_time = rtc_esp32_get_time,
};

/* Device instantiation via devicetree */
#define RTC_ESP32_DEVICE(inst)                                                                     \
	static struct rtc_esp32_data rtc_esp32_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, rtc_esp32_init, NULL, &rtc_esp32_data_##inst, NULL,            \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rtc_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_ESP32_DEVICE)

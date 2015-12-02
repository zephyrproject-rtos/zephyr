/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _RTC_H_
#define _RTC_H_
#include <stdint.h>
#include <device.h>
#include <misc/util.h>

enum clk_rtc_div {
	CLK_RTC_DIV_1,
	CLK_RTC_DIV_2,
	CLK_RTC_DIV_4,
	CLK_RTC_DIV_8,
	CLK_RTC_DIV_16,
	CLK_RTC_DIV_32,
	CLK_RTC_DIV_64,
	CLK_RTC_DIV_128,
	CLK_RTC_DIV_256,
	CLK_RTC_DIV_512,
	CLK_RTC_DIV_1024,
	CLK_RTC_DIV_2048,
	CLK_RTC_DIV_4096,
	CLK_RTC_DIV_8192,
	CLK_RTC_DIV_16384,
	CLK_RTC_DIV_32768
};
#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define RTC_DIVIDER CLK_RTC_DIV_1

/** Number of RTC ticks in a second */
#define RTC_ALARM_SECOND (32768 / BIT(RTC_DIVIDER))

/** Number of RTC ticks in a minute */
#define RTC_ALARM_MINUTE (RTC_ALARM_SECOND * 60)

/** Number of RTC ticks in an hour */
#define RTC_ALARM_HOUR (RTC_ALARM_MINUTE * 60)

/** Number of RTC ticks in a day */
#define RTC_ALARM_DAY (RTC_ALARM_HOUR * 24)



struct rtc_config {
	uint32_t init_val;
	/*!< enable/disable alarm  */
	uint8_t alarm_enable;
	/*!< initial configuration value for the 32bit RTC alarm value  */
	uint32_t alarm_val;
	/*!< Pointer to function to call when alarm value
	 * matches current RTC value */
	void (*cb_fn)(struct device *dev);
};

typedef void (*rtc_api_enable)(struct device *dev);
typedef void (*rtc_api_disable)(struct device *dev);
typedef int (*rtc_api_set_config)(struct device *dev,
				  struct rtc_config *config);
typedef int (*rtc_api_set_alarm)(struct device *dev,
				 const uint32_t alarm_val);
typedef uint32_t (*rtc_api_read)(struct device *dev);

struct rtc_driver_api {
	rtc_api_enable enable;
	rtc_api_disable disable;
	rtc_api_read read;
	rtc_api_set_config set_config;
	rtc_api_set_alarm set_alarm;
};

static inline uint32_t rtc_read(struct device *dev)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	return api->read(dev);
}

static inline void rtc_enable(struct device *dev)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	api->enable(dev);
}


static inline void rtc_disable(struct device *dev)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	api->disable(dev);
}

static inline int rtc_set_config(struct device *dev,
				 struct rtc_config *cfg)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	return api->set_config(dev, cfg);
}

static inline int rtc_set_alarm(struct device *dev,
				const uint32_t alarm_val)
{
	struct rtc_driver_api *api;

	api = (struct rtc_driver_api *)dev->driver_api;
	return api->set_alarm(dev, alarm_val);
}

#endif

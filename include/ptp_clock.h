/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PTP_CLOCK_H_
#define _PTP_CLOCK_H_

#include <stdint.h>
#include <device.h>
#include <misc/util.h>
#include <net/gptp.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Special alignment is needed for ptp_clock which is stored in
 * a ptp_clock linker section if there are more than one ptp clock
 * in the system. If there is only one ptp clock,
 * then this alignment is not needed, unfortunately this cannot be
 * known beforehand.
 *
 * The ptp_clock struct needs to be aligned to 32 byte boundary,
 * otherwise the __ptp_clock_end will point to wrong location and ptp_clock
 * initialization done in ptp_clock_init() will not find proper values
 * for the second interface.
 *
 * So this alignment is a workaround and should eventually be removed.
 */
#define __ptp_clock_align __aligned(32)

struct ptp_clock;

struct ptp_clock_driver_api {
	int (*set)(struct ptp_clock *clk, struct net_ptp_time *tm);
	int (*get)(struct ptp_clock *clk, struct net_ptp_time *tm);
	int (*adjust)(struct ptp_clock *clk, int increment);
	int (*rate_adjust)(struct ptp_clock *clk, float ratio);
};

/**
 * @brief PTP Clock structure
 *
 * Used to handle a ptp clock on top of a device driver instance.
 * There can be many ptp_clock instance against the same device.
 */
struct ptp_clock {
	/** The actually device driver instance the ptp_clock is related to */
	struct device *dev;

	/** API for the ptp clock. */
	struct ptp_clock_driver_api const *api;
} __ptp_clock_align;

#define PTP_CLOCK_GET_NAME(dev_name, sfx) (__ptp_clock_##dev_name##_##sfx)
#define PTP_CLOCK_GET(dev_name, sfx)					\
	((struct ptp_clock *)&PTP_CLOCK_GET_NAME(dev_name, sfx))

#define PTP_CLOCK_INIT(dev_name, sfx, api)				\
	static const struct ptp_clock					\
			(PTP_CLOCK_GET_NAME(dev_name, sfx)) __used	\
	__attribute__((__section__(".ptp_clock.data"))) = {		\
		.dev = &(DEVICE_NAME_GET(dev_name)),			\
		.api = &(api),						\
	}								\

/* PTP clock initialization macros */

#define PTP_CLOCK_DEVICE_INIT(dev_name, api)				\
	PTP_CLOCK_INIT(dev_name, 0, api)

#define PTP_CLOCK_INIT_INSTANCE(dev_name, instance, api)		\
	PTP_CLOCK_INIT(dev_name, instance, api)

static inline void ptp_clock_set(struct ptp_clock *clk, struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api = clk->api;

	api->set(clk, tm);
}

static inline void ptp_clock_get(struct ptp_clock *clk, struct net_ptp_time *tm)
{
	const struct ptp_clock_driver_api *api = clk->api;

	api->get(clk, tm);
}

static inline void ptp_clock_adjust(struct ptp_clock *clk, int increment)
{
	const struct ptp_clock_driver_api *api = clk->api;

	api->adjust(clk, increment);
}

static inline void ptp_clock_rate_adjust(struct ptp_clock *clk, float rate)
{
	const struct ptp_clock_driver_api *api = clk->api;

	api->rate_adjust(clk, rate);
}

struct ptp_clock *ptp_clock_lookup_by_dev(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* __PTP_CLOCK_H__ */

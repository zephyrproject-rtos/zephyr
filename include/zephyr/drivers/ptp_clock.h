/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup ptp_clock_interface
 * @brief Main header file for PTP (Precision Time Protocol) clock driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_

/**
 * @brief Interfaces for Precision Time Protocol (PTP) clocks.
 * @defgroup ptp_clock_interface PTP Clock
 * @since 1.13
 * @version 1.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Name of the PTP clock driver */
#if !defined(PTP_CLOCK_NAME)
#define PTP_CLOCK_NAME "PTP_CLOCK"
#endif

/** Number of fractional bits in a scaled parts-per-million rate adjustment. */
#define PTP_CLOCK_SCALED_PPM_SHIFT 16

/** One part per million in the scaled rate-adjustment representation. */
#define PTP_CLOCK_SCALED_PPM_ONE (INT64_C(1) << PTP_CLOCK_SCALED_PPM_SHIFT)

/**
 * @def_driverbackendgroup{PTP Clock,ptp_clock_interface}
 * @{
 */

/**
 * @brief Set the time of the PTP clock.
 * See ptp_clock_set() for argument description.
 */
typedef int (*ptp_clock_api_set_t)(const struct device *dev, struct net_ptp_time *tm);

/**
 * @brief Get the time of the PTP clock.
 * See ptp_clock_get() for argument description.
 */
typedef int (*ptp_clock_api_get_t)(const struct device *dev, struct net_ptp_time *tm);

/**
 * @brief Adjust the PTP clock time.
 * See ptp_clock_adjust() for argument description.
 */
typedef int (*ptp_clock_api_adjust_t)(const struct device *dev, int increment);

/**
 * @brief Adjust the PTP clock rate ratio based on its nominal frequency.
 * See ptp_clock_rate_adjust() for argument description.
 *
 * @deprecated Use ptp_clock_api_adjust_rate_t instead.
 */
typedef int (*ptp_clock_api_rate_adjust_t)(const struct device *dev, double ratio);

/**
 * @brief Adjust the PTP clock rate by an offset from its nominal frequency.
 * See ptp_clock_adjust_rate() for argument description.
 */
typedef int (*ptp_clock_api_adjust_rate_t)(const struct device *dev, int64_t scaled_ppm);

/**
 * @driver_ops{PTP Clock}
 */
__subsystem struct ptp_clock_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_set
	 */
	ptp_clock_api_set_t set;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_get
	 */
	ptp_clock_api_get_t get;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_adjust
	 */
	ptp_clock_api_adjust_t adjust;
	/**
	 * @driver_ops_optional @copybrief ptp_clock_rate_adjust
	 *
	 * Only used if @c adjust_rate is not implemented.
	 *
	 * @deprecated Implement @c adjust_rate instead.
	 */
	__deprecated ptp_clock_api_rate_adjust_t rate_adjust;
	/**
	 * @driver_ops_mandatory @copybrief ptp_clock_adjust_rate
	 *
	 * A driver that implements the deprecated @c rate_adjust may leave this unset.
	 */
	ptp_clock_api_adjust_rate_t adjust_rate;
};

/**
 * @brief Convert a scaled parts-per-million rate offset to parts per billion.
 *
 * @param scaled_ppm Rate offset in parts per million with a 16-bit binary fractional field
 *
 * @return Rate offset in parts per billion, rounded to nearest
 */
static inline int64_t ptp_clock_scaled_ppm_to_ppb(int64_t scaled_ppm)
{
	/* ppb = scaled_ppm * 1000 / 2^16 = scaled_ppm * 125 / 2^13 */
	int64_t rem = (scaled_ppm % 8192) * 125;

	return (scaled_ppm / 8192) * 125 + (rem + ((rem < 0) ? -4096 : 4096)) / 8192;
}

/**
 * @brief Apply a scaled parts-per-million rate offset to a nominal value.
 *
 * Computes `base * (1 + scaled_ppm / (1000000 * 2^16))` with integer arithmetic, rounded to
 * nearest. Meant for drivers that tune the clock rate through an addend register.
 *
 * @param base Nominal value
 * @param scaled_ppm Rate offset in parts per million with a 16-bit binary fractional field
 * @param result Where to store the adjusted value
 *
 * @retval 0 Success
 * @retval -ERANGE Rate offset is beyond +-100 % or the adjusted value does not fit in 32 bits
 */
static inline int ptp_clock_adjust_by_scaled_ppm(uint32_t base, int64_t scaled_ppm,
						 uint32_t *result)
{
	const int64_t limit = 1000000 * PTP_CLOCK_SCALED_PPM_ONE;
	int128_t product;
	uint64_t value;
	uint64_t diff;
	uint64_t frac;

	if ((scaled_ppm < -limit) || (scaled_ppm > limit)) {
		return -ERANGE;
	}

	/*
	 * diff = base * |scaled_ppm| / (1000000 * 2^16), where 1000000 * 2^16 = 15625 * 2^22.
	 * The product can exceed 64 bits, but fits again once it is shifted right by 22.
	 */
	i128_multiply_i64_i64(base, (scaled_ppm < 0) ? -scaled_ppm : scaled_ppm, &product);
	diff = (product.high << 42) | (product.low >> 22);
	frac = ((diff % 15625U) << 22) | (product.low & BIT64_MASK(22));
	diff /= 15625U;
	if (frac >= (UINT64_C(15625) << 21)) {
		diff++;
	}

	value = (scaled_ppm < 0) ? (base - diff) : (base + diff);
	if (value > UINT32_MAX) {
		return -ERANGE;
	}

	*result = (uint32_t)value;

	return 0;
}

/** @} */

/**
 * @brief Set the time of the PTP clock.
 *
 * @param dev PTP clock device
 * @param tm Time to set
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_set(const struct device *dev,
				struct net_ptp_time *tm)
{
	return DEVICE_API_GET(ptp_clock, dev)->set(dev, tm);
}

/**
 * @brief Get the time of the PTP clock.
 *
 * @param dev PTP clock device
 * @param tm Where to store the current time.
 *
 * @return 0 if ok, <0 if error
 */
__syscall int ptp_clock_get(const struct device *dev, struct net_ptp_time *tm);

static inline int z_impl_ptp_clock_get(const struct device *dev,
				       struct net_ptp_time *tm)
{
	return DEVICE_API_GET(ptp_clock, dev)->get(dev, tm);
}

/**
 * @brief Adjust the PTP clock time.
 *
 * @param dev PTP clock device
 * @param increment Increment of the clock in nanoseconds
 *
 * @return 0 if ok, <0 if error
 */
static inline int ptp_clock_adjust(const struct device *dev, int increment)
{
	return DEVICE_API_GET(ptp_clock, dev)->adjust(dev, increment);
}

/**
 * @brief Adjust the PTP clock rate ratio based on its nominal frequency
 *
 * If the driver only implements the scaled parts-per-million operation, the ratio is converted
 * and passed to that one.
 *
 * @deprecated Use ptp_clock_adjust_rate() instead.
 *
 * @param dev PTP clock device
 * @param rate Rate ratio based on its nominal frequency
 *
 * @retval 0 Success
 * @retval -ERANGE Rate ratio is not representable as scaled parts per million
 * @retval -errno Other negative errno code on failure
 */
__deprecated static inline int ptp_clock_rate_adjust(const struct device *dev, double rate)
{
	const struct ptp_clock_driver_api *api = DEVICE_API_GET(ptp_clock, dev);
	double scaled_ppm;

	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)
	if (api->rate_adjust != NULL) {
		return api->rate_adjust(dev, rate);
	}
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)

	scaled_ppm = (rate - 1.0) * (1000000.0 * PTP_CLOCK_SCALED_PPM_ONE);
	scaled_ppm += (scaled_ppm < 0.0) ? -0.5 : 0.5;

	/* Written so that NaN is rejected as well. */
	if (!((scaled_ppm >= (double)INT64_MIN) && (scaled_ppm < -(double)INT64_MIN))) {
		return -ERANGE;
	}

	return api->adjust_rate(dev, (int64_t)scaled_ppm);
}

/**
 * @brief Adjust the PTP clock rate by an offset from its nominal frequency
 *
 * The offset is not cumulative, it always refers to the nominal frequency of the clock.
 * If the driver only implements the rate ratio operation, the offset is converted and passed
 * to that one.
 *
 * @param dev PTP clock device
 * @param scaled_ppm Signed frequency offset from nominal in parts per million with a 16-bit
 *                   binary fractional field. For example, @c PTP_CLOCK_SCALED_PPM_ONE
 *                   represents 1 ppm.
 *
 * @retval 0 Success
 * @retval -errno Negative errno code on failure
 */
static inline int ptp_clock_adjust_rate(const struct device *dev, int64_t scaled_ppm)
{
	const struct ptp_clock_driver_api *api = DEVICE_API_GET(ptp_clock, dev);

	if (unlikely(api->adjust_rate == NULL)) {
		/* Drivers that still implement the deprecated operation are served as well. */
		TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)
		return api->rate_adjust(dev, 1.0 + (double)scaled_ppm /
						     (1000000.0 * PTP_CLOCK_SCALED_PPM_ONE));
		TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS)
	}

	return api->adjust_rate(dev, scaled_ppm);
}

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/ptp_clock.h>

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PTP_CLOCK_H_ */

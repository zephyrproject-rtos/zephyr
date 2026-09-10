/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public functions for the Precision Time Protocol time specification.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 * and version 2020 of IEEE 802.1AS ("gPTP").
 */

#ifndef ZEPHYR_INCLUDE_NET_PTP_TIME_H_
#define ZEPHYR_INCLUDE_NET_PTP_TIME_H_

/**
 * @brief Precision Time Protocol time specification
 * @defgroup ptp_time PTP time
 * @since 1.13
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_time.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief (Generalized) Precision Time Protocol Timestamp format.
 *
 * @details This structure represents a timestamp according to the Precision
 * Time Protocol standard ("PTP", IEEE 1588, section 5.3.3), the Generalized
 * Precision Time Protocol standard ("gPTP", IEEE 802.1AS, section 6.4.3.4), or
 * any other well-defined context in which precision structured timestamps are
 * required on network messages in Zephyr.
 *
 * Seconds are encoded as a 48 bits unsigned integer. Nanoseconds are encoded
 * as a 32 bits unsigned integer.
 *
 * In the context of (g)PTP, @em timestamps designate the time, relative to a
 * local clock ("LocalClock") at which the message timestamp point passes a
 * reference plane marking the boundary between the PTP Instance and the network
 * medium (IEEE 1855, section 7.3.4.2; IEEE 802.1AS, section 8.4.3).
 *
 * The exact definitions of the <em>message timestamp point</em> and
 * <em>reference plane</em> depends on the network medium and use case.
 *
 * For (g)PTP the media-specific message timestamp points and reference planes
 * are defined in the standard. In non-PTP contexts specific to Zephyr,
 * timestamps are measured relative to the same local clock but with a
 * context-specific message timestamp point and reference plane, defined below
 * per use case.
 *
 * A @em "LocalClock" is a freerunning clock, embedded into a well-defined
 * entity (e.g. a PTP Instance) and provides a common time to that entity
 * relative to an arbitrary epoch (IEEE 1855, section 3.1.26, IEEE 802.1AS,
 * section 3.16).
 *
 * In Zephyr, the local clock is usually any instance of a kernel system clock
 * driver, counter driver, RTC API driver or low-level counter/timer peripheral
 * (e.g. an ethernet peripheral with hardware timestamp support or a radio
 * timer) with sufficient precision for the context in which it is used.
 *
 * See IEEE 802.1AS, Annex B for specific performance requirements regarding
 * conformance of local clocks in the gPTP context. See IEEE 1588, Annex A,
 * section A5.4 for general performance requirements regarding PTP local clocks.
 * See IEEE 802.15.4-2020, section 15.7 for requirements in the context of
 * ranging applications and ibid., section 6.7.6 for the relation between guard
 * times and clock accuracy which again influence the precision required for
 * subprotocols like CSL, TSCH, RIT, etc.
 *
 * Applications that use timestamps across different subsystems or media must
 * ensure that they understand the definition of the respective reference planes
 * and interpret timestamps accordingly. Applications must further ensure that
 * timestamps are either all referenced to the same local clock or convert
 * between clocks based on sufficiently precise conversion algorithms.
 *
 * Timestamps may be measured on ingress (RX timestamps) or egress (TX
 * timestamps) of network messages. Timestamps can also be used to schedule a
 * network message to a well-defined point in time in the future at which it is
 * to be sent over the medium (timed TX). A future timestamp and a duration,
 * both referenced to the local clock, may be given to specify a time window at
 * which a network device should expect incoming messages (RX window).
 *
 * In Zephyr this timestamp structure is currently used in the following
 * contexts:
 *  * gPTP for Full Duplex Point-to-Point IEEE 802.3 links (IEEE 802.1AS,
 *    section 11): the reference plane and message timestamp points are as
 *    defined in the standard.
 *  * IEEE 802.15.4 timed TX and RX: Timestamps designate the point in time at
 *    which the end of the last symbol of the start-of-frame delimiter (SFD) (or
 *    equivalently, the start of the first symbol of the PHY header) is at the
 *    local antenna. The standard also refers to this as the "RMARKER" (IEEE
 *    802.15.4-2020, section 6.9.1) or "symbol boundary" (ibid., section 6.5.2),
 *    depending on the context. In the context of beacon timestamps, the
 *    difference between the timestamp measurement plane and the reference plane
 *    is defined by the MAC PIB attribute "macSyncSymbolOffset", ibid., section
 *    8.4.3.1, table 8-94.
 *
 * If further use cases are added to Zephyr using this timestamp structure,
 * their clock performance requirements, message timestamp points and reference
 * plane definition SHALL be added to the above list.
 */
struct net_ptp_time {
	/** Seconds encoded on 48 bits. */
	union {

/** @cond INTERNAL_HIDDEN */
		struct {
#ifdef CONFIG_LITTLE_ENDIAN
			uint32_t low;
			uint16_t high;
			uint16_t unused;
#else
			uint16_t unused;
			uint16_t high;
			uint32_t low;
#endif
		} _sec;
/** @endcond */

		/** Second value. */
		uint64_t second;
	};

	/** Nanoseconds. */
	uint32_t nanosecond;
};

#ifdef __cplusplus
}
#endif

/**
 * @brief Generalized Precision Time Protocol Extended Timestamp format.
 *
 * @details This structure represents an extended timestamp according to the
 * Generalized Precision Time Protocol standard (IEEE 802.1AS), see section
 * 6.4.3.5.
 *
 * Seconds are encoded as 48 bits unsigned integer. Fractional nanoseconds are
 * encoded as 48 bits, their unit is 2*(-16) ns.
 *
 * A precise definition of PTP timestamps and their uses in Zephyr is given in
 * the description of @ref net_ptp_time.
 */
struct net_ptp_extended_time {
	/** Seconds encoded on 48 bits. */
	union {

/** @cond INTERNAL_HIDDEN */
		struct {
#ifdef CONFIG_LITTLE_ENDIAN
			uint32_t low;
			uint16_t high;
			uint16_t unused;
#else
			uint16_t unused;
			uint16_t high;
			uint32_t low;
#endif
		} _sec;
/** @endcond */

		/** Second value. */
		uint64_t second;
	};

	/** Fractional nanoseconds on 48 bits. */
	union {

/** @cond INTERNAL_HIDDEN */
		struct {
#ifdef CONFIG_LITTLE_ENDIAN
			uint32_t low;
			uint16_t high;
			uint16_t unused;
#else
			uint16_t unused;
			uint16_t high;
			uint32_t low;
#endif
		} _fns;
/** @endcond */

		/** Fractional nanoseconds value. */
		uint64_t fract_nsecond;
	};
} __packed;

/**
 * @brief Convert a PTP timestamp to a nanosecond precision timestamp, both
 * related to the local network reference clock.
 *
 * @note Only timestamps representing up to ~290 years can be converted to
 * nanosecond timestamps. Larger timestamps will return the maximum
 * representable nanosecond precision timestamp.
 *
 * @param ts the PTP timestamp
 *
 * @return the corresponding nanosecond precision timestamp
 */
static inline net_time_t net_ptp_time_to_ns(struct net_ptp_time *ts)
{
	if (!ts) {
		return 0;
	}

	if (ts->second >= NET_TIME_SEC_MAX) {
		return NET_TIME_MAX;
	}

	return ((int64_t)ts->second * NSEC_PER_SEC) + ts->nanosecond;
}

/**
 * @brief Convert a nanosecond precision timestamp to a PTP timestamp, both
 * related to the local network reference clock.
 *
 * @param nsec a nanosecond precision timestamp
 *
 * @return the corresponding PTP timestamp
 */
static inline struct net_ptp_time ns_to_net_ptp_time(net_time_t nsec)
{
	struct net_ptp_time ts;

	__ASSERT_NO_MSG(nsec >= 0);

	ts.second = nsec / NSEC_PER_SEC;
	ts.nanosecond = nsec % NSEC_PER_SEC;
	return ts;
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_PTP_TIME_H_ */

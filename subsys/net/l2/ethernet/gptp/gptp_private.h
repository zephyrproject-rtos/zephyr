/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private functions for the Precision Time Protocol Stack.
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_PRIVATE_H
#define __GPTP_PRIVATE_H

#include <net/gptp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common defines for the gPTP stack. */
#define GPTP_THREAD_WAIT_TIMEOUT_MS 1
#define GPTP_MULTIPLE_PDELAY_RESP_WAIT (5 * 60 * MSEC_PER_SEC)

#if defined(CONFIG_NET_GPTP_STATISTICS)
#define GPTP_STATS_INC(port, var) (GPTP_PORT_PARAM_DS(port)->var++)
#else
#define GPTP_STATS_INC(port, var)
#endif

/**
 * @brief Is a slave acting as a slave.
 *
 * Utility to check if a port is configured as a slave.
 *
 * @param port Port to check.
 *
 * @return True if this is a slave port.
 */
bool gptp_is_slave_port(int port);

/**
 * @brief Convert the network interface to the correct port number.
 *
 * @param iface Network Interface acting as a ptp port.
 *
 * @return Number of the port if found, ENODEV otherwise.
 */
int gptp_get_port_number(struct net_if *iface);

/**
 * @brief Calculate a logInteral and store in Uscaled ns structure.
 *
 * @param interval Result of calculation.
 *
 * @param seconds Seconds of interval.
 *
 * @param log_msg_interval Logarithm 2 to apply to this interval.
 */
void gptp_set_time_itv(struct gptp_uscaled_ns *interval,
		       uint16_t seconds,
		       int8_t log_msg_interval);

/**
 * @brief Convert uscaled ns to ms for timer use.
 *
 * @param usns Pointer to uscaled nanoseconds to convert.
 *
 * @return INT32_MAX if value exceed timer max value, 0 if the result of the
 *	    conversion is less 1ms, the converted value otherwise.
 */
int32_t gptp_uscaled_ns_to_timer_ms(struct gptp_uscaled_ns *usns);

/**
 * @brief Update pDelay request interval and its timer.
 *
 * @param port Port number.
 *
 * @param log_val New logarithm 2 to apply to this interval.
 */
void gptp_update_pdelay_req_interval(int port, int8_t log_val);

/**
 * @brief Update sync interval and its timer.
 *
 * @param port Port number.
 *
 * @param log_val New logarithm 2 to apply to this interval.
 */
void gptp_update_sync_interval(int port, int8_t log_val);

/**
 * @brief Update announce interval and its timer.
 *
 * @param port Port number.
 *
 * @param log_val New logarithm 2 to apply to this interval.
 */

void gptp_update_announce_interval(int port, int8_t log_val);

/**
 * @brief Convert a ptp timestamp to nanoseconds.
 *
 * @param ts A PTP timestamp.
 *
 * @return Number of nanoseconds.
 */
static inline uint64_t gptp_timestamp_to_nsec(struct net_ptp_time *ts)
{
	if (!ts) {
		return 0;
	}

	return (ts->second * NSEC_PER_SEC) + ts->nanosecond;
}

/**
 * @brief Change the port state
 *
 * @param port Port number of the clock to use.
 * @param state New state
 */
#if CONFIG_NET_GPTP_LOG_LEVEL < LOG_LEVEL_DBG
void gptp_change_port_state(int port, enum gptp_port_state state);
#else
#define gptp_change_port_state(port, state)			\
	gptp_change_port_state_debug(port, state, __func__, __LINE__)

void gptp_change_port_state_debug(int port, enum gptp_port_state state,
				  const char *caller, int line);
#endif

#if CONFIG_NET_GPTP_LOG_LEVEL < LOG_LEVEL_DBG
void gptp_change_pa_info_state(
	int port,
	struct gptp_port_announce_information_state *pa_info_state,
	enum gptp_pa_info_states state);
#else
#define gptp_change_pa_info_state(port, pa_info_state, state)		\
	gptp_change_pa_info_state_debug(port, pa_info_state, state,	\
					__func__, __LINE__)

void gptp_change_pa_info_state_debug(
	int port,
	struct gptp_port_announce_information_state *pa_info_state,
	enum gptp_pa_info_states state,
	const char *caller, int line);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_PRIVATE_H */

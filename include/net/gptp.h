/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public functions for the Precision Time Protocol Stack.
 *
 */

#ifndef __GPTP_H
#define __GPTP_H

/**
 * @brief generic Precision Time Protocol (gPTP) support
 * @defgroup gptp gPTP support
 * @ingroup networking
 * @{
 */

#include <net/net_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPTP_CLOCK_ACCURACY_UNKNOWN 0xFE
#define GPTP_OFFSET_SCALED_LOG_VAR_UNKNOWN 0x436A
#define GPTP_PRIORITY1_NON_GM_CAPABLE 255
#define GPTP_PRIORITY2_DEFAULT 248

/**
 * @brief Scaled Nanoseconds.
 */
struct gptp_scaled_ns {
	/** High half. */
	s32_t high;

	/** Low half. */
	s64_t low;
} __packed;

/**
 * @brief UScaled Nanoseconds.
 */
struct gptp_uscaled_ns {
	/** High half. */
	u32_t high;

	/** Low half. */
	u64_t low;
} __packed;

/**
 * @brief Precision Time Protocol Timestamp format.
 *
 * This structure represents a timestamp according
 * to the Precision Time Protocol standard.
 *
 * Seconds are encoded on a 48 bits unsigned integer.
 * Nanoseconds are encoded on a 32 bits unsigned integer.
 */
struct net_ptp_time {
	/** Seconds encoded on 48 bits. */
	union {
		struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			u32_t low;
			u16_t high;
			u16_t unused;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			u16_t unused;
			u16_t high;
			u32_t low;
#else
#error "Unknown byte order"
#endif
		} _sec;
		u64_t second;
	};

	/** Nanoseconds. */
	u32_t nanosecond;
};

struct net_ptp_extended_timestamp {
	/** Seconds encoded on 48 bits. */
	union {
		struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			u32_t low;
			u16_t high;
			u16_t unused;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			u16_t unused;
			u16_t high;
			u32_t low;
#else
#error "Unknown byte order"
#endif
		} _sec;
		u64_t second;
	};

	/** Nanoseconds encoded on 48 bits. */
	union {
		struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			u32_t low;
			u16_t high;
			u16_t unused;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			u16_t unused;
			u16_t high;
			u32_t low;
#else
#error "Unknown byte order"
#endif
		} _nsec;
		u64_t nanosecond;
	};
};

#if defined(CONFIG_NET_GPTP)
#if defined(CONFIG_NEWLIB_LIBC)
#include <math.h>

#define GPTP_POW2(exp) pow(2, exp)
#else

static inline double _gptp_pow2(int exp)
{
	double res = 1;
	if (!exp) {
		return 1;
	}

	if (exp > 0) {
		while (exp--) {
			res *= 2;
		}
	} else {
		while (exp++) {
			res /= 2;
		}
	}

	return res;
}

#define GPTP_POW2(exp) _gptp_pow2(exp)
#endif

/*
 * TODO: k_uptime_get need to be replaced by the MAC ptp_clock.
 * The ptp_clock access infrastructure is not ready yet
 * so use it for the time being.
 * k_uptime time precision is in ms.
 */
#define GPTP_GET_CURRENT_TIME_NANOSECOND() (k_uptime_get() * 1000000)
#define GPTP_GET_CURRENT_TIME_USCALED_NS(uscaled_ns_ptr) \
	do { \
		(uscaled_ns_ptr)->low = \
		    GPTP_GET_CURRENT_TIME_NANOSECOND() << 16; \
		(uscaled_ns_ptr)->high = 0; \
	} while (0)

/**
 * @typedef gptp_phase_dis_callback_t
 * @brief Define callback that is called after a phase discontinuity has been
 *        sent by the grandmaster.
 * @param "u8_t *gm_identity" A pointer to first element of a
 *        ClockIdentity array. The size of the array is GPTP_CLOCK_ID_LEN.
 * @param "u16_t *gm_time_base" A pointer to the value of timeBaseIndicator
 *        of the current grandmaster.
 * @param "struct scaled_ns *last_gm_ph_change" A pointer to the value of
 *        lastGmPhaseChange received from grandmaster.
 * @param "double *last_gm_freq_change" A pointer to the value of
 *        lastGmFreqChange received from the grandmaster.
 */
typedef void (*gptp_phase_dis_callback_t)(
	u8_t *gm_identity,
	u16_t *time_base,
	struct gptp_scaled_ns *last_gm_ph_change,
	double *last_gm_freq_change);

/**
 * @brief Phase discontinuity callback structure.
 *
 * Stores the phase discontinuity callback information. Caller must make sure
 * that the variable pointed by this is valid during the lifetime of
 * registration. Typically this means that the variable cannot be
 * allocated from stack.
 */
struct gptp_phase_dis_cb {
	/** Node information for the slist. */
	sys_snode_t node;

	/** Phase discontinuity callback. */
	gptp_phase_dis_callback_t cb;
};

/**
 * @brief Register a phase discontinuity callback.
 *
 * @param phase_dis Caller specified handler for the callback.
 * @param cb Callback to register.
 */
void gptp_register_phase_dis_cb(struct gptp_phase_dis_cb *phase_dis,
				gptp_phase_dis_callback_t cb);

/**
 * @brief Unregister a phase discontinuity callback.
 *
 * @param phase_dis Caller specified handler for the callback.
 */
void gptp_unregister_phase_dis_cb(struct gptp_phase_dis_cb *phase_dis);

/**
 * @brief Call a phase discontinuity callback function.
 */
void gptp_call_phase_dis_cb(void);

/**
 * @brief Get gPTP time.
 *
 * @param "struct net_ptp_time *slave_time" A pointer to structure where
 *        timestamp will be saved.
 *
 * @param "bool *gm_present" A pointer to a boolean where status of the
 *        presence of a grand master will be saved.
 *
 * @return Error code. 0 if no error.
 */
int gptp_event_capture(struct net_ptp_time *slave_time, bool *gm_present);

/**
 * @brief Utility function to print clock id to a user supplied buffer.
 *
 * @param clk_id Clock id
 * @param output Output buffer
 * @param output_len Output buffer len
 *
 * @return Pointer to output buffer
 */
char *gptp_sprint_clock_id(const u8_t *clk_id, char *output,
			   size_t output_len);

/**
 * @typedef gptp_port_cb_t
 * @brief Callback used while iterating over gPTP ports
 *
 * @param port Port number
 * @param iface Pointer to network interface
 * @param user_data A valid pointer to user data or NULL
 */
typedef void (*gptp_port_cb_t)(int port, struct net_if *iface,
			       void *user_data);

/**
 * @brief Go through all the gPTP ports and call callback for each of them.
 *
 * @param cb User-supplied callback function to call
 * @param user_data User specified data
 */
void gptp_foreach_port(gptp_port_cb_t cb, void *user_data);

/**
 * @brief Get gPTP domain.
 * @details This contains all the configuration / status of the gPTP domain.
 *
 * @return Pointer to domain or NULL if not found.
 */
struct gptp_domain *gptp_get_domain(void);

#endif /* CONFIG_NET_GPTP */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __GPTP_H */

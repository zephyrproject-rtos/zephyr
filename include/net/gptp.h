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

#ifndef ZEPHYR_INCLUDE_NET_GPTP_H_
#define ZEPHYR_INCLUDE_NET_GPTP_H_

/**
 * @brief generic Precision Time Protocol (gPTP) support
 * @defgroup gptp gPTP support
 * @ingroup networking
 * @{
 */

#include <net/net_core.h>
#include <net/ptp_time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPTP_CLOCK_ACCURACY_UNKNOWN        0xFE

#define GPTP_OFFSET_SCALED_LOG_VAR_UNKNOWN 0x436A

#define GPTP_PRIORITY1_NON_GM_CAPABLE      255
#define GPTP_PRIORITY2_DEFAULT             248

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

#if defined(CONFIG_NEWLIB_LIBC)
#include <math.h>

#define GPTP_POW2(exp) pow(2, exp)
#else

static inline double _gptp_pow2(int exp)
{
	double res;

	if (exp >= 0) {
		res = 1 << exp;
	} else {
		res = 1.0;

		while (exp++) {
			res /= 2;
		}
	}

	return res;
}

#define GPTP_POW2(exp) _gptp_pow2(exp)
#endif

/* Message types. Event messages have BIT(3) set to 0, and general messages
 * have that bit set to 1. IEEE 802.1AS chapter 10.5.2.2.2
 */
#define GPTP_SYNC_MESSAGE                0x00
#define GPTP_DELAY_REQ_MESSAGE           0x01
#define GPTP_PATH_DELAY_REQ_MESSAGE      0x02
#define GPTP_PATH_DELAY_RESP_MESSAGE     0x03
#define GPTP_FOLLOWUP_MESSAGE            0x08
#define GPTP_DELAY_RESP_MESSAGE          0x09
#define GPTP_PATH_DELAY_FOLLOWUP_MESSAGE 0x0a
#define GPTP_ANNOUNCE_MESSAGE            0x0b
#define GPTP_SIGNALING_MESSAGE           0x0c
#define GPTP_MANAGEMENT_MESSAGE          0x0d

#define GPTP_IS_EVENT_MSG(msg_type)      (!((msg_type) & BIT(3)))

#define GPTP_CLOCK_ID_LEN                8

/**
 * @brief Port Identity.
 */
struct gptp_port_identity {
	/** Clock identity of the port. */
	u8_t clk_id[GPTP_CLOCK_ID_LEN];

	/** Number of the port. */
	u16_t port_number;
} __packed;

struct gptp_flags {
	union {
		/** Byte access. */
		u8_t octets[2];

		/** Whole field access. */
		u16_t all;
	};
} __packed;

struct gptp_hdr {
	/** Type of the message. */
	u8_t message_type:4;

	/** Transport specific, always 1. */
	u8_t transport_specific:4;

	/** Version of the PTP, always 2. */
	u8_t ptp_version:4;

	/** Reserved field. */
	u8_t reserved0:4;

	/** Total length of the message from the header to the last TLV. */
	u16_t message_length;

	/** Domain number, always 0. */
	u8_t domain_number;

	/** Reserved field. */
	u8_t reserved1;

	/** Message flags. */
	struct gptp_flags flags;

	/** Correction Field. The content depends of the message type. */
	s64_t correction_field;

	/** Reserved field. */
	u32_t reserved2;

	/** Port Identity of the sender. */
	struct gptp_port_identity port_id;

	/** Sequence Id. */
	u16_t sequence_id;

	/** Control value. Sync: 0, Follow-up: 2, Others: 5. */
	u8_t control;

	/** Message Interval in Log2 for Sync and Announce messages. */
	s8_t log_msg_interval;
} __packed;

#define GPTP_GET_CURRENT_TIME_USCALED_NS(port, uscaled_ns_ptr)		\
	do {								\
		(uscaled_ns_ptr)->low =					\
			gptp_get_current_time_nanosecond(port) << 16;	\
		(uscaled_ns_ptr)->high = 0;				\
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
 * @param slave_time A pointer to structure where timestamp will be saved.
 * @param gm_present A pointer to a boolean where status of the
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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_GPTP_H_ */

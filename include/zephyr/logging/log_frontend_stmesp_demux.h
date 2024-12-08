/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_DEMUX_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_DEMUX_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/mpsc_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup log_frontend_stmesp_apis Trace and Debug Domain APIs
 * @{
 * @}
 * @defgroup log_frontend_stpesp_demux_apis Logging frontend STMESP Demultiplexer API
 * @ingroup log_frontend_stmesp_apis
 * @{
 */

/** @brief Bits used to store major index. */
#define LOG_FRONTEND_STMESP_DEMUX_MAJOR_BITS 3

/** @brief Bits used to store severity level. */
#define LOG_FRONTEND_STMESP_DEMUX_LEVEL_BITS 3

/** @brief Bits used to store total length. */
#define LOG_FRONTEND_STMESP_DEMUX_TLENGTH_BITS 16

/** @brief Bits used to store package length. */
#define LOG_FRONTEND_STMESP_DEMUX_PLENGTH_BITS 10

/** @brief Maximum number of supported majors. */
#define LOG_FRONTEND_STMESP_DEMUX_MAJOR_MAX BIT(LOG_FRONTEND_STMESP_DEMUX_MAJOR_BITS)

/** @brief Log message type. */
#define LOG_FRONTEND_STMESP_DEMUX_TYPE_LOG 0

/** @brief Trace point message type. */
#define LOG_FRONTEND_STMESP_DEMUX_TYPE_TRACE_POINT 1

/** @brief HW event message type. */
#define LOG_FRONTEND_STMESP_DEMUX_TYPE_HW_EVENT 2

/** @brief Logging message header. */
struct log_frontend_stmesp_demux_log_header {
	/** Major index. */
	uint32_t major : LOG_FRONTEND_STMESP_DEMUX_MAJOR_BITS;

	/** Severity level. */
	uint32_t level : LOG_FRONTEND_STMESP_DEMUX_LEVEL_BITS;

	/** Total length excluding this header. */
	uint32_t total_len : LOG_FRONTEND_STMESP_DEMUX_TLENGTH_BITS;

	/** Hexdump data length. */
	uint32_t package_len : LOG_FRONTEND_STMESP_DEMUX_PLENGTH_BITS;
};

/** @brief Union for writing raw data to the logging message header. */
union log_frontend_stmesp_demux_header {
	/** Log header structure. */
	struct log_frontend_stmesp_demux_log_header log;

	/** Raw word. */
	uint32_t raw;
};

/** @brief Generic STP demux packet. */
struct log_frontend_stmesp_demux_packet_generic {
	/** Data for MPSC packet handling. */
	MPSC_PBUF_HDR;

	/** Type. */
	uint64_t type: 2;

	/** Flag indicating if packet is valid. */
	uint64_t content_invalid: 1;
};

/** @brief Packet with logging message. */
struct log_frontend_stmesp_demux_log {
	/** Data for MPSC packet handling. */
	MPSC_PBUF_HDR;

	/** Type. */
	uint64_t type: 2;

	/** Flag indicating if packet is valid. */
	uint64_t content_invalid: 1;

	/** Timestamp. */
	uint64_t timestamp: 59;

	/** Logging header. */
	struct log_frontend_stmesp_demux_log_header hdr;

	/** Padding so that data is 8 bytes aligned. */
	uint32_t padding;

	/** Content. */
	uint8_t data[];
};

/** @brief Packet with trace point. */
struct log_frontend_stmesp_demux_trace_point {
	/** Data for MPSC packet handling. */
	MPSC_PBUF_HDR;

	/** Type. */
	uint64_t type: 2;

	/** Flag indicating if packet is valid. */
	uint64_t content_invalid: 1;

	/** Flag indicating if trace point includes data. */
	uint64_t has_data: 1;

	/** Timestamp. */
	uint64_t timestamp: 58;

	/** Major ID. */
	uint16_t major;

	/** ID */
	uint16_t id;

	/** Content. */
	uint32_t data;
};

/** @brief Packet with HW event. */
struct log_frontend_stmesp_demux_hw_event {
	/** Data for MPSC packet handling. */
	MPSC_PBUF_HDR;

	/** Type. */
	uint64_t type: 2;

	/** Flag indicating if packet is valid. */
	uint64_t content_invalid: 1;

	/** Timestamp. */
	uint64_t timestamp: 59;

	/** HW event ID. */
	uint8_t evt;
};

/** @brief Union of all packet types. */
union log_frontend_stmesp_demux_packet {
	/** Pointer to generic mpsc_pbuf const packet. */
	const union mpsc_pbuf_generic *rgeneric;

	/** Pointer to generic mpsc_pbuf packet. */
	union mpsc_pbuf_generic *generic;

	/** Pointer to the log message. */
	struct log_frontend_stmesp_demux_log *log;

	/** Pointer to the trace point message. */
	struct log_frontend_stmesp_demux_trace_point *trace_point;

	/** Pointer to the HW event message. */
	struct log_frontend_stmesp_demux_hw_event *hw_event;

	/** Pointer to the generic log_frontend_stmesp_demux packet. */
	struct log_frontend_stmesp_demux_packet_generic *generic_packet;
};

/** @brief Demultiplexer configuration. */
struct log_frontend_stmesp_demux_config {
	/** Array with expected major ID's. */
	const uint16_t *m_ids;

	/** Array length. Must be not bigger than @ref LOG_FRONTEND_STMESP_DEMUX_MAJOR_MAX. */
	uint32_t m_ids_cnt;
};

/** @brief Initialize the demultiplexer.
 *
 * @param config Configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL on invalid configuration.
 */
int log_frontend_stmesp_demux_init(const struct log_frontend_stmesp_demux_config *config);

/** @brief Indicate major opcode in the STPv2 stream.
 *
 * @param id	Master ID.
 */
void log_frontend_stmesp_demux_major(uint16_t id);

/** @brief Indicate channel opcode in the STPv2 stream.
 *
 * @param id	Channel ID.
 */
void log_frontend_stmesp_demux_channel(uint16_t id);

/** @brief Indicate detected packet start (DMTS).
 *
 * @param data	Data. Can be NULL which indicates trace point without data.
 * @param ts	Timestamp. Can be NULL.
 */
int log_frontend_stmesp_demux_packet_start(uint32_t *data, uint64_t *ts);

/** @brief Indicate timestamp.
 *
 * Timestamp is separated from packet start because according to STM spec (3.2.2)
 * it is possible that timestamp is assigned to a later packet.
 *
 * @param ts	Timestamp.
 */
void log_frontend_stmesp_demux_timestamp(uint64_t ts);

/** @brief Indicate data.
 *
 * @param data	Data buffer.
 * @param len	Length.
 */
void log_frontend_stmesp_demux_data(uint8_t *data, size_t len);

/** @brief Indicate packet end (Flag). */
void log_frontend_stmesp_demux_packet_end(void);

/** @brief Get number of dropped messages and reset the counter.
 *
 * Message can be dropped if there is no room in the packet buffer.
 *
 * @return Number of dropped messages.
 */
uint32_t log_frontend_stmesp_demux_get_dropped(void);

/** @brief Claim packet.
 *
 * Get pointer to the pending packet with logging message. Packet must be freed
 * using @ref log_frontend_stmesp_demux_free.
 *
 * @return Pointer to the packet or NULL.
 */
union log_frontend_stmesp_demux_packet log_frontend_stmesp_demux_claim(void);

/** @brief Free previously claimed packet.
 *
 * See @ref log_frontend_stmesp_demux_claim.
 *
 * @param packet Packet.
 */
void log_frontend_stmesp_demux_free(union log_frontend_stmesp_demux_packet packet);

/** @brief Check if there are any started but not completed log messages.
 *
 * @retval True There is no pending started log message.
 * @retval False There is pending message.
 */
bool log_frontend_stmesp_demux_is_idle(void);

/** @brief Close any opened messages and mark them as invalid. */
void log_frontend_stmesp_demux_reset(void);

/** @brief Get maximum buffer utilization.
 *
 * @retval Non-negative Maximum buffer utilization.
 * @retval -ENOTSUP Feature not enabled.
 */
int log_frontend_stmesp_demux_max_utilization(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_FRONTEND_STMESP_DEMUX_H_ */

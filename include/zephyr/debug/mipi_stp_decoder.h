/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_MIPI_STP_DECODER_H__
#define ZEPHYR_INCLUDE_DEBUG_MIPI_STP_DECODER_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mipi_stp_decoder_apis STP Decoder API
 * @ingroup os_services
 * @{
 */

/** @brief STPv2 opcodes. */
enum mipi_stp_decoder_ctrl_type {
	STP_DATA4 = 1,
	STP_DATA8 = 2,
	STP_DATA16 = 4,
	STP_DATA32 = 8,
	STP_DATA64 = 16,
	STP_DECODER_NULL = 128,
	STP_DECODER_MAJOR,
	STP_DECODER_MERROR,
	STP_DECODER_CHANNEL,
	STP_DECODER_VERSION,
	STP_DECODER_FREQ,
	STP_DECODER_GERROR,
	STP_DECODER_FLAG,
	STP_DECODER_ASYNC,
	STP_DECODER_NOT_SUPPORTED,
};

/** @brief Convert type to a string literal.
 *
 * @param _type type
 * @return String literal.
 */
#define STP_DECODER_TYPE2STR(_type) \
	_type == STP_DATA4 ? "DATA4" : (\
	_type == STP_DATA8 ? "DATA8" : (\
	_type == STP_DATA16 ? "DATA16" : (\
	_type == STP_DATA32 ? "DATA32" : (\
	_type == STP_DATA64 ? "DATA64" : (\
	_type == STP_DECODER_NULL ? "NULL" : (\
	_type == STP_DECODER_MAJOR ? "MAJOR" : (\
	_type == STP_DECODER_MERROR ? "MERROR" : (\
	_type == STP_DECODER_CHANNEL ? "CHANNEL" : (\
	_type == STP_DECODER_VERSION ? "VERSION" : (\
	_type == STP_DECODER_FREQ ? "FREQ" : (\
	_type == STP_DECODER_GERROR ? "GERROR" : (\
	_type == STP_DECODER_FLAG ? "FLAG" : (\
	_type == STP_DECODER_ASYNC ? "ASYNC" : (\
	"Unknown"))))))))))))))

/** @brief Union with data associated with a given STP opcode. */
union mipi_stp_decoder_data {
	/** ID - used for major and channel. */
	uint16_t id;

	/** Frequency. */
	uint64_t freq;

	/** Version. */
	uint32_t ver;

	/** Error code. */
	uint32_t err;

	/** Dummy. */
	uint32_t dummy;

	/** Data. */
	uint64_t data;
};

/** @brief Callback signature.
 *
 * Callback is called whenever an element from STPv2 stream is decoded.
 *
 * @note Callback is called with interrupts locked.
 *
 * @param type		Type. See @ref mipi_stp_decoder_ctrl_type.
 * @param data		Data. Data associated with a given @p type.
 * @param ts		Timestamp. Present if not NULL.
 * @param marked	Set to true if opcode was marked.
 */
typedef void (*mipi_stp_decoder_cb)(enum mipi_stp_decoder_ctrl_type type,
				    union mipi_stp_decoder_data data,
				    uint64_t *ts, bool marked);

/** @brief Decoder configuration. */
struct mipi_stp_decoder_config {
	/** Indicates that decoder start in out of sync state. */
	bool start_out_of_sync;

	/** Callback. */
	mipi_stp_decoder_cb cb;
};

/** @brief Initialize the decoder.
 *
 * @param config	Configuration.
 *
 * @retval 0		On successful initialization.
 * @retval negative	On failure.
 */
int mipi_stp_decoder_init(const struct mipi_stp_decoder_config *config);

/** @brief Decode STPv2 stream.
 *
 * Function decodes the stream and calls the callback for every decoded element.
 *
 * @param data		Data.
 * @param len		Data length.
 *
 * @retval 0		On successful decoding.
 * @retval negative	On failure.
 */
int mipi_stp_decoder_decode(const uint8_t *data, size_t len);

/** @brief Indicate synchronization loss.
 *
 * If detected, then decoder starts to look for ASYNC marker and drops all data
 * until ASYNC is found. Synchronization can be lost when there is data loss (e.g.
 * due to overflow).
 */
void mipi_stp_decoder_sync_loss(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_MIPI_STP_DECODER_H__ */

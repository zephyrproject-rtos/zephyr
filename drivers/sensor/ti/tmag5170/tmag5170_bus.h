/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

/** The TMAG5170 always exchanges 32 bits per SPI frame */
#define TMAG5170_SPI_BUFFER_LEN 4U

/** Command sent within the lower nibble of the last frame byte */
#define TMAG5170_CMD_TRIGGER_CONVERSION BIT(0U)

/**
 * @brief RTIO based SPI bus context of a single TMAG5170 instance.
 */
struct tmag5170_bus {
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
	} rtio;
	/** Frame buffers used by the synchronous convenience wrappers.
	 *
	 * They must not be placed on the stack, because the SQEs referencing
	 * them are processed asynchronously by the RTIO executor.
	 */
	uint8_t tx_frame[TMAG5170_SPI_BUFFER_LEN];
	uint8_t rx_frame[TMAG5170_SPI_BUFFER_LEN];
};

/**
 * @brief Encode a register read frame, including the CRC when enabled.
 *
 * @param buffer_tx Frame buffer of at least TMAG5170_SPI_BUFFER_LEN bytes
 * @param reg Register address to read from
 * @param cmd Command bits (e.g. TMAG5170_CMD_TRIGGER_CONVERSION)
 */
void tmag5170_frame_encode_read(uint8_t *buffer_tx, uint8_t reg, uint8_t cmd);

/**
 * @brief Encode a register write frame, including the CRC when enabled.
 *
 * @param buffer_tx Frame buffer of at least TMAG5170_SPI_BUFFER_LEN bytes
 * @param reg Register address to write to
 * @param data Register value to write
 */
void tmag5170_frame_encode_write(uint8_t *buffer_tx, uint8_t reg, uint16_t data);

/**
 * @brief Decode a received frame and verify its CRC when enabled.
 *
 * @param buffer_rx Received frame of TMAG5170_SPI_BUFFER_LEN bytes
 * @param output Optional destination of the decoded register value
 *
 * @retval 0 on success
 * @retval -EIO if the CRC of the received frame does not match
 */
int tmag5170_frame_decode(const uint8_t *buffer_rx, uint16_t *output);

/**
 * @brief Prepare a single full-duplex frame as RTIO submission.
 *
 * The caller owns @p buffer_tx and @p buffer_rx; both need to stay valid until
 * the submission has been completed.
 *
 * @param bus Bus context of the instance
 * @param buffer_tx Frame to shift out, already encoded and CRC'ed
 * @param buffer_rx Destination of the frame shifted in, may be NULL
 * @param out Optional destination of the last acquired SQE, so it can be
 *            chained by the caller
 *
 * @retval Number of acquired SQEs on success
 * @retval -ENOMEM if no SQE could be acquired
 */
int tmag5170_prep_frame_rtio_async(const struct tmag5170_bus *bus, const uint8_t *buffer_tx,
				   uint8_t *buffer_rx, struct rtio_sqe **out);

/**
 * @brief Synchronously transmit a pre-encoded frame, bypassing CRC generation.
 *
 * @param bus Bus context of the instance
 * @param frame Frame of TMAG5170_SPI_BUFFER_LEN bytes to shift out as-is
 *
 * @retval 0 on success, negative errno otherwise
 */
int tmag5170_transmit_frame_rtio(struct tmag5170_bus *bus, const uint8_t *frame);

/**
 * @brief Synchronously read a register.
 *
 * @param bus Bus context of the instance
 * @param reg Register address to read from
 * @param cmd Command bits (e.g. TMAG5170_CMD_TRIGGER_CONVERSION)
 * @param output Optional destination of the register value
 *
 * @retval 0 on success, negative errno otherwise
 */
int tmag5170_read_register_rtio(struct tmag5170_bus *bus, uint8_t reg, uint8_t cmd,
				uint16_t *output);

/**
 * @brief Synchronously write a register.
 *
 * @param bus Bus context of the instance
 * @param reg Register address to write to
 * @param data Register value to write
 *
 * @retval 0 on success, negative errno otherwise
 */
int tmag5170_write_register_rtio(struct tmag5170_bus *bus, uint8_t reg, uint16_t data);

#endif /* ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_BUS_H_ */

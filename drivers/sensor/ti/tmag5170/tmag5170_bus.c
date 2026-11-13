/*
 * Copyright (c) 2023 Michal Morsisko
 * Copyright (c) 2026 Swarovski Optik AG & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_TMAG5170_CRC)
#include <zephyr/sys/crc.h>
#endif

#include "tmag5170_bus.h"

#define TMAG5170_CRC_SEED 0xFU
#define TMAG5170_CRC_POLY 0x3U

#define TMAG5170_FRAME_GET_CRC(buf)      ((buf)[3] & 0x0FU)
#define TMAG5170_FRAME_ZERO_CRC(buf)     ((buf)[3] &= 0xF0U)
#define TMAG5170_FRAME_SET_CRC(buf, crc) ((buf)[3] |= ((crc) & 0x0FU))

static void tmag5170_frame_finalize(uint8_t *buffer_tx)
{
#if defined(CONFIG_TMAG5170_CRC)
	TMAG5170_FRAME_ZERO_CRC(buffer_tx);
	uint8_t crc = crc4_ti(TMAG5170_CRC_SEED, buffer_tx, TMAG5170_SPI_BUFFER_LEN);

	TMAG5170_FRAME_SET_CRC(buffer_tx, crc);
#else
	ARG_UNUSED(buffer_tx);
#endif
}

void tmag5170_frame_encode_read(uint8_t *buffer_tx, uint8_t reg, uint8_t cmd)
{
	buffer_tx[0] = BIT(7) | reg;
	buffer_tx[1] = 0x00U;
	buffer_tx[2] = 0x00U;
	buffer_tx[3] = (cmd & BIT_MASK(4U)) << 4U;

	tmag5170_frame_finalize(buffer_tx);
}

void tmag5170_frame_encode_write(uint8_t *buffer_tx, uint8_t reg, uint16_t data)
{
	buffer_tx[0] = reg;
	buffer_tx[1] = (data >> 8) & 0xFFU;
	buffer_tx[2] = data & 0xFFU;
	buffer_tx[3] = 0x00U;

	tmag5170_frame_finalize(buffer_tx);
}

int tmag5170_frame_decode(const uint8_t *buffer_rx, uint16_t *output)
{
	int ret = 0;

#if defined(CONFIG_TMAG5170_CRC)
	uint8_t frame[TMAG5170_SPI_BUFFER_LEN];
	uint8_t read_crc = TMAG5170_FRAME_GET_CRC(buffer_rx);

	memcpy(frame, buffer_rx, sizeof(frame));
	TMAG5170_FRAME_ZERO_CRC(frame);

	if (read_crc != crc4_ti(TMAG5170_CRC_SEED, frame, sizeof(frame))) {
		ret = -EIO;
	}
#endif

	if (output != NULL) {
		*output = sys_get_be16(&buffer_rx[1]);
	}

	return ret;
}

int tmag5170_prep_frame_rtio_async(const struct tmag5170_bus *bus, const uint8_t *buffer_tx,
				   uint8_t *buffer_rx, struct rtio_sqe **out)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *frame_sqe = rtio_sqe_acquire(ctx);

	if (!frame_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	/* The TMAG5170 is a full-duplex device: the answer to the command
	 * shifted out on MOSI is shifted in on MISO within the very same
	 * 32-bit frame. Therefore a single transceive SQE is enough to
	 * perform one complete register access.
	 */
	rtio_sqe_prep_transceive(frame_sqe, iodev, RTIO_PRIO_NORM, buffer_tx, buffer_rx,
				 TMAG5170_SPI_BUFFER_LEN, NULL);

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = frame_sqe;
	}

	return 1;
}

static int tmag5170_submit_and_consume(const struct tmag5170_bus *bus, int num_sqe)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = rtio_submit(ctx, num_sqe);
	if (ret) {
		return ret;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			ret = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return ret;
}

int tmag5170_transmit_frame_rtio(struct tmag5170_bus *bus, const uint8_t *frame)
{
	int ret;

	memcpy(bus->tx_frame, frame, TMAG5170_SPI_BUFFER_LEN);

	ret = tmag5170_prep_frame_rtio_async(bus, bus->tx_frame, bus->rx_frame, NULL);
	if (ret < 0) {
		return ret;
	}

	return tmag5170_submit_and_consume(bus, ret);
}

int tmag5170_read_register_rtio(struct tmag5170_bus *bus, uint8_t reg, uint8_t cmd,
				uint16_t *output)
{
	int ret;

	tmag5170_frame_encode_read(bus->tx_frame, reg, cmd);
	memset(bus->rx_frame, 0, sizeof(bus->rx_frame));

	ret = tmag5170_prep_frame_rtio_async(bus, bus->tx_frame, bus->rx_frame, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = tmag5170_submit_and_consume(bus, ret);
	if (ret < 0) {
		return ret;
	}

	return tmag5170_frame_decode(bus->rx_frame, output);
}

int tmag5170_write_register_rtio(struct tmag5170_bus *bus, uint8_t reg, uint16_t data)
{
	int ret;

	tmag5170_frame_encode_write(bus->tx_frame, reg, data);

	ret = tmag5170_prep_frame_rtio_async(bus, bus->tx_frame, bus->rx_frame, NULL);
	if (ret < 0) {
		return ret;
	}

	return tmag5170_submit_and_consume(bus, ret);
}

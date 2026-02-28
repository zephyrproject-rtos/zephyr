/*
 * Copyright (c) 2025 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Raspberry Pi Pico mailbox
 * @ingroup mbox_rpi_pico_interface
 */

#ifndef ZEPHYR_DRIVERS_MISC_MBOX_PICO_RPI_PIO_PICO_RPI_H_
#define ZEPHYR_DRIVERS_MISC_MBOX_PICO_RPI_PIO_PICO_RPI_H_

/**
 * @brief Interfaces for Raspberry Pi Pico SIO FIFO mailbox.
 * @defgroup mbox_rpi_pico_interface Raspberry Pi Pico mailbox
 * @ingroup misc_interfaces
 *
 * @{
 */

#include <hardware/structs/sio.h>

/**
 * Check if FIFO is writeable
 *
 * @param sio_block pointer to sio struct
 * @retval true if the write FIFO isn't full
 * @retval false if there is no space to write to FIFO.
 */
static inline bool rpi_pico_mbox_write_ready(sio_hw_t *const sio_block)
{
	return sio_block->fifo_st & SIO_FIFO_ST_RDY_BITS;
}

/**
 * Check if FIFO is readable
 *
 * @param sio_block pointer to sio struct
 * @retval true if the read FIFO has data available
 * @retval false otherwise
 */
static inline bool rpi_pico_mbox_read_valid(sio_hw_t *const sio_block)
{
	return sio_block->fifo_st & SIO_FIFO_ST_VLD_BITS;
}

/**
 * Discard pending messages in FIFO
 *
 * @param sio_block pointer to sio struct
 */
static inline void rpi_pico_mbox_drain(sio_hw_t *const sio_block)
{
	while (rpi_pico_mbox_read_valid(sio_block)) {
		(void)sio_block->fifo_rd;
	}
}

/**
 * Check if FIFO is writeable
 *
 * @param sio_block pointer to sio struct
 * @param value to be pushed
 */
static inline void rpi_pico_mbox_write(sio_hw_t *const sio_block, uint32_t value)
{
	sio_block->fifo_wr = value;
}

/**
 * Check if FIFO is readable
 *
 * @param sio_block pointer to sio struct
 * @retval value received from the FIFO
 */
static inline uint32_t rpi_pico_mbox_read(sio_hw_t *const sio_block)
{
	return sio_block->fifo_rd;
}


/**
 * @}
 */

#endif /* ZEPHYR_DRIVERS_MISC_MBOX_PICO_RPI_PIO_PICO_RPI_H_ */

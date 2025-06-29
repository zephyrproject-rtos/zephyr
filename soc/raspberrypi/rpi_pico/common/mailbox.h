/*
 * Copyright (c) 2025 Dan Collins
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Mailbox interface for the Raspberry Pi RP2 family processors
 */

#ifndef _RPI_PICO_MAILBOX_H_
#define _RPI_PICO_MAILBOX_H_

#include <hardware/structs/sio.h>
#include "cmsis_gcc.h"

static inline void mailbox_put_blocking(uint32_t value)
{
	while (!(sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS)) {
		;
	}

	sio_hw->fifo_wr = value;

	/* Inform other CPU about FIFO update. */
	__SEV();
}

static inline uint32_t mailbox_pop_blocking(void)
{
	while (!(sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS)) {
		__WFE();
	}

	return sio_hw->fifo_rd;
}

static inline void mailbox_flush(void)
{
	/* Read all valid data from the mailbox. */
	while (sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS) {
		(void)sio_hw->fifo_rd;
	}
}

#endif /* _RPI_PICO_MAILBOX_H_ */

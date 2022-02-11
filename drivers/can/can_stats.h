/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_CAN_STATS_H_
#define ZEPHYR_DRIVERS_CAN_CAN_STATS_H_

#if defined(CONFIG_CAN_STATS)

#include <device.h>
#include <stats/stats.h>

/** @cond INTERNAL_HIDDEN */

STATS_SECT_START(can)
STATS_SECT_ENTRY32(bit0_error)
STATS_SECT_ENTRY32(bit1_error)
STATS_SECT_ENTRY32(stuff_error)
STATS_SECT_ENTRY32(crc_error)
STATS_SECT_ENTRY32(form_error)
STATS_SECT_ENTRY32(ack_error)
STATS_SECT_END;

STATS_NAME_START(can)
STATS_NAME(can, bit0_error)
STATS_NAME(can, bit1_error)
STATS_NAME(can, stuff_error)
STATS_NAME(can, crc_error)
STATS_NAME(can, form_error)
STATS_NAME(can, ack_error)
STATS_NAME_END(can);

/** @endcond */

/**
 * @brief Increment the bit0 error counter for a CAN device
 *
 * The bit0 error counter is incremented when the CAN controller is unable to
 * transmit a dominant bit.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_BIT0_ERROR_INC(stats_) STATS_INC(stats_, bit0_error)

/**
 * @brief Increment the bit1 (recessive) error counter for a CAN device
 *
 * The bit1 error counter is incremented when the CAN controller is unable to
 * transmit a recessive bit.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_BIT1_ERROR_INC(stats_) STATS_INC(stats_, bit1_error)

/**
 * @brief Increment the stuffing error counter for a CAN device
 *
 * The stuffing error counter is incremented when the CAN controller detects a
 * bit stuffing error.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_STUFF_ERROR_INC(stats_) STATS_INC(stats_, stuff_error)

/**
 * @brief Increment the CRC error counter for a CAN device
 *
 * The CRC error counter is incremented when the CAN controller detects a frame
 * with an invalid CRC.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_CRC_ERROR_INC(stats_) STATS_INC(stats_, crc_error)

/**
 * @brief Increment the form error counter for a CAN device
 *
 * The form error counter is incremented when the CAN controller detects a
 * fixed-form bit field containing illegal bits.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_FORM_ERROR_INC(stats_) STATS_INC(stats_, form_error)

/**
 * @brief Increment the acknowledge error counter for a CAN device
 *
 * The acknowledge error counter is incremented when the CAN controller does not
 * monitor a dominant bit in the ACK slot.
 *
 * @param stats_ The ``struct stats_can`` statistics group for the driver instance.
 */
#define CAN_STATS_ACK_ERROR_INC(stats_) STATS_INC(stats_, ack_error)

/**
 * @brief Initialize CAN controller device statistics
 *
 * @param dev Pointer to the device structure for the driver instance.
 */
static inline void can_stats_init(const struct device *dev, struct stats_can *stats)
{
	stats_init(&stats->s_hdr, STATS_SIZE_32, 6, STATS_NAME_INIT_PARMS(can));
	stats_register(dev->name, &stats->s_hdr);
}

#else  /* CONFIG_CAN_STATS */

struct stats_can {
};

#define CAN_STATS_BIT0_ERROR_INC(stats_)
#define CAN_STATS_BIT1_ERROR_INC(stats_)
#define CAN_STATS_STUFF_ERROR_INC(stats_)
#define CAN_STATS_CRC_ERROR_INC(stats_)
#define CAN_STATS_FORM_ERROR_INC(stats_)
#define CAN_STATS_ACK_ERROR_INC(stats_)

static inline void can_stats_init(const struct device *dev, struct stats_can *stats)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(stats);
}

#endif /* CONFIG_CAN_STATS */

#endif /* ZEPHYR_DRIVERS_CAN_CAN_STATS_H_ */

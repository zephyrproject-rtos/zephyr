/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup atm90e26_interface
 * @brief Device-specific quantities of the Microchip ATM90E26 energy metering IC.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E26_H_
#define ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E26_H_

/**
 * @defgroup atm90e26_interface ATM90E26
 * @ingroup energy_meter_interface
 * @brief Microchip ATM90E26 single-phase energy metering IC
 *
 * The ATM90E26 measures the voltage, the L line current on @ref ENERGY_METER_LINE_A and the
 * N line current on @ref ENERGY_METER_LINE_N, with power, power factor and phase angle for
 * each current channel. Frequency and energy are reported on @ref ENERGY_METER_LINE_TOTAL.
 *
 * The chip clears its energy registers when they are read, and the driver adds each reading
 * to a 64-bit accumulator. Read the energy quantities often enough that a 16-bit register
 * (6553.5 CF pulses) does not overflow between two reads.
 *
 * The current registers saturate at 65.535 A and the power registers at 32.767 kW. For
 * larger loads, calibrate the chip to read a fraction of the actual current and set the
 * current-scale devicetree property; the driver then multiplies current, power and energy back
 * to their actual values.
 *
 * Event callbacks run in the system work queue. Pulse events 0 and 1 are CF1 and CF2, alarm
 * event 0 is IRQ and fault event 0 is WarnOut.
 * @{
 */

#include <zephyr/drivers/energy_meter.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device-specific quantities of the ATM90E26.
 */
enum energy_meter_quantity_atm90e26 {
	/** Absolute active energy, in uWh, on the total line. */
	ENERGY_METER_ATM90E26_ACTIVE_ENERGY_ABS = ENERGY_METER_QUANTITY_PRIV_START,
	/** Absolute reactive energy, in uvarh, on the total line. */
	ENERGY_METER_ATM90E26_REACTIVE_ENERGY_ABS,
	/**
	 * SysStatus register, on the total line. Reading it clears the flags, which releases
	 * the IRQ and WarnOut pins.
	 */
	ENERGY_METER_ATM90E26_SYS_STATUS,
	/** EnStatus metering status register, on the total line. */
	ENERGY_METER_ATM90E26_EN_STATUS,
	/** Number of quantities, common and device-specific. */
	ENERGY_METER_ATM90E26_QUANTITY_COUNT,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E26_H_ */

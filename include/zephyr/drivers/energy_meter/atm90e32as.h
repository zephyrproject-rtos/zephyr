/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup atm90e32as_interface
 * @brief Device-specific quantities of the Microchip ATM90E32AS energy metering IC.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E32AS_H_
#define ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E32AS_H_

/**
 * @defgroup atm90e32as_interface ATM90E32AS
 * @ingroup energy_meter_interface
 * @brief Microchip ATM90E32AS poly-phase energy metering IC
 *
 * The ATM90E32AS supports the common quantities on lines A to C, the neutral current on
 * line N, and totals on @ref ENERGY_METER_LINE_TOTAL for power, power factor and energy.
 * Frequency and temperature are reported on @ref ENERGY_METER_LINE_TOTAL.
 *
 * The chip clears its energy registers when they are read, and the driver adds each reading
 * to a 64-bit accumulator. Read the energy quantities often enough that a 16-bit register
 * (655.35 CF pulses) does not overflow between two reads.
 *
 * The current and power registers saturate at 65.535 A. For larger currents, calibrate the
 * chip to read a fraction of the actual current and set the current-scale devicetree property;
 * the driver then multiplies current, power and energy back to their actual values.
 *
 * The chip measures its temperature about every 100 ms. Until the first measurement after
 * initialization or resume, reading @ref ENERGY_METER_TEMPERATURE returns -EAGAIN.
 *
 * With pm0-gpios and pm1-gpios, suspending the device puts the chip in Idle mode, where it
 * does not meter. With device runtime PM enabled this happens whenever no read is in
 * progress, so leave runtime PM disabled for continuous energy metering.
 *
 * Event callbacks run in the system work queue. Pulse events 0 to 3 are CF1 to CF4, alarm
 * events 0 and 1 are IRQ0 and IRQ1, and fault event 0 is WarnOut.
 * @{
 */

#include <zephyr/drivers/energy_meter.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device-specific quantities of the ATM90E32AS.
 */
enum energy_meter_quantity_atm90e32as {
	/** Active fundamental power, in uW, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_FUND_POWER = ENERGY_METER_QUANTITY_PRIV_START,
	/** Active harmonic power, in uW, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_HARM_POWER,
	/** Imported active fundamental energy, in uWh, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_FUND_ENERGY_IMPORT,
	/** Exported active fundamental energy, in uWh, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_FUND_ENERGY_EXPORT,
	/** Imported active harmonic energy, in uWh, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_HARM_ENERGY_IMPORT,
	/** Exported active harmonic energy, in uWh, on lines A to C and the total. */
	ENERGY_METER_ATM90E32AS_HARM_ENERGY_EXPORT,
	/** Voltage peak in the SagPeakDetCfg period, in uV, on lines A to C. */
	ENERGY_METER_ATM90E32AS_VOLTAGE_PEAK,
	/** Current peak in the SagPeakDetCfg period, in uA, on lines A to C. */
	ENERGY_METER_ATM90E32AS_CURRENT_PEAK,
	/**
	 * EMMState0 in bits 31:16 and EMMState1 in bits 15:0, on the total line.
	 */
	ENERGY_METER_ATM90E32AS_EMM_STATE,
	/**
	 * EMMIntState0 in bits 31:16 and EMMIntState1 in bits 15:0, on the total line.
	 * Reading it clears the reported interrupt flags, which releases IRQ0 and IRQ1.
	 */
	ENERGY_METER_ATM90E32AS_EMM_INT_STATE,
	/** Number of quantities, common and device-specific. */
	ENERGY_METER_ATM90E32AS_QUANTITY_COUNT,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_ATM90E32AS_H_ */

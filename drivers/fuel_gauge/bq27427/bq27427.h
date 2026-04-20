/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FUELGAUGE_BQ27427_BQ27427_H_
#define ZEPHYR_DRIVERS_FUELGAUGE_BQ27427_BQ27427_H_

/* Commands */
enum bq27427_commands {
	/* Standard commands */
	BQ27427_CMD_CONTROL = 0x00u,                         /* Control() */
	BQ27427_CMD_TEMPERATURE = 0x02u,                     /* Temperature() */
	BQ27427_CMD_VOLTAGE = 0x04u,                         /* Voltage() */
	BQ27427_CMD_FLAGS = 0x06u,                           /* Flags() */
	BQ27427_CMD_NOMINAL_AVAILABLE_CAPACITY = 0x08u,      /* NominalAvailableCapacity() */
	BQ27427_CMD_FULL_AVAILABLE_CAPACITY = 0x0Au,         /* FullAvailableCapacity() */
	BQ27427_CMD_REMAINING_CAPACITY = 0x0Cu,              /* RemainingCapacity() */
	BQ27427_CMD_FULL_CHARGE_CAPACITY = 0x0Eu,            /* FullChargeCapacity() */
	BQ27427_CMD_AVERAGE_CURRENT = 0x10u,                 /* AverageCurrent() */
	BQ27427_CMD_AVERAGE_POWER = 0x18u,                   /* AveragePower() */
	BQ27427_CMD_STATE_OF_CHARGE = 0x1Cu,                 /* StateOfCharge() SOC */
	BQ27427_CMD_INTERNAL_TEMPERATURE = 0x1Eu,            /* InternalTemperature() */
	BQ27427_CMD_REMAINING_CAPACITY_UNFILTERED = 0x28u,   /* RemainingCapacityUnfiltered() */
	BQ27427_CMD_REMAINING_CAPACITY_FILTERED = 0x2Au,     /* RemainingCapacityFiltered() */
	BQ27427_CMD_FULL_CHARGE_CAPACITY_UNFILTERED = 0x2Cu, /* FullChargeCapacityUnfiltered() */
	BQ27427_CMD_FULL_CHARGE_CAPACITY_FILTERED = 0x2Eu,   /* FullChargeCapacityFiltered() */
	BQ27427_CMD_STATE_OF_CHARGE_UNFILTERED = 0x30u,      /* StateOfChargeUnfiltered() */

	/* Extended commands */
	BQ27427_EXT_CMD_DATA_CLASS = 0x3Eu,          /* DataClass() */
	BQ27427_EXT_CMD_DATA_BLOCK = 0x3Fu,          /* DataBlock() */
	BQ27427_EXT_CMD_BLOCK_DATA_START = 0x40u,    /* BlockData() start (0x40–0x5F) */
	BQ27427_EXT_CMD_BLOCK_DATA_CHECKSUM = 0x60u, /* BlockDataCheckSum() */
	BQ27427_EXT_CMD_BLOCK_DATA_CONTROL = 0x61u,  /* BlockDataControl() */
};

#endif

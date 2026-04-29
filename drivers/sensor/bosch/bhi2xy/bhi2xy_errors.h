/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_ERRORS_H_
#define ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_ERRORS_H_

#include <stdint.h>

typedef enum {
	BHI2XY_ERROR_NONE = 0x00,

	/* Bootloader / Firmware Upload Errors */
	BHI2XY_ERROR_FW_EXPECTED_VER_MISMATCH =
		0x10, /* Bootloader reports: Firmware Expected Version Mismatch */
	BHI2XY_ERROR_FW_UPLOAD_BAD_HEADER_CRC =
		0x11, /* Bootloader reports: Firmware Upload Failed: Bad Header CRC */
	BHI2XY_ERROR_FW_UPLOAD_SHA_HASH_MISMATCH =
		0x12, /* Bootloader reports: Firmware Upload Failed: SHA Hash Mismatch */
	BHI2XY_ERROR_FW_UPLOAD_BAD_IMAGE_CRC =
		0x13, /* Bootloader reports: Firmware Upload Failed: Bad Image CRC */
	BHI2XY_ERROR_FW_UPLOAD_ECDSA_FAILED = 0x14, /* Bootloader reports: Firmware Upload Failed:
						       ECDSA Signature Verification Failed */
	BHI2XY_ERROR_FW_UPLOAD_BAD_PUB_KEY_CRC =
		0x15, /* Bootloader reports: Firmware Upload Failed: Bad Public Key CRC */
	BHI2XY_ERROR_FW_UPLOAD_SIGNED_FW_REQUIRED =
		0x16, /* Bootloader reports: Firmware Upload Failed: Signed Firmware Required */
	BHI2XY_ERROR_FW_UPLOAD_HEADER_MISSING =
		0x17, /* Bootloader reports: Firmware Upload Failed: FW Header Missing */
	BHI2XY_ERROR_UNEXPECTED_WATCHDOG_RESET =
		0x19, /* Bootloader reports: Unexpected Watchdog Reset */
	BHI2XY_ERROR_ROM_VERSION_MISMATCH = 0x1A, /* ROM Version Mismatch */
	BHI2XY_ERROR_FATAL_FW_ERROR = 0x1B,       /* Bootloader reports: Fatal Firmware Error */

	/* Chained Firmware Errors */
	BHI2XY_ERROR_CHAINED_FW_NEXT_PAYLOAD_NOT_FOUND =
		0x1C, /* Chained Firmware Error: Next Payload Not Found */
	BHI2XY_ERROR_CHAINED_FW_PAYLOAD_NOT_VALID =
		0x1D, /* Chained Firmware Error: Payload Not Valid */
	BHI2XY_ERROR_CHAINED_FW_ENTRIES_INVALID =
		0x1E, /* Chained Firmware Error: Payload Entries Invalid */

	BHI2XY_ERROR_OTP_CRC_INVALID =
		0x1F, /* Bootloader reports: Bootloader Error: OTP CRC Invalid */

	/* Sensor Initialization & Runtime Errors */
	BHI2XY_ERROR_FW_INIT_FAILED = 0x20, /* Firmware Init Failed */
	BHI2XY_ERROR_SENSOR_INIT_UNEXPECTED_ID =
		0x21, /* Sensor Init Failed: Unexpected Device ID */
	BHI2XY_ERROR_SENSOR_INIT_NO_RESPONSE =
		0x22,                              /* Sensor Init Failed: No Response from Device */
	BHI2XY_ERROR_SENSOR_INIT_UNKNOWN = 0x23,   /* Sensor Init Failed: Unknown */
	BHI2XY_ERROR_NO_VALID_DATA = 0x24,         /* Sensor Error: No Valid Data */
	BHI2XY_ERROR_SLOW_SAMPLE_RATE = 0x25,      /* Slow Sample Rate */
	BHI2XY_ERROR_DATA_OVERFLOW = 0x26,         /* Data Overflow (saturated sensor data) */
	BHI2XY_ERROR_STACK_OVERFLOW = 0x27,        /* Stack Overflow */
	BHI2XY_ERROR_INSUFFICIENT_FREE_RAM = 0x28, /* Insufficient Free RAM */
	BHI2XY_ERROR_SENSOR_INIT_DRIVER_PARSE_ERR =
		0x29,                                /* Sensor Init Failed: Driver Parsing Error */
	BHI2XY_ERROR_TOO_MANY_RAM_BANKS = 0x2A,      /* Too Many RAM Banks Required */
	BHI2XY_ERROR_INVALID_EVENT_SPECIFIED = 0x2B, /* Invalid Event Specified */
	BHI2XY_ERROR_MORE_THAN_32_ON_CHANGE = 0x2C,  /* More than 32 On Change */
	BHI2XY_ERROR_FW_TOO_LARGE = 0x2D,            /* Firmware Too Large */
	BHI2XY_ERROR_INVALID_RAM_BANKS = 0x2F,       /* Invalid RAM Banks */
	BHI2XY_ERROR_MATH_ERROR = 0x30,              /* Math Error */

	/* System & Instruction Errors */
	BHI2XY_ERROR_MEMORY_ERROR = 0x40,        /* Memory Error */
	BHI2XY_ERROR_SWI3_ERROR = 0x41,          /* SWI3 Error */
	BHI2XY_ERROR_SWI4_ERROR = 0x42,          /* SWI4 Error */
	BHI2XY_ERROR_ILLEGAL_INSTRUCTION = 0x43, /* Illegal Instruction Error */
	BHI2XY_ERROR_UNHANDLED_INTERRUPT =
		0x44, /* Bootloader reports: Unhandled Interrupt Error / Exception */
	BHI2XY_ERROR_INVALID_MEMORY_ACCESS = 0x45, /* Invalid Memory Access */

	/* Algorithm Errors (BSX) */
	BHI2XY_ERROR_ALG_BSX_INIT = 0x50,              /* Algorithm Error: BSX Init */
	BHI2XY_ERROR_ALG_BSX_DO_STEP = 0x51,           /* Algorithm Error: BSX Do Step */
	BHI2XY_ERROR_ALG_UPDATE_SUB = 0x52,            /* Algorithm Error: Update Sub */
	BHI2XY_ERROR_ALG_GET_SUB = 0x53,               /* Algorithm Error: Get Sub */
	BHI2XY_ERROR_ALG_GET_PHYS = 0x54,              /* Algorithm Error: Get Phys */
	BHI2XY_ERROR_ALG_UNSUPPORTED_PHYS_RATE = 0x55, /* Algorithm Error: Unsupported Phys Rate */
	BHI2XY_ERROR_ALG_DRIVER_NOT_FOUND = 0x56,      /* Algorithm Error: Cannot find BSX Driver */

	/* Self-Test & FOC */
	BHI2XY_ERROR_SELF_TEST_FAILURE = 0x60,         /* Sensor Self-Test Failure */
	BHI2XY_ERROR_SELF_TEST_X_AXIS = 0x61,          /* Sensor Self-Test X Axis Failure */
	BHI2XY_ERROR_SELF_TEST_Y_AXIS = 0x62,          /* Sensor Self-Test Y Axis Failure */
	BHI2XY_ERROR_SELF_TEST_Z_AXIS = 0x64,          /* Sensor Self-Test Z Axis Failure */
	BHI2XY_ERROR_FOC_FAILURE = 0x65,               /* FOC Failure */
	BHI2XY_ERROR_SENSOR_BUSY = 0x66,               /* Sensor Busy */
	BHI2XY_ERROR_SELF_TEST_FOC_UNSUPPORTED = 0x6F, /* Self-Test or FOC Test Unsupported */

	/* Host Interface & Communication */
	BHI2XY_ERROR_NO_HOST_INTERRUPT_SET = 0x72, /* No Host Interrupt Set */
	BHI2XY_ERROR_EVENT_ID_UNKNOWN_SIZE =
		0x73, /* Event ID Passed to Host Interface Has No Known Size */
	BHI2XY_ERROR_HOST_DL_UNDERFLOW =
		0x75, /* Host Download Channel Underflow (Host Read Too Fast) */
	BHI2XY_ERROR_HOST_UL_OVERFLOW =
		0x76,                      /* Host Upload Channel Overflow (Host Wrote Too Fast) */
	BHI2XY_ERROR_HOST_DL_EMPTY = 0x77, /* Host Download Channel Empty */
	BHI2XY_ERROR_DMA_ERROR = 0x78,     /* DMA Error */
	BHI2XY_ERROR_CORRUPTED_INPUT_BLOCK_CHAIN = 0x79,  /* Corrupted Input Block Chain */
	BHI2XY_ERROR_CORRUPTED_OUTPUT_BLOCK_CHAIN = 0x7A, /* Corrupted Output Block Chain */
	BHI2XY_ERROR_BUFFER_BLOCK_MANAGER_ERROR = 0x7B,   /* Buffer Block Manager Error */
	BHI2XY_ERROR_INPUT_CHANNEL_ALIGNMENT = 0x7C,      /* Input Channel Not Word Aligned */
	BHI2XY_ERROR_TOO_MANY_FLUSH_EVENTS = 0x7D,        /* Too Many Flush Events */
	BHI2XY_ERROR_UNKNOWN_HOST_CHANNEL_ERROR = 0x7E,   /* Unknown Host Channel Error */

	BHI2XY_ERROR_DECIMATION_TOO_LARGE = 0x81,      /* Decimation Too Large */
	BHI2XY_ERROR_MASTER_QUEUE_OVERFLOW = 0x90,     /* Master SPI/I2C Queue Overflow */
	BHI2XY_ERROR_CALLBACK_ERROR = 0x91,            /* SPI/I2C Callback Error */
	BHI2XY_ERROR_TIMER_SCHEDULING_ERROR = 0xA0,    /* Timer Scheduling Error */
	BHI2XY_ERROR_INVALID_GPIO_FOR_HOST_IRQ = 0xB0, /* Invalid GPIO for Host IRQ */
	BHI2XY_ERROR_SENDING_INIT_META_EVENTS = 0xB1,  /* Error Sending Initialized Meta Events */

	/* Command Processing */
	BHI2XY_ERROR_CMD_ERROR = 0xC0,           /* Bootloader reports: Command Error */
	BHI2XY_ERROR_CMD_TOO_LONG = 0xC1,        /* Bootloader reports: Command Too Long */
	BHI2XY_ERROR_CMD_BUFFER_OVERFLOW = 0xC2, /* Bootloader reports: Command Buffer Overflow */

	/* User Mode / System Calls */
	BHI2XY_ERROR_USER_SYSCALL_INVALID = 0xD0, /* User Mode Error: Sys Call Invalid */
	BHI2XY_ERROR_USER_TRAP_INVALID = 0xD1,    /* User Mode Error: Trap Invalid */

	/* Extended / Specific Errors */
	BHI2XY_ERROR_FW_UPLOAD_HEADER_CORRUPT =
		0xE1, /* Firmware Upload Failed: Firmware header corrupt */
	BHI2XY_ERROR_INJECT_INVALID_INPUT_STREAM =
		0xE2 /* Sensor Data Injection: Invalid input stream */

} bhi2xy_sensor_error_t;

const char *bhi2xy_get_sensor_error_text(uint8_t sensor_error);
const char *bhi2xy_get_api_error(int8_t error_code);

#endif /* ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_ERRORS_H_ */

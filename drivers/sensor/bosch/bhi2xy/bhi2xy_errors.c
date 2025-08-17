/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy_errors.h"

#include "bhy2_defs.h"

const char *bhi2xy_get_sensor_error_text(uint8_t sensor_error)
{
	switch (sensor_error) {
	case BHI2XY_ERROR_NONE:
		return "";

	/* Bootloader / Firmware Upload Errors */
	case BHI2XY_ERROR_FW_EXPECTED_VER_MISMATCH:
		return "[Sensor error] Bootloader reports: Firmware Expected Version Mismatch";
	case BHI2XY_ERROR_FW_UPLOAD_BAD_HEADER_CRC:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Header CRC";
	case BHI2XY_ERROR_FW_UPLOAD_SHA_HASH_MISMATCH:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: SHA Hash "
		       "Mismatch";
	case BHI2XY_ERROR_FW_UPLOAD_BAD_IMAGE_CRC:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Image CRC";
	case BHI2XY_ERROR_FW_UPLOAD_ECDSA_FAILED:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: ECDSA Signature "
		       "Verification Failed";
	case BHI2XY_ERROR_FW_UPLOAD_BAD_PUB_KEY_CRC:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: Bad Public Key "
		       "CRC";
	case BHI2XY_ERROR_FW_UPLOAD_SIGNED_FW_REQUIRED:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: Signed Firmware "
		       "Required";
	case BHI2XY_ERROR_FW_UPLOAD_HEADER_MISSING:
		return "[Sensor error] Bootloader reports: Firmware Upload Failed: FW Header "
		       "Missing";
	case BHI2XY_ERROR_UNEXPECTED_WATCHDOG_RESET:
		return "[Sensor error] Bootloader reports: Unexpected Watchdog Reset";
	case BHI2XY_ERROR_ROM_VERSION_MISMATCH:
		return "[Sensor error] ROM Version Mismatch";
	case BHI2XY_ERROR_FATAL_FW_ERROR:
		return "[Sensor error] Bootloader reports: Fatal Firmware Error";

	/* Chained Firmware Errors */
	case BHI2XY_ERROR_CHAINED_FW_NEXT_PAYLOAD_NOT_FOUND:
		return "[Sensor error] Chained Firmware Error: Next Payload Not Found";
	case BHI2XY_ERROR_CHAINED_FW_PAYLOAD_NOT_VALID:
		return "[Sensor error] Chained Firmware Error: Payload Not Valid";
	case BHI2XY_ERROR_CHAINED_FW_ENTRIES_INVALID:
		return "[Sensor error] Chained Firmware Error: Payload Entries Invalid";

	case BHI2XY_ERROR_OTP_CRC_INVALID:
		return "[Sensor error] Bootloader reports: Bootloader Error: OTP CRC Invalid";

	/* Sensor Initialization & Runtime Errors */
	case BHI2XY_ERROR_FW_INIT_FAILED:
		return "[Sensor error] Firmware Init Failed";
	case BHI2XY_ERROR_SENSOR_INIT_UNEXPECTED_ID:
		return "[Sensor error] Sensor Init Failed: Unexpected Device ID";
	case BHI2XY_ERROR_SENSOR_INIT_NO_RESPONSE:
		return "[Sensor error] Sensor Init Failed: No Response from Device";
	case BHI2XY_ERROR_SENSOR_INIT_UNKNOWN:
		return "[Sensor error] Sensor Init Failed: Unknown";
	case BHI2XY_ERROR_NO_VALID_DATA:
		return "[Sensor error] Sensor Error: No Valid Data";
	case BHI2XY_ERROR_SLOW_SAMPLE_RATE:
		return "[Sensor error] Slow Sample Rate";
	case BHI2XY_ERROR_DATA_OVERFLOW:
		return "[Sensor error] Data Overflow (saturated sensor data)";
	case BHI2XY_ERROR_STACK_OVERFLOW:
		return "[Sensor error] Stack Overflow";
	case BHI2XY_ERROR_INSUFFICIENT_FREE_RAM:
		return "[Sensor error] Insufficient Free RAM";
	case BHI2XY_ERROR_SENSOR_INIT_DRIVER_PARSE_ERR:
		return "[Sensor error] Sensor Init Failed: Driver Parsing Error";
	case BHI2XY_ERROR_TOO_MANY_RAM_BANKS:
		return "[Sensor error] Too Many RAM Banks Required";
	case BHI2XY_ERROR_INVALID_EVENT_SPECIFIED:
		return "[Sensor error] Invalid Event Specified";
	case BHI2XY_ERROR_MORE_THAN_32_ON_CHANGE:
		return "[Sensor error] More than 32 On Change";
	case BHI2XY_ERROR_FW_TOO_LARGE:
		return "[Sensor error] Firmware Too Large";
	case BHI2XY_ERROR_INVALID_RAM_BANKS:
		return "[Sensor error] Invalid RAM Banks";
	case BHI2XY_ERROR_MATH_ERROR:
		return "[Sensor error] Math Error";

	/* System & Instruction Errors */
	case BHI2XY_ERROR_MEMORY_ERROR:
		return "[Sensor error] Memory Error";
	case BHI2XY_ERROR_SWI3_ERROR:
		return "[Sensor error] SWI3 Error";
	case BHI2XY_ERROR_SWI4_ERROR:
		return "[Sensor error] SWI4 Error";
	case BHI2XY_ERROR_ILLEGAL_INSTRUCTION:
		return "[Sensor error] Illegal Instruction Error";
	case BHI2XY_ERROR_UNHANDLED_INTERRUPT:
		return "[Sensor error] Bootloader reports: Unhandled Interrupt Error / Exception / "
		       "Postmortem Available";
	case BHI2XY_ERROR_INVALID_MEMORY_ACCESS:
		return "[Sensor error] Invalid Memory Access";

	/* Algorithm Errors (BSX) */
	case BHI2XY_ERROR_ALG_BSX_INIT:
		return "[Sensor error] Algorithm Error: BSX Init";
	case BHI2XY_ERROR_ALG_BSX_DO_STEP:
		return "[Sensor error] Algorithm Error: BSX Do Step";
	case BHI2XY_ERROR_ALG_UPDATE_SUB:
		return "[Sensor error] Algorithm Error: Update Sub";
	case BHI2XY_ERROR_ALG_GET_SUB:
		return "[Sensor error] Algorithm Error: Get Sub";
	case BHI2XY_ERROR_ALG_GET_PHYS:
		return "[Sensor error] Algorithm Error: Get Phys";
	case BHI2XY_ERROR_ALG_UNSUPPORTED_PHYS_RATE:
		return "[Sensor error] Algorithm Error: Unsupported Phys Rate";
	case BHI2XY_ERROR_ALG_DRIVER_NOT_FOUND:
		return "[Sensor error] Algorithm Error: Cannot find BSX Driver";

	/* Self-Test & FOC */
	case BHI2XY_ERROR_SELF_TEST_FAILURE:
		return "[Sensor error] Sensor Self-Test Failure";
	case BHI2XY_ERROR_SELF_TEST_X_AXIS:
		return "[Sensor error] Sensor Self-Test X Axis Failure";
	case BHI2XY_ERROR_SELF_TEST_Y_AXIS:
		return "[Sensor error] Sensor Self-Test Y Axis Failure";
	case BHI2XY_ERROR_SELF_TEST_Z_AXIS:
		return "[Sensor error] Sensor Self-Test Z Axis Failure";
	case BHI2XY_ERROR_FOC_FAILURE:
		return "[Sensor error] FOC Failure";
	case BHI2XY_ERROR_SENSOR_BUSY:
		return "[Sensor error] Sensor Busy";
	case BHI2XY_ERROR_SELF_TEST_FOC_UNSUPPORTED:
		return "[Sensor error] Self-Test or FOC Test Unsupported";

	/* Host Interface & Communication */
	case BHI2XY_ERROR_NO_HOST_INTERRUPT_SET:
		return "[Sensor error] No Host Interrupt Set";
	case BHI2XY_ERROR_EVENT_ID_UNKNOWN_SIZE:
		return "[Sensor error] Event ID Passed to Host Interface Has No Known Size";
	case BHI2XY_ERROR_HOST_DL_UNDERFLOW:
		return "[Sensor error] Host Download Channel Underflow (Host Read Too Fast)";
	case BHI2XY_ERROR_HOST_UL_OVERFLOW:
		return "[Sensor error] Host Upload Channel Overflow (Host Wrote Too Fast)";
	case BHI2XY_ERROR_HOST_DL_EMPTY:
		return "[Sensor error] Host Download Channel Empty";
	case BHI2XY_ERROR_DMA_ERROR:
		return "[Sensor error] DMA Error";
	case BHI2XY_ERROR_CORRUPTED_INPUT_BLOCK_CHAIN:
		return "[Sensor error] Corrupted Input Block Chain";
	case BHI2XY_ERROR_CORRUPTED_OUTPUT_BLOCK_CHAIN:
		return "[Sensor error] Corrupted Output Block Chain";
	case BHI2XY_ERROR_BUFFER_BLOCK_MANAGER_ERROR:
		return "[Sensor error] Buffer Block Manager Error";
	case BHI2XY_ERROR_INPUT_CHANNEL_ALIGNMENT:
		return "[Sensor error] Input Channel Not Word Aligned";
	case BHI2XY_ERROR_TOO_MANY_FLUSH_EVENTS:
		return "[Sensor error] Too Many Flush Events";
	case BHI2XY_ERROR_UNKNOWN_HOST_CHANNEL_ERROR:
		return "[Sensor error] Unknown Host Channel Error";

	case BHI2XY_ERROR_DECIMATION_TOO_LARGE:
		return "[Sensor error] Decimation Too Large";
	case BHI2XY_ERROR_MASTER_QUEUE_OVERFLOW:
		return "[Sensor error] Master SPI/I2C Queue Overflow";
	case BHI2XY_ERROR_CALLBACK_ERROR:
		return "[Sensor error] SPI/I2C Callback Error";
	case BHI2XY_ERROR_TIMER_SCHEDULING_ERROR:
		return "[Sensor error] Timer Scheduling Error";
	case BHI2XY_ERROR_INVALID_GPIO_FOR_HOST_IRQ:
		return "[Sensor error] Invalid GPIO for Host IRQ";
	case BHI2XY_ERROR_SENDING_INIT_META_EVENTS:
		return "[Sensor error] Error Sending Initialized Meta Events";

	/* Command Processing */
	case BHI2XY_ERROR_CMD_ERROR:
		return "[Sensor error] Bootloader reports: Command Error";
	case BHI2XY_ERROR_CMD_TOO_LONG:
		return "[Sensor error] Bootloader reports: Command Too Long";
	case BHI2XY_ERROR_CMD_BUFFER_OVERFLOW:
		return "[Sensor error] Bootloader reports: Command Buffer Overflow";

	/* User Mode / System Calls */
	case BHI2XY_ERROR_USER_SYSCALL_INVALID:
		return "[Sensor error] User Mode Error: Sys Call Invalid";
	case BHI2XY_ERROR_USER_TRAP_INVALID:
		return "[Sensor error] User Mode Error: Trap Invalid";

	/* Extended / Specific Errors */
	case BHI2XY_ERROR_FW_UPLOAD_HEADER_CORRUPT:
		return "[Sensor error] Firmware Upload Failed: Firmware header corrupt";
	case BHI2XY_ERROR_INJECT_INVALID_INPUT_STREAM:
		return "[Sensor error] Sensor Data Injection: Invalid input stream";

	default:
		return "[Sensor error] Unknown error code";
	}
}

const char *bhi2xy_get_api_error(int8_t error_code)
{
	switch (error_code) {
	case BHY2_OK:
		return "";
	case BHY2_E_NULL_PTR:
		return "[API Error] Null pointer";
	case BHY2_E_INVALID_PARAM:
		return "[API Error] Invalid parameter";
	case BHY2_E_IO:
		return "[API Error] IO error";
	case BHY2_E_MAGIC:
		return "[API Error] Invalid firmware";
	case BHY2_E_TIMEOUT:
		return "[API Error] Timed out";
	case BHY2_E_BUFFER:
		return "[API Error] Invalid buffer";
	case BHY2_E_INVALID_FIFO_TYPE:
		return "[API Error] Invalid FIFO type";
	case BHY2_E_INVALID_EVENT_SIZE:
		return "[API Error] Invalid Event size";
	case BHY2_E_PARAM_NOT_SET:
		return "[API Error] Parameter not set";
	default:
		return "[API Error] Unknown API error code";
	}
}

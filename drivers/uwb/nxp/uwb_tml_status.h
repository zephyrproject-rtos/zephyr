/*
 * Copyright 2012-2019,2021-2023 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * UWB Status Values - Function Return Codes
 */

#ifndef __UWB_TML_STATUS_H__
#define __UWB_TML_STATUS_H__

typedef uint16_t UWBSTATUS; /* Return values */

/**
 * The function indicates successful completion
 */
#define UWBSTATUS_SUCCESS (0x0000)

/**
 *  The function indicates successful completion
 */
#define UWBSTATUS_OK (UWBSTATUS_SUCCESS)

/**
 * At least one parameter could not be properly interpreted
 */
#define UWBSTATUS_INVALID_PARAMETER (0x0001)

/**
 * The buffer provided by the caller is too small
 */
#define UWBSTATUS_BUFFER_TOO_SMALL (0x0003)

/**
 * Device specifier/handle value is invalid for the operation
 */
#define UWBSTATUS_INVALID_DEVICE (0xFFFF)

/**
 * A non-blocking function returns this immediately to indicate
 * that an internal operation is in progress
 */
#define UWBSTATUS_PENDING (0x000D)

/**
 * A board communication error occurred
 * (e.g. Configuration went wrong)
 */
#define UWBSTATUS_BOARD_COMMUNICATION_ERROR (0x000F)

/**
 * Invalid State of the particular state machine
 */
#define UWBSTATUS_INVALID_STATE (0x0011)

/**
 * The Layer is already initialized, hence initialization repeated.
 */
#define UWBSTATUS_ALREADY_INITIALISED (0x0032)

/**
 *  The system is busy with the previous operation.
 */
#define UWBSTATUS_BUSY (0x006F)

/* NDEF Mapping error codes */

/** Read operation failed */
#define UWBSTATUS_READ_FAILED (0x0014)

/**
 * Write operation failed
 */
#define UWBSTATUS_WRITE_FAILED (0x0015)

/**
 * Response Time out for the control message(UWBC not responded)
 */
#define UWBSTATUS_RESPONSE_TIMEOUT (0x0025)

/**
 * Unknown error Status Codes
 */
#define UWBSTATUS_UNKNOWN_ERROR (0x00FE)

/**
 * Status code for failure
 */
#define UWBSTATUS_FAILED (0x00FF)

/**
 * Indicates Connection failed
 */
#define UWBSTATUS_CONNECTION_FAILED (0x0047)

/** IRQ Read Timeout */
#define UWBSTATUS_IRQ_READ_TIMEOUT (-1)

/** UWB Status enums.
 *
 * To be used in future versions for better debugging and developer friendlyness.
 */

typedef enum {
	/** @ref UWBSTATUS_SUCCESS */
	kUWBSTATUS_SUCCESS = UWBSTATUS_SUCCESS, /**< **0x0000** */
	/** @ref UWBSTATUS_OK */
	kUWBSTATUS_OK = UWBSTATUS_OK, /**< **0x0000** */
	/** @ref UWBSTATUS_INVALID_PARAMETER */
	kUWBSTATUS_INVALID_PARAMETER = UWBSTATUS_INVALID_PARAMETER, /**< **0x0001** */
	/** @ref UWBSTATUS_BUFFER_TOO_SMALL */
	kUWBSTATUS_BUFFER_TOO_SMALL = UWBSTATUS_BUFFER_TOO_SMALL, /**< **0x0003** */
	/** @ref UWBSTATUS_INVALID_DEVICE */
	kUWBSTATUS_INVALID_DEVICE = UWBSTATUS_INVALID_DEVICE, /**< **0xFFFF** */
	/** @ref UWBSTATUS_PENDING */
	kUWBSTATUS_PENDING = UWBSTATUS_PENDING, /**< **0x000D** */
	/** @ref UWBSTATUS_BOARD_COMMUNICATION_ERROR */
	kUWBSTATUS_BOARD_COMMUNICATION_ERROR =
		UWBSTATUS_BOARD_COMMUNICATION_ERROR, /**< **0x000F** */
	/** @ref UWBSTATUS_INVALID_STATE */
	kUWBSTATUS_INVALID_STATE = UWBSTATUS_INVALID_STATE, /**< **0x0011** */
	/** @ref UWBSTATUS_ALREADY_INITIALISED */
	kUWBSTATUS_ALREADY_INITIALISED = UWBSTATUS_ALREADY_INITIALISED, /**< **0x0032** */
	/** @ref UWBSTATUS_BUSY */
	kUWBSTATUS_BUSY = UWBSTATUS_BUSY, /**< **0x006F** */
	/** @ref UWBSTATUS_READ_FAILED */
	kUWBSTATUS_READ_FAILED = UWBSTATUS_READ_FAILED, /**< **0x0014** */
	/** @ref UWBSTATUS_WRITE_FAILED */
	kUWBSTATUS_WRITE_FAILED = UWBSTATUS_WRITE_FAILED, /**< **0x0015** */
	/** @ref UWBSTATUS_RESPONSE_TIMEOUT */
	kUWBSTATUS_RESPONSE_TIMEOUT = UWBSTATUS_RESPONSE_TIMEOUT, /**< **0x0025** */
	/** @ref UWBSTATUS_UNKNOWN_ERROR */
	kUWBSTATUS_UNKNOWN_ERROR = UWBSTATUS_UNKNOWN_ERROR, /**< **0x00FE** */
	/** @ref UWBSTATUS_FAILED */
	kUWBSTATUS_FAILED = UWBSTATUS_FAILED, /**< **0x00FF** */
	/** @ref UWBSTATUS_CONNECTION_FAILED */
	kUWBSTATUS_CONNECTION_FAILED = UWBSTATUS_CONNECTION_FAILED, /**< **0x0047** */
	kUWBSTATUS_RETRY,                                           /**< **0x004A** */
} UWBStatus_t;

/** @} */

#endif /* __UWB_TML_STATUS_H__ */

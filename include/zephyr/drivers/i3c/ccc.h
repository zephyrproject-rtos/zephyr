/*
 * Copyright 2022 Intel Corporation
 * Copyright 2023 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_CCC_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_CCC_H_

/**
 * @brief I3C Common Command Codes
 * @defgroup i3c_ccc I3C Common Command Codes
 * @ingroup i3c_interface
 * @{
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum CCC ID for broadcast */
#define I3C_CCC_BROADCAST_MAX_ID		0x7FU

/**
 * Enable Events Command
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENEC(broadcast)			((broadcast) ? 0x00U : 0x80U)

/**
 * Disable Events Command
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_DISEC(broadcast)		((broadcast) ? 0x01U : 0x81U)

/**
 * Enter Activity State
 *
 * @param as Desired activity state
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENTAS(as, broadcast)		(((broadcast) ? 0x02U : 0x82U) + (as))

/**
 * Enter Activity State 0
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENTAS0(broadcast)		I3C_CCC_ENTAS(0, broadcast)

/**
 * Enter Activity State 1
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENTAS1(broadcast)		I3C_CCC_ENTAS(1, broadcast)

/**
 * Enter Activity State 2
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENTAS2(broadcast)		I3C_CCC_ENTAS(2, broadcast)

/**
 * Enter Activity State 3
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENTAS3(broadcast)		I3C_CCC_ENTAS(3, broadcast)

/** Reset Dynamic Address Assignment (Broadcast) */
#define I3C_CCC_RSTDAA				0x06U

/** Enter Dynamic Address Assignment (Broadcast) */
#define I3C_CCC_ENTDAA				0x07U

/** Define List of Targets (Broadcast) */
#define I3C_CCC_DEFTGTS				0x08U

/**
 * Set Max Write Length (Broadcast or Direct)
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_SETMWL(broadcast)		((broadcast) ? 0x09U : 0x89U)

/**
 * Set Max Read Length (Broadcast or Direct)
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_SETMRL(broadcast)		((broadcast) ? 0x0AU : 0x8AU)

/** Enter Test Mode (Broadcast) */
#define I3C_CCC_ENTTM				0x0BU

/** Set Bus Context (Broadcast) */
#define I3C_CCC_SETBUSCON			0x0CU

/**
 * Data Transfer Ending Procedure Control
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_ENDXFER(broadcast)		((broadcast) ? 0x12U : 0x92U)

/** Enter HDR Mode (HDR-DDR) (Broadcast) */
#define I3C_CCC_ENTHDR(x)			(0x20U + (x))

/** Enter HDR Mode 0 (HDR-DDR) (Broadcast) */
#define I3C_CCC_ENTHDR0				0x20U

/** Enter HDR Mode 1 (HDR-TSP) (Broadcast) */
#define I3C_CCC_ENTHDR1				0x21U

/** Enter HDR Mode 2 (HDR-TSL) (Broadcast) */
#define I3C_CCC_ENTHDR2				0x22U

/** Enter HDR Mode 3 (HDR-BT) (Broadcast) */
#define I3C_CCC_ENTHDR3				0x23U

/** Enter HDR Mode 4 (Broadcast) */
#define I3C_CCC_ENTHDR4				0x24U

/** Enter HDR Mode 5 (Broadcast) */
#define I3C_CCC_ENTHDR5				0x25U

/** Enter HDR Mode 6 (Broadcast) */
#define I3C_CCC_ENTHDR6				0x26U

/** Enter HDR Mode 7 (Broadcast) */
#define I3C_CCC_ENTHDR7				0x27U

/**
 * Exchange Timing Information (Broadcast or Direct)
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_SETXTIME(broadcast)		((broadcast) ? 0x28U : 0x98U)

/** Set All Addresses to Static Addresses (Broadcast) */
#define I3C_CCC_SETAASA				0x29U

/**
 * Target Reset Action
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_RSTACT(broadcast)		((broadcast) ? 0x2AU : 0x9AU)

/** Define List of Group Address (Broadcast) */
#define I3C_CCC_DEFGRPA				0x2BU

/**
 * Reset Group Address
 *
 * @param broadcast True if broadcast, false if direct.
 */
#define I3C_CCC_RSTGRPA(broadcast)		((broadcast) ? 0x2CU : 0x9CU)

/** Multi-Lane Data Transfer Control (Broadcast) */
#define I3C_CCC_MLANE(broadcast)		((broadcast) ? 0x2DU : 0x9DU)

/**
 * Vendor/Standard Extension
 *
 * @param broadcast True if broadcast, false if direct.
 * @param id Extension ID.
 */
#define I3C_CCC_VENDOR(broadcast, id)		((id) + ((broadcast) ? 0x61U : 0xE0U))

/** Set Dynamic Address from Static Address (Direct) */
#define I3C_CCC_SETDASA				0x87U

/** Set New Dynamic Address (Direct) */
#define I3C_CCC_SETNEWDA			0x88U

/** Get Max Write Length (Direct) */
#define I3C_CCC_GETMWL				0x8BU

/** Get Max Read Length (Direct) */
#define I3C_CCC_GETMRL				0x8CU

/** Get Provisioned ID (Direct) */
#define I3C_CCC_GETPID				0x8DU

/** Get Bus Characteristics Register (Direct) */
#define I3C_CCC_GETBCR				0x8EU

/** Get Device Characteristics Register (Direct) */
#define I3C_CCC_GETDCR				0x8FU

/** Get Device Status (Direct) */
#define I3C_CCC_GETSTATUS			0x90U

/** Get Accept Controller Role (Direct) */
#define I3C_CCC_GETACCCR			0x91U

/** Set Bridge Targets (Direct) */
#define I3C_CCC_SETBRGTGT			0x93U

/** Get Max Data Speed (Direct) */
#define I3C_CCC_GETMXDS				0x94U

/** Get Optional Feature Capabilities (Direct) */
#define I3C_CCC_GETCAPS				0x95U

/** Set Route (Direct) */
#define I3C_CCC_SETROUTE			0x96U

/** Device to Device(s) Tunneling Control (Direct) */
#define I3C_CCC_D2DXFER				0x97U

/** Get Exchange Timing Information (Direct) */
#define I3C_CCC_GETXTIME			0x99U

/** Set Group Address (Direct) */
#define I3C_CCC_SETGRPA				0x9BU

struct i3c_device_desc;

/**
 * @brief Payload structure for Direct CCC to one target.
 */
struct i3c_ccc_target_payload {
	/** Target address */
	uint8_t addr;

	/** @c 0 for Write, @c 1 for Read */
	uint8_t rnw:1;

	/**
	 * - For Write CCC, pointer to the byte array of data
	 *   to be sent, which may contain the Sub-Command Byte
	 *   and additional data.
	 * - For Read CCC, pointer to the byte buffer for data
	 *   to be read into.
	 */
	uint8_t *data;

	/** Length in bytes for @p data. */
	size_t data_len;

	/**
	 * Total number of bytes transferred
	 *
	 * A Target can issue an EoD or the Controller can abort a transfer
	 * before the length of the buffer. It is expected for the driver to
	 * write to this after the transfer.
	 */
	size_t num_xfer;

	/**
	 * SDR Error Type
	 *
	 * Error from I3C Specification v1.1.1 section 5.1.10.2. It is expected
	 * for the driver to write to this.
	 */
	enum i3c_sdr_controller_error_types err;
};

/**
 * @brief Payload structure for one CCC transaction.
 */
struct i3c_ccc_payload {
	struct {
		/**
		 * The CCC ID (@c I3C_CCC_*).
		 */
		uint8_t id;

		/**
		 * Pointer to byte array of data for this CCC.
		 *
		 * This is the bytes following the CCC command in CCC frame.
		 * Set to @c NULL if no associated data.
		 */
		uint8_t *data;

		/** Length in bytes for optional data array. */
		size_t data_len;

		/**
		 * Total number of bytes transferred
		 *
		 * A Controller can abort a transfer before the length of the buffer.
		 * It is expected for the driver to write to this after the transfer.
		 */
		size_t num_xfer;

		/**
		 * SDR Error Type
		 *
		 * Error from I3C Specification v1.1.1 section 5.1.10.2. It is expected
		 * for the driver to write to this.
		 */
		enum i3c_sdr_controller_error_types err;
	} ccc;

	struct {
		/**
		 * Array of struct i3c_ccc_target_payload.
		 *
		 * Each element describes the target and associated
		 * payloads for this CCC.
		 *
		 * Use with Direct CCC.
		 */
		struct i3c_ccc_target_payload *payloads;

		/** Number of targets */
		size_t num_targets;
	} targets;
};

/**
 * @brief Payload for ENEC/DISEC CCC (Target Events Command).
 */
struct i3c_ccc_events {
	/**
	 * Event byte:
	 * - Bit[0]: ENINT/DISINT:
	 *   - Target Interrupt Requests
	 * - Bit[1]: ENCR/DISCR:
	 *   - Controller Role Requests
	 * - Bit[3]: ENHJ/DISHJ:
	 *   - Hot-Join Event
	 */
	uint8_t events;
} __packed;

/** Enable Events (ENEC) - Target Interrupt Requests. */
#define I3C_CCC_ENEC_EVT_ENINTR		BIT(0)

/** Enable Events (ENEC) - Controller Role Requests. */
#define I3C_CCC_ENEC_EVT_ENCR		BIT(1)

/** Enable Events (ENEC) - Hot-Join Event. */
#define I3C_CCC_ENEC_EVT_ENHJ		BIT(3)

#define I3C_CCC_ENEC_EVT_ALL		\
	(I3C_CCC_ENEC_EVT_ENINTR | I3C_CCC_ENEC_EVT_ENCR | I3C_CCC_ENEC_EVT_ENHJ)

/** Disable Events (DISEC) - Target Interrupt Requests. */
#define I3C_CCC_DISEC_EVT_DISINTR	BIT(0)

/** Disable Events (DISEC) - Controller Role Requests. */
#define I3C_CCC_DISEC_EVT_DISCR		BIT(1)

/** Disable Events (DISEC) - Hot-Join Event. */
#define I3C_CCC_DISEC_EVT_DISHJ		BIT(3)

#define I3C_CCC_DISEC_EVT_ALL		\
	(I3C_CCC_DISEC_EVT_DISINTR | I3C_CCC_DISEC_EVT_DISCR | I3C_CCC_DISEC_EVT_DISHJ)

/*
 * Events for both enabling and disabling since
 * they have the same bits.
 */

/** Events - Target Interrupt Requests. */
#define I3C_CCC_EVT_INTR		BIT(0)

/** Events - Controller Role Requests. */
#define I3C_CCC_EVT_CR			BIT(1)

/** Events - Hot-Join Event. */
#define I3C_CCC_EVT_HJ			BIT(3)

/** Bitmask for all events. */
#define I3C_CCC_EVT_ALL			\
	(I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR | I3C_CCC_EVT_HJ)

/**
 * @brief Payload for SETMWL/GETMWL CCC (Set/Get Maximum Write Length).
 *
 * @note For drivers and help functions, the raw data coming
 * back from target device is in big endian. This needs to be
 * translated back to CPU endianness before passing back to
 * function caller.
 */
struct i3c_ccc_mwl {
	/** Maximum Write Length */
	uint16_t len;
} __packed;

/**
 * @brief Payload for SETMRL/GETMRL CCC (Set/Get Maximum Read Length).
 *
 * @note For drivers and help functions, the raw data coming
 * back from target device is in big endian. This needs to be
 * translated back to CPU endianness before passing back to
 * function caller.
 */
struct i3c_ccc_mrl {
	/** Maximum Read Length */
	uint16_t len;

	/** Optional IBI Payload Size */
	uint8_t ibi_len;
} __packed;

/**
 * @brief The active controller part of payload for DEFTGTS CCC.
 *
 * This is used by DEFTGTS (Define List of Targets) CCC to describe
 * the active controller on the I3C bus.
 */
struct i3c_ccc_deftgts_active_controller {
	/** Dynamic Address of Active Controller */
	uint8_t addr;

	/** Device Characteristic Register of Active Controller */
	uint8_t dcr;

	/** Bus Characteristic Register of Active Controller */
	uint8_t bcr;

	/** Static Address of Active Controller */
	uint8_t static_addr;
};

/**
 * @brief The target device part of payload for DEFTGTS CCC.
 *
 * This is used by DEFTGTS (Define List of Targets) CCC to describe
 * the existing target devices on the I3C bus.
 */
struct i3c_ccc_deftgts_target {
	/** Dynamic Address of a target device, or a group address */
	uint8_t addr;

	union {
		/**
		 * Device Characteristic Register of a I3C target device
		 * or a group.
		 */
		uint8_t dcr;

		/** Legacy Virtual Register for legacy I2C device. */
		uint8_t lvr;
	};

	/** Bus Characteristic Register of a target device or a group */
	uint8_t bcr;

	/** Static Address of a target device or a group */
	uint8_t static_addr;
};

/**
 * @brief Payload for DEFTGTS CCC (Define List of Targets).
 *
 * @note @p i3c_ccc_deftgts_target is an array of targets, where
 * the number of elements is dependent on the number of I3C targets
 * on the bus. Please have enough space for both read and write of
 * this CCC.
 */
struct i3c_ccc_deftgts {
	/** Number of Targets (and Groups) present on the I3C Bus */
	uint8_t count;

	/** Data describing the active controller */
	struct i3c_ccc_deftgts_active_controller active_controller;

	/** Data describing the target(s) on the bus */
	struct i3c_ccc_deftgts_target targets[];
} __packed;

/**
 * @brief Defining byte values for ENTTM.
 */
enum i3c_ccc_enttm_defbyte {
	/** Remove all I3C Devices from Test Mode */
	ENTTM_EXIT_TEST_MODE = 0x00U,

	/** Indicates that I3C Devices shall return a random 32-bit value
	 * in the PID during the Dynamic Address Assignment procedure
	 */
	ENTTM_VENDOR_TEST_MODE = 0x01U,
};

/**
 * @brief Payload for a single device address.
 *
 * This is used for:
 * - SETDASA (Set Dynamic Address from Static Address)
 * - SETNEWDA (Set New Dynamic Address)
 * - SETGRPA (Set Group Address)
 * - GETACCCR (Get Accept Controller Role)
 *
 * Note that the target address is encoded within
 * struct i3c_ccc_target_payload instead of being encoded in
 * this payload.
 */
struct i3c_ccc_address {
	/**
	 * - For SETDASA, Static Address to be assigned as
	 *   Dynamic Address.
	 * - For SETNEWDA, new Dynamic Address to be assigned.
	 * - For SETGRPA, new Group Address to be set.
	 * - For GETACCCR, the correct address of Secondary
	 *   Controller.
	 *
	 * @note For SETDATA, SETNEWDA and SETGRPA,
	 * the address is left-shift by 1, and bit[0] is always 0.
	 *
	 * @note Fpr SET GETACCCR, the address is left-shift by 1,
	 * and bit[0] is the calculated odd parity bit.
	 */
	uint8_t addr;
} __packed;

/**
 * @brief Payload for GETPID CCC (Get Provisioned ID).
 */
struct i3c_ccc_getpid {
	/**
	 * 48-bit Provisioned ID.
	 *
	 * @note Data is big-endian where first byte is MSB.
	 */
	uint8_t pid[6];
} __packed;

/**
 * @brief Payload for GETBCR CCC (Get Bus Characteristics Register).
 */
struct i3c_ccc_getbcr {
	/** Bus Characteristics Register */
	uint8_t bcr;
} __packed;

/**
 * @brief Payload for GETDCR CCC (Get Device Characteristics Register).
 */
struct i3c_ccc_getdcr {
	/** Device Characteristics Register */
	uint8_t dcr;
} __packed;


/**
 * @brief Indicate which format of GETSTATUS to use.
 */
enum i3c_ccc_getstatus_fmt {
	/** GETSTATUS Format 1 */
	GETSTATUS_FORMAT_1,

	/** GETSTATUS Format 2 */
	GETSTATUS_FORMAT_2,
};

/**
 * @brief Defining byte values for GETSTATUS Format 2.
 */
enum i3c_ccc_getstatus_defbyte {
	/** Target status. */
	GETSTATUS_FORMAT_2_TGTSTAT = 0x00U,

	/** PRECR - Alternate status format describing Controller-capable device. */
	GETSTATUS_FORMAT_2_PRECR = 0x91U,

	/** Invalid defining byte. */
	GETSTATUS_FORMAT_2_INVALID = 0x100U
};

/**
 * @brief Payload for GETSTATUS CCC (Get Device Status).
 */
union i3c_ccc_getstatus {
	struct {
		/**
		 * Device Status
		 * - Bit[15:8]: Reserved.
		 * - Bit[7:6]: Activity Mode.
		 * - Bit[5]: Protocol Error.
		 * - Bit[4]: Reserved.
		 * - Bit[3:0]: Number of Pending Interrupts.
		 *
		 * @note For drivers and help functions, the raw data coming
		 * back from target device is in big endian. This needs to be
		 * translated back to CPU endianness before passing back to
		 * function caller.
		 */
		uint16_t status;
	} fmt1;

	union {
		/**
		 * Defining Byte 0x00: TGTSTAT
		 *
		 * @see i3c_ccc_getstatus::fmt1::status
		 */
		uint16_t tgtstat;

		/**
		 * Defining Byte 0x91: PRECR
		 * - Bit[15:8]: Vendor Reserved
		 * - Bit[7:2]: Reserved
		 * - Bit[1]: Handoff Delay NACK
		 * - Bit[0]: Deep Sleep Detected
		 *
		 * @note For drivers and help functions, the raw data coming
		 * back from target device is in big endian. This needs to be
		 * translated back to CPU endianness before passing back to
		 * function caller.
		 */
		uint16_t precr;

		uint16_t raw_u16;
	} fmt2;
} __packed;

/** GETSTATUS Format 1 - Protocol Error bit. */
#define I3C_CCC_GETSTATUS_PROTOCOL_ERR				BIT(5)

/** GETSTATUS Format 1 - Activity Mode bitmask. */
#define I3C_CCC_GETSTATUS_ACTIVITY_MODE_MASK			GENMASK(7U, 6U)

/**
 * @brief GETSTATUS Format 1 - Activity Mode
 *
 * Obtain Activity Mode from GETSTATUS Format 1 value obtained via
 * GETSTATUS.
 *
 * @param status GETSTATUS Format 1 value
 */
#define I3C_CCC_GETSTATUS_ACTIVITY_MODE(status)			\
	FIELD_GET(I3C_CCC_GETSTATUS_ACTIVITY_MODE_MASK, (status))

/** GETSTATUS Format 1 - Number of Pending Interrupts bitmask. */
#define I3C_CCC_GETSTATUS_NUM_INT_MASK				GENMASK(3U, 0U)

/**
 * @brief GETSTATUS Format 1 - Number of Pending Interrupts
 *
 * Obtain Number of Pending Interrupts from GETSTATUS Format 1 value
 * obtained via GETSTATUS.
 *
 * @param status GETSTATUS Format 1 value
 */
#define I3C_CCC_GETSTATUS_NUM_INT(status)			\
	FIELD_GET(I3C_CCC_GETSTATUS_NUM_INT_MASK, (status))

/** GETSTATUS Format 2 - PERCR - Deep Sleep Detected bit. */
#define I3C_CCC_GETSTATUS_PRECR_DEEP_SLEEP_DETECTED		BIT(0)

/** GETSTATUS Format 2 - PERCR - Handoff Delay NACK. */
#define I3C_CCC_GETSTATUS_PRECR_HANDOFF_DELAY_NACK		BIT(1)

/**
 * @brief One Bridged Target for SETBRGTGT payload.
 */
struct i3c_ccc_setbrgtgt_tgt {
	/**
	 * Dynamic address of the bridged target.
	 *
	 * @note The address is left-shift by 1, and bit[0]
	 * is always 0.
	 */
	uint8_t addr;

	/**
	 * 16-bit ID for the bridged target.
	 *
	 * @note For drivers and help functions, the raw data coming
	 * back from target device is in big endian. This needs to be
	 * translated back to CPU endianness before passing back to
	 * function caller.
	 */
	uint16_t id;
} __packed;

/**
 * @brief Payload for SETBRGTGT CCC (Set Bridge Targets).
 *
 * Note that the bridge target address is encoded within
 * struct i3c_ccc_target_payload instead of being encoded in
 * this payload.
 */
struct i3c_ccc_setbrgtgt {
	/** Number of bridged targets */
	uint8_t count;

	/** Array of bridged targets */
	struct i3c_ccc_setbrgtgt_tgt targets[];
} __packed;

/**
 * @brief Indicate which format of getmxds to use.
 */
enum i3c_ccc_getmxds_fmt {
	/** GETMXDS Format 1 */
	GETMXDS_FORMAT_1,

	/** GETMXDS Format 2 */
	GETMXDS_FORMAT_2,

	/** GETMXDS Format 3 */
	GETMXDS_FORMAT_3,
};

/**
 * @brief Enum for I3C Get Max Data Speed (GETMXDS) Format 3 Defining Byte Values.
 */
enum i3c_ccc_getmxds_defbyte {
	/** Standard Target Write/Read speed parameters, and optional Maximum Read Turnaround Time
	 */
	GETMXDS_FORMAT_3_WRRDTURN = 0x00U,

	/** Delay parameters for a Controller-capable Device, and it's expected Activity State
	 * during a Controller Handoff
	 */
	GETMXDS_FORMAT_3_CRHDLY = 0x91U,

	/** Invalid defining byte. */
	GETMXDS_FORMAT_3_INVALID = 0x100,
};


/**
 * @brief Payload for GETMXDS CCC (Get Max Data Speed).
 */
union i3c_ccc_getmxds {
	struct {
		/** maxWr */
		uint8_t maxwr;

		/** maxRd */
		uint8_t maxrd;
	} fmt1;

	struct {
		/** maxWr */
		uint8_t maxwr;

		/** maxRd */
		uint8_t maxrd;

		/**
		 * Maximum Read Turnaround Time in microsecond.
		 *
		 * This is in little-endian where first byte is LSB.
		 */
		uint8_t maxrdturn[3];
	} fmt2;

	struct {
		/**
		 * Defining Byte 0x00: WRRDTURN
		 *
		 * @see i3c_ccc_getmxds::fmt2
		 */
		uint8_t wrrdturn[5];

		/**
		 * Defining Byte 0x91: CRHDLY
		 * - Bit[2]: Set Bus Activity State
		 * - Bit[1:0]: Controller Handoff Activity State
		 */
		uint8_t crhdly1;
	} fmt3;
} __packed;

/** Get Max Data Speed (GETMXDS) - Default Max Sustained Data Rate. */
#define I3C_CCC_GETMXDS_MAX_SDR_FSCL_MAX			0

/** Get Max Data Speed (GETMXDS) - 8MHz Max Sustained Data Rate. */
#define I3C_CCC_GETMXDS_MAX_SDR_FSCL_8MHZ			1

/** Get Max Data Speed (GETMXDS) - 6MHz Max Sustained Data Rate. */
#define I3C_CCC_GETMXDS_MAX_SDR_FSCL_6MHZ			2

/** Get Max Data Speed (GETMXDS) - 4MHz Max Sustained Data Rate. */
#define I3C_CCC_GETMXDS_MAX_SDR_FSCL_4MHZ			3

/** Get Max Data Speed (GETMXDS) - 2MHz Max Sustained Data Rate. */
#define I3C_CCC_GETMXDS_MAX_SDR_FSCL_2MHZ			4

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround <= 8ns. */
#define I3C_CCC_GETMXDS_TSCO_8NS				0

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround <= 9ns. */
#define I3C_CCC_GETMXDS_TSCO_9NS				1

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround <= 10ns. */
#define I3C_CCC_GETMXDS_TSCO_10NS				2

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround <= 11ns. */
#define I3C_CCC_GETMXDS_TSCO_11NS				3

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround <= 12ns. */
#define I3C_CCC_GETMXDS_TSCO_12NS				4

/** Get Max Data Speed (GETMXDS) - Clock to Data Turnaround > 12ns. */
#define I3C_CCC_GETMXDS_TSCO_GT_12NS				7

/** Get Max Data Speed (GETMXDS) - maxWr - Optional Defining Byte Support. */
#define I3C_CCC_GETMXDS_MAXWR_DEFINING_BYTE_SUPPORT		BIT(3)

/** Get Max Data Speed (GETMXDS) - Max Sustained Data Rate bitmask. */
#define I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL_MASK			GENMASK(2U, 0U)

/**
 * @brief Get Max Data Speed (GETMXDS) - maxWr - Max Sustained Data Rate
 *
 * Obtain Max Sustained Data Rate value from GETMXDS maxWr value
 * obtained via GETMXDS.
 *
 * @param maxwr GETMXDS maxWr value.
 */
#define I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL(maxwr)		\
	FIELD_GET(I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL_MASK, (maxwr))

/** Get Max Data Speed (GETMXDS) - maxRd - Write-to-Read Permits Stop Between. */
#define I3C_CCC_GETMXDS_MAXRD_W2R_PERMITS_STOP_BETWEEN		BIT(6)

/** Get Max Data Speed (GETMXDS) - maxRd - Clock to Data Turnaround bitmask. */
#define I3C_CCC_GETMXDS_MAXRD_TSCO_MASK				GENMASK(5U, 3U)

/**
 * @brief Get Max Data Speed (GETMXDS) - maxRd - Clock to Data Turnaround
 *
 * Obtain Clock to Data Turnaround value from GETMXDS maxRd value
 * obtained via GETMXDS.
 *
 * @param maxrd GETMXDS maxRd value.
 */
#define I3C_CCC_GETMXDS_MAXRD_TSCO(maxrd)			\
	FIELD_GET(I3C_CCC_GETMXDS_MAXRD_TSCO_MASK, (maxrd))

/** Get Max Data Speed (GETMXDS) - maxRd - Max Sustained Data Rate bitmask. */
#define I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL_MASK			GENMASK(2U, 0U)

/**
 * @brief Get Max Data Speed (GETMXDS) - maxRd - Max Sustained Data Rate
 *
 * Obtain Max Sustained Data Rate value from GETMXDS maxRd value
 * obtained via GETMXDS.
 *
 * @param maxrd GETMXDS maxRd value.
 */
#define I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL(maxrd)		\
	FIELD_GET(I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL_MASK, (maxrd))

/** Get Max Data Speed (GETMXDS) - CRDHLY1 - Set Bus Activity State bit shift value. */
#define I3C_CCC_GETMXDS_CRDHLY1_SET_BUS_ACT_STATE		BIT(2)

/** Get Max Data Speed (GETMXDS) - CRDHLY1 - Controller Handoff Activity State bitmask. */
#define I3C_CCC_GETMXDS_CRDHLY1_CTRL_HANDOFF_ACT_STATE_MASK	GENMASK(1U, 0U)

/**
 * @brief Get Max Data Speed (GETMXDS) - CRDHLY1 - Controller Handoff Activity State
 *
 * Obtain Controller Handoff Activity State value from GETMXDS value
 * obtained via GETMXDS.
 *
 * @param crhdly1 GETMXDS value.
 */
#define I3C_CCC_GETMXDS_CRDHLY1_CTRL_HANDOFF_ACT_STATE(crhdly1)	\
	FIELD_GET(I3C_CCC_GETMXDS_CRDHLY1_CTRL_HANDOFF_ACT_STATE_MASK, (chrdly1))

/**
 * @brief Indicate which format of GETCAPS to use.
 */
enum i3c_ccc_getcaps_fmt {
	/** GETCAPS Format 1 */
	GETCAPS_FORMAT_1,

	/** GETCAPS Format 2 */
	GETCAPS_FORMAT_2,
};

/**
 * @brief Enum for I3C Get Capabilities (GETCAPS) Format 2 Defining Byte Values.
 */
enum i3c_ccc_getcaps_defbyte {
	/** Standard Target capabilities and features. */
	GETCAPS_FORMAT_2_TGTCAPS = 0x00U,

	/** Fixed 32b test pattern. */
	GETCAPS_FORMAT_2_TESTPAT = 0x5AU,

	/** Controller handoff capabilities and features. */
	GETCAPS_FORMAT_2_CRCAPS = 0x91U,

	/** Virtual Target capabilities and features. */
	GETCAPS_FORMAT_2_VTCAPS = 0x93U,

	/** Debug-capable Device capabilities and features. */
	GETCAPS_FORMAT_2_DBGCAPS = 0xD7U,

	/** Invalid defining byte. */
	GETCAPS_FORMAT_2_INVALID = 0x100,
};

/**
 * @brief Payload for GETCAPS CCC (Get Optional Feature Capabilities).
 *
 * @note Only supports GETCAPS Format 1 and Format 2. In I3C v1.0 this was
 * GETHDRCAP which only returned a single byte which is the same as the
 * GETCAPS1 byte.
 */
union i3c_ccc_getcaps {
	union {
		/**
		 * I3C v1.0 HDR Capabilities
		 * - Bit[0]: HDR-DDR
		 * - Bit[1]: HDR-TSP
		 * - Bit[2]: HDR-TSL
		 * - Bit[7:3]: Reserved
		 */
		uint8_t gethdrcap;

		/**
		 * I3C v1.1+ Device Capabilities
		 * Byte 1 GETCAPS1
		 * - Bit[0]: HDR-DDR
		 * - Bit[1]: HDR-TSP
		 * - Bit[2]: HDR-TSL
		 * - Bit[3]: HDR-BT
		 * - Bit[7:4]: Reserved
		 * Byte 2 GETCAPS2
		 * - Bit[3:0]: I3C 1.x Specification Version
		 * - Bit[5:4]: Group Address Capabilities
		 * - Bit[6]: HDR-DDR Write Abort
		 * - Bit[7]: HDR-DDR Abort CRC
		 * Byte 3 GETCAPS3
		 * - Bit[0]: Multi-Lane (ML) Data Transfer Support
		 * - Bit[1]: Device to Device Transfer (D2DXFER) Support
		 * - Bit[2]: Device to Device Transfer (D2DXFER) IBI Capable
		 * - Bit[3]: Defining Byte Support in GETCAPS
		 * - Bit[4]: Defining Byte Support in GETSTATUS
		 * - Bit[5]: HDR-BT CRC-32 Support
		 * - Bit[6]: IBI MDB Support for Pending Read Notification
		 * - Bit[7]: Reserved
		 * Byte 4 GETCAPS4
		 * - Bit[7:0]: Reserved
		 */
		uint8_t getcaps[4];
	} fmt1;

	union {
		/**
		 * Defining Byte 0x00: TGTCAPS
		 *
		 * @see i3c_ccc_getcaps::fmt1::getcaps
		 */
		uint8_t tgtcaps[4];

		/**
		 * Defining Byte 0x5A: TESTPAT
		 *
		 * @note should always be 0xA55AA55A in big endian
		 */
		uint32_t testpat;

		/**
		 * Defining Byte 0x91: CRCAPS
		 * Byte 1 CRCAPS1
		 * - Bit[0]: Hot-Join Support
		 * - Bit[1]: Group Management Support
		 * - Bit[2]: Multi-Lane Support
		 * Byte 2 CRCAPS2
		 * - Bit[0]: In-Band Interrupt Support
		 * - Bit[1]: Controller Pass-Back
		 * - Bit[2]: Deep Sleep Capable
		 * - Bit[3]: Delayed Controller Handoff
		 */
		uint8_t crcaps[2];

		/**
		 * Defining Byte 0x93: VTCAPS
		 * Byte 1 VTCAPS1
		 * - Bit[2:0]: Virtual Target Type
		 * - Bit[4]: Side Effects
		 * - Bit[5]: Shared Peripheral Detect
		 * Byte 2 VTCAPS2
		 * - Bit[1:0]: Interrupt Requests
		 * - Bit[2]: Address Remapping
		 * - Bit[4:3]: Bus Context and Conditions
		 */
		uint8_t vtcaps[2];
	} fmt2;
} __packed;

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR-DDR mode bit. */
#define I3C_CCC_GETCAPS1_HDR_DDR				BIT(0)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR-TSP mode bit. */
#define I3C_CCC_GETCAPS1_HDR_TSP				BIT(1)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR-TSL mode bit. */
#define I3C_CCC_GETCAPS1_HDR_TSL				BIT(2)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR-BT mode bit. */
#define I3C_CCC_GETCAPS1_HDR_BT					BIT(3)

/**
 * @brief Get Optional Feature Capabilities Byte 1 (GETCAPS) - HDR Mode
 *
 * Get the bit corresponding to HDR mode.
 *
 * @param x HDR mode
 */
#define I3C_CCC_GETCAPS1_HDR_MODE(x)				BIT(x)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 0. */
#define I3C_CCC_GETCAPS1_HDR_MODE0				BIT(0)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 1. */
#define I3C_CCC_GETCAPS1_HDR_MODE1				BIT(1)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 2. */
#define I3C_CCC_GETCAPS1_HDR_MODE2				BIT(2)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 3. */
#define I3C_CCC_GETCAPS1_HDR_MODE3				BIT(3)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 4. */
#define I3C_CCC_GETCAPS1_HDR_MODE4				BIT(4)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 5. */
#define I3C_CCC_GETCAPS1_HDR_MODE5				BIT(5)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 6. */
#define I3C_CCC_GETCAPS1_HDR_MODE6				BIT(6)

/** Get Optional Feature Capabilities Byte 1 (GETCAPS) Format 1 - HDR Mode 7. */
#define I3C_CCC_GETCAPS1_HDR_MODE7				BIT(7)

/** Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 - HDR-DDR Write Abort bit. */
#define I3C_CCC_GETCAPS2_HDRDDR_WRITE_ABORT			BIT(6)

/** Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 - HDR-DDR Abort CRC bit. */
#define I3C_CCC_GETCAPS2_HDRDDR_ABORT_CRC			BIT(7)

/**
 * @brief Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 -
 *        Group Address Capabilities bitmask.
 */
#define I3C_CCC_GETCAPS2_GRPADDR_CAP_MASK			GENMASK(5U, 4U)

/**
 * @brief Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 - Group Address Capabilities.
 *
 * Obtain Group Address Capabilities value from GETCAPS Format 1 value
 * obtained via GETCAPS.
 *
 * @param getcaps2 GETCAPS2 value.
 */
#define I3C_CCC_GETCAPS2_GRPADDR_CAP(getcaps2)			\
	FIELD_GET(I3C_CCC_GETCAPS2_GRPADDR_CAP_MASK, (getcaps2))

/**
 * @brief Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 -
 *        I3C 1.x Specification Version bitmask.
 */
#define I3C_CCC_GETCAPS2_SPEC_VER_MASK				GENMASK(3U, 0U)

/**
 * @brief Get Optional Feature Capabilities Byte 2 (GETCAPS) Format 1 -
 *        I3C 1.x Specification Version.
 *
 * Obtain I3C 1.x Specification Version value from GETCAPS Format 1 value
 * obtained via GETCAPS.
 *
 * @param getcaps2 GETCAPS2 value.
 */
#define I3C_CCC_GETCAPS2_SPEC_VER(getcaps2)			\
	FIELD_GET(I3C_CCC_GETCAPS2_SPEC_VER_MASK, (getcaps2))

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        Multi-Lane Data Transfer Support bit.
 */
#define I3C_CCC_GETCAPS3_MLANE_SUPPORT				BIT(0)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        Device to Device Transfer (D2DXFER) Support bit.
 */
#define I3C_CCC_GETCAPS3_D2DXFER_SUPPORT			BIT(1)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        Device to Device Transfer (D2DXFER) IBI Capable bit.
 */
#define I3C_CCC_GETCAPS3_D2DXFER_IBI_CAPABLE			BIT(2)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        Defining Byte Support in GETCAPS bit.
 */
#define I3C_CCC_GETCAPS3_GETCAPS_DEFINING_BYTE_SUPPORT		BIT(3)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        Defining Byte Support in GETSTATUS bit.
 */
#define I3C_CCC_GETCAPS3_GETSTATUS_DEFINING_BYTE_SUPPORT	BIT(4)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        HDR-BT CRC-32 Support bit.
 */
#define I3C_CCC_GETCAPS3_HDRBT_CRC32_SUPPORT			BIT(5)

/**
 * @brief Get Optional Feature Capabilities Byte 3 (GETCAPS) Format 1 -
 *        IBI MDB Support for Pending Read Notification bit.
 */
#define I3C_CCC_GETCAPS3_IBI_MDR_PENDING_READ_NOTIFICATION	BIT(6)

/**
 * @brief Get Fixed Test Pattern (GETCAPS) Format 2 -
 *        Fixed Test Pattern Byte 1.
 */
#define I3C_CCC_GETCAPS_TESTPAT1				0xA5

/**
 * @brief Get Fixed Test Pattern (GETCAPS) Format 2 -
 *        Fixed Test Pattern Byte 2.
 */
#define I3C_CCC_GETCAPS_TESTPAT2				0x5A

/**
 * @brief Get Fixed Test Pattern (GETCAPS) Format 2 -
 *        Fixed Test Pattern Byte 3.
 */
#define I3C_CCC_GETCAPS_TESTPAT3				0xA5

/**
 * @brief Get Fixed Test Pattern (GETCAPS) Format 2 -
 *        Fixed Test Pattern Byte 4.
 */
#define I3C_CCC_GETCAPS_TESTPAT4				0x5A

/**
 * @brief Get Fixed Test Pattern (GETCAPS) Format 2 -
 *        Fixed Test Pattern Word in Big Endian.
 */
#define I3C_CCC_GETCAPS_TESTPAT					0xA55AA55A

/**
 * @brief Get Controller Handoff Capabilities Byte 1 (GETCAPS) Format 2 -
 *        Hot-Join Support.
 */
#define I3C_CCC_GETCAPS_CRCAPS1_HJ_SUPPORT			BIT(0)

/**
 * @brief Get Controller Handoff Capabilities Byte 1 (GETCAPS) Format 2 -
 *        Group Management Support.
 */
#define I3C_CCC_GETCAPS_CRCAPS1_GRP_MANAGEMENT_SUPPORT		BIT(1)

/**
 * @brief Get Controller Handoff Capabilities Byte 1 (GETCAPS) Format 2 -
 *        Multi-Lane Support.
 */
#define I3C_CCC_GETCAPS_CRCAPS1_ML_SUPPORT			BIT(2)

/**
 * @brief Get Controller Handoff Capabilities Byte 2 (GETCAPS) Format 2 -
 *        In-Band Interrupt Support.
 */
#define I3C_CCC_GETCAPS_CRCAPS2_IBI_TIR_SUPPORT			BIT(0)

/**
 * @brief Get Controller Handoff Capabilities Byte 2 (GETCAPS) Format 2 -
 *        Controller Pass-Back.
 */
#define I3C_CCC_GETCAPS_CRCAPS2_CONTROLLER_PASSBACK		BIT(1)

/**
 * @brief Get Controller Handoff Capabilities Byte 2 (GETCAPS) Format 2 -
 *        Deep Sleep Capable.
 */
#define I3C_CCC_GETCAPS_CRCAPS2_DEEP_SLEEP_CAPABLE		BIT(2)

/**
 * @brief Get Controller Handoff Capabilities Byte 2 (GETCAPS) Format 2 -
 *        Deep Sleep Capable.
 */
#define I3C_CCC_GETCAPS_CRCAPS2_DELAYED_CONTROLLER_HANDOFF	BIT(3)

/** Get Capabilities (GETCAPS) - VTCAP1 - Virtual Target Type bitmask. */
#define I3C_CCC_GETCAPS_VTCAP1_VITRUAL_TARGET_TYPE_MASK		GENMASK(2U, 0U)

/**
 * @brief Get Capabilities (GETCAPS) - VTCAP1 - Virtual Target Type
 *
 * Obtain Virtual Target Type value from VTCAP1 value
 * obtained via GETCAPS format 2 VTCAP def byte.
 *
 * @param vtcap1 VTCAP1 value.
 */
#define I3C_CCC_GETCAPS_VTCAP1_VITRUAL_TARGET_TYPE(vtcap1)	\
	FIELD_GET(I3C_CCC_GETCAPS_VTCAP1_VITRUAL_TARGET_TYPE_MASK, (vtcap1))

/**
 * @brief Get Virtual Target Capabilities Byte 1 (GETCAPS) Format 2 -
 *        Side Effects.
 */
#define I3C_CCC_GETCAPS_VTCAP1_SIDE_EFFECTS			BIT(4)

/**
 * @brief Get Virtual Target Capabilities Byte 1 (GETCAPS) Format 2 -
 *        Shared Peripheral Detect.
 */
#define I3C_CCC_GETCAPS_VTCAP1_SHARED_PERIPH_DETECT		BIT(5)

/** Get Capabilities (GETCAPS) - VTCAP2 - Interrupt Requests bitmask. */
#define I3C_CCC_GETCAPS_VTCAP2_INTERRUPT_REQUESTS_MASK		GENMASK(1U, 0U)

/**
 * @brief Get Capabilities (GETCAPS) - VTCAP2 - Interrupt Requests
 *
 * Obtain Interrupt Requests value from VTCAP2 value
 * obtained via GETCAPS format 2 VTCAP def byte.
 *
 * @param vtcap2 VTCAP2 value.
 */
#define I3C_CCC_GETCAPS_VTCAP2_INTERRUPT_REQUESTS(vtcap2)	\
	FIELD_GET(I3C_CCC_GETCAPS_VTCAP2_INTERRUPT_REQUESTS_MASK, (vtcap2))

/**
 * @brief Get Virtual Target Capabilities Byte 2 (GETCAPS) Format 2 -
 *        Address Remapping.
 */
#define I3C_CCC_GETCAPS_VTCAP2_ADDRESS_REMAPPING		BIT(2)

/** Get Capabilities (GETCAPS) - VTCAP2 - Bus Context and Condition bitmask. */
#define I3C_CCC_GETCAPS_VTCAP2_BUS_CONTEXT_AND_COND_MASK	GENMASK(4U, 3U)

/**
 * @brief Get Capabilities (GETCAPS) - VTCAP2 - Bus Context and Condition
 *
 * Obtain Bus Context and Condition value from VTCAP2 value
 * obtained via GETCAPS format 2 VTCAP def byte.
 *
 * @param vtcap2 VTCAP2 value.
 */
#define I3C_CCC_GETCAPS_VTCAP2_BUS_CONTEXT_AND_COND(vtcap2)	\
	FIELD_GET(I3C_CCC_GETCAPS_VTCAP2_BUS_CONTEXT_AND_COND_MASK, (vtcap2))

/**
 * @brief Enum for I3C Reset Action (RSTACT) Defining Byte Values.
 */
enum i3c_ccc_rstact_defining_byte {
	/** No Reset on Target Reset Pattern. */
	I3C_CCC_RSTACT_NO_RESET = 0x00U,

	/** Reset the I3C Peripheral Only. */
	I3C_CCC_RSTACT_PERIPHERAL_ONLY = 0x01U,

	/** Reset the Whole Target. */
	I3C_CCC_RSTACT_RESET_WHOLE_TARGET = 0x02U,

	/** Debug Network Adapter Reset. */
	I3C_CCC_RSTACT_DEBUG_NETWORK_ADAPTER = 0x03U,

	/** Virtual Target Detect. */
	I3C_CCC_RSTACT_VIRTUAL_TARGET_DETECT = 0x04U,

	/** Return Time to Reset Peripheral */
	I3C_CCC_RSTACT_RETURN_TIME_TO_RESET_PERIPHERAL = 0x81U,

	/** Return Time to Reset Whole Target */
	I3C_CCC_RSTACT_RETURN_TIME_TO_WHOLE_TARGET = 0x82U,

	/** Return Time for Debug Network Adapter Reset */
	I3C_CCC_RSTACT_RETURN_TIME_FOR_DEBUG_NETWORK_ADAPTER_RESET = 0x83U,

	/** Return Virtual Target Indication */
	I3C_CCC_RSTACT_RETURN_VIRTUAL_TARGET_INDICATION = 0x84U,
};

/**
 * @name Set Bus Context MIPI I3C Specification v1.Y Minor Version (SETBUSCON)
 * @anchor I3C_CCC_SETBUSCON_I3C_SPEC
 *
 * - CONTEXT[7:6]: 2'b00
 *
 * - CONTEXT[5]: I3C Specification Editorial Revision (within Minor Version)
 *   - 0: Version 1.Y.0
 *   - 1: Version 1.Y.1 or greater
 *
 * - CONTEXT[4]: I3C Specification Family
 *   - 0: MIPI I3C Specification
 *   - 1: MIPI I3C Basic Specification
 *
 * - CONTEXT[3:0]: I3C Specification Minor Version (v1.Y)
 *   - 0: Illegal, do not use (see Note below)
 *        (It would encode v1.0, but SETBUSCON was not available in I3C Basic v1.0)
 *   - 1-15: Version 1.1 - Version 1.15
 *
 * Examples:  Bit[5]  Bit[4]   Bits[3:0]
 *    I3C Basic v1.1.0:  1’b0 || 1’b1 || 4’b0001 or 8’b00010001
 *    I3C Basic v1.1.1:  1’b1 || 1’b1 || 4’b0001 or 8’b00110001
 *    I3C Basic v1.2.0:  1’b0 || 1’b1 || 4’b0010 or 8’b00010010
 *
 * @{
 */

/** I3C Specification Minor Version shift mask */
#define I3C_CCC_SETBUSCON_I3C_SPEC_MINOR_VER_MASK		GENMASK(3U, 0U)

/**
 * @brief I3C Specification Minor Version (v1.Y)
 *
 * Set the context bits for SETBUSCON
 *
 * @param y I3C Specification Minor Version Number
 */
#define I3C_CCC_SETBUSCON_I3C_SPEC_MINOR_VER(y)			\
	FIELD_PREP(I3C_CCC_SETBUSCON_I3C_SPEC_MINOR_VER_MASK, (y))

/** MIPI I3C Specification */
#define I3C_CCC_SETBUSCON_I3C_SPEC_I3C_SPEC			0

/** MIPI I3C Basic Specification */
#define I3C_CCC_SETBUSCON_I3C_SPEC_I3C_BASIC_SPEC		BIT(4)

/** Version 1.Y.0 */
#define I3C_CCC_SETBUSCON_I3C_SPEC_I3C_SPEC_EDITORIAL_1_Y_0	0

/** Version 1.Y.1 or greater */
#define I3C_CCC_SETBUSCON_I3C_SPEC_I3C_SPEC_EDITORIAL_1_Y_1	BIT(5)

/** @} */

/**
 * @name Set Bus Context Other Standards Organizations (SETBUSCON)
 * @anchor I3C_CCC_SETBUSCON_OTHER_STANDARDS
 *
 * @{
 */

/**
 * @brief JEDEC Sideband
 *
 * JEDEC SideBand Bus device, compliant to JESD403 Specification v1.0 or later.
 */
#define I3C_CCC_SETBUSCON_OTHER_STANDARDS_JEDEC_SIDEBAND	128

/**
 * @brief MCTP
 *
 * MCTP for system manageability (conforming to the content protocol defined in
 * the MCTP I3C Transport Binding Specification, released by DMTF, version 1.0
 * or newer)
 */
#define I3C_CCC_SETBUSCON_OTHER_STANDARDS_MCTP			129

/**
 * @brief ETSI
 *
 * ETSI for Secure Smart Platform Devices used for mobile networks authentication
 * and other ETSI security functions in mobile ecosystem
 */
#define I3C_CCC_SETBUSCON_OTHER_STANDARDS_ETSI			130

/** @} */

/**
 * @brief Test if I3C CCC payload is for broadcast.
 *
 * This tests if the CCC payload is for broadcast.
 *
 * @param[in] payload Pointer to the CCC payload.
 *
 * @retval true if payload target is broadcast
 * @retval false if payload target is direct
 */
static inline bool i3c_ccc_is_payload_broadcast(const struct i3c_ccc_payload *payload)
{
	return (payload->ccc.id <= I3C_CCC_BROADCAST_MAX_ID);
}

/**
 * @brief Get BCR from a target
 *
 * Helper function to get BCR (Bus Characteristic Register) from
 * target device.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] bcr Pointer to the BCR payload structure.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getbcr(struct i3c_device_desc *target,
		      struct i3c_ccc_getbcr *bcr);

/**
 * @brief Get DCR from a target
 *
 * Helper function to get DCR (Device Characteristic Register) from
 * target device.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] dcr Pointer to the DCR payload structure.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getdcr(struct i3c_device_desc *target,
		      struct i3c_ccc_getdcr *dcr);

/**
 * @brief Get PID from a target
 *
 * Helper function to get PID (Provisioned ID) from
 * target device.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] pid Pointer to the PID payload structure.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getpid(struct i3c_device_desc *target,
		      struct i3c_ccc_getpid *pid);

/**
 * @brief Broadcast RSTACT to reset I3C Peripheral (Format 1).
 *
 * Helper function to broadcast Target Reset Action (RSTACT) to
 * all connected targets.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] action What reset action to perform.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_rstact_all(const struct device *controller,
			  enum i3c_ccc_rstact_defining_byte action);

/**
 * @brief Single target RSTACT to reset I3C Peripheral.
 *
 * Helper function to do Target Reset Action (RSTACT) to
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] action What reset action to perform.
 * @param[in] get True if a get, False if set
 * @param[out] data Pointer to RSTACT payload received.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_rstact(const struct i3c_device_desc *target,
			  enum i3c_ccc_rstact_defining_byte action,
			  bool get,
			  uint8_t *data);

/**
 * @brief Single target RSTACT to reset I3C Peripheral (Format 2).
 *
 * Helper function to do Target Reset Action (RSTACT, format 2) to
 * one target. This is a Direct Write.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] action What reset action to perform.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_rstact_fmt2(const struct i3c_device_desc *target,
			  enum i3c_ccc_rstact_defining_byte action)
{
	return i3c_ccc_do_rstact(target, action, false, NULL);
}

/**
 * @brief Single target RSTACT to reset I3C Peripheral (Format 3).
 *
 * Helper function to do Target Reset Action (RSTACT, format 3) to
 * one target. This is a Direct Read.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] action What reset action to perform.
 * @param[out] data Pointer to RSTACT payload received.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_rstact_fmt3(const struct i3c_device_desc *target,
			  enum i3c_ccc_rstact_defining_byte action,
			  uint8_t *data)
{
	return i3c_ccc_do_rstact(target, action, true, data);
}

/**
 * @brief Broadcast RSTDAA to reset dynamic addresses for all targets.
 *
 * Helper function to reset dynamic addresses of all connected targets.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_rstdaa_all(const struct device *controller);

/**
 * @brief Set Dynamic Address from Static Address for a target
 *
 * Helper function to do SETDASA (Set Dynamic Address from Static Address)
 * for a particular target.
 *
 * Note this does not update @p target with the new dynamic address.
 *
 * @param[in] target Pointer to the target device descriptor where
 *                   the device is configured with a static address.
 * @param[in] da Struct of the Dynamic address
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setdasa(const struct i3c_device_desc *target,
			  struct i3c_ccc_address da);

/**
 * @brief Set New Dynamic Address for a target
 *
 * Helper function to do SETNEWDA(Set New Dynamic Address) for a particular target.
 *
 * Note this does not update @p target with the new dynamic address.
 *
 * @param[in] target Pointer to the target device descriptor where
 *                   the device is configured with a static address.
 * @param[in] new_da Struct of the Dynamic address
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setnewda(const struct i3c_device_desc *target,
			  struct i3c_ccc_address new_da);

/**
 * @brief Broadcast ENEC/DISEC to enable/disable target events.
 *
 * Helper function to broadcast Target Events Command to enable or
 * disable target events (ENEC/DISEC).
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] enable ENEC if true, DISEC if false.
 * @param[in] events Pointer to the event struct.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_events_all_set(const struct device *controller,
			      bool enable, struct i3c_ccc_events *events);

/**
 * @brief Direct CCC ENEC/DISEC to enable/disable target events.
 *
 * Helper function to send Target Events Command to enable or
 * disable target events (ENEC/DISEC) on a single target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] enable ENEC if true, DISEC if false.
 * @param[in] events Pointer to the event struct.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_events_set(struct i3c_device_desc *target,
			  bool enable, struct i3c_ccc_events *events);

/**
 * @brief Direct ENTAS to set the Activity State.
 *
 * Helper function to broadcast Activity State Command on a single
 * target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] as Activity State level
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_entas(const struct i3c_device_desc *target, uint8_t as);

/**
 * @brief Direct ENTAS0
 *
 * Helper function to do ENTAS0 setting the minimum bus activity level to 1us
 * on a single target.
 *
 * @param[in] target Pointer to the target device descriptor.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas0(const struct i3c_device_desc *target)
{
	return i3c_ccc_do_entas(target, 0);
}

/**
 * @brief Direct ENTAS1
 *
 * Helper function to do ENTAS1 setting the minimum bus activity level to 100us
 * on a single target.
 *
 * @param[in] target Pointer to the target device descriptor.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas1(const struct i3c_device_desc *target)
{
	return i3c_ccc_do_entas(target, 1);
}

/**
 * @brief Direct ENTAS2
 *
 * Helper function to do ENTAS2 setting the minimum bus activity level to 2ms
 * on a single target.
 *
 * @param[in] target Pointer to the target device descriptor.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas2(const struct i3c_device_desc *target)
{
	return i3c_ccc_do_entas(target, 2);
}

/**
 * @brief Direct ENTAS3
 *
 * Helper function to do ENTAS3 setting the minimum bus activity level to 50ms
 * on a single target.
 *
 * @param[in] target Pointer to the target device descriptor.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas3(const struct i3c_device_desc *target)
{
	return i3c_ccc_do_entas(target, 3);
}

/**
 * @brief Broadcast ENTAS to set the Activity State.
 *
 * Helper function to broadcast Activity State Command.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] as Activity State level
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_entas_all(const struct device *controller, uint8_t as);

/**
 * @brief Broadcast ENTAS0
 *
 * Helper function to do ENTAS0 setting the minimum bus activity level to 1us
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas0_all(const struct device *controller)
{
	return i3c_ccc_do_entas_all(controller, 0);
}

/**
 * @brief Broadcast ENTAS1
 *
 * Helper function to do ENTAS1 setting the minimum bus activity level to 100us
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas1_all(const struct device *controller)
{
	return i3c_ccc_do_entas_all(controller, 1);
}

/**
 * @brief Broadcast ENTAS2
 *
 * Helper function to do ENTAS2 setting the minimum bus activity level to 2ms
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas2_all(const struct device *controller)
{
	return i3c_ccc_do_entas_all(controller, 2);
}

/**
 * @brief Broadcast ENTAS3
 *
 * Helper function to do ENTAS3 setting the minimum bus activity level to 50ms
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_entas3_all(const struct device *controller)
{
	return i3c_ccc_do_entas_all(controller, 3);
}

/**
 * @brief Broadcast SETMWL to Set Maximum Write Length.
 *
 * Helper function to do SETMWL (Set Maximum Write Length) to
 * all connected targets.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] mwl Pointer to SETMWL payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setmwl_all(const struct device *controller,
			  const struct i3c_ccc_mwl *mwl);

/**
 * @brief Single target SETMWL to Set Maximum Write Length.
 *
 * Helper function to do SETMWL (Set Maximum Write Length) to
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] mwl Pointer to SETMWL payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setmwl(const struct i3c_device_desc *target,
		      const struct i3c_ccc_mwl *mwl);

/**
 * @brief Single target GETMWL to Get Maximum Write Length.
 *
 * Helper function to do GETMWL (Get Maximum Write Length) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] mwl Pointer to GETMWL payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getmwl(const struct i3c_device_desc *target,
		      struct i3c_ccc_mwl *mwl);

/**
 * @brief Broadcast SETMRL to Set Maximum Read Length.
 *
 * Helper function to do SETMRL (Set Maximum Read Length) to
 * all connected targets.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] mrl Pointer to SETMRL payload.
 * @param[in] has_ibi_size True if also sending the optional IBI payload
 *                         size. False if not sending.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setmrl_all(const struct device *controller,
			  const struct i3c_ccc_mrl *mrl,
			  bool has_ibi_size);

/**
 * @brief Single target SETMRL to Set Maximum Read Length.
 *
 * Helper function to do SETMRL (Set Maximum Read Length) to
 * one target.
 *
 * Note this uses the BCR of the target to determine whether
 * to send the optional IBI payload size.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] mrl Pointer to SETMRL payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setmrl(const struct i3c_device_desc *target,
		      const struct i3c_ccc_mrl *mrl);

/**
 * @brief Single target GETMRL to Get Maximum Read Length.
 *
 * Helper function to do GETMRL (Get Maximum Read Length) of
 * one target.
 *
 * Note this uses the BCR of the target to determine whether
 * to send the optional IBI payload size.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] mrl Pointer to GETMRL payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getmrl(const struct i3c_device_desc *target,
		      struct i3c_ccc_mrl *mrl);

/**
 * @brief Broadcast ENTTM
 *
 * Helper function to do ENTTM (Enter Test Mode) to all devices
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] defbyte Defining Byte for ENTTM.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_enttm(const struct device *controller,
			 enum i3c_ccc_enttm_defbyte defbyte);

/**
 * @brief Single target GETSTATUS to Get Target Status.
 *
 * Helper function to do GETSTATUS (Get Target Status) of
 * one target.
 *
 * Note this uses the BCR of the target to determine whether
 * to send the optional IBI payload size.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] status Pointer to GETSTATUS payload.
 * @param[in] fmt Which GETSTATUS to use.
 * @param[in] defbyte Defining Byte if using format 2.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getstatus(const struct i3c_device_desc *target,
			 union i3c_ccc_getstatus *status,
			 enum i3c_ccc_getstatus_fmt fmt,
			 enum i3c_ccc_getstatus_defbyte defbyte);

/**
 * @brief Single target GETSTATUS to Get Target Status (Format 1).
 *
 * Helper function to do GETSTATUS (Get Target Status, format 1) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] status Pointer to GETSTATUS payload.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getstatus_fmt1(const struct i3c_device_desc *target,
					    union i3c_ccc_getstatus *status)
{
	return i3c_ccc_do_getstatus(target, status,
				    GETSTATUS_FORMAT_1,
				    GETSTATUS_FORMAT_2_INVALID);
}

/**
 * @brief Single target GETSTATUS to Get Target Status (Format 2).
 *
 * Helper function to do GETSTATUS (Get Target Status, format 2) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] status Pointer to GETSTATUS payload.
 * @param[in] defbyte Defining Byte for GETSTATUS format 2.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getstatus_fmt2(const struct i3c_device_desc *target,
					    union i3c_ccc_getstatus *status,
					    enum i3c_ccc_getstatus_defbyte defbyte)
{
	return i3c_ccc_do_getstatus(target, status,
				    GETSTATUS_FORMAT_2, defbyte);
}

/**
 * @brief Single target GETCAPS to Get Target Status.
 *
 * Helper function to do GETCAPS (Get Capabilities) of
 * one target.
 *
 * This should only be supported if Advanced Capabilities Bit of
 * the BCR is set
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETCAPS payload.
 * @param[in] fmt Which GETCAPS to use.
 * @param[in] defbyte Defining Byte if using format 2.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getcaps(const struct i3c_device_desc *target,
			 union i3c_ccc_getcaps *caps,
			 enum i3c_ccc_getcaps_fmt fmt,
			 enum i3c_ccc_getcaps_defbyte defbyte);

/**
 * @brief Single target GETCAPS to Get Capabilities (Format 1).
 *
 * Helper function to do GETCAPS (Get Capabilities, format 1) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETCAPS payload.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getcaps_fmt1(const struct i3c_device_desc *target,
					    union i3c_ccc_getcaps *caps)
{
	return i3c_ccc_do_getcaps(target, caps,
				    GETCAPS_FORMAT_1,
				    GETCAPS_FORMAT_2_INVALID);
}

/**
 * @brief Single target GETCAPS to Get Capabilities (Format 2).
 *
 * Helper function to do GETCAPS (Get Capabilities, format 2) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETCAPS payload.
 * @param[in] defbyte Defining Byte for GETCAPS format 2.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getcaps_fmt2(const struct i3c_device_desc *target,
					    union i3c_ccc_getcaps *caps,
					    enum i3c_ccc_getcaps_defbyte defbyte)
{
	return i3c_ccc_do_getcaps(target, caps,
				    GETCAPS_FORMAT_2, defbyte);
}

/**
 * @brief Single target to Set Vendor / Standard Extension CCC
 *
 * Helper function to set Vendor / Standard Extension CCC of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] id Vendor CCC ID.
 * @param[in] payload Pointer to payload.
 * @param[in] len Length of payload. 0 if no payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setvendor(const struct i3c_device_desc *target,
			uint8_t id,
			uint8_t *payload,
			size_t len);

/**
 * @brief Single target to Get Vendor / Standard Extension CCC
 *
 * Helper function to get Vendor / Standard Extension CCC of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] id Vendor CCC ID.
 * @param[out] payload Pointer to payload.
 * @param[in] len Maximum Expected Length of the payload
 * @param[out] num_xfer Length of the received payload
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getvendor(const struct i3c_device_desc *target,
			uint8_t id,
			uint8_t *payload,
			size_t len,
			size_t *num_xfer);

/**
 * @brief Single target to Get Vendor / Standard Extension CCC
 * with a defining byte
 *
 * Helper function to get Vendor / Standard Extension CCC of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[in] id Vendor CCC ID.
 * @param[in] defbyte Defining Byte
 * @param[out] payload Pointer to payload.
 * @param[in] len Maximum Expected Length of the payload
 * @param[out] num_xfer Length of the received payload
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getvendor_defbyte(const struct i3c_device_desc *target,
			uint8_t id,
			uint8_t defbyte,
			uint8_t *payload,
			size_t len,
			size_t *num_xfer);

/**
 * @brief Broadcast Set Vendor / Standard Extension CCC
 *
 * Helper function to broadcast Vendor / Standard Extension CCC
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] id Vendor CCC ID.
 * @param[in] payload Pointer to payload.
 * @param[in] len Length of payload. 0 if no payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setvendor_all(const struct device *controller,
			uint8_t id,
			uint8_t *payload,
			size_t len);

/**
 * @brief Broadcast SETAASA to set all target's dynamic address to their
 * static address.
 *
 * Helper function to set dynamic addresses of all connected targets to
 * their static address.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setaasa_all(const struct device *controller);

/**
 * @brief Single target GETMXDS to Get Max Data Speed.
 *
 * Helper function to do GETMXDS (Get Max Data Speed) of
 * one target.
 *
 * This should only be supported if Max Data Speed Limit Bit of
 * the BCR is set
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETMXDS payload.
 * @param[in] fmt Which GETMXDS to use.
 * @param[in] defbyte Defining Byte if using format 3.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_getmxds(const struct i3c_device_desc *target,
			 union i3c_ccc_getmxds *caps,
			 enum i3c_ccc_getmxds_fmt fmt,
			 enum i3c_ccc_getmxds_defbyte defbyte);

/**
 * @brief Single target GETMXDS to Get Max Data Speed (Format 1).
 *
 * Helper function to do GETMXDS (Get Max Data Speed, format 1) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETMXDS payload.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getmxds_fmt1(const struct i3c_device_desc *target,
					    union i3c_ccc_getmxds *caps)
{
	return i3c_ccc_do_getmxds(target, caps,
				    GETMXDS_FORMAT_1,
				    GETMXDS_FORMAT_3_INVALID);
}

/**
 * @brief Single target GETMXDS to Get Max Data Speed (Format 2).
 *
 * Helper function to do GETMXDS (Get Max Data Speed, format 2) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETMXDS payload.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getmxds_fmt2(const struct i3c_device_desc *target,
					    union i3c_ccc_getmxds *caps)
{
	return i3c_ccc_do_getmxds(target, caps,
				    GETMXDS_FORMAT_2,
					GETMXDS_FORMAT_3_INVALID);
}

/**
 * @brief Single target GETMXDS to Get Max Data Speed (Format 3).
 *
 * Helper function to do GETMXDS (Get Max Data Speed, format 3) of
 * one target.
 *
 * @param[in] target Pointer to the target device descriptor.
 * @param[out] caps Pointer to GETMXDS payload.
 * @param[in] defbyte Defining Byte for GETMXDS format 3.
 *
 * @return @see i3c_do_ccc
 */
static inline int i3c_ccc_do_getmxds_fmt3(const struct i3c_device_desc *target,
					    union i3c_ccc_getmxds *caps,
					    enum i3c_ccc_getmxds_defbyte defbyte)
{
	return i3c_ccc_do_getmxds(target, caps,
				    GETMXDS_FORMAT_3, defbyte);
}

/**
 * @brief Broadcast DEFTGTS
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] deftgts Pointer to the deftgts payload.
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_deftgts_all(const struct device *controller,
			   struct i3c_ccc_deftgts *deftgts);

/**
 * @brief Broadcast SETBUSCON to set the bus context
 *
 * Helper function to set the bus context of all connected targets.
 *
 * @param[in] controller Pointer to the controller device driver instance.
 * @param[in] context Pointer to context byte values
 * @param[in] length Length of the context buffer
 *
 * @return @see i3c_do_ccc
 */
int i3c_ccc_do_setbuscon(const struct device *controller,
				uint8_t *context, uint16_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_CCC_H_ */

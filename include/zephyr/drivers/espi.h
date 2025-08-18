/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup espi_interface
 * @brief Main header file for eSPI (Enhanced Serial Peripheral Interface) driver API.
 */

#ifndef ZEPHYR_INCLUDE_ESPI_H_
#define ZEPHYR_INCLUDE_ESPI_H_

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Enhanced Serial Peripheral Interface (eSPI)
 *        controllers.
 * @defgroup espi_interface ESPI
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief eSPI I/O mode capabilities
 */
enum espi_io_mode {
	/** Single data line mode (traditional SPI) */
	ESPI_IO_MODE_SINGLE_LINE = BIT(0),
	/** Dual data line mode */
	ESPI_IO_MODE_DUAL_LINES = BIT(1),
	/** Quad data line mode */
	ESPI_IO_MODE_QUAD_LINES = BIT(2),
};

/**
 * @code
 *+----------------------------------------------------------------------+
 *|                                                                      |
 *|  eSPI controller                     +-------------+                 |
 *|                      +-----------+   |    Power    |   +----------+  |
 *|                      |Out of band|   |  management |   |   GPIO   |  |
 *|  +------------+      |processor  |   |  controller |   |  sources |  |
 *|  |  SPI flash |      +-----------+   +-------------+   +----------+  |
 *|  | controller |            |                |               |        |
 *|  +------------+            |                |               |        |
 *|   |  |    |                +--------+       +---------------+        |
 *|   |  |    |                         |               |                |
 *|   |  |    +-----+   +--------+   +----------+  +----v-----+          |
 *|   |  |          |   |  LPC   |   | Tunneled |  | Tunneled |          |
 *|   |  |          |   | bridge |   |  SMBus   |  |   GPIO   |          |
 *|   |  |          |   +--------+   +----------+  +----------+          |
 *|   |  |          |        |             |             |               |
 *|   |  |          |        ------+       |             |               |
 *|   |  |          |              |       |             |               |
 *|   |  |   +------v-----+    +---v-------v-------------v----+          |
 *|   |  |   | eSPI Flash |    |    eSPI protocol block       |          |
 *|   |  |   |   access   +--->+                              |          |
 *|   |  |   +------------+    +------------------------------+          |
 *|   |  |                             |                                 |
 *|   |  +-----------+                 |                                 |
 *|   |              v                 v                                 |
 *|   |           XXXXXXXXXXXXXXXXXXXXXXX                                |
 *|   |            XXXXXXXXXXXXXXXXXXXXX                                 |
 *|   |             XXXXXXXXXXXXXXXXXXX                                  |
 *+----------------------------------------------------------------------+
 *   |                      |
 *   v             +-----------------+
 * +---------+     |  |   |   |   |  |
 * |  Flash  |     |  |   |   |   |  |
 * +---------+     |  +   +   +   +  |    eSPI bus
 *                 | CH0 CH1 CH2 CH3 |    (logical channels)
 *                 |  +   +   +   +  |
 *                 |  |   |   |   |  |
 *                 +-----------------+
 *                          |
 *+-----------------------------------------------------------------------+
 *|  eSPI target                                                          |
 *|                                                                       |
 *|       CH0         |     CH1      |      CH2      |    CH3             |
 *|   eSPI endpoint   |    VWIRE     |      OOB      |   Flash            |
 *+-----------------------------------------------------------------------+
 * @endcode
 */

/**
 * @brief eSPI channel.
 *
 * Identifies each eSPI logical channel supported by eSPI controller
 * Each channel allows independent traffic, but the assignment of channel
 * type to channel number is fixed.
 *
 * Note that generic commands are not associated with any channel, so traffic
 * over eSPI can occur if all channels are disabled or not ready
 */
enum espi_channel {
	ESPI_CHANNEL_PERIPHERAL = BIT(0), /**< Peripheral channel (channel 0) */
	ESPI_CHANNEL_VWIRE      = BIT(1), /**< Virtual Wire channel (channel 1) */
	ESPI_CHANNEL_OOB        = BIT(2), /**< Out-of-Band channel (channel 2) */
	ESPI_CHANNEL_FLASH      = BIT(3), /**< Flash Access channel (channel 3) */
};

/**
 * @brief eSPI bus event.
 *
 * eSPI bus event to indicate events for which user can register callbacks
 */
enum espi_bus_event {
	/** Indicates the eSPI bus was reset either via eSPI reset pin.
	 * eSPI drivers should convey the eSPI reset status to eSPI driver clients
	 * following eSPI specification reset pin convention:
	 * 0-eSPI bus in reset, 1-eSPI bus out-of-reset
	 *
	 * Note: There is no need to send this callback for in-band reset.
	 */
	ESPI_BUS_RESET                      = BIT(0),

	/** Indicates the eSPI HW has received channel enable notification from eSPI host,
	 * once the eSPI channel is signaled as ready to the eSPI host,
	 * eSPI drivers should convey the eSPI channel ready to eSPI driver client via this event.
	 */
	ESPI_BUS_EVENT_CHANNEL_READY        = BIT(1),

	/** Indicates the eSPI HW has received a virtual wire message from eSPI host.
	 * eSPI drivers should convey the eSPI virtual wire latest status.
	 */
	ESPI_BUS_EVENT_VWIRE_RECEIVED       = BIT(2),

	/** Indicates the eSPI HW has received a Out-of-band packet from eSPI host.
	 */
	ESPI_BUS_EVENT_OOB_RECEIVED         = BIT(3),

	/** Indicates the eSPI HW has received a peripheral eSPI host event.
	 * eSPI drivers should convey the peripheral type.
	 */
	ESPI_BUS_PERIPHERAL_NOTIFICATION    = BIT(4),
	/** Indicates the eSPI HW has received a Target Attached Flash (TAF) notification event */
	ESPI_BUS_TAF_NOTIFICATION           = BIT(5),
};

/**
 * @brief eSPI peripheral channel events.
 *
 * eSPI peripheral channel event types to signal users.
 */
enum espi_pc_event {
	/** Bus channel ready event */
	ESPI_PC_EVT_BUS_CHANNEL_READY = BIT(0),
	/** Bus master enable event */
	ESPI_PC_EVT_BUS_MASTER_ENABLE = BIT(1),
};

/**
 * @cond INTERNAL_HIDDEN
 *
 */
#define ESPI_PERIPHERAL_INDEX_0  0ul
#define ESPI_PERIPHERAL_INDEX_1  1ul
#define ESPI_PERIPHERAL_INDEX_2  2ul

#define ESPI_TARGET_TO_CONTROLLER   0ul
#define ESPI_CONTROLLER_TO_TARGET   1ul

#define ESPI_VWIRE_SRC_ID0       0ul
#define ESPI_VWIRE_SRC_ID1       1ul
#define ESPI_VWIRE_SRC_ID2       2ul
#define ESPI_VWIRE_SRC_ID3       3ul
#define ESPI_VWIRE_SRC_ID_MAX    4ul

#define ESPI_PERIPHERAL_NODATA   0ul

#define E8042_START_OPCODE      0x50
#define E8042_MAX_OPCODE        0x5F

#define EACPI_START_OPCODE      0x60
#define EACPI_MAX_OPCODE        0x6F

#define ECUSTOM_START_OPCODE    0xF0
#define ECUSTOM_MAX_OPCODE      0xFF

/** @endcond */

/**
 * @brief eSPI peripheral notification type.
 *
 * eSPI peripheral notification event details to indicate which peripheral
 * trigger the eSPI callback
 */
enum espi_virtual_peripheral {
	/** UART peripheral */
	ESPI_PERIPHERAL_UART,
	/** 8042 Keyboard Controller peripheral */
	ESPI_PERIPHERAL_8042_KBC,
	/** Host I/O peripheral */
	ESPI_PERIPHERAL_HOST_IO,
	/** Debug Port 80 peripheral */
	ESPI_PERIPHERAL_DEBUG_PORT80,
	/** Private Host I/O peripheral */
	ESPI_PERIPHERAL_HOST_IO_PVT,
	/** Private Host I/O peripheral 2 */
	ESPI_PERIPHERAL_HOST_IO_PVT2,
	/** Private Host I/O peripheral 3 */
	ESPI_PERIPHERAL_HOST_IO_PVT3,
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD) || defined(__DOXYGEN__)
	/**
	 * Embedded Controller Host Command peripheral
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD}
	 */
	ESPI_PERIPHERAL_EC_HOST_CMD,
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */
};

/**
 * @brief eSPI cycle types supported over eSPI peripheral channel
 */
enum espi_cycle_type {
	/** 32-bit memory read cycle */
	ESPI_CYCLE_MEMORY_READ32,
	/** 64-bit memory read cycle */
	ESPI_CYCLE_MEMORY_READ64,
	/** 32-bit memory write cycle */
	ESPI_CYCLE_MEMORY_WRITE32,
	/** 64-bit memory write cycle */
	ESPI_CYCLE_MEMORY_WRITE64,
	/** Message cycle with no data */
	ESPI_CYCLE_MESSAGE_NODATA,
	/** Message cycle with data */
	ESPI_CYCLE_MESSAGE_DATA,
	/** Successful completion with no data */
	ESPI_CYCLE_OK_COMPLETION_NODATA,
	/** Successful completion with data */
	ESPI_CYCLE_OKCOMPLETION_DATA,
	/** Unsuccessful completion with no data */
	ESPI_CYCLE_NOK_COMPLETION_NODATA,
};

/**
 * @brief eSPI system platform signals that can be sent or received through
 * virtual wire channel
 */
enum espi_vwire_signal {
	/* Virtual wires that can only be send from controller to target */
	ESPI_VWIRE_SIGNAL_SLP_S3,        /**< Sleep S3 state signal */
	ESPI_VWIRE_SIGNAL_SLP_S4,        /**< Sleep S4 state signal */
	ESPI_VWIRE_SIGNAL_SLP_S5,        /**< Sleep S5 state signal */
	ESPI_VWIRE_SIGNAL_OOB_RST_WARN,  /**< Out-of-band reset warning signal */
	ESPI_VWIRE_SIGNAL_PLTRST,        /**< Platform reset signal */
	ESPI_VWIRE_SIGNAL_SUS_STAT,      /**< Suspend status signal */
	ESPI_VWIRE_SIGNAL_NMIOUT,        /**< NMI output signal */
	ESPI_VWIRE_SIGNAL_SMIOUT,        /**< SMI output signal */
	ESPI_VWIRE_SIGNAL_HOST_RST_WARN, /**< Host reset warning signal */
	ESPI_VWIRE_SIGNAL_SLP_A,         /**< Sleep A state signal */
	ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, /**< Suspend power down acknowledge signal */
	ESPI_VWIRE_SIGNAL_SUS_WARN,      /**< Suspend warning signal */
	ESPI_VWIRE_SIGNAL_SLP_WLAN,      /**< Sleep WLAN signal */
	ESPI_VWIRE_SIGNAL_SLP_LAN,       /**< Sleep LAN signal */
	ESPI_VWIRE_SIGNAL_HOST_C10,      /**< Host C10 state signal */
	ESPI_VWIRE_SIGNAL_DNX_WARN,      /**< DNX (Debug and eXception) warning signal */

	/* Virtual wires that can only be sent from target to controller */
	ESPI_VWIRE_SIGNAL_PME,              /**< Power Management Event signal */
	ESPI_VWIRE_SIGNAL_WAKE,             /**< Wake signal */
	ESPI_VWIRE_SIGNAL_OOB_RST_ACK,      /**< Out-of-band reset acknowledge signal */
	ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS,  /**< Target boot status signal */
	ESPI_VWIRE_SIGNAL_ERR_NON_FATAL,    /**< Non-fatal error signal */
	ESPI_VWIRE_SIGNAL_ERR_FATAL,        /**< Fatal error signal */
	ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE, /**< Target boot done signal */
	ESPI_VWIRE_SIGNAL_HOST_RST_ACK,     /**< Host reset acknowledge signal */
	ESPI_VWIRE_SIGNAL_RST_CPU_INIT,     /**< Reset CPU initialization signal */
	ESPI_VWIRE_SIGNAL_SMI,              /**< System Management Interrupt signal */
	ESPI_VWIRE_SIGNAL_SCI,              /**< System Control Interrupt signal */
	ESPI_VWIRE_SIGNAL_DNX_ACK,          /**< DNX acknowledge signal */
	ESPI_VWIRE_SIGNAL_SUS_ACK,          /**< Suspend acknowledge signal */

	/*
	 * Virtual wire GPIOs that can be sent from target to controller for
	 * platform specific usage.
	 */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_0,  /**< Target GPIO 0 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_1,  /**< Target GPIO 1 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_2,  /**< Target GPIO 2 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_3,  /**< Target GPIO 3 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_4,  /**< Target GPIO 4 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_5,  /**< Target GPIO 5 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_6,  /**< Target GPIO 6 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_7,  /**< Target GPIO 7 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_8,  /**< Target GPIO 8 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_9,  /**< Target GPIO 9 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_10, /**< Target GPIO 10 signal */
	ESPI_VWIRE_SIGNAL_TARGET_GPIO_11, /**< Target GPIO 11 signal */

	/** Number of Virtual Wire signals */
	ESPI_VWIRE_SIGNAL_COUNT
};

/**
 * @name USB-C port over current signal aliases
 * @{
 */
/** USB-C port 0 over current signal */
#define ESPI_VWIRE_SIGNAL_OCB_0 ESPI_VWIRE_SIGNAL_TARGET_GPIO_0
/** USB-C port 1 over current signal */
#define ESPI_VWIRE_SIGNAL_OCB_1 ESPI_VWIRE_SIGNAL_TARGET_GPIO_1
/** USB-C port 2 over current signal */
#define ESPI_VWIRE_SIGNAL_OCB_2 ESPI_VWIRE_SIGNAL_TARGET_GPIO_2
/** USB-C port 3 over current signal */
#define ESPI_VWIRE_SIGNAL_OCB_3 ESPI_VWIRE_SIGNAL_TARGET_GPIO_3
/** @} */

/**
 * @brief eSPI LPC peripheral opcodes
 *
 * Opcodes for LPC peripheral operations that are tunneled through eSPI.
 * These opcodes are used to interact with legacy LPC devices.
 */
enum lpc_peripheral_opcode {
	/* Read transactions */
	E8042_OBF_HAS_CHAR = 0x50, /**< Check if 8042 output buffer has character */
	E8042_IBF_HAS_CHAR,        /**< Check if 8042 input buffer has character */
	/* Write transactions */
	E8042_WRITE_KB_CHAR, /**< Write character to 8042 keyboard buffer */
	E8042_WRITE_MB_CHAR, /**< Write character to 8042 mouse buffer */
	/* Write transactions without input parameters */
	E8042_RESUME_IRQ, /**< Resume 8042 interrupt processing */
	E8042_PAUSE_IRQ,  /**< Pause 8042 interrupt processing */
	E8042_CLEAR_OBF,  /**< Clear 8042 output buffer */
	/* Status transactions */
	E8042_READ_KB_STS, /**< Read 8042 keyboard status */
	E8042_SET_FLAG,    /**< Set 8042 flag */
	E8042_CLEAR_FLAG,  /**< Clear 8042 flag */
	/* ACPI read transactions */
	EACPI_OBF_HAS_CHAR = EACPI_START_OPCODE, /**< Check if ACPI output buffer has character */
	EACPI_IBF_HAS_CHAR,                      /**< Check if ACPI input buffer has character */
	/* ACPI write transactions */
	EACPI_WRITE_CHAR, /**< Write character to ACPI output buffer */
	/* ACPI status transactions */
	EACPI_READ_STS,  /**< Read ACPI status */
	EACPI_WRITE_STS, /**< Write ACPI status */
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION) || defined(__DOXYGEN__)
	/**
	 * Shared memory region support to return the ACPI response data
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION}
	 */
	EACPI_GET_SHARED_MEMORY,
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE) || defined(__DOXYGEN__)
	/* Other customized transactions */
	/**
	 * Enable host subsystem interrupt (custom)
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE}
	 */
	ECUSTOM_HOST_SUBS_INTERRUPT_EN = ECUSTOM_START_OPCODE,
	/**
	 * Get host command parameter memory (custom)
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE}
	 */
	ECUSTOM_HOST_CMD_GET_PARAM_MEMORY,
	/**
	 * Get host command parameter memory size (custom)
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE}
	 */
	ECUSTOM_HOST_CMD_GET_PARAM_MEMORY_SIZE,
	/**
	 * Send host command result (custom)
	 * @kconfig_dep{CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE}
	 */
	ECUSTOM_HOST_CMD_SEND_RESULT,
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
};

/** KBC 8042 event: Input Buffer Full */
#define HOST_KBC_EVT_IBF BIT(0)
/** KBC 8042 event: Output Buffer Empty */
#define HOST_KBC_EVT_OBE BIT(1)

/**
 * @brief Event data format for KBC events.
 *
 * Event data (@ref espi_event.evt_data) for Keyboard Controller (8042)
 * events, allowing to manipulate the raw event data as a bit field.
 */
struct espi_evt_data_kbc {
	/** Event type identifier */
	uint32_t type: 8;
	/** Event data payload */
	uint32_t data: 8;
	/** Event flags */
	uint32_t evt: 8;
	/** Reserved field for future use */
	uint32_t reserved: 8;
};

/**
 * @brief Event data format for ACPI events.
 *
 * Event data (@ref espi_event.evt_data) for ACPI events, allowing to manipulate the raw event
 * data as a bit field.
 */
struct espi_evt_data_acpi {
	/** Event type identifier */
	uint32_t type: 8;
	/** Event data payload */
	uint32_t data: 8;
	/** Reserved field for future use */
	uint32_t reserved: 16;
};

/**
 * @brief Event data format for Private Channel (PVT) events.
 *
 * Event data (@ref espi_event.evt_data) for Private Channel (PVT) events, allowing to manipulate
 * the raw event data as a bit field.
 */
struct espi_evt_data_pvt {
	/** Event type identifier */
	uint32_t type: 8;
	/** Event data payload */
	uint32_t data: 8;
	/** Reserved field for future use */
	uint32_t reserved: 16;
};

/**
 * @brief eSPI event
 *
 * Represents an eSPI bus event that is passed to application callbacks,
 * providing details about what occurred.
 */
struct espi_event {
	/** The type of event that occurred. */
	enum espi_bus_event evt_type;
	/**
	 * Additional details for the event.
	 *
	 * The meaning depends on @ref evt_type.
	 */
	uint32_t evt_details;
	/**
	 * Data associated with the event.
	 *
	 * The meaning depends on @ref evt_type.
	 * @c espi_evt_data_* structures can be used to manipulate the raw event data as a bit
	 * field.
	 */
	uint32_t evt_data;
};

/**
 * @brief eSPI bus configuration parameters
 */
struct espi_cfg {
	/** Supported I/O mode */
	enum espi_io_mode io_caps;
	/** Supported channels */
	enum espi_channel channel_caps;
	/** Maximum supported frequency in MHz */
	uint8_t max_freq;
};

/**
 * @brief eSPI peripheral request packet.
 *
 * Defines the format for peripheral channel (CH0) transactions, which are used
 * for memory, I/O, and message cycles.
 */
struct espi_request_packet {
	/** Type of eSPI cycle being performed. */
	enum espi_cycle_type cycle_type;
	/** Transaction tag for tracking. */
	uint8_t tag;
	/** Length of the data payload in bytes. */
	uint16_t len;
	/** Target address for the transaction. */
	uint32_t address;
	/** Pointer to the data buffer for read or write operations. */
	uint8_t *data;
};

/**
 * @brief eSPI out-of-band transaction packet format
 *
 * - For Tx packet, eSPI driver client shall specify the OOB payload data and its length in bytes.
 * - For Rx packet, eSPI driver client shall indicate the maximum number of bytes that can receive,
 *   while the eSPI driver should update the length field with the actual data received/available.
 *
 * @note In all cases, the @ref len does not include OOB header size 3 bytes.
 */
struct espi_oob_packet {
	/** Pointer to the data buffer. */
	uint8_t *buf;
	/**
	 * Length of the data in bytes (excluding the 3-byte OOB header).
	 * On reception, this is updated by the driver to the actual size.
	 */
	uint16_t len;
};

/**
 * @brief eSPI flash transactions packet format
 */
struct espi_flash_packet {
	/** Pointer to the data buffer. */
	uint8_t *buf;
	/** Flash address to access. */
	uint32_t flash_addr;
	/**
	 * Length of the data in bytes for read/write, or the size of the
	 * sector/block for an erase operation.
	 */
	uint16_t len;
};

/**
 * @brief Opaque type representing an eSPI callback.
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct espi_callback.
 * Beware such structure should not be allocated on stack.
 *
 * @note To help setting a callback, see @ref espi_init_callback() helper.
 */
struct espi_callback;

/**
 * @typedef espi_callback_handler_t
 * @brief Define the application callback handler function signature.
 *
 * @param dev Device struct for the eSPI device.
 * @param cb Original struct espi_callback owning this handler.
 * @param espi_evt event details that trigger the callback handler.
 *
 */
typedef void (*espi_callback_handler_t) (const struct device *dev,
					 struct espi_callback *cb,
					 struct espi_event espi_evt);

/**
 * @cond INTERNAL_HIDDEN
 */
struct espi_callback {
	/** This is meant to be used in the driver only */
	sys_snode_t node;

	/** Actual callback function being called when relevant */
	espi_callback_handler_t handler;

	/** An event which user is interested in, if 0 the callback
	 * will never be called. Such evt_mask can be modified whenever
	 * necessary by the owner, and thus will affect the handler being
	 * called or not.
	 */
	enum espi_bus_event evt_type;
};
/** @endcond */

/**
 * @cond INTERNAL_HIDDEN
 *
 * eSPI driver API definition and system call entry points
 *
 * (Internal use only.)
 */
typedef int (*espi_api_config)(const struct device *dev, struct espi_cfg *cfg);
typedef bool (*espi_api_get_channel_status)(const struct device *dev,
					    enum espi_channel ch);
/* Logical Channel 0 APIs */
typedef int (*espi_api_read_request)(const struct device *dev,
				     struct espi_request_packet *req);
typedef int (*espi_api_write_request)(const struct device *dev,
				      struct espi_request_packet *req);
typedef int (*espi_api_lpc_read_request)(const struct device *dev,
					 enum lpc_peripheral_opcode op,
					 uint32_t *data);
typedef int (*espi_api_lpc_write_request)(const struct device *dev,
					  enum lpc_peripheral_opcode op,
					  uint32_t *data);
/* Logical Channel 1 APIs */
typedef int (*espi_api_send_vwire)(const struct device *dev,
				   enum espi_vwire_signal vw,
				   uint8_t level);
typedef int (*espi_api_receive_vwire)(const struct device *dev,
				      enum espi_vwire_signal vw,
				      uint8_t *level);
/* Logical Channel 2 APIs */
typedef int (*espi_api_send_oob)(const struct device *dev,
				 struct espi_oob_packet *pckt);
typedef int (*espi_api_receive_oob)(const struct device *dev,
				    struct espi_oob_packet *pckt);
/* Logical Channel 3 APIs */
typedef int (*espi_api_flash_read)(const struct device *dev,
				   struct espi_flash_packet *pckt);
typedef int (*espi_api_flash_write)(const struct device *dev,
				    struct espi_flash_packet *pckt);
typedef int (*espi_api_flash_erase)(const struct device *dev,
				    struct espi_flash_packet *pckt);
/* Callbacks and traffic intercept */
typedef int (*espi_api_manage_callback)(const struct device *dev,
					struct espi_callback *callback,
					bool set);

__subsystem struct espi_driver_api {
	espi_api_config config;
	espi_api_get_channel_status get_channel_status;
	espi_api_read_request read_request;
	espi_api_write_request write_request;
	espi_api_lpc_read_request read_lpc_request;
	espi_api_lpc_write_request write_lpc_request;
	espi_api_send_vwire send_vwire;
	espi_api_receive_vwire receive_vwire;
	espi_api_send_oob send_oob;
	espi_api_receive_oob receive_oob;
	espi_api_flash_read flash_read;
	espi_api_flash_write flash_write;
	espi_api_flash_erase flash_erase;
	espi_api_manage_callback manage_callback;
};

/**
 * @endcond
 */

/**
 * @brief Configure operation of a eSPI controller.
 *
 * This routine provides a generic interface to override eSPI controller
 * capabilities.
 *
 * If this eSPI controller is acting as target, the values set here
 * will be discovered as part through the GET_CONFIGURATION command
 * issued by the eSPI controller during initialization.
 *
 * If this eSPI controller is acting as controller, the values set here
 * will be used by eSPI controller to determine minimum common capabilities with
 * eSPI target then send via SET_CONFIGURATION command.
 *
 * @code
 * +---------+   +---------+     +------+          +---------+   +---------+
 * |  eSPI   |   |  eSPI   |     | eSPI |          |  eSPI   |   |  eSPI   |
 * |  target |   | driver  |     |  bus |          |  driver |   |  host   |
 * +--------+   +---------+     +------+          +---------+   +---------+
 *     |              |            |                   |             |
 *     | espi_config  | Set eSPI   |       Set eSPI    | espi_config |
 *     +--------------+ ctrl regs  |       cap ctrl reg| +-----------+
 *     |              +-------+    |          +--------+             |
 *     |              |<------+    |          +------->|             |
 *     |              |            |                   |             |
 *     |              |            |                   |             |
 *     |              |            | GET_CONFIGURATION |             |
 *     |              |            +<------------------+             |
 *     |              |<-----------|                   |             |
 *     |              | eSPI caps  |                   |             |
 *     |              |----------->+    response       |             |
 *     |              |            |------------------>+             |
 *     |              |            |                   |             |
 *     |              |            | SET_CONFIGURATION |             |
 *     |              |            +<------------------+             |
 *     |              |            |  accept           |             |
 *     |              |            +------------------>+             |
 *     +              +            +                   +             +
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg the device runtime configuration for the eSPI controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by eSPI target.
 */
__syscall int espi_config(const struct device *dev, struct espi_cfg *cfg);

static inline int z_impl_espi_config(const struct device *dev,
				     struct espi_cfg *cfg)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->config(dev, cfg);
}

/**
 * @brief Query whether a logical channel is ready.
 *
 * This routine allows to check if logical channel is ready before use.
 * Note that queries for channels not supported will always return false.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ch the eSPI channel for which status is to be retrieved.
 *
 * @retval true If eSPI channel is ready.
 * @retval false otherwise.
 */
__syscall bool espi_get_channel_status(const struct device *dev,
				       enum espi_channel ch);

static inline bool z_impl_espi_get_channel_status(const struct device *dev,
						  enum espi_channel ch)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->get_channel_status(dev, ch);
}

/**
 * @brief Sends memory, I/O or message read request over eSPI.
 *
 * This routine provides a generic interface to send a read request packet.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req Address of structure representing a memory,
 *            I/O or message read request.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI controller doesn't support raw packets and instead
 *         low memory transactions are handled by controller hardware directly.
 * @retval -EIO General input / output error, failed to send over the bus.
 */
__syscall int espi_read_request(const struct device *dev,
				struct espi_request_packet *req);

static inline int z_impl_espi_read_request(const struct device *dev,
					   struct espi_request_packet *req)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->read_request) {
		return -ENOTSUP;
	}

	return api->read_request(dev, req);
}

/**
 * @brief Sends memory, I/O or message write request over eSPI.
 *
 * This routine provides a generic interface to send a write request packet.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req Address of structure representing a memory, I/O or
 *            message write request.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI controller doesn't support raw packets and instead
 *         low memory transactions are handled by controller hardware directly.
 * @retval -EIO General input / output error, failed to send over the bus.
 */
__syscall int espi_write_request(const struct device *dev,
				 struct espi_request_packet *req);

static inline int z_impl_espi_write_request(const struct device *dev,
					    struct espi_request_packet *req)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->write_request) {
		return -ENOTSUP;
	}

	return api->write_request(dev, req);
}

/**
 * @brief Reads SOC data from a LPC peripheral with information
 * updated over eSPI.
 *
 * This routine provides a generic interface to read a block whose
 * information was updated by an eSPI transaction. Reading may trigger
 * a transaction. The eSPI packet is assembled by the HW block.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param op Enum representing opcode for peripheral type and read request.
 * @param data Parameter to be read from to the LPC peripheral.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
__syscall int espi_read_lpc_request(const struct device *dev,
				    enum lpc_peripheral_opcode op,
				    uint32_t *data);

static inline int z_impl_espi_read_lpc_request(const struct device *dev,
					       enum lpc_peripheral_opcode op,
					       uint32_t *data)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->read_lpc_request) {
		return -ENOTSUP;
	}

	return api->read_lpc_request(dev, op, data);
}

/**
 * @brief Writes data to a LPC peripheral which generates an eSPI transaction.
 *
 * This routine provides a generic interface to write data to a block which
 * triggers an eSPI transaction. The eSPI packet is assembled by the HW
 * block.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param op Enum representing an opcode for peripheral type and write request.
 * @param data Represents the parameter passed to the LPC peripheral.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI peripheral is off or not supported.
 * @retval -EINVAL for unimplemented lpc opcode, but in range.
 */
__syscall int espi_write_lpc_request(const struct device *dev,
				     enum lpc_peripheral_opcode op,
				     uint32_t *data);

static inline int z_impl_espi_write_lpc_request(const struct device *dev,
						enum lpc_peripheral_opcode op,
						uint32_t *data)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->write_lpc_request) {
		return -ENOTSUP;
	}

	return api->write_lpc_request(dev, op, data);
}

/**
 * @brief Sends system/platform signal as a virtual wire packet.
 *
 * This routine provides a generic interface to send a virtual wire packet
 * from target to controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param signal The signal to be sent to eSPI controller.
 * @param level The level of signal requested. LOW (0) or HIGH (1).
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to send over the bus.
 * @retval -EINVAL invalid signal.
 * @retval -ETIMEDOUT timeout waiting for eSPI controller to process the VW.
 */
__syscall int espi_send_vwire(const struct device *dev,
			      enum espi_vwire_signal signal,
			      uint8_t level);

static inline int z_impl_espi_send_vwire(const struct device *dev,
					 enum espi_vwire_signal signal,
					 uint8_t level)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->send_vwire(dev, signal, level);
}

/**
 * @brief Retrieves level status for a signal encapsulated in a virtual wire.
 *
 * This routine provides a generic interface to request a virtual wire packet
 * from eSPI controller and retrieve the signal level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param signal the signal to be requested from eSPI controller.
 * @param[out] level the level of signal requested 0b LOW, 1b HIGH.
 *
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_receive_vwire(const struct device *dev,
				 enum espi_vwire_signal signal,
				 uint8_t *level);

static inline int z_impl_espi_receive_vwire(const struct device *dev,
					    enum espi_vwire_signal signal,
					    uint8_t *level)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->receive_vwire(dev, signal, level);
}

/**
 * @brief Sends SMBus transaction (out-of-band) packet over eSPI bus.
 *
 * This routine provides an interface to encapsulate a SMBus transaction
 * and send into packet over eSPI bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the packet representation of SMBus transaction.
 *
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_send_oob(const struct device *dev,
			    struct espi_oob_packet *pckt);

static inline int z_impl_espi_send_oob(const struct device *dev,
				       struct espi_oob_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->send_oob) {
		return -ENOTSUP;
	}

	return api->send_oob(dev, pckt);
}

/**
 * @brief Receives SMBus transaction (out-of-band) packet from eSPI bus.
 *
 * This routine provides an interface to receive and decode an SMBus
 * transaction from eSPI bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the packet representation of SMBus transaction.
 *
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_receive_oob(const struct device *dev,
			       struct espi_oob_packet *pckt);

static inline int z_impl_espi_receive_oob(const struct device *dev,
					  struct espi_oob_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->receive_oob) {
		return -ENOTSUP;
	}

	return api->receive_oob(dev, pckt);
}

/**
 * @brief Sends a read request packet for shared flash.
 *
 * This routine provides an interface to send a request to read the flash
 * component shared between the eSPI controller and eSPI targets.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of read flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by controller.
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_read_flash(const struct device *dev,
			      struct espi_flash_packet *pckt);

static inline int z_impl_espi_read_flash(const struct device *dev,
					 struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_read) {
		return -ENOTSUP;
	}

	return api->flash_read(dev, pckt);
}

/**
 * @brief Sends a write request packet for shared flash.
 *
 * This routine provides an interface to send a request to write to the flash
 * components shared between the eSPI controller and eSPI targets.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of write flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by controller.
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_write_flash(const struct device *dev,
			       struct espi_flash_packet *pckt);

static inline int z_impl_espi_write_flash(const struct device *dev,
					  struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_write) {
		return -ENOTSUP;
	}

	return api->flash_write(dev, pckt);
}

/**
 * @brief Sends a write request packet for shared flash.
 *
 * This routine provides an interface to send a request to erase the flash
 * components shared between the eSPI controller and eSPI targets.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of erase flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by controller.
 * @retval -EIO General input / output error, failed request to controller.
 */
__syscall int espi_flash_erase(const struct device *dev,
			       struct espi_flash_packet *pckt);

static inline int z_impl_espi_flash_erase(const struct device *dev,
					  struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_erase) {
		return -ENOTSUP;
	}

	return api->flash_erase(dev, pckt);
}

/**
 * Callback model
 *
 * @code
 *+-------+                  +-------------+   +------+     +---------+
 *|  App  |                  | eSPI driver |   |  HW  |     |eSPI Host|
 *+---+---+                  +-------+-----+   +---+--+     +----+----+
 *    |                              |             |             |
 *    |   espi_init_callback         |             |             |
 *    +----------------------------> |             |             |
 *    |   espi_add_callback          |             |
 *    +----------------------------->+             |
 *    |                              |             |  eSPI reset |  eSPI host
 *    |                              |    IRQ      +<------------+  resets the
 *    |                              | <-----------+             |  bus
 *    |<-----------------------------|             |             |
 *    | Report eSPI bus reset        | Processed   |             |
 *    |                              | within the  |             |
 *    |                              | driver      |             |
 *    |                              |             |             |
 *    |                              |             |  VW CH ready|  eSPI host
 *    |                              |    IRQ      +<------------+  enables VW
 *    |                              | <-----------+             |  channel
 *    |                              |             |             |
 *    |                              | Processed   |             |
 *    |                              | within the  |             |
 *    |                              | driver      |             |
 *    |                              |             |             |
 *    |                              |             | Memory I/O  |  Peripheral
 *    |                              |             <-------------+  event
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | Report peripheral event      |             |             |
 *    | and data for the event       |             |             |
 *    |                              |             |             |
 *    |                              |             | SLP_S5      |  eSPI host
 *    |                              |             <-------------+  send VWire
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | App enables/configures       |             |             |
 *    | discrete regulator           |             |             |
 *    |                              |             |             |
 *    |   espi_send_vwire_signal     |             |             |
 *    +------------------------------>------------>|------------>|
 *    |                              |             |             |
 *    |                              |             | HOST_RST    |  eSPI host
 *    |                              |             <-------------+  send VWire
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | App reset host-related       |             |             |
 *    | data structures              |             |             |
 *    |                              |             |             |
 *    |                              |             |   C10       |  eSPI host
 *    |                              |             +<------------+  send VWire
 *    |                              <-------------+             |
 *    <------------------------------+             |             |
 *    | App executes                 |             |             |
 *    + power mgmt policy            |             |             |
 * @endcode
 */

/**
 * @brief Helper to initialize an espi_callback structure properly.
 *
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param evt_type indicates the eSPI event relevant for the handler.
 * for VWIRE_RECEIVED event the data will indicate the new level asserted
 */
static inline void espi_init_callback(struct espi_callback *callback,
				      espi_callback_handler_t handler,
				      enum espi_bus_event evt_type)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->evt_type = evt_type;
}

/**
 * @brief Add an application callback.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * eSPI event is not specified.
 *
 * Note: enables to add as many callback as needed on the same device.
 */
static inline int espi_add_callback(const struct device *dev,
				    struct espi_callback *callback)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->manage_callback) {
		return -ENOTSUP;
	}

	return api->manage_callback(dev, callback, true);
}

/**
 * @brief Remove an application callback.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @warning It is explicitly permitted, within a callback handler, to
 * remove the registration for the callback that is running, i.e. @p
 * callback.  Attempts to remove other registrations on the same
 * device may result in undefined behavior, including failure to
 * invoke callbacks that remain registered and unintended invocation
 * of removed callbacks.
 *
 * Note: enables to remove as many callbacks as added through
 *       espi_add_callback().
 */
static inline int espi_remove_callback(const struct device *dev,
				       struct espi_callback *callback)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->manage_callback) {
		return -ENOTSUP;
	}

	return api->manage_callback(dev, callback, false);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <zephyr/syscalls/espi.h>
#endif /* ZEPHYR_INCLUDE_ESPI_H_ */

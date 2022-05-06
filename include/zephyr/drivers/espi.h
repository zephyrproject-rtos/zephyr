/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for eSPI driver
 */

#ifndef ZEPHYR_INCLUDE_ESPI_H_
#define ZEPHYR_INCLUDE_ESPI_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief eSPI Driver APIs
 * @defgroup espi_interface ESPI Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief eSPI I/O mode capabilities
 */
enum espi_io_mode {
	ESPI_IO_MODE_SINGLE_LINE = BIT(0),
	ESPI_IO_MODE_DUAL_LINES = BIT(1),
	ESPI_IO_MODE_QUAD_LINES = BIT(2),
};

/**
 *+----------------------------------------------------------------------+
 *|                                                                      |
 *|  eSPI host                           +-------------+                 |
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
 *|  eSPI slave                                                           |
 *|                                                                       |
 *|       CH0         |     CH1      |      CH2      |    CH3             |
 *|   eSPI endpoint   |    VWIRE     |      OOB      |   Flash            |
 *+-----------------------------------------------------------------------+
 *
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
	ESPI_CHANNEL_PERIPHERAL = BIT(0),
	ESPI_CHANNEL_VWIRE      = BIT(1),
	ESPI_CHANNEL_OOB        = BIT(2),
	ESPI_CHANNEL_FLASH      = BIT(3),
};

/**
 * @brief eSPI bus event.
 *
 * eSPI bus event to indicate events for which user can register callbacks
 */
enum espi_bus_event {
	ESPI_BUS_RESET                      = BIT(0),
	ESPI_BUS_EVENT_CHANNEL_READY        = BIT(1),
	ESPI_BUS_EVENT_VWIRE_RECEIVED       = BIT(2),
	ESPI_BUS_EVENT_OOB_RECEIVED         = BIT(3),
	ESPI_BUS_PERIPHERAL_NOTIFICATION    = BIT(4),
};

/**
 * @brief eSPI peripheral channel events.
 *
 * eSPI peripheral channel event types to indicate users.
 */
enum espi_pc_event {
	ESPI_PC_EVT_BUS_CHANNEL_READY = BIT(0),
	ESPI_PC_EVT_BUS_MASTER_ENABLE = BIT(1),
};

/**
 * @cond INTERNAL_HIDDEN
 *
 */
#define ESPI_PERIPHERAL_INDEX_0  0ul
#define ESPI_PERIPHERAL_INDEX_1  1ul
#define ESPI_PERIPHERAL_INDEX_2  2ul

#define ESPI_SLAVE_TO_MASTER     0ul
#define ESPI_MASTER_TO_SLAVE     1ul

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
	ESPI_PERIPHERAL_UART,
	ESPI_PERIPHERAL_8042_KBC,
	ESPI_PERIPHERAL_HOST_IO,
	ESPI_PERIPHERAL_DEBUG_PORT80,
	ESPI_PERIPHERAL_HOST_IO_PVT,
#if defined(CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD)
	ESPI_PERIPHERAL_EC_HOST_CMD,
#endif /* CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD */
};

/**
 * @brief eSPI cycle types supported over eSPI peripheral channel
 */
enum espi_cycle_type {
	ESPI_CYCLE_MEMORY_READ32,
	ESPI_CYCLE_MEMORY_READ64,
	ESPI_CYCLE_MEMORY_WRITE32,
	ESPI_CYCLE_MEMORY_WRITE64,
	ESPI_CYCLE_MESSAGE_NODATA,
	ESPI_CYCLE_MESSAGE_DATA,
	ESPI_CYCLE_OK_COMPLETION_NODATA,
	ESPI_CYCLE_OKCOMPLETION_DATA,
	ESPI_CYCLE_NOK_COMPLETION_NODATA,
};

/**
 * @brief eSPI system platform signals that can be send or receive through
 * virtual wire channel
 */
enum espi_vwire_signal {
	/* Virtual wires that can only be send from master to slave */
	ESPI_VWIRE_SIGNAL_SLP_S3,
	ESPI_VWIRE_SIGNAL_SLP_S4,
	ESPI_VWIRE_SIGNAL_SLP_S5,
	ESPI_VWIRE_SIGNAL_OOB_RST_WARN,
	ESPI_VWIRE_SIGNAL_PLTRST,
	ESPI_VWIRE_SIGNAL_SUS_STAT,
	ESPI_VWIRE_SIGNAL_NMIOUT,
	ESPI_VWIRE_SIGNAL_SMIOUT,
	ESPI_VWIRE_SIGNAL_HOST_RST_WARN,
	ESPI_VWIRE_SIGNAL_SLP_A,
	ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK,
	ESPI_VWIRE_SIGNAL_SUS_WARN,
	ESPI_VWIRE_SIGNAL_SLP_WLAN,
	ESPI_VWIRE_SIGNAL_SLP_LAN,
	ESPI_VWIRE_SIGNAL_HOST_C10,
	ESPI_VWIRE_SIGNAL_DNX_WARN,
	/* Virtual wires that can only be sent from slave to master */
	ESPI_VWIRE_SIGNAL_PME,
	ESPI_VWIRE_SIGNAL_WAKE,
	ESPI_VWIRE_SIGNAL_OOB_RST_ACK,
	ESPI_VWIRE_SIGNAL_SLV_BOOT_STS,
	ESPI_VWIRE_SIGNAL_ERR_NON_FATAL,
	ESPI_VWIRE_SIGNAL_ERR_FATAL,
	ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE,
	ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
	ESPI_VWIRE_SIGNAL_RST_CPU_INIT,
	/* System management interrupt */
	ESPI_VWIRE_SIGNAL_SMI,
	/* System control interrupt */
	ESPI_VWIRE_SIGNAL_SCI,
	ESPI_VWIRE_SIGNAL_DNX_ACK,
	ESPI_VWIRE_SIGNAL_SUS_ACK,
};

/* eSPI LPC peripherals. */
enum lpc_peripheral_opcode {
	/* Read transactions */
	E8042_OBF_HAS_CHAR = 0x50,
	E8042_IBF_HAS_CHAR,
	/* Write transactions */
	E8042_WRITE_KB_CHAR,
	E8042_WRITE_MB_CHAR,
	/* Write transactions without input parameters */
	E8042_RESUME_IRQ,
	E8042_PAUSE_IRQ,
	E8042_CLEAR_OBF,
	/* Status transactions */
	E8042_READ_KB_STS,
	E8042_SET_FLAG,
	E8042_CLEAR_FLAG,
	/* ACPI read transactions */
	EACPI_OBF_HAS_CHAR = EACPI_START_OPCODE,
	EACPI_IBF_HAS_CHAR,
	/* ACPI write transactions */
	EACPI_WRITE_CHAR,
	/* ACPI status transactions */
	EACPI_READ_STS,
	EACPI_WRITE_STS,
#if defined(CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION)
	/* Shared memory region support to return the ACPI response data */
	EACPI_GET_SHARED_MEMORY,
#endif /* CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION */
#if defined(CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE)
	/* Other customized transactions */
	ECUSTOM_HOST_SUBS_INTERRUPT_EN = ECUSTOM_START_OPCODE,
	ECUSTOM_HOST_CMD_GET_PARAM_MEMORY,
	ECUSTOM_HOST_CMD_SEND_RESULT,
#endif /* CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE */
};

/* KBC 8042 event: Input Buffer Full */
#define HOST_KBC_EVT_IBF BIT(0)
/* KBC 8042 event: Output Buffer Empty */
#define HOST_KBC_EVT_OBE BIT(1)
/**
 * @brief Bit field definition of evt_data in struct espi_event for KBC.
 */
struct espi_evt_data_kbc {
	uint32_t type:8;
	uint32_t data:8;
	uint32_t evt:8;
	uint32_t reserved:8;
};

/**
 * @brief Bit field definition of evt_data in struct espi_event for ACPI.
 */
struct espi_evt_data_acpi {
	uint32_t type:8;
	uint32_t data:8;
	uint32_t reserved:16;
};

/**
 * @brief eSPI event
 */
struct espi_event {
	/** Event type */
	enum espi_bus_event evt_type;
	/** Additional details for bus event type */
	uint32_t evt_details;
	/** Data associated to the event */
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
 * @brief eSPI peripheral request packet format
 */
struct espi_request_packet {
	enum espi_cycle_type cycle_type;
	uint8_t tag;
	uint16_t len;
	uint32_t address;
	uint8_t *data;
};

/**
 * @brief eSPI out-of-band transaction packet format
 */
struct espi_oob_packet {
	uint8_t *buf;
	uint16_t len;
};

/**
 * @brief eSPI flash transactions packet format
 */
struct espi_flash_packet {
	uint8_t *buf;
	uint32_t flash_addr;
	uint16_t len;
};

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
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct espi_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see espi_init_callback() below
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
 * If this eSPI controller is acting as slave, the values set here
 * will be discovered as part through the GET_CONFIGURATION command
 * issued by the eSPI master during initialization.
 *
 * If this eSPI controller is acting as master, the values set here
 * will be used by eSPI master to determine minimum common capabilities with
 * eSPI slave then send via SET_CONFIGURATION command.
 *
 * +--------+   +---------+     +------+          +---------+   +---------+
 * |  eSPI  |   |  eSPI   |     | eSPI |          |  eSPI   |   |  eSPI   |
 * |  slave |   | driver  |     |  bus |          |  driver |   |  host   |
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
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg the device runtime configuration for the eSPI controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by eSPI slave.
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
 * @brief Query to see if it a channel is ready.
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
 * This routines provides a generic interface to send a read request packet.
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
 * This routines provides a generic interface to send a write request packet.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param req Address of structure representing a memory, I/O or
 *            message write request.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP if eSPI controller doesn't support raw packets and instead
 *         low memory transactions are handled by controller hardware directly.
 * @retval -EINVAL General input / output error, failed to send over the bus.
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
 * This routines provides a generic interface to send a virtual wire packet
 * from slave to master.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param signal The signal to be send to eSPI master.
 * @param level The level of signal requested LOW or HIGH.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to send over the bus.
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
 * This routines provides a generic interface to request a virtual wire packet
 * from eSPI master and retrieve the signal level.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param signal the signal to be requested from eSPI master.
 * @param level the level of signal requested 0b LOW, 1b HIGH.
 *
 * @retval -EIO General input / output error, failed request to master.
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
 * This routines provides an interface to encapsulate a SMBus transaction
 * and send into packet over eSPI bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the packet representation of SMBus transaction.
 *
 * @retval -EIO General input / output error, failed request to master.
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
 * This routines provides an interface to receive and decoded a SMBus
 * transaction from eSPI bus
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the packet representation of SMBus transaction.
 *
 * @retval -EIO General input / output error, failed request to master.
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
 * This routines provides an interface to send a request to read the flash
 * component shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of read flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
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
 * This routines provides an interface to send a request to write to the flash
 * components shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of write flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
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
 * This routines provides an interface to send a request to write to the flash
 * components shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of write flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
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
 *    |                              |             |             |
 *    |                              | Processed   |             |
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
 */

/**
 * @brief Helper to initialize a struct espi_callback properly.
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
#include <syscalls/espi.h>
#endif /* ZEPHYR_INCLUDE_ESPI_H_ */

/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SMBUS_H_
#define ZEPHYR_INCLUDE_DRIVERS_SMBUS_H_

/**
 * @brief SMBus Interface
 * @defgroup smbus_interface SMBus Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SMBus Protocol commands
 * @{
 *
 * SMBus Specification defines the following SMBus protocols operations
 */

/**
 * SMBus Quick protocol is a very simple command with no data sent or
 * received. Peripheral may denote only R/W bit, which can still be
 * used for the peripheral management, for example to switch peripheral
 * On/Off. Quick protocol can also be used for peripheral devices
 * scanning.
 *
 * @code
 * 0                   1
 * 0 1 2 3 4 5 6 7 8 9 0
 * +-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |D|A|P|
 * +-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_QUICK			0b000

/**
 * SMBus Byte protocol can send or receive one byte of data.
 *
 * @code
 * Byte Write
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A| Command code  |A|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Byte Read
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |R|A| Byte received |N|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_BYTE			0b001

/**
 * SMBus Byte Data protocol sends the first byte (command) followed
 * by read or write one byte.
 *
 * @code
 * Byte Data Write
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A|   Data Write  |A|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Byte Data Read
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A|S| Periph Addr |R|A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Data Read   |N|P|
 * +-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_BYTE_DATA		0b010

/**
 * SMBus Word Data protocol sends the first byte (command) followed
 * by read or write two bytes.
 *
 * @code
 * Word Data Write
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A| Data Write Low|A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data Write Hi |A|P|
 * +-+-+-+-+-+-+-+-+-+-+
 *
 * Word Data Read
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A|S| Periph Addr |R|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |A| Data Read Low |A|  Data Read Hi |N|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_WORD_DATA		0b011

/**
 * SMBus Process Call protocol is Write Word followed by
 * Read Word. It is named so because the command sends data and waits
 * for the peripheral to return a reply.
 *
 * @code
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A| Data Write Low|A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data Write Hi |A|S| Periph Addr |R|A| Data Read Low |A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data Read Hi  |N|P|
 * +-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_PROC_CALL		0b100

/**
 * SMBus Block protocol reads or writes a block of data up to 32 bytes.
 * The Count byte specifies the amount of data.
 *
 * @code
 *
 * SMBus Block Write
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A| Send Count=N  |A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Data Write 1 |A|      ...      |A|  Data Write N |A|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * SMBus Block Read
 *
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A|S| Periph Addr |R|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |A|  Recv Count=N |A|  Data Read 1  |A|      ...      |A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Data Read N  |N|P|
 * +-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_BLOCK			0b101

/**
 * SMBus Block Write - Block Read Process Call protocol is
 * Block Write followed by Block Read.
 *
 * @code
 * 0                   1                   2
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S| Periph Addr |W|A|  Command code |A|   Count = N   |A|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Data Write 1 |A|      ...      |A|  Data Write N |A|S|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Periph Addr |R|A|  Recv Count=N |A|  Data Read 1  |A| |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     ...     |A|  Data Read N  |N|P|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */
#define SMBUS_CMD_BLOCK_PROC		0b111
/** @} */

/** Maximum number of bytes in SMBus Block protocol */
#define SMBUS_BLOCK_BYTES_MAX		32

/**
 * @name SMBus device functionality
 * @{
 *
 * The following parameters describe the functionality of the SMBus device
 */

/** Peripheral to act as Controller. */
#define SMBUS_MODE_CONTROLLER		BIT(0)

/** Support Packet Error Code (PEC) checking */
#define SMBUS_MODE_PEC			BIT(1)

/** Support Host Notify functionality */
#define SMBUS_MODE_HOST_NOTIFY		BIT(2)

/** Support SMBALERT signal functionality */
#define SMBUS_MODE_SMBALERT		BIT(3)

/** @} */

/**
 * @name SMBus special reserved addresses
 * @{
 *
 * The following addresses are reserved by SMBus specification
 */

/**
 * @brief Alert Response Address (ARA)
 *
 * A broadcast address used by the system host as part of the
 * Alert Response Protocol.
 */
#define SMBUS_ADDRESS_ARA		0x0c

/** @} */

/**
 * @name SMBus read / write direction
 * @{
 */

/** @brief SMBus read / write direction */
enum smbus_direction {
	/** Write a message to SMBus peripheral */
	SMBUS_MSG_WRITE = 0,
	/** Read a message from SMBus peripheral */
	SMBUS_MSG_READ = 1,
};

/** @} */

/** @cond INTERNAL_HIDDEN */
#define SMBUS_MSG_RW_MASK		BIT(0)
/** @endcond  */

struct smbus_callback;

/**
 * @brief Define SMBus callback handler function signature.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param cb Structure smbus_callback owning this handler.
 * @param addr Address of the SMBus peripheral device.
 */
typedef void (*smbus_callback_handler_t)(const struct device *dev,
					 struct smbus_callback *cb,
					 uint8_t addr);

/**
 * @brief SMBus callback structure
 *
 * Used to register a callback in the driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * is a unique pointer of struct smbus_callback.
 *
 * Note: Such struct should not be allocated on stack.
 */
struct smbus_callback {
	/** This should be used in driver for a callback list management */
	sys_snode_t node;

	/** Actual callback function being called when relevant */
	smbus_callback_handler_t handler;

	/** Peripheral device address */
	uint8_t addr;
};

/**
 * @brief Complete SMBus DT information
 */
struct smbus_dt_spec {
	/** SMBus bus */
	const struct device *bus;
	/** Address of the SMBus peripheral device */
	uint16_t addr;
};

/**
 * @brief Structure initializer for smbus_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * smbus_dt_spec</tt> by reading the relevant bus and address data from
 * the devicetree.
 *
 * @param node_id Devicetree node identifier for the SMBus device whose
 *                struct smbus_dt_spec to create an initializer for
 */
#define SMBUS_DT_SPEC_GET(node_id)				\
	{							\
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),		\
		.addr = DT_REG_ADDR(node_id)			\
	}

/**
 * @brief Structure initializer for smbus_dt_spec from devicetree instance
 *
 * This is equivalent to
 * <tt>SMBUS_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 */
#define SMBUS_DT_SPEC_INST_GET(inst) SMBUS_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */

typedef int (*smbus_api_configure_t)(const struct device *dev,
				     uint32_t dev_config);
typedef int (*smbus_api_get_config_t)(const struct device *dev,
				      uint32_t *dev_config);
typedef int (*smbus_api_quick_t)(const struct device *dev,
				 uint16_t addr, enum smbus_direction);
typedef int (*smbus_api_byte_write_t)(const struct device *dev,
				      uint16_t addr, uint8_t byte);
typedef int (*smbus_api_byte_read_t)(const struct device *dev,
				     uint16_t addr, uint8_t *byte);
typedef int (*smbus_api_byte_data_write_t)(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t byte);
typedef int (*smbus_api_byte_data_read_t)(const struct device *dev,
					  uint16_t addr, uint8_t cmd,
					  uint8_t *byte);
typedef int (*smbus_api_word_data_write_t)(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint16_t word);
typedef int (*smbus_api_word_data_read_t)(const struct device *dev,
					  uint16_t addr, uint8_t cmd,
					  uint16_t *word);
typedef int (*smbus_api_pcall_t)(const struct device *dev,
				 uint16_t addr, uint8_t cmd,
				 uint16_t send_word, uint16_t *recv_word);
typedef int (*smbus_api_block_write_t)(const struct device *dev,
				       uint16_t addr, uint8_t cmd,
				       uint8_t count, uint8_t *buf);
typedef int (*smbus_api_block_read_t)(const struct device *dev,
				      uint16_t addr, uint8_t cmd,
				      uint8_t *count, uint8_t *buf);
typedef int (*smbus_api_block_pcall_t)(const struct device *dev,
				       uint16_t addr, uint8_t cmd,
				       uint8_t send_count, uint8_t *send_buf,
				       uint8_t *recv_count, uint8_t *recv_buf);
typedef int (*smbus_api_smbalert_cb_t)(const struct device *dev,
				       struct smbus_callback *cb);
typedef int (*smbus_api_host_notify_cb_t)(const struct device *dev,
					  struct smbus_callback *cb);

__subsystem struct smbus_driver_api {
	smbus_api_configure_t configure;
	smbus_api_get_config_t get_config;
	smbus_api_quick_t smbus_quick;
	smbus_api_byte_write_t smbus_byte_write;
	smbus_api_byte_read_t smbus_byte_read;
	smbus_api_byte_data_write_t smbus_byte_data_write;
	smbus_api_byte_data_read_t smbus_byte_data_read;
	smbus_api_word_data_write_t smbus_word_data_write;
	smbus_api_word_data_read_t smbus_word_data_read;
	smbus_api_pcall_t smbus_pcall;
	smbus_api_block_write_t smbus_block_write;
	smbus_api_block_read_t smbus_block_read;
	smbus_api_block_pcall_t smbus_block_pcall;
	smbus_api_smbalert_cb_t smbus_smbalert_set_cb;
	smbus_api_smbalert_cb_t smbus_smbalert_remove_cb;
	smbus_api_host_notify_cb_t smbus_host_notify_set_cb;
	smbus_api_host_notify_cb_t smbus_host_notify_remove_cb;
};

/**
 * @endcond
 */

#if defined(CONFIG_SMBUS_STATS) || defined(__DOXYGEN__)

#include <zephyr/stats/stats.h>

/** @cond INTERNAL_HIDDEN */

STATS_SECT_START(smbus)
STATS_SECT_ENTRY32(bytes_read)
STATS_SECT_ENTRY32(bytes_written)
STATS_SECT_ENTRY32(command_count)
STATS_SECT_END;

STATS_NAME_START(smbus)
STATS_NAME(smbus, bytes_read)
STATS_NAME(smbus, bytes_written)
STATS_NAME(smbus, command_count)
STATS_NAME_END(smbus);

struct smbus_device_state {
	struct device_state devstate;
	struct stats_smbus stats;
};

/**
 * @brief Define a statically allocated and section assigned smbus device state
 */
#define Z_SMBUS_DEVICE_STATE_DEFINE(node_id, dev_name)			\
	static struct smbus_device_state Z_DEVICE_STATE_NAME(dev_name)	\
	__attribute__((__section__(".z_devstate")));

/**
 * @brief Define an smbus device init wrapper function
 *
 * This does device instance specific initialization of common data
 * (such as stats) and calls the given init_fn
 */
#define Z_SMBUS_INIT_FN(dev_name, init_fn)				\
	static inline int						\
		UTIL_CAT(dev_name, _init)(const struct device *dev)	\
	{								\
		struct smbus_device_state *state =			\
			CONTAINER_OF(dev->state,			\
				     struct smbus_device_state,		\
				     devstate);				\
		stats_init(&state->stats.s_hdr, STATS_SIZE_32, 4,	\
			   STATS_NAME_INIT_PARMS(smbus));		\
		stats_register(dev->name, &(state->stats.s_hdr));	\
		return init_fn(dev);					\
	}

/** @endcond */

/**
 * @brief Updates the SMBus stats
 *
 * @param dev Pointer to the device structure for the SMBus driver instance
 * to update stats for.
 * @param sent Number of bytes sent
 * @param recv Number of bytes received
 */
static inline void smbus_xfer_stats(const struct device *dev, uint8_t sent,
				    uint8_t recv)
{
	struct smbus_device_state *state =
		CONTAINER_OF(dev->state, struct smbus_device_state, devstate);

	STATS_INC(state->stats, command_count);
	STATS_INCN(state->stats, bytes_read, recv);
	STATS_INCN(state->stats, bytes_written, sent);
}

/**
 * @brief Like DEVICE_DT_DEFINE() with SMBus specifics.
 *
 * @details Defines a device which implements the SMBus API. May
 * generate a custom device_state container struct and init_fn
 * wrapper when needed depending on SMBus @kconfig{CONFIG_SMBUS_STATS}.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Name of the init function of the driver.
 *
 * @param pm_device PM device resources reference
 * (NULL if device does not use PM).
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for
 * details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct
 * used by the driver. Can be NULL.
 */
#define SMBUS_DEVICE_DT_DEFINE(node_id, init_fn, pm_device,		\
			       data_ptr, cfg_ptr, level, prio,		\
			       api_ptr, ...)				\
	Z_SMBUS_DEVICE_STATE_DEFINE(node_id,				\
				    Z_DEVICE_DT_DEV_NAME(node_id));	\
	Z_SMBUS_INIT_FN(Z_DEVICE_DT_DEV_NAME(node_id), init_fn)		\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_NAME(node_id),		\
			DEVICE_DT_NAME(node_id),			\
			&UTIL_CAT(Z_DEVICE_DT_DEV_NAME(node_id), _init),\
			pm_device,					\
			data_ptr, cfg_ptr, level, prio,			\
			api_ptr,					\
			&(Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_NAME	\
					      (node_id)).devstate),	\
			__VA_ARGS__)

#else /* CONFIG_SMBUS_STATS */

static inline void smbus_xfer_stats(const struct device *dev, uint8_t sent,
				    uint8_t recv)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sent);
	ARG_UNUSED(recv);
}

#define SMBUS_DEVICE_DT_DEFINE(node_id, init_fn, pm_device,		\
			     data_ptr, cfg_ptr, level, prio,		\
			     api_ptr, ...)				\
	DEVICE_DT_DEFINE(node_id, &init_fn, pm_device,			\
			     data_ptr, cfg_ptr, level, prio,		\
			     api_ptr, __VA_ARGS__)

#endif /* CONFIG_SMBUS_STATS */

/**
 * @brief Like SMBUS_DEVICE_DT_DEFINE() for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to SMBUS_DEVICE_DT_DEFINE().
 *
 * @param ... other parameters as expected by SMBUS_DEVICE_DT_DEFINE().
 */
#define SMBUS_DEVICE_DT_INST_DEFINE(inst, ...)		\
	SMBUS_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Configure operation of a SMBus host controller.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 * for the SMBus controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
__syscall int smbus_configure(const struct device *dev, uint32_t dev_config);

static inline int z_impl_smbus_configure(const struct device *dev,
					 uint32_t dev_config)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	return api->configure(dev, dev_config);
}

/**
 * @brief Get configuration of a SMBus host controller.
 *
 * This routine provides a way to get current configuration. It is allowed to
 * call the function before smbus_configure, because some SMBus ports can be
 * configured during init process. However, if the SMBus port is not configured,
 * smbus_get_config returns an error.
 *
 * smbus_get_config can return cached config or probe hardware, but it has to be
 * up to date with current configuration.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param dev_config Pointer to return bit-packed 32-bit value of
 * the SMBus controller configuration.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_get_config() is not implemented
 * by the driver.
 */
__syscall int smbus_get_config(const struct device *dev, uint32_t *dev_config);

static inline int z_impl_smbus_get_config(const struct device *dev,
					  uint32_t *dev_config)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->get_config == NULL) {
		return -ENOSYS;
	}

	return api->get_config(dev, dev_config);
}

/**
 * @brief Add SMBUSALERT callback for a SMBus host controller.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param cb Pointer to a callback structure.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_smbalert_set_cb() is not implemented
 * by the driver.
 */
__syscall int smbus_smbalert_set_cb(const struct device *dev,
				    struct smbus_callback *cb);

static inline int z_impl_smbus_smbalert_set_cb(const struct device *dev,
					       struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_smbalert_set_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_smbalert_set_cb(dev, cb);
}

/**
 * @brief Remove SMBUSALERT callback from a SMBus host controller.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param cb Pointer to a callback structure.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_smbalert_remove_cb() is not implemented
 * by the driver.
 */
__syscall int smbus_smbalert_remove_cb(const struct device *dev,
				       struct smbus_callback *cb);

static inline int z_impl_smbus_smbalert_remove_cb(const struct device *dev,
						  struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_smbalert_remove_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_smbalert_remove_cb(dev, cb);
}

/**
 * @brief Add Host Notify callback for a SMBus host controller.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param cb Pointer to a callback structure.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_host_notify_set_cb() is not implemented
 * by the driver.
 */
__syscall int smbus_host_notify_set_cb(const struct device *dev,
				       struct smbus_callback *cb);

static inline int z_impl_smbus_host_notify_set_cb(const struct device *dev,
						  struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_host_notify_set_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_host_notify_set_cb(dev, cb);
}

/**
 * @brief Remove Host Notify callback from a SMBus host controller.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param cb Pointer to a callback structure.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_host_notify_remove_cb() is not implemented
 * by the driver.
 */
__syscall int smbus_host_notify_remove_cb(const struct device *dev,
					  struct smbus_callback *cb);

static inline int z_impl_smbus_host_notify_remove_cb(const struct device *dev,
						     struct smbus_callback *cb)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_host_notify_remove_cb == NULL) {
		return -ENOSYS;
	}

	return api->smbus_host_notify_remove_cb(dev, cb);
}

/**
 * @brief Perform SMBus Quick operation
 *
 * This routine provides a generic interface to perform SMBus Quick
 * operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * driver configured in controller mode.
 * @param addr Address of the SMBus peripheral device.
 * @param direction Direction Read or Write.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_quick() is not implemented
 * by the driver.
 */
__syscall int smbus_quick(const struct device *dev, uint16_t addr,
			  enum smbus_direction direction);

static inline int z_impl_smbus_quick(const struct device *dev, uint16_t addr,
				     enum smbus_direction direction)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_quick == NULL) {
		return -ENOSYS;
	}

	if (direction != SMBUS_MSG_READ && direction != SMBUS_MSG_WRITE) {
		return -EINVAL;
	}

	return  api->smbus_quick(dev, addr, direction);
}

/**
 * @brief Perform SMBus Byte Write operation
 *
 * This routine provides a generic interface to perform SMBus
 * Byte Write operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param byte Byte to be sent to the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_byte_write() is not implemented
 * by the driver.
 */
__syscall int smbus_byte_write(const struct device *dev, uint16_t addr,
			       uint8_t byte);

static inline int z_impl_smbus_byte_write(const struct device *dev,
					  uint16_t addr, uint8_t byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_write(dev, addr, byte);
}

/**
 * @brief Perform SMBus Byte Read operation
 *
 * This routine provides a generic interface to perform SMBus
 * Byte Read operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param byte Byte received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_byte_read() is not implemented
 * by the driver.
 */
__syscall int smbus_byte_read(const struct device *dev, uint16_t addr,
			      uint8_t *byte);

static inline int z_impl_smbus_byte_read(const struct device *dev,
					 uint16_t addr, uint8_t *byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_read(dev, addr, byte);
}

/**
 * @brief Perform SMBus Byte Data Write operation
 *
 * This routine provides a generic interface to perform SMBus
 * Byte Data Write operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param byte Byte to be sent to the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_byte_data_write() is not implemented
 * by the driver.
 */
__syscall int smbus_byte_data_write(const struct device *dev, uint16_t addr,
				    uint8_t cmd, uint8_t byte);

static inline int z_impl_smbus_byte_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint8_t byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_data_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_data_write(dev, addr, cmd, byte);
}

/**
 * @brief Perform SMBus Byte Data Read operation
 *
 * This routine provides a generic interface to perform SMBus
 * Byte Data Read operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param byte Byte received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_byte_data_read() is not implemented
 * by the driver.
 */
__syscall int smbus_byte_data_read(const struct device *dev, uint16_t addr,
				   uint8_t cmd, uint8_t *byte);

static inline int z_impl_smbus_byte_data_read(const struct device *dev,
					      uint16_t addr, uint8_t cmd,
					      uint8_t *byte)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_byte_data_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_byte_data_read(dev, addr, cmd, byte);
}

/**
 * @brief Perform SMBus Word Data Write operation
 *
 * This routine provides a generic interface to perform SMBus
 * Word Data Write operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param word Word (16-bit) to be sent to the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_word_data_write() is not implemented
 * by the driver.
 */
__syscall int smbus_word_data_write(const struct device *dev, uint16_t addr,
				    uint8_t cmd, uint16_t word);

static inline int z_impl_smbus_word_data_write(const struct device *dev,
					       uint16_t addr, uint8_t cmd,
					       uint16_t word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_word_data_write == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_word_data_write(dev, addr, cmd, word);
}

/**
 * @brief Perform SMBus Word Data Read operation
 *
 * This routine provides a generic interface to perform SMBus
 * Word Data Read operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param word Word (16-bit) received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_word_data_read() is not implemented
 * by the driver.
 */
__syscall int smbus_word_data_read(const struct device *dev, uint16_t addr,
				   uint8_t cmd, uint16_t *word);

static inline int z_impl_smbus_word_data_read(const struct device *dev,
					      uint16_t addr, uint8_t cmd,
					      uint16_t *word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_word_data_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_word_data_read(dev, addr, cmd, word);
}

/**
 * @brief Perform SMBus Process Call operation
 *
 * This routine provides a generic interface to perform SMBus
 * Process Call operation, which means Write 2 bytes following by
 * Read 2 bytes.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param send_word Word (16-bit) to be sent to the peripheral device.
 * @param recv_word Word (16-bit) received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_pcall() is not implemented
 * by the driver.
 */
__syscall int smbus_pcall(const struct device *dev, uint16_t addr,
			  uint8_t cmd, uint16_t send_word, uint16_t *recv_word);

static inline int z_impl_smbus_pcall(const struct device *dev,
				     uint16_t addr, uint8_t cmd,
				     uint16_t send_word, uint16_t *recv_word)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_pcall == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_pcall(dev, addr, cmd, send_word, recv_word);
}

/**
 * @brief Perform SMBus Block Write operation
 *
 * This routine provides a generic interface to perform SMBus
 * Block Write operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param count Size of the data block buffer. Maximum 32 bytes.
 * @param buf Data block buffer to be sent to the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_block_write() is not implemented
 * by the driver.
 */
__syscall int smbus_block_write(const struct device *dev, uint16_t addr,
				uint8_t cmd, uint8_t count, uint8_t *buf);

static inline int z_impl_smbus_block_write(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t count, uint8_t *buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_write == NULL) {
		return -ENOSYS;
	}

	if (count < 1 || count > SMBUS_BLOCK_BYTES_MAX) {
		return -EINVAL;
	}

	return  api->smbus_block_write(dev, addr, cmd, count, buf);
}

/**
 * @brief Perform SMBus Block Read operation
 *
 * This routine provides a generic interface to perform SMBus
 * Block Read operation.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param count Size of the data peripheral sent. Maximum 32 bytes.
 * @param buf Data block buffer received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_block_read() is not implemented
 * by the driver.
 */
__syscall int smbus_block_read(const struct device *dev, uint16_t addr,
			       uint8_t cmd, uint8_t *count, uint8_t *buf);

static inline int z_impl_smbus_block_read(const struct device *dev,
					  uint16_t addr, uint8_t cmd,
					  uint8_t *count, uint8_t *buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_read == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_block_read(dev, addr, cmd, count, buf);
}

/**
 * @brief Perform SMBus Block Process Call operation
 *
 * This routine provides a generic interface to perform SMBus
 * Block Process Call operation. This operation is basically
 * Block Write followed by Block Read.
 *
 * @param dev Pointer to the device structure for the SMBus driver instance.
 * @param addr Address of the SMBus peripheral device.
 * @param cmd Command byte which is sent to peripheral device first.
 * @param snd_count Size of the data block buffer to send.
 * @param snd_buf Data block buffer send to the peripheral device.
 * @param rcv_count Size of the data peripheral sent.
 * @param rcv_buf Data block buffer received from the peripheral device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If function smbus_block_pcall() is not implemented
 * by the driver.
 */
__syscall int smbus_block_pcall(const struct device *dev,
				uint16_t addr, uint8_t cmd,
				uint8_t snd_count, uint8_t *snd_buf,
				uint8_t *rcv_count, uint8_t *rcv_buf);

static inline int z_impl_smbus_block_pcall(const struct device *dev,
					   uint16_t addr, uint8_t cmd,
					   uint8_t snd_count, uint8_t *snd_buf,
					   uint8_t *rcv_count, uint8_t *rcv_buf)
{
	const struct smbus_driver_api *api =
		(const struct smbus_driver_api *)dev->api;

	if (api->smbus_block_pcall == NULL) {
		return -ENOSYS;
	}

	return  api->smbus_block_pcall(dev, addr, cmd, snd_count, snd_buf,
				       rcv_count, rcv_buf);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/smbus.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SMBUS_H_ */

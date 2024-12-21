/*
 * Copyright 2022 Intel Corporation
 * Copyright 2023 Meta Platforms, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_H_

/**
 * @brief I3C Interface
 * @defgroup i3c_interface I3C Interface
 * @since 3.2
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i3c/addresses.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/i3c/devicetree.h>
#include <zephyr/drivers/i3c/ibi.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Bus Characteristic Register (BCR)
 * @anchor I3C_BCR
 *
 * - BCR[7:6]: Device Role
 *   - 0: I3C Target
 *   - 1: I3C Controller capable
 *   - 2: Reserved
 *   - 3: Reserved
 *   .
 * - BCR[5]: Advanced Capabilities
 *   - 0: Does not support optional advanced capabilities.
 *   - 1: Supports optional advanced capabilities which
 *        can be viewed via GETCAPS CCC.
 *   .
 * - BCR[4]: Virtual Target Support
 *   - 0: Is not a virtual target.
 *   - 1: Is a virtual target.
 *   .
 * - BCR[3]: Offline Capable
 *   - 0: Will always response to I3C commands.
 *   - 1: Will not always response to I3C commands.
 *   .
 * - BCR[2]: IBI Payload
 *   - 0: No data bytes following the accepted IBI.
 *   - 1: One data byte (MDB, Mandatory Data Byte) follows
 *        the accepted IBI. Additional data bytes may also
 *        follows.
 *   .
 * - BCR[1]: IBI Request Capable
 *   - 0: Not capable
 *   - 1: Capable
 *   .
 * - BCR[0]: Max Data Speed Limitation
 *   - 0: No Limitation
 *   - 1: Limitation obtained via GETMXDS CCC.
 *   .
 *
 * @{
 */

/**
 * @brief Max Data Speed Limitation bit.
 *
 * 0 - No Limitation.
 * 1 - Limitation obtained via GETMXDS CCC.
 */
#define I3C_BCR_MAX_DATA_SPEED_LIMIT			BIT(0)

/** @brief IBI Request Capable bit. */
#define I3C_BCR_IBI_REQUEST_CAPABLE			BIT(1)

/**
 * @brief IBI Payload bit.
 *
 * 0 - No data bytes following the accepted IBI.
 * 1 - One data byte (MDB, Mandatory Data Byte) follows the accepted IBI.
 *     Additional data bytes may also follows.
 */
#define I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE		BIT(2)

/**
 * @brief Offline Capable bit.
 *
 * 0 - Will always respond to I3C commands.
 * 1 - Will not always respond to I3C commands.
 */
#define I3C_BCR_OFFLINE_CAPABLE				BIT(3)

/**
 * @brief Virtual Target Support bit.
 *
 * 0 - Is not a virtual target.
 * 1 - Is a virtual target.
 */
#define I3C_BCR_VIRTUAL_TARGET				BIT(4)

/**
 * @brief Advanced Capabilities bit.
 *
 * 0 - Does not support optional advanced capabilities.
 * 1 - Supports optional advanced capabilities which can be viewed via
 *     GETCAPS CCC.
 */
#define I3C_BCR_ADV_CAPABILITIES			BIT(5)

/** Device Role - I3C Target. */
#define I3C_BCR_DEVICE_ROLE_I3C_TARGET			0U

/** Device Role - I3C Controller Capable. */
#define I3C_BCR_DEVICE_ROLE_I3C_CONTROLLER_CAPABLE	1U

/** Device Role bit shift mask. */
#define I3C_BCR_DEVICE_ROLE_MASK			GENMASK(7U, 6U)

/**
 * @brief Device Role
 *
 * Obtain Device Role value from the BCR value obtained via GETBCR.
 *
 * @param bcr BCR value
 */
#define I3C_BCR_DEVICE_ROLE(bcr)			\
	FIELD_GET(I3C_BCR_DEVICE_ROLE_MASK, (bcr))

/** @} */

/**
 * @name Legacy Virtual Register (LVR)
 * @anchor I3C_LVR
 *
 * Legacy Virtual Register (LVR)
 * - LVR[7:5]: I2C device index:
 *   - 0: I2C device has a 50 ns spike filter where
 *        it is not affected by high frequency on SCL.
 *   - 1: I2C device does not have a 50 ns spike filter
 *        but can work with high frequency on SCL.
 *   - 2: I2C device does not have a 50 ns spike filter
 *        and cannot work with high frequency on SCL.
 * - LVR[4]: I2C mode indicator:
 *   - 0: FM+ mode
 *   - 1: FM mode
 * - LVR[3:0]: Reserved.
 *
 * @{
 */

/** I2C FM+ Mode. */
#define I3C_LVR_I2C_FM_PLUS_MODE			0

/** I2C FM Mode. */
#define I3C_LVR_I2C_FM_MODE				1

/** I2C Mode Indicator bitmask. */
#define I3C_LVR_I2C_MODE_MASK				BIT(4)

/**
 * @brief I2C Mode
 *
 * Obtain I2C Mode value from the LVR value.
 *
 * @param lvr LVR value
 */
#define I3C_LVR_I2C_MODE(lvr)				\
	FIELD_GET(I3C_LVR_I2C_MODE_MASK, (lvr))

/**
 * @brief I2C Device Index 0.
 *
 * I2C device has a 50 ns spike filter where it is not affected by high
 * frequency on SCL.
 */
#define I3C_LVR_I2C_DEV_IDX_0				0

/**
 * @brief I2C Device Index 1.
 *
 * I2C device does not have a 50 ns spike filter but can work with high
 * frequency on SCL.
 */
#define I3C_LVR_I2C_DEV_IDX_1				1

/**
 * @brief I2C Device Index 2.
 *
 * I2C device does not have a 50 ns spike filter and cannot work with high
 * frequency on SCL.
 */
#define I3C_LVR_I2C_DEV_IDX_2				2

/** I2C Device Index bitmask. */
#define I3C_LVR_I2C_DEV_IDX_MASK			GENMASK(7U, 5U)

/**
 * @brief I2C Device Index
 *
 * Obtain I2C Device Index value from the LVR value.
 *
 * @param lvr LVR value
 */
#define I3C_LVR_I2C_DEV_IDX(lvr)			\
	FIELD_GET(I3C_LVR_I2C_DEV_IDX_MASK, (lvr))

/** @} */

/**
 * @brief I3C bus mode
 */
enum i3c_bus_mode {
	/** Only I3C devices are on the bus. */
	I3C_BUS_MODE_PURE,

	/**
	 * Both I3C and legacy I2C devices are on the bus.
	 * The I2C devices have 50ns spike filter on SCL.
	 */
	I3C_BUS_MODE_MIXED_FAST,

	/**
	 * Both I3C and legacy I2C devices are on the bus.
	 * The I2C devices do not have 50ns spike filter on SCL
	 * and can tolerate maximum SDR SCL clock frequency.
	 */
	I3C_BUS_MODE_MIXED_LIMITED,

	/**
	 * Both I3C and legacy I2C devices are on the bus.
	 * The I2C devices do not have 50ns spike filter on SCL
	 * but cannot tolerate maximum SDR SCL clock frequency.
	 */
	I3C_BUS_MODE_MIXED_SLOW,

	I3C_BUS_MODE_MAX = I3C_BUS_MODE_MIXED_SLOW,
	I3C_BUS_MODE_INVALID,
};

/**
 * @brief I2C bus speed under I3C bus.
 *
 * Only FM and FM+ modes are supported for I2C devices under I3C bus.
 */
enum i3c_i2c_speed_type {
	/** I2C FM mode */
	I3C_I2C_SPEED_FM,

	/** I2C FM+ mode */
	I3C_I2C_SPEED_FMPLUS,

	I3C_I2C_SPEED_MAX = I3C_I2C_SPEED_FMPLUS,
	I3C_I2C_SPEED_INVALID,
};

/**
 * @brief I3C data rate
 *
 * I3C data transfer rate defined by the I3C specification.
 */
enum i3c_data_rate {
	/** Single Data Rate messaging */
	I3C_DATA_RATE_SDR,

	/** High Data Rate - Double Data Rate messaging */
	I3C_DATA_RATE_HDR_DDR,

	/** High Data Rate - Ternary Symbol Legacy-inclusive-Bus */
	I3C_DATA_RATE_HDR_TSL,

	/** High Data Rate - Ternary Symbol for Pure Bus */
	I3C_DATA_RATE_HDR_TSP,

	/** High Data Rate - Bulk Transport */
	I3C_DATA_RATE_HDR_BT,

	I3C_DATA_RATE_MAX = I3C_DATA_RATE_HDR_BT,
	I3C_DATA_RATE_INVALID,
};

/**
 * @brief I3C SDR Controller Error Codes
 *
 * These are error codes defined by the I3C specification.
 *
 * #I3C_ERROR_CE_UNKNOWN and #I3C_ERROR_CE_NONE are not
 * official error codes according to the specification.
 * These are there simply to aid in error handling during
 * interactions with the I3C drivers and subsystem.
 */
enum i3c_sdr_controller_error_codes {
	/** Transaction after sending CCC */
	I3C_ERROR_CE0,

	/** Monitoring Error */
	I3C_ERROR_CE1,

	/** No response to broadcast address (0x7E) */
	I3C_ERROR_CE2,

	/** Failed Controller Handoff */
	I3C_ERROR_CE3,

	/** Unknown error (not official error code) */
	I3C_ERROR_CE_UNKNOWN,

	/** No error (not official error code) */
	I3C_ERROR_CE_NONE,

	I3C_ERROR_CE_MAX = I3C_ERROR_CE_UNKNOWN,
	I3C_ERROR_CE_INVALID,
};

/**
 * @brief I3C SDR Target Error Codes
 *
 * These are error codes defined by the I3C specification.
 *
 * #I3C_ERROR_TE_UNKNOWN and #I3C_ERROR_TE_NONE are not
 * official error codes according to the specification.
 * These are there simply to aid in error handling during
 * interactions with the I3C drivers and subsystem.
 */
enum i3c_sdr_target_error_codes {
	/**
	 * Invalid Broadcast Address or
	 * Dynamic Address after DA assignment
	 */
	I3C_ERROR_TE0,

	/** CCC Code */
	I3C_ERROR_TE1,

	/** Write Data */
	I3C_ERROR_TE2,

	/** Assigned Address during Dynamic Address Arbitration */
	I3C_ERROR_TE3,

	/** 0x7E/R missing after RESTART during Dynamic Address Arbitration */
	I3C_ERROR_TE4,

	/** Transaction after detecting CCC */
	I3C_ERROR_TE5,

	/** Monitoring Error */
	I3C_ERROR_TE6,

	/** Dead Bus Recovery */
	I3C_ERROR_DBR,

	/** Unknown error (not official error code) */
	I3C_ERROR_TE_UNKNOWN,

	/** No error (not official error code) */
	I3C_ERROR_TE_NONE,

	I3C_ERROR_TE_MAX = I3C_ERROR_TE_UNKNOWN,
	I3C_ERROR_TE_INVALID,
};

/**
 * @brief I3C Transfer API
 * @defgroup i3c_transfer_api I3C Transfer API
 * @{
 */

/*
 * I3C_MSG_* are I3C Message flags.
 */

/** Write message to I3C bus. */
#define I3C_MSG_WRITE			(0U << 0U)

/** Read message from I3C bus. */
#define I3C_MSG_READ			BIT(0)

/** @cond INTERNAL_HIDDEN */
#define I3C_MSG_RW_MASK			BIT(0)
/** @endcond  */

/** Send STOP after this message. */
#define I3C_MSG_STOP			BIT(1)

/**
 * RESTART I3C transaction for this message.
 *
 * @note Not all I3C drivers have or require explicit support for this
 * feature. Some drivers require this be present on a read message
 * that follows a write, or vice-versa.  Some drivers will merge
 * adjacent fragments into a single transaction using this flag; some
 * will not.
 */
#define I3C_MSG_RESTART			BIT(2)

/** Transfer use HDR mode */
#define I3C_MSG_HDR			BIT(3)

/** Skip I3C broadcast header. Private Transfers only. */
#define I3C_MSG_NBCH			BIT(4)

/** I3C HDR Mode 0 */
#define I3C_MSG_HDR_MODE0		BIT(0)

/** I3C HDR Mode 1 */
#define I3C_MSG_HDR_MODE1		BIT(1)

/** I3C HDR Mode 2 */
#define I3C_MSG_HDR_MODE2		BIT(2)

/** I3C HDR Mode 3 */
#define I3C_MSG_HDR_MODE3		BIT(3)

/** I3C HDR Mode 4 */
#define I3C_MSG_HDR_MODE4		BIT(4)

/** I3C HDR Mode 5 */
#define I3C_MSG_HDR_MODE5		BIT(5)

/** I3C HDR Mode 6 */
#define I3C_MSG_HDR_MODE6		BIT(6)

/** I3C HDR Mode 7 */
#define I3C_MSG_HDR_MODE7		BIT(7)

/** I3C HDR-DDR (Double Data Rate) */
#define I3C_MSG_HDR_DDR			I3C_MSG_HDR_MODE0

/** I3C HDR-TSP (Ternary Symbol Pure-bus) */
#define I3C_MSG_HDR_TSP			I3C_MSG_HDR_MODE1

/** I3C HDR-TSL (Ternary Symbol Legacy-inclusive-bus) */
#define I3C_MSG_HDR_TSL			I3C_MSG_HDR_MODE2

/** I3C HDR-BT (Bulk Transport) */
#define I3C_MSG_HDR_BT			I3C_MSG_HDR_MODE3

/** @} */

/**
 * @addtogroup i3c_transfer_api
 * @{
 */

/**
 * @brief One I3C Message.
 *
 * This defines one I3C message to transact on the I3C bus.
 *
 * @note Some of the configurations supported by this API may not be
 * supported by specific SoC I3C hardware implementations, in
 * particular features related to bus transactions intended to read or
 * write data from different buffers within a single transaction.
 * Invocations of i3c_transfer() may not indicate an error when an
 * unsupported configuration is encountered.  In some cases drivers
 * will generate separate transactions for each message fragment, with
 * or without presence of #I3C_MSG_RESTART in #flags.
 */
struct i3c_msg {
	/** Data buffer in bytes */
	uint8_t			*buf;

	/** Length of buffer in bytes */
	uint32_t		len;

	/**
	 * Total number of bytes transferred
	 *
	 * A Target can issue an EoD or the Controller can abort a transfer
	 * before the length of the buffer. It is expected for the driver to
	 * write to this after the transfer.
	 */
	uint32_t		num_xfer;

	/** Flags for this message */
	uint8_t			flags;

	/**
	 * HDR mode (@c I3C_MSG_HDR_MODE*) for transfer
	 * if any @c I3C_MSG_HDR_* is set in #flags.
	 *
	 * Use SDR mode if none is set.
	 */
	uint8_t			hdr_mode;

	/** HDR command code field (7-bit) for HDR-DDR, HDR-TSP and HDR-TSL */
	uint8_t			hdr_cmd_code;
};

/** @} */

/**
 * @brief Type of configuration being passed to configure function.
 */
enum i3c_config_type {
	I3C_CONFIG_CONTROLLER,
	I3C_CONFIG_TARGET,
	I3C_CONFIG_CUSTOM,
};

/**
 * @brief Configuration parameters for I3C hardware to act as controller.
 */
struct i3c_config_controller {
	/**
	 * True if the controller is to be the secondary controller
	 * of the bus. False to be the primary controller.
	 */
	bool is_secondary;

	struct {
		/** SCL frequency (in Hz) for I3C transfers. */
		uint32_t i3c;

		/** SCL frequency (in Hz) for I2C transfers. */
		uint32_t i2c;
	} scl;

	/**
	 * Bit mask of supported HDR modes (0 - 7).
	 *
	 * This can be used to enable or disable HDR mode
	 * supported by the hardware at runtime.
	 */
	uint8_t supported_hdr;
};

/**
 * @brief Custom I3C configuration parameters.
 *
 * This can be used to configure the I3C hardware on parameters
 * not covered by i3c_config_controller or i3c_config_target.
 * Mostly used to configure vendor specific parameters of the I3C
 * hardware.
 */
struct i3c_config_custom {
	/** ID of the configuration parameter. */
	uint32_t id;

	union {
		/** Value of configuration parameter. */
		uintptr_t val;

		/**
		 * Pointer to configuration parameter.
		 *
		 * Mainly used to pointer to a struct that
		 * the device driver understands.
		 */
		void *ptr;
	};
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
struct i3c_device_desc;
struct i3c_device_id;
struct i3c_i2c_device_desc;
struct i3c_target_config;

__subsystem struct i3c_driver_api {
	/**
	 * For backward compatibility to I2C API.
	 *
	 * @see i2c_driver_api for more information.
	 *
	 * @internal
	 * @warning DO NOT MOVE! Must be at the beginning.
	 * @endinternal
	 */
	struct i2c_driver_api i2c_api;

	/**
	 * Configure the I3C hardware.
	 *
	 * @see i3c_configure()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param type Type of configuration parameters being passed
	 *             in @p config.
	 * @param config Pointer to the configuration parameters.
	 *
	 * @return See i3c_configure()
	 */
	int (*configure)(const struct device *dev,
			 enum i3c_config_type type, void *config);

	/**
	 * Get configuration of the I3C hardware.
	 *
	 * @see i3c_config_get()
	 *
	 * @param[in] dev Pointer to controller device driver instance.
	 * @param[in] type Type of configuration parameters being passed
	 *                 in @p config.
	 * @param[in, out] config Pointer to the configuration parameters.
	 *
	 * @return See i3c_config_get()
	 */
	int (*config_get)(const struct device *dev,
			  enum i3c_config_type type, void *config);

	/**
	 * Perform bus recovery
	 *
	 * Controller only API.
	 *
	 * @see i3c_recover_bus()
	 *
	 * @param dev Pointer to controller device driver instance.
	 *
	 * @return See i3c_recover_bus()
	 */
	int (*recover_bus)(const struct device *dev);

	/**
	 * I3C Device Attach
	 *
	 * Optional API.
	 *
	 * @see i3c_attach_i3c_device()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_attach_i3c_device()
	 */
	int (*attach_i3c_device)(const struct device *dev,
			struct i3c_device_desc *target);

	/**
	 * I3C Address Update
	 *
	 * Optional API.
	 *
	 * @see i3c_reattach_i3c_device()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 * @param old_dyn_addr Old dynamic address
	 *
	 * @return See i3c_reattach_i3c_device()
	 */
	int (*reattach_i3c_device)(const struct device *dev,
			struct i3c_device_desc *target,
			uint8_t old_dyn_addr);

	/**
	 * I3C Device Detach
	 *
	 * Optional API.
	 *
	 * @see i3c_detach_i3c_device()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_detach_i3c_device()
	 */
	int (*detach_i3c_device)(const struct device *dev,
			struct i3c_device_desc *target);

	/**
	 * I2C Device Attach
	 *
	 * Optional API.
	 *
	 * @see i3c_attach_i2c_device()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_attach_i2c_device()
	 */
	int (*attach_i2c_device)(const struct device *dev,
			struct i3c_i2c_device_desc *target);

	/**
	 * I2C Device Detach
	 *
	 * Optional API.
	 *
	 * @see i3c_detach_i2c_device()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_detach_i2c_device()
	 */
	int (*detach_i2c_device)(const struct device *dev,
			struct i3c_i2c_device_desc *target);

	/**
	 * Perform Dynamic Address Assignment via ENTDAA.
	 *
	 * Controller only API.
	 *
	 * @see i3c_do_daa()
	 *
	 * @param dev Pointer to controller device driver instance.
	 *
	 * @return See i3c_do_daa()
	 */
	int (*do_daa)(const struct device *dev);

	/**
	 * Send Common Command Code (CCC).
	 *
	 * Controller only API.
	 *
	 * @see i3c_do_ccc()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param payload Pointer to the CCC payload.
	 *
	 * @return See i3c_do_ccc()
	 */
	int (*do_ccc)(const struct device *dev,
		      struct i3c_ccc_payload *payload);

	/**
	 * Transfer messages in I3C mode.
	 *
	 * @see i3c_transfer()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 * @param msg Pointer to I3C messages.
	 * @param num_msgs Number of messages to transfer.
	 *
	 * @return See i3c_transfer()
	 */
	int (*i3c_xfers)(const struct device *dev,
			 struct i3c_device_desc *target,
			 struct i3c_msg *msgs,
			 uint8_t num_msgs);

	/**
	 * Find a registered I3C target device.
	 *
	 * Controller only API.
	 *
	 * This returns the I3C device descriptor of the I3C device
	 * matching the incoming @p id.
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param id Pointer to I3C device ID.
	 *
	 * @return See i3c_device_find().
	 */
	struct i3c_device_desc *(*i3c_device_find)(const struct device *dev,
						   const struct i3c_device_id *id);

	/**
	 * Raise In-Band Interrupt (IBI).
	 *
	 * Target device only API.
	 *
	 * @see i3c_ibi_request()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param request Pointer to IBI request struct.
	 *
	 * @return See i3c_ibi_request()
	 */
	int (*ibi_raise)(const struct device *dev,
			 struct i3c_ibi *request);

	/**
	 * Enable receiving IBI from a target.
	 *
	 * Controller only API.
	 *
	 * @see i3c_ibi_enable()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_ibi_enable()
	 */
	int (*ibi_enable)(const struct device *dev,
			  struct i3c_device_desc *target);

	/**
	 * Disable receiving IBI from a target.
	 *
	 * Controller only API.
	 *
	 * @see i3c_ibi_disable()
	 *
	 * @param dev Pointer to controller device driver instance.
	 * @param target Pointer to target device descriptor.
	 *
	 * @return See i3c_ibi_disable()
	 */
	int (*ibi_disable)(const struct device *dev,
			   struct i3c_device_desc *target);

	/**
	 * Register config as target device of a controller.
	 *
	 * This tells the controller to act as a target device
	 * on the I3C bus.
	 *
	 * Target device only API.
	 *
	 * @see i3c_target_register()
	 *
	 * @param dev Pointer to the controller device driver instance.
	 * @param cfg I3C target device configuration
	 *
	 * @return See i3c_target_register()
	 */
	int (*target_register)(const struct device *dev,
			       struct i3c_target_config *cfg);

	/**
	 * Unregister config as target device of a controller.
	 *
	 * This tells the controller to stop acting as a target device
	 * on the I3C bus.
	 *
	 * Target device only API.
	 *
	 * @see i3c_target_unregister()
	 *
	 * @param dev Pointer to the controller device driver instance.
	 * @param cfg I3C target device configuration
	 *
	 * @return See i3c_target_unregister()
	 */
	int (*target_unregister)(const struct device *dev,
				 struct i3c_target_config *cfg);

	/**
	 * Write to the TX FIFO
	 *
	 * This writes to the target tx fifo
	 *
	 * Target device only API.
	 *
	 * @see i3c_target_tx_write()
	 *
	 * @param dev Pointer to the controller device driver instance.
	 * @param buf Pointer to the buffer
	 * @param len Length of the buffer
	 * @param hdr_mode HDR mode
	 *
	 * @return See i3c_target_tx_write()
	 */
	int (*target_tx_write)(const struct device *dev,
				 uint8_t *buf, uint16_t len, uint8_t hdr_mode);

#ifdef CONFIG_I3C_RTIO
	/**
	 * RTIO
	 *
	 * @see i3c_iodev_submit()
	 *
	 * @param dev Pointer to the controller device driver instance.
	 * @param iodev_sqe Pointer to the
	 *
	 * @return See i3c_iodev_submit()
	 */
	void (*iodev_submit)(const struct device *dev,
				 struct rtio_iodev_sqe *iodev_sqe);
#endif
};

/**
 * @endcond
 */

/**
 * @brief Structure used for matching I3C devices.
 */
struct i3c_device_id {
	/** Device Provisioned ID */
	const uint64_t pid:48;
};

/**
 * @brief Structure initializer for i3c_device_id from PID
 *
 * This helper macro expands to a static initializer for a i3c_device_id
 * by populating the PID (Provisioned ID) field.
 *
 * @param pid Provisioned ID.
 */
#define I3C_DEVICE_ID(pid)						\
	{								\
		.pid = pid						\
	}

/**
 * @brief Structure describing a I3C target device.
 *
 * Instances of this are passed to the I3C controller device APIs,
 * for example:
 * - i3c_device_register() to tell the controller of a target device.
 * - i3c_transfers() to initiate data transfers between controller and
 *   target device.
 *
 * Fields #bus, #pid and #static_addr must be initialized by the module that
 * implements the target device behavior prior to passing the object reference
 * to I3C controller device APIs. #static_addr can be zero if target device does
 * not have static address.
 *
 * Internal field @c node should not be initialized or modified manually.
 */
struct i3c_device_desc {
	sys_snode_t node;

	/** I3C bus to which this target device is attached */
	const struct device * const bus;

	/** Device driver instance of the I3C device */
	const struct device * const dev;

	/** Device Provisioned ID */
	const uint64_t pid:48;

	/**
	 * Static address for this target device.
	 *
	 * 0 if static address is not being used, and only dynamic
	 * address is used. This means that the target device must
	 * go through ENTDAA (Dynamic Address Assignment) to get
	 * a dynamic address before it can communicate with
	 * the controller. This means SETAASA and SETDASA CCC
	 * cannot be used to set dynamic address on the target
	 * device (as both are to tell target device to use static
	 * address as dynamic address).
	 */
	const uint8_t static_addr;

	/**
	 * Initial dynamic address.
	 *
	 * This is specified in the device tree property "assigned-address"
	 * to indicate the desired dynamic address during address
	 * assignment (SETDASA and ENTDAA).
	 *
	 * 0 if there is no preference.
	 */
	const uint8_t init_dynamic_addr;

	/**
	 * Device support for SETAASA
	 *
	 * This will be used as an optimization for bus initializtion if the
	 * device supports SETAASA.
	 */
	const bool supports_setaasa;

	/**
	 * Dynamic Address for this target device used for communication.
	 *
	 * This is to be set by the controller driver in one of
	 * the following situations:
	 * - During Dynamic Address Assignment (during ENTDAA)
	 * - Reset Dynamic Address Assignment (RSTDAA)
	 * - Set All Addresses to Static Addresses (SETAASA)
	 * - Set New Dynamic Address (SETNEWDA)
	 * - Set Dynamic Address from Static Address (SETDASA)
	 *
	 * 0 if address has not been assigned.
	 */
	uint8_t dynamic_addr;

#if defined(CONFIG_I3C_USE_GROUP_ADDR) || defined(__DOXYGEN__)
	/**
	 * Group address for this target device. Set during:
	 * - Reset Group Address(es) (RSTGRPA)
	 * - Set Group Address (SETGRPA)
	 *
	 * 0 if group address has not been assigned.
	 * Only available if @kconfig{CONFIG_I3C_USE_GROUP_ADDR} is set.
	 */
	uint8_t group_addr;
#endif /* CONFIG_I3C_USE_GROUP_ADDR */

	/**
	 * Bus Characteristic Register (BCR)
	 * @see @ref I3C_BCR
	 */
	uint8_t bcr;

	/**
	 * Device Characteristic Register (DCR)
	 *
	 * Describes the type of device. Refer to official documentation
	 * on what this number means.
	 */
	uint8_t dcr;

	struct {
		/** Maximum Read Speed */
		uint8_t maxrd;

		/** Maximum Write Speed */
		uint8_t maxwr;

		/** Maximum Read turnaround time in microseconds. */
		uint32_t max_read_turnaround;
	} data_speed;

	struct {
		/** Maximum Read Length */
		uint16_t mrl;

		/** Maximum Write Length */
		uint16_t mwl;

		/** Maximum IBI Payload Size. Valid only if BCR[2] is 1. */
		uint8_t max_ibi;
	} data_length;

	/** Describes advanced (Target) capabilities and features */
	struct {
		union {
			/**
			 * I3C v1.0 HDR Capabilities (@c I3C_CCC_GETCAPS1_*)
			 * - Bit[0]: HDR-DDR
			 * - Bit[1]: HDR-TSP
			 * - Bit[2]: HDR-TSL
			 * - Bit[7:3]: Reserved
			 */
			uint8_t gethdrcap;

			/**
			 * I3C v1.1+ GETCAPS1 (@c I3C_CCC_GETCAPS1_*)
			 * - Bit[0]: HDR-DDR
			 * - Bit[1]: HDR-TSP
			 * - Bit[2]: HDR-TSL
			 * - Bit[3]: HDR-BT
			 * - Bit[7:4]: Reserved
			 */
			uint8_t getcap1;
		};

		/**
		 *  GETCAPS2 (@c I3C_CCC_GETCAPS2_*)
		 * - Bit[3:0]: I3C 1.x Specification Version
		 * - Bit[5:4]: Group Address Capabilities
		 * - Bit[6]: HDR-DDR Write Abort
		 * - Bit[7]: HDR-DDR Abort CRC
		 */
		uint8_t getcap2;

		/**
		 * GETCAPS3 (@c I3C_CCC_GETCAPS3_*)
		 * - Bit[0]: Multi-Lane (ML) Data Transfer Support
		 * - Bit[1]: Device to Device Transfer (D2DXFER) Support
		 * - Bit[2]: Device to Device Transfer (D2DXFER) IBI Capable
		 * - Bit[3]: Defining Byte Support in GETCAPS
		 * - Bit[4]: Defining Byte Support in GETSTATUS
		 * - Bit[5]: HDR-BT CRC-32 Support
		 * - Bit[6]: IBI MDB Support for Pending Read Notification
		 * - Bit[7]: Reserved
		 */
		uint8_t getcap3;

		/**
		 * GETCAPS4
		 * - Bit[7:0]: Reserved
		 */
		uint8_t getcap4;
	} getcaps;

	/** @cond INTERNAL_HIDDEN */
	/**
	 * Private data by the controller to aid in transactions. Do not modify.
	 */
	void *controller_priv;
	/** @endcond */

#if defined(CONFIG_I3C_USE_IBI) || defined(__DOXYGEN__)
	/**
	 * In-Band Interrupt (IBI) callback.
	 * Only available if @kconfig{CONFIG_I3C_USE_IBI} is set.
	 */
	i3c_target_ibi_cb_t ibi_cb;
#endif /* CONFIG_I3C_USE_IBI */
};

/**
 * @brief Structure describing a I2C device on I3C bus.
 *
 * Instances of this are passed to the I3C controller device APIs,
 * for example:
 * () i3c_i2c_device_register() to tell the controller of an I2C device.
 * () i3c_i2c_transfers() to initiate data transfers between controller
 *    and I2C device.
 *
 * Fields other than @c node must be initialized by the module that
 * implements the device behavior prior to passing the object
 * reference to I3C controller device APIs.
 */
struct i3c_i2c_device_desc {
	sys_snode_t node;

	/** I3C bus to which this I2C device is attached */
	const struct device *bus;

	/** Static address for this I2C device. */
	const uint16_t addr;

	/**
	 * Legacy Virtual Register (LVR)
	 * @see @ref I3C_LVR
	 */
	const uint8_t lvr;

	/** @cond INTERNAL_HIDDEN */
	/**
	 * Private data by the controller to aid in transactions. Do not modify.
	 */
	void *controller_priv;
	/** @endcond */
};

/**
 * @brief Structure for describing attached devices for a controller.
 *
 * This contains slists of attached I3C and I2C devices.
 *
 * This is a helper struct that can be used by controller device
 * driver to aid in device management.
 */
struct i3c_dev_attached_list {
	/**
	 * Address slots:
	 * - Aid in dynamic address assignment.
	 * - Quick way to find out if a target address is
	 *   a I3C or I2C device.
	 */
	struct i3c_addr_slots addr_slots;

	struct {
		/**
		 * Linked list of attached I3C devices.
		 */
		sys_slist_t i3c;

		/**
		 * Linked list of attached I2C devices.
		 */
		sys_slist_t i2c;
	} devices;
};

/**
 * @brief Structure for describing known devices for a controller.
 *
 * This contains arrays of known I3C and I2C devices.
 *
 * This is a helper struct that can be used by controller device
 * driver to aid in device management.
 */
struct i3c_dev_list {
	/**
	 * Pointer to array of known I3C devices.
	 */
	struct i3c_device_desc * const i3c;

	/**
	 * Pointer to array of known I2C devices.
	 */
	struct i3c_i2c_device_desc * const i2c;

	/**
	 * Number of I3C devices in array.
	 */
	const uint8_t num_i3c;

	/**
	 * Number of I2C devices in array.
	 */
	const uint8_t num_i2c;
};

/**
 * This structure is common to all I3C drivers and is expected to be
 * the first element in the object pointed to by the config field
 * in the device structure.
 */
struct i3c_driver_config {
	/** I3C/I2C device list struct. */
	struct i3c_dev_list dev_list;
};

/**
 * This structure is common to all I3C drivers and is expected to be the first
 * element in the driver's struct driver_data declaration.
 */
struct i3c_driver_data {
	/** Controller Configuration */
	struct i3c_config_controller ctrl_config;

	/** Attached I3C/I2C devices and addresses */
	struct i3c_dev_attached_list attached_dev;
};

/**
 * @brief iterate over all I3C devices present on the bus
 *
 * @param bus: the I3C bus device pointer
 * @param desc: an I3C device descriptor pointer updated to point to the current slot
 *	 at each iteration of the loop
 */
#define I3C_BUS_FOR_EACH_I3CDEV(bus, desc)                                                         \
	SYS_SLIST_FOR_EACH_CONTAINER(                                                              \
		&((struct i3c_driver_data *)(bus->data))->attached_dev.devices.i3c, desc, node)

/**
 * @brief iterate over all I2C devices present on the bus
 *
 * @param bus: the I3C bus device pointer
 * @param desc: an I2C device descriptor pointer updated to point to the current slot
 *	 at each iteration of the loop
 */
#define I3C_BUS_FOR_EACH_I2CDEV(bus, desc)                                                         \
	SYS_SLIST_FOR_EACH_CONTAINER(                                                              \
		&((struct i3c_driver_data *)(bus->data))->attached_dev.devices.i2c, desc, node)

/**
 * @brief Find a I3C target device descriptor by ID.
 *
 * This finds the I3C target device descriptor in the device list
 * matching the provided ID struct (@p id).
 *
 * @param dev_list Pointer to the device list struct.
 * @param id Pointer to I3C device ID struct.
 *
 * @return Pointer to the I3C target device descriptor, or
 *         `NULL` if none is found.
 */
struct i3c_device_desc *i3c_dev_list_find(const struct i3c_dev_list *dev_list,
					  const struct i3c_device_id *id);

/**
 * @brief Find a I3C target device descriptor by dynamic address.
 *
 * This finds the I3C target device descriptor in the attached
 * device list matching the dynamic address (@p addr)
 *
 * @param dev Pointer to controller device driver instance.
 * @param addr Dynamic address to be matched.
 *
 * @return Pointer to the I3C target device descriptor, or
 *         `NULL` if none is found.
 */
struct i3c_device_desc *i3c_dev_list_i3c_addr_find(const struct device *dev,
						   uint8_t addr);

/**
 * @brief Find a I2C target device descriptor by address.
 *
 * This finds the I2C target device descriptor in the attached
 * device list matching the address (@p addr)
 *
 * @param dev Pointer to controller device driver instance.
 * @param addr Address to be matched.
 *
 * @return Pointer to the I2C target device descriptor, or
 *         `NULL` if none is found.
 */
struct i3c_i2c_device_desc *i3c_dev_list_i2c_addr_find(const struct device *dev,
							   uint16_t addr);

/**
 * @brief Helper function to find a usable address during ENTDAA.
 *
 * This is a helper function to find a usable address during
 * Dynamic Address Assignment. Given the PID (@p pid), it will
 * search through the device list for the matching device
 * descriptor. If the device descriptor indicates that there is
 * a preferred address (i.e. assigned-address in device tree,
 * i3c_device_desc::init_dynamic_addr), this preferred
 * address will be returned if this address is still available.
 * If it is not available, another free address will be returned.
 *
 * If @p must_match is true, the PID (@p pid) must match
 * one of the device in the device list.
 *
 * If @p must_match is false, this will return an arbitrary
 * address. This is useful when not all devices are described in
 * device tree. Or else, the DAA process cannot proceed since
 * there is no address to be assigned.
 *
 * If @p assigned_okay is true, it will return the same address
 * already assigned to the device
 * (i3c_device_desc::dynamic_addr). If no address has been
 * assigned, it behaves as if @p assigned_okay is false.
 * This is useful for assigning the same address to the same
 * device (for example, hot-join after device coming back from
 * suspend).
 *
 * If @p assigned_okay is false, the device cannot have an address
 * assigned already (that i3c_device_desc::dynamic_addr is not
 * zero). This is mainly used during the initial DAA.
 *
 * @param[in] addr_slots Pointer to address slots struct.
 * @param[in] dev_list Pointer to the device list struct.
 * @param[in] pid Provisioned ID of device to be assigned address.
 * @param[in] must_match True if PID must match devices in
 *			 the device list. False otherwise.
 * @param[in] assigned_okay True if it is okay to return the
 *                          address already assigned to the target
 *                          matching the PID (@p pid).
 * @param[out] target Store the pointer of the device descriptor
 *                    if it matches the incoming PID (@p pid).
 * @param[out] addr Address to be assigned to target device.
 *
 * @retval 0 if successful.
 * @retval -ENODEV if no device matches the PID (@p pid) in
 *                 the device list and @p must_match is true.
 * @retval -EINVAL if the device matching PID (@p pid) already
 *                 has an address assigned or invalid function
 *                 arguments.
 */
int i3c_dev_list_daa_addr_helper(struct i3c_addr_slots *addr_slots,
				 const struct i3c_dev_list *dev_list,
				 uint64_t pid, bool must_match,
				 bool assigned_okay,
				 struct i3c_device_desc **target,
				 uint8_t *addr);

/**
 * @brief Configure the I3C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param type Type of configuration parameters being passed
 *             in @p config.
 * @param config Pointer to the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static inline int i3c_configure(const struct device *dev,
				enum i3c_config_type type, void *config)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->configure == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, type, config);
}

/**
 * @brief Get configuration of the I3C hardware.
 *
 * This provides a way to get the current configuration of the I3C hardware.
 *
 * This can return cached config or probed hardware parameters, but it has to
 * be up to date with current configuration.
 *
 * @param[in] dev Pointer to controller device driver instance.
 * @param[in] type Type of configuration parameters being passed
 *                 in @p config.
 * @param[in,out] config Pointer to the configuration parameters.
 *
 * Note that if @p type is #I3C_CONFIG_CUSTOM, @p config must contain
 * the ID of the parameter to be retrieved.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static inline int i3c_config_get(const struct device *dev,
				 enum i3c_config_type type, void *config)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->config_get == NULL) {
		return -ENOSYS;
	}

	return api->config_get(dev, type, config);
}

/**
 * @brief Attempt bus recovery on the I3C bus.
 *
 * This routine asks the controller to attempt bus recovery.
 *
 * @retval 0 If successful.
 * @retval -EBUSY If bus recovery fails.
 * @retval -EIO General input / output error.
 * @retval -ENOSYS Bus recovery is not supported by the controller driver.
 */
static inline int i3c_recover_bus(const struct device *dev)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->recover_bus == NULL) {
		return -ENOSYS;
	}

	return api->recover_bus(dev);
}

/**
 * @brief Attach an I3C device
 *
 * Called to attach a I3C device to the addresses. This is
 * typically called before a SETDASA or ENTDAA to reserve
 * the addresses. This will also call the optional api to
 * update any registers within the driver if implemented.
 *
 * @warning
 * Use cases involving multiple writers to the i3c/i2c devices must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the i3c/i2c devices.
 *
 * @param target Pointer to the target device descriptor
 *
 * @retval 0 If successful.
 * @retval -EINVAL If address is not available or if the device
 *     has already been attached before
 */
int i3c_attach_i3c_device(struct i3c_device_desc *target);

/**
 * @brief Reattach I3C device
 *
 * called after every time an I3C device has its address
 * changed. It can be because the device has been powered
 * down and has lost its address, or it can happen when a
 * device had a static address and has been assigned a
 * dynamic address with SETDASA or a dynamic address has
 * been updated with SETNEWDA. This will also call the
 * optional api to update any registers within the driver
 * if implemented.
 *
 * @warning
 * Use cases involving multiple writers to the i3c/i2c devices must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the i3c/i2c devices.
 *
 * @param target Pointer to the target device descriptor
 * @param old_dyn_addr The old dynamic address of target device, 0 if
 *            there was no old dynamic address
 *
 * @retval 0 If successful.
 * @retval -EINVAL If address is not available
 */
int i3c_reattach_i3c_device(struct i3c_device_desc *target, uint8_t old_dyn_addr);

/**
 * @brief Detach I3C Device
 *
 * called to remove an I3C device and to free up the address
 * that it used. If it's dynamic address was not set, then it
 * assumed that SETDASA failed and will free it's static addr.
 * This will also call the optional api to update any registers
 * within the driver if implemented.
 *
 * @warning
 * Use cases involving multiple writers to the i3c/i2c devices must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the i3c/i2c devices.
 *
 * @param target Pointer to the target device descriptor
 *
 * @retval 0 If successful.
 * @retval -EINVAL If device is already detached
 */
int i3c_detach_i3c_device(struct i3c_device_desc *target);

/**
 * @brief Attach an I2C device
 *
 * Called to attach a I2C device to the addresses. This will
 * also call the optional api to update any registers within
 * the driver if implemented.
 *
 * @warning
 * Use cases involving multiple writers to the i3c/i2c devices must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the i3c/i2c devices.
 *
 * @param target Pointer to the target device descriptor
 *
 * @retval 0 If successful.
 * @retval -EINVAL If address is not available or if the device
 *     has already been attached before
 */
int i3c_attach_i2c_device(struct i3c_i2c_device_desc *target);

/**
 * @brief Detach I2C Device
 *
 * called to remove an I2C device and to free up the address
 * that it used. This will also call the optional api to
 * update any registers within the driver if implemented.
 *
 * @warning
 * Use cases involving multiple writers to the i3c/i2c devices must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the i3c/i2c devices.
 *
 * @param target Pointer to the target device descriptor
 *
 * @retval 0 If successful.
 * @retval -EINVAL If device is already detached
 */
int i3c_detach_i2c_device(struct i3c_i2c_device_desc *target);

/**
 * @brief Perform Dynamic Address Assignment on the I3C bus.
 *
 * This routine asks the controller to perform dynamic address assignment
 * where the controller belongs. Only the active controller of the bus
 * should do this.
 *
 * @note For controller driver implementation, the controller should perform
 * SETDASA to allow static addresses to be the dynamic addresses before
 * actually doing ENTDAA.
 *
 * @param dev Pointer to the device structure for the controller driver
 *            instance.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 * @retval -ENODEV If a provisioned ID does not match to any target devices
 *                 in the registered device list.
 * @retval -ENOSPC No more free addresses can be assigned to target.
 * @retval -ENOSYS Dynamic address assignment is not supported by
 *                 the controller driver.
 */
static inline int i3c_do_daa(const struct device *dev)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->do_daa == NULL) {
		return -ENOSYS;
	}

	return api->do_daa(dev);
}

/**
 * @brief Send CCC to the bus.
 *
 * @param dev Pointer to the device structure for the controller driver
 *            instance.
 * @param payload Pointer to the structure describing the CCC payload.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General Input / output error.
 * @retval -EINVAL Invalid valid set in the payload structure.
 * @retval -ENOSYS Not implemented.
 */
__syscall int i3c_do_ccc(const struct device *dev,
			 struct i3c_ccc_payload *payload);

static inline int z_impl_i3c_do_ccc(const struct device *dev,
				    struct i3c_ccc_payload *payload)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->do_ccc == NULL) {
		return -ENOSYS;
	}

	return api->do_ccc(dev, payload);
}

/**
 * @addtogroup i3c_transfer_api
 * @{
 */

/**
 * @brief Perform data transfer from the controller to a I3C target device.
 *
 * This routine provides a generic interface to perform data transfer
 * to a target device synchronously. Use i3c_read()/i3c_write()
 * for simple read or write.
 *
 * The array of message @p msgs must not be `NULL`.  The number of
 * message @p num_msgs may be zero, in which case no transfer occurs.
 *
 * @note Not all scatter/gather transactions can be supported by all
 * drivers.  As an example, a gather write (multiple consecutive
 * i3c_msg buffers all configured for #I3C_MSG_WRITE) may be packed
 * into a single transaction by some drivers, but others may emit each
 * fragment as a distinct write transaction, which will not produce
 * the same behavior.  See the documentation of i3c_msg for
 * limitations on support for multi-message bus transactions.
 *
 * @param target I3C target device descriptor.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
__syscall int i3c_transfer(struct i3c_device_desc *target,
			   struct i3c_msg *msgs, uint8_t num_msgs);

static inline int z_impl_i3c_transfer(struct i3c_device_desc *target,
				      struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)target->bus->api;

	return api->i3c_xfers(target->bus, target, msgs, num_msgs);
}

/** @} */

/**
 * Find a registered I3C target device.
 *
 * Controller only API.
 *
 * This returns the I3C device descriptor of the I3C device
 * matching the incoming @p id.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id Pointer to I3C device ID.
 *
 * @return Pointer to I3C device descriptor, or `NULL` if
 *         no I3C device found matching incoming @p id.
 */
static inline
struct i3c_device_desc *i3c_device_find(const struct device *dev,
					const struct i3c_device_id *id)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->i3c_device_find == NULL) {
		return NULL;
	}

	return api->i3c_device_find(dev, id);
}

/**
 * @addtogroup i3c_ibi
 * @{
 */

/**
 * @brief Raise an In-Band Interrupt (IBI).
 *
 * This raises an In-Band Interrupt (IBI) to the active controller.
 *
 * @param dev Pointer to controller device driver instance.
 * @param request Pointer to the IBI request struct.
 *
 * @retval 0 if operation is successful.
 * @retval -EIO General input / output error.
 */
static inline int i3c_ibi_raise(const struct device *dev,
				struct i3c_ibi *request)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->ibi_raise == NULL) {
		return -ENOSYS;
	}

	return api->ibi_raise(dev, request);
}

/**
 * @brief Enable IBI of a target device.
 *
 * This enables IBI of a target device where the IBI has already been
 * request.
 *
 * @param target I3C target device descriptor.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input / output error.
 * @retval -ENOMEM If these is no more empty entries in
 *                 the controller's IBI table (if the controller
 *                 uses such table).
 */
static inline int i3c_ibi_enable(struct i3c_device_desc *target)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)target->bus->api;

	if (api->ibi_enable == NULL) {
		return -ENOSYS;
	}

	return api->ibi_enable(target->bus, target);
}

/**
 * @brief Disable IBI of a target device.
 *
 * This enables IBI of a target device where the IBI has already been
 * request.
 *
 * @param target I3C target device descriptor.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input / output error.
 * @retval -ENODEV If IBI is not previously enabled for @p target.
 */
static inline int i3c_ibi_disable(struct i3c_device_desc *target)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)target->bus->api;

	if (api->ibi_disable == NULL) {
		return -ENOSYS;
	}

	return api->ibi_disable(target->bus, target);
}

/**
 * @brief Check if target's IBI has payload.
 *
 * This reads the BCR from the device descriptor struct to determine
 * whether IBI from device has payload.
 *
 * Note that BCR must have been obtained from device and
 * i3c_device_desc::bcr must be set.
 *
 * @return True if IBI has payload, false otherwise.
 */
static inline int i3c_ibi_has_payload(struct i3c_device_desc *target)
{
	return (target->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE)
		== I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE;
}

/**
 * @brief Check if device is IBI capable.
 *
 * This reads the BCR from the device descriptor struct to determine
 * whether device is capable of IBI.
 *
 * Note that BCR must have been obtained from device and
 * i3c_device_desc::bcr must be set.
 *
 * @return True if IBI has payload, false otherwise.
 */
static inline int i3c_device_is_ibi_capable(struct i3c_device_desc *target)
{
	return (target->bcr & I3C_BCR_IBI_REQUEST_CAPABLE)
		== I3C_BCR_IBI_REQUEST_CAPABLE;
}

/**
 * @brief Check if the target is controller capable
 *
 * This reads the BCR from the device descriptor struct to determine
 * whether the target is controller capable
 *
 * Note that BCR must have been obtained from device and
 * i3c_device_desc::bcr must be set.
 *
 * @return True if target is controller capable, false otherwise.
 */
static inline int i3c_device_is_controller_capable(struct i3c_device_desc *target)
{
	return I3C_BCR_DEVICE_ROLE(target->bcr)
		== I3C_BCR_DEVICE_ROLE_I3C_CONTROLLER_CAPABLE;
}

/** @} */

/**
 * @addtogroup i3c_transfer_api
 * @{
 */

/**
 * @brief Write a set amount of data to an I3C target device.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param target I3C target device descriptor.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes to write.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_write(struct i3c_device_desc *target,
			    const uint8_t *buf, uint32_t num_bytes)
{
	struct i3c_msg msg;

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP;
	msg.hdr_mode = 0;
	msg.hdr_cmd_code = 0;

	return i3c_transfer(target, &msg, 1);
}

/**
 * @brief Read a set amount of data from an I3C target device.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param target I3C target device descriptor.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes to read.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_read(struct i3c_device_desc *target,
			   uint8_t *buf, uint32_t num_bytes)
{
	struct i3c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_READ | I3C_MSG_STOP;
	msg.hdr_mode = 0;
	msg.hdr_cmd_code = 0;

	return i3c_transfer(target, &msg, 1);
}

/**
 * @brief Write then read data from an I3C target device.
 *
 * This supports the common operation "this is what I want", "now give
 * it to me" transaction pair through a combined write-then-read bus
 * transaction.
 *
 * @param target I3C target device descriptor.
 * @param write_buf Pointer to the data to be written
 * @param num_write Number of bytes to write
 * @param read_buf Pointer to storage for read data
 * @param num_read Number of bytes to read
 *
 * @retval 0 if successful
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_write_read(struct i3c_device_desc *target,
				 const void *write_buf, size_t num_write,
				 void *read_buf, size_t num_read)
{
	struct i3c_msg msg[2];

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I3C_MSG_WRITE;
	msg[0].hdr_mode = 0;
	msg[0].hdr_cmd_code = 0;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I3C_MSG_RESTART | I3C_MSG_READ | I3C_MSG_STOP;
	msg[1].hdr_mode = 0;
	msg[1].hdr_cmd_code = 0;

	return i3c_transfer(target, msg, 2);
}

/**
 * @brief Read multiple bytes from an internal address of an I3C target device.
 *
 * This routine reads multiple bytes from an internal address of an
 * I3C target device synchronously.
 *
 * Instances of this may be replaced by i3c_write_read().
 *
 * @param target I3C target device descriptor,
 * @param start_addr Internal address from which the data is being read.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_burst_read(struct i3c_device_desc *target,
				 uint8_t start_addr,
				 uint8_t *buf,
				 uint32_t num_bytes)
{
	return i3c_write_read(target,
			      &start_addr, sizeof(start_addr),
			      buf, num_bytes);
}

/**
 * @brief Write multiple bytes to an internal address of an I3C target device.
 *
 * This routine writes multiple bytes to an internal address of an
 * I3C target device synchronously.
 *
 * @warning The combined write synthesized by this API may not be
 * supported on all I3C devices.  Uses of this API may be made more
 * portable by replacing them with calls to i3c_write() passing a
 * buffer containing the combined address and data.
 *
 * @param target I3C target device descriptor.
 * @param start_addr Internal address to which the data is being written.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes being written.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_burst_write(struct i3c_device_desc *target,
				  uint8_t start_addr,
				  const uint8_t *buf,
				  uint32_t num_bytes)
{
	struct i3c_msg msg[2];

	msg[0].buf = &start_addr;
	msg[0].len = 1U;
	msg[0].flags = I3C_MSG_WRITE;
	msg[0].hdr_mode = 0;
	msg[0].hdr_cmd_code = 0;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I3C_MSG_WRITE | I3C_MSG_STOP;
	msg[1].hdr_mode = 0;
	msg[1].hdr_cmd_code = 0;

	return i3c_transfer(target, msg, 2);
}

/**
 * @brief Read internal register of an I3C target device.
 *
 * This routine reads the value of an 8-bit internal register of an I3C target
 * device synchronously.
 *
 * @param target I3C target device descriptor.
 * @param reg_addr Address of the internal register being read.
 * @param value Memory pool that stores the retrieved register value.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_reg_read_byte(struct i3c_device_desc *target,
				    uint8_t reg_addr, uint8_t *value)
{
	return i3c_write_read(target,
			      &reg_addr, sizeof(reg_addr),
			      value, sizeof(*value));
}

/**
 * @brief Write internal register of an I3C target device.
 *
 * This routine writes a value to an 8-bit internal register of an I3C target
 * device synchronously.
 *
 * @note This function internally combines the register and value into
 * a single bus transaction.
 *
 * @param target I3C target device descriptor.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_reg_write_byte(struct i3c_device_desc *target,
				     uint8_t reg_addr, uint8_t value)
{
	uint8_t tx_buf[2] = {reg_addr, value};

	return i3c_write(target, tx_buf, 2);
}

/**
 * @brief Update internal register of an I3C target device.
 *
 * This routine updates the value of a set of bits from an 8-bit internal
 * register of an I3C target device synchronously.
 *
 * @note If the calculated new register value matches the value that
 * was read this function will not generate a write operation.
 *
 * @param target I3C target device descriptor.
 * @param reg_addr Address of the internal register being updated.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 */
static inline int i3c_reg_update_byte(struct i3c_device_desc *target,
				      uint8_t reg_addr, uint8_t mask,
				      uint8_t value)
{
	uint8_t old_value, new_value;
	int rc;

	rc = i3c_reg_read_byte(target, reg_addr, &old_value);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i3c_reg_write_byte(target, reg_addr, new_value);
}

/**
 * @brief Dump out an I3C message
 *
 * Dumps out a list of I3C messages. For any that are writes (W), the data is
 * displayed in hex.
 *
 * It looks something like this (with name "testing"):
 *
 * @code
 * D: I3C msg: testing, addr=56
 * D:    W len=01:
 * D: contents:
 * D: 06                      |.
 * D:    W len=0e:
 * D: contents:
 * D: 00 01 02 03 04 05 06 07 |........
 * D: 08 09 0a 0b 0c 0d       |......
 * @endcode
 *
 * @param name Name of this dump, displayed at the top.
 * @param msgs Array of messages to dump.
 * @param num_msgs Number of messages to dump.
 * @param target I3C target device descriptor.
 */
void i3c_dump_msgs(const char *name, const struct i3c_msg *msgs,
		   uint8_t num_msgs, struct i3c_device_desc *target);

/** @} */

/**
 * @brief Generic helper function to perform bus initialization.
 *
 * @param dev Pointer to controller device driver instance.
 * @param i3c_dev_list Pointer to I3C device list.
 *
 * @retval 0 If successful.
 * @retval -EBUSY Bus is busy.
 * @retval -EIO General input / output error.
 * @retval -ENODEV If a provisioned ID does not match to any target devices
 *                 in the registered device list.
 * @retval -ENOSPC No more free addresses can be assigned to target.
 * @retval -ENOSYS Dynamic address assignment is not supported by
 *                 the controller driver.
 */
int i3c_bus_init(const struct device *dev,
		 const struct i3c_dev_list *i3c_dev_list);

/**
 * @brief Get basic information from device and update device descriptor.
 *
 * This retrieves some basic information:
 *   * Bus Characteristics Register (GETBCR)
 *   * Device Characteristics Register (GETDCR)
 *   * Max Read Length (GETMRL)
 *   * Max Write Length (GETMWL)
 * from the device and update the corresponding fields of the device
 * descriptor.
 *
 * This only updates the field(s) in device descriptor
 * only if CCC operations succeed.
 *
 * @param[in,out] target I3C target device descriptor.
 *
 * @retval 0 if successful.
 * @retval -EIO General Input/Output error.
 */
int i3c_device_basic_info_get(struct i3c_device_desc *target);

/**
 * @brief Check if the bus has a secondary controller.
 *
 * This reads the BCR from the device descriptor struct of all targets
 * to determine whether a device is a secondary controller.
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @return True if the bus has a secondary controller, false otherwise.
 */
bool i3c_bus_has_sec_controller(const struct device *dev);

/**
 * @brief Send the CCC DEFTGTS
 *
 * This builds the payload required for DEFTGTS and transmits it out
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @retval 0 if successful.
 * @retval -ENOMEM No memory to build the payload.
 * @retval -EIO General Input/Output error.
 */
int i3c_bus_deftgts(const struct device *dev);

#if defined(CONFIG_I3C_RTIO) || defined(__DOXYGEN__)

struct i3c_iodev_data {
	const struct device *bus;
	const struct i3c_device_id dev_id;
};

/**
 * @brief Fallback submit implementation
 *
 * This implementation will schedule a blocking I3C transaction on the bus via the RTIO work
 * queue. It is only used if the I3C driver did not implement the iodev_submit function.
 *
 * @param dev Pointer to the device structure for an I3C controller driver.
 * @param iodev_sqe Prepared submissions queue entry connected to an iodev
 *                  defined by I3C_DT_IODEV_DEFINE.
 */
void i3c_iodev_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Submit request(s) to an I3C device with RTIO
 *
 * @param iodev_sqe Prepared submissions queue entry connected to an iodev
 *                  defined by I3C_DT_IODEV_DEFINE.
 */
static inline void i3c_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct i3c_iodev_data *data =
		(const struct i3c_iodev_data *)iodev_sqe->sqe.iodev->data;
	const struct i3c_driver_api *api = (const struct i3c_driver_api *)data->bus->api;

	if (api->iodev_submit == NULL) {
		rtio_iodev_sqe_err(iodev_sqe, -ENOSYS);
		return;
	}
	api->iodev_submit(data->bus, iodev_sqe);
}

extern const struct rtio_iodev_api i3c_iodev_api;

/**
 * @brief Define an iodev for a given dt node on the bus
 *
 * These do not need to be shared globally but doing so
 * will save a small amount of memory.
 *
 * @param name Symbolic name of the iodev to define
 * @param node_id Devicetree node identifier
 */
#define I3C_DT_IODEV_DEFINE(name, node_id)					\
	const struct i3c_iodev_data _i3c_iodev_data_##name = {			\
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),				\
		.dev_id = I3C_DEVICE_ID_DT(node_id),				\
	};									\
	RTIO_IODEV_DEFINE(name, &i3c_iodev_api, (void *)&_i3c_iodev_data_##name)

/**
 * @brief Copy the i3c_msgs into a set of RTIO requests
 *
 * @param r RTIO context
 * @param iodev RTIO IODev to target for the submissions
 * @param msgs Array of messages
 * @param num_msgs Number of i3c msgs in array
 *
 * @retval sqe Last submission in the queue added
 * @retval NULL Not enough memory in the context to copy the requests
 */
struct rtio_sqe *i3c_rtio_copy(struct rtio *r,
			       struct rtio_iodev *iodev,
			       const struct i3c_msg *msgs,
			       uint8_t num_msgs);

#endif /* CONFIG_I3C_RTIO */

/*
 * This needs to be after declaration of struct i3c_driver_api,
 * or else compiler complains about undefined type inside
 * the static inline API wrappers.
 */
#include <zephyr/drivers/i3c/target_device.h>

/*
 * Include High-Data-Rate (HDR) inline helper functions
 */
#include <zephyr/drivers/i3c/hdr_ddr.h>

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <zephyr/syscalls/i3c.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_H_ */

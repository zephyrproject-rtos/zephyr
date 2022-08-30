/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SD Host Controller public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDHC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDHC_H_
#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>

/**
 * @brief SDHC interface
 * @defgroup sdhc_interface SDHC interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef _cplusplus
extern "C" {
#endif


/**
 * @name SD command timeouts
 * @{
 */
#define SDHC_TIMEOUT_FOREVER (-1)
/** @} */

/**
 * @brief SD host controller command structure
 *
 * This command structure is used to send command requests to an SD
 * host controller, which will be sent to SD devices.
 */
struct sdhc_command {
	uint32_t opcode; /*!< SD Host specification CMD index */
	uint32_t arg; /*!< SD host specification argument */
	uint32_t response[4]; /*!< SD card response field */
	uint32_t response_type; /*!< Expected SD response type */
	unsigned int retries; /*!< Max number of retries */
	int timeout_ms; /*!< Command timeout in milliseconds */
};

#define SDHC_NATIVE_RESPONSE_MASK 0xF
#define SDHC_SPI_RESPONSE_TYPE_MASK 0xF0

/**
 * @brief SD host controller data structure
 *
 * This command structure is used to send data transfer requests to an SD
 * host controller, which will be sent to SD devices.
 */
struct sdhc_data {
	unsigned int block_addr; /*!< Block to start read from */
	unsigned int block_size; /*!< Block size */
	unsigned int blocks; /*!< Number of blocks */
	unsigned int bytes_xfered; /*!< populated with number of bytes sent by SDHC */
	void *data; /*!< Data to transfer or receive */
	int timeout_ms; /*!< data timeout in milliseconds */
};

/**
 * @brief SD bus mode.
 *
 * Most controllers will use push/pull, including spi, but
 * SDHC controllers that implement SD host specification can support open
 * drain mode
 */
enum sdhc_bus_mode {
	SDHC_BUSMODE_OPENDRAIN = 1,
	SDHC_BUSMODE_PUSHPULL = 2,
};

/**
 * @brief SD host controller power
 *
 * Many host controllers can control power to attached SD cards.
 * This enum allows applications to request the host controller power off
 * the SD card.
 */
enum sdhc_power {
	SDHC_POWER_OFF = 1,
	SDHC_POWER_ON = 2,
};

/**
 * @brief SD host controller bus width
 *
 * Only relevant in SD mode, SPI does not support bus width. UHS cards will
 * use 4 bit data bus, all cards start in 1 bit mode
 */
enum sdhc_bus_width {
	SDHC_BUS_WIDTH1BIT = 1U,
	SDHC_BUS_WIDTH4BIT = 4U,
	SDHC_BUS_WIDTH8BIT = 8U,
};

/**
 * @brief SD host controller timing mode
 *
 * Used by SD host controller to determine the timing of the cards attached
 * to the bus. Cards start with legacy timing, but UHS-II cards can go up to
 * SDR104.
 */
enum sdhc_timing_mode {
	SDHC_TIMING_LEGACY = 1U,
	/*!< Legacy 3.3V Mode */
	SDHC_TIMING_HS = 2U,
	/*!< Legacy High speed mode (3.3V) */
	SDHC_TIMING_SDR12 = 3U,
	/*!< Identification mode & SDR12 */
	SDHC_TIMING_SDR25 = 4U,
	/*!< High speed mode & SDR25 */
	SDHC_TIMING_SDR50 = 5U,
	/*!< SDR49 mode*/
	SDHC_TIMING_SDR104 = 6U,
	/*!< SDR104 mode */
	SDHC_TIMING_DDR50 = 7U,
	/*!< DDR50 mode */
	SDHC_TIMING_DDR52 = 8U,
	/*!< DDR52 mode */
	SDHC_TIMING_HS200 = 9U,
	/*!< HS200 mode */
	SDHC_TIMING_HS400 = 10U,
	/*!< HS400 mode */
};

/**
 * @brief SD voltage
 *
 * UHS cards can run with 1.8V signalling for improved power consumption. Legacy
 * cards may support 3.0V signalling, and all cards start at 3.3V.
 * Only relevant for SD controllers, not SPI ones.
 */
enum sd_voltage {
	SD_VOL_3_3_V = 1U,
	/*!< card operation voltage around 3.3v */
	SD_VOL_3_0_V = 2U,
	/*!< card operation voltage around 3.0v */
	SD_VOL_1_8_V = 3U,
	/*!< card operation voltage around 1.8v */
	SD_VOL_1_2_V = 4U,
	/*!< card operation voltage around 1.2v */
};

/**
 * @brief SD host controller capabilities
 *
 * SD host controller capability flags. These flags should be set by the SDHC
 * driver, using the @ref sdhc_get_host_props api.
 */
struct sdhc_host_caps {
	unsigned int timeout_clk_freq: 5;
	unsigned int _rsvd_6: 1;
	unsigned int timeout_clk_unit: 1;
	unsigned int sd_base_clk: 8;
	unsigned int max_blk_len: 2;
	unsigned int bus_8_bit_support: 1;
	unsigned int adma_2_support: 1;
	unsigned int _rsvd_20: 1;
	unsigned int high_spd_support: 1;
	unsigned int sdma_support: 1;
	unsigned int suspend_res_support: 1;
	unsigned int vol_330_support: 1;
	unsigned int vol_300_support: 1;
	unsigned int vol_180_support: 1;
	unsigned int address_64_bit_support_v4: 1;
	unsigned int address_64_bit_support_v3: 1;
	unsigned int sdio_async_interrupt_support: 1;
	unsigned int slot_type: 2;
	unsigned int sdr50_support: 1;
	unsigned int sdr104_support: 1;
	unsigned int ddr50_support: 1;
	unsigned int uhs_2_support: 1;
	unsigned int drv_type_a_support: 1;
	unsigned int drv_type_c_support: 1;
	unsigned int drv_type_d_support: 1;
	unsigned int _rsvd_39: 1;
	unsigned int retune_timer_count: 4;
	unsigned int sdr50_needs_tuning: 1;
	unsigned int retuning_mode: 2;
	unsigned int clk_multiplier: 8;
	unsigned int _rsvd_56: 3;
	unsigned int adma3_support: 1;
	unsigned int vdd2_180_support: 1;
	unsigned int _rsvd_61: 3;
	unsigned int hs200_support: 1;
	unsigned int hs400_support: 1;
};

/**
 * @brief SD host controller I/O control structure
 *
 * Controls I/O settings for the SDHC. Note that only a subset of these settings
 * apply to host controllers in SPI mode. Populate this struct, then call
 * @ref sdhc_set_io to apply I/O settings
 */
struct sdhc_io {
	enum sdhc_clock_speed clock; /*!< Clock rate */
	enum sdhc_bus_mode bus_mode; /*!< command output mode */
	enum sdhc_power power_mode; /*!< SD power supply mode */
	enum sdhc_bus_width bus_width; /*!< SD bus width */
	enum sdhc_timing_mode timing; /*!< SD bus timing */
	enum sd_driver_type driver_type; /*!< SD driver type */
	enum sd_voltage signal_voltage; /*!< IO signalling voltage (usually 1.8 or 3.3V) */
};

/**
 * @brief SD host controller properties
 *
 * Populated by the host controller using @ref sdhc_get_host_props api.
 */
struct sdhc_host_props {
	unsigned int f_max; /*!< Max bus frequency */
	unsigned int f_min; /*!< Min bus frequency */
	unsigned int power_delay; /*!< Delay to allow SD to power up or down (in ms) */
	struct sdhc_host_caps host_caps; /*!< Host capability bitfield */
	uint32_t max_current_330; /*!< Max current (in mA) at 3.3V */
	uint32_t max_current_300; /*!< Max current (in mA) at 3.0V */
	uint32_t max_current_180; /*!< Max current (in mA) at 1.8V */
	bool is_spi; /*!< Is the host using SPI mode */
};

__subsystem struct sdhc_driver_api {
	int (*reset)(const struct device *dev);
	int (*request)(const struct device *dev,
			struct sdhc_command *cmd,
			struct sdhc_data *data);
	int (*set_io)(const struct device *dev, struct sdhc_io *ios);
	int (*get_card_present)(const struct device *dev);
	int (*execute_tuning)(const struct device *dev);
	int (*card_busy)(const struct device *dev);
	int (*get_host_props)(const struct device *dev,
				struct sdhc_host_props *props);
};

/**
 * @brief reset SDHC controller state
 *
 * Used when the SDHC has encountered an error. Resetting the SDHC controller
 * should clear all errors on the SDHC, but does not necessarily reset I/O
 * settings to boot (this can be done with @ref sdhc_set_io)
 *
 * @param dev: SD host controller device
 * @retval 0 reset succeeded
 * @retval -ETIMEDOUT: controller reset timed out
 * @retval -EIO: reset failed
 */
__syscall int sdhc_hw_reset(const struct device *dev);

static inline int z_impl_sdhc_hw_reset(const struct device *dev)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->reset) {
		return -ENOSYS;
	}

	return api->reset(dev);
}


/**
 * @brief Send command to SDHC
 *
 * Sends a command to the SD host controller, which will send this command to
 * attached SD cards.
 * @param dev: SDHC device
 * @param cmd: SDHC command
 * @param data: SDHC data. Leave NULL to send SD command without data.
 * @retval 0 command was sent successfully
 * @retval -ETIMEDOUT command timed out while sending
 * @retval -ENOTSUP host controller does not support command
 * @retval -EIO: I/O error
 */
__syscall int sdhc_request(const struct device *dev, struct sdhc_command *cmd,
		struct sdhc_data *data);

static inline int z_impl_sdhc_request(const struct device *dev,
		struct sdhc_command *cmd, struct sdhc_data *data)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->request) {
		return -ENOSYS;
	}

	return api->request(dev, cmd, data);
}

/**
 * @brief set I/O properties of SDHC
 *
 * I/O properties should be reconfigured when the card has been sent a command
 * to change its own SD settings. This function can also be used to toggle
 * power to the SD card.
 * @param dev: SDHC device
 * @param io: I/O properties
 * @return 0 I/O was configured correctly
 * @return -ENOTSUP controller does not support these I/O settings
 * @return -EIO controller could not configure I/O settings
 */
__syscall int sdhc_set_io(const struct device *dev, struct sdhc_io *io);

static inline int z_impl_sdhc_set_io(const struct device *dev,
	struct sdhc_io *io)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->set_io) {
		return -ENOSYS;
	}

	return api->set_io(dev, io);
}

/**
 * @brief check for SDHC card presence
 *
 * Checks if card is present on the SD bus. Note that if a controller
 * requires cards be powered up to detect presence, it should do so in
 * this function.
 * @param dev: SDHC device
 * @retval 1 card is present
 * @retval 0 card is not present
 * @retval -EIO I/O error
 */
__syscall int sdhc_card_present(const struct device *dev);

static inline int z_impl_sdhc_card_present(const struct device *dev)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->get_card_present) {
		return -ENOSYS;
	}

	return api->get_card_present(dev);
}


/**
 * @brief run SDHC tuning
 *
 * SD cards require signal tuning for UHS modes SDR104 and SDR50. This function
 * allows an application to request the SD host controller to tune the card.
 * @param dev: SDHC device
 * @retval 0 tuning succeeded, card is ready for commands
 * @retval -ETIMEDOUT: tuning failed after timeout
 * @retval -ENOTSUP: controller does not support tuning
 * @retval -EIO: I/O error while tuning
 */
__syscall int sdhc_execute_tuning(const struct device *dev);

static inline int z_impl_sdhc_execute_tuning(const struct device *dev)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->execute_tuning) {
		return -ENOSYS;
	}

	return api->execute_tuning(dev);
}

/**
 * @brief check if SD card is busy
 *
 * This check should generally be implemented as checking the line level of the
 * DAT[0:3] lines of the SD bus. No SD commands need to be sent, the controller
 * simply needs to report the status of the SD bus.
 * @param dev: SDHC device
 * @retval 0 card is not busy
 * @retval 1 card is busy
 * @retval -EIO I/O error
 */
__syscall int sdhc_card_busy(const struct device *dev);

static inline int z_impl_sdhc_card_busy(const struct device *dev)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->card_busy) {
		return -ENOSYS;
	}

	return api->card_busy(dev);
}


/**
 * @brief Get SD host controller properties
 *
 * Gets host properties from the host controller. Host controller should
 * initialize all values in the @ref sdhc_host_props structure provided.
 * @param dev: SDHC device
 * @param props property structure to be filled by sdhc driver
 * @retval 0 function succeeded.
 * @retval -ENOTSUP host controller does not support this call
 */
__syscall int sdhc_get_host_props(const struct device *dev,
	struct sdhc_host_props *props);

static inline int z_impl_sdhc_get_host_props(const struct device *dev,
	struct sdhc_host_props *props)
{
	const struct sdhc_driver_api *api =
		(const struct sdhc_driver_api *)dev->api;

	if (!api->get_host_props) {
		return -ENOSYS;
	}

	return api->get_host_props(dev, props);
}

/**
 * @}
 */

#ifdef _cplusplus
}
#endif

#include <syscalls/sdhc.h>
#endif /* ZEPHYR_INCLUDE_DRIVERS_SDHC_H_ */

/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for MIPI-DSI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_H_
#define ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_H_

/**
 * @brief MIPI-DSI driver APIs
 * @defgroup mipi_dsi_interface MIPI-DSI driver APIs
 * @ingroup io_interfaces
 * @{
 */
#include <errno.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/dt-bindings/mipi_dsi/mipi_dsi.h>

#ifdef __cplusplus
extern "C" {
#endif

/** MIPI-DSI display timings. */
struct mipi_dsi_timings {
	/** Horizontal active video. */
	uint32_t hactive;
	/** Horizontal front porch. */
	uint32_t hfp;
	/** Horizontal back porch. */
	uint32_t hbp;
	/** Horizontal sync length. */
	uint32_t hsync;
	/** Vertical active video. */
	uint32_t vactive;
	/** Vertical front porch. */
	uint32_t vfp;
	/** Vertical back porch. */
	uint32_t vbp;
	/** Vertical sync length. */
	uint32_t vsync;
};

/**
 * @name MIPI-DSI Device mode flags.
 * @{
 */

/** Video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/** Video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/** Video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/** Enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/** Enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/** Disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/** Disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/** Disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/** Flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/** Disable EoT packets in HS mode */
#define MIPI_DSI_MODE_EOT_PACKET	BIT(9)
/** Device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/** Transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

/** @} */

/** MIPI-DSI device. */
struct mipi_dsi_device {
	/** Number of data lanes. */
	uint8_t data_lanes;
	/** Display timings. */
	struct mipi_dsi_timings timings;
	/** Pixel format. */
	uint32_t pixfmt;
	/** Mode flags. */
	uint32_t mode_flags;
};

/*
 * Per message flag to indicate the message must be sent
 * using Low Power Mode instead of controller default.
 */
#define MIPI_DSI_MSG_USE_LPM BIT(0x0)

/** MIPI-DSI read/write message. */
struct mipi_dsi_msg {
	/** Payload data type. */
	uint8_t type;
	/** Flags controlling message transmission. */
	uint16_t flags;
	/** Command (only for DCS) */
	uint8_t cmd;
	/** Transmission buffer length. */
	size_t tx_len;
	/** Transmission buffer. */
	const void *tx_buf;
	/** Reception buffer length. */
	size_t rx_len;
	/** Reception buffer. */
	void *rx_buf;
};

/** MIPI-DSI host driver API. */
__subsystem struct mipi_dsi_driver_api {
	int (*attach)(const struct device *dev, uint8_t channel,
		      const struct mipi_dsi_device *mdev);
	ssize_t (*transfer)(const struct device *dev, uint8_t channel,
			    struct mipi_dsi_msg *msg);
	int (*detach)(const struct device *dev, uint8_t channel,
		      const struct mipi_dsi_device *mdev);
};

/**
 * @brief Attach a new device to the MIPI-DSI bus.
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param mdev MIPI-DSI device description.
 *
 * @return 0 on success, negative on error
 */
static inline int mipi_dsi_attach(const struct device *dev,
				  uint8_t channel,
				  const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_driver_api *api = (const struct mipi_dsi_driver_api *)dev->api;

	return api->attach(dev, channel, mdev);
}

/**
 * @brief Transfer data to/from a device attached to the MIPI-DSI bus.
 *
 * @param dev MIPI-DSI device.
 * @param channel Device channel (VID).
 * @param msg Message.
 *
 * @return Size of the transferred data on success, negative on error.
 */
static inline ssize_t mipi_dsi_transfer(const struct device *dev,
					uint8_t channel,
					struct mipi_dsi_msg *msg)
{
	const struct mipi_dsi_driver_api *api = (const struct mipi_dsi_driver_api *)dev->api;

	return api->transfer(dev, channel, msg);
}

/**
 * @brief MIPI-DSI generic read.
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param params Buffer containing request parameters.
 * @param nparams Number of parameters.
 * @param buf Buffer where read data will be stored.
 * @param len Length of the reception buffer.
 *
 * @return Size of the read data on success, negative on error.
 */
ssize_t mipi_dsi_generic_read(const struct device *dev, uint8_t channel,
			      const void *params, size_t nparams,
			      void *buf, size_t len);

/**
 * @brief MIPI-DSI generic write.
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param buf Transmission buffer.
 * @param len Length of the transmission buffer
 *
 * @return Size of the written data on success, negative on error.
 */
ssize_t mipi_dsi_generic_write(const struct device *dev, uint8_t channel,
			       const void *buf, size_t len);

/**
 * @brief MIPI-DSI DCS read.
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param cmd DCS command.
 * @param buf Buffer where read data will be stored.
 * @param len Length of the reception buffer.
 *
 * @return Size of the read data on success, negative on error.
 */
ssize_t mipi_dsi_dcs_read(const struct device *dev, uint8_t channel,
			  uint8_t cmd, void *buf, size_t len);

/**
 * @brief MIPI-DSI DCS write.
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param cmd DCS command.
 * @param buf Transmission buffer.
 * @param len Length of the transmission buffer
 *
 * @return Size of the written data on success, negative on error.
 */
ssize_t mipi_dsi_dcs_write(const struct device *dev, uint8_t channel,
			   uint8_t cmd, const void *buf, size_t len);


/**
 * @brief Detach a device from the MIPI-DSI bus
 *
 * @param dev MIPI-DSI host device.
 * @param channel Device channel (VID).
 * @param mdev MIPI-DSI device description.
 *
 * @return 0 on success, negative on error
 */
static inline int mipi_dsi_detach(const struct device *dev,
				  uint8_t channel,
				  const struct mipi_dsi_device *mdev)
{
	const struct mipi_dsi_driver_api *api = (const struct mipi_dsi_driver_api *)dev->api;

	if (api->detach == NULL) {
		return -ENOSYS;
	}

	return api->detach(dev, channel, mdev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_H_ */

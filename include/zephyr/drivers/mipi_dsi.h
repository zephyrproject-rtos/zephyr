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
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/mipi_dsi/mipi_dsi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name MIPI-DSI DCS (Display Command Set)
 * @{
 */

#define MIPI_DCS_NOP                        0x00U
#define MIPI_DCS_SOFT_RESET                 0x01U
#define MIPI_DCS_GET_COMPRESSION_MODE       0x03U
#define MIPI_DCS_GET_DISPLAY_ID             0x04U
#define MIPI_DCS_GET_RED_CHANNEL            0x06U
#define MIPI_DCS_GET_GREEN_CHANNEL          0x07U
#define MIPI_DCS_GET_BLUE_CHANNEL           0x08U
#define MIPI_DCS_GET_DISPLAY_STATUS         0x09U
#define MIPI_DCS_GET_POWER_MODE             0x0AU
#define MIPI_DCS_GET_ADDRESS_MODE           0x0BU
#define MIPI_DCS_GET_PIXEL_FORMAT           0x0CU
#define MIPI_DCS_GET_DISPLAY_MODE           0x0DU
#define MIPI_DCS_GET_SIGNAL_MODE            0x0EU
#define MIPI_DCS_GET_DIAGNOSTIC_RESULT      0x0FU
#define MIPI_DCS_ENTER_SLEEP_MODE           0x10U
#define MIPI_DCS_EXIT_SLEEP_MODE            0x11U
#define MIPI_DCS_ENTER_PARTIAL_MODE         0x12U
#define MIPI_DCS_ENTER_NORMAL_MODE          0x13U
#define MIPI_DCS_EXIT_INVERT_MODE           0x20U
#define MIPI_DCS_ENTER_INVERT_MODE          0x21U
#define MIPI_DCS_SET_GAMMA_CURVE            0x26U
#define MIPI_DCS_SET_DISPLAY_OFF            0x28U
#define MIPI_DCS_SET_DISPLAY_ON             0x29U
#define MIPI_DCS_SET_COLUMN_ADDRESS         0x2AU
#define MIPI_DCS_SET_PAGE_ADDRESS           0x2BU
#define MIPI_DCS_WRITE_MEMORY_START         0x2CU
#define MIPI_DCS_WRITE_LUT                  0x2DU
#define MIPI_DCS_READ_MEMORY_START          0x2EU
#define MIPI_DCS_SET_PARTIAL_ROWS           0x30U
#define MIPI_DCS_SET_PARTIAL_COLUMNS        0x31U
#define MIPI_DCS_SET_SCROLL_AREA            0x33U
#define MIPI_DCS_SET_TEAR_OFF               0x34U
#define MIPI_DCS_SET_TEAR_ON                0x35U
#define MIPI_DCS_SET_ADDRESS_MODE           0x36U
#define MIPI_DCS_SET_SCROLL_START           0x37U
#define MIPI_DCS_EXIT_IDLE_MODE             0x38U
#define MIPI_DCS_ENTER_IDLE_MODE            0x39U
#define MIPI_DCS_SET_PIXEL_FORMAT           0x3AU
#define MIPI_DCS_WRITE_MEMORY_CONTINUE      0x3CU
#define MIPI_DCS_SET_3D_CONTROL             0x3DU
#define MIPI_DCS_READ_MEMORY_CONTINUE       0x3EU
#define MIPI_DCS_GET_3D_CONTROL             0x3FU
#define MIPI_DCS_SET_VSYNC_TIMING           0x40U
#define MIPI_DCS_SET_TEAR_SCANLINE          0x44U
#define MIPI_DCS_GET_SCANLINE               0x45U
#define MIPI_DCS_SET_DISPLAY_BRIGHTNESS     0x51U
#define MIPI_DCS_GET_DISPLAY_BRIGHTNESS     0x52U
#define MIPI_DCS_WRITE_CONTROL_DISPLAY      0x53U
#define MIPI_DCS_GET_CONTROL_DISPLAY        0x54U
#define MIPI_DCS_WRITE_POWER_SAVE           0x55U
#define MIPI_DCS_GET_POWER_SAVE             0x56U
#define MIPI_DCS_SET_CABC_MIN_BRIGHTNESS    0x5EU
#define MIPI_DCS_GET_CABC_MIN_BRIGHTNESS    0x5FU
#define MIPI_DCS_READ_DDB_START             0xA1U
#define MIPI_DCS_READ_DDB_CONTINUE          0xA8U

#define MIPI_DCS_PIXEL_FORMAT_24BIT         0x77
#define MIPI_DCS_PIXEL_FORMAT_18BIT         0x66
#define MIPI_DCS_PIXEL_FORMAT_16BIT         0x55
#define MIPI_DCS_PIXEL_FORMAT_12BIT         0x33
#define MIPI_DCS_PIXEL_FORMAT_8BIT          0x22
#define MIPI_DCS_PIXEL_FORMAT_3BIT          0x11

/** @} */

/**
 * @name MIPI-DSI Address mode register fields.
 * @{
 */

#define MIPI_DCS_ADDRESS_MODE_MIRROR_Y      BIT(7)
#define MIPI_DCS_ADDRESS_MODE_MIRROR_X      BIT(6)
#define MIPI_DCS_ADDRESS_MODE_SWAP_XY       BIT(5)
#define MIPI_DCS_ADDRESS_MODE_REFRESH_BT    BIT(4)
#define MIPI_DCS_ADDRESS_MODE_BGR           BIT(3)
#define MIPI_DCS_ADDRESS_MODE_LATCH_RL      BIT(2)
#define MIPI_DCS_ADDRESS_MODE_FLIP_X        BIT(1)
#define MIPI_DCS_ADDRESS_MODE_FLIP_Y        BIT(0)

/** @} */

/**
 * @name MIPI-DSI Processor-to-Peripheral transaction types.
 * @{
 */

#define MIPI_DSI_V_SYNC_START				0x01U
#define MIPI_DSI_V_SYNC_END				0x11U
#define MIPI_DSI_H_SYNC_START				0x21U
#define MIPI_DSI_H_SYNC_END				0x31U
#define MIPI_DSI_COLOR_MODE_OFF				0x02U
#define MIPI_DSI_COLOR_MODE_ON				0x12U
#define MIPI_DSI_SHUTDOWN_PERIPHERAL			0x22U
#define MIPI_DSI_TURN_ON_PERIPHERAL			0x32U
#define MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM		0x03U
#define MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM		0x13U
#define MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM		0x23U
#define MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM		0x04U
#define MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM		0x14U
#define MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM		0x24U
#define MIPI_DSI_DCS_SHORT_WRITE			0x05U
#define MIPI_DSI_DCS_SHORT_WRITE_PARAM			0x15U
#define MIPI_DSI_DCS_READ				0x06U
#define MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE		0x37U
#define MIPI_DSI_END_OF_TRANSMISSION			0x08U
#define MIPI_DSI_NULL_PACKET				0x09U
#define MIPI_DSI_BLANKING_PACKET			0x19U
#define MIPI_DSI_GENERIC_LONG_WRITE			0x29U
#define MIPI_DSI_DCS_LONG_WRITE				0x39U
#define MIPI_DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20	0x0CU
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR24		0x1CU
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR16		0x2CU
#define MIPI_DSI_PACKED_PIXEL_STREAM_30			0x0DU
#define MIPI_DSI_PACKED_PIXEL_STREAM_36			0x1DU
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR12		0x3DU
#define MIPI_DSI_PACKED_PIXEL_STREAM_16			0x0EU
#define MIPI_DSI_PACKED_PIXEL_STREAM_18			0x1EU
#define MIPI_DSI_PIXEL_STREAM_3BYTE_18			0x2EU
#define MIPI_DSI_PACKED_PIXEL_STREAM_24			0x3EU

/** @} */

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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MIPI_DSI_H_ */

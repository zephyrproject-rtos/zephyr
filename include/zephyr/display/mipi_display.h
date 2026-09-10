/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Display definitions for MIPI devices
 * @ingroup mipi_interface
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_MIPI_DISPLAY_H_
#define ZEPHYR_INCLUDE_DISPLAY_MIPI_DISPLAY_H_

/**
 * @brief MIPI Display definitions
 * @defgroup mipi_interface MIPI Display interface
 * @ingroup display_interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name MIPI-DSI DCS (Display Command Set)
 * @{
 */

/** NOP */
#define MIPI_DCS_NOP                        0x00U
/** Soft Reset */
#define MIPI_DCS_SOFT_RESET                 0x01U
/** Get Compression Mode */
#define MIPI_DCS_GET_COMPRESSION_MODE       0x03U
/** Get Display ID */
#define MIPI_DCS_GET_DISPLAY_ID             0x04U
/** Get Red Channel */
#define MIPI_DCS_GET_RED_CHANNEL            0x06U
/** Get Green Channel */
#define MIPI_DCS_GET_GREEN_CHANNEL          0x07U
/** Get Blue Channel */
#define MIPI_DCS_GET_BLUE_CHANNEL           0x08U
/** Get Display Status */
#define MIPI_DCS_GET_DISPLAY_STATUS         0x09U
/** Get Power Mode */
#define MIPI_DCS_GET_POWER_MODE             0x0AU
/** Get Address Mode */
#define MIPI_DCS_GET_ADDRESS_MODE           0x0BU
/** Get Pixel Format */
#define MIPI_DCS_GET_PIXEL_FORMAT           0x0CU
/** Get Display Mode */
#define MIPI_DCS_GET_DISPLAY_MODE           0x0DU
/** Get Signal Mode */
#define MIPI_DCS_GET_SIGNAL_MODE            0x0EU
/** Get Diagnostic Result */
#define MIPI_DCS_GET_DIAGNOSTIC_RESULT      0x0FU
/** Enter Sleep Mode */
#define MIPI_DCS_ENTER_SLEEP_MODE           0x10U
/** Exit Sleep Mode */
#define MIPI_DCS_EXIT_SLEEP_MODE            0x11U
/** Enter Partial Mode */
#define MIPI_DCS_ENTER_PARTIAL_MODE         0x12U
/** Enter Normal Mode */
#define MIPI_DCS_ENTER_NORMAL_MODE          0x13U
/** Exit Invert Mode */
#define MIPI_DCS_EXIT_INVERT_MODE           0x20U
/** Enter Invert Mode */
#define MIPI_DCS_ENTER_INVERT_MODE          0x21U
/** Set Gamma Curve */
#define MIPI_DCS_SET_GAMMA_CURVE            0x26U
/** Set Display Off */
#define MIPI_DCS_SET_DISPLAY_OFF            0x28U
/** Set Display On */
#define MIPI_DCS_SET_DISPLAY_ON             0x29U
/** Set Column Address */
#define MIPI_DCS_SET_COLUMN_ADDRESS         0x2AU
/** Set Page Address */
#define MIPI_DCS_SET_PAGE_ADDRESS           0x2BU
/** Write Memory Start */
#define MIPI_DCS_WRITE_MEMORY_START         0x2CU
/** Write LUT */
#define MIPI_DCS_WRITE_LUT                  0x2DU
/** Read Memory Start */
#define MIPI_DCS_READ_MEMORY_START          0x2EU
/** Set Partial Rows */
#define MIPI_DCS_SET_PARTIAL_ROWS           0x30U
/** Set Partial Columns */
#define MIPI_DCS_SET_PARTIAL_COLUMNS        0x31U
/** Set Scroll Area */
#define MIPI_DCS_SET_SCROLL_AREA            0x33U
/** Set Tear Off */
#define MIPI_DCS_SET_TEAR_OFF               0x34U
/** Set Tear On */
#define MIPI_DCS_SET_TEAR_ON                0x35U
/** Set Address Mode */
#define MIPI_DCS_SET_ADDRESS_MODE           0x36U
/** Set Scroll Start */
#define MIPI_DCS_SET_SCROLL_START           0x37U
/** Exit Idle Mode */
#define MIPI_DCS_EXIT_IDLE_MODE             0x38U
/** Enter Idle Mode */
#define MIPI_DCS_ENTER_IDLE_MODE            0x39U
/** Set Pixel Format */
#define MIPI_DCS_SET_PIXEL_FORMAT           0x3AU
/** Write Memory Continue */
#define MIPI_DCS_WRITE_MEMORY_CONTINUE      0x3CU
/** Set 3D Control */
#define MIPI_DCS_SET_3D_CONTROL             0x3DU
/** Read Memory Continue */
#define MIPI_DCS_READ_MEMORY_CONTINUE       0x3EU
/** Get 3D Control */
#define MIPI_DCS_GET_3D_CONTROL             0x3FU
/** Set VSync Timing */
#define MIPI_DCS_SET_VSYNC_TIMING           0x40U
/** Set Tear Scanline */
#define MIPI_DCS_SET_TEAR_SCANLINE          0x44U
/** Get Scanline */
#define MIPI_DCS_GET_SCANLINE               0x45U
/** Set Display Brightness */
#define MIPI_DCS_SET_DISPLAY_BRIGHTNESS     0x51U
/** Get Display Brightness */
#define MIPI_DCS_GET_DISPLAY_BRIGHTNESS     0x52U
/** Write Control Display */
#define MIPI_DCS_WRITE_CONTROL_DISPLAY      0x53U
/** Get Control Display */
#define MIPI_DCS_GET_CONTROL_DISPLAY        0x54U
/** Write Power Save */
#define MIPI_DCS_WRITE_POWER_SAVE           0x55U
/** Get Power Save */
#define MIPI_DCS_GET_POWER_SAVE             0x56U
/** Set CABC Min Brightness */
#define MIPI_DCS_SET_CABC_MIN_BRIGHTNESS    0x5EU
/** Get CABC Min Brightness */
#define MIPI_DCS_GET_CABC_MIN_BRIGHTNESS    0x5FU
/** Read DDB Start */
#define MIPI_DCS_READ_DDB_START             0xA1U
/** Read DDB Continue */
#define MIPI_DCS_READ_DDB_CONTINUE          0xA8U

/** 24-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_24BIT         0x77
/** 18-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_18BIT         0x66
/** 16-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_16BIT         0x55
/** 12-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_12BIT         0x33
/** 8-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_8BIT          0x22
/** 3-bit Pixel Format */
#define MIPI_DCS_PIXEL_FORMAT_3BIT          0x11

/** @} */

/**
 * @name MIPI-DSI Address mode register fields.
 * @{
 */

/** Mirror Y */
#define MIPI_DCS_ADDRESS_MODE_MIRROR_Y      BIT(7)
/** Mirror X */
#define MIPI_DCS_ADDRESS_MODE_MIRROR_X      BIT(6)
/** Swap XY */
#define MIPI_DCS_ADDRESS_MODE_SWAP_XY       BIT(5)
/** Refresh Bottom to Top */
#define MIPI_DCS_ADDRESS_MODE_REFRESH_BT    BIT(4)
/** BGR Order */
#define MIPI_DCS_ADDRESS_MODE_BGR           BIT(3)
/** Latch Right to Left */
#define MIPI_DCS_ADDRESS_MODE_LATCH_RL      BIT(2)
/** Flip X */
#define MIPI_DCS_ADDRESS_MODE_FLIP_X        BIT(1)
/** Flip Y */
#define MIPI_DCS_ADDRESS_MODE_FLIP_Y        BIT(0)

/** @} */

/**
 * @name MIPI-DSI Processor-to-Peripheral transaction types.
 * @{
 */

/** Sync Event, V Sync Start */
#define MIPI_DSI_V_SYNC_START				0x01U
/** Sync Event, V Sync End */
#define MIPI_DSI_V_SYNC_END				0x11U
/** Sync Event, H Sync Start */
#define MIPI_DSI_H_SYNC_START				0x21U
/** Sync Event, H Sync End */
#define MIPI_DSI_H_SYNC_END				0x31U
/** Color Mode Off Command */
#define MIPI_DSI_COLOR_MODE_OFF				0x02U
/** Color Mode On Command */
#define MIPI_DSI_COLOR_MODE_ON				0x12U
/** Shutdown Peripheral Command */
#define MIPI_DSI_SHUTDOWN_PERIPHERAL			0x22U
/** Turn On Peripheral Command */
#define MIPI_DSI_TURN_ON_PERIPHERAL			0x32U
/** Generic Short WRITE Packet with 0 parameters */
#define MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM		0x03U
/** Generic Short WRITE Packet with 1 parameter */
#define MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM		0x13U
/** Generic Short WRITE Packet with 2 parameters */
#define MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM		0x23U
/** Generic READ Request with 0 parameters */
#define MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM		0x04U
/** Generic READ Request with 1 parameter */
#define MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM		0x14U
/** Generic READ Request with 2 parameters */
#define MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM		0x24U
/** DCS Short Write Command with 0 parameters */
#define MIPI_DSI_DCS_SHORT_WRITE			0x05U
/** DCS Short Write Command with 1 parameter */
#define MIPI_DSI_DCS_SHORT_WRITE_PARAM			0x15U
/** DCS Read Request with 0 parameters */
#define MIPI_DSI_DCS_READ				0x06U
/** Set Maximum Return Packet Size Command */
#define MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE		0x37U
/** End of Transmission Packet (EoTp) */
#define MIPI_DSI_END_OF_TRANSMISSION			0x08U
/** Null Packet (Long) */
#define MIPI_DSI_NULL_PACKET				0x09U
/** Blanking Packet (Long) */
#define MIPI_DSI_BLANKING_PACKET			0x19U
/** Generic Long Write */
#define MIPI_DSI_GENERIC_LONG_WRITE			0x29U
/** DCS Long Write / write_LUT Command */
#define MIPI_DSI_DCS_LONG_WRITE				0x39U
/** Loosely Packed Pixel Stream, 20-bit YCbCr 4:2:2 Format */
#define MIPI_DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20	0x0CU
/** Packed Pixel Stream, 24-bit YCbCr 4:2:2 Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR24		0x1CU
/** Packed Pixel Stream, 16-bit YCbCr 4:2:2 Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR16		0x2CU
/** Packed Pixel Stream, 30-bit Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_30			0x0DU
/** Packed Pixel Stream, 36-bit Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_36			0x1DU
/** Packed Pixel Stream, 12-bit YCbCr 4:2:0 Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR12		0x3DU
/** Packed Pixel Stream, 16-bit Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_16			0x0EU
/** Packed Pixel Stream, 18-bit Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_18			0x1EU
/** Pixel Stream, 18-bit Format in Three Bytes */
#define MIPI_DSI_PIXEL_STREAM_3BYTE_18			0x2EU
/** Packed Pixel Stream, 24-bit Format */
#define MIPI_DSI_PACKED_PIXEL_STREAM_24			0x3EU

/** @} */


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DISPLAY_MIPI_DISPLAY_H_ */

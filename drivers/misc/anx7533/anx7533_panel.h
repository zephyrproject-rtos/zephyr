/*
 * Copyright(c) 2017, Analogix Semiconductor. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ANX_CHICAGO_PANEL_H__
#define __ANX_CHICAGO_PANEL_H__

#include <zephyr/device.h>

#define CHICAGO_FEATURE_EDID_AUTO 1

#define DCS_LONG_PACKET_TIMEOUT		100

/******************************************************************************
Description: Defined for Constant
******************************************************************************/
#define PANEL_SLEEP_IN_DELAY 70    // Wait > 4 frame (60Hz = 66.666ms)
#define PANEL_SLEEP_OUT_DELAY 110  // Wait >= 100ms for Sharp X02
#define PANEL_DISPLAY_ON_DELAY 60  // Wait >= 40ms
#define PANEL_DISPLAY_OFF_DELAY 20 // Wait >= 1 frame (60Hz = 16.666ms)

/******************************************************************************
Description: Customized panel set-up for Calico.
******************************************************************************/
// Panel timing for ONE panel
// using v1.5 timing parameters (JBD4020 min..typ..max values)
//
#define PANEL_FRAME_RATE 60
#define PANEL_H_ACTIVE 640  //2..640..640
#define PANEL_V_ACTIVE 480  //2..480..480
#define PANEL_VFP 8         //2..30
#define PANEL_VSYNC 2       //2..2
#define PANEL_VBP 2         //2..2
#define PANEL_HFP 20        //4..20
#define PANEL_HSYNC 4       //4..4
#define PANEL_HBP 20        //4..20

// Other MIPI TX difinition
#define PANEL_COUNT 2
#define MIPI_TOTAL_PORT 2
#define MIPI_LANE_NUMBER 1
#define MIPI_DSC_STATUS DSC_NO_DSC
#define MIPI_VIDEO_MODE VIDEOMODE_SIDE
#define MIPI_DISPLAY_EYE DISPLAY_LEFT
#define MIPI_TRANSMIT_MODE MOD_BURST
#define PANEL_M_MULTIPLY 120 //* 1.2; use 123 for Sharp X02 panel

/******************************************************************************
Description: Defined for Constant
******************************************************************************/
#define PANEL_POWER_SUPPLY_ON 1
#define PANEL_POWER_SUPPLY_OFF 0

/******************************************************************************
Description: Defined for Constant
******************************************************************************/
#define PANEL_TURN_ON				1
#define PANEL_TURN_OFF				0

// for DCS 
#define PANEL_SLEEP_OUT				1
#define PANEL_SLEEP_IN				0
#define PANEL_DISPLAY_ON			1
#define PANEL_DISPLAY_OFF			0

/******************************************************************************
Description: Display Command Set List
******************************************************************************/
// Packet Data Type definition
#define DATASHORT_VSYNC_START		0x01
#define DATASHORT_VSYNC_END		0x11
#define DATASHORT_HSYNC_START		0x21
#define DATASHORT_HSYNC_END		0x31
#define DATASHORT_COMPRESSION_MODE	0x07
#define DATASHORT_EoTp			0x08
#define DATASHORT_CM_OFF		0x02
#define DATASHORT_CM_ON			0x12
#define DATASHORT_SHUTDOWN_PERIPH	0x22
#define DATASHORT_TURNON_PERIPH		0x32
#define DATASHORT_GEN_WRITE_0		0x03
#define DATASHORT_GEN_WRITE_1		0x13
#define DATASHORT_GEN_WRITE_2		0x23
#define DATASHORT_GEN_READ_0		0x04
#define DATASHORT_GEN_READ_1		0x14
#define DATASHORT_GEN_READ_2		0x24
#define DATASHORT_DCS_WRITE_0		0x05
#define DATASHORT_DCS_WRITE_1		0x15
#define DATASHORT_DCS_READ_0		0x06
#define DATASHORT_EXE_QUEUE		0x16
#define DATASHORT_SET_RET_SIZE		0x37
#define DATALONG_NULL_PACKET		0x09
#define DATALONG_BLANK_PACKET		0x19
#define DATALONG_GEN_WRITE		0x29
#define DATALONG_DCS_WRITE		0x39
#define DATALONG_PIC_PARAMETER		0x0A
#define DATALONG_COMPRESSED_STREAM	0x0B
#define DATALONG_30RGB_STREAM		0x0D
#define DATALONG_24RGB_STREAM		0x3E

/******************************************************************************
Description: DCS Command definition
******************************************************************************/
// DCS Command definition
#define	DCS_nop				0x00
#define	DCS_soft_reset			0x01
#define	DCS_get_compression_mode	0x03
#define	DCS_get_error_count_on_DSI	0x05
#define	DCS_get_red_channel		0x06
#define	DCS_get_green_channel		0x07
#define	DCS_get_blue_channel		0x08
#define	DCS_get_power_mode		0x0A
#define	DCS_get_address_mode		0x0B
#define	DCS_get_pixel_format		0x0C
#define	DCS_get_display_mode		0x0D
#define	DCS_get_signal_mode		0x0E
#define	DCS_get_diagnostic_result	0x0F
#define	DCS_enter_sleep_mode		0x10
#define	DCS_exit_sleep_mode		0x11
#define	DCS_enter_partial_mode		0x12
#define	DCS_enter_normal_mode		0x13
#define	DCS_get_image_checksum_rgb	0x14
#define	DCS_get_image_checksum_ct	0x15
#define	DCS_exit_invert_mode		0x20
#define	DCS_enter_invert_mode		0x21
#define	DCS_set_gamma_curve		0x26
#define	DCS_set_display_off		0x28
#define	DCS_set_display_on		0x29
#define	DCS_set_column_address		0x2A
#define	DCS_set_page_address		0x2B
#define	DCS_write_memory_start		0x2C
#define	DCS_write_LUT			0x2D
#define	DCS_read_memory_start		0x2E
#define	DCS_set_partial_rows		0x30
#define	DCS_set_partial_columns		0x31
#define	DCS_set_scroll_area		0x33
#define	DCS_set_tear_off		0x34
#define	DCS_set_tear_on			0x35
#define	DCS_set_address_mode		0x36
#define	DCS_set_scroll_start		0x37
#define	DCS_exit_idle_mode		0x38
#define	DCS_enter_idle_mode		0x39
#define	DCS_set_pixel_format		0x3A
#define	DCS_write_memory_continue	0x3C
#define	DCS_set_3D_control		0x3D
#define	DCS_read_memory_continue	0x3E
#define	DCS_get_3D_control		0x3F
#define	DCS_set_vsync_timing		0x40
#define	DCS_set_tear_scanline		0x44
#define	DCS_get_scanline		0x45
#define	DCS_set_display_brightness	0x51
#define	DCS_get_display_brightness	0x52
#define	DCS_write_control_display	0x53
#define	DCS_get_control_display		0x54
#define	DCS_write_power_save		0x55
#define	DCS_get_power_save		0x56
#define	DCS_set_CABC_min_brightness	0x5E
#define	DCS_get_CABC_min_brightness	0x5F
#define	DCS_read_DDB_start		0xA1
#define	DCS_read_PPS_start		0xA2
#define	DCS_read_DDB_continue		0xA8
#define	DCS_read_PPS_continue		0xA9

/******************************************************************************
Description: Short packet structure
******************************************************************************/
// Short packet
struct packet_short
{
	uint8_t	mipi_port;	// Selected MIPI port
	uint8_t	data_type;	// Packet Data Type
	uint8_t	param1;		// Parameter 1
	uint8_t	param2;		// Parameter 2
	uint8_t	*pData;		// Read data for Read Command, NULL for Write Command
};

/******************************************************************************
Description: Long packet structure
******************************************************************************/
// Long packet
struct packet_long
{
	uint8_t	mipi_port;	// Selected MIPI port
	uint8_t	data_type;	// Packet Data Type
	uint16_t	word_count;	// how many bytes are in packet payload
	uint8_t	*pData;		// PACKET DATA (Payload)
};

/******************************************************************************
Description: Panel type structure
******************************************************************************/
// Panel type
struct panel_param
{
	uint16_t	h_active;	// Horizon resolution
	uint16_t	v_active;	// Vertical resolution
	uint16_t	vfp;		// Vertical front proch
	uint16_t	vsync;		// Vertical sync width
	uint16_t	vbp;		// Vertical back proch
	uint16_t	hfp;		// Horizon front proch
	uint16_t	hsync;		// Horizon sync width
	uint16_t	hbp;		// Horizon back proch
};

/******************************************************************************
Description: Panel transmit mode
******************************************************************************/
// Panel transmit mode
enum pane_trans_mode {
	MOD_NON_BURST_PULSES,
	MOD_NON_BURST_EVENTS,
	MOD_BURST,
};

/******************************************************************************
Description: DSC enum
******************************************************************************/
enum dsc_status {
	DSC_NONE,
	DSC_NO_DSC,
	DSC_ONE_TO_THREE,
};

/******************************************************************************
Description: MIPI Video Mode emun
******************************************************************************/
enum video_mode {
	VIDEOMODE_NONE,
	VIDEOMODE_ONE,
	VIDEOMODE_SIDE,
	VIDEOMODE_STACKED,
};

/******************************************************************************
Description: Display eye sequence emun
******************************************************************************/
enum display_eye {
	DISPLAY_NONE,
	DISPLAY_LEFT,
	DISPLAY_RIGHT
};

#endif  /* __ANX_CHICAGO_PANEL_H__ */


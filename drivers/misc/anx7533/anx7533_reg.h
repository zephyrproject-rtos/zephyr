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

#ifndef __ANX7533_CHICAGO_REG_H__
#define __ANX7533_CHICAGO_REG_H__

#include "anx7533_config.h"

/******************************************************************************
Description: Defined for Constant
******************************************************************************/
#define ANX7533_TURN_ON				1
#define ANX7533_TURN_OFF			0

#define ANX7533_RESET_UP_DELAY		11		// delay >= 10ms, ocm start to work
#define ANX7533_RESET_DOWN_DELAY	1
#define ANX7533_CHIPPOWER_UP_DELAY	21		// delay >= 20ms, for clock stable
#define ANX7533_CHIPPOWER_DOWN_DELAY	1

#define HDP_FORCE	1
#define	HDP_UNFORCE	0
#define HDP_DATA_HIGH	1
#define HDP_DATA_LOW	0

#define DP_CABLE_IN	1
#define DP_CABLE_OUT	0

#define MIPI_PORT_LP	0
#define MIPI_PORT_HS	1


/******************************************************************************
 Register List
******************************************************************************/
// Register Slave ID definition
#define SLAVEID_SPI			0x01
#define SLAVEID_PPS			0x02
#define SLAVEID_EDID		0x03
#define SLAVEID_MIPI_CTRL	0x08
#define SLAVEID_SERDES		0x0A
#define SLAVEID_DP_TOP		0xA0
#define SLAVEID_DPCD		0x10
#define SLAVEID_MAIN_LINK	0x20
#define SLAVEID_DP_IP		0x30
#define SLAVEID_AUDIO		0x50
#define SLAVEID_VIDEO		0x60
#define SLAVEID_PLL			0x70
#define SLAVEID_MIPI_PORT0	0xC0
#define SLAVEID_MIPI_PORT1	0xD0
#define SLAVEID_MIPI_PORT2	0xE0
#define SLAVEID_MIPI_PORT3	0xF0

/* defined in debug guide */
#define SLAVEID_DP_RX1	        0x11
#define SLAVEID_DP_RX2		0x12
#define SLAVEID_DP_DEBUG	0x22

/********************************************
 * SLAVEID_DP_RX1 0x11 register offset_addr
 ********************************************/
#define ADDR_LINK_BW_SET	0x00
#define ADDR_LANE_CNT_SET	0x01

/********************************************
 * SLAVEID_DP_RX2 0x12 register offset_addr
 ********************************************/
#define ADDR_LANE0_STATUS	0x02
#define ADDR_LANE1_STATUS	0x03
#define ADDR_LANE_ALIGN_STATUS	0x04
#define ADDR_LANE0_ERR_CNT0	0x10
#define ADDR_LANE0_ERR_CNT1	0x11
#define ADDR_LANE1_ERR_CNT0	0x12
#define ADDR_LANE1_ERR_CNT1	0x13

/********************************************
 * SLAVEID_DP_RX1 0x11 register offset_addr
 ********************************************/
#define ADDR_DEBUG_REG1		0x90
#define ADDR_DEBUG_REG2		0xA4
#define ADDR_DEBUG_REG3		0xC4


//*************************************************
//SLAVEID_SPI 0x0100 register offset_addr definition
//*************************************************
#define R_RAM_CS			0x0000
#define R_RAM_ADDR_H		0x0001
#define R_RAM_ADDR_L		0x0002
#define R_RAM_LEN_H			0x0003
#define R_RAM_LEN_L			0x0004
#define R_RAM_CTRL			0x0005
// bit definition
#define FLASH_DONE			_BIT7
#define BOOT_LOAD_DONE		_BIT6
#define CRC_OK				_BIT5
#define LOAD_DONE			_BIT4
#define DECRYPT_EN			_BIT1
#define LOAD_START			_BIT0

#define R_FLASH_ADDR_H		0x000F
#define R_FLASH_ADDR_L		0x0010
#define R_FLASH_ADDR_0		0x0011
#define R_FLASH_LEN_H		0x0031
#define R_FLASH_LEN_L		0x0032
#define R_FLASH_RW_CTRL		0x0033
// bit definition
#define READ_DELAY_SELECT		_BIT7
#define GENERAL_INSTRUCTION_EN	_BIT6
#define FLASH_ERASE_EN 			_BIT5
#define RDID_READ_EN			_BIT4
#define REMS_READ_EN			_BIT3
#define WRITE_STATUS_EN			_BIT2
#define FLASH_READ 				_BIT1
#define FLASH_WRITE				_BIT0

#define R_FLASH_STATUS_0	0x0034
#define R_FLASH_STATUS_2	0x0036
// data definition
// Flash operation commands - refer to Table 2 in GD25D10B/05B datasheet
#define  WRITE_ENABLE      0x06
#define  WRITE_DISABLE     0x04
#define  DEEP_POWER_DOWN   0xB9
#define  DEEP_PD_REL       0xAB  /* Release from Deep Power-Down */
#define  CHIP_ERASE_A      0xC7
#define  CHIP_ERASE_B      0x60

#define R_FLASH_STATUS_3	0x0037
// data definition
#define  SECTOR_ERASE		0x20
#define  BLOCK_ERASE_32K	0x52
#define  BLOCK_ERASE_64K	0xD8

#define R_FLASH_STATUS_4	0x0038
// bit definition
/* Status Register Protect bit, operates in conjunction with the Write Protect (WP#) signal */
/* The SRP bit and WP signal set the device to the Hardware Protected mode. */
/* When the SRP = 1, and WP# signal is Low, the non-volatile bits of the Status Register (SRP, BP2, BP1, BP0)
   become read-only and the Write Status Register (WRSR) instruction is not executed. */
/* The default value of SRP bit is 0. */
#define  SRP0   _BIT7

/* Block Protect bits */
/* These bits are non-volatile. They define the size of the area to be software protected against Program and Erase commands. */
/* These bits are written with the Write Status Register (WRSR) command. */
/* When the (BP4, BP3, BP2, BP1, BP0) bits are set to 1, the relevant memory area becomes protected against Page Program (PP),
   Sector Erase (SE), and Block Erase (BE) commands. Refer to Table 1.0 in GD25D10B/05B datasheet for details. */
/* The (BP4, BP3, BP2, BP1, BP0) bits can be written provided that the Hardware Protected mode has not been set. */
#define  BP4   _BIT6
#define  BP3   _BIT5
#define  BP2   _BIT4
#define  BP1   _BIT3
#define  BP0   _BIT2

/* Write Enable Latch bit, indicates the status of the internal Write Enable Latch. */
/* When WEL bit is 1, the internal Write Enable Latch is set. */
/* When WEL bit is 0, the internal Write Enable Latch is reset, and Write Status Register, Program or
   Erase commands are NOT accepted. */
/* The default value of WEL bit is 0. */
#define  WEL   _BIT1

/* Write In Progress bit, indicates whether the memory is busy in program/erase/write status register progress. */
/* When WIP bit is 1, it means the device is busy in program/erase/write status register progress. */
/* When WIP bit is 0, it means the device is not in program/erase/write status register progress. */
/* The default value of WIP bit is 0. */
#define  WIP   _BIT0

#define R_FLASH_CTRL_0		0x003F
// bit definition
#define	AUX_DET				_BIT5

#define R_DSC_CTRL_0		0x0040
// bit definition
#define READ_STATUS_EN		_BIT7
#define DSC_EN				_BIT0


#define	GPIO_CTRL_1			0x0048	
#define	GPIO_CTRL_2			0x0049
// bit definition
#define HDCP14_ST			_BIT6
#define R_AUX_DET_MASK		_BIT5
#define TRAN_ORDER			_BIT4
// data definition
#define PANEL_INFO_MASK		0x1F

#define GPIO_STATUS_1		0x004B
// bit definition
#define FLASH_WP			_BIT0

#define FLASH_READ_D0		0x0060
// data definition
#define FLASH_READ_MAX_LENGTH	0x20
#define FLASH_WRITE_MAX_LENGTH	0x20

#define R_SYS_CTRL_1		0x0081
#define REG_BAUD_DIV_RATIO	0x0087
#define REG_OCM_INTR_END_COUNTER	0x0088
#define OCM_DEBUG_CTRL		0x0089
// bit definition
#define OCM_INT_GATE		_BIT7
#define OCM_RESET			_BIT6

#define INT_NOTIFY_MCU0		0x0090
// bit definition 
#define AUX_CABLE_OUT		_BIT0
#define VIDEO_STABLE		_BIT1
#define VIDEO_RE_CALCULATE	_BIT2
#define AUX_CABLE_IN		_BIT3
#define VIDEO_INPUT_EMPTY	_BIT4
#define AUDIO_MN_RST		_BIT5
#define AUDIO_PLL_RST		_BIT7

#define INT_NOTIFY_MCU1		0x0091
// bit definition 
#define CHIP_STANDBY_MODE		_BIT0
#define CHIP_NORMAL_MODE		_BIT1
#define DP_PHY_CTS_START		_BIT2
#define DP_PHY_CTS_STOP			_BIT3
#define DP_LINK_TRAINING_FAIL	_BIT4

#define MISC_NOTIFY_OCM1	0x0092
// bit definition
#define TURN_ON_OCM_MAILBOX			_BIT1

#define VIDEO_STABLE_DELAY_L	0x0093
#define VIDEO_STABLE_DELAY_H	0x0094
#define VIDEO_EMPTY_DELAY		0x0095

#define CHIP_ID_HIGH		0x0096
#define CHIP_ID_LOW			0x0097
#define OCM_VERSION_MAJOR	0x0098
#define OCM_BUILD_NUM		0x0099
#define SECURE_OCM_VERSION	0x009B
#define HDCP_LOAD_STATUS	0x009C
// bit definition
#define OCM_FW_CRC32			_BIT7
#define HDCP_22_FW_LOAD_DONE	_BIT5
#define HDCP_22_KEY_LOAD_DONE	_BIT4
#define HDCP_14_KEY_LOAD_DONE	_BIT3
#define HDCP_22_FW_CRC32 		_BIT2
#define HDCP_22_KEY_CRC32		_BIT1
#define HDCP_14_KEY_CRC32		_BIT0

#define SW_PANEL_FRAME_RATE	0x009D

#define MISC_NOTIFY_OCM0	0x009E
// bit definition
#define RESET_DP_PHY_WHEN_VIDEO_MUTE	_BIT1
#define ENABLE_EMPTY_RESET_DP_PHY		_BIT2
#define ENABLE_DP_LS_CHECK				_BIT3
#define ENABLE_STANDBY_MODE				_BIT4
#define AUD_MCLK_ALWAYS_ON				_BIT5
#define PANEL_INFO_SET_DONE				_BIT6
#define MCU_LOAD_DONE					_BIT7

#define M_VALUE_MULTIPLY	0x009F 
#define SW_H_ACTIVE_L		0x00A0
#define SW_H_ACTIVE_H		0x00A1
// data definition
#define SW_H_ACTIVE_H_BITS	0x3F

#define SW_HFP_L			0x00A2
#define SW_HFP_H			0x00A3
// data definition
#define SW_HFP_H_BITS	0x0F

#define SW_HSYNC_L			0x00A4
#define SW_HSYNC_H			0x00A5
// data definition
#define SW_HSYNC_H_BITS	0x0F

#define SW_HBP_L			0x00A6
#define SW_HBP_H			0x00A7
// data definition
#define SW_HBP_H_BITS	0x0F

#define SW_V_ACTIVE_L		0x00A8
#define SW_V_ACTIVE_H		0x00A9
// data definition
#define SW_V_ACTIVE_H_BITS	0x3F

#define SW_VFP_L			0x00AA
#define SW_VFP_H			0x00AB
// data definition
#define SW_VFP_H_BITS	0x0F

#define SW_VSYNC_L			0x00AC
#define SW_VSYNC_H			0x00AD
// data definition
#define SW_VSYNC_H_BITS	0x0F

#define SW_VBP_L			0x00AE
#define SW_VBP_H			0x00AF
// data definition
#define SW_VBP_H_BITS	0x0F

#define SW_PANEL_INFO_0		0x00B0
// data definition
#define REG_PANEL_COUNT_SHIFT		6
#define REG_MIPI_TOTAL_PORT_SHIFT	4
#define REG_MIPI_LANE_COUNT_SHIFT	2
#define REG_PANEL_VIDEO_MODE_SHIFT	0
// data definition
#define REG_PANEL_COUNT			0xC0
#define REG_MIPI_TOTAL_PORT		0x30
#define REG_MIPI_LANE_COUNT		0x0C
#define REG_PANEL_VIDEO_MODE	0x03

#define SW_PANEL_INFO_1		0x00B1
// data definition
#define REG_PANEL_TRANS_MODE_SHIFT	2
// data definition
#define REG_PANEL_TRANS_MODE		0x0C
// bit definition 
#define REG_PANEL_ORDER			_BIT0
#define REG_PANEL_DSC_MODE		_BIT1
#define VIDEO_BIST_MODE			_BIT5
#define SET_DPHY_TIMING			_BIT6

#define SW_AUD_MAUD_SVAL_7_0	0xB2
#define SW_AUD_MAUD_SVAL_15_8	0xB3
#define SW_AUD_MAUD_SVAL_23_16	0xB4
#define SW_AUD_NAUD_SVAL_7_0	0xB5
#define SW_AUD_NAUD_SVAL_15_8	0xB6
#define SW_AUD_NAUD_SVAL_23_16	0xB7

#define TEST_PATTERN_CTRL	0x00ED
// data definition
	#define TEST_PATTERN_EN		0x80
	#define DISPLAY_BIST_EN		0x08
	#define TEST_PATTERN_MODE1	0x10
	#define TEST_PATTERN_MODE2	0x20
	#define TEST_PATTERN_MODE3	0x30

#define H_BLANK_L			0x00EF
#define H_BLANK_H			0x00F0

#define REG_BYTE_CLK_EN		0x00F1
#define MIPI_CLK_EN			0x00F2
#define CONFIG_X_ACCESS_FIFO	0x00F6 // config_x(i2c_master) read data out
#define CONFIG_X_CTRL_0			0x00F7 // config_x(i2c_master) device address
#define CONFIG_X_CTRL_1			0x00F8 // config_x(i2c_master) offset
#define CONFIG_X_CTRL_2			0x00F9
// bit definition
#define CONFIG_X_GLH_SEL		_BIT7
#define CONFIG_X_CMD			_BIT6|_BIT5|_BIT4
#define I2C_BYTE_READ			0x00
#define I2C_BYTE_WRITE			0x10
#define I2C_RESET				0x40
#define CONFIG_X_SPEED			_BIT3|_BIT2
#define CONFIG_X_NO_STOP		_BIT1
#define CONFIG_X_NO_ACK			_BIT0

#define CONFIG_X_CTRL_3			0x00FA // config_x(i2c_master) access number low byte
#define CONFIG_X_CTRL_4			0x00FB
// bit definition
#define CONFIG_X_DDC_STATE			_BIT7|_BIT6|_BIT5|_BIT4|_BIT3
#define CONFIG_X_MODE				_BIT2
#define CONFIG_X_ACCESS_NUM_HIGH	_BIT1|_BIT0

#define CONFIG_X_CTRL_5			0x00FC
#define CONFIG_X_CTRL_6			0x00FD
#define OCM_DEBUG				0x00FE
#define R_VERSION			0x00FF

//*************************************************
//SLAVEID_PPS 0x0200 register offset_addr definition
//*************************************************
#define	REG_ADDR_ACTIVE_LINE_CFG_L	0x0014
#define	REG_ADDR_ACTIVE_LINE_CFG_H	0x0015
// data definition
#define REG_V_ACTIVE_H_BITS	0x3F

#define	REG_ADDR_V_SYNC_CFG			0x0017
#define REG_ADDR_TOTAL_PIXEL_CFG_L	0x0019
#define REG_ADDR_TOTAL_PIXEL_CFG_H	0x001A
// data definition
#define REG_H_TOTAL_H_BITS	0x3F

#define	REG_ADDR_ACTIVE_PIXEL_CFG_L	0x001B
#define REG_ADDR_ACTIVE_PIXEL_CFG_H	0x001C
// data definition
#define REG_H_ACTIVE_H_BITS	0x3F

#define REG_ADDR_H_F_PORCH_CFG_L	0x001D
#define REG_ADDR_H_F_PORCH_CFG_H	0x001E
// data definition
#define REG_HFP_H_BITS	0x0F

#define REG_ADDR_H_SYNC_CFG_L		0x001F
#define REG_ADDR_H_SYNC_CFG_H		0x0020
// data definition
#define REG_HSYNC_H_BITS	0x0F

#define REG_ADDR_H_B_PORCH_CFG_L	0x0021
#define	REG_ADDR_H_B_PORCH_CFG_H	0x0022
// data definition
#define REG_HBP_H_BITS	0x0F

#define REG_ADDR_V_F_PORCH_CFG_L	0x0023
#define	REG_ADDR_V_F_PORCH_CFG_H	0x0024
// data definition
#define REG_VFP_H_BITS	0x0F

#define REG_ADDR_V_B_PORCH_CFG_L	0x0025
#define	REG_ADDR_V_B_PORCH_CFG_H	0x0026
// data definition
#define REG_VBP_H_BITS	0x0F

#define DSC_DEBUG_2			0x007C
// bit definitions
#define FIFO_OF_ERR_BUF00	_BIT7
#define FIFO_OF_ERR_BUF01	_BIT6
#define FIFO_OF_ERR_BUF10	_BIT5
#define FIFO_OF_ERR_BUF11	_BIT4
#define FIFO_UF_ERR_BUF00	_BIT3
#define FIFO_UF_ERR_BUF01	_BIT2
#define FIFO_UF_ERR_BUF10	_BIT1
#define FIFO_UF_ERR_BUF11	_BIT0

#define PPS_REG_0			0x0080
#define PPS_REG_96			0x00e0
#define PPS_REG_97			0x00e1
#define PPS_REG_98			0x00e2
#define PPS_REG_99			0x00e3
#define PPS_REG_100			0x00e4
#define PPS_REG_101			0x00e5
#define PPS_REG_102			0x00e6
#define PPS_REG_103			0x00e7
#define PPS_REG_104			0x00e8
#define PPS_REG_105			0x00e9
#define PPS_REG_106			0x00ea
#define PPS_REG_107			0x00eb
#define PPS_REG_108			0x00ec
#define PPS_REG_109			0x00ed
#define PPS_REG_110			0x00ee
#define PPS_REG_111			0x00ef

#define R_MISC_DEBUG				0x00FB
#define PRE_FILLER_BUF0_15_8		0x00FC
#define PRE_FILLER_BUF0_7_0			0x00FD
#define PRE_FILLER_BUF1_15_8		0x00FE
#define PRE_FILLER_BUF1_7_0			0x00FF

//*************************************************
//EDID 0x0300 buffer register offset_addr definition
//*************************************************
#define EDID_EXTENSION_BUF		0x0080

// Other EDID information
#define EDID_LENGTH	128
#define EDID_EXTENSION_LENGTH	128
// EDID position
#define EDID_MANUFACTURER_ID_H	0x08
#define EDID_MANUFACTURER_ID_L	0x09
#define EDID_PRODUCT_ID_L	0x0A
#define EDID_PRODUCT_ID_H	0x0B
#define EDID_SERIAL_NUM_L	0x0C
#define EDID_SERIAL_NUM_H	0x0E
#define EDID_WEEK			0x10
#define EDID_YEAR			0x11

#define EDID_DB1_PIXEL_CLOCK_L	0x36
#define EDID_DB1_PIXEL_CLOCK_H	0x37

#define EDID_DB1_BASE		0x36
#define EDID_DB2_BASE		0x48
#define EDID_DB3_BASE		0x5A
#define EDID_DB4_BASE		0x6C

#define EDID_DB_MAX			4
#define EDID_DB_SIZE		18
#define EDID_DB_DUMMY		0x10

#define EDID_PIXEL_CLK_L	0
#define EDID_PIXEL_CLK_H	1
#define EDID_HACTIVE_L		2
#define EDID_HBP_L			3
#define EDID_HACT_HBP_H		4
#define EDID_VACTIVE_L		5
#define EDID_VBP_L			6
#define EDID_VACT_VBP_H		7
#define	EDID_HFP_L			8
#define	EDID_HSYNC_L		9
#define	EDID_VFP_VSYNC_L	10
#define EDID_HFP_HSYNC_VFP_VSYNC_H	11
#define EDID_H_DISPLAY_SIZE		12
#define EDID_FEATURES_BITMAP	17

#define EDID_EXTERN_PANEL_DATA	0x49
#define EDID_VFP_MAX_VALUE		63

//*************************************************
//SLAVEID_MIPI_CTRL 0x0800 register offset_addr definition
//*************************************************
#define R_MIPI_TX_PORT_PD		0x0000
// bit definition
#define MIPI_PORT_3_PD		_BIT7
#define MIPI_PORT_2_PD		_BIT6
#define MIPI_PORT_1_PD		_BIT5
#define MIPI_PORT_0_PD		_BIT4

#define R_MIP_TX_PHY_TIMER_0	0x0001
#define R_MIP_TX_PHY_TIMER_1	0x0002
#define R_MIP_TX_PHY_TIMER_2	0x0003
#define R_MIP_TX_PHY_TIMER_3	0x0004
#define R_MIP_TX_PHY_TIMER_4	0x0005
#define R_MIP_TX_PHY_TIMER_5	0x0006
#define R_MIP_TX_PHY_TIMER_6	0x0007
#define R_MIP_TX_PHY_TIMER_7	0x0008
#define R_MIP_TX_PHY_TIMER_8	0x0009
#define R_MIP_TX_PHY_TIMER_9	0x000A
#define R_MIP_TX_PHY_TIMER_10	0x000B
#define R_MIP_TX_PHY_TIMER_11	0x000C
#define R_MIP_TX_PHY_TIMER_12	0x000D
#define R_MIP_TX_PHY_TIMER_13	0x000E
#define R_MIP_TX_TLPX_COUNTER	0x0018
#define R_MIP_TX_SELECT			0x0019
#define R_MIP_TX_STATE			0x0022
// bit definition
#define MIPI_TX_STABLE_STATE	_BIT0

#define R_MIP_TX_INT			0x0023
// bit definition
#define MIPI_PORT_3_INT			_BIT5
#define MIPI_PORT_2_INT			_BIT4
#define MIPI_PORT_1_INT			_BIT3
#define MIPI_PORT_0_INT			_BIT2
#define MIPI_STABLE_INT			_BIT1
#define MIPI_JITTER_INT			_BIT0

#define R_MIP_TX_INT_MASK		0x0024
// bit definition
#define	MIPI_STABLE_INT_MASK	_BIT1
#define MIPI_JITTER_INT_MASK	_BIT0

#define R_MIP_TX_INT_CLR		0x0025
// bit definition
#define MIPI_STABLE_INT_CLR		_BIT1
#define MIPI_JITTER_INT_CLR		_BIT0

#define R_MIP_TX_PHY_TIMER_14	0x002A
#define R_MIP_TX_PHY_TIMER_15	0x002B
#define R_MIP_TX_PHY_TIMER_16	0x002C
#define R_MIP_TX_PHY_TIMER_17	0x002D
#define R_MIP_TX_PHY_TIMER_18	0x002E
#define R_MIP_TX_PHY_TIMER_19	0x002F
#define R_MIP_TX_PHY_TIMER_20	0x0030
#define R_MIP_TX_PHY_TIMER_21	0x0031
#define R_MIPI_PHY				0x003A

//*************************************************
//SLAVEID_SERDES 0x0A00 register offset_addr definition
//*************************************************
#define SERDES_REG_2				0x0002
#define SERDES_REG_3				0x0003
// bit definition
#define REG_BYPASS_SIGNAL_DET		_BIT5
#define REG_BYPASS_STATE			_BIT4

#define SERDES_REG_7				0x0007
#define SERDES_REG_9				0x0009
// bit definition
#define REG_LANE_OK_SW				_BIT4
#define SWAP_AUX_R					_BIT5
#define SWAP_AUX_T					_BIT6

#define DPPLL_REG2					0x0022
// bit definition
#define EXT_INTR_OUTPUT				_BIT2

#define SERDES_REG_38				0x0026
// data definition
#define ALL_SET_LANE0				0x00
#define ALL_SET_LANE1				0x55
#define ALL_SET_LANE2				0xAA
#define ALL_SET_LANE3				0xFF
// data definition
#define PHY_SINK_TEST_LANE_SEL_0	0x00
#define PHY_SINK_TEST_LANE_SEL_1	0x10
#define PHY_SINK_TEST_LANE_SEL_2	0x20
#define PHY_SINK_TEST_LANE_SEL_3	0x30

#define SERDES_SET_1_RX_REG1		0x002B
#define SERDES_SET_2_RX_REG2		0x002C
#define SERDES_SET_5_RX_REG5		0x002F

#define SERDES_SET_7				0x0031
#define SERDES_SET_8_RX_REG8		0x0032
#define SERDES_SET_9_RX_REG9		0x0033
#define SERDES_SET_13_RX_REG13		0x0037
#define SERDES_SET_14_RX_REG14		0x0038
#define SERDES_SET_15_RX_REG15		0x0039
#define SERDES_POWER_CONTROL		0x003C
// bit definition
#define OCM_LOAD_DONE				_BIT4
#define PD_V10_AUX_PHY				_BIT3
#define PD_V10_DPRX					_BIT2
#define PD_V10_APLL					_BIT1
#define PD_V10_MIPI					_BIT0


#define REG0_0_RX_REG0				0x003E
#define REG0_1_RX_REG0				0x003F
#define REG0_2_RX_REG0				0x0040
#define REG0_3_RX_REG0				0x0041
#define REG7_0_RX_REG7				0x0042
#define REG7_1_RX_REG7				0x0043
#define REG7_2_RX_REG7				0x0044
#define REG7_3_RX_REG7				0x0045
#define REG16_0_RX_REG16			0x0046
#define REG16_1_RX_REG16			0x0047
#define REG16_2_RX_REG16			0x0048
#define REG16_3_RX_REG16			0x0049
#define REG_CR						0x004A

//*************************************************
//SLAVEID_DP_TOP 0xA000 register offset_addr definition
//*************************************************
#define ADDR_PWD_SEL			0x0000
#define HDCP1_PWD_SW_EN			_BIT4

#define ADDR_PWD_CTRL_0			0x0004
// bit definition
#define PWD_AUX_CH				_BIT4

#define ADDR_PWD_CTRL_1			0x0008
// bit definition
#define PWD_HDCP2				_BIT5
#define PWD_HDCP1				_BIT4
#define	PWD_AUDPLL				_BIT3
#define PWD_AUD					_BIT2

#define ADDR_RST_SEL_0			0x0010
// bit definition
#define HDCP1DEC_AUTO_RST		_BIT5
#define HDCP1AUTH_AUTO_RST		_BIT4
#define AUD_AUTO_RST			_BIT3

#define ADDR_RST_SEL_1			0x0014
// bit definition
#define MAIN_VID_CHANGE_RST_EN	_BIT4
#define ALIGN_STATUS_RST_EN		_BIT3
#define LANE_DEC_RST_EN			_BIT2
#define AUDIOPLL_AUTO_RST		_BIT1
#define VIDEOPLL_AUTO_RST		_BIT0


#define ADDR_RST_CTRL_0			0x0018
// bit definition
#define RST_SW_RESET			_BIT1
#define RST_LOGIC_RESET			_BIT0

#define ADDR_RST_CTRL_1			0x001C
// bit definition
#define RST_SERDESCTRL_RESET	_BIT7
#define RST_HDCP2_RESET			_BIT6
#define RST_HDCP1DEC_RESET		_BIT5
#define RST_HDCP1AUTH_RESET		_BIT4
#define RST_VID_RESET			_BIT3
#define RST_MAINLINK_RESET		_BIT2
#define RST_RXTOP_RESET			_BIT1
#define RST_AUX_CH_RESET		_BIT0

#define ADDR_RST_CTRL_2			0x0020
// bit definition
#define RST_VIDEOPLLPRE_RESET	_BIT4
#define RST_VIDEOPLL_RESET		_BIT3
#define RST_AUDIOPLL_RESET		_BIT2
#define RST_AUDIOMNADJ_RESET	_BIT1
#define RST_AUDIO_RESET			_BIT0

#define ADDR_MISC_CTRL			0x0030
#define ADDR_POWER_STATUS		0x0040
// bit definition
#define AUX_STATUS				_BIT2
#define MAIN_LINK_STATUS		_BIT1
#define VIDEO_STATUS			_BIT0

#define ADDR_SW_INTR_CTRL		0x0044
// bit definition
#define PLL_INTR				_BIT7
#define PLL_INTR_MASK			_BIT6
#define SOFT_INTR				_BIT0

#define ADDR_INTR				0x0048
#define ADDR_INTR_MASK			0x004c
#define MAIN_LINK_INTR		_BIT0
#define DP_IP_INTR			_BIT1
#define DPCD_INTR			_BIT2
#define AUDIO_INTR			_BIT3
#define VIDEO_INTR			_BIT4
#define SERDES_INTR			_BIT5		
#define HDCP1_INTR			_BIT6
#define VIDEO_ON_INT		_BIT7

//*************************************************
//SLAVEID_DPCD 0x1000 register offset_addr definition
//*************************************************
#define DPCD_REV				0x0000
#define MAX_LINK_RATE			0x0001
#define MAX_LANE_COUNT			0x0002
#define MAX_DOWNSPREAD			0x0003
#define DP_PWR_VOLTAGE_CAP		0x0004
#define DOWN_STREAM_PORT_PRESENT	0x0005
#define MAIN_LINK_CHANNEL_CODING	0x0006
#define DOWN_STREAM_PORT_COUNT		0x0007
#define RECEIVE_PORT0_CAP_0			0x0008
#define RECEIVE_PORT0_CAP_1			0x0009
#define RECEIVE_PORT1_CAP_0			0x000A
#define eDP_CONFIGURATION_CAP		0x000D
#define TRAINING_AUX_RD_INTERVAL	0x000E

#define LINK_BW_SET				0x0100
// data definition
#define DPCD_BW_1P62G	0x06
#define DPCD_BW_2P7G	0x0A
#define DPCD_BW_5P4G	0x14
#define DPCD_BW_6P75G	0x1E

#define LANE_COUNT_SET			0x0101
#define TRAINING_PATTERN_SET	0x0102
// data definition
#define TRAINING_PATTERN_SELECT	0x03
#define LT_NOT_IN_PROGRESS		0x00
#define LT_PATTERN_SEQUENCE1	0x01
#define LT_PATTERN_SEQUENCE2	0x02

#define SINK_COUNT					0x0200
#define LANE_ALIGN_STATUS_UPDATED	0x0204
// bit definition
#define INTERLANE_ALIGN_DONE		_BIT0

#define TEST_SINK_MISC				0x0246
// data definition
#define TEST_CRC_COUNT				0x0F
// bit definition
#define TEST_CRC_SUPPORTED			_BIT5

#define TEST_SINK					0x0270
// bit definition
#define TEST_SINK_START			_BIT0
#define PHY_SINK_TEST_LANE_SEL	0x30
#define	PHY_SINK_TEST_LANE_EN	_BIT7
#define PHY_SINK_TEST_LANE_SEL_POS	4

#define DPCD_SINK_IEEE_OUI_FIRST		0x0400
#define DPCD_SINK_IEEE_OUI_SECOND		0x0401
#define DPCD_SINK_IEEE_OUI_THIRD		0x0402

#define ADDR_BW_CHANGE			0x04F1
#define DPCD_BW_CHANDE  	_BIT6

#define AUX_FLOATING_ZONE		0x04F2
#define FLOATING_DPCD_0_INTR_0	_BIT0

#define DPCD_INTR0				0x04F4
#define DPCD_INTR1				0x04F5
#define DPCD_INTR2				0x04F6
#define DPCD_INTR3				0x04F7

#define DPCD_BRANCH_IEEE_OUI_FIRST		0x0500
#define DPCD_BRANCH_IEEE_OUI_SECOND		0x0501
#define DPCD_BRANCH_IEEE_OUI_THIRD		0x0502

#define DPCD_POWEWR_STATE		0x0600
// data difinition
#define POWER_STATE_MASK		0x07
#define POWER_STATE_NORMAL		0x01
#define POWER_STATE_SLEEP		0x02
#define POWER_STATE_STANDBY		0x05

//*************************************************
//SLAVEID_MAIN_LINK 0x2000 register offset_addr definition
//*************************************************
#define ADDR_HSTART_DBG					0x0048
#define ADDR_HSW_DBG					0x0050
#define ADDR_HTOTAL_DBG					0x0058
#define ADDR_HWIDTH15_8_DBG				0x0060
#define ADDR_HWIDTH7_0_DBG				0x0064
#define ADDR_VHEIGHT15_8_DBG			0x0068
#define ADDR_VHEIGHT7_0_DBG				0x006C
#define ADDR_VSW_DBG					0x0078
#define ADDR_VTOTAL_DBG					0x0080
#define ADDR_MAIN_LINK_CTRL				0x009C
// bit definition
#define FORCE_MN_READY				_BIT7
#define FORCE_M_N					_BIT6
#define FIELD_INV_EN				_BIT5
#define MAIN_WRITE_EN				_BIT4
#define REFILL_ON_FIFO_ERROR		_BIT3
#define RESET_ON_FIFO_ERROR			_BIT2
#define OVERFLOW_COMPENSATE_EN		_BIT1
#define UNDERFLOW_COMPENSATE_EN		_BIT0

#define ADDR_M_FORCE_VALUE_3			0x00A0
#define ADDR_M_FORCE_VALUE_2			0x00A4
#define ADDR_M_FORCE_VALUE_1			0x00A8
#define ADDR_N_FORCE_VALUE_3			0x00AC
#define ADDR_N_FORCE_VALUE_2			0x00B0
#define ADDR_N_FORCE_VALUE_1			0x00B4
#define ADDR_VIDEO_ACTIVE_MASK			0x00D4
// bit defintion
#define VIDEO_UNMUTE_JUDGE_MASK			_BIT2
#define MSA_JUDGE_MASK					_BIT1
#define VIDEO_M_READY_MASK				_BIT0

#define ADDR_VIDEO_FIFO_PRE_FILLER_L	0x00E8
#define ADDR_VIDEO_FIFO_PRE_FILLER_H	0x00EC
#define ADDR_AVI_INFOR					0x0100
#define ADDR_LINK_LAYER_STATE_2			0x0284
// bit definition
#define MAIN_DATA_OUT_EN				_BIT4

#define ADDR_VID_M_RPT_23_16			0x0290
#define ADDR_VID_M_RPT_15_8				0x0294
#define ADDR_VID_M_RPT_7_0				0x0298
#define ADDR_NVID_23_16					0x029C
#define ADDR_NVID_15_8					0x02A0
#define ADDR_NVID_7_0					0x02A4
#define ADDR_AUD_PACK_STATUS			0x02B4
// data definition
#define AUDIO_CHANNEL_COUNT				0x07

#define ADDR_MAIN_LINK_INTR0			0x02C0
#define ADDR_MAIN_LINK_INTR1			0x02C4
#define ADDR_MAIN_LINK_INTR2			0x02C8
// bit definition
#define BE_CNT_ERR_IN_FRAME_INTR		_BIT1
#define BS_CNT_ERR_IN_FRAME_INTR		_BIT0

#define ADDR_MAIN_LINK_INTR0_MASK		0x02D0
// bit definition
#define M_N_AUD_IND						_BIT6
#define VSC_PACKAGE_CHANGE				_BIT5
#define SPD_INFO_CHANGE					_BIT4
#define AVI_INFO_CHANGE					_BIT3
#define MPEG_INFO_CHANGE				_BIT2
#define OTHER_INFO_RECEIVED_INT			_BIT1
#define AUDIO_INFO_CHANGE				_BIT0

#define ADDR_MAIN_LINK_INTR1_MASK		0x02D4
// bit definition
#define MSA_UPDATE_INTR					_BIT6
#define AUDIO_CH_COUNT_CHANGE_INT		_BIT5
#define AUDIO_M_N_CHANGE_INT			_BIT4
#define MAIN_LOST_FLAG					_BIT3
#define VOTING_ERROR					_BIT2
#define MAIN_FIFO_OVFL					_BIT1
#define MAIN_FIFO_UNFL					_BIT0

#define ADDR_MAIN_LINK_INTR2_MASK		0x02D8
// bit defintion
#define BE_CNT_ERR_IN_LINE_INTR			_BIT3
#define BS_CNT_ERR_IN_LINE_INTR			_BIT2
#define BE_CNT_ERR_IN_FRAME_INTR		_BIT1
#define BS_CNT_ERR_IN_FRAME_INTR		_BIT0

#define ADDR_MAIN_LINK_INTR0_AEC		0x02E0
#define ADDR_MAIN_LINK_INTR1_AEC		0x02E4
#define ADDR_MAIN_LINK_INTR2_AEC		0x02E8
#define ADDR_MAIN_LINK_STATUS_0			0x02F0
// bit definition
#define VIDEO_MN_READY_INT				_BIT1
#define VIDEO_UNMUTE_INT				_BIT0


//*************************************************
//SLAVEID_DP_IP 0x3000 register offset_addr definition
//*************************************************
#define ADDR_SYSTEM_CTRL_0			0x0028
// bit definition
#define SYNC_STATUS_SEL			_BIT5
#define ALIGN_LATCH_LOW			_BIT4
#define FORCE_HPD_VALUE			_BIT3
#define FORCE_HPD_EN			_BIT2
#define POL_MAKE_SEL			_BIT1
#define CHK_DU_EN				_BIT0

#define ADDR_SYSTEM_CTRL_1			0x002C
#define ADDR_SYSTEM_STATUS_0		0x0034
#define ADDR_SYSTEM_STATUS_1		0x0038

#define ADDR_PRBS_CTRL			0x005C
#define ADDR_RC_TRAINING_RESULT		0x0058
#define ADDR_PRBS31_ERR_IND		0x000C

// bit definition
#define AUX_CMD_RCV			_BIT1
#define AUX_CMD_REPLY		_BIT0

#define ADDR_RCD_PN_CONVERTE			0x0064
// bit definition
#define BYPASS_RC_PAT_CHK			_BIT5

#define ADDR_HDCP2_CTRL				0x0140
// bit definition
#define HDCP2_FW_EN		_BIT5
#define HDCP_VERSION	_BIT6

#define ADDR_HDCP2_DEBUG_2				0x014C
#define ADDR_HDCP2_STATUS				0x0154
// bit definition
#define ENCRYPT_STATUS					_BIT1

#define ADDR_FLOATING_DPCD_ZONE_0_L		0x0180
#define ADDR_FLOATING_DPCD_ZONE_0_H		0x0184
#define ADDR_FLOATING_DPCD_ZONE_1_L		0x0188
#define ADDR_FLOATING_DPCD_ZONE_1_H		0x018C
#define ADDR_FLOATING_DPCD_ZONE_2_L		0x0190
#define ADDR_FLOATING_DPCD_ZONE_2_H		0x0194
#define ADDR_FLOATING_DPCD_ZONE_3_L		0x0198
#define ADDR_FLOATING_DPCD_ZONE_3_H		0x019C

#define ADDR_HPD_ACTIVE_WAIT_TIMER	0x0270

#define ADDR_AUX_CH_STATUS			0x030C
// bit definition
#define AUX_ADO_INTR_LAT		_BIT3
#define AUX_TIMEOUT_LAT			_BIT2
#define AUX_MII_ERR_LAT			_BIT1
#define AUX_LEN_ERR_LAT			_BIT0

#define ADDR_DPIP_INTR				0x0320
#define ADDR_DPIP_INTR_MASK			0x0330
// bit definition
#define ALIGN_STATUS_UNLOCK_INTR	_BIT3
#define LINK_TRAINING_FAIL_INTR		_BIT2
#define AUX_CH_ERR_INTR				_BIT1
#define VID_FORMAT_CHANGE			_BIT0

#define ADDR_DPIP_INTR_AEC			0x0340
#define ADDR_AUX_ADDR_L				0x0368
// bit definition
#define AUX_ADDR_ERROR				0x40

//*************************************************
//SLAVEID_AUDIO 0x5000 register offset_addr definition
//*************************************************
#define ADDR_AUD_MCTRL		0x000C
// bit definition
#define AUD_SOFTMUTE_EN				_BIT7
#define HARD_MUTE_EN				_BIT3
#define AUD_MUTEDONE_INT_ON_UNMUTE	_BIT2
#define AUD_UNMUTE_ON_DE			_BIT1
#define AUD_MUTE					_BIT0

#define ADDR_AUD_INTR		0x0010
#define ADDR_AUD_INTR_MASK	0x0014
// bit definition
#define AUD_CH_STATUS_CHANGE_INT	_BIT7
#define VBID_AUD_MUTE_INTR			_BIT6
#define AUD_FIFO_UND_INT			_BIT5
#define AUD_FIFO_OVER_INT			_BIT4
#define AUD_LINKERR_INT				_BIT3
#define AUD_DECERR_INT				_BIT2
#define AUD_SPDIFERR_INT			_BIT1
#define AUD_MUTEDONE_INT			_BIT0

#define ADDR_AUD_AAC		0x0044
// bit definition
#define AAC_EN				_BIT0

#define ADDR_AUD_INTR_AEC	0x0048
#define ADDR_AUD_ACR_CTRL_1	0x0050
// bit definition
#define MAUD_SEL			_BIT1
#define NAUD_SEL			_BIT0

#define ADDR_NAUD_SVAL_7_0		0x0054
#define ADDR_NAUD_SVAL_15_8		0x0058
#define ADDR_NAUD_SVAL_23_16	0x005C
#define ADDR_MAUD_SVAL_7_0		0x0084
#define ADDR_MAUD_SVAL_15_8		0x0088
#define ADDR_MAUD_SVAL_23_16	0x008C

#define ADDR_DBG_AUD_SAMPLE_CNT_7_0		0x00C8
#define ADDR_DBG_AUD_SAMPLE_CNT_15_8	0x00CC
#define	ADDR_AUD_CTRL1		0x00DC
#define	ADDR_AUD_CTRL2		0x00E0
#define ADDR_AUD_AFC_CTRL	0x0100
// bit definition
#define RE_COUNT_EN			_BIT2
#define STEP_SCALE			_BIT1
#define AFC_FUNC_EN			_BIT0

#define	ADDR_AUD_CTRL4		0x0248	
#define	ADDR_DBG_AUD_CS_3	0x0270
// bit definition
#define AUD_SAMPLE_RATE		0x0F

//*************************************************
//SLAVEID_VIDEO 0x6000 register offset_addr definition
//*************************************************
#define ADDR_ACT_PIX_LOW	0x0010
#define ADDR_ACT_PIX_HIGH	0x0014
#define ADDR_ACT_LINE_LOW	0x0018
#define ADDR_ACT_LINE_HIGH	0x001C
#define ADDR_VID_STABLE_DET	0x0040
// bit definition
#define VID_STRM_STABLE_STATUS	_BIT2

#define ADDR_VID_INT		0x00C0
#define ADDR_VID_INT_MASK	0x00C4
// bit definition
#define VSYNC_DET_INT			_BIT5
#define V_RES_CHANGED			_BIT4
#define H_RES_CHANGED			_BIT3
#define SYNC_POL_CHANGED		_BIT2
#define VID_TYPE_CHANGED		_BIT1
#define VID_STRM_STABLE_INT		_BIT0

//*************************************************
//SLAVEID_PLL 0x7000 register offset_addr definition
//*************************************************
#define ADDR_VPLL_ANALOG_4	0x0010
#define ADDR_VPLL_CTRL_0	0x0014
#define ADDR_VPLL_CTRL_2	0x001C
#define ADDR_VPLL_CTRL_3	0x0020
#define ADDR_PLL_INTR		0x0090
#define ADDR_PLL_INTR_MASK	0x0094
// bit definition
#define VPLL_UNLOCK_INT		_BIT3
#define APLL_UNLOCK_INT		_BIT2
#define APLL_INTP_N_CHG		_BIT1
#define APLL_INTP_FS_CHG	_BIT0

#define ADDR_PLL_INTR_AEC	0x0098

//*************************************************
//MIPI TX 0xC000 register offset_addr definition
//*************************************************
#define PWR_UP				0x0004
#define CLKMGR_CFG			0x0008
#define DPI_COLOR_CODING	0x0010
#define DPI_CFG_POL			0x0014
#define PCKHDL_CFG			0x002C
#define GEN_VCID			0x0030
#define MODE_CFG			0x0034
#define VID_MODE_CFG		0x0038
#define VID_PKT_SIZE		0x003C
#define VID_HSA_TIME		0x0048
#define VID_HBP_TIME		0x004C
#define VID_HLINE_TIME		0x0050
#define VID_VSA_LINES		0x0054
#define VID_VBP_LINES		0x0058
#define VID_VFP_LINES		0x005C
#define VID_VACTIVE_LINES	0x0060
#define CMD_MODE_CFG		0x0068
#define LPCLK_CTRL			0x0094
#define PHY_TMR_LPCLK_CFG	0x0098
#define PHY_TMR_CFG			0x009C
#define PHY_RSTZ			0x00A0
#define PHY_IF_CFG			0x00A4
#define INT_MSK0			0x00C4
#define INT_MSK1			0x00C8
#define GEN_HDR				0x006C
#define GEN_PLD_DATA		0x0070
#define CMD_PKT_STATUS		0x0074
// bit definition
#define gen_pld_w_empty		_BIT2

#define DSC_PARAMETER		0x00F0

#endif  /* _ANX_CHICAGO_REG_H_ */


/*
 * Copyright (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RFID_CR95HF_H
#define RFID_CR95HF_H

#define CR95HF_SND_BUF_SIZE 50
#define CR95HF_RCV_BUF_SIZE 258

#define CR95HF_WFE_TAG_DETECTOR_ARRAY { 0, CR95HF_CMD_IDLE, 0xE,                                 \
		CR95HF_WU_SOURCE_TAG_DETECTION | CR95HF_WU_SOURCE_LOW_PULSE_IRQ_IN,              \
		CR95HF_ENTER_CTRL_DETECTION_H, CR95HF_ENTER_CTRL_DETECTION_L,                    \
		CR95HF_WU_CTRL_DETECTION_H, CR95HF_WU_CTRL_DETECTION_L,                          \
		CR95HF_LEAVE_CTRL_DETECTION_H, CR95HF_LEAVE_CTRL_DETECTION_L,                    \
		CR95HF_DEFAULT_WU_PERIOD, CR95HF_DEFAULT_OSC_START, CR95HF_DEFAULT_DAC_START,    \
		CR95HF_DEFAULT_DAC_DATA_H, CR95HF_DEFAULT_DAC_DATA_L, CR95HF_DEFAULT_SWING_COUNT,\
		CR95HF_DEFAULT_MAX_SLEEP }

#define CR95HF_CMD_IDNREQUEST              0x01   /* Requests short information about the CR95HF
						   * and its revision
						   */
#define CR95HF_CMD_PROTOCOLSELECT          0x02   /* Selects the RF communication protocol and
						   * specifies certain protocol-related parameters
						   */
#define CR95HF_CMD_SENDRECV                0x04   /* Sends data using the previously selected
						   * protocol and receives the tag response
						   */
#define CR95HF_CMD_IDLE                    0x07   /* Switches the CR95HF into a low consumption
						   * Wait for Event (WFE) mode
						   */
#define CR95HF_CMD_RDREG                   0x08   /* Reads Wake-up event register or the Analog
						   * Register Configuration (ARC_B) register
						   */
#define CR95HF_CMD_WRREG                   0x09   /* Writes Analog Register Configuration (ARC_B)
						   * register or writes index of ARC_B register
						   * address
						   */
#define CR95HF_CMD_BAUDRATE                0x0A   /* Sets the UART baud rate */
#define CR95HF_CMD_ECHO                    0x55   /* CR95HF returns an ECHO response (0x55) */

#define CR95HF_WU_SOURCE_TIMEOUT           0x01
#define CR95HF_WU_SOURCE_TAG_DETECTION     0x02
#define CR95HF_WU_SOURCE_LOW_PULSE_IRQ_IN  0x08
#define CR95HF_WU_SOURCE_LOW_PULSE_SPI_SS  0x10

#define CR95HF_ENTER_CTRL_SLEEP_H          0x01
#define CR95HF_ENTER_CTRL_SLEEP_L          0x00
#define CR95HF_ENTER_CTRL_HIBERNATE_H      0x04
#define CR95HF_ENTER_CTRL_HIBERNATE_L      0x00
#define CR95HF_ENTER_CTRL_CALIBRATION_H    0xA1
#define CR95HF_ENTER_CTRL_CALIBRATION_L    0x00
#define CR95HF_ENTER_CTRL_DETECTION_H      0x21
#define CR95HF_ENTER_CTRL_DETECTION_L      0x00

#define CR95HF_WU_CTRL_HIBERNATE_H         0x04
#define CR95HF_WU_CTRL_HIBERNATE_L         0x00
#define CR95HF_WU_CTRL_SLEEP_H             0x38
#define CR95HF_WU_CTRL_SLEEP_L             0x00
#define CR95HF_WU_CTRL_CALIBRATION_H       0xF8
#define CR95HF_WU_CTRL_CALIBRATION_L       0x01
#define CR95HF_WU_CTRL_DETECTION_H         0x79
#define CR95HF_WU_CTRL_DETECTION_L         0x01

#define CR95HF_LEAVE_CTRL_HIBERNATE_H      0x18
#define CR95HF_LEAVE_CTRL_HIBERNATE_L      0x00
#define CR95HF_LEAVE_CTRL_SLEEP_H          0x18
#define CR95HF_LEAVE_CTRL_SLEEP_L          0x00
#define CR95HF_LEAVE_CTRL_CALIBRATION_H    0x18
#define CR95HF_LEAVE_CTRL_CALIBRATION_L    0x00
#define CR95HF_LEAVE_CTRL_DETECTION_H      0x18
#define CR95HF_LEAVE_CTRL_DETECTION_L      0x00

#define CR95HF_DEFAULT_WU_PERIOD           0x20
#define CR95HF_DEFAULT_OSC_START           0x60
#define CR95HF_DEFAULT_DAC_START           0x60
#define CR95HF_DEFAULT_DAC_DATA_H          0x64
#define CR95HF_DEFAULT_DAC_DATA_L          0x74
#define CR95HF_DEFAULT_SWING_COUNT         0x3F
#define CR95HF_DEFAULT_MAX_SLEEP           0x28

#define CR95HF_FIELD_OFF                   0x00   /* Field OFF */
#define CR95HF_PROTOCOL_ISO_15693          0x01   /* ISO/IEC 15693 */
#define CR95HF_PROTOCOL_ISO_14443_A        0x02   /* ISO/IEC 14443-A */
#define CR95HF_PROTOCOL_ISO_14443_B        0x03   /* ISO/IEC 14443-B */
#define CR95HF_PROTOCOL_ISO_18092_NFC      0x04   /* ISO/IEC 18092 /NFC */

#define CR95HF_ISO_14443_106_KBPS       0x00   /* 106 Kbps */
#define CR95HF_ISO_14443_212_KBPS       0x01   /* 212 Kbps (2) */
#define CR95HF_ISO_14443_424_KBPS       0x02   /* 424 Kbps */

#define CR95HF_SELECT_14443_A_ARRAY { 0, 2, 4, 2, \
	((CR95HF_ISO_14443_106_KBPS & 0x03) << 6) | ((CR95HF_ISO_14443_106_KBPS & 0x03) << 4), \
	1, 0x80 }

#define CR95HF_READY_TO_READ 0x8
#define CR95HF_READY_TO_WRITE 0x4
#endif  /* RFID_CR95HF_H */

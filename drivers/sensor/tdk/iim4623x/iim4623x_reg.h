/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_REG_H
#define ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_REG_H

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

/* Structures used for the iim4623x protocol packets */

/**
 * Every packet contains a preamble with a header, message length, and type.
 *
 * The header value is a magical number depending on the packet direction (TX being
 * host-to-iim4623x).
 */
#define IIM4623X_PCK_HEADER_TX 0x2424
#define IIM4623X_PCK_HEADER_RX 0x2323

struct iim4623x_pck_preamble {
	uint16_t header;
	uint8_t length;
	uint8_t type;
} __packed;

/**
 * Every packet contains a postamble with a checksum and a footer.
 *
 * The checksum is a simple sum-of-all-bytes comprising of packet type and payload.
 * The footer is a magical number: 0x0D0A.
 */
#define IIM4623X_PCK_FOOTER 0x0D0A

struct iim4623x_pck_postamble {
	uint16_t checksum;
	uint16_t footer;
} __packed;

/* Structure to describe the version command response */
struct iim4623x_pck_resp_version {
	uint8_t maj;
	uint8_t min;
	uint8_t _reserved[8];
} __packed;

/* Structure to describe the read-user-register command response */
struct iim4623x_pck_resp_read_user_reg {
	uint8_t _reserved;
	uint8_t addr;
	uint8_t page;
	uint8_t read_len;
	uint8_t error_code;
	uint8_t error_mask;
	uint8_t reg_val[72]; /* Varying size based off `read_len`, 72 is the max */
} __packed;

/* Structure to generically describe response packets from the iim4623x */
struct iim4623x_pck_resp {
	struct iim4623x_pck_preamble preamble;
	/**
	 * The acknowledgment packet is a special case wherein the packet number represents
	 * an error_code instead
	 */
	union {
		uint16_t pck_num;
		struct {
			uint8_t error_code;
			uint8_t _reserved;
		} ack;
	};
	/* Describe the payload which varies based off which command the response is tied to */
	union {
		/* Get Version */
		struct iim4623x_pck_resp_version version;
		/* Get Serial Number */
		uint8_t serial_number[16];
		/* Read User Registers */
		struct iim4623x_pck_resp_read_user_reg read_user_reg;
		/* IMU Self-Test */
		uint8_t self_test[6];
		/**
		 * The following commands get basic acknowledgment packets:
		 *  - Write User Register
		 *  - Select Streaming Interface
		 *  - Set UTC Time (no response in streaming mode)
		 *  - Enable SensorFT (no response in streaming mode)
		 *  - Disable SensorFT (no response in streaming mode)
		 *
		 * The following commands never get replies:
		 *  - Start Streaming
		 *  - Stop Streaming
		 */
	};
	/* The postamble is described using a VLA since the payload itself varies in length */
	uint8_t postamble_buf[];
} __packed;

/* Retrieve a pointer to the postamble of an iim4623x packet via. a pointer to the packet */
#define IIM4623X_GET_POSTAMBLE(pck_ptr)                                                            \
	(struct iim4623x_pck_postamble *)((uint8_t *)pck_ptr +                                     \
					  (((struct iim4623x_pck_preamble *)pck_ptr)->length -     \
					   sizeof(struct iim4623x_pck_postamble)))

/* Structures used for streaming mode data packets */

/* Conveniently wrap the collection of XYZ values from the iim4623x */
union iim4623x_xyz_values {
	struct {
		uint8_t x[4];
		uint8_t y[4];
		uint8_t z[4];
	} __packed buf;

	struct {
		float x;
		float y;
		float z;
	} __packed;
};

/* Describe the iim4623x wire format for the payload of streaming mode data packets */
struct iim4623x_pck_strm_payload {
	union {
		struct {
			uint8_t gyro: 5;
			uint8_t accel: 3;
		} __packed;
		uint8_t buf;
	} status;
	uint8_t sample_counter;
	uint64_t timestamp;
	union iim4623x_xyz_values accel;
	union iim4623x_xyz_values gyro;
	union {
		uint8_t buf[4];
		float val;
	} temp;
	union iim4623x_xyz_values delta_vel;   /* delta_vel output is disabled by default */
	union iim4623x_xyz_values delta_angle; /* delta_angle output is disabled by default */
} __packed;

/* Describe the complete iim4623x streaming mode data packet */
struct iim4623x_pck_strm {
	struct iim4623x_pck_preamble preamble;
	struct iim4623x_pck_strm_payload payload;
	struct iim4623x_pck_postamble postamble;
} __packed;

/* Acknowledgment packet error codes */
enum iim4623x_cmd_error_code {
	IIM4623X_EC_ACK = 0x00,
	IIM4623X_EC_NACK = 0x01,
	IIM4623X_EC_WRITE = 0x02,
	IIM4623X_EC_READ = 0x03,
	IIM4623X_EC_INVAL = 0x04,
	/* Reserved */
	IIM4623X_EC_WRITE_FLASH = 0x06,
	IIM4623X_EC_READ_FLASH = 0x07,
	/* Reserved */
	/* Reserved */
	IIM4623X_EC_WRITE_USER = 0x0a,
	IIM4623X_EC_READ_USER = 0x0b,
	IIM4623X_EC_FLASH_ENDURANCE = 0x0c,
	/* Reserved ... */
};

/**
 * The protocol requires a minimum amount of bytes to be transferred. Any smaller packets must be
 * zero padded after the postamble
 */
#define IIM4623X_MIN_TX_LEN 20

/* Calculate packet length given a payload length */
#define IIM4623X_PACKET_LEN(payload_len)                                                           \
	(sizeof(struct iim4623x_pck_preamble) + payload_len + sizeof(struct iim4623x_pck_postamble))

/* Calculate the packet length to a READ_USER_REGISTER command given the amount of bytes read */
#define IIM4623X_READ_REG_RESP_LEN(payload_len)                                                    \
	(sizeof(struct iim4623x_pck_preamble) + 2 +                                                \
	 sizeof(struct iim4623x_pck_resp_read_user_reg) - 72 + payload_len +                       \
	 sizeof(struct iim4623x_pck_postamble))

/* Obtain total amount of bytes to transfer (incl. zero padding) given a payload len */
#define IIM4623X_TX_LEN(payload_len) MAX(IIM4623X_PACKET_LEN(payload_len), IIM4623X_MIN_TX_LEN)

/* For convenience, explicitly define the length of an acknowledgment packet */
#define IIM4623X_PCK_ACK_LEN 10

/* Define all command types */
#define IIM4623X_CMD_GET_VERSION                0x20
#define IIM4623X_CMD_GET_SERIAL_NUMBER          0x26
#define IIM4623X_CMD_READ_USER_REGISTER         0x11
#define IIM4623X_CMD_WRITE_USER_REGISTER        0x12
#define IIM4623X_CMD_IMU_SELF_TEST              0x2B
#define IIM4623X_CMD_SET_UTC_TIME               0x2D
#define IIM4623X_CMD_SELECT_STREAMING_INTERFACE 0x30
#define IIM4623X_CMD_START_STREAMING            0x27
#define IIM4623X_CMD_STOP_STREAMING             0x28
#define IIM4623X_CMD_ENABLE_SENSORFT            0x2E
#define IIM4623X_CMD_DISABLE_SENSORFT           0x2F

/* The type field value of streaming mode data packets */
#define IIM4623X_STRM_PCK_TYPE 0xAB

/**
 * Describe the register map of the iim4623x.
 *
 * The register map consist of two pages, one for configuration (incl. some user data) and one for
 * sensor data output.
 */

#define IIM4623X_PAGE_CFG         0x00
#define IIM4623X_PAGE_SENSOR_DATA 0x01

/* Registers in page 0 - the configuration page */

#define IIM4623X_REG_WHO_AM_I   0x00
#define IIM4623X_WHO_AM_I_46234 0xEA
#define IIM4623X_WHO_AM_I_46230 0xE6

#define IIM4623X_REG_SN     0x01
#define IIM4623X_REG_SN_LEN 16

#define IIM4623X_REG_FW_REV     0x11
#define IIM4623X_REG_FW_REV_LEN 2

#define IIM4623X_REG_FLASH_ENDURANCE     0x15
#define IIM4623X_REG_FLASH_ENDURANCE_LEN 4

#define IIM4623X_REG_DATA_FMT     0x19
#define IIM4623X_DATA_FMT_FLOAT   0x00 /* IEEE-754 float */
#define IIM4623X_DATA_FMT_QFORMAT 0x01 /* integer in two's complement */

#define IIM4623X_REG_SAMPLE_RATE_DIV     0x1A
#define IIM4623X_REG_SAMPLE_RATE_DIV_LEN 2

#define IIM4623X_REG_SEL_OUT_DATA     0x1C
#define IIM4623X_SEL_OUT_DATA_ACCEL   BIT(0)
#define IIM4623X_SEL_OUT_DATA_GYRO    BIT(1)
#define IIM4623X_SEL_OUT_DATA_TEMP    BIT(2)
#define IIM4623X_SEL_OUT_DATA_D_ANGLE BIT(3)
#define IIM4623X_SEL_OUT_DATA_D_VEL   BIT(4)

#define IIM4623X_REG_UART_IF_CFG 0x1D

#define IIM4623X_REG_SYNC_CFG 0x1E

#define IIM4623X_REG_USR_SCRATCH1    0x1F
#define IIM4623X_REG_USR_SCRATCH2    0x27
#define IIM4623X_REG_USR_SCRATCH_LEN 8

#define IIM4623X_REG_SAVE_ALL_CFG 0x2F

#define IIM4623X_REG_BW_CFG               0x30
#define IIM4623X_BW_CFG_PACK(accel, gyro) ((accel << 4) | gyro | 0x44)

#define IIM4623X_REG_ACCEL_CFG   0x33
#define IIM4623X_ACCEL_CFG_SHIFT 5 /* lower 5 bits are reserved */
#define IIM4623X_ACCEL_CFG_FS_16 0x0
#define IIM4623X_ACCEL_CFG_FS_8  0x1
#define IIM4623X_ACCEL_CFG_FS_4  0x2
#define IIM4623X_ACCEL_CFG_FS_2  0x3

#define IIM4623X_REG_GYRO_CFG     0x34
#define IIM4623X_GYRO_CFG_SHIFT   5 /* lower 5 bits are reserved */
#define IIM4623X_GYRO_CFG_FS_2000 0x0
#define IIM4623X_GYRO_CFG_FS_1000 0x1
#define IIM4623X_GYRO_CFG_FS_480  0x2
#define IIM4623X_GYRO_CFG_FS_250  0x3

#define IIM4623X_REG_EXT_CALIB_CFG 0x3F

#define IIM4623X_REG_EXT_ACCEL_X_BIAS   0x40
#define IIM4623X_REG_EXT_ACCEL_Y_BIAS   0x44
#define IIM4623X_REG_EXT_ACCEL_Z_BIAS   0x48
#define IIM4623X_REG_EXT_ACCEL_BIAS_LEN 4

#define IIM4623X_REG_EXT_GYRO_X_BIAS   0x4C
#define IIM4623X_REG_EXT_GYRO_Y_BIAS   0x50
#define IIM4623X_REG_EXT_GYRO_Z_BIAS   0x54
#define IIM4623X_REG_EXT_GYRO_BIAS_LEN 4

#define IIM4623X_REG_EXT_ACCEL_SENS_MAT11   0x58
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT12   0x5C
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT13   0x60
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT21   0x64
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT22   0x68
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT23   0x6C
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT31   0x70
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT32   0x74
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT33   0x78
#define IIM4623X_REG_EXT_ACCEL_SENS_MAT_LEN 4

#define IIM4623X_REG_EXT_GYRO_SENS_MAT11   0x7C
#define IIM4623X_REG_EXT_GYRO_SENS_MAT12   0x80
#define IIM4623X_REG_EXT_GYRO_SENS_MAT13   0x84
#define IIM4623X_REG_EXT_GYRO_SENS_MAT21   0x88
#define IIM4623X_REG_EXT_GYRO_SENS_MAT22   0x8C
#define IIM4623X_REG_EXT_GYRO_SENS_MAT23   0x90
#define IIM4623X_REG_EXT_GYRO_SENS_MAT31   0x94
#define IIM4623X_REG_EXT_GYRO_SENS_MAT32   0x98
#define IIM4623X_REG_EXT_GYRO_SENS_MAT33   0x9C
#define IIM4623X_REG_EXT_GYRO_SENS_MAT_LEN 4

#define IIM4623X_REG_CUSTOM_GRAVITY     0xA4
#define IIM4623X_REG_CUSTOM_GRAVITY_LEN 4

#define IIM4623X_REG_RESET_ALL_CFG 0xA8

/* Registers in page 1 - the sensor output data page */

#define IIM4623X_REG_SAMPLE_STATUS 0x00

#define IIM4623X_REG_SENSOR_STATUS 0x01

#define IIM4623X_REG_SAMPLE_COUNTER 0x02

#define IIM4623X_REG_TIMESTAMP     0x03
#define IIM4623X_REG_TIMESTAMP_LEN 8

#define IIM4623X_REG_ACCEL_X   0x0B
#define IIM4623X_REG_ACCEL_Y   0x0F
#define IIM4623X_REG_ACCEL_Z   0x13
#define IIM4623X_REG_ACCEL_LEN 4

#define IIM4623X_REG_GYRO_X   0x17
#define IIM4623X_REG_GYRO_Y   0x1B
#define IIM4623X_REG_GYRO_Z   0x1F
#define IIM4623X_REG_GYRO_LEN 4

#define IIM4623X_REG_TEMP     0x23
#define IIM4623X_REG_TEMP_LEN 4

#define IIM4623X_REG_DELTA_VEL_X   0x27
#define IIM4623X_REG_DELTA_VEL_Y   0x2B
#define IIM4623X_REG_DELTA_VEL_Z   0x2F
#define IIM4623X_REG_DELTA_VEL_LEN 4

#define IIM4623X_REG_DELTA_ANGLE_X   0x33
#define IIM4623X_REG_DELTA_ANGLE_Y   0x37
#define IIM4623X_REG_DELTA_ANGLE_Z   0x3B
#define IIM4623X_REG_DELTA_ANGLE_LEN 4

#endif /* ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_REG_H */

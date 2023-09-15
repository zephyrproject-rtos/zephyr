/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_GROW_R502A_H_
#define ZEPHYR_DRIVERS_SENSOR_GROW_R502A_H_

/*
 * @brief confirmation code present in acknowledgment packet
 *
 *################################################################################
 *|Confirmation code	|		Definition |
 *################################################################################
 *|0x00			|commad execution complete |
 *--------------------------------------------------------------------------------
 *|0x01			|error when receiving data package |
 *--------------------------------------------------------------------------------
 *|0x02			|no finger on the sensor |
 *--------------------------------------------------------------------------------
 *|0x03			|fail to enroll the finger |
 *--------------------------------------------------------------------------------
 *|0x06			|fail to generate character file due to over-disorderly |
 *|			|fingerprint image |
 *--------------------------------------------------------------------------------
 *|0x07			|fail to generate character file due to lackness of |
 *|			|character point or over-smallness of fingerprint image.|
 *--------------------------------------------------------------------------------
 *|0x08			|finger doesn’t match |
 *--------------------------------------------------------------------------------
 *|0x09			|fail to find the matching finger |
 *--------------------------------------------------------------------------------
 *|0x0A			|fail to combine the character files |
 *--------------------------------------------------------------------------------
 *|0x0B			|addressing PageID is beyond the finger library |
 *--------------------------------------------------------------------------------
 *|0x0C			|error reading template from library or invalid |
 *|			|template |
 *--------------------------------------------------------------------------------
 *|0x0D			|error when uploading template |
 *--------------------------------------------------------------------------------
 *|0x0E			|Module can’t receive the following data packages |
 *--------------------------------------------------------------------------------
 *|0x0F			|error when uploading image |
 *--------------------------------------------------------------------------------
 *|0x10			|fail to delete the template |
 *--------------------------------------------------------------------------------
 *|0x11			|fail to clear finger library |
 *--------------------------------------------------------------------------------
 *|0x13			|wrong password! |
 *--------------------------------------------------------------------------------
 *|0x15			|fail to generate image for the lackness of valid |
 *|			|primary image |
 *--------------------------------------------------------------------------------
 *|0x18			|error when writing flash |
 *--------------------------------------------------------------------------------
 *|0x1A			|invalid register number |
 *--------------------------------------------------------------------------------
 *|0x1B			|incorrect configuration of register |
 *--------------------------------------------------------------------------------
 */

#define R502A_OK 0x00 /*commad execution complete*/
#define R502A_NOT_FOUND 0x09 /*fail to find the matching finger*/

/*Package Identifier's definition*/
#define R502A_COMMAND_PACKET 0x1 /*Command packet*/
#define R502A_DATA_PACKET 0x2 /*Data packet, must follow command packet or acknowledge packet*/
#define R502A_ACK_PACKET 0x7 /*Acknowledge packet*/
#define R502A_END_DATA_PACKET 0x8 /*End of data packet*/

/*Instruction code's definition*/
#define R502A_GENIMAGE 0x01 /*Collect finger image*/
#define R502A_IMAGE2TZ 0x02 /*To generate character file from image*/
#define R502A_MATCH 0x03 /*Carry out precise matching of two templates*/
#define R502A_SEARCH 0x04 /*Search the finger library*/
#define R502A_REGMODEL 0x05 /*To combine character files and generate template*/
#define R502A_STORE 0x06 /*To store template*/
#define R502A_LOAD 0x07 /*To read/load template*/
#define R502A_UPCHAR 0x08 /*To upload template*/
#define R502A_DOWNCHAR 0x09 /*To download template*/
#define R502A_IMGUPLOAD 0x0A /*To upload image*/
#define R502A_DELETE 0x0C /*To delete template*/
#define R502A_EMPTYLIBRARY 0x0D /*To empty the library*/
#define R502A_SETSYSPARAM 0x0E /*To set system parameter*/
#define R502A_READSYSPARAM 0x0F /*To read system parameter*/
#define R502A_SETPASSWORD 0x12 /*To set password*/
#define R502A_VERIFYPASSWORD 0x13 /*To verify password*/
#define R502A_GETRANDOM 0x14 /*To generate a random code*/
#define R502A_TEMPLATECOUNT 0x1D /*To read finger template numbers*/
#define R502A_READTEMPLATEINDEX 0x1F /*Read fingerprint template index table*/
#define R502A_LED_CONFIG 0x35 /*Aura LED Config*/
#define R502A_CHECKSENSOR 0x36 /*Check sensor*/
#define R502A_SOFTRESET 0x3D /*Soft reset*/
#define R502A_HANDSHAKE 0x40 /*Handshake*/
#define R502A_BADPACKET 0xFE /* Bad packet was sent*/

#define R502A_STARTCODE 0xEF01 /*Fixed value, High byte transferred first*/
#define R502A_DEFAULT_PASSWORD 0x00000000
#define R502A_DEFAULT_ADDRESS 0xFFFFFFFF
#define R502A_DEFAULT_CAPACITY 200
#define R502A_HANDSHAKE_BYTE 0x55
#define R02A_LIBRARY_START_IDX	0

#define R502A_STARTCODE_IDX 0
#define R502A_ADDRESS_IDX 2
#define R502A_PID_IDX 6 /* Package identifier index*/
#define R502A_PKG_LEN_IDX 7
#define R502A_CC_IDX 9 /* Confirmation code index*/

#define R502A_STARTCODE_LEN 2
#define R502A_ADDRESS_LEN 4
#define R502A_PKG_LEN	2
#define R502A_CHECKSUM_LEN 2 /* Checksum length in uart packages*/
#define R502A_HEADER_LEN 9

#define R502A_CHAR_BUF_1 1
#define R502A_CHAR_BUF_2 2
#define R502A_CHAR_BUF_SIZE 384 /* Maximum size of characteristic value buffer*/
#define R502A_TEMPLATE_SIZE 768 /* Maximum size of template, twice of CHAR_BUF*/
#define R502A_MAX_BUF_SIZE 779 /*sum of checksum, header and template sizes*/
#define R502A_BUF_SIZE 64
#define R502A_TEMPLATES_PER_PAGE 256
#define R502A_TEMP_TABLE_BUF_SIZE 32
#define R502A_DELETE_COUNT_OFFSET 1

#define R502A_DELAY 200
#define R502A_RETRY_DELAY 5

#define LED_CTRL_BREATHING 0x01
#define LED_CTRL_FLASHING 0x02
#define LED_CTRL_ON_ALWAYS 0x03
#define LED_CTRL_OFF_ALWAYS 0x04
#define LED_CTRL_ON_GRADUALLY 0x05
#define LED_CTRL_OFF_GRADUALLY 0x06

#define LED_SPEED_HALF 0x50
#define LED_SPEED_FULL 0xFF

#define LED_COLOR_RED 0x01
#define LED_COLOR_BLUE 0x02
#define LED_COLOR_PURPLE 0x03

struct led_params {
	uint8_t ctrl_code;
	uint8_t color_idx;
	uint8_t speed; /* Speed 0x00-0xff */
	uint8_t cycle; /* Number of cycles | 0-infinite, 1-255 */
};

union r502a_packet {
	struct {
		uint8_t	start[R502A_STARTCODE_LEN];
		uint8_t	addr[R502A_ADDRESS_LEN];
		uint8_t	pid;
		uint8_t	len[R502A_PKG_LEN];
		uint8_t data[R502A_BUF_SIZE];
	};

	uint8_t buf[R502A_BUF_SIZE];
};

struct r502a_buf {
	uint8_t *data;
	size_t len;
};

struct grow_r502a_data {
#ifdef CONFIG_GROW_R502A_TRIGGER
	const struct device *gpio_dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;
#if defined(CONFIG_GROW_R502A_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_GROW_R502A_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_GROW_R502A_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_GROW_R502A_TRIGGER */

	struct r502a_buf tx_buf;
	struct r502a_buf rx_buf;

	struct k_mutex lock;
	struct k_sem uart_rx_sem;

	uint16_t finger_id;
	uint16_t matching_score;
	uint16_t template_count;
};

struct grow_r502a_config {
	const struct device *dev;
	struct gpio_dt_spec vin_gpios;
	struct gpio_dt_spec act_gpios;
	uint32_t comm_addr;
#ifdef CONFIG_GROW_R502A_TRIGGER
	struct gpio_dt_spec int_gpios;
#endif /* CONFIG_GROW_R502A_TRIGGER */
};

#ifdef CONFIG_GROW_R502A_TRIGGER
int grow_r502a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler);

int grow_r502a_init_interrupt(const struct device *dev);
#endif /* CONFIG_GROW_R502A_TRIGGER */

#endif /*_GROW_R502A_H_*/

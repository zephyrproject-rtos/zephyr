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

#define R502A_NOT_MATCH_CC 0x08 /* templates of two buffers not matching*/
#define R502A_NOT_FOUND_CC 0x09 /*fail to find the matching finger*/
#define R502A_FINGER_MATCH_NOT_FOUND 0
#define R502A_FINGER_MATCH_FOUND 1

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

#define R502A_COMMON_ACK_LEN 12

#define R502A_STARTCODE_LEN 2
#define R502A_ADDRESS_LEN 4
#define R502A_PKG_LEN	2
#define R502A_CHECKSUM_LEN 2 /* Checksum length in uart packages*/
#define R502A_HEADER_LEN 9

#define R502A_CHAR_BUF_1 1
#define R502A_CHAR_BUF_2 2
#define R502A_CHAR_BUF_TOTAL 2

#define R502A_CHAR_BUF_SIZE 384 /* Maximum size of characteristic value buffer*/
#define R502A_TEMPLATE_SIZE 768 /* Maximum size of template, twice of CHAR_BUF*/
#define R502A_TEMPLATE_MAX_SIZE (R502A_CHAR_BUF_TOTAL * R502A_TEMPLATE_SIZE)

#define R502A_MAX_BUF_SIZE  (CONFIG_R502A_DATA_PKT_SIZE + R502A_COMMON_ACK_LEN)

#define R502A_TEMPLATES_PER_PAGE 256
#define R502A_TEMP_TABLE_BUF_SIZE 32
#define R502A_DELETE_COUNT_OFFSET 1

#define R502A_DELAY 200
#define R502A_RETRY_DELAY 5

/*LED glow control code*/
enum r502a_led_ctrl_code {
	R502A_LED_CTRL_BREATHING = 0x01,
	R502A_LED_CTRL_FLASHING,
	R502A_LED_CTRL_ON_ALWAYS,
	R502A_LED_CTRL_OFF_ALWAYS,
	R502A_LED_CTRL_ON_GRADUALLY,
	R502A_LED_CTRL_OFF_GRADUALLY,
};

/* LED glow speed code
 * if needed, use desired speed between 0-255
 */
enum r502a_led_speed {
	R502A_LED_SPEED_MAX = 0x00,
	R502A_LED_SPEED_HALF = 0x50,
	R502A_LED_SPEED_MIN = 0xFF,
};

/* LED glowing cycle
 * if needed, use desired cycle count between 1-255
 */
enum r502a_led_cycle {
	R502A_LED_CYCLE_INFINITE = 0x00,
	R502A_LED_CYCLE_1,
	R502A_LED_CYCLE_2,
	R502A_LED_CYCLE_3,
	R502A_LED_CYCLE_4,
	R502A_LED_CYCLE_5,
	R502A_LED_CYCLE_255 = 0xFF,
};


struct r502a_led_params {
	uint8_t ctrl_code;
	uint8_t color_idx;
	uint8_t speed; /* Speed 0x00-0xff */
	uint8_t cycle; /* Number of cycles | 0-infinite, 1-255 */
};

union r502a_packet {
	struct {
		uint16_t start;
		uint32_t addr;
		uint8_t	pid;
		uint16_t len;
		uint8_t data[CONFIG_R502A_DATA_PKT_SIZE];
	} __packed;

	uint8_t buf[R502A_MAX_BUF_SIZE];
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
	uint16_t pkt_len;

	struct k_mutex lock;
	struct k_sem uart_tx_sem;
	struct k_sem uart_rx_sem;

	uint16_t template_count;
	uint8_t led_color;
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

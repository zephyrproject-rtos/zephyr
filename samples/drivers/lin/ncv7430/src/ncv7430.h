/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* NCV7430 command definitions for commander command frames */
#define NCV7430_CMD_GET_FULL_STATUS    0x01
#define NCV7430_CMD_GET_ACTUAL_PARAM_1 0x02
#define NCV7430_CMD_GET_ACTUAL_PARAM_2 0x03
#define NCV7430_CMD_GET_OTP_PARAM_1    0x04
#define NCV7430_CMD_GET_OTP_PARAM_2    0x05

/* NCV7430 supported response frame identifiers */
#define NCV7430_FRAME_ID_GET_LED_CONTROL_RESPONSE 0x10
#define NCV7430_FRAME_ID_GET_STATUS_RESPONSE      0x11
#define NCV7430_FRAME_ID_GET_COLOR_RESPONSE       0x12

/* NCV7430 supported reading frame identifiers */
#define NCV7430_FRAME_ID_READING 0x20

/* NCV7430 supported writing frame identifiers */
#define NCV7430_FRAME_ID_SET_LED_CONTROL_WRITING         0x23
#define NCV7430_FRAME_ID_SET_LED_COLOR_WRITING           0x24
#define NCV7430_FRAME_ID_SET_PRIMARY_CAL_PARAM_WRITING   0x25
#define NCV7430_FRAME_ID_SET_OTP_PARAM_WRITING           0x27
#define NCV7430_FRAME_ID_SET_INTENSITY_SHORT_WRITING     0x2E
#define NCV7430_FRAME_ID_SET_COLOR_SHORT_WRITING         0x2F
#define NCV7430_FRAME_ID_SET_SECONDARY_CAL_PARAM_WRITING 0x36

/* Standard LIN frame identifiers */
#define NCV7430_COMMANDER_COMMAND  0x3C
#define NCV7430_RESPONDER_RESPONSE 0x3D

/* NCV7430 data lengths */
#define NCV7430_8_BYTES 8
#define NCV7430_4_BYTES 4
#define NCV7430_2_BYTES 2

/* NCV7430 broad bit values */
#define NCV7430_GROUP_ADDRESSING      0
#define NCV7430_INDIVIDUAL_ADDRESSING 1

/* AppCMD value is always 0x80 */
#define NCV7430_APPCMD 0x80

/* For filling unused payload bytes. Unused bytes must be set as 1 for NCV7430 */
#define NCV7430_FILL 0xff

/* Mask helpers */
#define NCV7430_6_BITS_MASK   0x3F
#define NCV7430_AD_MASK       0xC0
#define NCV7430_BIT_7_MASK    0x80
#define NCV7430_BIT_6_MASK    0x40
#define NCV7430_BITS_1_3_MASK 0x0E
#define NCV7430_BITS_0_3_MASK BIT_MASK(4)
#define NCV7430_BITS_0_5_MASK BIT_MASK(6)

/* NCV7430 LIN node address */
#define NCV7430_NODE_ADDRESS 0x00

/* NCV7430 LED enable mask definitions */
#define NCV7430_LED_R_EN_MSK BIT(0)
#define NCV7430_LED_G_EN_MSK BIT(1)
#define NCV7430_LED_B_EN_MSK BIT(2)

/* NCV7430 LED modulation frequency definitions */
#define NCV7430_LED_MOD_FREQ_122_HZ 0
#define NCV7430_LED_MOD_FREQ_244_HZ 1

/* RGB LED color structure */
struct rgb_led_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

/* NCV7430 API function prototypes */
int ncv7430_init(const struct device *dev, uint8_t addr);
int ncv7430_set_led_color(const struct device *dev, uint8_t addr, struct rgb_led_color *color);
int ncv7430_get_led_color(const struct device *dev, uint8_t addr, struct rgb_led_color *color);

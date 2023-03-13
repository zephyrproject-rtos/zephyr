/*
 * Copyright (C) 2022 Ryan Walker <info@interruptlabs.ca>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define IQS5XX_FW_FILE_LEN  64
#define IQS5XX_NUM_RETRIES  10
#define IQS5XX_NUM_CONTACTS 5
#define IQS5XX_WR_BYTES_MAX 2

#define IQS5XX_PROD_NUM_IQS550 40
#define IQS5XX_PROD_NUM_IQS572 58
#define IQS5XX_PROD_NUM_IQS525 52

#define IQS5XX_SHOW_RESET BIT(7)
#define IQS5XX_ACK_RESET  BIT(7)

#define IQS5XX_SUSPEND BIT(0)
#define IQS5XX_RESUME  0

#define IQS5XX_SETUP_COMPLETE BIT(6)
#define IQS5XX_WDT	      BIT(5)
#define IQS5XX_ALP_REATI      BIT(3)
#define IQS5XX_REATI	      BIT(2)

#define IQS5XX_TP_EVENT	     BIT(2)
#define IQS5XX_GESTURE_EVENT BIT(1)
#define IQS5XX_EVENT_MODE    BIT(0)

#define IQS5XX_FLIP_X BIT(0)
#define IQS5XX_FLIP_Y BIT(1)

#define IQS5XX_PROD_NUM		  0x0000
#define IQS5XX_GEST_EV0		  0x000D
#define IQS5XX_GEST_EV1		  0x000E
#define IQS5XX_SYS_INFO0	  0x000F
#define IQS5XX_SYS_INFO1	  0x0010
#define IQS5XX_NUM_FINGERS	  0x0011
#define IQS5XX_SYS_CTRL0	  0x0431
#define IQS5XX_SYS_CTRL1	  0x0432
#define IQS5XX_REPORT_LP1	  0x0580
#define IQS5XX_REPORT_LP2	  0x0582
#define IQS5XX_TO_ACTIVE	  0x0584
#define IQS5XX_TO_IDLE_TCH	  0x0585
#define IQS5XX_TO_IDLE		  0x0586
#define IQS5XX_TO_LP1		  0x0587
#define IQS5XX_SYS_CFG0		  0x058E
#define IQS5XX_SYS_CFG1		  0x058F
#define IQS5XX_TOTAL_RX		  0x063D
#define IQS5XX_TOTAL_TX		  0x063E
#define IQS5XX_XY_CONFIG	  0x0669
#define IQS5XX_X_RES		  0x066E
#define IQS5XX_Y_RES		  0x0670
#define IQS5XX_EXP_FILE		  0x0677
#define IQS5XX_SINGLE_FINGER_GEST 0x06b7
#define IQS5XX_MULTI_FINGER_GEST  0x06b8
#define IQS5XX_TAP_TIME		  0x06b9
#define IQS5XX_CHKSM		  0x83C0
#define IQS5XX_APP		  0x8400
#define IQS5XX_CSTM		  0xBE00
#define IQS5XX_PMAP_END		  0xBFFF
#define IQS5XX_END_COMM		  0xEEEE

#define IQS5XX_CHKSM_LEN (IQS5XX_APP - IQS5XX_CHKSM)
#define IQS5XX_APP_LEN	 (IQS5XX_CSTM - IQS5XX_APP)
#define IQS5XX_CSTM_LEN	 (IQS5XX_PMAP_END + 1 - IQS5XX_CSTM)
#define IQS5XX_PMAP_LEN	 (IQS5XX_PMAP_END + 1 - IQS5XX_CHKSM)

#define IQS5XX_REC_HDR_LEN   4
#define IQS5XX_REC_LEN_MAX   255
#define IQS5XX_REC_TYPE_DATA 0x00
#define IQS5XX_REC_TYPE_EOF  0x01

#define IQS5XX_BL_ADDR_MASK   0x40
#define IQS5XX_BL_CMD_VER     0x00
#define IQS5XX_BL_CMD_READ    0x01
#define IQS5XX_BL_CMD_EXEC    0x02
#define IQS5XX_BL_CMD_CRC     0x03
#define IQS5XX_BL_BLK_LEN_MAX 64
#define IQS5XX_BL_ID	      0x0200
#define IQS5XX_BL_STATUS_NONE 0xEE
#define IQS5XX_BL_CRC_PASS    0x00
#define IQS5XX_BL_CRC_FAIL    0x01
#define IQS5XX_BL_ATTEMPTS    3

#define IQS5XX_SWIPE_Y_NEG  0x20
#define IQS5XX_SWIPE_Y_POS  0x10
#define IQS5XX_SWIPE_X_POS  0x08
#define IQS5XX_SWIPE_X_NEG  0x04
#define IQS5XX_TAP_AND_HOLD 0x02
#define IQS5XX_SINGLE_TAP   0x01

#define IQS5XX_ZOOM	      0x04
#define IQS5XX_SCROLL	      0x02
#define IQS5XX_TWO_FINGER_TAP 0x01

struct iqs5xx_touch_data {
	uint16_t abs_x;
	uint16_t abs_y;
	uint16_t touch_str;
	uint8_t touch_area;
} __packed;

struct iqs5xx_regmap {
	uint8_t gesture_event[2];
	uint8_t sys_info[2];
	uint8_t num_fin;
	uint16_t rel_x;
	uint16_t rel_y;
	struct iqs5xx_touch_data touch_data[IQS5XX_NUM_CONTACTS];
} __packed;

struct iqs5xx_data {
	struct gpio_callback gpio_cb;
	const struct device *dev;
	struct iqs5xx_regmap regmap;
	struct k_work work;
};

struct iqs5xx_dev_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec rdy_gpio;
};

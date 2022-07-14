/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3194_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3194_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct is31fl3194_bitwise {
	uint8_t bit0 : 1;
	uint8_t bit1 : 1;
	uint8_t bit2 : 1;
	uint8_t bit3 : 1;
	uint8_t bit4 : 1;
	uint8_t bit5 : 1;
	uint8_t bit6 : 1;
	uint8_t bit7 : 1;
};

#define IS31FL3194_NUM_LEDS_MAX	    3

#define IS31FL3194_ID 0xCEU

#define IS31FL3194_PRODUCT_ID	  0x00U
#define IS31FL3194_OPERATING_CONF 0x01U
struct is31fl3194_ops_conf {
	uint8_t ssd : 1;
	uint8_t rgb : 2;
	uint8_t unused_01 : 1;
	uint8_t out : 3;
	uint8_t unused_02 : 1;
};

#define IS31FL3194_OUTPUT_ENABLE 0x02U
struct is31fl3194_out_en {
	uint8_t en : 3;
	uint8_t unused_01 : 5;
};

#define IS31FL3194_CURRENT_BAND 0x03U
struct is31fl3194_cbx {
	uint8_t cb1 : 2;
	uint8_t cb2 : 2;
	uint8_t cb3 : 2;
	uint8_t unused_01 : 2;
};

#define IS31FL3194_OUT1_CURRENT_LEVEL 0x10U
#define IS31FL3194_OUT2_CURRENT_LEVEL 0x21U
#define IS31FL3194_OUT3_CURRENT_LEVEL 0x32U
struct is31fl3194_outx_cl {
	uint8_t cl : 8;
};

#define IS31FL3194_COLOR_UPDATE 0x40U
#define IS31FL3194_RESET	0x4FU

union is31fl3194_reg {
	struct is31fl3194_ops_conf ops_conf;
	struct is31fl3194_out_en out_en;
	struct is31fl3194_cbx cbx;
	struct is31fl3194_outx_cl outx_cl;
	struct is31fl3194_bitwise bitwise;
	uint8_t byte;
};

struct is31fl3194_regs {
	struct is31fl3194_ops_conf ops_conf;
	struct is31fl3194_out_en out_en;
	struct is31fl3194_cbx cbx;
	struct is31fl3194_outx_cl outx_cl[IS31FL3194_NUM_LEDS_MAX];
};

typedef int32_t (*is31fl3194_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*is31fl3194_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

struct is31fl3194_ctx {
	/** Component mandatory fields **/
	is31fl3194_write_ptr write_reg;
	is31fl3194_read_ptr read_reg;
	struct is31fl3194_regs *regs;
	/** Customizable optional pointer **/
	void *handle;
};

int is31fl3194_product_id_get(const struct is31fl3194_ctx *ctx, uint8_t *val);

enum is31fl3194_ssd {
	IS31FL3194_SW_SHUTDOWN_MODE = 0,
	IS31FL3194_NORMAL_OPERATION = 1,
};
int is31fl3194_ops_ssd_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_ssd val);
void is31fl3194_ops_ssd_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_ssd *val);

enum is31fl3194_out {
	IS31FL3194_OUT1 = 0b00,
	IS31FL3194_OUT2 = 0b01,
	IS31FL3194_OUT3 = 0b10,
};

enum is31fl3194_outx_en {
	IS31FL3194_OUT_DISABLE = 0b0,
	IS31FL3194_OUT_ENABLE = 0b1,
};
int is31fl3194_out_en_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			  enum is31fl3194_outx_en val);
int is31fl3194_out_en_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			  enum is31fl3194_outx_en *val);

enum is31fl3194_current_band {
	IS31FL3194_CURRENT_BAND_1 = 0b00,
	IS31FL3194_CURRENT_BAND_2 = 0b01,
	IS31FL3194_CURRENT_BAND_3 = 0b10,
	IS31FL3194_CURRENT_BAND_4 = 0b11,
};
int is31fl3194_current_band_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
				enum is31fl3194_current_band val);
int is31fl3194_current_band_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
				enum is31fl3194_current_band *val);

int is31fl3194_outx_cl_set(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			   struct is31fl3194_outx_cl value);
int is31fl3194_outx_cl_get(const struct is31fl3194_ctx *ctx, enum is31fl3194_out out,
			   struct is31fl3194_outx_cl *value);

#define IS31FL3194_UPDATE 0xC5U

int is31fl3194_color_update(const struct is31fl3194_ctx *ctx);

int is31fl3194_reset(const struct is31fl3194_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3194_H_ */

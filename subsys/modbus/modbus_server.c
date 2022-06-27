/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on mbs_core.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded Modbus Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <modbus_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modbus_s, CONFIG_MODBUS_LOG_LEVEL);

/*
 * This functions are used to reset and update server's
 * statistics and communications counters.
 */
#ifdef CONFIG_MODBUS_FC08_DIAGNOSTIC
void modbus_reset_stats(struct modbus_context *ctx)
{
	/* Initialize all MODBUS event counters. */
	ctx->mbs_msg_ctr = 0;
	ctx->mbs_crc_err_ctr = 0;
	ctx->mbs_except_ctr = 0;
	ctx->mbs_server_msg_ctr = 0;
	ctx->mbs_noresp_ctr = 0;
}

static void update_msg_ctr(struct modbus_context *ctx)
{
	ctx->mbs_msg_ctr++;
}

static void update_crcerr_ctr(struct modbus_context *ctx)
{
	ctx->mbs_crc_err_ctr++;
}

static void update_excep_ctr(struct modbus_context *ctx)
{
	ctx->mbs_except_ctr++;
}

static void update_server_msg_ctr(struct modbus_context *ctx)
{
	ctx->mbs_server_msg_ctr++;
}

static void update_noresp_ctr(struct modbus_context *ctx)
{
	ctx->mbs_noresp_ctr++;
}
#else
#define modbus_reset_stats(...)
#define update_msg_ctr(...)
#define update_crcerr_ctr(...)
#define update_excep_ctr(...)
#define update_server_msg_ctr(...)
#define update_noresp_ctr(...)
#endif /* CONFIG_MODBUS_FC08_DIAGNOSTIC */

/*
 * This function sets the indicated error response code into the response frame.
 * Then the routine is called to calculate the error check value.
 */
static void mbs_exception_rsp(struct modbus_context *ctx, uint8_t excep_code)
{
	const uint8_t excep_bit = BIT(7);

	LOG_INF("FC 0x%02x Error 0x%02x", ctx->rx_adu.fc, excep_code);

	update_excep_ctr(ctx);

	ctx->tx_adu.fc |= excep_bit;
	ctx->tx_adu.data[0] = excep_code;
	ctx->tx_adu.length = 1;
}

/*
 * FC 01 (0x01) Read Coils
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Coils     2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Byte count            1 Bytes
 *  Coil status           N * 1 Byte
 */
static bool mbs_fc01_coil_read(struct modbus_context *ctx)
{
	const uint16_t coils_limit = 2000;
	const uint8_t request_len = 4;
	uint8_t *presp;
	bool coil_state;
	int err;
	uint16_t coil_addr;
	uint16_t coil_qty;
	uint16_t num_bytes;
	uint8_t bit_mask;
	uint16_t coil_cntr;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length");
		return false;
	}

	if (ctx->mbs_user_cb->coil_rd == NULL) {
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	coil_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	coil_qty = sys_get_be16(&ctx->rx_adu.data[2]);

	/* Make sure we don't exceed the allowed limit per request */
	if (coil_qty == 0 || coil_qty > coils_limit) {
		LOG_ERR("Number of coils limit exceeded");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	/* Calculate byte count for response. */
	num_bytes = ((coil_qty - 1) / 8) + 1;
	/* Number of data bytes + byte count. */
	ctx->tx_adu.length = num_bytes + 1;
	/* Set number of data bytes in response message. */
	ctx->tx_adu.data[0] = (uint8_t)num_bytes;

	/* Clear bytes in response */
	presp = &ctx->tx_adu.data[1];
	memset(presp, 0, num_bytes);

	/* Reset the pointer to the start of the response payload */
	presp = &ctx->tx_adu.data[1];
	/* Start with bit 0 in response byte data mask. */
	bit_mask = BIT(0);
	/* Initialize loop counter. */
	coil_cntr = 0;

	/* Loop through each coil requested. */
	while (coil_cntr < coil_qty) {

		err = ctx->mbs_user_cb->coil_rd(coil_addr, &coil_state);
		if (err != 0) {
			LOG_INF("Coil address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}

		if (coil_state) {
			*presp |= bit_mask;
		}

		coil_addr++;
		/* Increment coil counter. */
		coil_cntr++;
		/* Determine if 8 data bits have been filled. */
		if ((coil_cntr % 8) == 0) {
			/* Reset the data mask. */
			bit_mask = BIT(0);
			/* Increment frame data index. */
			presp++;
		} else {
			/*
			 * Still in same data byte, so shift the data mask
			 * to the next higher bit position.
			 */
			bit_mask <<= 1;
		}
	}

	return true;
}

/*
 * FC 02 (0x02) Read Discrete Inputs
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Inputs    2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Byte count            1 Bytes
 *  Input status           N * 1 Byte
 */
static bool mbs_fc02_di_read(struct modbus_context *ctx)
{
	const uint16_t di_limit = 2000;
	const uint8_t request_len = 4;
	uint8_t *presp;
	bool di_state;
	int err;
	uint16_t di_addr;
	uint16_t di_qty;
	uint16_t num_bytes;
	uint8_t bit_mask;
	uint16_t di_cntr;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length");
		return false;
	}

	if (ctx->mbs_user_cb->discrete_input_rd == NULL) {
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	di_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	di_qty = sys_get_be16(&ctx->rx_adu.data[2]);

	/* Make sure we don't exceed the allowed limit per request */
	if (di_qty == 0 || di_qty > di_limit) {
		LOG_ERR("Number of inputs limit exceeded");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	/* Get number of bytes needed for response. */
	num_bytes = ((di_qty - 1) / 8) + 1;
	/* Number of data bytes + byte count. */
	ctx->tx_adu.length = num_bytes + 1;
	/* Set number of data bytes in response message. */
	ctx->tx_adu.data[0] = (uint8_t)num_bytes;

	/* Clear bytes in response */
	presp = &ctx->tx_adu.data[1];
	for (di_cntr = 0; di_cntr < num_bytes; di_cntr++) {
		*presp++ = 0x00;
	}

	/* Reset the pointer to the start of the response payload */
	presp = &ctx->tx_adu.data[1];
	/* Start with bit 0 in response byte data mask. */
	bit_mask = BIT(0);
	/* Initialize loop counter. */
	di_cntr = 0;

	/* Loop through each DI requested. */
	while (di_cntr < di_qty) {

		err = ctx->mbs_user_cb->discrete_input_rd(di_addr, &di_state);
		if (err != 0) {
			LOG_INF("Discrete input address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}

		if (di_state) {
			*presp |= bit_mask;
		}

		di_addr++;
		/* Increment DI counter. */
		di_cntr++;
		/* Determine if 8 data bits have been filled. */
		if ((di_cntr % 8) == 0) {
			/* Reset the data mask. */
			bit_mask = BIT(0);
			/* Increment data frame index. */
			presp++;
		} else {
			/*
			 * Still in same data byte, so shift the data mask
			 * to the next higher bit position.
			 */
			bit_mask <<= 1;
		}
	}

	return true;
}

/*
 * 03 (0x03) Read Holding Registers
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Registers 2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Byte count            1 Bytes
 *  Register Value        N * 2 Byte
 */
static bool mbs_fc03_hreg_read(struct modbus_context *ctx)
{
	const uint16_t regs_limit = 125;
	const uint8_t request_len = 4;
	uint8_t *presp;
	uint16_t err;
	uint16_t reg_addr;
	uint16_t reg_qty;
	uint16_t num_bytes;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length");
		return false;
	}

	reg_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	reg_qty = sys_get_be16(&ctx->rx_adu.data[2]);

	if ((reg_addr < MODBUS_FP_EXTENSIONS_ADDR) ||
	    !IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
		/* Read integer register */
		if (ctx->mbs_user_cb->holding_reg_rd == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > regs_limit) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		/* Get number of bytes needed for response. */
		num_bytes = (uint8_t)(reg_qty * sizeof(uint16_t));
	} else {
		/* Read floating-point register */
		if (ctx->mbs_user_cb->holding_reg_rd_fp == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > (regs_limit / 2)) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		/* Get number of bytes needed for response. */
		num_bytes = (uint8_t)(reg_qty * sizeof(float));
	}

	/* Number of data bytes + byte count. */
	ctx->tx_adu.length = num_bytes + 1;
	/* Set number of data bytes in response message. */
	ctx->tx_adu.data[0] = (uint8_t)num_bytes;

	/* Reset the pointer to the start of the response payload */
	presp = &ctx->tx_adu.data[1];
	/* Loop through each register requested. */
	while (reg_qty > 0) {
		if (reg_addr < MODBUS_FP_EXTENSIONS_ADDR) {
			uint16_t reg;

			/* Read integer register */
			err = ctx->mbs_user_cb->holding_reg_rd(reg_addr, &reg);
			if (err == 0) {
				sys_put_be16(reg, presp);
				presp += sizeof(uint16_t);
			}

		} else if (IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
			float fp;
			uint32_t reg;

			/* Read floating-point register */
			err = ctx->mbs_user_cb->holding_reg_rd_fp(reg_addr, &fp);
			if (err == 0) {
				memcpy(&reg, &fp, sizeof(reg));
				sys_put_be32(reg, presp);
				presp += sizeof(uint32_t);
			}
		}

		if (err != 0) {
			LOG_INF("Holding register address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}

		/* Increment current register address */
		reg_addr++;
		reg_qty--;
	}

	return true;
}

/*
 * 04 (0x04) Read Input Registers
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Registers 2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Byte count            1 Bytes
 *  Register Value        N * 2 Byte
 */
static bool mbs_fc04_inreg_read(struct modbus_context *ctx)
{
	const uint16_t regs_limit = 125;
	const uint8_t request_len = 4;
	uint8_t *presp;
	int err;
	uint16_t reg_addr;
	uint16_t reg_qty;
	uint16_t num_bytes;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length");
		return false;
	}

	reg_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	reg_qty = sys_get_be16(&ctx->rx_adu.data[2]);

	if ((reg_addr < MODBUS_FP_EXTENSIONS_ADDR) ||
	    !IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
		/* Read integer register */
		if (ctx->mbs_user_cb->input_reg_rd == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > regs_limit) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		/* Get number of bytes needed for response. */
		num_bytes = (uint8_t)(reg_qty * sizeof(uint16_t));
	} else {
		/* Read floating-point register */
		if (ctx->mbs_user_cb->input_reg_rd_fp == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > (regs_limit / 2)) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		/* Get number of bytes needed for response. */
		num_bytes = (uint8_t)(reg_qty * sizeof(float));
	}

	/* Number of data bytes + byte count. */
	ctx->tx_adu.length = num_bytes + 1;
	/* Set number of data bytes in response message. */
	ctx->tx_adu.data[0] = (uint8_t)num_bytes;

	/* Reset the pointer to the start of the response payload */
	presp = &ctx->tx_adu.data[1];
	/* Loop through each register requested. */
	while (reg_qty > 0) {
		if (reg_addr < MODBUS_FP_EXTENSIONS_ADDR) {
			uint16_t reg;

			/* Read integer register */
			err = ctx->mbs_user_cb->input_reg_rd(reg_addr, &reg);
			if (err == 0) {
				sys_put_be16(reg, presp);
				presp += sizeof(uint16_t);
			}

		} else if (IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
			float fp;
			uint32_t reg;

			/* Read floating-point register */
			err = ctx->mbs_user_cb->input_reg_rd_fp(reg_addr, &fp);
			if (err == 0) {
				memcpy(&reg, &fp, sizeof(reg));
				sys_put_be32(reg, presp);
				presp += sizeof(uint32_t);
			}
		}

		if (err != 0) {
			LOG_INF("Input register address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}

		/* Increment current register number */
		reg_addr++;
		reg_qty--;
	}

	return true;
}

/*
 * FC 05 (0x05) Write Single Coil
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Output Address        2 Bytes
 *  Output Value          2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Output Address        2 Bytes
 *  Output Value          2 Bytes
 */
static bool mbs_fc05_coil_write(struct modbus_context *ctx)
{
	const uint8_t request_len = 4;
	const uint8_t response_len = 4;
	int err;
	uint16_t coil_addr;
	uint16_t coil_val;
	bool coil_state;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length %u", ctx->rx_adu.length);
		return false;
	}

	if (ctx->mbs_user_cb->coil_wr == NULL) {
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	/* Get the desired coil address and coil value */
	coil_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	coil_val = sys_get_be16(&ctx->rx_adu.data[2]);

	/* See if coil needs to be OFF? */
	if (coil_val == MODBUS_COIL_OFF_CODE) {
		coil_state = false;
	} else {
		coil_state = true;
	}

	err = ctx->mbs_user_cb->coil_wr(coil_addr, coil_state);

	if (err != 0) {
		LOG_INF("Coil address not supported");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
		return true;
	}

	/* Assemble response payload */
	ctx->tx_adu.length = response_len;
	sys_put_be16(coil_addr, &ctx->tx_adu.data[0]);
	sys_put_be16(coil_val, &ctx->tx_adu.data[2]);

	return true;
}

/*
 * 06 (0x06) Write Single Register
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Register Address      2 Bytes
 *  Register Value        2 Bytes
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Register Address      2 Bytes
 *  Register Value        2 Bytes
 */
static bool mbs_fc06_hreg_write(struct modbus_context *ctx)
{
	const uint8_t request_len = 4;
	const uint8_t response_len = 4;
	int err;
	uint16_t reg_addr;
	uint16_t reg_val;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length %u", ctx->rx_adu.length);
		return false;
	}

	if (ctx->mbs_user_cb->holding_reg_wr == NULL) {
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	reg_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	reg_val = sys_get_be16(&ctx->rx_adu.data[2]);

	err = ctx->mbs_user_cb->holding_reg_wr(reg_addr, reg_val);

	if (err != 0) {
		LOG_INF("Register address not supported");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
		return true;
	}

	/* Assemble response payload */
	ctx->tx_adu.length = response_len;
	sys_put_be16(reg_addr, &ctx->tx_adu.data[0]);
	sys_put_be16(reg_val, &ctx->tx_adu.data[2]);

	return true;
}

/*
 * 08 (0x08) Diagnostics
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Sub-function code     2 Bytes
 *  Data                  N * 2 Byte
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Sub-function code     2 Bytes
 *  Data                  N * 2 Byte
 */
#ifdef CONFIG_MODBUS_FC08_DIAGNOSTIC
static bool mbs_fc08_diagnostics(struct modbus_context *ctx)
{
	const uint8_t request_len = 4;
	const uint8_t response_len = 4;
	uint16_t sfunc;
	uint16_t data;

	if (ctx->rx_adu.length != request_len) {
		LOG_ERR("Wrong request length %u", ctx->rx_adu.length);
		return false;
	}

	sfunc = sys_get_be16(&ctx->rx_adu.data[0]);
	data = sys_get_be16(&ctx->rx_adu.data[2]);

	switch (sfunc) {
	case MODBUS_FC08_SUBF_QUERY:
		/* Sub-function 0x00 return Query Data */
		break;

	case MODBUS_FC08_SUBF_CLR_CTR:
		/* Sub-function 0x0A clear Counters and Diagnostic */
		modbus_reset_stats(ctx);
		break;

	case MODBUS_FC08_SUBF_BUS_MSG_CTR:
		/* Sub-function 0x0B return Bus Message Count */
		data = ctx->mbs_msg_ctr;
		break;

	case MODBUS_FC08_SUBF_BUS_CRC_CTR:
		/* Sub-function 0x0C return Bus Communication Error Count */
		data = ctx->mbs_crc_err_ctr;
		break;

	case MODBUS_FC08_SUBF_BUS_EXCEPT_CTR:
		/* Sub-function 0x0D return Bus Exception Error Count */
		data = ctx->mbs_except_ctr;
		break;

	case MODBUS_FC08_SUBF_SERVER_MSG_CTR:
		/* Sub-function 0x0E return Server Message Count */
		data = ctx->mbs_server_msg_ctr;
		break;

	case MODBUS_FC08_SUBF_SERVER_NO_RESP_CTR:
		/* Sub-function 0x0F return Server No Response Count */
		data = ctx->mbs_noresp_ctr;
		break;

	default:
		LOG_INF("Sub-function not supported");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	/* Assemble response payload */
	ctx->tx_adu.length = response_len;
	sys_put_be16(sfunc, &ctx->tx_adu.data[0]);
	sys_put_be16(data, &ctx->tx_adu.data[2]);

	return true;
}
#else
static bool mbs_fc08_diagnostics(struct modbus_context *ctx)
{
	mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);

	return true;
}
#endif

/*
 * FC 15 (0x0F) Write Multiple Coils
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Outputs   2 Bytes
 *  Byte Count            1 Byte
 *  Outputs Value         N * 1 Byte
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Outputs   2 Bytes
 */
static bool mbs_fc15_coils_write(struct modbus_context *ctx)
{
	const uint16_t coils_limit = 2000;
	const uint8_t request_len = 6;
	const uint8_t response_len = 4;
	uint8_t temp = 0;
	int err;
	uint16_t coil_addr;
	uint16_t coil_qty;
	uint16_t num_bytes;
	uint16_t coil_cntr;
	uint8_t data_ix;
	bool coil_state;

	if (ctx->rx_adu.length < request_len) {
		LOG_ERR("Wrong request length %u", ctx->rx_adu.length);
		return false;
	}

	if (ctx->mbs_user_cb->coil_wr == NULL) {
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		return true;
	}

	coil_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	coil_qty = sys_get_be16(&ctx->rx_adu.data[2]);
	/* Get the byte count for the data. */
	num_bytes = ctx->rx_adu.data[4];

	if (coil_qty == 0 || coil_qty > coils_limit) {
		LOG_ERR("Number of coils limit exceeded");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	/* Be sure byte count is valid for quantity of coils. */
	if (((((coil_qty - 1) / 8) + 1) !=  num_bytes) ||
	    (ctx->rx_adu.length  != (num_bytes + 5))) {
		LOG_ERR("Mismatch in the number of coils");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	coil_cntr = 0;
	/* The 1st coil data byte is 6th element in payload */
	data_ix = 5;
	/* Loop through each coil to be forced. */
	while (coil_cntr < coil_qty) {
		/* Move to the next data byte after every eight bits. */
		if ((coil_cntr % 8) == 0) {
			temp = ctx->rx_adu.data[data_ix++];
		}

		if (temp & BIT(0)) {
			coil_state = true;
		} else {
			coil_state = false;
		}

		err = ctx->mbs_user_cb->coil_wr(coil_addr + coil_cntr,
						coil_state);

		if (err != 0) {
			LOG_INF("Coil address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}

		/* Shift the data one bit position * to the right. */
		temp >>= 1;
		/* Increment the COIL counter. */
		coil_cntr++;
	}

	/* Assemble response payload */
	ctx->tx_adu.length = response_len;
	sys_put_be16(coil_addr, &ctx->tx_adu.data[0]);
	sys_put_be16(coil_qty, &ctx->tx_adu.data[2]);

	return true;
}

/*
 * FC16 (0x10) Write Multiple registers
 *
 * Request Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Registers 2 Bytes
 *  Byte Count            1 Byte
 *  Registers Value       N * 1 Byte
 *
 * Response Payload:
 *  Function code         1 Byte
 *  Starting Address      2 Bytes
 *  Quantity of Registers 2 Bytes
 *
 * If the address of the request exceeds or is equal to MODBUS_FP_EXTENSIONS_ADDR,
 * then the function would write to multiple 'floating-point' according to
 * the 'Daniels Flow Meter' extensions.  This means that each register
 * requested is considered as a 32-bit IEEE-754 floating-point format.
 */
static bool mbs_fc16_hregs_write(struct modbus_context *ctx)
{
	const uint16_t regs_limit = 125;
	const uint8_t request_len = 6;
	const uint8_t response_len = 4;
	uint8_t *prx_data;
	int err;
	uint16_t reg_addr;
	uint16_t reg_qty;
	uint16_t num_bytes;
	uint8_t reg_size;

	if (ctx->rx_adu.length < request_len) {
		LOG_ERR("Wrong request length %u", ctx->rx_adu.length);
		return false;
	}

	reg_addr = sys_get_be16(&ctx->rx_adu.data[0]);
	reg_qty = sys_get_be16(&ctx->rx_adu.data[2]);
	/* Get the byte count for the data. */
	num_bytes = ctx->rx_adu.data[4];

	if ((reg_addr < MODBUS_FP_EXTENSIONS_ADDR) ||
	    !IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
		/* Write integer register */
		if (ctx->mbs_user_cb->holding_reg_wr == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > regs_limit) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		reg_size = sizeof(uint16_t);
	} else {
		/* Write floating-point register */
		if (ctx->mbs_user_cb->holding_reg_wr_fp == NULL) {
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
			return true;
		}

		if (reg_qty == 0 || reg_qty > (regs_limit / 2)) {
			LOG_ERR("Number of registers limit exceeded");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
			return true;
		}

		reg_size = sizeof(float);
	}

	/* Compare number of bytes and payload length */
	if ((ctx->rx_adu.length - 5) != num_bytes) {
		LOG_ERR("Mismatch in the number of bytes");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	if ((num_bytes / reg_qty) != (uint16_t)reg_size) {
		LOG_ERR("Mismatch in the number of registers");
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_VAL);
		return true;
	}

	/* The 1st registers data byte is 6th element in payload */
	prx_data = &ctx->rx_adu.data[5];

	for (uint16_t reg_cntr = 0; reg_cntr < reg_qty; reg_cntr++) {
		uint16_t addr = reg_addr + reg_cntr;

		if ((reg_addr < MODBUS_FP_EXTENSIONS_ADDR) ||
		    !IS_ENABLED(CONFIG_MODBUS_FP_EXTENSIONS)) {
			uint16_t reg_val = sys_get_be16(prx_data);

			prx_data += sizeof(uint16_t);
			err = ctx->mbs_user_cb->holding_reg_wr(addr, reg_val);
		} else {
			uint32_t reg_val = sys_get_be32(prx_data);
			float fp;

			/* Write to floating point register */
			memcpy(&fp, &reg_val, sizeof(float));
			prx_data += sizeof(uint32_t);
			err = ctx->mbs_user_cb->holding_reg_wr_fp(addr, fp);
		}

		if (err != 0) {
			LOG_INF("Register address not supported");
			mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_DATA_ADDR);
			return true;
		}
	}

	/* Assemble response payload */
	ctx->tx_adu.length = response_len;
	sys_put_be16(reg_addr, &ctx->tx_adu.data[0]);
	sys_put_be16(reg_qty, &ctx->tx_adu.data[2]);

	return true;
}

bool modbus_server_handler(struct modbus_context *ctx)
{
	bool send_reply = false;
	uint8_t addr = ctx->rx_adu.unit_id;
	uint8_t fc = ctx->rx_adu.fc;

	LOG_DBG("Server RX handler %p", ctx);
	update_msg_ctr(ctx);

	if (ctx->rx_adu_err != 0) {
		update_noresp_ctr(ctx);
		if (ctx->rx_adu_err == -EIO) {
			update_crcerr_ctr(ctx);
		}

		return false;
	}

	if (addr != 0 && addr != ctx->unit_id) {
		update_noresp_ctr(ctx);
		return false;
	}

	/* Prepare response header */
	ctx->tx_adu.trans_id = ctx->rx_adu.trans_id;
	ctx->tx_adu.proto_id = ctx->rx_adu.proto_id;
	ctx->tx_adu.unit_id = addr;
	ctx->tx_adu.fc = fc;

	update_server_msg_ctr(ctx);

	switch (fc) {
	case MODBUS_FC01_COIL_RD:
		send_reply = mbs_fc01_coil_read(ctx);
		break;

	case MODBUS_FC02_DI_RD:
		send_reply = mbs_fc02_di_read(ctx);
		break;

	case MODBUS_FC03_HOLDING_REG_RD:
		send_reply = mbs_fc03_hreg_read(ctx);
		break;

	case MODBUS_FC04_IN_REG_RD:
		send_reply = mbs_fc04_inreg_read(ctx);
		break;

	case MODBUS_FC05_COIL_WR:
		send_reply = mbs_fc05_coil_write(ctx);
		break;

	case MODBUS_FC06_HOLDING_REG_WR:
		send_reply = mbs_fc06_hreg_write(ctx);
		break;

	case MODBUS_FC08_DIAGNOSTICS:
		send_reply = mbs_fc08_diagnostics(ctx);
		break;

	case MODBUS_FC15_COILS_WR:
		send_reply = mbs_fc15_coils_write(ctx);
		break;

	case MODBUS_FC16_HOLDING_REGS_WR:
		send_reply = mbs_fc16_hregs_write(ctx);
		break;

	default:
		LOG_ERR("Function code 0x%02x not implemented", fc);
		mbs_exception_rsp(ctx, MODBUS_EXC_ILLEGAL_FC);
		send_reply = true;
		break;
	}

	if (addr == 0) {
		/* Broadcast address, do not reply */
		send_reply = false;
	}

	if (send_reply == false) {
		update_noresp_ctr(ctx);
	}

	return send_reply;
}

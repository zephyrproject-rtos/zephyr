/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on mbm_core.c from uC/Modbus Stack.
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
#include <sys/byteorder.h>
#include <mb_rtu_internal.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mb_rtu_c, CONFIG_MODBUS_RTU_LOG_LEVEL);

static int mbm_validate_response_fc(struct mb_rtu_context *ctx,
				    const uint8_t node_addr,
				    uint8_t fc)
{
	uint8_t resp_fc = ctx->rx_frame.fc;
	uint8_t excep_code = ctx->rx_frame.data[0];
	const uint8_t excep_bit = BIT(7);
	const uint8_t excep_mask = BIT_MASK(7);

	if (node_addr != ctx->rx_frame.addr) {
		return -EIO;
	}

	if (fc != (resp_fc & excep_mask)) {
		return -EIO;
	}

	if (resp_fc & excep_bit) {
		if (excep_code > MODBUS_EXC_NONE) {
			return excep_code;
		}

		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_MODBUS_RTU_FP_EXTENSIONS
static int mbm_validate_fc03fp_response(struct mb_rtu_context *ctx, float *ptbl)
{
	uint8_t resp_byte_cnt;
	uint8_t req_byte_cnt;
	uint16_t req_qty;
	uint8_t *resp_data;

	resp_byte_cnt = ctx->rx_frame.data[0];
	resp_data = &ctx->rx_frame.data[1];
	req_qty = sys_get_be16(&ctx->tx_frame.data[2]);
	req_byte_cnt = req_qty * sizeof(float);

	if (req_byte_cnt != resp_byte_cnt) {
		LOG_ERR("Mismatch in the number of registers");
		return -EINVAL;
	}

	for (uint16_t i = 0; i < req_qty; i++) {
		uint32_t reg_val = sys_get_be32(resp_data);

		memcpy(&ptbl[i], &reg_val, sizeof(float));
		resp_data += sizeof(uint32_t);
	}

	return 0;
}
#endif

static int mbm_validate_rd_response(struct mb_rtu_context *ctx,
				    const uint8_t node_addr,
				    uint8_t fc,
				    uint8_t *data)
{
	int err;
	uint8_t resp_byte_cnt;
	uint8_t req_byte_cnt;
	uint16_t req_qty;
	uint16_t req_addr;
	uint8_t *resp_data;
	uint16_t *data_p16 = (uint16_t *)data;

	if (data == NULL) {
		return -EINVAL;
	}

	resp_byte_cnt = ctx->rx_frame.data[0];
	resp_data = &ctx->rx_frame.data[1];
	req_qty = sys_get_be16(&ctx->tx_frame.data[2]);
	req_addr = sys_get_be16(&ctx->tx_frame.data[0]);

	switch (fc) {
	case MODBUS_FC01_COIL_RD:
	case MODBUS_FC02_DI_RD:
		req_byte_cnt = (uint8_t)((req_qty - 1) / 8) + 1;
		if (req_byte_cnt != resp_byte_cnt) {
			LOG_ERR("Mismatch in the number of coils or inputs");
			err = -EINVAL;
		} else {
			for (uint8_t i = 0; i < resp_byte_cnt; i++) {
				data[i] = resp_data[i];
			}

			err = 0;
		}
		break;

	case MODBUS_FC03_HOLDING_REG_RD:
		if (IS_ENABLED(CONFIG_MODBUS_RTU_FP_EXTENSIONS) &&
		    (req_addr >= MODBUS_RTU_FP_ADDR)) {
			err = mbm_validate_fc03fp_response(ctx, (float *)data);
			break;
		}
		/* fallthrough */
	case MODBUS_FC04_IN_REG_RD:
		req_byte_cnt = req_qty * sizeof(uint16_t);
		if (req_byte_cnt != resp_byte_cnt) {
			LOG_ERR("Mismatch in the number of registers");
			err = -EINVAL;
		} else {
			for (uint16_t i = 0; i < req_qty; i++) {
				data_p16[i] = sys_get_be16(resp_data);
				resp_data += sizeof(uint16_t);
			}

			err = 0;
		}
		break;

	default:
		LOG_ERR("Validation not implemented for FC 0x%02x", fc);
		err = -ENOTSUP;
	}

	return err;
}

static int mbm_validate_fc08_response(struct mb_rtu_context *ctx,
				      const uint8_t node_addr,
				      uint16_t *data)
{
	int err;
	uint16_t resp_sfunc;
	uint16_t resp_data;
	uint16_t req_sfunc;
	uint16_t req_data;

	if (data == NULL) {
		return -EINVAL;
	}

	req_sfunc = sys_get_be16(&ctx->tx_frame.data[0]);
	req_data = sys_get_be16(&ctx->tx_frame.data[2]);
	resp_sfunc = sys_get_be16(&ctx->rx_frame.data[0]);
	resp_data = sys_get_be16(&ctx->rx_frame.data[2]);

	if (req_sfunc != resp_sfunc) {
		LOG_ERR("Mismatch in the sub-function code");
		return -EINVAL;
	}

	switch (resp_sfunc) {
	case MODBUS_FC08_SUBF_QUERY:
	case MODBUS_FC08_SUBF_CLR_CTR:
		if (req_data != resp_data) {
			LOG_ERR("Request and response data are different");
			err = -EINVAL;
		} else {
			*data = resp_data;
			err = 0;
		}
		break;

	case MODBUS_FC08_SUBF_BUS_MSG_CTR:
	case MODBUS_FC08_SUBF_BUS_CRC_CTR:
	case MODBUS_FC08_SUBF_BUS_EXCEPT_CTR:
	case MODBUS_FC08_SUBF_SERVER_MSG_CTR:
	case MODBUS_FC08_SUBF_SERVER_NO_RESP_CTR:
		*data = resp_data;
		err = 0;
		break;

	default:
		err = -EINVAL;
	}

	return err;
}

static int mbm_validate_wr_response(struct mb_rtu_context *ctx,
				    const uint8_t node_addr,
				    uint8_t fc)
{
	int err;
	uint16_t req_addr;
	uint16_t req_value;
	uint16_t resp_addr;
	uint16_t resp_value;

	req_addr = sys_get_be16(&ctx->tx_frame.data[0]);
	req_value = sys_get_be16(&ctx->tx_frame.data[2]);
	resp_addr = sys_get_be16(&ctx->rx_frame.data[0]);
	resp_value = sys_get_be16(&ctx->rx_frame.data[2]);

	switch (fc) {
	case MODBUS_FC05_COIL_WR:
	case MODBUS_FC06_HOLDING_REG_WR:
	case MODBUS_FC15_COILS_WR:
	case MODBUS_FC16_HOLDING_REGS_WR:
		if (req_addr != resp_addr || req_value != resp_value) {
			err = ENXIO;
		} else {
			err = 0;
		}
		break;

	default:
		LOG_ERR("Validation not implemented for FC 0x%02x", fc);
		err = -ENOTSUP;
	}

	return err;
}

static int mbm_send_cmd(struct mb_rtu_context *ctx, const uint8_t node_addr,
			uint8_t fc, void *data)
{
	int err;

	ctx->tx_frame.addr = node_addr;
	ctx->tx_frame.fc = fc;

	mb_tx_frame(ctx);

	if (k_sem_take(&ctx->client_wait_sem, K_USEC(ctx->rxwait_to)) != 0) {
		LOG_WRN("Client wait-for-RX timeout");
		err = -EIO;
		goto exit_error;
	}

	if (ctx->rx_frame_err != 0) {
		err = ctx->rx_frame_err;
		goto exit_error;
	}

	err = mbm_validate_response_fc(ctx, node_addr, fc);
	if (err < 0) {
		LOG_ERR("Failed to validate address or function code");
		goto exit_error;
	} else if (err > 0) {
		LOG_INF("Modbus FC %u, error code %u", fc, err);
		goto exit_error;
	}

	switch (fc) {
	case MODBUS_FC01_COIL_RD:
	case MODBUS_FC02_DI_RD:
	case MODBUS_FC03_HOLDING_REG_RD:
	case MODBUS_FC04_IN_REG_RD:
		err = mbm_validate_rd_response(ctx, node_addr, fc, data);
		break;

	case MODBUS_FC08_DIAGNOSTICS:
		err = mbm_validate_fc08_response(ctx, node_addr, data);
		break;

	case MODBUS_FC05_COIL_WR:
	case MODBUS_FC06_HOLDING_REG_WR:
	case MODBUS_FC15_COILS_WR:
	case MODBUS_FC16_HOLDING_REGS_WR:
		err = mbm_validate_wr_response(ctx, node_addr, fc);
		break;

	default:
		LOG_ERR("FC 0x%02x not implemented", fc);
		err = -ENOTSUP;
	}

exit_error:
	return err;
}

int modbus_read_coils(const int iface,
		      const uint8_t node_addr,
		      const uint16_t start_addr,
		      uint8_t *const coil_tbl,
		      const uint16_t num_coils)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_coils, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC01_COIL_RD, coil_tbl);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_read_dinputs(const int iface,
			const uint8_t node_addr,
			const uint16_t start_addr,
			uint8_t *const di_tbl,
			const uint16_t num_di)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_di, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC02_DI_RD, di_tbl);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_read_holding_regs(const int iface,
			     const uint8_t node_addr,
			     const uint16_t start_addr,
			     uint16_t *const reg_buf,
			     const uint16_t num_regs)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_regs, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC03_HOLDING_REG_RD, reg_buf);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}


#ifdef CONFIG_MODBUS_RTU_FP_EXTENSIONS
int modbus_read_holding_regs_fp(const int iface,
			       const uint8_t node_addr,
			       const uint16_t start_addr,
			       float *const reg_buf,
			       const uint16_t num_regs)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_regs, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC03_HOLDING_REG_RD, reg_buf);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}
#endif

int modbus_read_input_regs(const int iface,
			   const uint8_t node_addr,
			   const uint16_t start_addr,
			   uint16_t *const reg_buf,
			   const uint16_t num_regs)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_regs, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, 4, reg_buf);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_write_coil(const int iface,
		      const uint8_t node_addr,
		      const uint16_t coil_addr,
		      const bool coil_state)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;
	uint16_t coil_val;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	if (coil_state == false) {
		coil_val = MODBUS_COIL_OFF_CODE;
	} else {
		coil_val = MODBUS_COIL_ON_CODE;
	}

	ctx->tx_frame.length = 4;
	sys_put_be16(coil_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(coil_val, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC05_COIL_WR, NULL);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_write_holding_reg(const int iface,
			     const uint8_t node_addr,
			     const uint16_t start_addr,
			     const uint16_t reg_val)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(reg_val, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC06_HOLDING_REG_WR, NULL);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_request_diagnostic(const int iface,
			      const uint8_t node_addr,
			      const uint16_t sfunc,
			      const uint16_t data,
			      uint16_t *const data_out)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	int err;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	ctx->tx_frame.length = 4;
	sys_put_be16(sfunc, &ctx->tx_frame.data[0]);
	sys_put_be16(data, &ctx->tx_frame.data[2]);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC08_DIAGNOSTICS, data_out);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_write_coils(const int iface,
		       const uint8_t node_addr,
		       const uint16_t start_addr,
		       uint8_t *const coil_tbl,
		       const uint16_t num_coils)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	uint8_t num_bytes;
	int err;
	uint8_t *data_ptr;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_coils, &ctx->tx_frame.data[2]);

	num_bytes = (uint8_t)(((num_coils - 1) / 8) + 1);
	ctx->tx_frame.data[4] = num_bytes;
	data_ptr = &ctx->tx_frame.data[5];
	ctx->tx_frame.length = 5 + num_bytes;

	memcpy(data_ptr, coil_tbl, num_bytes);

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC15_COILS_WR, NULL);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

int modbus_write_holding_regs(const int iface,
			      const uint8_t node_addr,
			      const uint16_t start_addr,
			      uint16_t *const reg_buf,
			      const uint16_t num_regs)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	uint8_t num_bytes;
	int err;
	uint8_t *data_ptr;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_regs, &ctx->tx_frame.data[2]);

	num_bytes = (uint8_t) (num_regs * sizeof(uint16_t));
	ctx->tx_frame.length = num_bytes + 5;
	ctx->tx_frame.data[4] = num_bytes;
	data_ptr = &ctx->tx_frame.data[5];

	for (uint16_t i = 0; i < num_regs; i++) {
		sys_put_be16(reg_buf[i], data_ptr);
		data_ptr += sizeof(uint16_t);
	}

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC16_HOLDING_REGS_WR, NULL);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}

#ifdef CONFIG_MODBUS_RTU_FP_EXTENSIONS
int modbus_write_holding_regs_fp(const int iface,
				 const uint8_t node_addr,
				 const uint16_t start_addr,
				 float *const reg_buf,
				 const uint16_t num_regs)
{
	struct mb_rtu_context *ctx = mb_get_context(iface);
	uint8_t num_bytes;
	int err;
	uint8_t *data_ptr;

	if (ctx == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&ctx->iface_lock, K_FOREVER);

	sys_put_be16(start_addr, &ctx->tx_frame.data[0]);
	sys_put_be16(num_regs, &ctx->tx_frame.data[2]);

	num_bytes = (uint8_t) (num_regs * sizeof(float));
	ctx->tx_frame.length = num_bytes + 5;
	ctx->tx_frame.data[4] = num_bytes;
	data_ptr = &ctx->tx_frame.data[5];

	for (uint16_t i = 0; i < num_regs; i++) {
		uint32_t reg_val;

		memcpy(&reg_val, &reg_buf[i], sizeof(reg_val));
		sys_put_be32(reg_val, data_ptr);
		data_ptr += sizeof(uint32_t);
	}

	err = mbm_send_cmd(ctx, node_addr, MODBUS_FC16_HOLDING_REGS_WR, NULL);
	k_mutex_unlock(&ctx->iface_lock);

	return err;
}
#endif

/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _I3C_SEDI_H_
#define _I3C_SEDI_H_

#define I3C_DEVICE_NUM_MAX          8
#define I3C_MAX_IBI_PAYLOAD_LEN     (4*7)

enum {
	I3C_REGISTER_IBI = 1,
	I3C_GET_DEV_IDX,
	I3C_SET_SPEED,
	I3C_GET_SPEED_TYPE,
	I3C_READ_SLAVE_INFO,
};

typedef enum {
	I3C_SENSOR_TYPE_DYNAMIC = 0,
	I3C_SENSOR_TYPE_STATIC,
	I3C_SENSOR_TYPE_I2C_LEGACY,
} i3c_sensor_type_t;

typedef enum {
	I3C_SPEED_AUTO = 0,
	I3C_SPEED_100KHZ,
	I3C_SPEED_400KHZ,
	I3C_SPEED_1MHZ,
	I3C_SPEED_2MHZ,
	I3C_SPEED_4MHZ,
	I3C_SPEED_6MHZ,
	I3C_SPEED_8MHZ,
	I3C_SPEED_10MHZ,
	I3C_SPEED_INVALID = 255,
} i3c_speed_type;

typedef void (*sc_i3c_ibi_cb_t)(uint8_t bus, uint8_t addr_index, void *cb_arg);

struct i3c_ibi_param {
	uint8_t bus;
	struct i3c_device_desc *target;
	sc_i3c_ibi_cb_t ibi_cb;
	uint32_t cookie;
	void *cb_arg;
};

struct i3c_ibi_req {
	struct i3c_device_desc *target;
	uint32_t    cookie;
	uint8_t     len;
	uint8_t    *payload;
};

typedef struct {
	uint8_t         dev_type;
	uint8_t         dyn_addr;
	uint8_t         static_addr;
} i3c_sensor_info;

typedef struct {
	sc_i3c_ibi_cb_t ibi_cb;
	uint32_t        cookie;
	int8_t          ibi_len;
	uint8_t         ibi_payload[I3C_MAX_IBI_PAYLOAD_LEN];
	bool            ibi_enabled;
	uint8_t         bcr;
	i3c_sensor_info sensor;
	struct i3c_device_desc *i3c;
	void            *cb_arg;
} i3c_slave_device;

typedef struct {
	struct i3c_device_desc *target;
	uint8_t         dev_idx;
} i3c_get_dev_index;

typedef struct {
	uint8_t         dev_type;
	void            *target;
	i3c_speed_type  speed;
} i3c_speed_info;

#define I3C_MSG_IMM_COMBO	BIT(5)
#define I3C_MSG_I2C_TRAN	BIT(6)

static inline int i3c_immediate_write(struct i3c_device_desc *target, const uint8_t *buf,
			    uint32_t num_bytes)
{
	if (num_bytes > 4) {
		return -EIO;
	}
	struct i3c_msg msg = {0};

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_IMM_COMBO;

	return i3c_transfer(target, &msg, 1);
}

static inline int i3c_regular_write(struct i3c_device_desc *target, const uint8_t *buf,
			    uint32_t num_bytes)
{
	struct i3c_msg msg = {0};

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = 0;

	return i3c_transfer(target, &msg, 1);
}

static inline int i3c_regular_read(struct i3c_device_desc *target, const uint8_t *buf,
			    uint32_t num_bytes)
{
	struct i3c_msg msg = {0};

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = I3C_MSG_READ;

	return i3c_transfer(target, &msg, 1);
}

static inline int i3c_combo_read(struct i3c_device_desc *target, const uint8_t *buf,
			    uint32_t num_bytes)
{
	struct i3c_msg msg = {0};

	msg.buf = (uint8_t *)buf;
	msg.len = num_bytes;
	msg.flags = (I3C_MSG_READ | I3C_MSG_IMM_COMBO);

	return i3c_transfer(target, &msg, 1);
}

static inline int i3c_do_i2c_xfers(struct i3c_device_desc *target, const struct i2c_msg *i2c_xfers,
				uint8_t nxfers)
{
	struct i3c_msg msg[nxfers];
	int i;

	for (i = 0; i < nxfers; i++) {
		msg[i].buf = i2c_xfers[i].buf;
		msg[i].len = i2c_xfers[i].len;
		msg[i].flags = (i2c_xfers[i].flags & I2C_MSG_RW_MASK) ?
						I3C_MSG_READ : 0;
		msg[i].flags |= I3C_MSG_I2C_TRAN;
	}

	return i3c_transfer(target, msg, nxfers);
}

static inline int i3c_reg_write_data(struct i3c_device_desc *target,
				uint8_t reg_addr, uint8_t *buf,
				uint8_t n)
{
	uint8_t tx_buf[sizeof(reg_addr) + n];

	memcpy(tx_buf, &reg_addr, sizeof(reg_addr));
	memcpy(tx_buf + sizeof(reg_addr), buf, n);

	return i3c_write(target, tx_buf, sizeof(reg_addr) + n);
}

/* Append private to avoid name conflict with i3c_burst_write in i3c.h */
static inline int i3c_burst_write_private(struct i3c_device_desc *target,
				const uint8_t *start_addr,
				uint8_t addr_len,
				const uint8_t *buf,
				uint32_t num_bytes)
{
	struct i3c_msg msg[2] = {0};

	msg[0].buf = (uint8_t *)start_addr;
	msg[0].len = addr_len;
	msg[0].flags = I3C_MSG_IMM_COMBO;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I3C_MSG_IMM_COMBO;

	return i3c_transfer(target, msg, 2);
}

/* Append private to avoid name conflict with i3c_burst_write in i3c.h */
static inline int i3c_write_read_private(struct i3c_device_desc *target,
				const void *write_buf,
				size_t num_write,
				void *read_buf,
				size_t num_read)
{
	struct i3c_msg msg[2] = {0};

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I3C_MSG_IMM_COMBO;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I3C_MSG_READ;

	return i3c_transfer(target, msg, 2);
}

#endif /* _I3C_SEDI_H_*/

/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_CAN_H__
#define __INCLUDE_CAN_H__

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Controller Area Network Identifier structure
 *
 * bit 0-28	: CAN identifier (11/29 bit)
 * bit 29	: error message frame flag (0 = data frame, 1 = error message)
 * bit 30	: remote transmission request flag (1 = rtr frame)
 * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef u32_t canid_t;

#define CAN_EFF_FLAG	0x80000000U	/* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG	0x40000000U	/* remote transmission request */
#define CAN_ERR_FLAG	0x20000000U	/* error message frame */

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK	0x000007FFU	/* standard frame format (SFF) */
#define CAN_EFF_MASK	0x1FFFFFFFU	/* extended frame format (EFF) */
#define CAN_ERR_MASK	0x1FFFFFFFU	/* omit EFF, RTR, ERR flags */

#define CAN_MAX_DLEN	8

struct can_frame {
	canid_t	id;
	u8_t	dlc;
	u8_t	__pad[3];
	u8_t 	data[CAN_MAX_DLEN];
};

enum can_bitrate {
	CAN_BITRATE_10_KBIT  =   10000,
	CAN_BITRATE_20_KBIT  =   20000,
	CAN_BITRATE_50_KBIT  =   50000,
	CAN_BITRATE_125_KBIT =  125000,
	CAN_BITRATE_250_KBIT =  250000,
	CAN_BITRATE_500_KBIT =  500000,
	CAN_BITRATE_800_KBIT =  800000,
	CAN_BITRATE_1_MBIT   = 1000000,
};

enum can_mode {
	CAN_INIT_MODE,
	CAN_NORMAL_MODE,
	CAN_SLEEP_MODE,
	CAN_INT_LOOP_BACK_MODE,
	CAN_EXT_LOOP_BACK_MODE,
	CAN_BUS_MONITOR_MODE,
};

#define CAN_ERROR_BOF		0x00000001
#define CAN_ERROR_PASSIVE	0x00000002
#define CAN_ERROR_WARNING	0x00000004
#define CAN_ERROR_STUFF		0x00000008
#define CAN_ERROR_FORM		0x00000010
#define CAN_ERROR_ACK		0x00000020
#define CAN_ERROR_BIT_REC	0x00000040
#define CAN_ERROR_BIT_DOM	0x00000080
#define CAN_ERROR_CRC		0x00000100

struct can_error_counters {
	u32_t tec;
	u32_t rec;
};

struct can_filter {
	canid_t	id;
	canid_t	mask;
};

struct can_recv_cb {
	struct can_filter filter;
	void (*cb)(struct device *dev, struct can_frame *f);
	int fifo;
};

struct can_driver_api {
	int (*set_mode)(struct device *dev, enum can_mode mode);
	int (*set_bitrate)(struct device *dev, enum can_bitrate bitrate);
	int (*get_error_counters)(struct device *dev, struct can_error_counters *cntrs);

	int (*send)(struct device *dev, struct can_frame *f);
	int (*register_recv_callbacks)(struct device *dev, const struct can_recv_cb *callback_table, u32_t callback_table_size);
	int (*register_error_callback)(struct device *dev, void (*error_cb)(struct device *dev, u32_t error_flags));
};

static inline int can_set_mode(struct device *dev, enum can_mode mode)
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->set_mode)
		return api->set_mode(dev, mode);
	else
		return -ENOTSUP;
}

static inline int can_set_bitrate(struct device *dev, enum can_bitrate bitrate)
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->set_bitrate)
		return api->set_bitrate(dev, bitrate);
	else
		return -ENOTSUP;
}

static inline int can_get_error_counters(struct device *dev, struct can_error_counters *ec)
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->get_error_counters)
		return api->get_error_counters(dev, ec);
	else
		return -ENOTSUP;
}

static inline int can_send(struct device *dev, struct can_frame *f)
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->send)
		return api->send(dev, f);
	else
		return -ENOTSUP;
}

static inline int can_register_recv_callbacks(struct device *dev, const struct can_recv_cb *callback_table, u32_t callback_table_size)
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->register_recv_callbacks)
		return api->register_recv_callbacks(dev, callback_table, callback_table_size);
	else
		return -ENOTSUP;
}

static inline int can_register_error_callback(struct device *dev, void (*error_cb)(struct device *dev, u32_t error_hint))
{
	const struct can_driver_api *api = dev->driver_api;

	if (api->register_error_callback)
		return api->register_error_callback(dev, error_cb);
	else
		return -ENOTSUP;
}


#ifdef __cplusplus
}
#endif

#endif /*__INCLUDE_CAN_H__*/


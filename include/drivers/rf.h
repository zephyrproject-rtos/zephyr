/*
 * Copyright 2021 Leonard Pollak
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RF_H_
#define ZEPHYR_INCLUDE_DRIVERS_RF_H_

#include <zephyr/types.h>
#include <device.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TODO: different way to handle packets? e.g. network buffers?
 */
union rf_packet {
	uint8_t buffer[CONFIG_RF_MAX_PACKET_SIZE];
	struct {
		uint8_t length;
		uint8_t payload[CONFIG_RF_MAX_PACKET_SIZE - 1];
	} __packed;
};

/**
 * @typedef rf_freq_t
 * @brief Provides a type for setting/getting RF_DEVICE_FREQUENCY
 */

typedef uint64_t rf_freq_t;

/**
 * @typedef rf_chan_t
 * @brief Provides a type for setting/getting RF_DEVICE_CHANNEL
 */
typedef uint16_t rf_chan_t;

/**
 * @brief Provides a type for setting/getting RF_DEVICE_MODULATION_FORMAT
 */
enum rf_mod_format {
	/* On-off keying */
	RF_MODULATION_OOK = 0,
	/* Amplitude-shift keying */
	RF_MODULATION_ASK,
	/* 2-frequency-shift keying */
	RF_MODULATION_2FSK,
	/* 4-frequency-shift  keying */
	RF_MODULATION_4FSK,
	/* Gaussian frequency-shift keying */
	RF_MODULATION_GFSK,
	/* Audio frequency-shift keying */
	RF_MODULATION_AFSK,
	/* Binary-phase-shift keying */
	RF_MODULATION_2PSK,
	/* Minimum-shift keying/Offset quadrature phase-shift keying */
	RF_MODULATION_MSK,
	/* Gaussian minimum-shift keying */
	RF_MODULATION_GMSK,
	/* Direct-sequence spread spectrum */
	RF_MODULATION_DSSS,
};

/**
 * @typedef rf_baud_t
 * @brief Provides a type for setting/getting RF_DEVICE_BAUDRATE
 */
typedef uint32_t rf_baud_t;

/**
 * @typedef rf_power_t
 * @brief Provides a type for setting/getting RF_DEVICE_OUTPUT_POWER
 */
typedef int16_t rf_power_t;

/**
 * @brief RF device operating modes
 */
enum rf_op_mode {
	/* Perform (self-)calibration of the device */
	RF_MODE_CALIBRATE = 0,
	/* Power off the device */
	RF_MODE_POWER_OFF,
	/* Put the device to sleep */
	RF_MODE_SLEEP,
	/* Idle state of the rf device with shortest possible transistion time to RX/TX */
	RF_MODE_IDLE,
	/* Put device to low power state and activate RX on an event */
	RF_MODE_RX_WAKE_ON_EVENT,
	/* Put device to low power and periodically wake up to listen for incomnig data */
	RF_MODE_RX_WAKE_PERIODIC,
	/* Put the device into RX */
	RF_MODE_RX,
	/* Put the device into TX */
	RF_MODE_TX,
};

/**
 * @brief RF events
 */
enum rf_event {
	RF_EVENT_STUCK = 0,
	RF_EVENT_RECV_READY,
	RF_EVENT_SEND_READY,
	RF_EVENT_RECV_DONE,
	RF_EVENT_SEND_DONE,
	RF_EVENT_CHANNEL_CLEAR,
	RF_EVENT_WAKEUP,
};

/**
 * @brief RF hardware filter
 */
enum rf_hw_filter {
	RF_HW_FILTER_NONE = 0,
	RF_HW_FILTER_CRC8,
	RF_HW_FILTER_CRC16,
	RF_HW_FILTER_ADDR,
	RF_HW_FILTER_LENGTH,
};

/**
 * @brief RF device set/get arguments
 */
enum rf_device_arg {
	RF_DEVICE_FREQUENCY = 0,
	RF_DEVICE_CHANNEL,
	RF_DEVICE_MODULATION_FORMAT,
	RF_DEVICE_BAUDRATE,
	RF_DEVICE_OUTPUT_POWER,
	RF_DEVICE_OPERATING_MODE,
	RF_DEVICE_SET_EVENT_CB,
	RF_DEVICE_HW_FILTER,
	RF_DEVICE_SETTINGS,
	RF_DEVICE_CALIBRATION_SETTINGS,
	RF_DEVICE_POWER_TABLE,
};

/**
 * @typedef rf_event_cb_t
 * @brief Callback handler for RF events
 */
typedef void (*rf_event_cb_t)(const struct device *dev,
				enum rf_event event,
				void *event_params);

/**
 * @typedef rf_device_set_t
 * @brief API for setting device parameters
 */
typedef int (*rf_device_set_t)(const struct device *dev,
				    enum rf_device_arg cfg,
				    void *val);

/**
 * @typedef rf_device_get_t
 * @brief API for getting device parameters
 */
typedef int (*rf_device_get_t)(const struct device *dev,
				    enum rf_device_arg cfg,
				    void *val);

/**
 * @typedef rf_device_send_t
 * @brief API for synchronously sending data
 */
typedef int (*rf_device_send_t)(const struct device *dev,
				     union rf_packet *pkt);

/**
 * @typedef rf_device_recv_t
 * @brief API for synchronously receiving data
 */
typedef int (*rf_device_recv_t)(const struct device *dev,
				     union rf_packet *pkt);

/**
 * @brief Callback handler for RF wake event
 */
__subsystem struct rf_driver_api {
	rf_device_set_t device_set;
	rf_device_get_t device_get;
	rf_device_send_t send;
	rf_device_recv_t recv;
};

/**
 * @brief Set configuration parameter of the RF device
 *
 * LONG TEXT
 *
 * @param dev Pointer to the rf device
 * @param cfg
 * @param val
 *
 * @return 0 if successful, negative errno code if failure.
 */
//__syscall int rf_device_set(const struct device *rf,
//					enum rf_device_arg cfg,
//					void *val);
//
//static inline int z_impl_rf_device_set(const struct device *rf,
//					enum rf_device_arg cfg,
//					void *val)
static inline int rf_device_set(const struct device *rf,
					enum rf_device_arg cfg,
					void *val)
{
	const struct rf_driver_api *api =
		(const struct rf_driver_api *)rf->api;

	if (api->device_set== NULL) {
		return -ENOSYS;
	}

	return api->device_set(rf, cfg, val);
}

/**
 * @brief Get configuration parameter of the RF device
 *
 * LONG TEXT
 *
 * @param dev Pointer to the rf device
 * @param cfg
 * @param val
 *
 * @return 0 if successful, negative errno code if failure.
 */
//__syscall int rf_device_get(const struct device *rf,
//					enum rf_device_arg cfg,
//					void *val);
//
//static inline int z_impl_rf_device_get(const struct device *rf,
//					enum rf_device_arg cfg,
//					void *val)
static inline int rf_device_get(const struct device *rf,
					enum rf_device_arg cfg,
					void *val)
{
	const struct rf_driver_api *api =
		(const struct rf_driver_api *)rf->api;

	if (api->device_get == NULL) {
		return -ENOSYS;
	}

	return api->device_get(rf, cfg, val);
}

/**
 * @brief Send a packet
 *
 * LONG TEXT
 *
 * @param dev Pointer to the rf device
 * @param pkt Pointer to the packet
 *
 * @return 0 if successful, negative errno code if failure.
 */
//__syscall int rf_send(const struct device *rf, union rf_packet *pkt);
//
//static inline int z_impl_rf_send(const struct device *rf, union rf_packet *pkt)
static inline int rf_send(const struct device *rf, union rf_packet *pkt)
{
	const struct rf_driver_api *api =
		(const struct rf_driver_api *)rf->api;

	return api->send(rf, pkt);
}

/**
 * @brief Receive a packet
 *
 * LONG TEXT
 *
 * @param dev Pointer to the rf device
 * @param pkt Pointer to the packet
 *
 * @return 0 if successful, negative errno code if failure.
 */
//__syscall int rf_recv(const struct device *rf, union rf_packet *pkt);
//
//static inline int z_impl_rf_recv(const struct device *rf, union rf_packet *pkt)
static inline int rf_recv(const struct device *rf, union rf_packet *pkt)
{
	const struct rf_driver_api *api =
		(const struct rf_driver_api *)rf->api;

	return api->recv(rf, pkt);
}

#ifdef __cplusplus
}
#endif

//#include <syscalls/rf.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_RF_H_ */
